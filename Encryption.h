#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <stdio.h>
#include <string>
#include <string.h>

#ifndef ENCRYPTION_H_
#define ENCRYPTION_H_

#define RSA_KEYLEN 2048
#define AES_KEYLEN 256
#define AES_ROUNDS 6

#define KEY_SERVER_PRIVATE 0
#define KEY_SERVER_PUB 	   1
#define KEY_CLIENT_PUB     2
#define KEY_AES            3
#define KEY_AES_IV         4

class Encryption {
public:
	Encryption();
	Encryption(unsigned char *remotePubKey, size_t remotePubKeyLen);
    ~Encryption();
    int EncryptAes(const unsigned char *msg, size_t msgLen, unsigned char **encMsg);
    int DecryptAes(unsigned char *encMsg, size_t encMsgLen, unsigned char **decMsg);
    int writeKeyToFile(FILE *fd, int key);
    int getRemotePubKey(unsigned char **pubKey);
    int setRemotePubKey(unsigned char *pubKey, size_t pubKeyLen);
    int getLocalPubKey(unsigned char **pubKey);
    int getLocalPrivateKey(unsigned char **privateKey);
    int getAesKey(unsigned char **aesKey);
    int setAesKey(unsigned char *aesKey, size_t aesKeyLen);
    int getAesIV(unsigned char **aesIV);
    int setAesIV(unsigned char *aesIV, size_t aesIVLen);
private:
	static EVP_PKEY *localKeyPair;
	EVP_PKEY *remotePubKey;

	EVP_CIPHER_CTX *EncryptRsaCtx;
	EVP_CIPHER_CTX *EncryptAesCtx;

	EVP_CIPHER_CTX *DecryptRsaCtx;
	EVP_CIPHER_CTX *DecryptAesCtx;

	unsigned char *aesKey;
	unsigned char *aesIV;

	int init();
	int genTestClientKey();
};

#endif /* ENCRYPTION_H_ */