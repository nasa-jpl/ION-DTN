/***************************************************************
 * \file general_functions_ported_from_ion.h
 *
 * \brief In this file there are the declarations of some functions
 *        used by the CGR's interface for ION.
 *
 * \par Ported from ION 3.7.0 by
 *      Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 * 
 * \par Supervisor
 *      Carlo Caini, carlo.caini@unibo.it
 *
 * \par Date
 *      16/02/20
 ***************************************************************/

#ifndef CGR_UNIBO_PORTED_FROM_ION_GENERAL_FUNCTIONS_PORTED_FROM_ION_H_
#define CGR_UNIBO_PORTED_FROM_ION_GENERAL_FUNCTIONS_PORTED_FROM_ION_H_

#include "../../../ici/include/ion.h"
#include "../../../ici/include/platform.h"
#include "../../../ici/include/sdrstring.h"
#include "../../library/cgr.h"
#include "../../library/bpP.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern void removeRoute(PsmPartition ionwm, PsmAddress routeElt);
extern int create_ion_node_routing_object(IonNode *terminusNode, PsmPartition ionwm,
		CgrVdb *cgrvdb);

#ifdef __cplusplus
}
#endif

#endif /* CGR_UNIBO_PORTED_FROM_ION_GENERAL_FUNCTIONS_PORTED_FROM_ION_H_ */
