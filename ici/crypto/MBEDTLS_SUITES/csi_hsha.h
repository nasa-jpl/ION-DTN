/*****************************************************************************
 **
 ** File Name: csi_hsha.h
 **
 ** Description: This file defines implementations of the ION Cryptographic
 **              interface as related to HMAC-SHA(1, 256, 384) suites.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  02/05/16  E. Birrane     Initial Implementation [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/

#ifndef _CSI_HSHA_H_
#define _CSI_HSHA_H_

#include "platform.h"
#include "mbedtls/md.h"
#include "mbedtls/entropy.h"

#include "csi.h"
#include "csi_debug.h"

int hsha_init(mbedtls_entropy_context *entropy);
void hsha_teardown();

uint32_t hsha_parm_get_len(csi_csid_t suite, csi_parmid_t parmid);


extern uint32_t hsha_blocksize(csi_csid_t suite);

extern csi_val_t hsha_rand(csi_csid_t suite, uint32_t len);


extern uint32_t hsha_ctx_len(csi_csid_t suite);
extern mbedtls_md_context_t *hsha_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc);
extern uint8_t  hsha_ctx_free(csi_csid_t suite, void *context);

extern uint32_t hsha_sign_res_len(csi_csid_t suite, void *context);

extern int8_t  hsha_sign_start(csi_csid_t suite, void *context);
extern int8_t  hsha_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc);
extern int8_t  hsha_sign_finish(csi_csid_t suite, void *context, csi_val_t *result, csi_svcid_t svc);
extern int8_t  hsha_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc);

//extern csi_val_t hsha_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_svcid_t svc);


#endif /* CSI_HSHA_H_ */
