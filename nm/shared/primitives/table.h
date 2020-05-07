/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** File Name: table.h
 **
 ** Description: This implements a typed table template and instances of that
 **              template.
 **
 ** Notes:
 **   - Tables are ephemeral and exist only when they are created to be
 **     returned in reports.
 **   - Table templates are AMM objects that are stored and updated.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/15/16  E. Birrane    Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 **  09/30/18  E. Birrane    Updated to AMP v0.5. (JHU/APL)
 *****************************************************************************/
#ifndef TABLE_H_
#define TABLE_H_

#include "stdint.h"

#include "../utils/nm_types.h"
#include "../utils/vector.h"
#include "../utils/db.h"
#include "ari.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define TBL_DEFAULT_ENC_SIZE  1024
#define TBLT_DEFAULT_ENC_SIZE 1024

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */
typedef struct
{
	ari_t    *id;   /**> The ID of the table template that this populates. */
	vector_t rows; /**> Vector of rows in the table. (tnvc_t*) */
} tbl_t;

typedef tbl_t* (*tblt_build_fn)(ari_t *id);



typedef struct
{
	amp_type_e type;
	char *name;
} tblt_col_t;


typedef struct
{
	ari_t *id;      /* The ID of the template. */
	vector_t cols; /* Column information for each col. (table_col_t*). */

	tblt_build_fn build;
	db_desc_t desc;
} tblt_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int      tbl_add_row(tbl_t *tbl, tnvc_t *row);

void     tbl_clear(tbl_t *tbl);

tbl_t*   tbl_copy_ptr(tbl_t *tbl);

tbl_t*   tbl_create(ari_t *id);

void*   tbl_deserialize_ptr(QCBORDecodeContext *it, int *success);

tbl_t*   tbl_deserialize_raw(blob_t *data, int *success);

tnvc_t*  tbl_get_row(tbl_t *tbl, int row_idx);

void     tbl_release(tbl_t *tbl, int destroy);

int      tbl_num_rows(tbl_t *tbl);

int      tbl_serialize(QCBOREncodeContext *encoder, void *item);

blob_t*   tbl_serialize_wrapper(tbl_t *tbl);

void      tbl_cb_del_fn(void *item);
void*     tbl_cb_copy_fn(void *item);
int tbl_cb_comp_fn(void *i1, void *i2);



/* tbl Template Functions */

int       tblt_add_col(tblt_t *tblt, amp_type_e type, char *name);


void      tblt_cb_del_fn(void *item);
int tblt_cb_comp_fn(void *i1, void *i2);
void*     tblt_cb_copy_fn(void *item);
void      tblt_cb_ht_del_fn(rh_elt_t *elt);

int       tblt_check_row(tblt_t *tblt, tnvc_t *row);

tblt_t*   tblt_copy_ptr(tblt_t *tblt);

tblt_t*   tblt_create(ari_t *id, tblt_build_fn build);

amp_type_e tblt_get_type(tblt_t *tblt, int idx);

int       tblt_num_cols(tblt_t *tblt);

void      tblt_release(tblt_t *tblt, int destroy);


void      tblt_col_cb_del_fn(void *item);
void*     tblt_col_cb_copy_fn(void *item);


#endif // TABLE_H_
