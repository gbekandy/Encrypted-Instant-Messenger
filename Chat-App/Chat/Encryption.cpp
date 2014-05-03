#include "Encryption.h"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "openssl/evp.h"

EVP_PKEY* Encryption::localKeyPair;

Encryption::Encryption() {
    localKeyPair = NULL;
    remotePubKey = NULL;

    #ifdef PSUEDO_CLIENT
        genTestClientKey();
    #endif

    init();
}

Encryption::Encryption(unsigned char *remotePubKey, size_t remotePubKeyLen) {
    localKeyPair = NULL;
    this->remotePubKey = NULL;

    setRemotePubKey(remotePubKey, remotePubKeyLen);
    init();
}

Encryption::~Encryption() {
    EVP_PKEY_free(remotePubKey);

    // EVP_CIPHER_CTX_cleanup(EncryptRsaCtx);
    EVP_CIPHER_CTX_cleanup(EncryptAesCtx);

    // EVP_CIPHER_CTX_cleanup(DecryptRsaCtx);
    EVP_CIPHER_CTX_cleanup(DecryptAesCtx);

    // free(EncryptRsaCtx);
    free(EncryptAesCtx);

    // free(DecryptRsaCtx);
    free(DecryptAesCtx);

    free(aesKey);
    free(aesIV);
}

int Encryption::EncryptAes(const unsigned char *msg, size_t msgLen, unsigned char **encMsg) {
    size_t blockLen = 0;
    size_t encMsgLen = 0;

    *encMsg = (unsigned char*) malloc(msgLen + AES_BLOCK_SIZE);
    if (encMsg == NULL) {
        return -1;
    }

    // setting up cipher context for AES CBC encryption
    if (!EVP_EncryptInit_ex(EncryptAesCtx, EVP_aes_256_cbc(), NULL, aesKey, aesIV)) {
        return -1;
    }

    // encrypts a message of msgLen from msg to encMsg. Number of bytes written in blockLen
    if (!EVP_EncryptUpdate(EncryptAesCtx, *encMsg, (int*)&blockLen, (unsigned char*)msg, msgLen)) {
        return -1;
    }
    encMsgLen += blockLen;

    // uses padding to finish off the remaining message to be encrypted
    if (!EVP_EncryptFinal_ex(EncryptAesCtx, *encMsg + encMsgLen, (int*)&blockLen)) {
        return -1;
    }

    EVP_CIPHER_CTX_cleanup(EncryptAesCtx);

    return encMsgLen + blockLen;
}

int Encryption::DecryptAes(unsigned char *encMsg, size_t encMsgLen, unsigned char **decMsg) {
    size_t decLen = 0;
    size_t blockLen = 0;

    *decMsg = (unsigned char*)malloc(encMsgLen);
    if (*decMsg == NULL) {
        return -1;
    }

    // setting up cipher context for AES CBC decryption
    if (!EVP_DecryptInit_ex(DecryptAesCtx, EVP_aes_256_cbc(), NULL, aesKey, aesIV)) {
        return -1;
    }

    // decrypts a message of encMsgLen from encMsg to decMsg. Number of bytes written in blockLen.
    if (!EVP_DecryptUpdate(DecryptAesCtx, (unsigned char*)*decMsg, (int*)&blockLen, encMsg, (int)encMsgLen)) {
        return -1;
    }
    decLen += blockLen;

    // uses padding to finish off remaining message to be decrypted
    if (!EVP_DecryptFinal_ex(DecryptAesCtx, (unsigned char*)*decMsg + decLen, (int*)&blockLen)) {
        return -1;
    }
    decLen += blockLen;

    EVP_CIPHER_CTX_cleanup(DecryptAesCtx);

    return (int)decLen;
}

// Initializes the keys that are going to be used
int Encryption::init() {
    // initialize
    // EncryptRsaCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    EncryptAesCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));

    // DecryptRsaCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));
    DecryptAesCtx = (EVP_CIPHER_CTX*)malloc(sizeof(EVP_CIPHER_CTX));

    // malloc check
    if (/* EncryptRsaCtx == NULL  || */ EncryptAesCtx == NULL || /* DecryptRsaCtx == NULL  || */ DecryptAesCtx == NULL) {
        return -1;
    }

    // Init these here to make valgrind happy
        // EVP_CIPHER_CTX_init(EncryptRsaCtx);
        EVP_CIPHER_CTX_init(EncryptAesCtx);

        // EVP_CIPHER_CTX_init(DecryptRsaCtx);
        EVP_CIPHER_CTX_init(DecryptAesCtx);

    //init RSA
    /* EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);  // create pkey algorithm context for RSA

    if(EVP_PKEY_keygen_init(ctx) <= 0) { // initialize the keygen
        return FAILURE;
    }

    if(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEYLEN) <= 0) { // set length of key
        return FAILURE;
    }

    if(EVP_PKEY_keygen(ctx, &localKeyPair) <= 0) { // generate key
        return FAILURE
    } */

    // free context
    //EVP_PKEY_CTX_free(ctx);

    // init AES
    aesKey = (unsigned char*)malloc(AES_KEYLEN/8);
    aesIV = (unsigned char*)malloc(AES_KEYLEN/8);

    unsigned char *aesPass = (unsigned char*)malloc(AES_KEYLEN / 8);
    unsigned char *aesSalt = (unsigned char*)malloc(8);

    if(aesKey == NULL || aesIV == NULL || aesPass == NULL || aesSalt == NULL) {
        return -1;
    }

    // Can use password based key derivation for AES, or random data, this uses
    // random data here, can be changed later to fit to project

    #ifdef USE_PBKDF

    // Get some random data to use as the AES pass and salt
        if(RAND_bytes(aesPass, AES_KEYLEN/8) == 0) {
            return -1;
        }

        if(RAND_bytes(aesSalt, 8) == 0) {
            return -1;
        }

    // generate 256 bit key
        if(EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), aesSalt, aesPass, AES_KEYLEN/8, AES_ROUNDS, aesKey, aesIV) == 0) {
            return -1;
        }
    #else
        if(RAND_bytes(aesKey, AES_KEYLEN/8) == 0) {
            return -1;
        }

        if(RAND_bytes(aesIV, AES_KEYLEN/8) == 0) {
            return -1;
        }
    #endif

    free(aesPass);
        free(aesSalt);

        return 0;
}

int Encryption::writeKeyToFile(FILE *fd, int key) {
    switch(key) {
        // write private key in PEM format (certificate)
        case KEY_SERVER_PRIVATE:
            if (!PEM_write_PrivateKey(fd, localKeyPair, NULL, NULL, 0, 0, NULL)) {
                return -1;
            }
            break;
        // write public key for server in PEM format
        case KEY_SERVER_PUB:
            if (!PEM_write_PUBKEY(fd, localKeyPair)) {
                return -1;
            }
            break;
        // write public key for client in PEM format
        case KEY_CLIENT_PUB:
            if (!PEM_write_PUBKEY(fd, remotePubKey)) {
                return -1;
            }
            break;
        // write the aes key to file
        case KEY_AES:
            fwrite(aesKey, 1, AES_KEYLEN, fd);
            break;
        // write te aes IV to file
        case KEY_AES_IV:
            fwrite(aesIV, 1, AES_KEYLEN, fd);
            break;

        default:
            return -1;
    }
    return 0;
}

int Encryption::getRemotePubKey(unsigned char **pubKey)	{

    // BIO is and I/O abstraction in openssl
    // PEM files contain certificates in them
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, remotePubKey);

    // BIO_pending is number of pending characters
    // in the read/write of BIO
    int pubKeyLen = BIO_pending(bio);
    *pubKey = (unsigned char*)malloc(pubKeyLen);
    if (pubKey == NULL) {
        return -1;
    }

    // reads len from bio into buffer
    BIO_read(bio, *pubKey, pubKeyLen);

    // makes last array member nul terminator
    (*pubKey)[pubKeyLen - 1] = '\0';

    // freebird
    BIO_free_all(bio);

    return pubKeyLen;
}

int Encryption::setRemotePubKey(unsigned char* pubKey, size_t pubKeyLen) {

    BIO *bio = BIO_new(BIO_s_mem());

    // check if the content written is the same
    // length as the pubkey; ensure proper len
    if (BIO_write(bio, pubKey, pubKeyLen) != (int)pubKeyLen) {
        return -1;
    }

    RSA *_pubKey = (RSA*)malloc(sizeof(RSA));
    if (_pubKey == NULL) {
        return -1;
    }

    // read the pubkey
    PEM_read_bio_PUBKEY(bio, &remotePubKey, NULL, NULL);

    BIO_free_all(bio);

    return 0;
}

// same as getRemotePubKey
int Encryption::getLocalPubKey(unsigned char** pubKey) {

    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, localKeyPair);


    int pubKeyLen = BIO_pending(bio);
    *pubKey = (unsigned char*)malloc(pubKeyLen);
    if (pubKey == NULL) {
        return -1;
    }

    BIO_read(bio, *pubKey, pubKeyLen);

    (*pubKey)[pubKeyLen - 1] = '\0';

    BIO_free_all(bio);

    return pubKeyLen;
}

int Encryption::getLocalPrivateKey(unsigned char **privateKey) {
    BIO *bio = BIO_new(BIO_s_mem());

    PEM_write_bio_PrivateKey(bio, localKeyPair, NULL, NULL, 0, 0, NULL);

    int privateKeyLen = BIO_pending(bio);
    *privateKey = (unsigned char*)malloc(privateKeyLen + 1);
    if (privateKey == NULL) {
        return -1;
    }

    BIO_read(bio, *privateKey, privateKeyLen);

    (*privateKey)[privateKeyLen] = '\0';

    BIO_free_all(bio);

    return privateKeyLen;
}

int Encryption::getAesKey(unsigned char **aesKey) {
    *aesKey = this->aesKey;
    return AES_KEYLEN/8;
}

int Encryption::setAesKey(unsigned char *aesKey, size_t aesKeyLen) {
    if ((int)aesKeyLen != AES_KEYLEN/8) {
        return -1;
    }

    memcpy(this->aesKey, aesKey, AES_KEYLEN/8);

    return 0;
}

int Encryption::getAesIV(unsigned char **aesIV) {
    *aesIV = this->aesIV;
    return AES_KEYLEN/16;
}

int Encryption::setAesIV(unsigned char *aesIV, size_t aesIVLen) {
    if ((int)aesIVLen != AES_KEYLEN/16) {
        return -1;
    }

    memcpy(this->aesIV, aesIV, AES_KEYLEN/16);

    return 0;
}

int Encryption::genTestClientKey() {

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEYLEN) <= 0) {
        return -1;
    }

    if (EVP_PKEY_keygen(ctx, &remotePubKey) <= 0) {
        return -1;
    }

    EVP_PKEY_CTX_free(ctx);

    return 0;
}