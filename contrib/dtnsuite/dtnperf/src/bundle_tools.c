/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

#define _GNU_SOURCE

#include <stdio.h>
#include "bundle_tools.h"
#include "definitions.h"
#include <al_bp_api.h>
#include <arpa/inet.h>
#include "utils.h"
#include "includes.h"

// static variables for stream operations
static char * buffer = NULL;
static u32_t buffer_len = 0;

static pthread_mutex_t mutexsendinfo;
static boolean_t mutex_initilized = FALSE;


/* ----------------------------------------------
 * bundles_needed
 * ---------------------------------------------- */
long bundles_needed (long data, long pl)
{
	long res = 0;
	ldiv_t r;

	r = ldiv(data, pl);
	res = r.quot;
	if (r.rem > 0)
		res += 1;

	return res;
} // end bundles_needed


/* ----------------------------
 * print_eid
 * ---------------------------- */
void print_eid(char * label, al_bp_endpoint_id_t * eid)
{
	printf("%s [%s]\n", label, eid->uri);
} // end print_eid

void destroy_info(send_information_t *send_info)
{
	pthread_mutex_destroy(&mutexsendinfo); // destroy mutex
	mutex_initilized = FALSE;
} // end destroy_info

void init_info(send_information_t *send_info, int window)
{
	int i;

	for (i = 0; i < window; i++)
	{
		send_info[i].bundle_id.creation_ts.secs = 0;
		send_info[i].bundle_id.creation_ts.seqno = 0;
	}

	pthread_mutex_init (&mutexsendinfo, NULL); // initi mutex
	mutex_initilized = TRUE;
} // end init_info

long add_info(send_information_t* send_info, al_bp_bundle_id_t bundle_id, struct timeval p_start, int window)
{
	int i;

	static u_int id = 0;
	static int last_inserted = -1;

	if (!mutex_initilized)
		return -1;

	pthread_mutex_lock(&mutexsendinfo);

	for (i = (last_inserted + 1); i < window ; i++)
	{
		if ((send_info[i].bundle_id.creation_ts.secs == 0) && (send_info[i].bundle_id.creation_ts.seqno == 0))
		{
			send_info[i].bundle_id.creation_ts.secs = bundle_id.creation_ts.secs;
			send_info[i].bundle_id.creation_ts.seqno = bundle_id.creation_ts.seqno;
			send_info[i].send_time.tv_sec = p_start.tv_sec;
			send_info[i].send_time.tv_usec = p_start.tv_usec;
			send_info[i].relative_id = id;
			last_inserted = i;
			pthread_mutex_unlock(&mutexsendinfo);
			return id++;
		}
	}
	for (i = 0; i <= last_inserted ; i++)
	{
		if ((send_info[i].bundle_id.creation_ts.secs == 0) && (send_info[i].bundle_id.creation_ts.seqno == 0))
		{
			send_info[i].bundle_id.creation_ts.secs = bundle_id.creation_ts.secs;
			send_info[i].bundle_id.creation_ts.seqno = bundle_id.creation_ts.seqno;
			send_info[i].send_time.tv_sec = p_start.tv_sec;
			send_info[i].send_time.tv_usec = p_start.tv_usec;
			send_info[i].relative_id = id;
			last_inserted = i;
			pthread_mutex_unlock(&mutexsendinfo);
			return id++;
		}
	}
	pthread_mutex_unlock(&mutexsendinfo);
	return -1;
} // end add_info

int is_in_info(send_information_t* send_info, al_bp_timestamp_t bundle_timestamp, int window)
{
	int i;

	static int last_collected = -1;

	if (!mutex_initilized)
		return -1;

	pthread_mutex_lock(&mutexsendinfo);

	for (i = (last_collected + 1); i < window; i++)
	{
		if ((send_info[i].bundle_id.creation_ts.secs == bundle_timestamp.secs) && (send_info[i].bundle_id.creation_ts.seqno == bundle_timestamp.seqno))
		{
			last_collected = i;
			pthread_mutex_unlock(&mutexsendinfo);
			return i;
		}
	}
	for (i = 0; i <= last_collected; i++)
	{
		if ((send_info[i].bundle_id.creation_ts.secs == bundle_timestamp.secs) && (send_info[i].bundle_id.creation_ts.seqno == bundle_timestamp.seqno))
		{
			last_collected = i;
			pthread_mutex_unlock(&mutexsendinfo);
			return i;
		}

	}
	pthread_mutex_unlock(&mutexsendinfo);

	return -1;
} // end is_in_info

int is_in_info_timestamp(send_information_t* send_info, al_bp_timestamp_t bundle_timestamp, int window)
{
	static const uint32_t TOLERANCE = 1;

	int oldest_i = -1;
	int i;

	if (!mutex_initilized)
		return -1;

	pthread_mutex_lock(&mutexsendinfo);

	for (i = 0; i < window; i++) 
	{
		if (send_info[i].bundle_id.creation_ts.secs >= bundle_timestamp.secs &&
				send_info[i].bundle_id.creation_ts.secs <= bundle_timestamp.secs + TOLERANCE)
		{
			if (oldest_i < 0 || 
					send_info[i].bundle_id.creation_ts.secs < send_info[oldest_i].bundle_id.creation_ts.secs ||
					(send_info[i].bundle_id.creation_ts.secs == send_info[oldest_i].bundle_id.creation_ts.secs &&
							send_info[i].bundle_id.creation_ts.seqno < send_info[oldest_i].bundle_id.creation_ts.seqno))
			{
				oldest_i = i;
			}
		}
	}

	pthread_mutex_unlock(&mutexsendinfo);
	return oldest_i;
}

int count_info(send_information_t* send_info, int window)
{
	int i;
	int count = 0;

	if (!mutex_initilized)
		return 0;

	pthread_mutex_lock(&mutexsendinfo);

	for (i = 0; i < window; i++)
	{
		if (send_info[i].bundle_id.creation_ts.secs != 0)
		{
			count++;
		}
	}

	pthread_mutex_unlock(&mutexsendinfo);
	return count;
}

void remove_from_info(send_information_t* send_info, int position)
{
	if (!mutex_initilized)
		return;
	pthread_mutex_lock(&mutexsendinfo);
	send_info[position].bundle_id.creation_ts.secs = 0;
	send_info[position].bundle_id.creation_ts.seqno = 0;
	pthread_mutex_unlock(&mutexsendinfo);
} // end remove_from_info

void set_bp_options(al_bp_bundle_object_t *bundle, dtnperf_connection_options_t *opt)
{
	al_bp_bundle_delivery_opts_t dopts = BP_DOPTS_NONE;

	// Bundle expiration
	al_bp_bundle_set_expiration(bundle, opt->expiration);

	// Bundle priority
	al_bp_bundle_set_priority(bundle, opt->priority);

	// Bundle unreliable
	al_bp_bundle_set_unreliable(bundle, opt->unreliable);

	// Bundle critical
	al_bp_bundle_set_critical(bundle, opt->critical);

	// Bundle flow label
	al_bp_bundle_set_flow_label(bundle, opt->flow_label);

	// Delivery receipt option
	if (opt->delivery_receipts)
		dopts |= BP_DOPTS_DELIVERY_RCPT;

	// Forward receipt option
	if (opt->forwarding_receipts)
		dopts |= BP_DOPTS_FORWARD_RCPT;

	// Custody transfer
	if (opt->custody_transfer && (opt->critical == FALSE))
		dopts |= BP_DOPTS_CUSTODY;

	// Custody receipts
	if (opt->custody_receipts)
		dopts |= BP_DOPTS_CUSTODY_RCPT;

	// Receive receipt
	if (opt->receive_receipts)
		dopts |= BP_DOPTS_RECEIVE_RCPT;

	// Deleted receipts
	if (opt->deleted_receipts)
		dopts |= BP_DOPTS_DELETE_RCPT;

	//Disable bundle fragmentation
	if (opt->disable_fragmentation)
		dopts |= BP_DOPTS_DO_NOT_FRAGMENT;

	//Set options
	al_bp_bundle_set_delivery_opts(bundle, dopts);

} // end set_bp_options

int open_payload_stream_read(al_bp_bundle_object_t bundle, FILE ** f)
{
	al_bp_bundle_payload_location_t pl_location;
	char * buffer;
	u32_t buffer_len;

	al_bp_bundle_get_payload_location(bundle, &pl_location);

	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_get_payload_mem(bundle, &buffer, &buffer_len);
		*f = fmemopen(buffer, buffer_len, "rb");
		if (*f == NULL)
			return -1;
	}
	else
	{
		al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
		*f = fopen(buffer, "rb");

		if (*f == NULL)
		{
			perror("open");
			return -1;
		}
	}
	return 0;
}

int close_payload_stream_read(FILE * f)
{
	return fclose(f);
}

int open_payload_stream_write(al_bp_bundle_object_t bundle, FILE ** f)
{
	al_bp_bundle_payload_location_t pl_location;

	al_bp_bundle_get_payload_location(bundle, &pl_location);

	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_get_payload_mem(bundle, &buffer, &buffer_len);
		*f= open_memstream(&buffer, (size_t *) &buffer_len);
		if (*f == NULL)
			return -1;
	}
	else
	{
		al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
		*f = fopen(buffer, "wb");
		if (*f == NULL)
			return -1;
	}

	return 0;
}

int close_payload_stream_write(al_bp_bundle_object_t * bundle, FILE *f)
{
	al_bp_bundle_payload_location_t pl_location;
	al_bp_bundle_get_payload_location(*bundle, &pl_location);

	fclose(f);
	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_set_payload_mem(bundle, buffer, buffer_len);
	}
	else
	{
		al_bp_bundle_set_payload_file(bundle, buffer, buffer_len);
	}

	return 0;
}

al_bp_error_t prepare_payload_header_and_ack_options(dtnperf_options_t *opt, FILE * f, uint32_t *crc, int *bytes_written)
{
	if (f == NULL)
		return BP_ENULLPNTR;

	HEADER_TYPE header;
	BUNDLE_OPT_TYPE options;
	uint16_t eid_len;
	uint32_t tmp_crc = 0;

	// header
	switch(opt->op_mode)
	{
	case 'T':
		header = TIME_HEADER;
		break;
	case 'D':
		header = DATA_HEADER;
		break;
	case 'F':
		header = FILE_HEADER;
		break;
	default:
		return BP_EINVAL;
	}
	// options
	options = 0;
	// ack to client
	if (opt->bundle_ack_options.ack_to_client)
		options |= BO_ACK_CLIENT_YES;
	else
		options |= BO_ACK_CLIENT_NO;

	//ack to monitor
	if (opt->bundle_ack_options.ack_to_mon == ATM_NORMAL)
		options |= BO_ACK_MON_NORMAL;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_YES)
		options |= BO_ACK_MON_FORCE_YES;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_NO)
		options |= BO_ACK_MON_FORCE_NO;
	// bundle ack expiration time
	if (opt->bundle_ack_options.set_ack_expiration)
		options |= BO_SET_EXPIRATION;
	// bundle ack priority
	if (opt->bundle_ack_options.set_ack_priority)
	{
		options |= BO_SET_PRIORITY;
		switch (opt->bundle_ack_options.ack_priority.priority)
		{
		case BP_PRIORITY_BULK:
			options |= BO_PRIORITY_BULK;
			break;
		case BP_PRIORITY_NORMAL:
			options |= BO_PRIORITY_NORMAL;
			break;
		case BP_PRIORITY_EXPEDITED:
			options |= BO_PRIORITY_EXPEDITED;
			break;
		case BP_PRIORITY_RESERVED:
			options |= BO_PRIORITY_RESERVED;
			break;
		default:
			break;
		}
	}

	if (opt->crc==TRUE)
		options |= BO_CRC_ENABLED;

	// write in payload
	fwrite(&header, HEADER_SIZE, 1, f);
	*bytes_written+=HEADER_SIZE;
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &header, HEADER_SIZE);
	fwrite(&options, BUNDLE_OPT_SIZE, 1, f);
	*bytes_written+=BUNDLE_OPT_SIZE;
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &options, BUNDLE_OPT_SIZE);
	// write lifetime of ack
	fwrite(&(opt->bundle_ack_options.ack_expiration),sizeof(al_bp_timeval_t), 1, f);
	*bytes_written+=sizeof(al_bp_timeval_t);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &(opt->bundle_ack_options.ack_expiration), sizeof(al_bp_timeval_t));
	// write CRC
	fwrite(&tmp_crc, BUNDLE_CRC_SIZE, 1, f);
	*bytes_written+=BUNDLE_CRC_SIZE;

	// write reply-to eid
	eid_len = strlen(opt->mon_eid);
	fwrite(&eid_len, sizeof(eid_len), 1, f);
	*bytes_written+=sizeof(eid_len);
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) &eid_len, sizeof(eid_len));
	fwrite(opt->mon_eid, eid_len, 1, f);
	*bytes_written+=eid_len;
	if (opt->crc==TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) opt->mon_eid, eid_len);
	return BP_SUCCESS;
}

int get_bundle_header_and_options(al_bp_bundle_object_t * bundle, HEADER_TYPE * header, dtnperf_bundle_ack_options_t * options)
{
	if (bundle == NULL)
		return -1;
	BUNDLE_OPT_TYPE opt;
	al_bp_timeval_t ack_lifetime;
	FILE * pl_stream = NULL;
	open_payload_stream_read(*bundle, &pl_stream);
	uint16_t eid_len;

	if (header != NULL)
	{
		// read header
		if(fread(header, HEADER_SIZE, 1, pl_stream) != 1){ return  BP_EINVAL;}
	}
	else
	{
		// skip header
		fseek(pl_stream, HEADER_SIZE, SEEK_SET);
	}

	if (options != NULL)
	{
		// initiate options
		options->ack_to_client = FALSE;
		options->ack_to_mon = ATM_NORMAL;
		options->set_ack_expiration = FALSE;
		options->set_ack_priority = FALSE;
		options->crc_enabled = FALSE;

		// read options
		if(fread(&opt, BUNDLE_OPT_SIZE, 1, pl_stream) != 1){ return  BP_EINVAL;}

		// ack to client
		if ((opt & BO_ACK_CLIENT_MASK) == BO_ACK_CLIENT_YES)
			options->ack_to_client = TRUE;
		else if ((opt & BO_ACK_CLIENT_MASK) == BO_ACK_CLIENT_NO)
			options->ack_to_client = FALSE;

		// ack to mon
		if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_NORMAL)
			options->ack_to_mon = ATM_NORMAL;
		if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_FORCE_YES)
			options->ack_to_mon = ATM_FORCE_YES;
		else if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_FORCE_NO)
			options->ack_to_mon = ATM_FORCE_NO;

		// expiration
		if (opt & BO_SET_EXPIRATION)
			options->set_ack_expiration = TRUE;

		// priority
		if (opt & BO_SET_PRIORITY)
		{
			options->set_ack_priority = TRUE;
			options->ack_priority.ordinal = 0;
			if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_BULK)
				options->ack_priority.priority = BP_PRIORITY_BULK;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_NORMAL)
				options->ack_priority.priority = BP_PRIORITY_NORMAL;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_EXPEDITED)
				options->ack_priority.priority = BP_PRIORITY_EXPEDITED;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_RESERVED)
				options->ack_priority.priority = BP_PRIORITY_RESERVED;
		}

		// CRC
		if (opt & BO_CRC_ENABLED)
			options->crc_enabled=TRUE;

		// lifetime
		if(fread(&ack_lifetime,sizeof(al_bp_timeval_t), 1, pl_stream) != 1){ return  BP_EINVAL;}
		options->ack_expiration = ack_lifetime;

		// crc
		bundle->payload->buf.buf_crc=0;
		if(fread(&bundle->payload->buf.buf_crc, BUNDLE_CRC_SIZE, 1, pl_stream) != 1){ return  BP_EINVAL;}

		// monitor
		if(fread(&eid_len, sizeof(eid_len), 1, pl_stream) != 1){ return  BP_EINVAL;}
		if(fread(bundle->spec->replyto.uri, eid_len, 1, pl_stream) != 1){ return  BP_EINVAL;}
		bundle->spec->replyto.uri[eid_len] = '\0';


	}
	else
	{
		// skip option
		fseek(pl_stream, BUNDLE_OPT_SIZE, SEEK_SET);
	}
	close_payload_stream_read(pl_stream);

	return 0;
}

u32_t get_header_size(char mode, uint16_t filename_len, uint16_t monitor_eid_len)
{
	u32_t result = 0;
	// Header Type,  congenstion char,  ack lifetime, crc, monitor eid, monitor eid length
	result = HEADER_SIZE + BUNDLE_OPT_SIZE + sizeof(al_bp_timeval_t) + BUNDLE_CRC_SIZE + sizeof(monitor_eid_len) + monitor_eid_len;
	if(mode == 'F')
	{
		// bundle lifetime, filename, filename len, dim file, offset
		result += sizeof(al_bp_timeval_t) + filename_len + sizeof(filename_len) + sizeof(uint32_t) + sizeof(uint32_t);
	}
	return result;
}


al_bp_error_t prepare_generic_payload(dtnperf_options_t *opt, FILE * f, uint32_t *crc, int *bytes_written)
{
	if (f == NULL)
		return BP_ENULLPNTR;

	char * pattern = PL_PATTERN;
	long remaining;
	int i;
	uint16_t monitor_eid_len;
	al_bp_error_t result;

	// RESET CRC
	*crc= 0;
	*bytes_written=0;
	monitor_eid_len = strlen(opt->mon_eid);
	remaining = opt->bundle_payload - get_header_size(opt->op_mode, 0, monitor_eid_len);

	// prepare header and congestion control
	result = prepare_payload_header_and_ack_options(opt, f, crc, bytes_written);

	// fill remainig payload with a pattern
	for (i = remaining; i > strlen(pattern); i -= strlen(pattern))
	{
		fwrite(pattern, strlen(pattern), 1, f);
		*bytes_written+=strlen(pattern);
		if (opt->crc == TRUE)
			*crc = calc_crc32_d8(*crc, (uint8_t*) pattern, strlen(pattern));
	}
	fwrite(pattern, remaining % strlen(pattern), 1, f);
	*bytes_written+=remaining%strlen(pattern);
	if (opt->crc == TRUE)
		*crc = calc_crc32_d8(*crc, (uint8_t*) pattern, remaining % strlen(pattern));

	return result;
}

al_bp_error_t prepare_force_stop_bundle(al_bp_bundle_object_t * start, al_bp_endpoint_id_t monitor,
		al_bp_timeval_t expiration, al_bp_bundle_priority_t priority)
{
	FILE * start_stream;
	HEADER_TYPE start_header = FORCE_STOP_HEADER;
	al_bp_endpoint_id_t none;
	al_bp_bundle_delivery_opts_t opts = BP_DOPTS_NONE;
	al_bp_bundle_set_payload_location(start, BP_PAYLOAD_MEM);
	open_payload_stream_write(*start, &start_stream);
	fwrite(&start_header, HEADER_SIZE, 1, start_stream);
	close_payload_stream_write(start, start_stream);
	al_bp_bundle_set_dest(start, monitor);
	al_bp_get_none_endpoint(&none);
	al_bp_bundle_set_replyto(start, none);
	al_bp_bundle_set_delivery_opts(start, opts);
	al_bp_bundle_set_expiration(start, expiration);
	al_bp_bundle_set_priority(start, priority);
	return BP_SUCCESS;
}

al_bp_error_t prepare_stop_bundle(al_bp_bundle_object_t * stop, al_bp_endpoint_id_t monitor,
		al_bp_timeval_t expiration, al_bp_bundle_priority_t priority, int sent_bundles)
{
	FILE * stop_stream;
	HEADER_TYPE stop_header = STOP_HEADER;
	al_bp_endpoint_id_t none;
	uint32_t buf;
	al_bp_bundle_delivery_opts_t opts = BP_DOPTS_NONE;
	al_bp_bundle_set_payload_location(stop, BP_PAYLOAD_MEM);
	open_payload_stream_write(*stop, &stop_stream);
	fwrite(&stop_header, HEADER_SIZE, 1, stop_stream);
	buf = htonl(sent_bundles);
	fwrite(&buf, sizeof(buf), 1, stop_stream);
	close_payload_stream_write(stop, stop_stream);
	al_bp_bundle_set_dest(stop, monitor);
	al_bp_get_none_endpoint(&none);
	al_bp_bundle_set_replyto(stop, monitor);
	al_bp_bundle_set_delivery_opts(stop, opts);
	al_bp_bundle_set_expiration(stop, expiration);
	al_bp_bundle_set_priority(stop, priority);
	return BP_SUCCESS;
}

al_bp_error_t get_info_from_stop(al_bp_bundle_object_t * stop, int * sent_bundles)
{
	FILE * stop_stream;
	uint32_t buf;
	open_payload_stream_read(*stop, &stop_stream);

	// skip header
	fseek(stop_stream, HEADER_SIZE, SEEK_SET);

	// read sent bundles num
	if(fread(&buf, sizeof(buf), 1, stop_stream) != 1){ return  BP_EINVAL;}

	* sent_bundles = (int) ntohl(buf);

	close_payload_stream_read(stop_stream);
	return BP_SUCCESS;
}
/**
 *
 */
al_bp_error_t prepare_server_ack_payload(dtnperf_server_ack_payload_t ack, dtnperf_bundle_ack_options_t *bundle_ack_options, char **payload, size_t *payload_size)
{
	FILE * buf_stream;
	char * buf;
	size_t buf_size;
	HEADER_TYPE header = DSA_HEADER;
	uint16_t eid_len;
	uint32_t timestamp_secs;
	uint32_t timestamp_seqno;

	// THESE ARE THE LAST 4 BYTES OF THE HEADER THAT CONTAINS ALL THE INFORMATION ABOUT EXTENSIONS
	uint32_t extension_header;
	extension_header = 0;

	// THIS FLAG IS = 1 IF EXTENSION IS USED AND THEN IT HAS TO BE ATTACHED AT THE END 
	// OF THE PAYLOAD
	uint8_t  extension=0;

	buf_stream = open_memstream(&buf, &buf_size);
	fwrite(&header, 1, HEADER_SIZE, buf_stream);
	eid_len = strlen(ack.bundle_source.uri);
	fwrite(&eid_len, sizeof(eid_len), 1, buf_stream);
	fwrite(&(ack.bundle_source.uri), 1, eid_len, buf_stream);
	timestamp_secs = (uint32_t) ack.bundle_creation_ts.secs;
	timestamp_seqno = (uint32_t) ack.bundle_creation_ts.seqno;
	fwrite(&timestamp_secs, 1, sizeof(uint32_t), buf_stream);
	fwrite(&timestamp_seqno, 1, sizeof(uint32_t), buf_stream);
	// CHECK IF THE CRC EXTENSION HAS TO BE ENABLED
	if (bundle_ack_options->crc_enabled == TRUE)
	{
		extension_header |= BO_CRC_ENABLED;
		extension=1;
	}
	// IF OTHER EXTENSIONS NEED TO BE USED, PUT THE 
	// CODE TO ENABLE THEM HERE...
	// [..]

	// CHECK IF ONE (OR MORE) EXTENSION HAS BEEN SET AND THEN WRITE IT
	// AT THE END OF THE PAYLOAD
	if (extension==1)
	{
		fwrite(&extension_header, sizeof(uint32_t), 1, buf_stream);
	}
	fclose(buf_stream);
	*payload = (char*)malloc(buf_size);
	memcpy(*payload, buf, buf_size);
	*payload_size = buf_size;
	free(buf);
	return BP_SUCCESS;
}

al_bp_error_t get_info_from_ack(al_bp_bundle_object_t * ack, al_bp_endpoint_id_t * reported_eid, al_bp_timestamp_t * reported_timestamp, uint32_t *extension_ack)
{
	al_bp_error_t error;
	HEADER_TYPE header;
	FILE * pl_stream;
	uint16_t eid_len;
	uint32_t timestamp_secs, timestamp_seqno;

	open_payload_stream_read(*ack, &pl_stream);
	if(fread(&header, HEADER_SIZE, 1, pl_stream) != 1){ return  BP_EINVAL;}
	if (header == DSA_HEADER)
	{
		if(fread(&eid_len, sizeof(eid_len), 1, pl_stream) != 1){ return  BP_EINVAL;}
		if (reported_eid != NULL)
		{
			if(fread(reported_eid->uri, eid_len, 1, pl_stream) != 1){ return  BP_EINVAL;}
			reported_eid->uri[eid_len] = '\0';

		}
		else
			fseek(pl_stream, eid_len, SEEK_CUR);

		if (reported_timestamp != NULL)
		{
			if(fread(&timestamp_secs, sizeof(uint32_t), 1, pl_stream) != 1){ return  BP_EINVAL;}
			if(fread(&timestamp_seqno, sizeof(uint32_t), 1, pl_stream) != 1){ return  BP_EINVAL;}
			reported_timestamp->secs = (u32_t) timestamp_secs;
			reported_timestamp->seqno = (u32_t) timestamp_seqno;
		}

		if (feof(pl_stream)==0)
		{
			// READ THE LAST 4 BYTES OF THE HEADER THAT CONTAIN ALL THE INFORMATION
			// ABOUT ETENSION
			uint32_t extension_bytes = fread(extension_ack, sizeof(uint32_t), 1, pl_stream);
			if(extension_bytes != 1)
			{
				*extension_ack = 0;
			}
		}
		else
			*extension_ack = 0;

		error = BP_SUCCESS;
	}
	else
		error = BP_ERRBASE;

	close_payload_stream_read(pl_stream);
	return error;
}

boolean_t check_metadata(extension_block_info_t* ext_block)
{
	return ext_block->metadata;
} // end check_metadata

void set_metadata_type(extension_block_info_t* ext_block, u_int64_t metadata_type)
{
	if (metadata_type > METADATA_TYPE_EXPT_MAX) {
		fprintf(stderr, "Value of metadata_type greater than maximum allowed\n");
		exit(1);
	}
	ext_block->metadata = TRUE;
	ext_block->metadata_type = metadata_type;
} // end set_metadata

void get_extension_block(extension_block_info_t* ext_block, al_bp_extension_block_t * extension_block)
{
	*extension_block = ext_block->block;
} // end get_extension_block

void set_block_buf(extension_block_info_t* ext_block, char * buf, u32_t len)
{
	if (ext_block->block.data.data_val != NULL) {
		free(ext_block->block.data.data_val);
		ext_block->block.data.data_val = NULL;
		ext_block->block.data.data_len = 0;
	}
	if (ext_block->metadata) {
		ext_block->block.data.data_val =
				(char *)malloc(sizeof(char) * (len + 1));
		ext_block->block.data.data_len = len;
		strcpy(ext_block->block.data.data_val, buf);
		free(buf);
	}
	else {
		ext_block->block.data.data_val = buf;
		ext_block->block.data.data_len = len;
	}
} // end set_block_buf

u32_t get_current_dtn_time()
{
	u32_t result;
	time_t dtn_epoch = (time_t) DTN_EPOCH;
	time_t current = time(NULL);
	result = (u32_t) difftime(current, dtn_epoch);
	return result;
}

int bundle_id_sprintf(char * dest, al_bp_bundle_id_t * bundle_id)
{
	char offset[16], length[16];
	if (bundle_id->frag_offset != 0)
	{
		sprintf(offset, "%d", bundle_id->frag_offset);
	}
	else
	{
		offset[0] = '\0';
	}
	if (bundle_id->orig_length != 0)
	{
		sprintf(length, "%d", bundle_id->orig_length);
	}
	else
	{
		length[0] = '\0';
	}
	return sprintf(dest, "%s, %u.%u,%s,%s", bundle_id->source.uri, bundle_id->creation_ts.secs,
			bundle_id->creation_ts.seqno, offset, length);
}

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it*/
void print_bundle(al_bp_bundle_object_t * bundle)
{

	debug_print(DEBUG_L2,"[DTNperf L2] printing BUNDLE...\n");
	debug_print(DEBUG_L2,"[DTNperf L2] id-creation timestamp: %u\tsequence number: %u\n", bundle->id->creation_ts.secs, bundle->id->creation_ts.seqno);
	debug_print(DEBUG_L2,"[DTNperf L2] id-source: %s\n", bundle->id->source.uri);
	debug_print(DEBUG_L2,"[DTNperf L2] id-original length: %u\tfragmentation offset: %u\n", bundle->id->orig_length, bundle->id->frag_offset);

	debug_print(DEBUG_L2,"[DTNperf L2] spec-creation timestamp: %u\tsequence number: %u\n\t\texpiration time: %u\n", bundle->spec->creation_ts.secs, bundle->spec->creation_ts.seqno, bundle->spec->expiration);
	debug_print(DEBUG_L2,"[DTNperf L2] spec-delivery register id: %u\tdelivery options: %d\n", bundle->spec->delivery_regid, bundle->spec->dopts);
	debug_print(DEBUG_L2,"[DTNperf L2] spec-source: %s\tdest: %s\treplyto: %s\n", bundle->spec->source.uri, bundle->spec->dest.uri, bundle->spec->replyto.uri);
	debug_print(DEBUG_L2,"[DTNperf L2] spec-is critical: %c\tpriority: %d,%u\tis unreliable: %c\n", bundle->spec->critical, bundle->spec->priority.priority, bundle->spec->priority.ordinal, bundle->spec->unreliable);
	debug_print(DEBUG_L2,"[DTNperf L2] spec-flow label: %u\n", bundle->spec->flow_label);
	if(bundle->spec->metadata.metadata_len > 0)
	{
		debug_print(DEBUG_L1,"[DTNperf L1] printing METADATA blocks...\n");
		print_ext_or_metadata_blocks(bundle->spec->metadata.metadata_len, bundle->spec->metadata.metadata_val);
		debug_print(DEBUG_L1,"[DTNperf L1] done printing METADATA blocks.\n");
	}
	else
	{
		debug_print(DEBUG_L1,"[DTNperf L1] no METADATA blocks specified.\n");

	}
	if(bundle->spec->blocks.blocks_len > 0)
	{
		debug_print(DEBUG_L1,"[DTNperf L1] printing EXTENSION blocks...\n");
		print_ext_or_metadata_blocks(bundle->spec->blocks.blocks_len, bundle->spec->blocks.blocks_val);
		debug_print(DEBUG_L1,"[DTNperf L1] done printing EXTENSION blocks.\n");
	}
	else
	{
		debug_print(DEBUG_L1,"[DTNperf L1] no EXTENSION blocks specified.\n");

	}
	debug_print(DEBUG_L2,"[DTNperf L2] done.\n");


//	debug_print(DEBUG_L2,"[DTNperf L2] payload-location: %d\n", bundle->payload->location);
//	debug_print(DEBUG_L2,"[DTNperf L2] id-creation timestamp: %u\tsequence number: %u\n", bundle->payload->status_report->bundle_id., bundle->id->creation_ts.seqno);
//	debug_print(DEBUG_L2,"[DTNperf L2] id-creation timestamp: %u\tsequence number: %u\n", bundle->id->creation_ts.secs, bundle->id->creation_ts.seqno);
//	debug_print(DEBUG_L2,"[DTNperf L2] id-creation timestamp: %u\tsequence number: %u\n", bundle->id->creation_ts.secs, bundle->id->creation_ts.seqno);
}

/* Author: Laura Mazzuca, laura.mazzuca@studio.unibo.it*/
void print_ext_or_metadata_blocks(u32_t blocks_len, al_bp_extension_block_t *blocks_val)
{
	int i=0;
	debug_print(DEBUG_L1,"[DTNperf L1] number of blocks: %lu\n", blocks_len);
	for (i = 0; i < blocks_len; i++)
	{
		debug_print(DEBUG_L1,"[DTNperf L1] Block[%d]\n", i);
		debug_print(DEBUG_L1,"[DTNperf L1]---type: %lu\n", blocks_val[i].type);
		debug_print(DEBUG_L1,"[DTNperf L1]---flags: %lu\n", blocks_val[i].flags);
		debug_print(DEBUG_L1,"[DTNperf L1]---data_len: %lu\n", blocks_val[i].data.data_len);
		debug_print(DEBUG_L1,"[DTNperf L1]---data_val: %s\n", blocks_val[i].data.data_val);
	}
}
