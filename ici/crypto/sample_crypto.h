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

#include "NULL_SUITES/sha1.h"
#include "NULL_SUITES/hmac.h"



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
 *                       SHA-256 FUNCTION DEFINITIONS                        *
 *****************************************************************************/

int sha256_context_length();
void sha256_init(void *context);
void sha256_update(void *context, unsigned char *data, int data_length);
void sha256_final(void *context, unsigned char *result, int resultLen);


#endif

