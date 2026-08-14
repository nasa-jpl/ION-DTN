/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2022 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bib_hmac_sha2.c
 **
 ** Namespace:
 **    bpsec_bhssci_   SCI INterface functions
 **    bpsec_bhsscutl  General utilities
 **
 ** Description:
 **
 **     This file implements The BIB-HMAC-SHA2 security context standardized
 **     by RFC9173.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  03/07/22  E. Birrane     Initial implementation
 *****************************************************************************/

#include "bib_hmac_sha2_sc.h"

#include "bpsec_util.h"
#include "sc_value.h"
#include "cbor.h"
#include "rfc9173_utils.h"


static sc_value_map gBHS_ParmMap[] = {
	{"key_name",    BPSEC_BHSSC_PARM_LTK_NAME,    SC_VAL_TYPE_PARM,   bpsec_scvm_strStrDecode, bpsec_scv_clear, bpsec_scvm_strStrEncode, NULL, NULL},
	{"sha_variant", BPSEC_BHSSC_PARM_SHA_VAR_ID,  SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
	{"wrapped_key", BPSEC_BHSSC_PARM_WRAPPED_KEY, SC_VAL_TYPE_PARM,   bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
	{"scope_flags", BPSEC_BHSSC_PARM_SCOPE_FLAGS, SC_VAL_TYPE_PARM,   bpsec_scvm_intStrDecode, bpsec_scv_clear, bpsec_scvm_intStrEncode, bpsec_scvm_intCborEncode, bpsec_scvm_intCborDecode},
	{"ehmac",       BPSEC_BHSSC_RESULT_EHMAC,     SC_VAL_TYPE_RESULT, bpsec_scvm_hexStrDecode, bpsec_scv_clear, bpsec_scvm_hexStrEncode, bpsec_scvm_hexCborEncode, bpsec_scvm_hexCborDecode},
	{NULL,-1, SC_VAL_TYPE_UNKNOWN, NULL, NULL, NULL, NULL, NULL}
};



/******************************************************************************
 * @brief Process a security operation in an incoming security block.
 *
 * @param[in]  state     - The state of the security block processing
 * @param[in]  wk        - The acquisition work area
 * @param[in]  asb       - The deserialized abstract security block
 * @param[in]  tgtBlkElt - The ELT of the target block in the wk area block list
 * @param[out] tgtResult - Security results generated for this target.
 *
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/

int bpsec_bhssci_procInBlk(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb,
		LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult)
{
	int result = 0;
	sc_value *assertedDigest = NULL;
	csi_val_t key_value;
	uint8_t *csi_ctx = NULL;
	csi_val_t csi_digest;
	BpsecSerializeData ippt_preamble;
	SdrObject ipptZco = 0;
	int ipptZcoLen = 0;
	int addData = 0;
	uint16_t sha_variant = 0;
	BpsecTxContext ctx;

	(void)asb;
	(void)tgtBlkElt;

	CHKERR(state);
	CHKERR(state->scStAction == SC_ACT_VERIFY);

	/* Initialize local arena for inbound verification */
	bpsec_ctx_init(&ctx, state->sdr, state->scStWm);

	sha_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BHSSC_PARM_SHA_VAR_ID, BPSEC_BHSSC_SV_DEFAULT);

	switch(sha_variant)
	{
		case CSTYPE_HMAC_SHA256:
		case CSTYPE_HMAC_SHA384:
		case CSTYPE_HMAC_SHA512:

			break;
		default:
			BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.",  bpsec_scvm_byIdNameFind(gBHS_ParmMap, BPSEC_BHSSC_PARM_SHA_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BHSSC_SV_DEFAULT);
			sha_variant = BPSEC_BHSSC_SV_DEFAULT;
			break;
	}

	if(state->scRawKey.scValLength == 0)
	{
		csi_cipherparms_t parms;
		result = 0;

		memset(&parms, 0, sizeof(csi_cipherparms_t));
		result = bpsec_scutl_keyUnwrap(state, BPSEC_BHSSC_PARM_LTK_NAME, &key_value, BPSEC_BHSSC_PARM_WRAPPED_KEY, CSTYPE_AES_KW, &parms);

		if(result == ERROR)
		{
			BPSEC_DEBUG_ERR("%s",
					"Cannot find a key for verification.");
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}

		state->scRawKey = bpsec_scv_memCsiConvert(key_value, SC_VAL_STORE_MEM, CSI_PARM_BEK);
	}
	else
	{
		key_value.contents = state->scRawKey.scRawValue.asPtr;
		key_value.len = state->scRawKey.scValLength;
	}

	if((assertedDigest = bpsec_scv_lystFind(tgtResult->scIndTargetResults, BPSEC_BHSSC_RESULT_EHMAC, SC_VAL_TYPE_RESULT)) == NULL)
	{
		BPSEC_DEBUG_ERR("No digest found for target %d", tgtResult->scTargetId);
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}
	csi_digest.len = assertedDigest->scValLength;
	csi_digest.contents = assertedDigest->scRawValue.asPtr;

	if((tgtResult->scTargetId == PayloadBlk) || (tgtResult->scTargetId == PrimaryBlk))
	{
		if((ipptZcoLen = bpsec_util_canonicalizeIn(wk, tgtResult->scTargetId, &ipptZco)) <= 0)
		{
			BPSEC_DEBUG_ERR("Cannot canonicalize block-type specific data of " UVAST_FIELDSPEC
					".",
					(uvast) tgtResult->scTargetId);
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}
		addData = 0;
	}
	else
	{
		ipptZcoLen = 0;
		ipptZco = 0;
		addData = 1;
	}

	ippt_preamble = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BHSSC_PARM_SCOPE_FLAGS, tgtResult->scTargetId, addData, NULL, wk);

	if((ippt_preamble.scSerializedLength <= 0) || (ippt_preamble.scSerializedText == NULL))
	{
		BPSEC_DEBUG_ERR("%s", "Cannot build IPPT Data.");
		if (ipptZco != 0) zco_destroy(state->sdr, ipptZco);
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}

	/* Immediately track the preamble text in the arena */
	ctx.resources[ctx.count].type = RES_HEAP;
	ctx.resources[ctx.count].ref.heap_ptr = ippt_preamble.scSerializedText;
	ctx.count++;

	if(ipptZco != 0)
	{
		csi_ctx = bpsec_bhsscutl_computeSignature(ippt_preamble, ipptZco, ipptZcoLen, sha_variant, key_value, CSI_SVC_VERIFY);
		zco_destroy(state->sdr, ipptZco);

		if (csi_ctx == NULL)
		{
			bpsec_ctx_abort(&ctx); /* Safely releases the tracked preamble text */
			return ERROR;
		}

		result = csi_sign_finish(sha_variant, csi_ctx, &csi_digest, CSI_SVC_VERIFY);
		csi_ctx_free(sha_variant, csi_ctx);
	}
	else
	{
		csi_val_t input;
		input.len = ippt_preamble.scSerializedLength;
		input.contents = ippt_preamble.scSerializedText;
		result = csi_sign_full(sha_variant, input, key_value, &csi_digest, CSI_SVC_VERIFY);
	}

	/* Cleanup tracked resources including preamble */
	bpsec_ctx_abort(&ctx);

	return (result == 1);
}




/******************************************************************************
 * @brief Process a security operation in an outgoing security block.
 *
 * @param[in]  state      - The state of the security block processing
 * @param[out] extraParms - Parameters to add to the outgoing security block
 * @param[in]  bundle     - The outgoing bundle
 * @param[in]  asb        - The deserialized abstract security block
 * @param[out] tgtResult  - Security results generated for this target.
 *
 * The extraParms list allows a security operation to generate a parameter that
 * will both be made available for future processing of operations in the same
 * block as well as serialized and included in the security block as it is
 * written to the bundle itself.
 *
 * @retval 1  - Success
 * @retval <1 - Failure
 *****************************************************************************/
int bpsec_bhssci_procOutBlk(sc_state *state, Lyst extraParms, Bundle *bundle,
		                    BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult)
{
	csi_val_t key;
	BpsecSerializeData ippt_preamble;
	SdrObject ipptZco = 0;
	int ipptZcoLen = 0;
	uint8_t *csi_ctx = NULL;
	sc_value *digest = NULL;
	csi_val_t csi_result;
	int result = 0;
	int addData = 0;
	uint16_t sha_variant = 0;
	BpsecTxContext ctx;

	(void)asb;

	CHKERR(state);
	CHKERR(state->scStAction == SC_ACT_SIGN);

	/* Initialize local arena for this transaction */
	bpsec_ctx_init(&ctx, state->sdr, state->scStWm);

	sha_variant = bpsec_rfc9173utl_intParmGet(state, BPSEC_BHSSC_PARM_SHA_VAR_ID, BPSEC_BHSSC_SV_DEFAULT);
	switch(sha_variant)
	{
		case CSTYPE_HMAC_SHA256:
		case CSTYPE_HMAC_SHA384:
		case CSTYPE_HMAC_SHA512:

			break;
		default:
			BPSEC_DEBUG_WARN("Incorrect value for parm %s, defaulting to %d.", bpsec_scvm_byIdNameFind(gBHS_ParmMap, BPSEC_BHSSC_PARM_SHA_VAR_ID, SC_VAL_TYPE_PARM), BPSEC_BHSSC_SV_DEFAULT);
			sha_variant = BPSEC_BHSSC_SV_DEFAULT;
			break;
	}

	if(state->scRawKey.scValLength == 0)
	{
		/* Track the wrappedKey allocation immediately */
		sc_value *wrappedKey = (sc_value *) bpsec_ctx_mbuftake(&ctx, sizeof(sc_value));
		if (wrappedKey == NULL)
		{
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}

		if(bpsec_rfc9173utl_sesKeyGet(state, BPSEC_BHSSC_PARM_LTK_NAME, BPSEC_BHSSC_PARM_WRAPPED_KEY, sha_variant, &key, wrappedKey) == ERROR)
		{
			BPSEC_DEBUG_ERR("Cannot get signing key for variant %d", sha_variant);
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}

		state->scRawKey = bpsec_scv_memCsiConvert(key, SC_VAL_TYPE_PARM, CSI_PARM_BEK);

		if((lyst_insert_last(extraParms, wrappedKey)) == NULL)
		{
			BPSEC_DEBUG_ERR("%s", "Unable to store wrapped key.");
			bpsec_scv_clear(0, &(state->scRawKey));
			bpsec_scv_clear(0, wrappedKey);
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}
	}
	else
	{
		key.contents = state->scRawKey.scRawValue.asPtr;
		key.len = state->scRawKey.scValLength;
	}

	if((tgtResult->scTargetId == PayloadBlk) || (tgtResult->scTargetId == PrimaryBlk))
	{
		if((ipptZcoLen = bpsec_util_canonicalizeOut(bundle, tgtResult->scTargetId, &ipptZco)) <= 0)
		{
			BPSEC_DEBUG_ERR("Cannot canonicalize block-type specific data of " UVAST_FIELDSPEC
					".",
					(uvast) tgtResult->scTargetId);
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}
	}
	else
	{
		ipptZcoLen = 0;
		ipptZco = 0;
		addData = 1;
	}

	ippt_preamble = bpsec_rfc9173utl_authDataBuild(state, BPSEC_BHSSC_PARM_SCOPE_FLAGS, tgtResult->scTargetId, addData, bundle, NULL);

	if((ippt_preamble.scSerializedLength <= 0) || (ippt_preamble.scSerializedText == NULL))
	{
		BPSEC_DEBUG_ERR("%s", "Cannot build IPPT Data.");
		if (ipptZco != 0) zco_destroy(state->sdr, ipptZco);
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}

	if(ipptZco != 0)
	{
		csi_ctx = bpsec_bhsscutl_computeSignature(ippt_preamble, ipptZco, ipptZcoLen, sha_variant, key, CSI_SVC_SIGN);
		zco_destroy(state->sdr, ipptZco);

		if (csi_ctx == NULL)
		{
			MRELEASE(ippt_preamble.scSerializedText);
			bpsec_ctx_abort(&ctx);
			return ERROR;
		}

		result = csi_sign_finish(sha_variant, csi_ctx, &csi_result, CSI_SVC_SIGN);
		csi_ctx_free(sha_variant, csi_ctx);
	}
	else
	{
		csi_val_t input;
		input.len = ippt_preamble.scSerializedLength;
		input.contents = ippt_preamble.scSerializedText;

		result = csi_sign_full(sha_variant, input, key, &csi_result, CSI_SVC_SIGN);
	}
	MRELEASE(ippt_preamble.scSerializedText);

	if(result == ERROR)
	{
		BPSEC_DEBUG_ERR("Processing error. Returning %d.", ERROR);
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}

	/* Track the digest allocation */
	digest = (sc_value *) bpsec_ctx_mbuftake(&ctx, sizeof(sc_value));
	if(digest == NULL)
	{
		BPSEC_DEBUG_ERR("%s", "Unable to allocate digest.");
		MRELEASE(csi_result.contents);
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}
	*digest = bpsec_scv_memCsiConvert(csi_result, SC_VAL_TYPE_RESULT, BPSEC_BHSSC_RESULT_EHMAC);

	if((lyst_insert_last(state->scStResults, digest)) == NULL)
	{
		BPSEC_DEBUG_ERR("%s", "Unable to append new result.");
		bpsec_scv_clear(0, digest); /* Also handles freeing csi_result.contents */
		bpsec_ctx_abort(&ctx);
		return ERROR;
	}

	bpsec_ctx_commit(&ctx);
	return 1;
}



/******************************************************************************
 * @brief Retrieves the sc value map for this SC
 *
 * @retval !NULL - The value map for this SC
 * @retval  NULL - There was an error.
 *****************************************************************************/
sc_value_map* bpsec_bhssci_valMapGet(void)
{
	return gBHS_ParmMap;
}



/******************************************************************************
 * @brief Calculate a digest for a given set of IPPT.
 *
 * @param[in]  preamble   The parts of the IPPT that are not in a ZCO.
 * @param[in]  zcoObj     ZCO to the target block block-type-specific data
 * @param[in]  zcoLen     The length of the ZCO.
 * @param[in]  csi_suite  Which digest to use.
 * @param[in]  csi_key    The key to use for signing.
 * @param[in]  svc        Whether we are signing or verifying.
 *
 * This function is used when part of the IPPT data exists in a ZCO and, thus,
 * might be very large requiring an iterative approach to calculating a digest.
 *
 * The digest is not returned by this function. Rather, the function returns
 * the CSI context for the digest creation. This allows the caller to either
 *
 *   1. Use the context to add more data to the digest if needed later
 *   2. Finalize the context to compute the digest result as-is.
 *
 * @retval !NULL - The CSI context for the digest.
 * @retval NULL  - Error
 *****************************************************************************/

uint8_t *bpsec_bhsscutl_computeSignature(BpsecSerializeData preamble, SdrObject zcoObj, int zcoLen, int csi_suite, csi_val_t csi_key, csi_svcid_t svc)
{
	Sdr sdr = getIonsdr();
	ZcoReader dataReader;
	unsigned int zcoRemaining = zcoLen;
	unsigned int zcoRead = 0;
	unsigned int preambleRemaining = 0;
	uint32_t chunkSize = csi_blocksize(csi_suite);
	csi_val_t chunkData;
	uint8_t *csi_ctx = NULL;
	void *cursor = NULL;
	int success = 1;


	/* Step 0 - Sanity check. */
	CHKNULL(csi_key.contents);


	/* Step 1 - Allocate a working buffer. */
	if((chunkData.contents = MTAKE(chunkSize)) == NULL)
	{
		BPSEC_DEBUG_ERR("Failure to allocate chunk size of %d", chunkSize);
		return NULL;
	}
	chunkData.len = chunkSize;


	/* Step 2 - Create the CSI context for the signing. */
	csi_ctx = csi_ctx_init(csi_suite, csi_key, CSI_SVC_SIGN);

	/* Catch context allocation failure BEFORE passing to csi_sign_start */
	if (csi_ctx == NULL)
	{
		BPSEC_DEBUG_ERR("%s", "Can't init context.");
		MRELEASE(chunkData.contents);
		return NULL;
	}

	if(csi_sign_start(csi_suite, csi_ctx) == ERROR)
	{
		BPSEC_DEBUG_ERR("%s", "Can't start context.");
		MRELEASE(chunkData.contents);
		csi_ctx_free(csi_suite, csi_ctx);
		return NULL;
	}


	/* Step 3 - Set up a ZCO reader and an associated transaction. */
	zco_start_transmitting(zcoObj, &dataReader);

	if ((sdr_begin_xn(sdr)) == 0)
	{
		BPSEC_DEBUG_ERR("%s", "Can't start txn.");
		MRELEASE(chunkData.contents);
		csi_ctx_free(csi_suite, csi_ctx);
		return NULL;
	}


	/*
	 * Step 4 - Start calculating signature with preamble data until
	 *          there is less than a whole chunk worth of such data
	 *          left.
	 */
	preambleRemaining = preamble.scSerializedLength;
	cursor = preamble.scSerializedText;

	while((success) && (preambleRemaining >= chunkSize))
	{
		memcpy(chunkData.contents, cursor, chunkSize);
		cursor = (char *)cursor + chunkSize;

		/* Add the data to the context.        */
		if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
		{
			BPSEC_DEBUG_ERR("%s", "Error updating signature.");

			/* Setting success to 0 skips remaining processing. */
			success = 0;
		}

		preambleRemaining -= chunkSize;
	}


	/*
	 * Step 5 - Add remaining preamble + start of ZCO to the digest.
	 *          Account for the case where the remaining preamble and
	 *          entire ZCO fit into a single chunk.
	 */
	if((success) && (preambleRemaining > 0))
	{
		int delta = chunkSize - preambleRemaining;

		memcpy(chunkData.contents, cursor, preambleRemaining);

		cursor = chunkData.contents + preambleRemaining;

		if(delta >= 0 && (unsigned int)delta >= zcoRemaining)
		{
			delta = zcoRemaining;
			chunkData.len = preambleRemaining + zcoRemaining;
		}

		zcoRead = zco_transmit(sdr, &dataReader, delta, cursor);
		if(delta < 0 || zcoRead != (unsigned int)delta)
		{
			BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", zcoRead, delta);

			/* Setting success to 0 skips remaining processing. */
			success = 0;
		}
		else if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
		{
			BPSEC_DEBUG_ERR("%s", "Error updating signature.");

			/* Setting success to 0 skips remaining processing. */
			success = 0;
		}

		zcoRemaining -= delta;
	}


	/* Step 6 - add any remaining ZCO data to the digest. */
	while((success) && (zcoRemaining > 0))
	{
		if(zcoRemaining < chunkSize)
		{
			chunkSize = zcoRemaining;
			chunkData.len = chunkSize;
		}

		zcoRead = zco_transmit(sdr, &dataReader, chunkSize, (char *) chunkData.contents);
		if(zcoRead != chunkSize)
		{
			BPSEC_DEBUG_ERR("Read %d bytes, but expected %d.", zcoRead, chunkSize);
			success = 0;
		}
		else if(csi_sign_update(csi_suite, csi_ctx, chunkData, svc) == ERROR)
		{
			BPSEC_DEBUG_ERR("%s", "Error updating signature.");
			success = 0;
		}

		zcoRemaining -= chunkSize;
	}

	/* Step 7 - Cleanup, to include handling error. */
	sdr_exit_xn(sdr);
	MRELEASE(chunkData.contents);

	if(!success)
	{
		csi_ctx_free(csi_suite, csi_ctx);
		csi_ctx = NULL;
	}

	BPSEC_DEBUG_PROC("-->"ADDR_FIELDSPEC,(uaddr)csi_ctx);
	return csi_ctx;
}
