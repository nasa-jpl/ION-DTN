/*****************************************************************************
 **
 ** File Name: csi_ecdsa.c
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to elliptic curve DSA suites.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  02/13/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef _CSI_ECDSA_H_
#define _CSI_ECDSA_H_

#include "platform.h"

#include "csi.h"
#include "csi_debug.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"

/**
 * csi_ecdsa_ctx
 *
 * There are two contexts that must be tracked for ECDSA operations.
 *
 * First is SHA context that will be used to generate the hash. This
 * MUST be one of (mbedtls_sha256_context or mbedtls_sha512_context)
 * which provide context for SHA-224, SHA-256, SHA-384, and SHA-512.
 * This field will be interpreted based on the "suite" passed into
 * the ecdsa_ctx_init function and stored in the "type" variable.
 *
 * Second is the ECDSA context which will be used to sign the
 * resultant hash. This MUST be of type mbedtls_ecdsa_context.
 */
typedef struct
{
	mbedtls_ecdsa_context *ecdsa_ctx;
	void *sha_ctx;
	uint8_t type;
} csi_ecdsa_ctx_t;



extern uint32_t  ecdsa_blocksize(csi_csid_t suite);

// Internal functions.
extern mbedtls_ecdsa_context *ecdsa_ctx_build(csi_csid_t suite, csi_val_t *key_info);

extern uint32_t  ecdsa_ctx_len(csi_csid_t suite);
extern uint8_t   ecdsa_ctx_free(csi_csid_t suite, void *context);
extern csi_ecdsa_ctx_t  *ecdsa_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc);

extern int8_t   ecdsa_sign_finish(csi_csid_t suite, void *context, csi_val_t *result, csi_svcid_t svc);
extern int8_t   ecdsa_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc);

extern uint32_t  ecdsa_sign_res_len(csi_csid_t suite, void *context);

extern int8_t   ecdsa_sign_start(csi_csid_t suite, void *context);
extern int8_t   ecdsa_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc);


/***
uint32_t    ecdsa_ctx_len(uint8_t suite, void *scratch);
uint8_t *   ecdsa_ctx_init(uint8_t suite, crypt_val_t *key_info, void *scratch);
uint8_t     ecdsa_finish(uint8_t suite, void *context, crypt_val_t *result, uint8_t function, void *scratch);
uint8_t     ecdsa_ctx_free(uint8_t suite, void *context, void *scratch);
crypt_val_t ecdsa_full(uint8_t suite, crypt_val_t *input, crypt_val_t key, uint8_t function, void *scratch);
uint32_t    ecdsa_get_blocksize(uint8_t suite, void *scratch);
uint8_t     ecdsa_reset(uint8_t suite, void *context, void *scratch);
uint32_t    ecdsa_res_len(uint8_t suite, void *context, crypt_blocksize_t* blocksize, uint8_t function);
uint8_t     ecdsa_update(uint8_t suite, void *context, crypt_val_t *data, uint8_t function, void *scratch);
crypt_val_t ecdsa_get_session_key(uint8_t suite, void *scratch);
uint32_t    ecdsa_get_session_key_len(uint8_t suite);
crypt_val_t ecdsa_block_update(uint8_t suite, void *context, crypt_val_t *data, uint8_t function, void *scratch);
***/




#endif /* CSI_ECDSA_H_ */
