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

#include "sha1.h"
/* #include "sha2.h" */
/* #include "rsa.h" */
#include "../crypto.h"

/*
typedef struct {
	struct hmac_st	hmac_metainf;
} NullSecStruct;
*/
/*****************************************************************************
 *                     HMAC-SHA-1 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int hmac_sha1_context_length()
{
	return 1; //sizeof(NullSecStruct); //1;
}

void hmac_sha1_init(void *context, unsigned char *key, int key_length)
{
    //    NullSecStruct * ctxt = (NullSecStruct *) context;
	//memset(ctxt, 0, sizeof(NullSecStruct));
	//memcpy(ctxt->hmac_metainf.key, key, key_length);
	return;
}

void hmac_sha1_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void hmac_sha1_final(void *context, unsigned char *result, int resultLen)
{
	memset(result,0,resultLen);

	/*
	NullSecStruct * ctxt = (NullSecStruct *) context;
	memset(result, 0, resultLen);
	// Set security result to be the first 5 bytes of the key...
	memcpy(result, ctxt->hmac_metainf.key, 5);
	*/
}

void hmac_sha1_reset(void *context)
{
	//memset(context, 0, sizeof(NullSecStruct));
	return;
}

int hmac_authenticate(char * mac_buffer, const int mac_size, const char * key, const int key_length, const char * message, const int message_length)
{
	memset(mac_buffer, 0, mac_size);
	return mac_size;

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



