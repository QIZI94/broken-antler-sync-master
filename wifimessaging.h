#ifndef WIFI_MESSAGING_H
#define WIFI_MESSAGING_H
#include <inttypes.h>

#include "messagingdefs.h"

#include "uniformmessage.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

class WIFIMessageDriver{
private: // definitions

private: // constants
	static constexpr uint8_t BROADCAST_MASK[] = {
		0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF
	};
public: // constants
	static constexpr uint32_t MAX_DELAY_US = 4000;
	static constexpr uint32_t SINGLE_TRANSMISSION_DELAY_US = 600;

public: // member functions
	void feedMessage(UniformMessage message){
		lastMessage = message;
	}
public: // interface
	void begin();
	void end() {}
	MessageTransmissionState sendMessage(const UniformMessage& messageIn);
	MessageReceptionState receiveMessage(UniformMessage& messageOut);
private:
	
	UniformMessage lastMessage;
	uint64_t uniqueSenderSequence = 0;
};

class WIFIMessageHandler{
private: // definitions
	using DeferredRepeatMask = uint16_t;
	using DeferredAtLeastOnceMask = uint8_t;
	//using DeferredAtLeastOnceMask = uint8_t;
private: //constants
	static constexpr uint8_t DEFERRED_MESSAGES_COUNT = UniformMessage::MESSAGE_TYPES_COUNT - 1;
	static constexpr uint8_t DEFERRED_REPEAT_MASK = 0b11;
	static constexpr uint8_t DEFERRED_SPECIAL_MESSAGE_REPEAT = 2;
	static constexpr uint16_t DEFERRED_REPEAT_TIME_US = 1500;
public: // constants
	static constexpr uint8_t DEFERRED_DEFAULT_REPEAT_COUNT = 3;
	static constexpr uint8_t DEFERRED_SUCCESSFUL_STOP_REPEAT = 0;
public: // interface
	void begin();
	void end() {

	}
	MessageReceptionState handleMessagesReception(WIFIMessageDriver& driver);
	MessageTransmissionState handleMessagesTransmission(WIFIMessageDriver& driver);
public:
	void sendDeferredMessage(const UniformMessage& messageIn);

private: // member variables
	QueueHandle_t messageTransmissionQueue;
};

inline WIFIMessageDriver wifiMessageDriver;
inline WIFIMessageHandler wifiMessageHandler;


extern void initWifiMessageHandling();

#endif // WIFI_MESSAGING_H