/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** \file ctrl.h
 **
 ** Description: This module contains the functions, structures, and other
 **              information used to capture both controls and macros.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/10/13  E. Birrane     Initial Implementation (JHU/APL)
 **  05/17/15  E. Birrane     Redesign around DTNMP v0.1 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef _CTRL_H
#define _CTRL_H

#include "../adm/adm.h"
#include "../utils/nm_types.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define DTNMP_RULE_EXEC_ALWAYS (-1)

// Control parameters not filled out yet.
#define CONTROL_INIT     (0)

// Control initialized and ready.
#define CONTROL_ACTIVE   (1)

// Control initialized, but disabled.
#define CONTROL_INACTIVE (2)


#define CTRL_SUCCESS (0)
#define CTRL_FAILURE (1)

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
	Object itemObj;           /**> Serialized ctrl in an SDR. */
	uint32_t size;            /**> Size of ctrl in ctrlObj.   */

	/* Descriptor Information. */

    /* Below is not kept in the SDR. */
	Object descObj;           /** > This descriptor in SDR. */
} ctrl_exec_desc_t;



/**
 * Describes a CONTROL structure.
 *
 * This structure captures a specific, executable instance of a
 * control in the system.
 *
 * This instance is associated with a general description of the
 * control, as captured in the adm_ctrl_t structure.
 *
 * This instance augments the adm_ctrl_t structure with specific
 * information for an instance, including:
 *
 * - The parameters for this instance of the control.
 * - When the control should be run.
 * - THe status of this control (active, inactive).
 *
 */
typedef struct {

	mid_t *mid;      /**> The full, parameterized MID. */
	time_t time;     /**> The time to run the control. */
    int    status;   /**> Control current status. */
    eid_t  sender;   /**> Who sent this ctrl def.    */

    /* Below is not serialized into or out of messages. */
	adm_ctrl_t *adm_ctrl;     /**> The generic description of this control. */
    uint32_t countdown_ticks; /**> # ticks before next eval.  */
    ctrl_exec_desc_t desc;    /**> SDR descriptor. */

} ctrl_exec_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


void         ctrl_clear_lyst(Lyst *list, ResourceLock *mutex, int destroy);

ctrl_exec_t* ctrl_create(time_t time, mid_t *mid, eid_t sender);

ctrl_exec_t* ctrl_deserialize(uint8_t *cursor,
		                      uint32_t size,
		                      uint32_t *bytes_used);

void         ctrl_release(ctrl_exec_t *msg);

uint8_t*     ctrl_serialize(ctrl_exec_t *msg, uint32_t *len);


#endif // _CTRL_H
