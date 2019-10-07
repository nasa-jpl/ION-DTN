/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2013 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file report.h
 **
 **
 ** Description: This file defines the structure of reports and report
 **              templates in the AMP.
 **
 ** Notes:
 **		1. The current implementation only supports a single report
 **		   recipient.
 **		2. Parameter maps only support 16 parameters.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/11/13  E. Birrane     Redesign of primitives architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/02/15  E. Birrane     Migrated to Typed Data Collections (TDCs) (Secure DTN - NASA: NNX14CS58P)
 **  09/28/18  E. Birrane     Update to latest AMP v0.5. (JHU/APL)
 *****************************************************************************/


#ifndef _REPORT_H_
#define _REPORT_H_

#include "../utils/nm_types.h"
#include "../utils/db.h"
#include "tnv.h"
#include "ari.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define RPT_DEFAULT_ENC_SIZE 1024
#define RPTTPL_DEFAULT_ENC_SIZE 1024

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */
#define RPT_MAP_GET_SRC_IDX(map) (map >> 4)
#define RPT_MAP_GET_DEST_IDX(map) (map & 0xF)

#define RPT_MAP_SET_SRC_IDX(map, item) (map |= ((item & 0xF) << 4))
#define RPT_MAP_SET_DEST_IDX(map, item) (map |= (item & 0xF))

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



/**
 * Describes a report template.
 *
 */

typedef struct
{
	ari_t *id;		   /**> The template id.   */
	ac_t contents;     /**> Each item is of type (ari_t *)*/

	db_desc_t desc;    /**> Descriptor of def in the SDR. */
} rpttpl_t;



typedef struct {

	time_t time;        /**> Time the report entries were generated. */
	ari_t *id;
	tnvc_t *entries;

    /* Non-serialized portions. */
    eid_t recipient;
} rpt_t;




/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int      rpt_add_entry(rpt_t *rpt, tnv_t *entry);

int      rpt_cb_comp_fn(void *i1, void *i2);

void     rpt_cb_del_fn(void *item);

void     rpt_clear(rpt_t *rpt);

rpt_t*   rpt_copy_ptr(rpt_t *src);

rpt_t*   rpt_create(ari_t *id, time_t time, tnvc_t *entries);

void*    rpt_deserialize_ptr(QCBORDecodeContext *it, int *success);

rpt_t*   rpt_deserialize_raw(blob_t *data, int *success);

void     rpt_release(rpt_t *rpt, int destroy);


int      rpt_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*   rpt_serialize_wrapper(rpt_t *rpt);


int       rpttpl_add_item(rpttpl_t *rpttpl, ari_t *item);


int       rpttpl_cb_comp_fn(void *i1, void *i2);
void      rpttpl_cb_del_fn(void *item);
void      rpttpl_cb_ht_del_fn(rh_elt_t *elt);

rpttpl_t *rpttpl_copy_ptr(rpttpl_t *rpttpl);

rpttpl_t* rpttpl_create(ari_t *id, ac_t items);

rpttpl_t* rpttpl_create_id(ari_t *id);

rpttpl_t* rpttpl_deserialize_ptr(QCBORDecodeContext *it, int *success);

rpttpl_t* rpttpl_deserialize_raw(blob_t *data, int *success);

void      rpttpl_release(rpttpl_t *rpttpl, int destroy);

int       rpttpl_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*   rpttpl_serialize_wrapper(rpttpl_t *rpttpl);

#endif /* _REPORT_H_ */
