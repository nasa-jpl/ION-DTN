/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **	      Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ** This file contains the functions that convert bp abstract types in ION types and vice versa.
 ********************************************************/

/*************
 *
 * bp_ion_conversion.c
 *
 * Conversions bp abstract types to ion types and viceversa
 *
 *************/
/****
 * These functions convert bp abstract types in ion types and viceversa
 * The prefix al_ion means the function takes a bp abstract type in and returns a ion type
 * so the conversion is bp -> ion
 * The prefix ion_al means the function takes a ion type in and returns a bp abstract type
 * so the conversion is ion -> bp
*****/

#ifdef ION_IMPLEMENTATION

#ifndef NEW_ZCO
#define  NEW_ZCO 1
#endif 


#include "al_bp_ion_conversions.h"

al_ion_handle_t al_ion_handle(al_bp_handle_t handle){
	return (al_ion_handle_t) handle;
}
al_bp_handle_t ion_al_handle(al_ion_handle_t handle){

	return (al_bp_handle_t) handle;
}

char * al_ion_endpoint_id(al_bp_endpoint_id_t endpoint_id)
{
	char * eid_ion;
	int length_eid = strlen(endpoint_id.uri)+1;
	eid_ion = (char *)(malloc(sizeof(char)*length_eid));
	memcpy(eid_ion,endpoint_id.uri,length_eid);
	return eid_ion;
}
al_bp_endpoint_id_t ion_al_endpoint_id(char * endpoint_id)
{
	al_bp_endpoint_id_t eid_bp;
	int length = strlen(endpoint_id)+1;
	memcpy(eid_bp.uri,endpoint_id,length);
	return eid_bp;
}

BpTimestamp al_ion_timestamp(al_bp_timestamp_t timestamp)
{
	BpTimestamp ion_timestamp;
	ion_timestamp.seconds = timestamp.secs;
	ion_timestamp.count = timestamp.seqno;
	return ion_timestamp;

}
al_bp_timestamp_t ion_al_timestamp(BpTimestamp timestamp)
{
	al_bp_timestamp_t bp_timestamp;
	bp_timestamp.secs = timestamp.seconds;
	bp_timestamp.seqno = timestamp.count;
	return bp_timestamp;
}

DtnTime al_ion_timeval(al_bp_timeval_t timeval){
	DtnTime dtntime;
	dtntime.seconds = timeval;
	dtntime.nanosec = 0;
	return dtntime;
}
al_bp_timeval_t ion_al_timeval(DtnTime timeval){
	al_bp_timeval_t time;
	time = timeval.seconds;
	return time;
}

unsigned char al_ion_bundle_srrFlags(al_bp_bundle_delivery_opts_t bundle_delivery_opts){
	int opts = 0;
	if(bundle_delivery_opts & BP_DOPTS_RECEIVE_RCPT)
		opts |= BP_RECEIVED_RPT;
	if(bundle_delivery_opts & BP_DOPTS_CUSTODY_RCPT)
		opts |= BP_CUSTODY_RPT;
	if(bundle_delivery_opts & BP_DOPTS_DELIVERY_RCPT)
		opts |= BP_DELIVERED_RPT;
	if(bundle_delivery_opts & BP_DOPTS_FORWARD_RCPT)
	 	opts |= BP_FORWARDED_RPT;
	if(bundle_delivery_opts & BP_DOPTS_DELETE_RCPT)
		opts |= BP_DELETED_RPT;
	 return opts;
}
al_bp_bundle_delivery_opts_t ion_al_bundle_srrFlags(unsigned char srrFlags){
	al_bp_bundle_delivery_opts_t opts = BP_DOPTS_NONE;
	if(srrFlags & BP_RECEIVED_RPT)
		opts |= BP_DOPTS_RECEIVE_RCPT;
	if(srrFlags & BP_CUSTODY_RPT)
		opts |= BP_DOPTS_CUSTODY_RCPT;
	if(srrFlags & BP_DELIVERED_RPT)
		opts |= BP_DOPTS_DELIVERY_RCPT;
	if(srrFlags & BP_FORWARDED_RPT)
		opts |= BP_DOPTS_FORWARD_RCPT;
	if(srrFlags & BP_DELETED_RPT)
		opts |= BP_DOPTS_DELETE_RCPT;
	return opts;
}

int al_ion_bundle_priority(al_bp_bundle_priority_t bundle_priority){
	switch(bundle_priority.priority)
	{
		case BP_PRIORITY_BULK : 	return BP_BULK_PRIORITY;
		case BP_PRIORITY_NORMAL : 	return BP_STD_PRIORITY;
		case BP_PRIORITY_EXPEDITED :return BP_EXPEDITED_PRIORITY;
		default : 					return -1;
	}

}
al_bp_bundle_priority_t ion_al_bunlde_priority(int bundle_priority){
	al_bp_bundle_priority_t bp_priority;
	bp_priority.ordinal = 0;
	switch(bundle_priority)
	{
		case BP_BULK_PRIORITY: bp_priority.priority = BP_PRIORITY_BULK; break;
		case BP_STD_PRIORITY : bp_priority.priority = BP_PRIORITY_NORMAL; break;
		case BP_EXPEDITED_PRIORITY : bp_priority.priority = BP_PRIORITY_EXPEDITED; break;
		default : bp_priority.priority = -1; break;
	}
	return bp_priority;
}

int al_ion_status_report_flags(al_bp_status_report_flags_t status_repot_flags){
	int ion_statusRpt_flags =0;
	if(status_repot_flags & BP_STATUS_RECEIVED)
		ion_statusRpt_flags |= BP_STATUS_RECEIVE;
	if(status_repot_flags & BP_STATUS_CUSTODY_ACCEPTED)
			ion_statusRpt_flags |= BP_STATUS_ACCEPT;
	if(status_repot_flags & BP_STATUS_FORWARDED)
			ion_statusRpt_flags |= BP_STATUS_FORWARD;
	if(status_repot_flags & BP_STATUS_DELIVERED)
			ion_statusRpt_flags |= BP_STATUS_DELIVER;
	if(status_repot_flags & BP_STATUS_DELETED)
			ion_statusRpt_flags |= BP_STATUS_DELETE;
	if(status_repot_flags & BP_STATUS_ACKED_BY_APP)
			ion_statusRpt_flags |= BP_STATUS_STATS;
	return ion_statusRpt_flags;
}
al_bp_status_report_flags_t ion_al_status_report_flags(int status_repot_flags)
{
	al_bp_status_report_flags_t bp_statusRpt_flags = 0;
	if(status_repot_flags & BP_RECEIVED_RPT)
//	{
		bp_statusRpt_flags |= BP_STATUS_RECEIVED;
	//	printf("\tRECEIVED %d\n",bp_statusRpt_flags);
//	}
	if(status_repot_flags & BP_CUSTODY_RPT)
//	{
		bp_statusRpt_flags |= BP_STATUS_CUSTODY_ACCEPTED;
	//	printf("\tCUSTODY %d\n",bp_statusRpt_flags);
//	}
	if(status_repot_flags & BP_FORWARDED_RPT)
//	{
		bp_statusRpt_flags |= BP_STATUS_FORWARDED;
	//	printf("\tFORWARDED %d\n",bp_statusRpt_flags);
//	}
	if(status_repot_flags & BP_DELIVERED_RPT)
//	{
		bp_statusRpt_flags |= BP_STATUS_DELIVERED;
	//	printf("\tDELIVERED %d\n",bp_statusRpt_flags);
//	}
	if(status_repot_flags & BP_DELETED_RPT)
//	{
		bp_statusRpt_flags |= BP_STATUS_DELETED;
//		printf("\tDELETE  %d\n",bp_statusRpt_flags);
//	}
/*	if(status_repot_flags & BP_STATUS_STATS)
	{
		bp_statusRpt_flags |= BP_STATUS_ACKED_BY_APP;
		printf("\tACKED BY APP\n");
	}
	//printf("\n\tflags al_bp: %d\n",bp_statusRpt_flags);*/
	return bp_statusRpt_flags;
}

BpSrReason al_ion_status_report_reason(al_bp_status_report_reason_t status_report_reason)
{
	return (BpSrReason) status_report_reason;
}
al_bp_status_report_reason_t ion_al_status_report_reason(BpSrReason status_report_reason)
{
	return (al_bp_status_report_reason_t) status_report_reason;
}

BpStatusRpt al_ion_bundle_status_report(al_bp_bundle_status_report_t bundle_status_report)
{
	BpStatusRpt ion_statusRpt;
	memset(&ion_statusRpt,0,sizeof(BpStatusRpt));
	ion_statusRpt.acceptanceTime = al_ion_timeval(bundle_status_report.ack_by_app_ts.secs);
	ion_statusRpt.creationTime = al_ion_timestamp(bundle_status_report.bundle_id.creation_ts);
	ion_statusRpt.deletionTime = al_ion_timeval(bundle_status_report.deletion_ts.secs);
	ion_statusRpt.deliveryTime = al_ion_timeval(bundle_status_report.delivery_ts.secs);
	ion_statusRpt.flags = al_ion_bundle_srrFlags(bundle_status_report.flags);
	ion_statusRpt.forwardTime = al_ion_timeval(bundle_status_report.forwarding_ts.secs);
	if(bundle_status_report.flags & BDL_IS_FRAGMENT	)
		ion_statusRpt.isFragment = 1;
	ion_statusRpt.fragmentOffset = bundle_status_report.bundle_id.frag_offset;
	ion_statusRpt.fragmentLength = bundle_status_report.bundle_id.orig_length;
	ion_statusRpt.reasonCode = al_ion_status_report_reason(bundle_status_report.reason);
	ion_statusRpt.receiptTime = al_ion_timeval(bundle_status_report.receipt_ts.secs);
	ion_statusRpt.sourceEid = al_ion_endpoint_id(bundle_status_report.bundle_id.source);
	return ion_statusRpt;
}
al_bp_bundle_status_report_t ion_al_bundle_status_report(BpStatusRpt bundle_status_report)
{
	al_bp_bundle_status_report_t bp_statusRpt;
	memset(&bp_statusRpt,0,sizeof(al_bp_bundle_status_report_t));
	bp_statusRpt.custody_ts.secs = ion_al_timeval(bundle_status_report.acceptanceTime);
	bp_statusRpt.deletion_ts.secs = ion_al_timeval(bundle_status_report.deletionTime);
	bp_statusRpt.delivery_ts.secs = ion_al_timeval(bundle_status_report.deliveryTime);
	bp_statusRpt.flags = ion_al_status_report_flags(bundle_status_report.flags);
	bp_statusRpt.forwarding_ts.secs = ion_al_timeval(bundle_status_report.forwardTime);
	bp_statusRpt.receipt_ts.secs = ion_al_timeval(bundle_status_report.receiptTime);
	bp_statusRpt.reason = ion_al_status_report_reason(bundle_status_report.reasonCode);
	bp_statusRpt.bundle_id.creation_ts = ion_al_timestamp(bundle_status_report.creationTime);
	bp_statusRpt.bundle_id.frag_offset = (u32_t) bundle_status_report.fragmentOffset;
	bp_statusRpt.bundle_id.orig_length = (u32_t) bundle_status_report.fragmentLength;
	bp_statusRpt.bundle_id.source = ion_al_endpoint_id(bundle_status_report.sourceEid);
	return bp_statusRpt;
}

Payload al_ion_bundle_payload(al_bp_bundle_payload_t bundle_payload, int  priority,BpAncillaryData ancillaryData)
{
	Payload payload;
	memset(&payload,0,sizeof(Payload));
	Sdr bpSdr = bp_get_sdr();
	sdr_begin_xn(bpSdr);
	if(bundle_payload.location == BP_PAYLOAD_MEM)
	{
		Object	buff;
		buff = sdr_malloc(bpSdr, bundle_payload.buf.buf_len);
		sdr_write(bpSdr, buff, bundle_payload.buf.buf_val, bundle_payload.buf.buf_len);
		#ifdef NEW_ZCO
			payload.content = ionCreateZco(ZcoSdrSource, buff, 0, bundle_payload.buf.buf_len, priority,
				ancillaryData.ordinal, ZcoOutbound, NULL);
		#else
			payload.content = zco_create(bpSdr, ZcoSdrSource, buff, 0, bundle_payload.buf.buf_len);
		#endif
		payload.length = zco_length(bpSdr,payload.content);
	}
	else
	{
		u32_t dimFile = 0;
		struct stat st;
		int type = 0;
		memset(&st, 0, sizeof(st));
		stat(bundle_payload.filename.filename_val, &st);
		dimFile = st.st_size;
		Object fileRef = sdr_find(bpSdr, bundle_payload.filename.filename_val, &type);
		if(fileRef == 0)
		{
			#ifdef NEW_ZCO
				fileRef = zco_create_file_ref(bpSdr, bundle_payload.filename.filename_val, "", ZcoOutbound);
			#else
				fileRef = zco_create_file_ref(bpSdr, bundle_payload.filename.filename_val, "");				
			#endif
			sdr_catlg(bpSdr, bundle_payload.filename.filename_val, 0, fileRef);
		}
		#ifdef NEW_ZCO
			payload.content = ionCreateZco(ZcoFileSource, fileRef, 0, (unsigned int) dimFile, priority,
			ancillaryData.ordinal, ZcoOutbound, NULL);
		#else
			payload.content = zco_create(bpSdr, ZcoFileSource, fileRef, 0, (unsigned int) dimFile);
		#endif
		payload.length = zco_length(bpSdr,payload.content);
	}
	sdr_end_xn(bpSdr);
	return payload;
}
al_bp_bundle_payload_t ion_al_bundle_payload(Payload bundle_payload,
								al_bp_bundle_payload_location_t location,char * filename){
	al_bp_bundle_payload_t payload;
	memset(&payload,0,sizeof(al_bp_bundle_payload_t));
	char *buffer = (char*) malloc(sizeof(char) * bundle_payload.length);
	ZcoReader zcoReader;
	memset(&zcoReader,0,sizeof(ZcoReader));
	Sdr sdr = bp_get_sdr();
	zco_start_receiving(bundle_payload.content,&zcoReader);
	sdr_begin_xn(sdr);
	zco_receive_source(sdr,&zcoReader,bundle_payload.length,buffer);
	sdr_end_xn(sdr);
	payload.location = location;
	if(location == BP_PAYLOAD_MEM)
	{
		payload.buf.buf_len = bundle_payload.length;
		payload.buf.buf_val = buffer;
		//payload.buf.buf_val = (char *)malloc(sizeof(char)*bundle_payload.length);
		//memcpy(payload.buf.buf_val, buffer, bundle_payload.length);
	}
	else
	{
		FILE * f = fopen(filename, "w+");
		fwrite(buffer, bundle_payload.length, 1, f);
		fclose(f);
		payload.filename.filename_len = strlen(filename)+1;
		payload.filename.filename_val = (char *)malloc(sizeof(char)*(payload.filename.filename_len));
		strcpy(payload.filename.filename_val,filename);
		free(buffer);
	}

	//free(buffer);
	return payload;
}

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 * The conversion only supports one metadata extesion block to be added. (As of September 2018)
 * The metadata_len signature parameter is for future extension to support multiple metadata
 * extension blocks to be added.
 *
 * In case it ever happens by only making every metadata var an array, the only thing to do
 * is add the index to the extendedCOS.
 */
int al_ion_metadata(u32_t metadata_len, al_bp_extension_block_t *metadata_val, BpAncillaryData* extendedCOS) {

	int i = 0;

	for (i = 0; i < metadata_len; i++)
	{
		u32_t datalen = metadata_val[i].data.data_len;

		/*
		 * check if datalen doesn't exceed ion maximum metadata length
		 */
		if (datalen >= BP_MAX_METADATA_LEN)
		{	return 0;	}

		/*
		 * assign metadata type (check on valid types is in parsing function)...
		 */
		extendedCOS->metadataType = metadata_val[i].type;

		/*
		 * ...and check if a string has been specified by command line.
		 * If yes, assign its value to BPAncillaryData variable.
		 */
		if (datalen != 0)
		{
			//use of this syntax because extendedCOS->metadataLen is unsigned char
			extendedCOS->metadataLen = datalen & 0xFF;

			//use of memcpy because extendedCOS->metadata is array of unsigned char
			memcpy(extendedCOS->metadata, metadata_val[i].data.data_val, datalen + 1);
		}
		else
		{
			/*
			 * otherwise just initialize variables to null values.
			 */
			extendedCOS->metadataLen = 0;
			memset(extendedCOS->metadata, 0, sizeof(extendedCOS->metadata));
		}
	}
	return 1;
}

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 * The conversion supports, in part, the possibility for more than one meb to be specified.
 *
 * If ION ever adds support for more than one metadata extension block, the assigment
 * from the AncillaryData contained in the BpDelivery bundle has to be modified accordingly.
 *
 * If it ever happens by only making each metadata variable an array, simply add index
 * and for loop.
 *
 */
void ion_al_metadata(BpDelivery dlvBundle, al_bp_bundle_spec_t * spec) {
	spec->metadata.metadata_len = 1;

	/*
	 * This re-initialization of metadata_val is to avoid eventual segmentation faluts.
	 */
	if (spec->metadata.metadata_val != NULL)
	{
		free(spec->metadata.metadata_val);
		spec->metadata.metadata_val = NULL;
	}
	spec->metadata.metadata_val = (al_bp_extension_block_t*)malloc(sizeof(al_bp_extension_block_t));

	/*
	 * metadata variables are of type unsigned char, so we must cast them to unsigned integers.
	 */
	spec->metadata.metadata_val[0].type = (unsigned int)dlvBundle.metadataType;
	spec->metadata.metadata_val[0].flags=0;

	spec->metadata.metadata_val[0].data.data_len = (unsigned int) dlvBundle.metadataLen;
	if (dlvBundle.metadataLen > 0)
	{
		spec->metadata.metadata_val[0].data.data_val = malloc(BP_MAX_METADATA_LEN * sizeof(char));
		memcpy(spec->metadata.metadata_val[0].data.data_val, dlvBundle.metadata, dlvBundle.metadataLen + 1);
	}
	else
	{
		spec->metadata.metadata_val[0].data.data_val = NULL;
	}
}

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it
 *
 * This is a to-do function to read extension blocks from ion
 * Problem is, as of October 2018 BpDelivery doesn't have a variable to save extblocks
 *
 */
//void ion_al_extension_block(BpDelivery dlvBundle, al_bp_bundle_spec_t * spec) {
//
//	spec->blocks.blocks_len = sizeof(dlvBundle.);
//	free(spec->metadata.metadata_val);
//	spec->metadata.metadata_val = (al_bp_extension_block_t*)malloc(sizeof(al_bp_extension_block_t));
//
//	spec->metadata.metadata_val[0].type = (unsigned int)dlvBundle.metadataType;
//	spec->metadata.metadata_val[0].flags=0;
//
//	spec->metadata.metadata_val[0].data.data_len = (unsigned int) dlvBundle.metadataLen;
//	if (dlvBundle.metadataLen > 0)
//	{
//		spec->metadata.metadata_val[0].data.data_val = malloc(BP_MAX_METADATA_LEN * sizeof(char));
//		memcpy(spec->metadata.metadata_val[0].data.data_val, dlvBundle.metadata, dlvBundle.metadataLen + 1);
//	}
//	else
//	{
//		spec->metadata.metadata_val[0].data.data_val = NULL;
//	}
//}
#endif /* ION_IMPLEMENTATION */
