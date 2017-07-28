/*****************************************************************************
 **
 ** File Name: tdc.h
 **
 ** Description: This implements a strongly-typed data collection, based on the
 **              original datalist data type proposed by Jeremy Mayer.
 **
 ** Notes:
 **
 ** Assumptions:
 ** 1. The typed data collection has less than 256 elements.
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/24/15  J. P. Mayer    Initial Implementation.
 **  06/27/15  E. Birrane     Migrate from datalist to TDC. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef TDC_H_
#define TDC_H_

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

#define CHKVALID(x) {if (x==DLIST_INVALID) {DTNMP_DEBUG_ERR("tdc","invalid tdc, exiting",NULL);return 0;}}

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


typedef struct
{
	uint8_t length; /* The number of types.  */
	uint8_t index;  /* current 0-based index */
	uint8_t* data;  /* Byte array of types in the encapsulated DC. */
} tdc_hdr_t;


typedef struct
{
	tdc_hdr_t hdr;
	Lyst datacol;   /* List of blob_t. */
} tdc_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int8_t           tdc_append(tdc_t *dst, tdc_t *src);
void             tdc_clear(tdc_t *tdc);
tdc_t*           tdc_create(Lyst *dc, uint8_t *types, uint8_t type_cnt);
int8_t           tdc_compare(tdc_t *t1, tdc_t *t2);
tdc_t*           tdc_copy(tdc_t *tdc);
tdc_t*           tdc_deserialize(uint8_t* buffer, uint32_t buffer_size, uint32_t* bytes_used);
void             tdc_destroy(tdc_t **tdc);
amp_type_e     tdc_get(tdc_t* tdc, uint8_t index, amp_type_e type, uint8_t** optr, size_t* outsize);
blob_t*          tdc_get_colentry(tdc_t* tdc, uint8_t index);
int8_t           tdc_get_count(tdc_t* tdc);
uint32_t         tdc_get_entry_size(tdc_t* tdc, uint8_t index);
amp_type_e     tdc_get_type(tdc_t* tdc, uint8_t index);
uint8_t          tdc_hdr_allocate(tdc_hdr_t* header, uint8_t dataSize);
uint8_t          tdc_hdr_reallocate(tdc_hdr_t* header, uint8_t newSize);
void           tdc_init(tdc_t *tdc);
amp_type_e     tdc_insert(tdc_t* tdc, amp_type_e type, uint8_t* data, uint32_t size);
uint8_t*         tdc_serialize(tdc_t *tdc, uint32_t *size);
amp_type_e     tdc_set_type(tdc_t* tdc, uint8_t index, amp_type_e type);
char*            tdc_to_str(tdc_t *tdc);


#endif // TDC_H_INCLUDED
