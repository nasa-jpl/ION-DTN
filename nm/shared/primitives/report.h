/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file report.h
 **
 **
 ** Description: Defines report data types for DTNMP.
 **
 ** Notes:
 **		1. Currently we do not support ACLs.
 **		2. Currently, report is destined to go to whoever requested the
 **		   production of the report.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/11/13  E. Birrane     Redesign of primitives architecture. (JHU/APL)
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t. (JHU/APL)
 **  07/02/15  E. Birrane     Migrated to Typed Data Collections (TDCs) (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/


#ifndef _REPORT_H_
#define _REPORT_H_

#include "lyst.h"

#include "../utils/nm_types.h"

#include "../primitives/def.h"
#include "../primitives/tdc.h"

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
#define RPT_MAP_GET_SRC_IDX(map) (map >> 8)
#define RPT_MAP_GET_DEST_IDX(map) (map & 0xFF)

#define RPT_MAP_SET_SRC_IDX(map, item) (map |= ((item & 0xFF) << 8))
#define RPT_MAP_SET_DEST_IDX(map, item) (map |= (item & 0xFF))

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


typedef struct
{
	mid_t *mid;		 /**> The MID identifying this report template. */

	uint8_t num_map;
	uint16_t *parm_map;   /**> List of uint16_t, with high 8 bits representing
	                      mid parm # and low 8 bits representing matching
	                      template id parm #. */
} rpttpl_item_t;



/**
 * Describes a report template.
 *
 */

typedef struct
{
	mid_t *id;				/**> The MID identifying this report template.   */

	Lyst contents;			/**> Series of report template definitions. This
	                             is a list of type rpttpl_item_t.            */

	def_gen_desc_t desc;    /**> Descriptor of def in the SDR. */
} rpttpl_t;


/**
 * Entry in a data report list, comprising a single report.
 *
 * NOTE: The contents can include other report entries.
 * NOTE: Support up to 32 parameters in the parm map.
 */
typedef struct
{
	mid_t *id;		    /**> The report ID. */
	tdc_t *contents;    /**> The typed data collection holding the report. */
} rpt_entry_t;



/**
 * This is a report message. A report message contains a series of report entries.
 * Associated Message Type(s): MSG_TYPE_RPT_DATA_RPT
 *
 * Purpose: A data report.
 * +------+---------+-----------+---------+   +---------+
 * | Time | RX Name | # Entries | ENTRY 1 |   | ENTRY N |
 * | [TS] |  [BLOB] |   [SDNV]  | [RPTE]  |...| [RPTE]  |
 * +------+---------+-----------+---------+   +---------+
 *
 * TODO: Rename this rpt_msg_t.
 *
 */
typedef struct {

	time_t time;        /**> Time the report entries were generated. */
    Lyst entries;       /**> The report entries (lyst of rpt_entry_t */

    /* Non-serialized portions. */
    eid_t recipient;
} rpt_t;




/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int      rpt_add_entry(rpt_t *rpt, rpt_entry_t *entry);
rpt_t*   rpt_create(time_t time, Lyst entries, eid_t recipient);
void     rpt_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);


rpt_t*   rpt_deserialize_data(uint8_t *cursor,
		                      uint32_t size,
		                      uint32_t *bytes_used);

void     rpt_release(rpt_t *msg);

uint8_t* rpt_serialize(rpt_t *msg, uint32_t *len);

char*    rpt_to_str(rpt_t *rpt);



void         rpt_entry_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);
rpt_entry_t* rpt_entry_create(mid_t *mid);
rpt_entry_t* rpt_entry_deserialize(uint8_t *cursor,
		                           uint32_t size,
		                           uint32_t *bytes_used);
char*        rpt_entry_lst_to_str(Lyst entries);
void         rpt_entry_release(rpt_entry_t *entry);
uint8_t*     rpt_entry_serialize(rpt_entry_t *entry, uint32_t *len);
uint8_t*     rpt_entry_serialize_lst(Lyst entries, uint32_t *len);
char*        rpt_entry_to_str(rpt_entry_t *entry);

int          rpttpl_add_item(rpttpl_t *rpttpl, rpttpl_item_t *item);
rpttpl_t* 	 rpttpl_create(mid_t *mid, Lyst items);
rpttpl_t* 	 rpttpl_create_from_mc(mid_t *mid, Lyst mc);
rpttpl_t*    rpttpl_find_by_id(Lyst rpttpls, ResourceLock *mutex, mid_t *id);

rpttpl_t*    rpttpl_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
rpttpl_t*	 rpttpl_duplicate(rpttpl_t *src);
void       	 rpttpl_release(rpttpl_t *rpttpl);
uint8_t*     rpttpl_serialize(rpttpl_t *rpttpl, uint32_t *len);


int			   rpttpl_item_add_parm_map(rpttpl_item_t *item, uint8_t item_idx, uint8_t rpt_idx);
rpttpl_item_t* rpttpl_item_create(mid_t *mid, uint32_t num_parm);
rpttpl_item_t* rpttpl_item_deserialize(uint8_t *cursor, uint32_t size, uint32_t *bytes_used);
rpttpl_item_t* rpttpl_item_duplicate(rpttpl_item_t *item);
Lyst           rpttpl_item_duplicate_lyst(Lyst items);
void		   rpttpl_item_release(rpttpl_item_t *item);
void           rpttpl_item_release_lyst(Lyst items);
uint8_t*       rpttpl_item_serialize(rpttpl_item_t *item, uint32_t *len);

#endif /* _REPORT_H_ */
