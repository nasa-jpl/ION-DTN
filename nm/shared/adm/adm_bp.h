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
 ** File Name: adm_bp.h
 **
 ** Description: This file contains the definitions of the Bundle Protocol
 **              ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 **      1. We current use a non-official OID root tree for DTN Bundle Protocol
 **         identifiers.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Initial Implementation.
 **  01/02/13  E. Birrane     Update to latest version of DTNMP. Cleanup.
 *****************************************************************************/

#ifndef ADM_BP_H_
#define ADM_BP_H_

#include "lyst.h"
#include "bpnm.h"

#include "shared/utils/nm_types.h"


#include "shared/adm/adm.h"




/*
 * [3] arrays ar eby classes of service.
 * 0 - BULK
 * 1 - NORM
 * 2 - EXP
 */


/*
 * +--------------------------------------------------------------------------+
 * |						      ADM CONSTANTS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * We will invent an OID space for BP ADM information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.dtnmp.bp
 * or 1.3.6.1.2.3.1
 * or, as OID,: 2A 06 01 02 03 01
 *
 * Note: dtnmp.bp is a made-up subtree.
 */


static char* BP_ADM_ROOT = "2B0601020301";
#define BP_ADM_ROOT_LEN (6)

/*
 * +--------------------------------------------------------------------------+
 * |					  ADM ATOMIC DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
 */


/*
 * Structure: Node Info is subtree at 01. Endpoint is subtree at 02. CLA is
 *            subtree at 03.
 *
 *                   ADM_BP_ROOT (2A0601020301)
 *                        |
 *      NODE_INFO (01)    |    ENDPOINT_INFO (02)       CTRL (03)
 *           +------------+----------+---------------------+
 *           |            |          |                     |
 */

/* Node-Specific Definitions */
static char* BP_ADM_DATA_NN = "2B060102030101";
#define BP_ADM_DATA_NN_LEN  (7)


/* Endpoint-Specific Definitions */
static char* BP_ADM_DATA_END_NN = "2B060102030102";
#define BP_ADM_DATA_END_NN_LEN  (7)


/* Bundle Protocol Controls */
static char* BP_ADM_DATA_CTRL_NN = "2B060102030103";
#define BP_ADM_DATA_CTRL_NN_LEN  (7)



/*
 * +--------------------------------------------------------------------------+
 * |					        FUNCTION PROTOTYPES  						  +
 * +--------------------------------------------------------------------------+
 */

void adm_bp_init();

/* Custom Print Functions */
char *bp_print_node_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *bp_print_endpoint_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);


/* Custom Size Functions. */
uint32_t bp_size_node_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_endpoint_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_id(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_version(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_restart_time(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_node_num_reg(uint8_t* buffer, uint64_t buffer_len);
uint32_t bp_size_endpoint_name(uint8_t* buffer, uint64_t buffer_len);


#endif //ADM_BP_H_
