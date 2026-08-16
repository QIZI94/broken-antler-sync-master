#include "wifimessaging.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>



#include "timer.h"

#include "payloadencryption.h"

//#include "animations.h"

struct Event{
	enum class Type : uint8_t{
		LED_ANIMATION,
		LED_AUDIOLINK_INIT,
		LED_AUDIOLINK_BASS
	};
	union{
		struct {
			Type type		: 4;
			uint8_t value	: 4;
		};
		uint8_t event;
	};
};

static constexpr EncryptedMessageFrame::EncryptionKey SECRET_ENCRYPTION_KEY_AES_128 = {
	0x10,0x22,0x33,0x44,
	0x55,0x66,0x77,0x88,
	0x99,0xaa,0xbb,0xcc,
	0xdd,0xee,0xff,0x01
};

static constexpr uint8_t esp32c3Signature = 0xC3;


void WIFIMessageDriver::begin(){

	WiFi.mode(WIFI_STA);

	ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));

	ESP_ERROR_CHECK(esp_now_init());

	esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MASK, 6);
    peer.channel = 0;
    peer.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

}


MessageTransmissionState WIFIMessageDriver::sendMessage(const UniformMessage& messageIn){
	
	if(messageIn.type == UniformMessage::Type::ALIVE){
		uniqueSenderSequence = uint64_t(-1);
		for(uint8_t i_repeat = 0; i_repeat < 3; ++i_repeat){
			UniformMessage alive = UniformMessage::Alive{.who =  messageIn.data.alive.who, .time = rtcNow() + SINGLE_TRANSMISSION_DELAY_US};
			EncryptedMessageFrame encryptedMessageFrame = EncryptedMessageFrame::fromUnencrypted(uniqueSenderSequence, alive);
			esp_now_send(BROADCAST_MASK, reinterpret_cast<const uint8_t*>(&encryptedMessageFrame), sizeof(EncryptedMessageFrame));
		}
		uniqueSenderSequence++;
		return MessageTransmissionState::DONE;
	}
	else if(uniqueSenderSequence == 0){
		++uniqueSenderSequence;
	}
	if(messageIn.type == UniformMessage::Type::TIME_SYNC){
		Serial.println("Time sync send");
	//MessageFrame messageFrame{.magicNumber = MAGIC_NUMBER, .message = messageIn};
		for(uint8_t i_repeat = 0; i_repeat < 3; ++i_repeat){
			UniformMessage timeSync = UniformMessage::TimeSync{.newTime = rtcNow() + SINGLE_TRANSMISSION_DELAY_US};
			EncryptedMessageFrame encryptedMessageFrame = EncryptedMessageFrame::fromUnencrypted(uniqueSenderSequence, timeSync);
			esp_now_send(BROADCAST_MASK, reinterpret_cast<const uint8_t*>(&encryptedMessageFrame), sizeof(EncryptedMessageFrame));
		}
		uniqueSenderSequence++;
	}
	else {
		EncryptedMessageFrame encryptedMessageFrame = EncryptedMessageFrame::fromUnencrypted(uniqueSenderSequence++, messageIn);
	//MessageFrame messageFrame{.magicNumber = MAGIC_NUMBER, .message = messageIn};
		for(uint8_t i_repeat = 0; i_repeat < 3; ++i_repeat){
			esp_now_send(BROADCAST_MASK, reinterpret_cast<const uint8_t*>(&encryptedMessageFrame), sizeof(EncryptedMessageFrame));
		}
	}

	
	
	//esp_now_send(BROADCAST_MASK, reinterpret_cast<uint8_t*>(&messageFrame), sizeof(MessageFrame));
	//esp_now_send(BROADCAST_MASK, reinterpret_cast<uint8_t*>(&messageFrame), sizeof(MessageFrame));
	return MessageTransmissionState::DONE;
}
MessageReceptionState WIFIMessageDriver::receiveMessage(UniformMessage& messageOut){
	messageOut = lastMessage;
	return MessageReceptionState::DONE;
}


void WIFIMessageHandler::begin(){
	messageTransmissionQueue = xQueueCreate(4, sizeof(UniformMessage));
}


MessageReceptionState WIFIMessageHandler::handleMessagesReception(WIFIMessageDriver &driver)
{
	using MessageType = UniformMessage::Type;
	UniformMessage message;
	driver.receiveMessage(message);

	switch (message.type) {
		case MessageType::ALIVE:
			//setRTC(message.data.alive.time);
			break;
		case MessageType::TIME_SYNC:
			// sync rtc here
			//setRTC(message.data.timeSync.newTime);
			break;
		case MessageType::TIMED_EVENT:
			//handleEvent(Event{.event = message.data.timedEvent.event}, message.data.timedEvent.atTime);
			
			// set animation
			break;
			
		case MessageType::LATENCY:
		case MessageType::REQUEST:
		case MessageType::CUSTOM_MESSAGE:
		case MessageType::NONE:
			break;
	}

	return MessageReceptionState::DONE;
}

MessageTransmissionState WIFIMessageHandler::handleMessagesTransmission(WIFIMessageDriver &driver)
{
	UniformMessage messageIn;
	BaseType_t result = xQueueReceive(messageTransmissionQueue, &messageIn, portMAX_DELAY);
	if (result == pdTRUE){
		driver.sendMessage(messageIn);
		return MessageTransmissionState::DONE;
	}

	return MessageTransmissionState::IDLE;
}

void WIFIMessageHandler::sendDeferredMessage(const UniformMessage &messageIn) {
	
	if(xQueueSend(messageTransmissionQueue, &messageIn, 0) != pdTRUE) {
    	// Queue full: remove oldest packet
		UniformMessage dummy;
		xQueueReceive(messageTransmissionQueue, &dummy, 0);

		// Try again
		xQueueSend(messageTransmissionQueue, &messageIn, 0);
	}
}



static void onWifiMessageReceive_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
	if (UniformMessage::MESSAGE_BYTE_SIZE == len) return;

	wifiMessageDriver.feedMessage(*reinterpret_cast<const UniformMessage*>(data));
	wifiMessageHandler.handleMessagesReception(wifiMessageDriver);
	
	// memcpy((void*)&latest, data, sizeof(Packet));
	// newData = true;
}

static void wifiMessageTransmission_callback(void*){
	while(1){
		wifiMessageHandler.handleMessagesTransmission(wifiMessageDriver);
	}
}

void initWifiMessageHandling() {
	Serial.print("Size: ");
	Serial.println(UniformMessage::MESSAGE_BYTE_SIZE);
	EncryptedMessageFrame::SetGlobalEncryptionKey(SECRET_ENCRYPTION_KEY_AES_128);
	// dry run
	{
		UniformMessage dryMessage;
		EncryptedMessageFrame dryFrame = EncryptedMessageFrame::fromUnencrypted(0, dryMessage);
		(void)dryFrame.decryptMessage(dryMessage);
	}

	wifiMessageDriver.begin();
	wifiMessageHandler.begin();

	xTaskCreate(
		wifiMessageTransmission_callback,
		"wifiMessageTransmission_callback",
		4096,      // Stack size in bytes on ESP32 Arduino
		nullptr,
		1,         // Priority
		nullptr
	);

	UniformMessage aliveToSend = UniformMessage::Alive{.who = esp32c3Signature, .time = rtcNow()};
	wifiMessageHandler.sendDeferredMessage(aliveToSend);
	Event timedEventFlags{.type = Event::Type::LED_AUDIOLINK_INIT, .value = 0};

	Serial.print("Event Type: ");
	Serial.print(int(timedEventFlags.type));
	Serial.print(" Event Value: ");
	Serial.println(int(timedEventFlags.value));

	

	UniformMessage messageToSend = UniformMessage::TimedEvent{.event = timedEventFlags.event, .atTime = 0};
	pinMode(1, OUTPUT); 
	digitalWrite(1,HIGH);
	//delayMicroseconds(500);
	digitalWrite(1,LOW);
	wifiMessageHandler.sendDeferredMessage(messageToSend);
	//wifiMessageHandler.sendDeferredMessage(messageToSend);
	/*WiFi.mode(WIFI_STA);



	ESP_ERROR_CHECK(esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE));

	ESP_ERROR_CHECK(esp_now_init());
	esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastMac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Failed to add broadcast peer");
        while (true);
    }

    //esp_now_register_recv_cb(onWifiMessageReceive_callback);

	Event timedEventFlags{.type = Event::Type::LED_ANIMATION, .value = 0};
	

	UniformMessage messageToSend = UniformMessage::TimedEvent{.event = timedEventFlags.value, .atTime = 0};
	delay(1000);
	auto start = micros();
	auto end = micros();
	start = micros();
	esp_now_send(
        broadcastMac,
        reinterpret_cast<uint8_t*>(&messageToSend),
        sizeof(messageToSend));
	end = micros();
	
	Serial.print("wifi send took: ");
	Serial.println(end - start);*/
}