#include <Arduino.h>
#include "uratmessaging.h"
#include "utils/crc.h"

///====== Low level UART Communication ======///

MessageTransmissionState UARTMessageDriver::sendMessage(const UniformMessage &messageIn) {
	/*Serial.print("Type:" );
	Serial.println(int(messageIn.type));*/
	MessageFrame uniformMessage{.asFrame = {.sync = MessageFrame::Data::SYNC_FLAG}};
	uniformMessage.asFrame.sync = MessageFrame::Data::SYNC_FLAG;

	static_assert(sizeof(UniformMessage::data) == 5);
	memcpy(&uniformMessage.asFrame.data[0], &messageIn, sizeof(UniformMessage));
	uint16_t crc = crc16(uniformMessage.asFrame.data, sizeof(uniformMessage.asFrame.data));

	uniformMessage.asFrame.crc[0] = uint8_t(crc);
	uniformMessage.asFrame.crc[1] = uint8_t(crc>>8);
	Serial1.write('\n');
	Serial1.write((const uint8_t*)&uniformMessage, sizeof(uniformMessage));
	Serial1.flush();

	return MessageTransmissionState::DONE;
}


MessageReceptionState UARTMessageDriver::receiveMessage(UniformMessage &messageOut) {

	/*Serial.print("Here: ");
	Serial.println(Serial.available());*/
	uint32_t nowMicros = micros();
    while (Serial1.available())
    {
		
        uint8_t receivedByte = Serial1.read();

        // ---------------------------------------------------------------------
        // Waiting for SYNC
        // ---------------------------------------------------------------------
        if (receptionBufferIndex == 0)
        {
            if (receivedByte != MessageFrame::Data::SYNC_FLAG)
            {
                continue;
            }
			receptionTimeoutLastTime = nowMicros;
        }
		

        receptionBuffer.asBuffer[receptionBufferIndex++] = receivedByte;

        // ---------------------------------------------------------------------
        // Full message received
        // ---------------------------------------------------------------------
        while (receptionBufferIndex == sizeof(MessageFrame))
        {
			
			
            receptionBufferIndex = 0;
			
            // Validate CRC
            if (validateMessage(receptionBuffer.asFrame))
            {
				
				memcpy(&messageOut, receptionBuffer.asFrame.data, sizeof(UniformMessage));
				return MessageReceptionState::DONE;
                //return true;
            }

            // -----------------------------------------------------------------
            // CRC failed
            //
            // IMPORTANT:
            // Try to recover sync immediately instead of discarding everything.
            // This prevents desync when SYNC appears inside stream.
            // -----------------------------------------------------------------
            for (size_t searchIdx = 1; searchIdx < sizeof(MessageFrame); ++searchIdx)
            {
				//Serial.print("Here: ");
                if (receptionBuffer.asBuffer[searchIdx] == MessageFrame::Data::SYNC_FLAG)
                {
					size_t searchWindowSize = sizeof(MessageFrame) - searchIdx;
                    memmove(receptionBuffer.asBuffer, &receptionBuffer.asBuffer[searchIdx], searchWindowSize);
                    receptionBufferIndex = searchWindowSize;
					receptionTimeoutLastTime = nowMicros;
					break;
                }
            }
        }
    }
	if(receptionBufferIndex == 0){
		return MessageReceptionState::IDLE;
	}
	else if((nowMicros - receptionTimeoutLastTime) >= RECEPTION_TIMEOUT_US){
		receptionBufferIndex = 0;
		return MessageReceptionState::TIMED_OUT_OR_FAILED;
	}
	
	return MessageReceptionState::IN_PROGRESS;
}

bool UARTMessageDriver::validateMessage(const MessageFrame::Data &uniformMessageData) {
	uint16_t receivedCrc = (uniformMessageData.crc[1] << 8) | uniformMessageData.crc[0];
    uint16_t computedCrc = crc16(
		uniformMessageData.data,
		sizeof(uniformMessageData.data)
	);

    return receivedCrc == computedCrc;
}
