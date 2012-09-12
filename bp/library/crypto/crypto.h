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

#define NULL_CRYPTO_SUITES

#include "platform.h"


/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/
extern char* crypto_suite_name;


/*****************************************************************************
 *                     HMAC-SHA-1 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int hmac_sha1_context_length();
void hmac_sha1_init(void *context, unsigned char *key, int key_length);
void hmac_sha1_update(void *context, unsigned char *data, int data_length);
void hmac_sha1_final(void *context, unsigned char *result, int resultLen);
void hmac_sha1_reset(void *context);
int hmac_authenticate(char * mac_buffer, const int mac_size, const char * key, const int key_length, const char * message, const int message_length);


/*****************************************************************************
 *                     HMAC-SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int hmac_sha256_context_length();
void hmac_sha256_init(void *context, unsigned char *key, int key_length);
void hmac_sha256_update(void *context, unsigned char *data, int data_length);
void hmac_sha256_final(void *context, unsigned char *result, int resultLen);
void hmac_sha256_reset(void *context);

/*****************************************************************************
 *                       SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int sha256_context_length();
void sha256_init(void *context);
void sha256_update(void *context, unsigned char *data, int data_length);
void sha256_final(void *context, unsigned char *result, int resultLen);


/*****************************************************************************
 *                       arc4 FUNCTION DEFINITIONS                           *
 *                       From polarssl polarssl.org                          *
 *****************************************************************************/

/**
 * \brief          ARC4 context structure
 */
typedef struct
{
    int x;                      /*!< permutation index */
    int y;                      /*!< permutation index */
    unsigned char m[256];       /*!< permutation table */
}
arc4_context;

/**
 * \brief          ARC4 key schedule
 *
 * \param ctx      ARC4 context to be initialized
 * \param key      the secret key
 * \param keylen   length of the key
 */
void arc4_setup( arc4_context *ctx, const unsigned char *key, unsigned int keylen );

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
int arc4_crypt( arc4_context *ctx, size_t length, const unsigned char *input,
                unsigned char *output );

#endif

