/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 **  This files contains the functions that convert bp abstract types in dtn types and vice versa.
 ********************************************************/

/*
 * al_al_dtn_conversions.c
 *
 */

#ifdef DTN2_IMPLEMENTATION

#include "al_bp_dtn_conversions.h"

/*
 * These functions convert bp abstract types in dtn types and viceversa
 * The prefix bp_dtn means the function takes a bp abstract type in and returns a dtn type
 * so the conversion is bp -> dtn
 * The prefix dtn_bp means the function takes a dtn type in and returns a bp abstract type
 * so the conversion is dtn -> bp
 */

dtn_handle_t al_dtn_handle(al_bp_handle_t handle)
{
	return (dtn_handle_t) handle;
}
al_bp_handle_t dtn_al_handle(dtn_handle_t handle)
{
	return (al_bp_handle_t) handle;
}

dtn_endpoint_id_t al_dtn_endpoint_id(al_bp_endpoint_id_t endpoint_id)
{
	dtn_endpoint_id_t dtn_eid;
	strncpy(dtn_eid.uri, endpoint_id.uri, DTN_MAX_ENDPOINT_ID);
	return dtn_eid;
}
al_bp_endpoint_id_t dtn_al_endpoint_id(dtn_endpoint_id_t endpoint_id)
{
	al_bp_endpoint_id_t bp_eid;
	strncpy(bp_eid.uri, endpoint_id.uri, AL_BP_MAX_ENDPOINT_ID);
	return bp_eid;
}

dtn_timeval_t al_dtn_timeval(al_bp_timeval_t timeval)
{
	return (al_bp_timeval_t) timeval;
}
al_bp_timeval_t dtn_al_timeval(dtn_timeval_t timeval)
{
	return (dtn_timeval_t) timeval;
}

dtn_timestamp_t al_dtn_timestamp(al_bp_timestamp_t timestamp)
{
	dtn_timestamp_t dtn_timestamp;
	dtn_timestamp.secs = timestamp.secs;
	dtn_timestamp.seqno = timestamp.seqno;
	return dtn_timestamp;
}
al_bp_timestamp_t dtn_al_timestamp(dtn_timestamp_t timestamp)
{
	al_bp_timestamp_t bp_timestamp;
	bp_timestamp.secs = timestamp.secs;
	bp_timestamp.seqno = timestamp.seqno;
	return bp_timestamp;
}

dtn_reg_token_t al_dtn_reg_token(al_bp_reg_token_t reg_token)
{
	return (dtn_reg_token_t) reg_token;
}
al_bp_reg_token_t dtn_al_reg_token(dtn_reg_token_t reg_token)
{
	return (al_bp_reg_token_t) reg_token;

}

dtn_reg_id_t al_dtn_reg_id(al_bp_reg_id_t reg_id)
{
	return (dtn_reg_id_t) reg_id;
}
al_bp_reg_id_t dtn_al_reg_id(dtn_reg_id_t reg_id)
{
	return (al_bp_reg_id_t) reg_id;
}

dtn_reg_info_t al_dtn_reg_info(al_bp_reg_info_t reg_info)
{
	dtn_reg_info_t dtn_reginfo;
	memset(&dtn_reginfo, 0, sizeof(dtn_reg_info_t));
	dtn_reginfo.endpoint = al_dtn_endpoint_id(reg_info.endpoint);
	dtn_reginfo.regid = al_dtn_reg_id(reg_info.regid);
	dtn_reginfo.flags = reg_info.flags;
	dtn_reginfo.replay_flags = reg_info.replay_flags;
	dtn_reginfo.expiration = al_dtn_timeval(reg_info.expiration);
	dtn_reginfo.reg_token = al_dtn_reg_token(reg_info.reg_token);
	dtn_reginfo.init_passive = reg_info.init_passive;
	dtn_reginfo.script.script_len = reg_info.script.script_len;
	if (reg_info.script.script_len == 0)
	{
		dtn_reginfo.script.script_val = NULL;
	}
	else
	{
		dtn_reginfo.script.script_val = (char*) malloc(reg_info.script.script_len + 1);
		strncpy(dtn_reginfo.script.script_val, reg_info.script.script_val, reg_info.script.script_len + 1);
	}
	return dtn_reginfo;
}
al_bp_reg_info_t dtn_al_reg_info(dtn_reg_info_t reg_info)
{
	al_bp_reg_info_t bp_reginfo;
	memset(&bp_reginfo, 0, sizeof(al_bp_reg_info_t));
	bp_reginfo.endpoint = dtn_al_endpoint_id(reg_info.endpoint);
	bp_reginfo.regid = dtn_al_reg_id(reg_info.regid);
	bp_reginfo.flags = reg_info.flags;
	bp_reginfo.replay_flags = reg_info.replay_flags;
	bp_reginfo.expiration = dtn_al_timeval(reg_info.expiration);
	bp_reginfo.reg_token = dtn_al_reg_token(reg_info.reg_token);
	bp_reginfo.init_passive = reg_info.init_passive;
	bp_reginfo.script.script_len = reg_info.script.script_len;
	if (reg_info.script.script_len == 0)
	{
		bp_reginfo.script.script_val = NULL;
	}
	else
	{
		bp_reginfo.script.script_val = (char*) malloc(reg_info.script.script_len + 1);
		strncpy(bp_reginfo.script.script_val, reg_info.script.script_val, reg_info.script.script_len + 1);
	}
	return bp_reginfo;
}

dtn_reg_flags_t al_dtn_reg_flags(al_bp_reg_flags_t reg_flags)
{
	return (dtn_reg_flags_t) reg_flags;
}
al_bp_reg_flags_t dtn_al_reg_flags(dtn_reg_flags_t reg_flags)
{
	return (al_bp_reg_flags_t) reg_flags;
}

dtn_bundle_delivery_opts_t al_dtn_bundle_delivery_opts(al_bp_bundle_delivery_opts_t bundle_delivery_opts)
{
	return (dtn_bundle_delivery_opts_t) bundle_delivery_opts;
}
al_bp_bundle_delivery_opts_t dtn_al_bundle_delivery_opts(dtn_bundle_delivery_opts_t bundle_delivery_opts)
{
	return (al_bp_bundle_delivery_opts_t) bundle_delivery_opts;
}


dtn_bundle_priority_t al_dtn_bundle_priority(al_bp_bundle_priority_t bundle_priority)
{
	return (dtn_bundle_priority_t) bundle_priority.priority;
}
al_bp_bundle_priority_t dtn_al_bundle_priority(dtn_bundle_priority_t bundle_priority)
{
	al_bp_bundle_priority_t bp_priority;
	bp_priority.priority = (al_bp_bundle_priority_enum) bundle_priority;
	bp_priority.ordinal = 0;
	return bp_priority;
}

dtn_bundle_spec_t al_dtn_bundle_spec(al_bp_bundle_spec_t bundle_spec)
{
	dtn_bundle_spec_t dtn_bundle_spec;
	int i;
	al_bp_extension_block_t dtn_bundle_block;
	memset(&dtn_bundle_spec, 0, sizeof(dtn_bundle_spec));
	memset(&dtn_bundle_block, 0, sizeof(dtn_bundle_block));
	dtn_bundle_spec.source = al_dtn_endpoint_id(bundle_spec.source);
	dtn_bundle_spec.dest = al_dtn_endpoint_id(bundle_spec.dest);
	dtn_bundle_spec.replyto = al_dtn_endpoint_id(bundle_spec.replyto);
	dtn_bundle_spec.priority = al_dtn_bundle_priority(bundle_spec.priority);
	dtn_bundle_spec.dopts = al_dtn_bundle_delivery_opts(bundle_spec.dopts);
	dtn_bundle_spec.expiration = al_dtn_timeval(bundle_spec.expiration);
	dtn_bundle_spec.creation_ts = al_dtn_timestamp(bundle_spec.creation_ts);
	dtn_bundle_spec.delivery_regid = al_dtn_reg_id(bundle_spec.delivery_regid);
	dtn_bundle_spec.blocks.blocks_len = bundle_spec.blocks.blocks_len;
	dtn_bundle_spec.metadata.metadata_len = bundle_spec.metadata.metadata_len;
	if(dtn_bundle_spec.blocks.blocks_len == 0)
		dtn_bundle_spec.blocks.blocks_val = NULL;
	else
	{
		dtn_bundle_spec.blocks.blocks_val =
				(dtn_extension_block_t*) malloc(bundle_spec.blocks.blocks_len);
	    for(i=0; i<bundle_spec.blocks.blocks_len; i++)
	    {
	    	dtn_bundle_block = bundle_spec.blocks.blocks_val[i];
	        dtn_bundle_spec.blocks.blocks_val[i].type = dtn_bundle_block.type;
	        dtn_bundle_spec.blocks.blocks_val[i].flags = dtn_bundle_block.flags;
	        dtn_bundle_spec.blocks.blocks_val[i].data.data_len = dtn_bundle_block.data.data_len;
	        if(dtn_bundle_block.data.data_len == 0)
	        	dtn_bundle_spec.blocks.blocks_val[i].data.data_val = NULL;
	        else
	        {
	        	dtn_bundle_spec.blocks.blocks_val[i].data.data_val =
	        			(char*) malloc(dtn_bundle_block.data.data_len + 1);
	            memcpy(dtn_bundle_spec.blocks.blocks_val[i].data.data_val,
	            		dtn_bundle_block.data.data_val, (dtn_bundle_block.data.data_len) + 1);
	            dtn_bundle_spec.blocks.blocks_val[i].data.data_val =
	            		(char*)dtn_bundle_block.data.data_val;
	        }
	    }
	}
	if (dtn_bundle_spec.metadata.metadata_len == 0)
		dtn_bundle_spec.metadata.metadata_val = NULL;
	else
	{
		dtn_bundle_spec.metadata.metadata_val =
				(dtn_extension_block_t*) malloc(bundle_spec.metadata.metadata_len);
	    for(i=0; i<bundle_spec.metadata.metadata_len; i++)
	    {
	    	dtn_bundle_block = bundle_spec.metadata.metadata_val[i];
	        dtn_bundle_spec.metadata.metadata_val[i].type = dtn_bundle_block.type;
	        dtn_bundle_spec.metadata.metadata_val[i].flags = dtn_bundle_block.flags;
	        dtn_bundle_spec.metadata.metadata_val[i].data.data_len = dtn_bundle_block.data.data_len;
	        if(dtn_bundle_block.data.data_len == 0)
	        	dtn_bundle_spec.metadata.metadata_val[i].data.data_val = NULL;
	        else
	        {
	        	dtn_bundle_spec.metadata.metadata_val[i].data.data_val =
	        			(char*) malloc(dtn_bundle_block.data.data_len + 1);
	            memcpy(dtn_bundle_spec.metadata.metadata_val[i].data.data_val,
	            		dtn_bundle_block.data.data_val, (dtn_bundle_block.data.data_len) + 1);
	            dtn_bundle_spec.metadata.metadata_val[i].data.data_val =
	            		(char*)dtn_bundle_block.data.data_val;
	        }
	    }
	}
	return dtn_bundle_spec;
}
al_bp_bundle_spec_t dtn_al_bundle_spec(dtn_bundle_spec_t bundle_spec)
{
	al_bp_bundle_spec_t bp_bundle_spec;
	int i;
	dtn_extension_block_t bp_bundle_block;
	memset(&bp_bundle_spec, 0, sizeof(bp_bundle_spec));
	memset(&bp_bundle_block, 0, sizeof(bp_bundle_block));
	bp_bundle_spec.source = dtn_al_endpoint_id(bundle_spec.source);
	bp_bundle_spec.dest = dtn_al_endpoint_id(bundle_spec.dest);
	bp_bundle_spec.replyto = dtn_al_endpoint_id(bundle_spec.replyto);
	bp_bundle_spec.priority = dtn_al_bundle_priority(bundle_spec.priority);
	bp_bundle_spec.dopts = dtn_al_bundle_delivery_opts(bundle_spec.dopts);
	bp_bundle_spec.expiration = dtn_al_timeval(bundle_spec.expiration);
	bp_bundle_spec.creation_ts = dtn_al_timestamp(bundle_spec.creation_ts);
	bp_bundle_spec.delivery_regid = dtn_al_reg_id(bundle_spec.delivery_regid);
	bp_bundle_spec.blocks.blocks_len = bundle_spec.blocks.blocks_len;
	bp_bundle_spec.metadata.metadata_len = bundle_spec.metadata.metadata_len;
	if(bp_bundle_spec.blocks.blocks_len == 0)
		bp_bundle_spec.blocks.blocks_val = NULL;
	else
	{
		bp_bundle_spec.blocks.blocks_val =
//dz debug				(al_bp_extension_block_t*) malloc(bundle_spec.blocks.blocks_len);
				(al_bp_extension_block_t*) malloc(bundle_spec.blocks.blocks_len * sizeof(al_bp_extension_block_t));
	    for(i=0; i<bundle_spec.blocks.blocks_len; i++)
	    {
	    	bp_bundle_block = bundle_spec.blocks.blocks_val[i];
	        bp_bundle_spec.blocks.blocks_val[i].type = bp_bundle_block.type;
	        bp_bundle_spec.blocks.blocks_val[i].flags = bp_bundle_block.flags;
	        bp_bundle_spec.blocks.blocks_val[i].data.data_len = bp_bundle_block.data.data_len;
	        if(bp_bundle_block.data.data_len == 0)
	        	bp_bundle_spec.blocks.blocks_val[i].data.data_val = NULL;
	        else
	        {
//dz debug	        	bp_bundle_spec.blocks.blocks_val[i].data.data_val =
//dz debug	        			(char*) malloc(bp_bundle_block.data.data_len + 1);
//dz debug	            memcpy(bp_bundle_spec.blocks.blocks_val[i].data.data_val,
//dz debug	            		bp_bundle_block.data.data_val, (bp_bundle_block.data.data_len) + 1);
	        	bp_bundle_spec.blocks.blocks_val[i].data.data_val =
	        			(char*) malloc(bp_bundle_block.data.data_len);
	            memcpy(bp_bundle_spec.blocks.blocks_val[i].data.data_val,
	            		bp_bundle_block.data.data_val, bp_bundle_block.data.data_len);

//XXX/dz - Just copied the data above so this is not needed and would overwrite the data_val pointerr 
//dz debug	            bp_bundle_spec.blocks.blocks_val[i].data.data_val =
//dz debug	            		(char*)bp_bundle_block.data.data_val;
	        }
	    }
	}
	if (bp_bundle_spec.metadata.metadata_len == 0)
		bp_bundle_spec.metadata.metadata_val = NULL;
	else
	{
		bp_bundle_spec.metadata.metadata_val =
				(al_bp_extension_block_t*) malloc(bundle_spec.metadata.metadata_len);
	    for(i=0; i<bundle_spec.metadata.metadata_len; i++)
	    {
	    	bp_bundle_block = bundle_spec.metadata.metadata_val[i];
	        bp_bundle_spec.metadata.metadata_val[i].type = bp_bundle_block.type;
	        bp_bundle_spec.metadata.metadata_val[i].flags = bp_bundle_block.flags;
	        bp_bundle_spec.metadata.metadata_val[i].data.data_len = bp_bundle_block.data.data_len;
	        if(bp_bundle_block.data.data_len == 0)
	        	bp_bundle_spec.metadata.metadata_val[i].data.data_val = NULL;
	        else
	        {
	        	bp_bundle_spec.metadata.metadata_val[i].data.data_val =
	        			(char*) malloc(bp_bundle_block.data.data_len + 1);
	            memcpy(bp_bundle_spec.metadata.metadata_val[i].data.data_val,
	            		bp_bundle_block.data.data_val, (bp_bundle_block.data.data_len) + 1);
	            bp_bundle_spec.metadata.metadata_val[i].data.data_val =
	            		(char*)bp_bundle_block.data.data_val;
	        }
	    }
	}
	return bp_bundle_spec;
}

dtn_bundle_payload_location_t al_dtn_bundle_payload_location(al_bp_bundle_payload_location_t bundle_payload_location)
{
	return (dtn_bundle_payload_location_t) bundle_payload_location;
}
al_bp_bundle_payload_location_t dtn_al_bundle_payload_location(dtn_bundle_payload_location_t bundle_payload_location)
{
	return (al_bp_bundle_payload_location_t) bundle_payload_location;
}

dtn_status_report_reason_t al_dtn_status_report_reason(al_bp_status_report_reason_t status_report_reason)
{
	return (dtn_status_report_reason_t) status_report_reason;
}
al_bp_status_report_reason_t dtn_al_status_report_reason(dtn_status_report_reason_t status_report_reason)
{
	return (al_bp_status_report_reason_t) status_report_reason;
}

dtn_status_report_flags_t al_dtn_status_report_flags(al_bp_status_report_flags_t status_report_flags)
{
	return (dtn_status_report_flags_t) status_report_flags;
}
al_bp_status_report_flags_t dtn_al_status_report_flags(dtn_status_report_flags_t status_report_flags)
{
	return (al_bp_status_report_flags_t) status_report_flags;
}

dtn_bundle_id_t al_dtn_bundle_id(al_bp_bundle_id_t bundle_id)
{
	dtn_bundle_id_t dtn_bundle_id;
	dtn_bundle_id.source = al_dtn_endpoint_id(bundle_id.source);
	dtn_bundle_id.creation_ts = al_dtn_timestamp(bundle_id.creation_ts);
	if(bundle_id.frag_offset < 0)
		dtn_bundle_id.frag_offset = -1;
	else
		dtn_bundle_id.frag_offset = bundle_id.frag_offset;
	dtn_bundle_id.orig_length = bundle_id.orig_length;
	return dtn_bundle_id;
}
al_bp_bundle_id_t dtn_al_bundle_id(dtn_bundle_id_t bundle_id)
{
	al_bp_bundle_id_t bp_bundle_id;
	bp_bundle_id.source = dtn_al_endpoint_id(bundle_id.source);
	bp_bundle_id.creation_ts = dtn_al_timestamp(bundle_id.creation_ts);
	if(bundle_id.frag_offset < 0)
		bp_bundle_id.frag_offset = -1;
	else
		bp_bundle_id.frag_offset = bundle_id.frag_offset;
	bp_bundle_id.orig_length = bundle_id.orig_length;
	return bp_bundle_id;
}

dtn_bundle_status_report_t al_dtn_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report)
{
	dtn_bundle_status_report_t dtn_bundle_status_report;
	memset(&dtn_bundle_status_report, 0, sizeof(dtn_bundle_status_report_t));
	dtn_bundle_status_report.bundle_id = al_dtn_bundle_id(bundle_status_report.bundle_id);
	dtn_bundle_status_report.reason = al_dtn_status_report_reason(bundle_status_report.reason);
	dtn_bundle_status_report.flags = al_dtn_status_report_flags(bundle_status_report.flags);
	dtn_bundle_status_report.receipt_ts = al_dtn_timestamp(bundle_status_report.receipt_ts);
	dtn_bundle_status_report.custody_ts = al_dtn_timestamp(bundle_status_report.custody_ts);
	dtn_bundle_status_report.forwarding_ts = al_dtn_timestamp(bundle_status_report.forwarding_ts);
	dtn_bundle_status_report.delivery_ts = al_dtn_timestamp(bundle_status_report.delivery_ts);
	dtn_bundle_status_report.deletion_ts = al_dtn_timestamp(bundle_status_report.deletion_ts);
	dtn_bundle_status_report.ack_by_app_ts = al_dtn_timestamp(bundle_status_report.ack_by_app_ts);
	return dtn_bundle_status_report;
}
al_bp_bundle_status_report_t * dtn_al_bundle_status_report(dtn_bundle_status_report_t bundle_status_report)
{
	al_bp_bundle_status_report_t * bp_bundle_status_report = (al_bp_bundle_status_report_t *) malloc(sizeof(al_bp_bundle_status_report_t));
	memset(bp_bundle_status_report, 0, sizeof(al_bp_bundle_status_report_t));
	//printf("AL_BP: fragment offset dtn %d\n",bundle_status_report.bundle_id.frag_offset);
	bp_bundle_status_report->bundle_id = dtn_al_bundle_id(bundle_status_report.bundle_id);
	//printf("AL_BP: fragment offset al_bp %lu\n",bp_bundle_status_report.bundle_id.frag_offset);
	bp_bundle_status_report->reason = dtn_al_status_report_reason(bundle_status_report.reason);
	bp_bundle_status_report->flags = dtn_al_status_report_flags(bundle_status_report.flags);
	bp_bundle_status_report->receipt_ts = dtn_al_timestamp(bundle_status_report.receipt_ts);
	bp_bundle_status_report->custody_ts = dtn_al_timestamp(bundle_status_report.custody_ts);
	bp_bundle_status_report->forwarding_ts = dtn_al_timestamp(bundle_status_report.forwarding_ts);
	bp_bundle_status_report->delivery_ts = dtn_al_timestamp(bundle_status_report.delivery_ts);
	bp_bundle_status_report->deletion_ts = dtn_al_timestamp(bundle_status_report.deletion_ts);
	bp_bundle_status_report->ack_by_app_ts = dtn_al_timestamp(bundle_status_report.ack_by_app_ts);
	return bp_bundle_status_report;
}

dtn_bundle_payload_t al_dtn_bundle_payload(al_bp_bundle_payload_t bundle_payload)
{
	dtn_bundle_payload_t dtn_bundle_payload;
	memset(&dtn_bundle_payload, 0, sizeof(dtn_bundle_payload));
	dtn_bundle_payload.location = al_dtn_bundle_payload_location(bundle_payload.location);
	dtn_bundle_payload.filename.filename_len = bundle_payload.filename.filename_len;
	if (bundle_payload.filename.filename_len == 0)
	{
		dtn_bundle_payload.filename.filename_val = NULL;
	}
	else
	{
		dtn_bundle_payload.filename.filename_val = (char*) malloc(bundle_payload.filename.filename_len + 1);
		strncpy(dtn_bundle_payload.filename.filename_val, bundle_payload.filename.filename_val, bundle_payload.filename.filename_len + 1);
	}
	dtn_bundle_payload.buf.buf_len = bundle_payload.buf.buf_len;
	if (bundle_payload.buf.buf_len == 0)
	{
		dtn_bundle_payload.buf.buf_val = NULL;
	}
	else
	{
		dtn_bundle_payload.buf.buf_val = (char*) malloc(bundle_payload.buf.buf_len);
		memcpy(dtn_bundle_payload.buf.buf_val, bundle_payload.buf.buf_val, bundle_payload.buf.buf_len);
	}
	if (bundle_payload.status_report == NULL)
	{
		dtn_bundle_payload.status_report = NULL;
	}
	else
	{
		dtn_bundle_status_report_t dtn_bundle_status_report = al_dtn_bundle_status_report(*(bundle_payload.status_report));
		dtn_bundle_payload.status_report = & dtn_bundle_status_report;
	}
	return dtn_bundle_payload;
}
al_bp_bundle_payload_t dtn_al_bundle_payload(dtn_bundle_payload_t bundle_payload)
{
	al_bp_bundle_payload_t bp_bundle_payload;
	memset(&bp_bundle_payload, 0, sizeof(bp_bundle_payload));
	bp_bundle_payload.location = dtn_al_bundle_payload_location(bundle_payload.location);
	bp_bundle_payload.filename.filename_len = bundle_payload.filename.filename_len;
	if (bundle_payload.filename.filename_len == 0 || bundle_payload.filename.filename_val == NULL)
	{
		bp_bundle_payload.filename.filename_val = NULL;
		bp_bundle_payload.filename.filename_len = 0;
	}
	else
	{
		bp_bundle_payload.filename.filename_val = (char*) malloc(bundle_payload.filename.filename_len + 1);
		strncpy(bp_bundle_payload.filename.filename_val, bundle_payload.filename.filename_val, bundle_payload.filename.filename_len + 1);
	}
	bp_bundle_payload.buf.buf_len = bundle_payload.buf.buf_len;
	if (bundle_payload.buf.buf_len == 0 || bundle_payload.buf.buf_val == NULL)
	{
		bp_bundle_payload.buf.buf_len = 0;
		bp_bundle_payload.buf.buf_val = NULL;
	}
	else
	{
		bp_bundle_payload.buf.buf_val = (char*) malloc(bundle_payload.buf.buf_len);
		memcpy(bp_bundle_payload.buf.buf_val, bundle_payload.buf.buf_val, bundle_payload.buf.buf_len);
	}if (bundle_payload.status_report == NULL)
	{
		bp_bundle_payload.status_report = NULL;
	}
	else
	{
		al_bp_bundle_status_report_t * bp_bundle_status_report = dtn_al_bundle_status_report(*(bundle_payload.status_report));
		bp_bundle_payload.status_report = bp_bundle_status_report;
	}

	return bp_bundle_payload;
}
#endif /* DTN2_IMPLEMENTATION */
