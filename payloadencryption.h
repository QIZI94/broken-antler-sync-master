#ifndef PAYLOAD_ENCRYPTION_H
#define PAYLOAD_ENCRYPTION_H


#include <inttypes.h>
#include <stddef.h>
#include "uniformmessage.h"

packet_struct EncryptedMessageFrame{
	using EncryptionKey = uint8_t[16];

	static constexpr size_t TAG_SIZE = 8;
	uint64_t sequence;
	uint8_t encrypted[sizeof(UniformMessage)];
	uint8_t tag[TAG_SIZE];

	bool decryptMessage(UniformMessage& decryptedMessage) const;

	static EncryptedMessageFrame fromUnencrypted(uint64_t sequence, const UniformMessage& unencryptedMessage);
	

	static void SetGlobalEncryptionKey(const EncryptionKey& encryptionKey);
};

#endif // PAYLOAD_ENCRYPTION_H