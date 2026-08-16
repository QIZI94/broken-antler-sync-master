#include "payloadencryption.h"

#include <esp_crypto_lock.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>


static mbedtls_gcm_context mbedtlsContext;
static volatile bool mbedtlsInitialized = false;

bool EncryptedMessageFrame::decryptMessage(UniformMessage& decryptedMessage) const {
	
	int result = mbedtls_gcm_auth_decrypt(
		&mbedtlsContext,

		sizeof(decryptedMessage),

		(const uint8_t*) &sequence,
		sizeof(sequence),

		nullptr,
		0,

		tag,
		sizeof(tag),

		encrypted,

		(uint8_t*) &decryptedMessage
	);

	return result == 0;
}
EncryptedMessageFrame EncryptedMessageFrame::fromUnencrypted(uint64_t sequence, const UniformMessage& unencryptedMessage){
	EncryptedMessageFrame messageFrame;

	messageFrame.sequence = sequence;
	mbedtls_gcm_crypt_and_tag(
		&mbedtlsContext,
		MBEDTLS_GCM_ENCRYPT,
		sizeof(unencryptedMessage),

		(const uint8_t*) &messageFrame.sequence,
		sizeof(messageFrame.sequence),

		nullptr,
		0,

		(const uint8_t*) &unencryptedMessage,
		messageFrame.encrypted,

		sizeof(tag),
		messageFrame.tag
	);

	return messageFrame;
}


void EncryptedMessageFrame::SetGlobalEncryptionKey(const EncryptionKey& encryptionKey){
	if(mbedtlsInitialized == false){
		mbedtls_gcm_init(&mbedtlsContext);

	}
	
    mbedtls_gcm_setkey(
        &mbedtlsContext,
        MBEDTLS_CIPHER_ID_AES,
        encryptionKey,
        128
	);
}