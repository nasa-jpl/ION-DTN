/*****************************************************************************
 **
 ** File Name: sbsp_instr.c
 **
 ** Description: Definitions supporting the collection of instrumentation
 **              for the SBSP implementation.
 **
 ** Notes:
 **  - \todo: Consider implementing the SBSP statistics using Shared Memory
 **           instead of reading and writing directly to the SDR each time.
 **  - \todo: Consider a command/function to permanently remove SBSP
 **           instrumentation from the SDR space.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/20/16  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 **  07/05/16  E. Birrane     Fixes to SDR recording.(Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "sbsp_util.h"
#include "sbsp_instr.h"

#if (SBSP_DEBUGGING == 1)
extern char		gMsg[];		/*	Debug message buffer.	*/
#endif




/******************************************************************************
 *
 * \par Function Name: getBpInstrDb
 *
 * \par Retrieve the SBSP Instrumentation counters from the SDR.
 *
 * \param[out] result  The instrumentation DB from the SDR.
 * \param[out] addr    THe loaction of the DB in the SDR.
 *
 * \par Notes:
 *    - It is assumed that the passed-in arguments have already been allocated.
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

static int getBpInstrDb(SbspInstrDB *result, Object *addr)
{
	static Object dbObj = 0;
	Sdr sdr = getIonsdr();
	sbsp_instr_misc_t misc;

    CHKERR(sdr_begin_xn(sdr));

    if(dbObj == 0)
    {
    	dbObj = sdr_find(sdr, SBSP_INSTR_SDR_NAME, NULL);

    	switch(dbObj)
    	{
    	case -1:  /* SDR error. */
    		sdr_cancel_xn(sdr);
    		*addr = 0;
    		return ERROR;
    		break;

    	case 0: /* Not found; Must create new DB. */

    		dbObj = sdr_malloc(sdr, sizeof(SbspInstrDB));
    		result->src = sdr_list_create(sdr);
    		result->misc = sdr_malloc(sdr, sizeof(sbsp_instr_misc_t));
    		memset(&misc, 0, sizeof(sbsp_instr_misc_t));
    		sdr_write(sdr, result->misc, (char *) &misc,
				sizeof(sbsp_instr_misc_t));
    		sdr_write(sdr, dbObj, (char *) result, sizeof(SbspInstrDB));
    		sdr_catlg(sdr, SBSP_INSTR_SDR_NAME, 0, dbObj);
    		break;

    	default: /* Found. */
    		sdr_read(sdr, (char *) result, dbObj, sizeof(SbspInstrDB));
    		break;
    	}
    }
    else
    {
		sdr_read(sdr, (char *) result, dbObj, sizeof(SbspInstrDB));
    }

    *addr = dbObj;
	sdr_end_xn(sdr);

    return 1;
}




/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_src
 *
 * \par Retrieve the SBSP Instrumentation counters from the SDR.
 *
 * \param[in]  eid      The source being queried, or NULL for anonymous
 * \param[out] result   The data for the source.
 * \param[out] sdrElt   The SDR ELT holding the address of the result.
 * \param[out] sdrData  The SDR address of the result.
 *
 * \par Notes:
 *    - It is assumed that any mutex around the data being queried has been
 *      taken by the calling function.
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

static int	sbsp_instr_get_src(char *eid, sbsp_src_instr_t *result,
			Object *sdrElt, Object *sdrData)
{
	Sdr sdr = getIonsdr();
	SbspInstrDB instr_db;
	Object elt;
	Object data;
	Object dbObj;
	CHKERR(result);
	CHKERR(sdrElt);

	CHKERR(sdr_begin_xn(sdr));

	if(getBpInstrDb(&instr_db, &dbObj) == ERROR)
	{
		sdr_cancel_xn(sdr);
		return ERROR;
	}

	if(eid == NULL)
	{
		sbsp_instr_misc_t tmp;
		sdr_read(sdr, (char *) &tmp, instr_db.misc,
				sizeof(sbsp_instr_misc_t));
		*result = tmp.anon;
		*sdrData = instr_db.misc;
		*sdrElt = 0;
		sdr_end_xn(sdr);

		return 1;
	}

	elt = sdr_list_first(sdr, instr_db.src);

	for (elt = sdr_list_first(sdr, instr_db.src); elt;
			elt = sdr_list_next(sdr, elt))
	{

		data = sdr_list_data(sdr, elt);

		/* Read in source item. */
		sdr_read(sdr, (char *) result, data, sizeof(sbsp_src_instr_t));

		if(strcmp(result->eid, eid) == 0)
		{
			*sdrElt = elt;
			*sdrData = data;
			sdr_end_xn(sdr);

			return 1;
		}
	}

	sdr_end_xn(sdr);

	return -1;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_update
 *
 * \par Update the SBSP Instrumentation counters for a source.
 *
 * \param[in]  eid      The source being updated, or NULL for anonymous
 * \param[in]  blk      The # blocks counter being updated
 * \param[in]  bytes    The # bytes counter being updated
 * \param[in]  type     The type of data being updated.
 *
 * \par Notes:
 *    - It is assumed that any mutex around the data being queried has been
 *      taken by the calling function.
 *    - It is assumed that blocks and bytes must be updated together because
 *      any new bytes will be paet of one or more new blocks, and any new blocks
 *      will generate new bytes.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation.
			     (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void sbsp_instr_update(char *src, uvast blk, uvast bytes, sbsp_instr_type_e type)
{
	Sdr sdr = getIonsdr();
	sbsp_src_instr_t instr;
	Object sdrElt = 0;
	Object sdrData = 0;

	CHKVOID(sdr_begin_xn(sdr));

	/* If we can't find the source to update, then add it. */
	if((sbsp_instr_get_src(src, &instr, &sdrElt, &sdrData)) == ERROR)
	{
		SbspInstrDB result;
		Object dbObj;

		if(getBpInstrDb(&result, &dbObj) == ERROR)
		{
			SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
			sdr_cancel_xn(sdr);
			return;
		}

		memset(&instr, 0, sizeof(sbsp_src_instr_t));
		istrcpy(instr.eid, src, MAX_EID_LEN);

		if ((sdrData = sdr_insert(sdr, (char *) &instr,
				sizeof(sbsp_src_instr_t))) == 0)
		{
			SBSP_DEBUG_ERR("Can't allocate %d bytes to SDR.",
					sizeof(sbsp_src_instr_t));
			sdr_cancel_xn(sdr);
			return;
		}

		sdrElt = sdr_list_insert_last(sdr, result.src, sdrData);
	}


	switch(type)
	{
	case BCB_TX_PASS:
		instr.bcb_blk_tx_pass += blk;
		instr.bcb_byte_tx_pass += bytes;
		break;
	case BCB_TX_FAIL:
		instr.bcb_blk_tx_fail += blk;
		instr.bcb_byte_tx_fail += bytes;
		break;
	case BCB_RX_PASS:
		instr.bcb_blk_rx_pass += blk;
		instr.bcb_byte_rx_pass += bytes;
		break;
	case BCB_RX_FAIL:
		instr.bcb_blk_rx_fail += blk;
		instr.bcb_byte_rx_fail += bytes;
		break;
	case BCB_RX_MISS:
		instr.bcb_blk_rx_miss += blk;
		instr.bcb_byte_rx_miss += bytes;
		break;
	case BCB_FWD:
		instr.bcb_blk_fwd += blk;
		instr.bcb_byte_fwd += bytes;
		break;
	case BIB_TX_PASS:
		instr.bib_blk_tx_pass += blk;
		instr.bib_byte_tx_pass += bytes;
		break;
	case BIB_TX_FAIL:
		instr.bib_blk_tx_fail += blk;
		instr.bib_byte_tx_fail += bytes;
		break;
	case BIB_RX_PASS:
		instr.bib_blk_rx_pass += blk;
		instr.bib_byte_rx_pass += bytes;
		break;
	case BIB_RX_FAIL:
		instr.bib_blk_rx_fail += blk;
		instr.bib_byte_rx_fail += bytes;
		break;
	case BIB_RX_MISS:
		instr.bib_blk_rx_miss += blk;
		instr.bib_byte_rx_miss += bytes;
		break;
	case BIB_FWD:
		instr.bib_blk_fwd += blk;
		instr.bib_byte_fwd += bytes;
		break;
	}
	instr.last_update = getCtime();


	sdr_write(sdr, sdrData, (char *) &instr, sizeof(sbsp_src_instr_t));

 	sdr_end_xn(sdr);
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_cleanup
 *
 * \par Cleans up any memory resources taken by the instrumentation counters.
 *
 * \par Notes:
 *    - Since everything is written to, and read from, SDR each time, there is
 *      nothing to do here.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void sbsp_instr_cleanup()
{

}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_clear_src
 *
 * \par Clears the counters associated with a given source and updated the
 *      last reset time.
 *
 * \param[in]  sdrElt     The location of the ELT holding the source data in
 *                        the SDR.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void sbsp_instr_clear_src(Object sdrElt)
{
	Sdr sdr = getIonsdr();
	Object sdrData = 0;

	if(sdrElt == 0)
	{
		return;
	}

	CHKVOID(sdr_begin_xn(sdr));

	if((sdrData = sdr_list_data(sdr, sdrElt)) != 0)
	{
		sdr_free(sdr, sdrData);
	}

	sdr_list_delete(sdr, sdrElt, NULL, NULL);

	sdr_end_xn(sdr);
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_misc
 *
 * \par Retrieve statistics for miscellaneous sources.
 *
 * \param[out] result  The statistics for the misc. sources.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  07/05/16  E. Birrane     Init result to 0. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_get_misc(sbsp_instr_misc_t *result)
{
	Sdr sdr = getIonsdr();
	SbspInstrDB instr_db;
	Object dbObj;

	CHKERR(result);

	if(getBpInstrDb(&instr_db, &dbObj) == ERROR)
	{
		return ERROR;
	}

	memset(result, 0, sizeof(sbsp_instr_misc_t));

	CHKERR(sdr_begin_xn(sdr));
	sdr_read(sdr, (char *) result, instr_db.misc,
			sizeof(sbsp_instr_misc_t));
	sdr_end_xn(sdr);

	return 1;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_instr_clear
 *
 * \par Clear all SBSP Instrumentation statistics.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_clear()
{
	Sdr sdr = getIonsdr();
	SbspInstrDB result;
	sbsp_instr_misc_t tmp;
	Object sdrElt;
	Object dbObj;

	if(getBpInstrDb(&result, &dbObj) == ERROR)
	{
		SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
		return ERROR;
	}

	/* Clear misc instr. data. */
	memset(&tmp, 0, sizeof(sbsp_instr_misc_t));

	CHKERR(sdr_begin_xn(sdr));

	sdr_write(sdr, result.misc, (char *) &tmp, sizeof(sbsp_instr_misc_t));

	/* Clear each source. */
	for(sdrElt = sdr_list_first(sdr, result.src); sdrElt;
			sdrElt = sdr_list_first(sdr, sdrElt))
	{
		sbsp_instr_clear_src(sdrElt);
	}

	sdr_end_xn(sdr);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_src_blk
 *
 * \par Retrieve block statistics for a single SBSP source.
 *
 * \param[in]  src_id  The source whose statistics are being queried.
 * \param[in]  type    The type of block statistic being queried.
 * \param[out] result  The block statistic.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int  sbsp_instr_get_src_blk(char *src_id, sbsp_instr_type_e type, uvast *result)
{
	sbsp_src_instr_t src;
	Object sdrElt = 0;
	Object sdrData = 0;
	CHKERR(result);

	if(sbsp_instr_get_src(src_id, &src, &sdrElt, &sdrData) == ERROR)
	{
		/*	Not necessarily an error, if query a source
			that does not exist.				*/
		SBSP_DEBUG_INFO("sbsp_instr_get_src_blk",
				"Can't get id for src %s", src_id);
		return ERROR;
	}

	switch(type)
	{
	case BCB_TX_PASS: *result = src.bcb_blk_tx_pass; break;
	case BCB_TX_FAIL: *result = src.bcb_blk_tx_fail; break;
	case BCB_RX_PASS: *result = src.bcb_blk_rx_pass; break;
	case BCB_RX_FAIL: *result = src.bcb_blk_rx_fail; break;
	case BCB_RX_MISS: *result = src.bcb_blk_rx_miss; break;
	case BCB_FWD:     *result = src.bcb_blk_fwd; break;
	case BIB_TX_PASS: *result = src.bib_blk_tx_pass; break;
	case BIB_TX_FAIL: *result = src.bib_blk_tx_fail; break;
	case BIB_RX_PASS: *result = src.bib_blk_rx_pass; break;
	case BIB_RX_FAIL: *result = src.bib_blk_rx_fail; break;
	case BIB_RX_MISS: *result = src.bib_blk_rx_miss; break;
	case BIB_FWD:     *result = src.bib_blk_fwd; break;
	default:
		SBSP_DEBUG_ERR("sbsp_instr_get_src_blk","Unknown type %d",
				type);
		return ERROR;
	}

	return 0;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_src_bytes
 *
 * \par Retrieve bytes statistics for a single SBSP source.
 *
 * \param[in]  src_id  The source whose statistics are being queried.
 * \param[in]  type    The type of byte statistic being queried.
 * \param[out] result  The byte statistic.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int  sbsp_instr_get_src_bytes(char *src_id, sbsp_instr_type_e type, uvast *result)
{
	sbsp_src_instr_t src;
	Object sdrElt = 0;
	Object sdrData = 0;
	CHKERR(result);

	if(sbsp_instr_get_src(src_id, &src, &sdrElt, &sdrData) == ERROR)
	{
		/*	Not necessarily an error, if query a source
			that does not exist.				*/
		SBSP_DEBUG_INFO("sbsp_instr_get_src_bytes",
				"Can't get id for src %c", src_id);
		return ERROR;
	}

	switch(type)
	{
	case BCB_TX_PASS: *result = src.bcb_byte_tx_pass; break;
	case BCB_TX_FAIL: *result = src.bcb_byte_tx_fail; break;
	case BCB_RX_PASS: *result = src.bcb_byte_rx_pass; break;
	case BCB_RX_FAIL: *result = src.bcb_byte_rx_fail; break;
	case BCB_RX_MISS: *result = src.bcb_byte_rx_miss; break;
	case BCB_FWD:     *result = src.bcb_byte_fwd; break;
	case BIB_TX_PASS: *result = src.bib_byte_tx_pass; break;
	case BIB_TX_FAIL: *result = src.bib_byte_tx_fail; break;
	case BIB_RX_PASS: *result = src.bib_byte_rx_pass; break;
	case BIB_RX_FAIL: *result = src.bib_byte_rx_fail; break;
	case BIB_RX_MISS: *result = src.bib_byte_rx_miss; break;
	case BIB_FWD:     *result = src.bib_byte_fwd; break;
	default:
		SBSP_DEBUG_ERR("sbsp_instr_get_src_bytes","Unknown type %d",
				type);
		return ERROR;
	}

	return 0;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_src_update
 *
 * \par Retrieve the last update time for a SBSP source.
 *
 * \param[in]  src_id  The source whose update time is being queried.
 * \param[out] result  The update time.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_get_src_update(char *src_id, time_t *result)
{
	sbsp_src_instr_t src;
	Object sdrElt = 0;
	Object sdrData = 0;

	CHKERR(result);

	if(sbsp_instr_get_src(src_id, &src, &sdrElt, &sdrData) == ERROR)
	{
		SBSP_DEBUG_ERR("sbsp_instr_get_src_bytes",
				"Can't get id for src %c", src_id);
		return ERROR;
	}

	*result = src.last_update;

	return 0;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_total_blk
 *
 * \par Retrieve block statistics summed across all sources
 *
 * \param[in]  type    The block statistic being queried.
 * \param[out] result  The block statistic value.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_get_total_blk(sbsp_instr_type_e type, uvast *result)
{
	sbsp_src_instr_t src;
	sbsp_instr_misc_t misc;
	Sdr sdr = getIonsdr();
	SbspInstrDB instr_db;
	Object elt;
	Object addr;
	Object dbObj;

	CHKERR(result);

	*result = 0;

	if(getBpInstrDb(&instr_db, &dbObj) == ERROR)
	{
		SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
		return ERROR;
	}

	CHKERR(sdr_begin_xn(sdr));

	sdr_read(sdr, (char *) &misc, instr_db.misc, sizeof(sbsp_instr_misc_t));

	switch(type)
	{
	case BCB_TX_PASS: *result += misc.anon.bcb_blk_tx_pass; break;
	case BCB_TX_FAIL: *result += misc.anon.bcb_blk_tx_fail; break;
	case BCB_RX_PASS: *result += misc.anon.bcb_blk_rx_pass; break;
	case BCB_RX_FAIL: *result += misc.anon.bcb_blk_rx_fail; break;
	case BCB_RX_MISS: *result += misc.anon.bcb_blk_rx_miss; break;
	case BCB_FWD:     *result += misc.anon.bcb_blk_fwd; break;
	case BIB_TX_PASS: *result += misc.anon.bib_blk_tx_pass; break;
	case BIB_TX_FAIL: *result += misc.anon.bib_blk_tx_fail; break;
	case BIB_RX_PASS: *result += misc.anon.bib_blk_rx_pass; break;
	case BIB_RX_FAIL: *result += misc.anon.bib_blk_rx_fail; break;
	case BIB_RX_MISS: *result += misc.anon.bib_blk_rx_miss; break;
	case BIB_FWD:     *result += misc.anon.bib_blk_fwd; break;
	default:
		SBSP_DEBUG_ERR("sbsp_instr_get_total_blk","Unknown type %d",
				type);
		sdr_cancel_xn(sdr);

		return ERROR;
	}

	/* Clear each source. */
	for(elt = sdr_list_first(sdr, instr_db.src); elt;
			elt = sdr_list_first(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &src, addr, sizeof(sbsp_src_instr_t));

		switch(type)
		{
		case BCB_TX_PASS: *result += src.bcb_blk_tx_pass; break;
		case BCB_TX_FAIL: *result += src.bcb_blk_tx_fail; break;
		case BCB_RX_PASS: *result += src.bcb_blk_rx_pass; break;
		case BCB_RX_FAIL: *result += src.bcb_blk_rx_fail; break;
		case BCB_RX_MISS: *result += src.bcb_blk_rx_miss; break;
		case BCB_FWD:     *result += src.bcb_blk_fwd; break;
		case BIB_TX_PASS: *result += src.bib_blk_tx_pass; break;
		case BIB_TX_FAIL: *result += src.bib_blk_tx_fail; break;
		case BIB_RX_PASS: *result += src.bib_blk_rx_pass; break;
		case BIB_RX_FAIL: *result += src.bib_blk_rx_fail; break;
		case BIB_RX_MISS: *result += src.bib_blk_rx_miss; break;
		case BIB_FWD:     *result += src.bib_blk_fwd; break;
		default:
			SBSP_DEBUG_ERR("sbsp_instr_get_total_blk",
					"Unknown type %d", type);
			sdr_cancel_xn(sdr);

			return ERROR;
		}
	}

	sdr_end_xn(sdr);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_total_bytes
 *
 * \par Retrieve byte statistics summed across all sources
 *
 * \param[in]  type    The byte statistic being queried.
 * \param[out] result  The byte statistic value.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_get_total_bytes(sbsp_instr_type_e type, uvast *result)
{
	sbsp_src_instr_t src;
	sbsp_instr_misc_t misc;
	Sdr sdr = getIonsdr();
	SbspInstrDB instr_db;
	Object elt;
	Object addr;
	Object dbObj;

	CHKERR(result);

	*result = 0;

	if(getBpInstrDb(&instr_db, &dbObj) == ERROR)
	{
		SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
		return ERROR;
	}

	CHKERR(sdr_begin_xn(sdr));

	oK(sdr_read(sdr, (char *) &misc, instr_db.misc,
			sizeof(sbsp_instr_misc_t)));

	switch(type)
	{
	case BCB_TX_PASS: *result += misc.anon.bcb_byte_tx_pass; break;
	case BCB_TX_FAIL: *result += misc.anon.bcb_byte_tx_fail; break;
	case BCB_RX_PASS: *result += misc.anon.bcb_byte_rx_pass; break;
	case BCB_RX_FAIL: *result += misc.anon.bcb_byte_rx_fail; break;
	case BCB_RX_MISS: *result += misc.anon.bcb_byte_rx_miss; break;
	case BCB_FWD:     *result += misc.anon.bcb_byte_fwd; break;
	case BIB_TX_PASS: *result += misc.anon.bib_byte_tx_pass; break;
	case BIB_TX_FAIL: *result += misc.anon.bib_byte_tx_fail; break;
	case BIB_RX_PASS: *result += misc.anon.bib_byte_rx_pass; break;
	case BIB_RX_FAIL: *result += misc.anon.bib_byte_rx_fail; break;
	case BIB_RX_MISS: *result += misc.anon.bib_byte_rx_miss; break;
	case BIB_FWD:     *result += misc.anon.bib_byte_fwd; break;
	default:
		SBSP_DEBUG_ERR("sbsp_instr_get_total_byte","Unknown type %d",
				type);
		sdr_cancel_xn(sdr);

		return ERROR;
	}

	for(elt = sdr_list_first(sdr, instr_db.src); elt;
			elt = sdr_list_first(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &src, addr, sizeof(sbsp_src_instr_t));

		switch(type)
		{
		case BCB_TX_PASS: *result += src.bcb_byte_tx_pass; break;
		case BCB_TX_FAIL: *result += src.bcb_byte_tx_fail; break;
		case BCB_RX_PASS: *result += src.bcb_byte_rx_pass; break;
		case BCB_RX_FAIL: *result += src.bcb_byte_rx_fail; break;
		case BCB_RX_MISS: *result += src.bcb_byte_rx_miss; break;
		case BCB_FWD:     *result += src.bcb_byte_fwd; break;
		case BIB_TX_PASS: *result += src.bib_byte_tx_pass; break;
		case BIB_TX_FAIL: *result += src.bib_byte_tx_fail; break;
		case BIB_RX_PASS: *result += src.bib_byte_rx_pass; break;
		case BIB_RX_FAIL: *result += src.bib_byte_rx_fail; break;
		case BIB_RX_MISS: *result += src.bib_byte_rx_miss; break;
		case BIB_FWD:     *result += src.bib_byte_fwd; break;
		default:
			SBSP_DEBUG_ERR("sbsp_instr_get_total_byte",
					"Unknown type %d", type);
			sdr_cancel_xn(sdr);

			return ERROR;
		}
	}

	sdr_end_xn(sdr);

	return 1;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_tot_update
 *
 * \par Retrieve the last update time for any SBSP source.
 *
 * \param[out] result  The last update time.
 *
 * \par Notes:
 *
 * \return 1 - Success
 *        ERROR - Failure
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int  sbsp_instr_get_tot_update(time_t *result)
{
	sbsp_src_instr_t src;
	Sdr sdr = getIonsdr();
	SbspInstrDB instr_db;
	Object elt;
	Object addr;
	Object dbObj;

	CHKERR(result);

	*result = 0;

	if(getBpInstrDb(&instr_db, &dbObj) == ERROR)
	{
		SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
		return ERROR;
	}

	CHKERR(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, instr_db.src); elt;
			elt = sdr_list_first(sdr, elt))
	{
		addr = sdr_list_data(sdr, elt);
		sdr_read(sdr, (char *) &src, addr, sizeof(sbsp_src_instr_t));

		if(src.last_update > *result)
		{
			*result = src.last_update;
		}
	}
	sdr_end_xn(sdr);

	return 1;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_num_keys
 *
 * \par Retrieve the number of keys known to SBSP.
 *
 * \par Notes:
 *
 * \return # Keys
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

uint32_t sbsp_instr_get_num_keys()
{
	int size;
	return (uint32_t) sec_get_sbspNumKeys(&size);
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_keynames
 *
 * \par Retrieve the key names known to SBSP as a comma-separated string.
 *
 * \par Notes:
 *
 * \return NULL - Error
 *         !NULL - Key name string.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

char *sbsp_instr_get_keynames()
{
	int size = 0;
	int num_keys = 0;
	uint32_t total_size = 0;
	char *result = NULL;

	num_keys = sec_get_sbspNumKeys(&size);

	/* Total size is size of each key, plus 1 character
	 * per key for a comma to separate values, plus
	 * a NULL terminator.
	 */
	total_size = (num_keys * size) + num_keys + 1;
	if((result = MTAKE(total_size)) == NULL)
	{
		SBSP_DEBUG_ERR("x sbsp_instr_get_keynames: Can't allocate %d \
bytes", total_size);
		return NULL;
	}

	sec_get_sbspKeys(result, total_size);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_csnames
 *
 * \par Retrieve the ciphersuite names known to SBSP as a comma-separated string.
 *
 * \par Notes:
 *
 * \return NULL - Error
 *         !NULL - ciphersuite name string.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

char * sbsp_instr_get_csnames()
{
	int size = 0;
	int num = 0;
	uint32_t total_size = 0;
	char *result = NULL;

	num = sec_get_sbspNumCSNames(&size);

	/* Total size is size of each key, plus 1 character
	 * per key for a comma to separate values, plus
	 * a NULL terminator.
	 */
	total_size = (num * size) + num + 1;
	if((result = MTAKE(total_size)) == NULL)
	{
		SBSP_DEBUG_ERR("x sbsp_instr_get_csnames: Can't allocate %d \
bytes", total_size);
		return NULL;
	}

	sec_get_sbspCSNames(result, total_size);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_get_srcnames
 *
 * \par Retrieve the sources known to SBSP as a comma-separated string.
 *
 * \par Notes:
 *    - \todo We pre-allocate space for full EID names, which is space wasteful,
 *      but walking the list to calculate name sizes in real time is time wasteful.
 *      Need to design a more efficient approach.
 *
 * \return NULL - Error
 *         !NULL - sources name string.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *  07/05/16  E. Birrane     Updated to use source names from SDR. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

char * sbsp_instr_get_srcnames()
{
	Sdr sdr = getIonsdr();
	SbspInstrDB result;
	Object addr = 0;
	Object data = 0;
	sbsp_src_instr_t tmp;
	Object sdrElt = 0;
	uint32_t num = 0;
	uint32_t total_size = 0;
	char *names = NULL;
	char *cursor = NULL;
	int first = 0;

	if(getBpInstrDb(&result, &addr) == ERROR)
	{
		SBSP_DEBUG_ERR("Can't retrieve SBSP Instr DB.", NULL);
		return NULL;
	}

	CHKNULL(sdr_begin_xn(sdr));

	num = sdr_list_length(sdr, result.src);

	/* Total size is size of each source name, plus 1 character
	 * per key for a comma to separate values, plus
	 * a NULL terminator.
	 */
	total_size = (num * MAX_EID_LEN) + num + 1;
	if((names = MTAKE(total_size)) == NULL)
	{
		SBSP_DEBUG_ERR("x sbsp_instr_get_srcnames: Can't allocate %d \
bytes", total_size);
		sdr_cancel_xn(sdr);
		return NULL;
	}

	/* Collect the names. */
	memset(names,0,total_size);
	cursor = names;
	for(sdrElt = sdr_list_first(sdr, result.src); sdrElt;
			sdrElt = sdr_list_first(sdr, sdrElt))
	{
		data = sdr_list_data(sdr, sdrElt);

     	/* Read in source item. */
		sdr_read(sdr, (char *) &tmp, data, sizeof(sbsp_src_instr_t));

		if(first == 0)
		{
			first = 1;
		}
		else
		{
			*cursor = ',';
			cursor++;
		}
		memcpy(cursor, tmp.eid, strlen(tmp.eid));
		cursor += strlen(tmp.eid);
	}

	sdr_end_xn(sdr);

	return names;
}


/******************************************************************************
 *
 * \par Function Name: sbsp_instr_init
 *
 * \par Initialize the SBSP Instrumentation.
 *
 * \par Notes:
 *    - \todo: This function may not be needed with the SDR_only approach, and
 *       should be revisited if the design does to a shared memory approach.
 *
 * \return ERROR - Error
 *         1 - Success.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

int sbsp_instr_init()
{
	SbspInstrDB tmp;
	Object dbObj;

	return getBpInstrDb(&tmp, &dbObj);
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_reset
 *
 * \par Reset all SBSP instrumentation.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void sbsp_instr_reset()
{
	sbsp_instr_clear();
}



/******************************************************************************
 *
 * \par Function Name: sbsp_instr_reset_src
 *
 * \par Reset SBSP instrumentation for a specific source.
 *
 * \param [in]  src_id  The source whose counters should be reset.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/20/16  E. Birrane     Initial implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

void sbsp_instr_reset_src(char *src_id)
{
	sbsp_src_instr_t src;
	Object sdrElt = 0;
	Object sdrData = 0;

	if(sbsp_instr_get_src(src_id, &src, &sdrElt, &sdrData) == ERROR)
	{
		return;
	}

	sbsp_instr_clear_src(sdrElt);

}


/******************************************************************************
      These are drafts of functions to use shared memory instead of SDR. They
      are not complete and likely very incorrect and are kept here as a
      starting point for any future effort to move this to using shared memory.

typedef struct
{
	sm_SemId mutex;
	PsmAddress src;
	sbsp_instr_misc_t misc;
} SbspInstrVdb;


static SbspInstrVdb *getBpInstrVDb()
{
	static SbspInstrVdb	*vdb = NULL;
	PsmPartition	wm;
	PsmAddress	vdbAddress;
	PsmAddress	elt;
	Sdr		sdr;

	char name[32] = "sbsp_instr";

	/ * On the first function call, grab the virtual db. * /
	if(vdb == NULL)
	{

		/ *	Attaching to volatile database.			* /
		wm = getIonwm();

		if (psm_locate(wm, name, &vdbAddress, &elt) < 0)
		{
			putErrmsg("Failed searching for sbsp vdb.", NULL);
			return vdb;
		}

		if (elt)
		{
			vdb = (SbspInstrVdb*) psp(wm, vdbAddress);
			return vdb;
		}
		else
		{
			sdr = getIonsdr();
			CHKNULL(sdr_begin_xn(sdr));/ *	To lock memory.	* /
			vdbAddress = psm_zalloc(wm, sizeof(SbspInstrVdb));
			if (vdbAddress == 0)
			{
				sdr_cancel_xn(sdr);
				putErrmsg("No space for sbsp dynamic database.",
						NULL);
				return NULL;
			}

			vdb = (SbspInstrVdb *) psp(wm, vdbAddress);

			memset((char *) vdb, 0, sizeof(SbspInstrVdb));

			/ *	Volatile database doesn't exist yet.	* /
			if(((vdb->src = sm_list_create(wm)) == 0) ||
			  (psm_catlg(wm, name, vdbAddress) < 0))
			{
				sdr_cancel_xn(sdr);
				putErrmsg("Can't initialize sbsp volatile \
database.", NULL);
				return NULL;
			}

			vdb->misc.last_reset = getCtime();
			vdb->mutex = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);

			/ * Read in data from SDR. * /
			sdr_end_xn(sdr);	/ *	Unlock memory.	* /
		}
	}

	return vdb;
}


static sbsp_src_instr_t *sbsp_instr_get_src(char *eid, Object *addr)
{
	PsmAddress elt;
	PsmPartition bpwm = getIonwm();
	SbspInstrVdb *instr_vdb = getBpInstrDb();

	CHKNULL(instr_vdb);

	if(eid == NULL)
	{
		return &(instr_vdb->misc.anon);
	}

	for (elt = sm_list_first(bpwm, instr_vdb->src); elt;
			elt = sm_list_next(bpwm, elt))
	{
		PsmAddress addr = sm_list_data(bpwm, elt);
		sbsp_src_instr_t *instr = (sbsp_src_instr_t*) psp(bpwm, addr);

		if(instr != NULL)
		{
			if(strlen(eid) == strlen(instr->eid))
			{
				if(memcmp(eid, instr->eid, strlen(eid)) == 0)
				{
					return instr;
				}
			}
		}
	}

	return NULL;
}


void sbsp_instr_cleanup()
{

	SbspInstrVdb *instr_vdb = getBpInstrDb();
	PsmAddress	elt;
	PsmPartition	bpwm = getIonwm();
	PsmAddress	addr;

	CHKVOID(instr_vdb);

	while ((elt = sm_list_first(bpwm, instr_vdb->src)) != 0)
	{
		addr = sm_list_data(bpwm, elt);

		oK(sm_list_delete(bpwm, elt, NULL, NULL));
		psm_free(bpwm, addr);
	}

	sm_list_destroy(bpwm, instr_vdb->src, NULL, NULL);

	sm_SemDelete(instr_vdb->mutex);


	PsmAddress	vdbAddress;
	char name[32] = "sbsp_instr";
	psm_locate(bpwm, name, &vdbAddress, &elt);
	psm_free(bpwm, vdbAddress);
}

int sbsp_instr_init()
{
	Sdr sdr = getIonsdr();
	SbspInstrVdb *instr_vdb = getBpInstrDb();
	SbspInstrDB instr_db;
	Object dbObj;
    Object elt;
    sbsp_src_instr_t* cur_src;
    sbsp_src_instr_t* tmp_src;
    PsmAddress	addr;
	PsmPartition	bpwm = getIonwm();

    CHKERR(instr_vdb);

	/ * Initialize the non-volatile database. * /
	memset((char*) &instr_db, 0, sizeof(SbspInstrDB));

	/ * Recover the SBSP Instr database, creating it if necessary. * /
	CHKERR(sdr_begin_xn(sdr));

	dbObj = sdr_find(sdr, "sbsp_instr", NULL);

	switch(dbObj)
	{
		case -1:  / / SDR error. * /
			sdr_cancel_xn(sdr);
			return -1;
			break;

		case 0: / / Not found; Must create new DB. * /
			dbObj = sdr_malloc(sdr, sizeof(SbspInstrDB));
			instr_db.src = sdr_list_create(sdr);
			instr_db.misc = sdr_malloc(sdr,
					sizeof(sbsp_instr_misc_t));

			sdr_write(sdr, dbObj, (char *) &instr_db,
					sizeof(SbspInstrDB));
			sdr_catlg(sdr, "sbsp_instr", 0, dbObj);
			break;

		default:  / * Found DB in the SDR * /
			/ * Read in the Database. * /
			sdr_read(sdr, (char *) &instr_db, dbObj,
					sizeof(SbspInstrDB));
			sdr_read(sdr, (char *) &(instr_vdb->misc),
					instr_db.misc,
					sizeof(sbsp_instr_misc_t));

			for (elt = sdr_list_first(sdr, instr_db.src); elt;
					elt = sdr_list_next(sdr, elt))
			{

				/ *	Create space for source item
					being read in from DB.		* /
				if((addr = psm_malloc(bpwm,
						sizeof(sbsp_src_instr_t))) == 0)
				{
					sdr_cancel_xn(sdr);
					return ERROR;
				}
				cur_src = (sbsp_src_instr_t *) psp(bpwm, addr);

				/ * Read in source item. * /
				sdr_read(sdr, (char *) cur_src,
						sdr_list_data(sdr, elt),
						sizeof(sbsp_src_instr_t));

				/ *	See if VDB already has this
					item.  If so, DB overwrites DB.
					If not, just add the read-in
					item to the list.		* /
				if ((tmp_src = sbsp_instr_get_src(cur_src->eid))
						== NULL)
				{
					if ((sm_list_insert_last(bpwm,
						instr_vdb->src, addr)) == 0)
					{
						psm_free(bpwm, addr);
					}
				}
				else
				{
					*tmp_src = *cur_src;
					psm_free(bpwm, addr);
				}
			}
			break;
	}

	if(sdr_end_xn(sdr))
	{
		return -1;
	}

	return 1;
}


void sbsp_instr_reset()
{
	PsmAddress elt;
	PsmPartition bpwm = getIonwm();
	sbsp_src_instr_t *instr = NULL;
	SbspInstrVdb *instr_vdb = getBpInstrDb();

	CHKVOID(instr_vdb);

	sm_SemTake(instr_vdb->mutex);

	sbsp_instr_clear_src(&(instr_vdb->misc.anon));


	for(elt = sm_list_first(bpwm, instr_vdb->src); elt;
			elt = sm_list_next(bpwm, elt))
	{
		PsmAddress addr = sm_list_data(bpwm, elt);
		instr = (sbsp_src_instr_t *) psp(bpwm, addr);
		sbsp_instr_clear_src(instr);
	}
	instr_vdb->misc.last_reset = getCtime();

	sm_SemGive(instr_vdb->mutex);
}


void sbsp_instr_reset_src(char *src)
{
	sbsp_src_instr_t *instr = NULL;
	SbspInstrVdb *instr_vdb = getBpInstrDb();

	CHKVOID(instr_vdb);
	sm_SemTake(instr_vdb->mutex);

	if((instr = sbsp_instr_get_src(src)) != NULL)
	{
		sbsp_instr_clear_src(instr);
	}

	sm_SemGive(instr_vdb->mutex);
}
******************************************************************************/
