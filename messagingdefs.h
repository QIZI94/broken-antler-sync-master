#ifndef MESSAGING_DEFS_H
#define MESSAGING_DEFS_H
enum class MessageReceptionState : uint8_t {
	IDLE,
	DONE,
	TIMED_OUT_OR_FAILED,
	IN_PROGRESS
};

enum class MessageTransmissionState : uint8_t {
	IDLE,
	DONE,
	TIMED_OUT_OR_FAILED,
	IN_PROGRESS
};

struct MessageProcessingState {
	MessageReceptionState messageReceptionState;
	MessageTransmissionState messageTransmissionState;

	operator bool() const{
		switch (messageReceptionState)
		{
			case MessageReceptionState::IDLE:
			case MessageReceptionState::DONE:
			case MessageReceptionState::TIMED_OUT_OR_FAILED:
				return false;
			default:
				break;
		}
		switch (messageTransmissionState){
			case MessageTransmissionState::IDLE:
			case MessageTransmissionState::DONE:
			case MessageTransmissionState::TIMED_OUT_OR_FAILED:
				return false;
			default:
				break;
		}
		return true;
	}
		
	operator MessageReceptionState() const {
		return messageReceptionState;
	}

	operator MessageTransmissionState() const {
		return messageTransmissionState;
	}
};

template<class MessageDriver, class MessageHandler>
struct MessageManager{
	
	void begin() {
		driver.begin();
		handler.begin();
	}

	MessageProcessingState run(){
		return {
			.messageReceptionState = handler.handleMessagesReception(driver),
			.messageTransmissionState = handler.handleMessagesTransmission(driver)
		};
	}

	void end() {
		driver.end();
		handler.end();
	}
	MessageDriver driver;
	MessageHandler handler;
};
#endif // MESSAGING_DEFS_H