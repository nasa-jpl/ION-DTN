/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2009 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

#include "crypto.h"

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/
char* crypto_suite_name="NULL_SUITES";


/*****************************************************************************
 *                     HMAC-SHA-1 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int hmac_sha1_context_length()
{
	// Made it maximum
	return 20; 
}

void hmac_sha1_init(void *context, unsigned char *key, int key_length)
{
	/* Set the context as the key, for specious authentication */
        if(key_length > 20) 
        {
          key_length = 20;
        }
	memset(context, 0, 20);
	memcpy(context, key, key_length);
	return;
}

void hmac_sha1_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void hmac_sha1_final(void *context, unsigned char *result, int resultLen)
{
	/* Context contains the key */
	/* This function simply sets the security result to be the key */
	memset(result,0,resultLen);
	if(resultLen > 20)
	{
		resultLen = 20;
	}
	memcpy(result, context, resultLen);
}

void hmac_sha1_reset(void *context)
{
	return;
}

int hmac_authenticate(char * mac_buffer, const int mac_size, const char * key, const int key_length, const char * message, const int message_length)
{
	memset(mac_buffer, 0, mac_size);
	return mac_size;

}


/*****************************************************************************
 *                     HMAC-SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int hmac_sha256_context_length()
{
	// Made it maximum
	return 32; 
}

void hmac_sha256_init(void *context, unsigned char *key, int key_length)
{
	/* Set the context as the key, for specious authentication */
        if(key_length > 32) 
        {
          key_length = 32;
        }
	memset(context, 0, 32);
	memcpy(context, key, key_length);
	return;
}

void hmac_sha256_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void hmac_sha256_final(void *context, unsigned char *result, int resultLen)
{
	/* Context contains the key */
	/* This function simply sets the security result to be the key */
	memset(result,0,resultLen);
	if(resultLen > 32)
	{
		resultLen = 32;
	}
	memcpy(result, context, resultLen);
}

void hmac_sha256_reset(void *context)
{
	return;
}


/*****************************************************************************
 *                       SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int sha256_context_length()
{
	return 1;
}

void sha256_init(void *context)
{
	return;
}

void sha256_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void sha256_final(void *context, unsigned char *result, int resultLen)
{
	memset(result, 0, resultLen);
}


/*****************************************************************************
 *                       RSA SIGNING FUNCTION DEFINITIONS                    *
 *****************************************************************************/

int rsa_sha256_sign_context_length()
{
	return 1;
}

void rsa_sha256_sign_init(void *context)
{
	return;
}

int rsa_sha256_sign(void *context, int hashlen, void *hashData, int signatureLen, void *signature)
{
	memset(signature, 0, signatureLen);
	return 0;
}

int rsa_sha256_verify(void *context, int hashlen, void *hashData, int signatureLen, void *signature)
{
	memset(signature, 0, signatureLen);
	return 0;
}


/*****************************************************************************
 *                       arc4 FUNCTION DEFINITIONS                           *
 *****************************************************************************/

/*
 * ARC4 key schedule
 */
void arc4_setup( arc4_context *ctx, const unsigned char *key, unsigned int keylen )
{
    return;
}

/*
 * ARC4 cipher function
 */
int arc4_crypt( arc4_context *ctx, size_t length, const unsigned char *input,
                unsigned char *output )
{
    memcpy(output, input, length);
    return( 0 );
}
