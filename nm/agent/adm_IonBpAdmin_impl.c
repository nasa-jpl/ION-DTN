/*****************************************************************************
 **
 ** File Name: ./agent/adm_IonBpAdmin_impl.c
 **
 ** Description: TODO
 **
 ** Notes: TODO
 **
 ** Assumptions: TODO
 **
 ** Modification History:
 **  YYYY-MM-DD  AUTHOR         DESCRIPTION
 **  ----------  ------------   ---------------------------------------------
 **  2017-11-11  AUTO           Auto generated c file 
 *****************************************************************************/

/*   START CUSTOM INCLUDES HERE  */
#include "bpP.h"

/*   STOP CUSTOM INCLUDES HERE   */

#include "adm_IonBpAdmin_impl.h"

/*   START CUSTOM FUNCTIONS HERE */

/*   STOP CUSTOM FUNCTIONS HERE  */


/* Metadata Functions */

value_t adm_IonBpAdmin_meta_name(tdc_t params)
{
	return val_from_string("adm_IonBpAdmin");
}

value_t adm_IonBpAdmin_meta_namespace(tdc_t params)
{
	return val_from_string("arn:DTN:IonBpAdmin");
}


value_t adm_IonBpAdmin_meta_version(tdc_t params)
{
	return val_from_string("00");
}


value_t adm_IonBpAdmin_meta_organization(tdc_t params)
{
	return val_from_string("JHUAPL");
}


/* Table Functions */


// This table lists all local endpoints, regardless of their scheme name.
// "columns": "STR:schemeName&STR:endpointNSS&UINT:appPid&STRrecvRule&STR:recvScript

table_t *adm_IonBpAdmin_table_endpoints()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "schemeName", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "endpointNSS", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "appPid", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "recvRule", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "recvScript", AMP_TYPE_STRING) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_endpoints BODY
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
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_endpoints BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;
}

// If protocolName is specified, this table lists all inducts established locally for the indicated CL protocol. Otherwise, it lists all locally established inducts, regardless of their protocol.
// "STR:protocolName&STR:ductName&UINT:cliPid&STR:cliCmd",
table_t *adm_IonBpAdmin_table_inducts()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "protocol", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "duct", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "cliCtrl", AMP_TYPE_STRING) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr             sdr = getIonsdr();
	ClProtocol      clpbuf;
	Object          obj_elt;
	PsmPartition    ionwm = getIonwm();
	VInduct         *vduct;
	PsmAddress      psm_elt;
	OBJ_POINTER(Induct, duct);
	OBJ_POINTER(ClProtocol, clp);
	char    cliCmdBuffer[SDRSTRING_BUFSZ];
	char    *cliCmd;
	Lyst 	cur_row = NULL;
	uint32_t size;
	uint8_t *val = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (obj_elt = sdr_list_first(sdr, (getBpConstants())->protocols);
			obj_elt; obj_elt = sdr_list_next(sdr, obj_elt))
	{
		sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, obj_elt), sizeof(ClProtocol));

		for (psm_elt = sm_list_first(ionwm, (getBpVdb())->inducts); psm_elt;
				psm_elt = sm_list_next(ionwm, psm_elt))
		{
			vduct = (VInduct *) psp(ionwm, sm_list_data(ionwm, psm_elt));
			if (strcmp(vduct->protocolName, clpbuf.name) == 0)
			{

				GET_OBJ_POINTER(sdr, Induct, duct, sdr_list_data(sdr,
						vduct->inductElt));
				GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);
				if (sdr_string_read(sdr, cliCmdBuffer, duct->cliCmd) < 0)
				{
					cliCmd = "?";
				}
				else
				{
					cliCmd = cliCmdBuffer;
				}

				 cur_row = lyst_create();

				 val = utils_serialize_string(clp->name, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_string(duct->name, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_uint(vduct->cliPid, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_string(cliCmd, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 table_add_row(table, cur_row);
			}
		}
	}

	sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_inducts BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;
}

// If protocolName is specified, this table lists all outducts established locally for the indicated CL protocol. Otherwise, it lists all locally established outducts, regardless of their protocol.
// "STR:protocolName&STR:ductName&UINT:clpPid&STR:cloCmd&UINT:maxPayloadLength",
table_t *adm_IonBpAdmin_table_outducts()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "protocol", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "duct", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "cloPid", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "cloCmd", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "maxPayloadLen",AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr             sdr = getIonsdr();
	ClProtocol      clpbuf;
	Object          elt;
	PsmPartition    ionwm = getIonwm();
	VOutduct        *vduct;
	PsmAddress      psm_elt;
	OBJ_POINTER(Outduct, duct);
	OBJ_POINTER(ClProtocol, clp);
	char    cloCmdBuffer[SDRSTRING_BUFSZ];
	char    *cloCmd;
	Lyst 	cur_row = NULL;
	uint32_t size;
	uint8_t *val = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sdr_list_first(sdr, (getBpConstants())->protocols);
			elt; elt = sdr_list_next(sdr, elt))
	{
		sdr_read(sdr, (char *) &clpbuf,
				sdr_list_data(sdr, elt), sizeof(ClProtocol));

		 for (psm_elt = sm_list_first(ionwm, (getBpVdb())->outducts); psm_elt;
				 psm_elt = sm_list_next(ionwm, psm_elt))
		 {
			 vduct = (VOutduct *) psp(ionwm, sm_list_data(ionwm, psm_elt));
			 if (strcmp(vduct->protocolName, clpbuf.name) == 0)
			 {

				 GET_OBJ_POINTER(sdr, Outduct, duct, sdr_list_data(sdr,
						 vduct->outductElt));
				 GET_OBJ_POINTER(sdr, ClProtocol, clp, duct->protocol);

				 if (duct->cloCmd == 0)
				 {
					 cloCmd = "?";
				 }
				 else if (sdr_string_read(sdr, cloCmdBuffer, duct->cloCmd) < 0)
				 {
					 cloCmd = "?";
				 }
				 else
				 {
					 cloCmd = cloCmdBuffer;
				 }

				 cur_row = lyst_create();

				 val = utils_serialize_string(clp->name, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_string(duct->name, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_uint(vduct->cloPid, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_string(cloCmd, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 val = utils_serialize_uint(duct->maxPayloadLen, &size);
				 dc_add(cur_row, val, size);
				 SRELEASE(val);

				 table_add_row(table, cur_row);
			 }
		 }

	}

	sdr_exit_xn(sdr);


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_outducts BODY
	 * +-------------------------------------------------------------------------+
	 */



	return table;
}

// This table lists all convergence layer protocols that can currently be utilized at the local node.
// STR:protocolName&UINT:payloadBytesPerFrame&UINT:overheadBytesPerFrame&UINT:protocolClass
table_t *adm_IonBpAdmin_table_protocols()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "Name", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "payloadBFF", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "overheadBFF", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "protocolClass",AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */

	 Sdr     sdr = getIonsdr();
	 Object  elt;
	 OBJ_POINTER(ClProtocol, clp);
	 Lyst 	cur_row = NULL;
	 uint32_t size;
	 uint8_t *val = NULL;

	 CHKNULL(sdr_begin_xn(sdr));
	 for (elt = sdr_list_first(sdr, (getBpConstants())->protocols); elt;
			 elt = sdr_list_next(sdr, elt))
	 {
		 GET_OBJ_POINTER(sdr, ClProtocol, clp, sdr_list_data(sdr, elt));

		 cur_row = lyst_create();

		 val = utils_serialize_string(clp->name, &size);
		 dc_add(cur_row, val, size);
		 SRELEASE(val);

		 val = utils_serialize_uint(clp->payloadBytesPerFrame, &size);
		 dc_add(cur_row, val, size);
		 SRELEASE(val);

		 val = utils_serialize_uint(clp->overheadPerFrame, &size);
		 dc_add(cur_row, val, size);
		 SRELEASE(val);

		 val = utils_serialize_uint(clp->protocolClass, &size);
		 dc_add(cur_row, val, size);
		 SRELEASE(val);

		 table_add_row(table, cur_row);
	 }

	 sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_protocols BODY
	 * +-------------------------------------------------------------------------+
	 */



	return table;

}

// This table lists all declared endpoint naming schemes.
// STR:schemeName&UINT:fwdPid&STR:fwdCmd&UINT:admAppPidSTR:admAppCmd
table_t *adm_IonBpAdmin_table_schemes()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "schemeName", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "fwdPid", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "fwdCmd", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "admAppPid", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "admAppCmd", AMP_TYPE_STRING) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */

	Sdr             sdr = getIonsdr();
	PsmPartition    ionwm = getIonwm();
	PsmAddress      elt;
	VScheme         *vscheme;
    OBJ_POINTER(Scheme, scheme);
    char    fwdCmdBuffer[SDRSTRING_BUFSZ];
    char    *fwdCmd;
    char    admAppCmdBuffer[SDRSTRING_BUFSZ];
    char    *admAppCmd;
    Lyst 	cur_row = NULL;
    uint32_t size;
    uint8_t *val = NULL;

	CHKNULL(sdr_begin_xn(sdr));
	for (elt = sm_list_first(ionwm, (getBpVdb())->schemes); elt;
			elt = sm_list_next(ionwm, elt))
	{
		vscheme = (VScheme *) psp(ionwm, sm_list_data(ionwm, elt));

        GET_OBJ_POINTER(sdr, Scheme, scheme, sdr_list_data(sdr,
                        vscheme->schemeElt));

        if (sdr_string_read(sdr, fwdCmdBuffer, scheme->fwdCmd) < 0)
        {
        	fwdCmd = "?";
        }
        else
        {
        	fwdCmd = fwdCmdBuffer;
        }
        if (sdr_string_read(sdr, admAppCmdBuffer, scheme->admAppCmd) < 0)
        {
        	admAppCmd = "?";
        }
        else
        {
        	admAppCmd = admAppCmdBuffer;
        }


		cur_row = lyst_create();

    	val = utils_serialize_string(scheme->name, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_uint(vscheme->fwdPid, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_string(fwdCmd, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_uint(vscheme->admAppPid, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_string(admAppCmd, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	table_add_row(table, cur_row);
	}

	sdr_exit_xn(sdr);


	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_schemes BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;
}

// This table lists all declared egress plans.
// STR:endpointName&UINT:PID&UINTT:TransmissionRate
table_t *adm_IonBpAdmin_table_egressPlans()
{
	table_t  *table = NULL;

	if((table = table_create(NULL, NULL)) == NULL)
	{
		return NULL;
	}

	if((table_add_col(table, "endpointName", AMP_TYPE_STRING) == ERROR) ||
	   (table_add_col(table, "PID", AMP_TYPE_UINT) == ERROR) ||
	   (table_add_col(table, "TxRate", AMP_TYPE_UINT) == ERROR))
	{
		table_destroy(table, 1);
		return NULL;
	}

	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_table_egressPlans BODY
	 * +-------------------------------------------------------------------------+
	 */


    Sdr             sdr = getIonsdr();
    PsmPartition    ionwm = getIonwm();
    PsmAddress      elt;
    VPlan           *vplan;
    OBJ_POINTER(BpPlan, plan);
    uint8_t *val = NULL;
    uint32_t size = 0;
    Lyst cur_row;

    CHKNULL(sdr_begin_xn(sdr));
    for (elt = sm_list_first(ionwm, (getBpVdb())->plans); elt;
    		elt = sm_list_next(ionwm, elt))
    {
    	vplan = (VPlan *) psp(ionwm, sm_list_data(ionwm, elt));


    	GET_OBJ_POINTER(sdr, BpPlan, plan, sdr_list_data(sdr, vplan->planElt));

		cur_row = lyst_create();

    	val = utils_serialize_string(plan->neighborEid, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_uint(vplan->clmPid, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	val = utils_serialize_uint(plan->nominalRate, &size);
    	dc_add(cur_row, val, size);
    	SRELEASE(val);

    	table_add_row(table, cur_row);
    }

    sdr_exit_xn(sdr);

	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_table_egressPlans BODY
	 * +-------------------------------------------------------------------------+
	 */

	return table;
}



/* Collect Functions */
// This is the version of ion that is currently installed and the crypto suite that BP was compiled with.
value_t adm_IonBpAdmin_get_version(tdc_t params)
{
	value_t result;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_get_version BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	result = val_from_string("Version 6");
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_get_version BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* Control Functions */
// This control establishes a DTN endpoint named endpointId on the local node. The remaining parameters indicate what is to be done when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception. If type is 0, then such bundles are to be discarded silently and immediately. If type is 1, then such bundles are to be enqueued for later delivery and, if recvScript is provided, recvScript is to be executed.
tdc_t *adm_IonBpAdmin_ctrl_endpointAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *EndpointId = NULL;
	uint32_t rule = 0;
	char *script = NULL;
	
	EndpointId = adm_extract_string(params,0,&success);
	if(success)
	{
	rule = adm_extract_uint(params,1,&success);
	}
	if(success);
	{
	script = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	addEndpoint(EndpointId, rule, script);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(EndpointId);
	SRELEASE(script);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control changes the action that is to be taken when bundles destined for this endpoint arrive at a time when no application has the endpoint open for bundle reception.
tdc_t *adm_IonBpAdmin_ctrl_endpointChange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointChange BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *EndpointId = NULL;
	uint32_t rule = 0;
	char *script = NULL;
	
	EndpointId = adm_extract_string(params,0,&success);
	if(success)
	{
	rule = adm_extract_uint(params,1,&success);
	}
	if(success);
	{
	script = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	updateEndpoint(EndpointId, rule, script);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(EndpointId);
	SRELEASE(script);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the endpoint identified by endpointId. The control will fail if any bundles are currently pending delivery to this endpoint.
tdc_t *adm_IonBpAdmin_ctrl_endpointDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *EndpointId = NULL;
	
	EndpointId = adm_extract_string(params,0,&success);
	
	if(success)
	{
	removeEndpoint(EndpointId);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(EndpointId);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_endpointDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control establishes a duct for reception of bundles via the indicated CL protocol. The duct's data acquisition structure is used and populated by the induct task whose operation is initiated by cliControl at the time the duct is started.
tdc_t *adm_IonBpAdmin_ctrl_inductAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	char *cmd = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	if(success)
	{
	cmd = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	addInduct(protocol, duct, cmd);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	SRELEASE(cmd);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This changes the control that is used to initiate operation of the induct task for the indicated duct.
tdc_t *adm_IonBpAdmin_ctrl_inductChange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductChange BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	char *cmd = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	if(success);
	{
	cmd = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	updateInduct(protocol, duct, cmd);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	SRELEASE(cmd);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the induct identified by protocolName and ductName. The control will fail if any bundles are currently pending acquisition via this induct.
tdc_t *adm_IonBpAdmin_ctrl_inductDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	removeInduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control starts the indicated induct task as defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_inductStart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductStart BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	bpStartInduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control stops the indicated induct task as defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_inductStop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductStop BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	bpStopInduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_inductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control declares the maximum number of bytes of SDR heap space that will be occupied by any single bundle acquisition activity (nominally the acquisition of a single bundle, but this is at the discretion of the convergence-layer input task). All data acquired in excess of this limit will be written to a temporary file pending extraction and dispatching of the acquired bundle or bundles. The default is the minimum allowed value (560 bytes), which is the approximate size of a ZCO file reference object; this is the minimum SDR heap space occupancy in the event that all acquisition is into a file.
tdc_t *adm_IonBpAdmin_ctrl_manageHeapMax(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_manageHeapMax BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	Sdr      sdr = getIonsdr();
	Object   bpdbObj = getBpDbObject();
	BpDB     bpdb;
	uint32_t heapmax = 0;
	int8_t   success = 0;
	
	heapmax = adm_extract_uint(params,0,&success);
	
	if((success) && (heapmax >= 560))
	{
	CHKNULL(sdr_begin_xn(sdr));
	sdr_stage(sdr, (char *) &bpdb, bpdbObj, sizeof(BpDB));
	bpdb.maxAcqInHeap = heapmax;
	sdr_write(sdr, bpdbObj, (char *) &bpdb, sizeof(BpDB));
	sdr_end_xn(sdr);
	*status = CTRL_SUCCESS;
	}
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_manageHeapMax BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control establishes a duct for transmission of bundles via the indicated CL protocol. the duct's data transmission structure is serviced by the outduct task whose operation is initiated by CLOcommand at the time the duct is started. A value of zero for maxPayloadLength indicates that bundles of any size can be accomodated;this is the default.
tdc_t *adm_IonBpAdmin_ctrl_outductAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	//	"STR:protocolName&STR:ductName&STR:cloCommand&UINT:maxPayloadLength",
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	char *cmd = NULL;
	uint32_t len = 0;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	if(success);
	{
	cmd = adm_extract_string(params,2,&success);
	}
	if(success)
	{
	len = adm_extract_uint(params, 3, &success);
	}
	
	if(success)
	{
	addOutduct(protocol, duct, cmd, len);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	SRELEASE(cmd);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control sets new values for the indicated duct's payload size limit and the control that is used to initiate operation of the outduct task for this duct.
tdc_t *adm_IonBpAdmin_ctrl_outductChange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductChange BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	char *cmd = NULL;
	uint32_t len = 0;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	if(success);
	{
	cmd = adm_extract_string(params,2,&success);
	}
	if(success)
	{
	len = adm_extract_uint(params, 3, &success);
	}
	
	if(success)
	{
	updateOutduct(protocol, duct, cmd, len);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	SRELEASE(cmd);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the outduct identified by protocolName and ductName. The control will fail if any bundles are currently pending transmission via this outduct.
tdc_t *adm_IonBpAdmin_ctrl_outductDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	removeOutduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control starts the indicated outduct task as defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_outductStart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductStart BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	bpStartOutduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control stops the indicated outduct task as defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_outductStop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductStop BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *protocol = NULL;
	char *duct = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	duct = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	bpStopOutduct(protocol, duct);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	SRELEASE(duct);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_outductStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control establishes access to the named convergence layer protocol at the local node. The payloadBytesPerFrame and overheadBytesPerFrame arguments are used in calculating the estimated transmission capacity consumption of each bundle, to aid in route computation and congesting forecasting. The optional nominalDataRate argument overrides the hard coded default continuous data rate for the indicated protocol for purposes of rate control. For all promiscuous prototocols-that is, protocols whose outducts are not specifically dedicated to transmission to a single identified convergence-layer protocol endpoint- the protocol's applicable nominal continuous data rate is the data rate that is always used for rate control over links served by that protocol; data rates are not extracted from contact graph information. This is because only the induct and outduct throttles for non-promiscuous protocols (LTP, TCP) can be dynamically adjusted in response to changes in data rate between the local node and its neighbors, as enacted per the contact plan. Even for an outduct of a non-promiscuous protocol the nominal data rate may be the authority for rate control, in the event that the contact plan lacks identified contacts with the node to which the outduct is mapped.
tdc_t *adm_IonBpAdmin_ctrl_protocolAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *protocol = NULL;
	uint32_t payload = 0;
	uint32_t overhead = 0;
	uint32_t rate = 0;
	
	protocol = adm_extract_string(params,0,&success);
	if(success)
	{
	payload = adm_extract_uint(params,1,&success);
	}
	if(success)
	{
	overhead = adm_extract_uint(params,2,&success);
	}
	if(success)
	{
	rate = adm_extract_uint(params,3,&success);
	}
	
	if(success)
	{
	addProtocol(protocol, payload, overhead, rate);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the convergence layer protocol identified by protocolName. The control will fail if any ducts are still locally declared for this protocol.
tdc_t *adm_IonBpAdmin_ctrl_protocolDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *protocol = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	
	if(success)
	{
	removeProtocol(protocol);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control starts all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_protocolStart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolStart BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *protocol = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStartProtocol(protocol);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control stops all induct and outduct tasks for inducts and outducts that have been defined for the indicated CL protocol on the local node.
tdc_t *adm_IonBpAdmin_ctrl_protocolStop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolStop BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *protocol = NULL;
	
	protocol = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStopProtocol(protocol);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(protocol);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_protocolStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control declares an endpoint naming scheme for use in endpoint IDs, which are structured as URIs: schemeName:schemeSpecificPart. forwarderControl will be executed when the scheme is started on this node, to initiate operation of a forwarding daemon for this scheme. adminAppControl will also be executed when the scheme is started on this node, to initiate operation of a daemon that opens a custodian endpoint identified within this scheme so that it can recieve and process custody signals and bundle status reports.
tdc_t *adm_IonBpAdmin_ctrl_schemeAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	char *fwdCmd = NULL;
	char *admCmd = NULL;
	
	name = adm_extract_string(params,0,&success);
	if(success)
	{
	fwdCmd = adm_extract_string(params,1,&success);
	}
	if(success)
	{
	admCmd = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	addScheme(name, fwdCmd, admCmd);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	SRELEASE(fwdCmd);
	SRELEASE(admCmd);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control sets the indicated scheme's forwarderControl and adminAppControl to the strings provided as arguments.
tdc_t *adm_IonBpAdmin_ctrl_schemeChange(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeChange BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *name = NULL;
	char *fwdCmd = NULL;
	char *admCmd = NULL;
	
	name = adm_extract_string(params,0,&success);
	if(success)
	{
	fwdCmd = adm_extract_string(params,1,&success);
	}
	if(success)
	{
	admCmd = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	updateScheme(name, fwdCmd, admCmd);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	SRELEASE(fwdCmd);
	SRELEASE(admCmd);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeChange BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control deletes the scheme identified by schemeName. The control will fail if any bundles identified in this scheme are pending forwarding, transmission, or delivery.
tdc_t *adm_IonBpAdmin_ctrl_schemeDel(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeDel BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	removeScheme(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeDel BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control starts the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.
tdc_t *adm_IonBpAdmin_ctrl_schemeStart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeStart BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStartScheme(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This control stops the forwarder and administrative endpoint tasks for the indicated scheme task on the local node.
tdc_t *adm_IonBpAdmin_ctrl_schemeStop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeStop BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStopScheme(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_schemeStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command establishes an egress plan governing transmission to the neighboring node[s] identified by endpoint_name.  The plan is functionally enacted by a bpclm(1) daemon dedicated to managing bundles queued for transmission to the indicated neighboring node[s].
tdc_t *adm_IonBpAdmin_ctrl_egressPlanAdd(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanAdd BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *name = NULL;
	uint32_t rate = 0;
	
	name = adm_extract_string(params,0,&success);
	if(success)
	{
	rate = adm_extract_uint(params,1,&success);
	}
	
	if(success)
	{
	addPlan(name, rate);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanAdd BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command sets a new value for the indicated plan's transmission rate.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanUpdate(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanUpdate BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *name = NULL;
	uint32_t rate = 0;
	
	name = adm_extract_string(params,0,&success);
	if(success)
	{
	rate = adm_extract_uint(params,1,&success);
	}
	
	if(success)
	{
	updatePlan(name, rate);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanUpdate BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command deletes the outduct identified by endpoint_name.  The command will fail if any bundles are currently pending transmission per this plan.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanDelete(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanDelete BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	removePlan(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanDelete BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command starts the bpclm task for the indicated egress plan.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanStart(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanStart BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStartPlan(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanStart BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command stops the bpclm task for the indicated egress plan.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanStop(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanStop BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpStopPlan(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanStop BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command disables transmission of bundles queued for transmission to the indicated node and reforwards all non-critical bundles currently queued for transmission to this node.  This may result in some or all of these bundles being enqueued for transmission (actually just retention) to the pseudo-node limbo
tdc_t *adm_IonBpAdmin_ctrl_egressPlanBlock(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanBlock BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpBlockPlan(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanBlock BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command re-enables transmission of bundles to the indicated node and reforwards all bundles in limbo in the hope that the unblocking of this egress plan will enable some of them to be transmitted.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanUnblock(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanUnblock BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *name = NULL;
	
	name = adm_extract_string(params,0,&success);
	
	if(success)
	{
	bpUnblockPlan(name);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(name);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanUnblock BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command declares the name of the endpoint to which bundles queued for transmission to the node[s] identified byendpoint_name must be re-routed.  Declaring gateway_endpoint_name to be the zero-length string '' disables re-routing: bundles will instead be transmitted using the plan's attached convergence-layer protocol outduct[s].
tdc_t *adm_IonBpAdmin_ctrl_egressPlanGateway(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanGateway BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *eid = NULL;
	char *viaEid = NULL;
	
	eid = adm_extract_string(params,0,&success);
	if(success)
	{
	viaEid = adm_extract_string(params,1,&success);
	}
	
	if(success)
	{
	
	setPlanViaEid(eid, viaEid);
	*status = CTRL_SUCCESS;
	}
	
	SRELEASE(eid);
	SRELEASE(viaEid);
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanGateway BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command declares that the indicated convergence-layer protocol outduct is now a viable device for transmitting bundles to the node[s] identified by endpoint_name.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanOutductAttach(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanOutductAttach BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *endpointName = NULL;
	char *protocolName = NULL;
	char *ductName = NULL;
	
	endpointName = adm_extract_string(params,0,&success);
	if(success)
	{
	protocolName = adm_extract_string(params,1,&success);
	}
	if(success)
	{
	ductName = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	VOutduct        *vduct;
	PsmAddress      vductElt;
	
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)
	{
	attachPlanDuct(endpointName, vduct->outductElt);
	*status = CTRL_SUCCESS;
	}
	}
	
	SRELEASE(endpointName);
	SRELEASE(protocolName);
	SRELEASE(ductName);
	
	
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanOutductAttach BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}

// This command declares that the indicated convergence-layer protocol outduct is no longer a viable device for transmitting bundles to the node[s] identified by endpoint_name.
tdc_t *adm_IonBpAdmin_ctrl_egressPlanOutductDetatch(eid_t *def_mgr, tdc_t params, int8_t *status)
{
	tdc_t* result = NULL;
	*status = CTRL_FAILURE;
	/*
	 * +-------------------------------------------------------------------------+
	 * |START CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanOutductDetatch BODY
	 * +-------------------------------------------------------------------------+
	 */

	
	int8_t success = 0;
	char *endpointName = NULL;
	char *protocolName = NULL;
	char *ductName = NULL;
	
	endpointName = adm_extract_string(params,0,&success);
	if(success)
	{
	protocolName = adm_extract_string(params,1,&success);
	}
	if(success)
	{
	ductName = adm_extract_string(params,2,&success);
	}
	
	if(success)
	{
	VOutduct        *vduct;
	PsmAddress      vductElt;
	
	findOutduct(protocolName, ductName, &vduct, &vductElt);
	if (vductElt != 0)
	{
	detachPlanDuct(endpointName, vduct->outductElt);
	*status = CTRL_SUCCESS;
	}
	}
	
	SRELEASE(endpointName);
	SRELEASE(protocolName);
	SRELEASE(ductName);
	
	
	/*
	 * +-------------------------------------------------------------------------+
	 * |STOP CUSTOM FUNCTION adm_IonBpAdmin_ctrl_egressPlanOutductDetatch BODY
	 * +-------------------------------------------------------------------------+
	 */
	return result;
}



/* OP Functions */
