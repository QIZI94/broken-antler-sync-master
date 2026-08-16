#ifndef UART_MESSAGING_H
#define UART_MESSAGING_H
#include <inttypes.h>

#include "messagingdefs.h"
#include "uniformmessage.h"

class UARTMessageDriver {
private: // definitions
	union MessageFrame{
		struct Data{
			static constexpr uint8_t SYNC_FLAG = 0x9B;
			// header
			uint8_t sync;
			// message type + data + terminator
			uint8_t data[sizeof(UniformMessage)];
			uint8_t crc[2];
		};

		Data asFrame;
		uint8_t asBuffer[sizeof(Data)];
	};

public: // constants
	static constexpr uint32_t RECEPTION_TIMEOUT_US = 1000;
public: // interface
	void begin() {}
	void end(){
		receptionTimeoutLastTime = 0;
		receptionBufferIndex = 0;
	}
	MessageTransmissionState sendMessage(const UniformMessage& messageIn);
	MessageReceptionState receiveMessage(UniformMessage& messageOut);
private: // member functions
	bool validateMessage(const MessageFrame::Data& uniformMessageData);
private: // member variables
	MessageFrame receptionBuffer{};
	uint32_t receptionTimeoutLastTime = 0;
	uint8_t receptionBufferIndex = 0;
};
#endif // UART_MESSAGING_H