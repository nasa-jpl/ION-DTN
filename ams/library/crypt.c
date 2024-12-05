/*
	default.crypt.c:	stub encryption and decryption functions
				for public distribution of AMS.

	Author: Scott Burleigh, JPL

	Copyright (c) 2005, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "amscommon.h"
#include "secrypt.h"


void non_crypto_initializer(unsigned char *array, size_t length) 
{
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t alphanumSize = sizeof(alphanum) - 1;

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    for (size_t i = 0; i < length; i++) 
	{
        array[i] = alphanum[rand() % alphanumSize];
    }
}


void	encryptUsingPublicKey(char *cyphertext, int *ctlen, char *key, int klen,
		char *plaintext, int ptlen)
{
	if (cyphertext == NULL || ctlen == NULL || key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPublicKey(char *plaintext, int *ptlen, char *key, int klen,
		char *cyphertext, int ctlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

void	encryptUsingPrivateKey(char *cyphertext, int *ctlen, char *key,
		int klen, char *plaintext, int ptlen)
{
	if (cyphertext == NULL || ctlen == NULL || key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return;
	}

	memcpy(cyphertext, plaintext, ptlen);
	*ctlen = ptlen;
}

void	decryptUsingPrivateKey(char *plaintext, int *ptlen, char *key, int klen,
		char *cyphertext, int ctlen)
{
	if (plaintext == NULL || ptlen == NULL || key == NULL || klen < 0
	|| cyphertext == NULL || ctlen < 0)
	{
		return;
	}

	memcpy(plaintext, cyphertext, ctlen);
	*ptlen = ctlen;
}

int	encryptUsingSymmetricKey(char **cyphertext, char *key,
		int klen, char *plaintext, int ptlen)
{
	if (key == NULL || klen < 0
	|| plaintext == NULL || ptlen < 0)
	{
		return -1;
	}

	size_t cyphertext_length = 0;
	size_t input_length = (size_t)ptlen;  

	unsigned char iv_initializer[16];
    non_crypto_initializer(iv_initializer, 16);  /* to seed the IV with random alpha-numeric */

	int mode=0; /* encrypt */
	/* cyphertext allocated in crypt_and_hash_buffer as we can not know its size yet*/
	int result = crypt_and_hash_buffer(mode, 
                                       (unsigned char*) iv_initializer, 
                                       (unsigned char*) plaintext, 
                                       (size_t*) &input_length, 
                                       (unsigned char**)cyphertext, 
                                       &cyphertext_length, 
                                       CIPHER, 
                                       MD, 
                                       key);
    if (result != 0)
    {	
    	writeErrMemo("Error: AMS encryptUsingSymmetricKey\n");
		if(*cyphertext)
		{
			RELEASE_CONTENT_SPACE(*cyphertext); 
			*cyphertext = NULL;
		}

        return -1;
    }	

    return (int)cyphertext_length;  /* length of the encrypted data */

}

int	decryptUsingSymmetricKey(char **plaintext, char *key,
		int klen, char *cyphertext, int ctlen)
{		
	if (key == NULL || klen <= 0
	|| cyphertext == NULL || ctlen <  0)
	{
		return -1;
	}
	
	size_t cyphertext_length = (size_t)ctlen;
	size_t plaintext_length = 0;

	int mode=1; /* decrypt */
	/* plaintext allocated in crypt_and_hash_buffer as we do not know its size (yet) */
	int result = crypt_and_hash_buffer(mode, 
                                       (unsigned char*) NULL, 
                                       (unsigned char*) cyphertext, 
                                       (size_t*) &cyphertext_length, 
                                       (unsigned char**)plaintext, 
                                       &plaintext_length, 
                                       CIPHER, 
                                       MD, 
                                       key);
    if (result != 0)
    {	
		writeErrMemo("AMS Decryption error\n");
		if(*plaintext)
		{
			RELEASE_CONTENT_SPACE(*plaintext); 
			*plaintext = NULL;
		}
        return -1;
    }	
    return (int)plaintext_length;  /* length of the decrypted data */
}
