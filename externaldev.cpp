#include "externaldev.h"
#include "utils/crc.h"
#include "timer.h"
#include "wifimessaging.h"

#include <Arduino.h>
///====== High level UART Communication ======///

static constexpr uint8_t esp32c3Signature = 0xC3;

static uint8_t repeatTimesync = 10;

MessageReceptionState UARTMessageHandler::handleMessagesReception(UARTMessageDriver &driver) {
	UniformMessage receivedMessage;
	MessageReceptionState messageReceiveState = driver.receiveMessage(receivedMessage);
	if(messageReceiveState == MessageReceptionState::DONE){
		/*Serial.print("Received: ");
		Serial.println(int(receivedMessage.type));*/
		UniformMessage acknowledgeOrResponse = UniformMessage::Acknowledge{.acknowledgedMessage = receivedMessage.type};
		
		switch(receivedMessage.type){
			case UniformMessage::Type::NONE:
				break;
			case UniformMessage::Type::TIME_SYNC:
				setDeferredRepeatCountMask(UniformMessage::Type::TIME_SYNC, DEFERRED_SUCCESSFUL_STOP_REPEAT);
				if(repeatTimesync > 0){
					transmissionLatency = (transmissionLatency + (rtcNow() - receivedMessage.data.timeSync.newTime)) >> 1;
					repeatTimesync--;
					sendDeferredMessage(UniformMessage::Type::TIME_SYNC);
					if(repeatTimesync == 1){
						sendDeferredMessage(UniformMessage::Type::LATENCY);
						initialized = true;
					}
				}
				{
					int64_t timeDiff = rtcNow() - receivedMessage.data.timeSync.newTime;
					driver.sendMessage(acknowledgeOrResponse);
				
					
					Serial.print("\nARD_TIME:");
					Serial.println(receivedMessage.data.timeSync.newTime);
					Serial.print("ESP_TIME:");
					Serial.println(rtcNow());

					Serial.print("Lat: ");
					Serial.println(transmissionLatency);
					Serial.print("Diff: ");
					Serial.println(timeDiff);
				}
				
				
				//Serial.println("msg:TS");
				//setRTC(receivedMessage.data.timeSync.newTime);
				
				break;
			case UniformMessage::Type::ACKNOWLEDGE:
				
				//Serial.println("msg:ACK");
				setDeferredRepeatCountMask(receivedMessage.data.acknowledge.acknowledgedMessage, DEFERRED_SUCCESSFUL_STOP_REPEAT);
				break;
			case UniformMessage::Type::REQUEST:
				//Serial.print("msg:R: ");
				//Serial.println(int(receivedMessage.data.request.requestedMessageType));
				//acknowledge = requestHandler(receivedMessage.data.request.requestedMessageType);
				if(buildMessage(acknowledgeOrResponse, receivedMessage.data.request.requestedMessageType)){
					acknowledgeOrResponse.isResponse = true;
				}
			default:
				driver.sendMessage(acknowledgeOrResponse);
				if(receivedMessage.isResponse){
					setDeferredRepeatCountMask(UniformMessage::Type::REQUEST, DEFERRED_SUCCESSFUL_STOP_REPEAT);
				}
				switch (receivedMessage.type){
				
					case UniformMessage::Type::ALIVE:
						transmissionLatency = 0;
						repeatTimesync = 20;
						initialized = false;
						//Serial.println("msg:A");
						//sendDeferredMessage(UniformMessage::Type::TIME_SYNC);
						sendDeferredMessage(UniformMessage::Type::TIME_SYNC);
						break;
					case UniformMessage::Type::TIMED_EVENT:
						{
							uint32_t ardTimeAdjusted = receivedMessage.data.timedEvent.atTime + (transmissionLatency);
							int64_t timeDiff = rtcNow() - int64_t(ardTimeAdjusted);
							Serial.print("ARD TIME: ");
							Serial.println(ardTimeAdjusted);
							Serial.print("ESP TIME: ");
							Serial.println(rtcNow());
							Serial.print("ESP-ARD: ");
							Serial.println(timeDiff);
							Serial.println();

							wifiMessageHandler.sendDeferredMessage(receivedMessage);
						}

					default:
						break;
				}
				
		}
	}
	return messageReceiveState;
}

MessageTransmissionState UARTMessageHandler::handleMessagesTransmission(UARTMessageDriver &driver) {
	uint32_t microsNow = micros();
	bool shouldRepeat = (microsNow - messageRepeatLastTime) >= DEFERRED_REPEAT_TIME_US;
	if(shouldRepeat || deferredSendAtLeastOnceMask != 0){
		
		//Serial.println(deferredMessageSendAndAckMask, BIN);
		UniformMessage deferredMessage;
		for(uint8_t messageTypeIndex = 0; messageTypeIndex < DEFERRED_MESSAGES_COUNT; ++messageTypeIndex) {
			
			if(!clearDeferredSendAtLeastOnce(UniformMessage::Type(messageTypeIndex)) && !shouldRepeat){
				continue;
			}
			uint8_t repeatCount = getDeferredRepeatCount(UniformMessage::Type(messageTypeIndex));
			if(repeatCount == 0x00){
				continue;
			}
			else if(repeatCount < 3){
				//Serial.print("Re: ");
				//Serial.println(messageTypeIndex);
			}
			if(buildMessage(deferredMessage, UniformMessage::Type(messageTypeIndex))){
				setDeferredRepeatCountMask(UniformMessage::Type(messageTypeIndex), --repeatCount);
				driver.sendMessage(deferredMessage);
				messageRepeatLastTime = microsNow;
			}
			else {
				// no message filled
				setDeferredRepeatCountMask(UniformMessage::Type(messageTypeIndex), DEFERRED_SUCCESSFUL_STOP_REPEAT);
			}
   		}
		
	}
	return deferredMessageSendAndAckMask == 0 ? MessageTransmissionState::DONE : MessageTransmissionState::IN_PROGRESS;
}

void UARTMessageHandler::sendDeferredMessage(UniformMessage::Type inType, uint8_t repeatCount) {
	//deferredMessages[uint8_t(message.type)] = message.data;
	setDeferredRepeatCountMask(inType, repeatCount);
	setDeferredSendAtLeastOnce(inType);
}

void UARTMessageHandler::requestDeferredMessage(UniformMessage::Type inType, uint8_t repeatCount) {
	deferredRequestType = inType;
	sendDeferredMessage(UniformMessage::Type::REQUEST, repeatCount);
}

bool UARTMessageHandler::buildMessage(UniformMessage &messageOut, UniformMessage::Type inType) {
	bool assembled = true;

	uint32_t rtcNowWithLatency = rtcNow() + (transmissionLatency);
	switch (inType){
		case UniformMessage::Type::LATENCY:
			messageOut.data.latency.times_us = transmissionLatency;
			break;
		case UniformMessage::Type::REQUEST:
			if(deferredRequestType != UniformMessage::Type::NONE) {
				messageOut.data.request.requestedMessageType = deferredRequestType;
			}
			else {
				assembled = false;
			}
			break;
		case UniformMessage::Type::ALIVE:
			
			messageOut.data.alive = UniformMessage::Alive{.who = esp32c3Signature, .time = rtcNowWithLatency};
			break;
		case UniformMessage::Type::TIME_SYNC:
			messageOut.data.timeSync.newTime = rtcNowWithLatency;
			break;
		case UniformMessage::Type::TIMED_EVENT:
			messageOut.data.timedEvent.atTime = rtcNow();
			break;
		
		default:
			assembled = false;
			break;
	}
	if(assembled){
		messageOut.type = inType;
	}
	return assembled;
}

void UARTMessageHandler::setDeferredRepeatCountMask(UniformMessage::Type msgType, uint8_t attemptCount) {
	uint8_t shift = uint8_t(msgType) << 1;
	deferredMessageSendAndAckMask = (deferredMessageSendAndAckMask & ~(DEFERRED_REPEAT_MASK << shift)) | (attemptCount << shift);

}

uint8_t UARTMessageHandler::getDeferredRepeatCount(UniformMessage::Type msgType) {
	uint8_t shift = uint8_t(msgType) << 1;
	return (deferredMessageSendAndAckMask >> shift) & DEFERRED_REPEAT_MASK;
}

void UARTMessageHandler::setDeferredSendAtLeastOnce(UniformMessage::Type msgType) {
	deferredSendAtLeastOnceMask |= 0x01 << uint8_t(msgType);
}

bool UARTMessageHandler::clearDeferredSendAtLeastOnce(UniformMessage::Type msgType) {
	const uint8_t msgTypeMask = 0x01 << uint8_t(msgType);
	const bool wasSet = (deferredSendAtLeastOnceMask & msgTypeMask) != 0;
	deferredSendAtLeastOnceMask &= ~msgTypeMask;

	return wasSet;
}



void initExternalDevices() {
	uartMessageManager.begin();
	uartMessageManager.handler.sendDeferredMessage(UniformMessage::Type::ALIVE);
}

void communicateWithExternalDevices() {
	/*if(Serial1.available()){
		Serial.print("Start Size: ");
		Serial.println(Serial1.available());
	}*/
	
	//while(recivedMsg == false){
	bool messageReceived = false;
	//delay(1);
	MessageProcessingState processingState = {.messageReceptionState = MessageReceptionState::IDLE, .messageTransmissionState = MessageTransmissionState::IDLE};
	do{
		
		auto start = micros();
		
		/*if(Serial1.available()){
			pinMode(LED_BUILTIN, OUTPUT);
			digitalWrite(LED_BUILTIN, HIGH);

			//Serial.println("Receiving");
		}*/
		
		processingState = uartMessageManager.run();
		
		auto end = micros();
		/*if(processingState == MessageReceptionState::DONE){
			
			Serial.print("timing: ");
			Serial.println(end - start);
			digitalWrite(LED_BUILTIN, LOW);
			pinMode(LED_BUILTIN, INPUT);
			Serial.print("Size after: ");
			Serial.println(Serial1.available());
			Serial.println("---------------");
			Serial.flush();
			
		}*/
		
		
				//delay(100);

	}while(processingState || !uartMessageManager.handler.isInitialized());
	

	
}
