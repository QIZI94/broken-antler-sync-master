#ifndef UNIFORM_MESSAGE_H
#define UNIFORM_MESSAGE_H

#define packet_struct struct __attribute__((packed))
packet_struct  UniformMessage{
	using TimeType = uint32_t;
	using LatencyType = int16_t;
	enum class Type : uint8_t{
		ACKNOWLEDGE,
		REQUEST,
		ALIVE,
		LATENCY,
		TIME_SYNC,
		TIMED_EVENT,
		CUSTOM_MESSAGE,

		NONE,

	};
	static constexpr uint8_t MESSAGE_TYPES_COUNT = uint8_t(Type::NONE) + 1;
	static constexpr uint8_t MESSAGE_DATA_BYTE_SIZE = sizeof(uint8_t) + sizeof(uint32_t);
	static constexpr uint8_t MESSAGE_BYTE_SIZE = sizeof(Type) + MESSAGE_DATA_BYTE_SIZE;


	packet_struct None{};
	
	packet_struct Request{
		Type requestedMessageType;
	};
	packet_struct Alive{
		uint8_t who;
		TimeType time;
	};

	packet_struct Latency{
		LatencyType times_us;
	};

	packet_struct TimeSync{
		TimeType newTime;
	};

	packet_struct TimedEvent{
		uint8_t event;
		TimeType atTime;
	};
	packet_struct CustomMessageData{
		uint8_t bytes[MESSAGE_DATA_BYTE_SIZE];
	};

	packet_struct Acknowledge{
		Type acknowledgedMessage;
	};

	union MessageData{
		
		None none;
		Alive alive;
		Latency latency;
		Request  request;
		TimeSync timeSync;
		TimedEvent timedEvent;
		Acknowledge acknowledge;

		uint8_t asBytes[MESSAGE_DATA_BYTE_SIZE];

		static constexpr uint8_t size() {
			return MESSAGE_DATA_BYTE_SIZE;
		}
	};

	constexpr UniformMessage() : data{.none = {}}, isResponse(false), type(Type::NONE){}
	constexpr UniformMessage(const MessageData& messageData, Type messageType, bool isResponse = false) : data(messageData), isResponse(isResponse), type(messageType){}
	constexpr UniformMessage(const Latency& latency, bool isResponse = false) : data{.latency = latency}, isResponse(isResponse), type(Type::LATENCY){}
	constexpr UniformMessage(const Request& request, bool isResponse = false) : data{.request = request}, isResponse(isResponse), type(Type::REQUEST){}
	constexpr UniformMessage(const Alive& alive, bool isResponse = false) : data{.alive = alive}, isResponse(isResponse), type(Type::ALIVE){}
	constexpr UniformMessage(const TimeSync& timeSync, bool isResponse = false) : data{.timeSync = timeSync}, isResponse(isResponse), type(Type::TIME_SYNC){}
	constexpr UniformMessage(const TimedEvent& timedEvent, bool isResponse = false) : data{.timedEvent = timedEvent}, isResponse(isResponse), type(Type::TIMED_EVENT){}
	constexpr UniformMessage(const Acknowledge& acknowledge, bool isResponse = false) : data{.acknowledge = acknowledge}, isResponse(isResponse), type(Type::ACKNOWLEDGE){}
	
	MessageData  data;
	struct{
		uint8_t isResponse	: 4;
		UniformMessage::Type type	: 4;
	};

	static constexpr uint8_t size() {
		return MESSAGE_BYTE_SIZE;
	}


};

#endif // UNIFORM_MESSAGE_H