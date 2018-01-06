/****************************************************************************
 **
 ** File Name: adm_ion_bp_admin_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History: 
 **  YYYY-MM-DD  AUTHOR           DESCRIPTION
 **  ----------  --------------   --------------------------------------------
 **  2018-01-05  AUTO             Auto-generated c file 
 **
 ****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "bpP.h"
/*   STOP CUSTOM INCLUDES HERE  */

#include "adm_ion_bp_admin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */
/*             TODO              */
/*   STOP CUSTOM FUNCTIONS HERE  */

void adm_ion_bp_admin_setup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION setup BODY
	 * +-------------------------------------------------------------------------+
	 */
}

void adm_ion_bp_admin_cleanup(){

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION cleanup BODY
	 * +-------------------------------------------------------------------------+
	 */
}


/* Metadata Functions */


value_t adm_ion_bp_admin_meta_name(tdc_t params)
{
	return val_from_string("adm_ion_bp_admin");
}


value_t adm_ion_bp_admin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:ion_bp_admin");
}


value_t adm_ion_bp_admin_meta_version(tdc_t params)
{
	return val_from_string("00");
}


value_t adm_ion_bp_admin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


/*
 * Local endpoints, regardless of scheme name.
 */

table_t* adm_ion_bp_admin_table_tbl_endpoints()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

	if(
			(table_add_col(table, "endpointId", AMP_TYPE_STR) == ERROR) ||
			(table_add_col(table, "type", AMP_TYPE_UINT) == ERROR) ||
			(table_add_col(table, "rcvScript", AMP_TYPE_STR) == ERROR)
	)
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */

Sdr             sdr = getIonsdr();
	PsmPartition    ionwm = getIonwm();
	VScheme         *vscheme;
	PsmAddress      psm_elt;
    PsmAddress      psm_elt2;
    VEndpoint       *vpoint;
    OBJ_POINTER(Endpoint, endpoint);
    OBJ_POINTER(Scheme, scheme);
    char    recvRule;
    char    recvScriptBuffer[SDRSTRING_BUFSZ];
    char    *recvScript = recvScriptBuffer;
	Lyst 	cur_row = NULL;
	uint32_t size;
	uint8_t *val = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (psm_elt = sm_list_first(ionwm, (getBpVdb())->schemes); psm_elt;
			psm_elt = sm_list_next(ionwm, psm_elt))
	{
		vscheme = (VScheme *) psp(ionwm,
				sm_list_data(ionwm, psm_elt));


		for (psm_elt2 = sm_list_first(ionwm, vscheme->endpoints); psm_elt2;
				psm_elt2 = sm_list_next(ionwm, psm_elt2))
		{
			vpoint = (VEndpoint *) psp(ionwm, sm_list_data(ionwm, psm_elt2));

			GET_OBJ_POINTER(sdr, Endpoint, endpoint, sdr_list_data(sdr,
					vpoint->endpointElt));
			GET_OBJ_POINTER(sdr, Scheme, scheme, endpoint->scheme);
			if (endpoint->recvRule == EnqueueBundle)
			{
				recvRule = 'q';
			}
			else
			{
				recvRule = 'x';
			}

			if (endpoint->recvScript == 0)
			{
				recvScriptBuffer[0] = '\0';
			}
			else
			{
				if (sdr_string_read(sdr, recvScriptBuffer, endpoint->recvScript)
						< 0)
				{
					recvScript = "?";
				}
			}

			cur_row = lyst_create();

			val = utils_serialize_string(scheme->name, &size);
			dc_add(cur_row, val, size);
			SRELEASE(val);

			val = utils_serialize_string(endpoint->nss, &size);
			dc_add(cur_row, val, size);
			SRELEASE(val);

			val = utils_serialize_uint(vpoint->appPid, &size);
			dc_add(cur_row, val, size);
			SRELEASE(val);

			val = utils_serialize_byte(recvRule, &size);
			dc_add(cur_row, val, size);
			SRELEASE(val);

			val = utils_serialize_string(recvScript, &size);
			dc_add(cur_row, val, size);
			SRELEASE(val);

			table_add_row(table, cur_row);
		}
	}

	sdr_exit_xn(sdr);
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Inducts established locally for the indicated CL protocol.
 */

table_t* adm_ion_bp_admin_table_tbl_inducts()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "protocolName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "ductName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "cliControl", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * If protocolName is specified, this table lists all outducts established locally for the indicated CL
 *  protocol. Otherwise, it lists all locally established outducts, regardless of their protocol.
 */

table_t* adm_ion_bp_admin_table_tbl_outducts()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "protocolName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "ductName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "cloControl", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "maxPayloadLength", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Convergence layer protocols that can currently be utilized at the local node.
 */

table_t* adm_ion_bp_admin_table_tbl_protocols()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "protocolName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "payloadBytesPerFrame", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "overheadBytesPerFrame", AMP_TYPE_UINT) == ERROR) ||
	(table_add_col(table, "nominalDataRate", AMP_TYPE_UINT) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/*
 * Declared endpoint naming schemes.
 */

table_t* adm_ion_bp_admin_table_tbl_schemes()
{
	table_t *table = NULL;
	if((table = table_create(NULL,NULL)) == NULL)
	{
		return NULL;
	}

if(
	(table_add_col(table, "schemeName", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "forwarderControl", AMP_TYPE_STR) == ERROR) ||
	(table_add_col(table, "adminAppControl", AMP_TYPE_STR) == ERROR)))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION tbl_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION tbl_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */
	return table;
}


/* Collect Functions */
/*
 * Version of installed ION BP Admin utility.
 */
value_t adm_ion_bp_admin_get_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */

/*
 * Establish DTN endpoint named endpointId on the local node. The remaining parameters indicate what is
 *  to be done when bundles destined for this endpoint arrive at a time when no application has the end
 * point open for bundle reception. If type is 'x', then such bundles are to be discarded silently and 
 * immediately. If type is 'q', then such bundles are to be enqueued for later delivery and, if recvScr
 * ipt is provided, recvScript is to be executed.
 */
tdc_t* adm_ion_bp_admin_ctrl_endpointadd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpointAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpointAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Change the action taken when bundles destined for this endpoint arrive at a time when no application
 *  has the endpoint open for bundle reception.
 */
tdc_t* adm_ion_bp_admin_ctrl_endpointchange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpointChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpointChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the endpoint identified by endpointId. The control will fail if any bundles are currently pen
 * ding delivery to this endpoint.
 */
tdc_t* adm_ion_bp_admin_ctrl_endpointdel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_endpointDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_endpointDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition
 *  structure is used and populated by the induct task whose operation is initiated by cliControl at th
 * e time the duct is started.
 */
tdc_t* adm_ion_bp_admin_ctrl_inductadd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_inductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_inductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Change the control used to initiate operation of the induct task for the indicated duct.
 */
tdc_t* adm_ion_bp_admin_ctrl_inductchange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_inductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_inductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the induct identified by protocolName and ductName. The control will fail if any bundles are 
 * currently pending acquisition via this induct.
 */
tdc_t* adm_ion_bp_admin_ctrl_inductdel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_inductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_inductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the indicated induct task as defined for the indicated CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_inductstart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_inductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_inductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the indicated induct task as defined for the indicated CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_inductstop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_inductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_inductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Until this control is executed, Bundle Protocol is not in operation on the local ION node and most b
 * padmin controls will fail.
 */
tdc_t* adm_ion_bp_admin_ctrl_init(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_init BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_init BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Declare the maximum number of bytes of SDR heap space that will be occupied by any single bundle acq
 * uisition activity (nominally the acquisition of a single bundle, but this is at the discretion of th
 * e convergence-layer input task). All data acquired in excess of this limit will be written to a temp
 * orary file pending extraction and dispatching of the acquired bundle or bundles. The default is the 
 * minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; thi
 * s is the minimum SDR heap space occupancy in the event that all acquisition is into a file.
 */
tdc_t* adm_ion_bp_admin_ctrl_manageheapmax(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_manageHeapMax BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_manageHeapMax BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish a duct for transmission of bundles via the indicated CL protocol. the duct's data transmis
 * sion structure is serviced by the outduct task whose operation is initiated by CLOcommand at the tim
 * e the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can b
 * e accomodated;this is the default.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductadd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Set new values for the indicated duct's payload size limit and the control that is used to initiate 
 * operation of the outduct task for this duct.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductchange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the outduct identified by protocolName and ductName. The control will fail if any bundles are
 *  currently pending transmission via this outduct.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductdel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the indicated outduct task as defined for the indicated CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductstart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Disable transmission of bundles queued for transmission to the indicated node and reforwards all non
 * -critical bundles currently queued for transmission to this node. This may result in some or all of 
 * these bundles being enqueued for transmission to the psuedo-node limbo.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductblock(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductBlock BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductBlock BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Re-enable transmission of bundles to the indicated node and reforwards all bundles in limbo in the h
 * ope that the unblocking of this egress plan will enable some of them to be transmitted.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductunblock(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductUnblock BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductUnblock BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the indicated outduct task as defined for the indicated CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_outductstop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_outductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_outductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Establish access to the named convergence layer protocol at the local node. The payloadBytesPerFrame
 *  and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity con
 * sumption of each bundle, to aid in route computation and congesting forecasting. The optional nomina
 * lDataRate argument overrides the hard coded default continuous data rate for the indicated protocol 
 * for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are 
 * not specifically dedicated to transmission to a single identified convergence-layer protocol endpoin
 * t- the protocol's applicable nominal continuous data rate is the data rate that is always used for r
 * ate control over links served by that protocol; data rates are not extracted from contact graph info
 * rmation. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, T
 * CP) can be dynamically adjusted in response to changes in data rate between the local node and its n
 * eighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nom
 * inal data rate may be the authority for rate control, in the event that the contact plan lacks ident
 * ified contacts with the node to which the outduct is mapped.
 */
tdc_t* adm_ion_bp_admin_ctrl_protocoladd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocolAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocolAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the convergence layer protocol identified by protocolName. The control will fail if any ducts
 *  are still locally declared for this protocol.
 */
tdc_t* adm_ion_bp_admin_ctrl_protocoldel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocolDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocolDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start all induct and outduct tasks for inducts and outducts that have been defined for the indicated
 *  CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_protocolstart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocolStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocolStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop all induct and outduct tasks for inducts and outducts that have been defined for the indicated 
 * CL protocol on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_protocolstop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_protocolStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_protocolStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName
 * :schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to i
 * nitiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when
 *  the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoi
 * nt identified within this scheme so that it can recieve and process custody signals and bundle statu
 * s reports.
 */
tdc_t* adm_ion_bp_admin_ctrl_schemeadd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_schemeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_schemeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Set the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments
 * .
 */
tdc_t* adm_ion_bp_admin_ctrl_schemechange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_schemeChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_schemeChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Delete the scheme identified by schemeName. The control will fail if any bundles identified in this 
 * scheme are pending forwarding, transmission, or delivery.
 */
tdc_t* adm_ion_bp_admin_ctrl_schemedel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_schemeDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_schemeDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start the forwarder and administrative endpoint tasks for the indicated scheme task on the local nod
 * e.
 */
tdc_t* adm_ion_bp_admin_ctrl_schemestart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_schemeStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_schemeStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop the forwarder and administrative endpoint tasks for the indicated scheme task on the local node
 * .
 */
tdc_t* adm_ion_bp_admin_ctrl_schemestop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_schemeStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_schemeStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Start all schemes and all protocols on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_start(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_start BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Stop all schemes and all protocols on the local node.
 */
tdc_t* adm_ion_bp_admin_ctrl_stop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_stop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}


/*
 * Enable/Disable production of a continuous stream of user selected Bundle Protocol activity indicatio
 * n characters. A watch parameter of 1 selects all BP activity indication characters, 0 deselects allB
 * P activity indication characters; any other activitySpec such as acz~ selects all activity indicatio
 * n characters in the string, deselecting all others. BP will print each selected activity indication 
 * character to stdout every time a processing event of the associated type occurs: a new bundle is que
 * ued for forwarding, b bundle is queued for transmission, c bundle is popped from its transmission qu
 * eue, m custody acceptance signal is recieved, w custody of bundle is accepted, x custody of bundle i
 * s refused, y bundle is accepted upon arrival, z bundle is queued for delivery to an application, ~ b
 * undle is abandoned (discarded) on attempt to forward it, ! bundle is destroyed due to TTL expiration
 * , & custody refusal signal is recieved, # bundle is queued for re-forwarding due to CL protocol fail
 * ures, j bundle is placed in 'limbo' for possible future reforwarding, k bundle is removed from 'limb
 * o' and queued for reforwarding, $ bundle's custodial retransmission timeout interval expired. 
 */
tdc_t* adm_ion_bp_admin_ctrl_watch(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION ctrl_watch BODY
	 * +-------------------------------------------------------------------------+
	 */
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION ctrl_watch BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
