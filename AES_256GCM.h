#ifndef AES_256GCM_H
#define AES_256GCM_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>



void handleErrors();
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad, int aad_len, unsigned char *key, unsigned char *iv,
	unsigned char *ciphertext, unsigned char *tag);

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad, int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	unsigned char *plaintext);

#endif