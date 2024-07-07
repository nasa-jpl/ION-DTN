/*****************************************************************************
 **
 ** File Name: csi_gcm.h
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to AES run in Galois Counter Mode (GCM).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  02/16/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef _CSI_GCM_H_
#define _CSI_GCM_H_

#include "platform.h"

#include "csi.h"
#include "csi_debug.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/gcm.h"
#include "mbedtls/entropy.h"


/*
 * gcm_ctx is the MBED_TLS GCM Context.
 * parms is the set of IV, Salt, ICV, and other information for this encryption.
 */
typedef struct
{
	mbedtls_gcm_context gcm_ctx;
	uint8_t mode;
} csi_gcm_context_t;


int gcm_init(mbedtls_entropy_context *entropy);

void gcm_teardown();

extern uint32_t   gcm_blocksize(csi_csid_t suite);

csi_val_t gcm_rand(csi_csid_t suite, uint32_t len);


extern uint32_t   gcm_ctx_len(csi_csid_t suite);
extern csi_gcm_context_t *  gcm_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc);
extern uint8_t    gcm_ctx_free(csi_csid_t suite, void *context);

extern int8_t     gcm_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc, csi_cipherparms_t *parms);

extern int8_t     gcm_crypt_full(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t key, csi_val_t input, csi_val_t *output);

extern int8_t     gcm_crypt_key(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t longtermkey, csi_val_t input, csi_val_t *output);


extern  csi_val_t gcm_crypt_parm_get(csi_csid_t suite, csi_parmid_t parmid);
extern  uint32_t  gcm_crypt_parm_get_len(csi_csid_t suite, csi_parmid_t parmid);

extern uint32_t   gcm_crypt_res_len(csi_csid_t suite, void *context, csi_blocksize_t blocksize, csi_svcid_t svc);

extern int8_t     gcm_crypt_start(csi_csid_t suite, void *context, csi_cipherparms_t parms);
extern csi_val_t  gcm_crypt_update(csi_csid_t suite, void *context, csi_svcid_t svc, csi_val_t data);


#endif /* CSI_GCM_H_ */
