/*****************************************************************************
 **
 ** File Name: table.h
 **
 ** Description: This implements a typed table.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/15/16  E. Birrane    Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef TABLE_H_
#define TABLE_H_

#include "stdint.h"

#include "lyst.h"

#include "../primitives/dc.h"
#include "../utils/nm_types.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

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
	Lyst cels; /* Lyst of type blob_t. a DC. */
} table_row_t;

typedef struct
{
	blob_t types; /* One byte per column type. */

	Lyst names; /* List of blob_t * */
} table_hdr_t;

typedef struct
{
	table_hdr_t hdr; /* Table header. */
	Lyst rows;      /* Lyst of table_row_t. */
} table_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int8_t    table_add_col(table_t *table, char *name, amp_type_e type);
int8_t    table_add_row(table_t *table, Lyst new_cels);
void      table_clear(table_t *table);
table_t*  table_create(blob_t *col_desc, Lyst names);
table_t*  table_copy(table_t *table);
table_t*  table_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t* bytes_used);
void      table_destroy(table_t *table, uint8_t destroy);
tdc_t*    table_extract_col(table_t *table, uint32_t col_idx);
tdc_t*    table_extract_row(table_t *table, uint32_t row_idx);
blob_t*   table_extract_cel(table_t *table, uint32_t col_idx, uint32_t row_idx);
int32_t   table_get_col_idx(table_t *table, blob_t *col_name);

int8_t   table_get_num_cols(table_t *table);
int8_t   table_get_num_rows(table_t *table);

table_t*  table_make_subtable(table_t *table, Lyst col_names);
int8_t    table_remove_row(table_t *table, uint32_t row_idx);
uint8_t*  table_serialize(table_t *table, uint32_t *size);

#endif // TABLE_H_
