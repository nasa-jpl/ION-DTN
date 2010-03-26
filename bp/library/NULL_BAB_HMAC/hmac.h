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

#ifndef _HMAC_H_
#define _HMAC_H_
#include <string.h>
#include <stdio.h>
#include "sha1.h"

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/

/**
 * Using SHA1 implementation provided by Git
 */
#define SHA1

/**
 * NOTE: All values are in 8-bit octets
 */
#ifdef SHA1
#define DIGEST_SIZE_BYTES       20      // Default digest size for SHA1...
#define BLOCK_SIZE_BYTES        64      // Set in RFC 2104
#define MAX_KEY_LEN_BYTES       64      // Arbitrary for now...
#define MIN_KEY_LEN_BYTES       20      // 160-bit key recommended minimum

#define hash_struct     moz_SHA_CTX

#else
#error Only SHA1 currently supported
#endif


/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

struct hmac_st {
        char    key [MAX_KEY_LEN_BYTES];
        int     key_length;
        char    digest [DIGEST_SIZE_BYTES];
        hash_struct hash;

        // I'm note quite sure these belong in here,
        // but I'd rather not have them float around
        char k_ipad [BLOCK_SIZE_BYTES];
        char k_opad [BLOCK_SIZE_BYTES];
};


void demo_print(char * name, char * buffer, int len);

int hmac_init(struct hmac_st * hmac, char * key, int key_length);
int hmac_hash(struct hmac_st * hmac, char * message, int message_length);
int hmac_final(struct hmac_st * hmac);
int hmac_authenticate(char * mac_buffer, 
                      const int mac_size, 
                      const char * key, 
                      const int key_length, 
                      const char * message, 
                      const int message_length);


#endif

