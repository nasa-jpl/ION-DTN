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

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include "platform.h"

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/

/*****************************************************************************
 *                   FAUX POLARSSL FUNCTION DEFINITIONS                      *
 *                       From polarssl polarssl.org                          *
 *****************************************************************************/

/**
 * \brief          ARC4 context structure
 */
typedef struct
{
	int		x;			/*!< permutation index */
	int		y;			/*!< permutation index */
	unsigned char	m[256];			/*!< permutation table */
} arc4_context;

/**
 * \brief          ARC4 key schedule
 *
 * \param ctx      ARC4 context to be initialized
 * \param key      the secret key
 * \param keylen   length of the key
 */
extern void	arc4_setup(arc4_context *ctx, const unsigned char *key,
			unsigned int keylen);

/**
 * \brief          ARC4 cipher function
 *
 * \param ctx      ARC4 context
 * \param length   length of the input data
 * \param input    buffer holding the input data
 * \param output   buffer for the output data
 *
 * \return         0 if successful
 */
extern int	arc4_crypt(arc4_context *ctx, size_t length,
			const unsigned char *input, unsigned char *output);

/*****************************************************************************
 *                     HMAC-SHA-1 FUNCTION DEFINITIONS                       *
 *****************************************************************************/

extern int	hmac_sha1_context_length();
extern void	hmac_sha1_init(void *context, unsigned char *key,
			int key_length);
extern void	hmac_sha1_update(void *context, unsigned char *data,
			int data_length);
extern void	hmac_sha1_final(void *context, unsigned char *result,
			int resultLen);
extern void	hmac_sha1_reset(void *context);
extern void	hmac_sha1_sign(const unsigned char *key, size_t keylen,
			const unsigned char *input, size_t ilen,
			unsigned char output[20]);
extern int	hmac_authenticate(char *mac_buffer, const int mac_size,
			const char *key, const int key_length,
			const char *message, const int message_length);

/*****************************************************************************
 *                     HMAC-SHA-256 FUNCTION DEFINITIONS                     *
 *****************************************************************************/

extern int	hmac_sha256_context_length();
extern void	hmac_sha256_init(void *context, unsigned char *key,
			int key_length);
extern void	hmac_sha256_update(void *context, unsigned char *data,
			int data_length);
extern void	hmac_sha256_final(void *context, unsigned char *result,
			int resultLen);
extern void	hmac_sha256_reset(void *context);

/*****************************************************************************
 *                       SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

extern int	sha256_context_length();
extern void	sha256_init(void *context);
extern void	sha256_update(void *context, unsigned char *data,
			int data_length);
extern void	sha256_final(void *context, unsigned char *result,
			int resultLen);
extern void	sha256_hash(unsigned char *data, int dataLength,
			unsigned char *result, int resultLen);

/*****************************************************************************
 *                       SHA-2 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

extern void	sha2(const unsigned char *input, size_t inputLength,
			unsigned char output[32], int is224);

/*****************************************************************************
 *                RSA AUTHENTICATION FUNCTION DEFINITIONS                    *
 *****************************************************************************/

extern int	rsa_sha256_sign_init(void **context, const char* keyValue,
			int keyLength);
extern int	rsa_sha256_sign_context_length(void *context);
extern int	rsa_sha256_sign(void *context, int hashlen, void *hashData,
			int signatureLen, void *signature);
extern int	rsa_sha256_verify_init(void **context, const char* keyValue,
			int keyLength);
extern int	rsa_sha256_verify_context_length(void *context);
extern int	rsa_sha256_verify(void *context, int hashlen, void *hashData,
			int signatureLen, void *signature);

#endif
