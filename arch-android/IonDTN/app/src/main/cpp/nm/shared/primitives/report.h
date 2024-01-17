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


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */



/**
 * Entry in a data report list, comprising a single report.
 */
typedef struct
{
	mid_t *id;		   /**> The report ID. */

	tdc_t *contents;   /**> The typed data collection holding the report. */
} rpt_entry_t;



/**
 * Associated Message Type(s): MSG_TYPE_RPT_DATA_RPT
 *
 * Purpose: A data report.
 * +------+---------+-----------+---------+   +---------+
 * | Time | RX Name | # Entries | ENTRY 1 |   | ENTRY N |
 * | [TS] |  [BLOB] |   [SDNV]  | [RPTE]  |...| [RPTE]  |
 * +------+---------+-----------+---------+   +---------+
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
rpt_entry_t* rpt_entry_create();
rpt_entry_t* rpt_entry_deserialize(uint8_t *cursor,
		                           uint32_t size,
		                           uint32_t *bytes_used);
char*        rpt_entry_lst_to_str(Lyst entries);
void         rpt_entry_release(rpt_entry_t *entry);
uint8_t*     rpt_entry_serialize(rpt_entry_t *entry, uint32_t *len);
uint8_t*     rpt_entry_serialize_lst(Lyst entries, uint32_t *len);
char*        rpt_entry_to_str(rpt_entry_t *entry);



#endif /* _REPORT_H_ */
