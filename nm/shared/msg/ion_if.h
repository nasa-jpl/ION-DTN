/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ion_if.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to connect to
 **              the local BPA.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/10/11  V.Ramachandran Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group". (JHU/APL)
 **  06/30/16  E. Birrane     Doc. Updates (Secure DTN - NASA: NNX14CS58P)
 **  10/02/18  E. Birrane     Update to AMP v0.5 (JHUAPL)
 *****************************************************************************/
#ifndef ION_IF_H_
#define ION_IF_H_

#include "bp.h"
#include "msg.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * The ION Interface structure captures state necessary to communicate with
 * the local Bundle Protocol Agent (BPA).
 */
typedef struct
{
	eid_t local_eid;
	BpSAP sap;
} iif_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int     iif_deregister_node(iif_t *iif);
eid_t   iif_get_local_eid(iif_t *iif);
int     iif_is_registered(iif_t *iif);
blob_t *iif_receive(iif_t *iif, msg_metadata_t *meta, int timeout, int *success);
int     iif_register_node(iif_t *iif, eid_t eid);
int     iif_send_grp(iif_t *iif, msg_grp_t *group, char *recipient);
int     iif_send_msg(iif_t *iif, int msg_type, void *msg, char *recipient);

#endif /* ION_IF_H_ */
