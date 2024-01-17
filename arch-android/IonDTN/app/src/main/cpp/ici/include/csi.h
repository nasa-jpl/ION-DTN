/*****************************************************************************
 **
 ** File Name: csi.h
 **
 ** Description: The crypto package defines a generic interface for ciphersuites.
 **              This interface is independent of any particular ciphersuite or
 **              cryptographic approach.
 **
 **				  The ION Cryptographic Interface provides a series of generic
 **				  functions that can be applied to any ION-supported ciphersuite.
 **				  Each function in the interface accepts a "suite" identifier which
 **				  indicates which suite should be used for the function.
 **
 **				  The functions provided by this interface are as follows:
 **
 **				  - The "ctx_len" function returns the length of the context used to
 **				  allow reentrant calls into the ciphersuite.
 **
 **				  - The "init", "reset", and "free" functions manage contexts for ciphersuites.
 **
 **				  - The "full" function applies the entire ciphersuite via a single
 **				  function call in cases where a streaming version of the ciphersuite
 **				  is not necessary.
 **
 **				  - The "update" and "finish" functions are used, with a context, to
 **				  provide incremental accumulation of a security result from a ciphersuite
 **				  given a reentrant context.
 **
 **
 ** Notes:
 **    1. ION does not ship with any security ciphersuites.
 **    2. The structures and functions defined in this interface do not imply
 **       any particular ciphersuite implementation and should not be considered
 **       as more or less appropriate for any ciphersuite library.
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTIONuint8_t *out
 **  --------  ------------   ---------------------------------------------
 **  09/20/15  E. Birrane     Update to generic interfaces [Secure DTN
 **                           implementation (NASA: NNX14CS58P)]
 *****************************************************************************/


#ifndef _CSI_H_
#define _CSI_H_

#define NULL_CRYPTO_SUITES

#include "platform.h"
#include "ion.h"

/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/

/** Ciphersuite types */
typedef enum {
	CSTYPE_HMAC_SHA1     = 0x001, /* From RFC 6257 */
	CSTYPE_HMAC_SHA256   = 0x005, /* From RFC 6257 */
	CSTYPE_HMAC_SHA384   = 0xD1,
  	CSTYPE_ECDSA_SHA256  = 0xD2,
	CSTYPE_ECDSA_SHA384  = 0xD3,
  	CSTYPE_SHA256_AES128 = 0xD4,
	CSTYPE_SHA384_AES256 = 0xD5,
	CSTYPE_ARC4		     = 0x006  /* From RFC 6257 */
} csi_csid_t;

typedef enum {
	CSI_SVC_SIGN,
	CSI_SVC_VERIFY,
	CSI_SVC_ENCRYPT,
	CSI_SVC_DECRYPT
} csi_svcid_t;

typedef enum {
	CSI_PARM_IV      = 1,
	CSI_PARM_KEYINFO = 3,
	CSI_PARM_INTSIG  = 5,
	CSI_PARM_SALT    = 7,
	CSI_PARM_ICV     = 8,
	CSI_PARM_BEK     = 9,
	CSI_PARM_BEKICV  = 10
} csi_parmid_t;

#define CSI_SUITE_NAME "MBEDTLS Suites"

typedef struct
{
	uint8_t *contents;
	int32_t len;
} csi_val_t;

typedef struct
{
	uint32_t plaintextLen;
	uint32_t chunkSize;
	uint32_t keySize;
} csi_blocksize_t;

typedef struct
{
	csi_val_t iv;
	csi_val_t salt;
	csi_val_t icv;
	csi_val_t intsig;
	csi_val_t add;
	csi_val_t keyinfo;
} csi_cipherparms_t;


/*****************************************************************************
 *          ION Ciphersuite Interface (CSI) Shared Functions                 *
 *****************************************************************************/
extern csi_cipherparms_t csi_build_parms(unsigned char *buf, uint32_t len);

extern void      csi_cipherparms_free(csi_cipherparms_t parms);
extern void      csi_init();
extern char     *csi_val_print(csi_val_t val, uint32_t maxLen);
extern csi_val_t csi_rand(uint32_t len);
extern csi_val_t csi_serialize_parms(csi_cipherparms_t parms);
extern csi_cipherparms_t csi_deserialize_parms(uint8_t *buffer, uint32_t len);
extern void      csi_teardown();
extern csi_val_t csi_extract_tlv(uint8_t itemNeeded, unsigned char *buf, uint32_t buflen);
extern csi_val_t csi_build_tlv(uint8_t id, uint32_t len, uint8_t *contents);


/*****************************************************************************
 *          ION Ciphersuite Interface (CSI) Common Functions                 *
 *****************************************************************************/

extern uint32_t csi_blocksize(csi_csid_t suite);

extern uint32_t csi_ctx_len(csi_csid_t suite);
extern uint8_t *csi_ctx_init(csi_csid_t suite, csi_val_t key_info, csi_svcid_t svc);
extern uint8_t  csi_ctx_free(csi_csid_t suite, void *context);



/*****************************************************************************
 *          ION Crypto Interface Integrity (SIGN/VERIFY) Functions           *
 *****************************************************************************/

extern uint32_t csi_sign_res_len(csi_csid_t suite, void *context);

extern int8_t  csi_sign_start(csi_csid_t suite, void *context);
extern int8_t  csi_sign_update(csi_csid_t suite, void *context, csi_val_t data, csi_svcid_t svc);
extern int8_t  csi_sign_finish(csi_csid_t suite, void *context, csi_val_t *result, csi_svcid_t svc);

extern int8_t csi_sign_full(csi_csid_t suite, csi_val_t input, csi_val_t key, csi_val_t *result, csi_svcid_t svc);


/*****************************************************************************
 *     ION Crypto Interface Confidentiality (ENCRYPT/DECRYPT) Functions     *
 *****************************************************************************/

extern int8_t    csi_crypt_finish(csi_csid_t suite, void *context, csi_svcid_t svc, csi_cipherparms_t *parms);
extern int8_t    csi_crypt_full(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t key, csi_val_t input, csi_val_t *output);
extern int8_t    csi_crypt_key(csi_csid_t suite, csi_svcid_t svc, csi_cipherparms_t *parms, csi_val_t longtermkey, csi_val_t input, csi_val_t *output);


extern csi_val_t csi_crypt_parm_get(csi_csid_t suite, csi_parmid_t parmid);
extern uint32_t  csi_crypt_parm_get_len(csi_csid_t suite, csi_parmid_t parmid);

extern uint32_t  csi_crypt_res_len(csi_csid_t suite, void *context, csi_blocksize_t blocksize, csi_svcid_t svc);

extern int8_t    csi_crypt_start(csi_csid_t suite, void *context, csi_cipherparms_t parms);

extern csi_val_t csi_crypt_update(csi_csid_t suite, void *context, csi_svcid_t svc, csi_val_t data);


#endif
