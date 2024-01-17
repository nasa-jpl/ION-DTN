/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm_ion_priv.h
 **
 ** Description: This file contains the definitions of the ION
 **              ADM.
 **
 ** Notes:
 **
 ** Assumptions:
 ** 	1. We current use a non-official OID root tree for DTN Bundle Protocol
 **         identifiers.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef ADM_ION_H_
#define ADM_ION_H_

#ifdef _HAVE_ION_ADM_

#include "lyst.h"
#include "bpnm.h"
#include "icinm.h"


#include "../utils/nm_types.h"
#include "../adm/adm.h"


/*
 * We will invent an OID space for ION ADM information, to live at:
 *
 * iso.identified-organization.dod.internet.mgmt.dtnmp.ion
 * or 1.3.6.1.2.3.4
 * or, as OID,: 2B 06 01 02 03 04
 *
 * Note: dtnmp.ion is a made-up subtree.
 */


/*
 * +--------------------------------------------------------------------------+
 * |						      ADM CONSTANTS  							  +
 * +--------------------------------------------------------------------------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |					  ADM ATOMIC DATA DEFINITIONS  						  +
 * +--------------------------------------------------------------------------+
 */


/*
 * Structure:
 *
 *                   ADM_ION_ROOT (2A0601020304)
 *                        |
 *      ICI (01)          |    INDUCT (02)    OUTDUCT (03)     NODE (04)
 *           +-----------------------+--------------+--------------+
 *           |                       |              |              |
 */

static char* ION_ADM_ROOT = "2B0601020304";
#define ION_ADM_ROOT_LEN (6)

static char* ION_ADM_ICI_NN = "2B060102030401";
#define ION_ADM_ICI_NN_LEN  (7)

static char* ION_ADM_INDUCT_NN = "2B060102030402";
#define ION_ADM_INDUCT_NN_LEN  (7)

static char* ION_ADM_OUTDUCT_NN = "2B060102030403";
#define ION_ADM_OUTDUCT_NN_LEN  (7)

static char* ION_ADM_NODE_NN = "2B060102030404";
#define ION_ADM_NODE_NN_LEN  (7)

static char* ION_ADM_CTRL_NN = "2B060102030405";
#define ION_ADM_CTRL_NN_LEN  (7)


void adm_ion_init();

/* Print Functions */
char *ion_print_sdr_state_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_node_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_induct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char *ion_outduct_print_all(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);


/* Sizing Functions */

/* ION ICI */
uint32_t ion_size_sdr_state_all(uint8_t* buffer, uint64_t buffer_len);

/* ION INDUCT */
uint32_t ion_induct_size_all(uint8_t* buffer, uint64_t buffer_len);

/* ION OUTDUCT */
uint32_t ion_outduct_size_all(uint8_t* buffer, uint64_t buffer_len);

/* ION NODE */
uint32_t ion_node_size_all(uint8_t* buffer, uint64_t buffer_len);
uint32_t ion_node_size_inducts(uint8_t* buffer, uint64_t buffer_len);
uint32_t ion_node_size_outducts(uint8_t* buffer, uint64_t buffer_len);

#endif /* _HAVE_ION_ADM_ */
#endif //ADM_ION_H_
