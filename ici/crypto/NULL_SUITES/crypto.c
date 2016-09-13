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

#include "ion.h"
#include "crypto.h"

#define	RSA_PKCS_V15	0
#define	RSA_PUBLIC	0
#define	RSA_PRIVATE	1
#define	SIG_RSA_SHA256	11

typedef int	mpi;

typedef struct
{
	int	ver;
	size_t	len;
	mpi	N;
	mpi	E;
	mpi	D;
	mpi	P;
	mpi	Q;
	mpi	DP;
	mpi	DQ;
	mpi	QP;
	mpi	RN;
	mpi	RP;
	mpi	RQ;
	int	padding;
	int	hash_id;
} rsa_context;

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/

/*****************************************************************************
 *                  FAUX POLARSSL FUNCTION DEFINITIONS                       *
 *****************************************************************************/

static void	sha1_hmac(const unsigned char *key, size_t keylen,
			const unsigned char *input, size_t ilen,
			unsigned char output[20])
{
	return;
}

static int	x509parse_key(rsa_context *rsa, const unsigned char *key,
			size_t keylen, const unsigned char *pwd, size_t pwdlen)
{
	return 0;
}

static int	x509parse_public_key(rsa_context *rsa, const unsigned char *key,
			size_t keylen)
{
	return 0;
}

static void	rsa_init(rsa_context *ctx, int padding, int hash_id)
{
	ctx->padding = padding;
	ctx->hash_id = hash_id;
}

static int	rsa_rsassa_pkcs1_v15_sign(rsa_context *ctx, int mode,
			int hash_id, unsigned int hashlen,
			const unsigned char *hash, unsigned char *sig)
{
	return 0;
}

static int	rsa_rsassa_pkcs1_v15_verify(rsa_context *ctx, int mode,
			int hash_id, unsigned int hashlen,
			const unsigned char *hash, unsigned char *sig)
{
	return 0;
}

void	sha2(const unsigned char *input, size_t ilen,
		unsigned char output[32], int is224)
{
	return;
}

/*
 * ARC4 key schedule
 */
void	arc4_setup( arc4_context *ctx, const unsigned char *key,
		unsigned int keylen )
{
	return;
}

/*
 * ARC4 cipher function
 */
int	arc4_crypt( arc4_context *ctx, size_t length,
		const unsigned char *input, unsigned char *output )
{
	memcpy(output, input, length);
	return( 0 );
}

/*****************************************************************************
 *                     HMAC-SHA-1 FUNCTION DEFINITIONS                       *
 *****************************************************************************/

int	hmac_sha1_context_length()
{
	return 20; 	/*	The maximum.				*/
}

void	hmac_sha1_init(void *context, unsigned char *key, int key_length)
{
	/*	Set the context as the key for specious authentication.	*/

	if (key_length > 20) 
	{
		key_length = 20;
	}

	memset(context, 0, 20);
	memcpy(context, key, key_length);
	return;
}

void	hmac_sha1_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void	hmac_sha1_final(void *context, unsigned char *result, int resultLen)
{
	/*	Context contains the key, per init function above.
		This function simply sets the security result to be
		the key.						*/

	memset(result, 0, resultLen);
	if(resultLen > 20)
	{
		resultLen = 20;
	}

	memcpy(result, context, resultLen);
}

void	hmac_sha1_reset(void *context)
{
	return;
}

void	hmac_sha1_sign(const unsigned char *key, size_t keylen,
		const unsigned char *input, size_t ilen,
		unsigned char output[20])
{
	sha1_hmac(key, keylen, input, ilen, output);
}

int	hmac_authenticate(char *mac_buffer, const int mac_size,
		const char *key, const int key_length, const char *message,
		const int message_length)
{
	memset(mac_buffer, 0, mac_size);
	return mac_size;
}

/*****************************************************************************
 *                     HMAC-SHA-256 FUNCTION DEFINITIONS                     *
 *****************************************************************************/

int	hmac_sha256_context_length()
{
	return 32; 	/*	The maximum.				*/
}

void	hmac_sha256_init(void *context, unsigned char *key, int key_length)
{
	/*	Set the context as the key for specious authentication.	*/

	if (key_length > 32) 
	{
		key_length = 32;
	}

	memset(context, 0, 32);
	memcpy(context, key, key_length);
	return;
}

void	hmac_sha256_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void	hmac_sha256_final(void *context, unsigned char *result, int resultLen)
{
	/*	Context contains the key, per init function above.
		This function simply sets the security result to be
		the key.						*/

	memset(result, 0, resultLen);
	if (resultLen > 32)
	{
		resultLen = 32;
	}

	memcpy(result, context, resultLen);
}

void	hmac_sha256_reset(void *context)
{
	return;
}

/*****************************************************************************
 *                       SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int	sha256_context_length()
{
	return 1;
}

void	sha256_init(void *context)
{
	return;
}

void	sha256_update(void *context, unsigned char *data, int data_length)
{
	return;
}

void	sha256_final(void *context, unsigned char *result, int resultLen)
{
	memset(result, 0, resultLen);
}

void	sha256_hash(unsigned char *data, int data_length,
		unsigned char *result, int resultLen)
{
	memset(result, 0, resultLen);
	sha2(data, data_length, result, 0);
}

/*****************************************************************************
 *                RSA AUTHENTICATION FUNCTION DEFINITIONS                    *
 *****************************************************************************/

int	rsa_sha256_sign_init(void **context, const char *keyValue,
		int keyLength)
{
	*context = MTAKE(sizeof(rsa_context));

	/*	Note that the hash_id parameter is ignored when using
	 *	RSA_PKCS_V15 padding					*/

	rsa_init((rsa_context *)(*context), RSA_PKCS_V15, 0);
	return x509parse_key((rsa_context *)(*context),
			(const unsigned char *) keyValue, (size_t) keyLength,
			NULL, 0);
}

int	rsa_sha256_sign_context_length(void *context)
{
	if (context == NULL)
	{
		return 0;
	}

	return ((rsa_context *) context)->len;
}

int	rsa_sha256_sign(void *context, int hashlen, void *hashData,
		int signatureLen, void *signature)
{
	memset(signature, 0, signatureLen);
	return rsa_rsassa_pkcs1_v15_sign((rsa_context *) context, RSA_PRIVATE,
			SIG_RSA_SHA256, 0, hashData, signature);
}

int	rsa_sha256_verify_init(void **context, const char* keyValue,
		int keyLength)
{
	*context = MTAKE(sizeof(rsa_context));

	/*	Note that the hash_id parameter is ignored when using
	 *	RSA_PKCS_V15 padding					*/

	rsa_init((rsa_context *)(*context), RSA_PKCS_V15, 0);
	return x509parse_public_key((rsa_context *)(*context),
			(const unsigned char *) keyValue, (size_t) keyLength);
}

int	rsa_sha256_verify_context_length(void *context)
{
	if (context == NULL)
	{
		return 0;
	}

	return ((rsa_context *) context)->len;
}

int	rsa_sha256_verify(void *context, int hashlen, void *hashData,
		int signatureLen, void *signature)
{
	return rsa_rsassa_pkcs1_v15_verify((rsa_context *) context, RSA_PUBLIC,
			SIG_RSA_SHA256, 0, hashData,
			(unsigned char *)signature);
}
