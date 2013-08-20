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
 **  08/10/11  V.Ramachandran Initial Implementation
 **  11/13/12  E. Birrane     Technical review, comment updates.
 **  06/25/13  E. Birrane     Renamed message "bundle" message "group".
 *****************************************************************************/
#ifndef ION_IF_H_
#define ION_IF_H_

#include "bp.h"
#include "shared/msg/pdu.h"

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


uint8_t     iif_deregister_node(iif_t *iif);
eid_t       iif_get_local_eid(iif_t *iif);
uint8_t     iif_is_registered(iif_t *iif);
uint8_t*    iif_receive(iif_t *iif, uint32_t *size, pdu_metadata_t *meta, int timeout);
uint8_t     iif_register_node(iif_t *iif, eid_t eid);
uint8_t     iif_send(iif_t *iif, pdu_group_t *group, char *recipient);

#endif /* ION_IF_H_ */
