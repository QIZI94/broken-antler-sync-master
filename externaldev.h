#ifndef EXTERNAL_DEVICES_H
#define EXTERNAL_DEVICES_H

#include <inttypes.h>

#include "uratmessaging.h"
class UARTMessageHandler{
private: // definitions
	using DeferredRepeatMask = uint16_t;
	using DeferredAtLeastOnceMask = uint8_t;
private: //constants
	static constexpr uint8_t DEFERRED_MESSAGES_COUNT = UniformMessage::MESSAGE_TYPES_COUNT - 1;
	static constexpr uint8_t DEFERRED_REPEAT_MASK = 0b11;
	static constexpr uint8_t DEFERRED_SPECIAL_MESSAGE_REPEAT = 2;
	static constexpr uint16_t DEFERRED_REPEAT_TIME_US = 1500;
public: // constants
	static constexpr uint8_t DEFERRED_DEFAULT_REPEAT_COUNT = 3;
	static constexpr uint8_t DEFERRED_SUCCESSFUL_STOP_REPEAT = 0;
public: // interface
	void begin() {}
	void end() {
		messageRepeatLastTime = 0;
		deferredMessageSendAndAckMask = 0x0000;
	}
	MessageReceptionState handleMessagesReception(UARTMessageDriver& driver);
	MessageTransmissionState handleMessagesTransmission(UARTMessageDriver& driver);
public: // member functions
	void sendDeferredMessage(UniformMessage::Type inType, uint8_t repeatCount = DEFERRED_DEFAULT_REPEAT_COUNT);
	void requestDeferredMessage(UniformMessage::Type inType, uint8_t repeatCount = DEFERRED_DEFAULT_REPEAT_COUNT);
	int16_t getTransmissionLatency() const {return transmissionLatency;}
	bool isInitialized() const {return initialized;}
private: // member functions
	bool buildMessage(UniformMessage& messageOut, UniformMessage::Type inType);
	void setDeferredRepeatCountMask(UniformMessage::Type msgType, uint8_t attemptCount);
	uint8_t getDeferredRepeatCount(UniformMessage::Type msgType);
	void setDeferredSendAtLeastOnce(UniformMessage::Type msgType);
	bool clearDeferredSendAtLeastOnce(UniformMessage::Type msgType);
private: // member variables
	//UniformMessage::MessageData deferredMessages[DEFERRED_MESSAGES_COUNT];
	uint32_t messageRepeatLastTime = 0;
	int16_t transmissionLatency = 0;
	DeferredRepeatMask deferredMessageSendAndAckMask = 0x0000;
	DeferredAtLeastOnceMask deferredSendAtLeastOnceMask = 0x00;
	UniformMessage::Type deferredRequestType = UniformMessage::Type::NONE;
	bool initialized = false;

};




inline MessageManager<UARTMessageDriver, UARTMessageHandler> uartMessageManager;

extern void initExternalDevices();
extern void communicateWithExternalDevices();

#endif