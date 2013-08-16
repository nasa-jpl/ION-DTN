/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
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
 **  01/11/13  E. Birrane     Redesign of primitives architecture.
 **  06/24/13  E. Birrane     Migrated from uint32_t to time_t.
 *****************************************************************************/


#ifndef _REPORT_H_
#define _REPORT_H_

#include "lyst.h"

#include "shared/utils/nm_types.h"

#include "shared/primitives/def.h"
/*
#include "shared/msg/pdu.h"
#include "shared/msg/msg_def.h"
#include "shared/msg/msg_reports.h"
*/

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
 * Associated Message Type(s): MSG_TYPE_RPT_DATA_LIST
 *
 * Purpose: Define a data list report.
 *
 * +---------------+
 * | Configured ID |
 * |     (MC)      |
 * +---------------+
 */
typedef struct {
    Lyst contents;        /**> The ordered MIDs comprising the data list. */
} rpt_items_t;



/**
 * Associated Message Type(s): MSG_TYPE_RPT_DATA_DEFS
 *
 * Purpose: Define a data definition report.
 *
 * +--------+------+----------+     +------+----------+
 * | # Defs | ID 1 |   Def 1  | ... | ID N |   Def N  |
 * | (SDNV) |(MID) |   (MC)   |     |(MID) |    (MC)  |
 * +--------+------+----------+     +------+----------+
 */
typedef struct {
    Lyst defs;            /**> The ordered MIDs comprising the data defs. */
} rpt_defs_t;



/**
 * Entry in a data report list, comprising a single report.
 */
typedef struct
{
	mid_t *id;		   /**> The report ID. */

	uvast size;     /**> The number of bytes of report data. */
	uint8_t *contents; /**> The report data. */
} rpt_data_entry_t;



/**
 * Associated Message Type(s): MSG_TYPE_RPT_DATA_RPT
 *
 * Purpose: A data report.
 * \todo Update the standard
 * +------+------+------+-------+---------+   +------+------+---------+
 * | Time | SIZE |ID    | Size  | Data[n] |   |  ID  | Size | Data[m] |
 * | (TS) |(SDNV)|(SDNV)|(SDNV) | (BYTEs) |...|(SDNV)|(SDNV)| (BYTEs) |
 * +------+------+------+-------+---------+   +------+------+---------+
 */
typedef struct {

	time_t time;        /**> Time the reports were generated. */
    Lyst reports;         /**> The reports (lyst of rpt_data_entry_t */


    eid_t recipient;
    uint32_t size;
} rpt_data_t;



/**
 * Associated Message Type(s): MSG_TYPE_RPT_PROD_SCHED
 *
 * Purpose: Define a data definition report.
 * \todo: Support for predicate types as well?
 *
 * +---------+--------+     +--------+
 * | # Rules | Rule 1 | ... | Rule N |
 * |  (SDNV) | (VAR)  |     |  (VAR) |
 * +---------+--------+     +--------+
 */
typedef struct {
    Lyst defs;            /**> Report defs (msg_ctrl_period_prod_t). */
} rpt_prod_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

void         rpt_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);

rpt_items_t* rpt_create_lst(Lyst contents);
rpt_defs_t*  rpt_create_defs(Lyst defs);
rpt_data_t*  rpt_create_data(time_t time, Lyst reports, eid_t recipient);
rpt_prod_t*  rpt_create_prod(Lyst defs);

def_gen_t*   rpt_find_custom(Lyst reportLyst, ResourceLock *mutex, mid_t *mid);

void         rpt_print_data_entry(rpt_data_entry_t *entry);

void         rpt_release_lst(rpt_items_t *msg);
void         rpt_release_defs(rpt_defs_t *msg);
void         rpt_release_data_entry(rpt_data_entry_t *entry);
void         rpt_release_data(rpt_data_t *msg);
void         rpt_release_prod(rpt_prod_t *msg);


#endif /* _REPORT_H_ */
