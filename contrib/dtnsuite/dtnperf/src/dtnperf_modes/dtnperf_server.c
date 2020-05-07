/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Marco Bertolazzi, marco.bertolazzi3@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * dtnperf_server.c
 */

#include "../dtnperf_debugger.h"

#include "dtnperf_server.h"
#include "../includes.h"
#include "../definitions.h"
#include "../bundle_tools.h"
#include "../file_transfer_tools.h"
#include "../utils.h"

#include <al_bp_api.h>
#include <al_bp_extB.h>

#ifdef mingw
#include <windows.h>
BOOL WINAPI ConsoleHandler(DWORD);
#endif

/*
 * Global variables
 */

// pthread variables
static pthread_t file_exp_timer;
static pthread_mutex_t mutexdata;

static file_transfer_info_list_t file_transfer_info_list;

// flags to exit cleanly

static al_bp_extB_registration_descriptor registration_descriptor;

static pthread_mutex_t mutex_forced_close;
static boolean_t forced_close = FALSE;
static boolean_t get_forced_close_server();
static void set_forced_close_server(boolean_t value);

/*  ----------------------------
 *          SERVER CODE
 *  ---------------------------- */
void run_dtnperf_server(dtnperf_global_options_t * perf_g_opt)
{
	/* ------------------------
	 * variables
	 * ------------------------ */

	dtnperf_options_t * perf_opt = perf_g_opt->perf_opt;
	dtnperf_connection_options_t * conn_opt = perf_g_opt->conn_opt;

	//al_bp_reg_info_t reginfo;
	al_bp_bundle_payload_location_t pl_location;
	al_bp_endpoint_id_t bundle_source_addr;
	al_bp_endpoint_id_t bundle_dest_addr;
	al_bp_endpoint_id_t bundle_replyto_addr;
	al_bp_endpoint_id_t local_eid;
	al_bp_error_t error;
	al_bp_extB_error_t utility_error;
	al_bp_bundle_object_t bundle_object;
	al_bp_bundle_object_t bundle_ack_object;
	al_bp_bundle_delivery_opts_t bundle_ack_dopts;
	al_bp_timestamp_t bundle_creation_timestamp;
	al_bp_timeval_t bundle_expiration;
	al_bp_bundle_priority_t bundle_priority;
	u32_t bundle_payload_len;
	dtnperf_server_ack_payload_t server_ack_payload;
	HEADER_TYPE bundle_header;
	dtnperf_bundle_ack_options_t bundle_ack_options;
	time_t current;
	char* pl_filename = NULL;
	size_t pl_filename_len = 0;
	char* pl_buffer = NULL;
	size_t pl_buffer_size = 0;
	boolean_t is_file_transfer_bundle;
	int indicator; // for file transfer functions purposes
	int num_ack; // for name file bundle ack

	boolean_t receive_ok;

	/* ------------------------
	 * initialize variables
	 * ------------------------ */


	perf_opt->dest_dir = correct_dirname(perf_opt->dest_dir);
	perf_opt->file_dir = correct_dirname(perf_opt->file_dir);

	num_ack=0;

	debugger_init(perf_opt->debug, perf_opt->create_log, perf_opt->log_filename);

	// initialize structures for file transfers
	file_transfer_info_list = file_transfer_info_list_create();

	// set out buffer size if daemon
	/*if (perf_opt->daemon)
	{
		setlinebuf(stdout);
		setlinebuf(stderr);
	}*/

	// show requested options (debug) REMOVED!
	/*if (debug_check_level(DEBUG_L1))
	{
		debug_print(DEBUG_L1,"\nOptions;\n");
		debug_print(DEBUG_L1, "\tendpoint:\t\t%s\n", perf_opt->bp_implementation == BP_ION ? SERV_EP_NUM_SERVICE : SERV_EP_STRING);
		debug_print(DEBUG_L1, "\tsave bundles to:\t%s\n", perf_opt->use_file ? "file":"memory");
		if(perf_opt->use_file)
			debug_print(DEBUG_L1, "\tbundles local dir:\t%s\n", perf_opt->dest_dir);
		printf("\tsend acks:\t%s\n", perf_opt->no_acks ? "no":"yes");
		if (!perf_opt->no_acks)
		{
			//printf("\tsend acks to monitor: %s\n", perf_opt->acks_to_mon ? "yes":"no");
			debug_print(DEBUG_L1,"\tacks expiration time: %d\n", (int) conn_opt->expiration);
			char * prior;
			switch(conn_opt->priority.priority)
			{
			case BP_PRIORITY_BULK:
				prior = "bulk";
				break;
			case BP_PRIORITY_NORMAL:
				prior = "normal";
				break;
			case BP_PRIORITY_EXPEDITED:
				prior = "expedited";
				break;
			case BP_PRIORITY_RESERVED:
				prior = "reserved";
				break;
			default:
				prior = "unknown";
				break;
			}
			debug_print(DEBUG_L1,"\tacks priority       : %s\n", prior);
		}
		debug_print(DEBUG_L1,"\n");
	}*/

	//Ctrl+C handler
#ifdef mingw
	signal(SIGINT,SIG_IGN);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE)) {
		fprintf(stderr, "Unable to install handler!\n");
		server_clean_exit(1);
	}
#else
	signal(SIGINT, server_handler);
#endif

	// create dir where dtnperf server will save incoming bundles
	if (mkpath(perf_opt->dest_dir) < 0)
	{
		perror("[DTNperf error] in opening bundle destination dir");
		al_bp_extB_destroy();
		debugger_destroy();
		exit(-1);
	}

	// create dir where dtnperf server will save incoming files
	if (mkpath(perf_opt->file_dir) < 0)
	{
		perror("[DTNperf error] in opening transferred files destination dir");
		debugger_destroy();
		exit(-1);
	}

	/*debug_print(DEBUG_L2, "[DEBUG_L2] building a local eid in format ");
	if (perf_opt->eid_format_forced == 'D')
		debug_print(DEBUG_L2, "forced DTN...");
	else if (perf_opt->eid_format_forced == 'I')
		debug_print(DEBUG_L2, "forced IPN...");
	else
		debug_print(DEBUG_L2, "standard...");*/
	utility_error = al_bp_extB_init(perf_opt->eid_format_forced, perf_opt->ipn_local_num);
	//debug_print(DEBUG_L2,"done\n");
	switch(utility_error)
	{
	case BP_EXTB_SUCCESS:
		break;
	default:
		error_print("[DTNperf fatal error] in initializing al_bp_extB\n");
		exit(1);
		break;
	}

	//open the connection to the bundle protocol router
	
	// XXX no more supported
	/*if (perf_opt->use_ip)
		utility_error = al_bp_extB_register(&registration_descriptor, perf_opt->ip_addr, perf_opt->ip_port, perf_opt->ipn_local_num, atoi(SERV_EP_NUM_SERVICE), SERV_EP_STRING, perf_opt->bp_implementation);
	else
		utility_error = al_bp_extB_register(&registration_descriptor, NULL, 0, perf_opt->ipn_local_num, atoi(SERV_EP_NUM_SERVICE), SERV_EP_STRING, perf_opt->bp_implementation);*/
	
	utility_error = al_bp_extB_register(&registration_descriptor, SERV_EP_STRING, atoi(SERV_EP_NUM_SERVICE));

	switch(utility_error)
	{
	case BP_EXTB_ERROPEN:
		fflush(stdout);
		error_print("[DTNperf fatal error] in opening bp handle: %s\n", al_bp_strerror(al_bp_extB_get_error(registration_descriptor)));
		debugger_destroy();
		exit(1);
		break;
	case BP_EXTB_ERRLOCALEID:
		fflush(stdout);
		error_print("[DTNperf fatal error] in building local EID: '%s'\n", al_bp_strerror(al_bp_extB_get_error(registration_descriptor)));
		server_clean_exit(1);
		break;
	case BP_EXTB_ERRREGISTER:
		fflush(stdout);
		error_print("[DTNperf error] in creating registration: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_strerror(al_bp_extB_errno(registration_descriptor)));
		server_clean_exit(1);
		break;

	default:
		break;
	}

	{
		al_bp_endpoint_id_t temp = al_bp_extB_get_local_eid(registration_descriptor);
		debug_print(DEBUG_OFF,"local_eid = %s\n", temp.uri);
	}
	debug_print(DEBUG_L1,"regid 0x%x\n", (unsigned int) al_bp_extB_get_regid(registration_descriptor));

	// set bundle destination type
	debug_print(DEBUG_L2,"[DEBUG_L2] choosing bundle destination type...");
	if (perf_opt->use_file)
		pl_location = BP_PAYLOAD_FILE;
	else
		pl_location = BP_PAYLOAD_MEM;
	debug_print(DEBUG_L2," done. Bundles will be saved into [file/memory]: %s\n", perf_opt->use_file ? "file" : "memory");

	// start thread
	pthread_mutex_init (&mutexdata, NULL);
	pthread_create(&file_exp_timer, NULL, file_expiration_timer, NULL);

	debug_print(DEBUG_L2,"[DEBUG_L2] entering infinite loop...\n");

	// start infinite loop
	while(1)
	{

		// create a bundle object
		debug_print(DEBUG_L2,"[DEBUG_L2] initiating memory for the next bundle... ");

		error = al_bp_bundle_create(&bundle_object);
		if (error != BP_SUCCESS)
		{
			fflush(stdout);
			error_print("[DTNperf fatal error] in initiating memory for bundles: %s\n", al_bp_strerror(error));
			server_clean_exit(1);
		}
		debug_print(DEBUG_L2,"done\n");

		// reset file transfer indicators
		is_file_transfer_bundle = FALSE;

		// wait until receive a bundle
		debug_print(DEBUG_L2,"[DEBUG_L2] waiting for the next bundle...\n");

		utility_error = al_bp_extB_receive(registration_descriptor, &bundle_object, pl_location, -1);
		receive_ok = FALSE;
		switch (utility_error)
		{
		case BP_EXTB_SUCCESS:
			receive_ok = TRUE;
			break;
		case BP_EXTB_ERRRECEPINTER:
			error_print("[DTNperf warning] bundle reception interrupted\n");
			al_bp_bundle_free(&bundle_object);
			break;
		case BP_EXTB_ERRTIMEOUT:
			error_print("[DTNperf warning] bundle reception timeout expired\n");
			al_bp_bundle_free(&bundle_object);
			break;
		default:
			fflush(stdout);
			error_print("[DTNperf warning] in getting recv reply: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_strerror(al_bp_extB_errno(registration_descriptor)));
			server_clean_exit(1);
			break;
		}

		if (receive_ok)
		{
			debug_print(DEBUG_L2," bundle received\n");

			// find payload size
			debug_print(DEBUG_L2,"[DEBUG_L2] calculating bundle payload size... ");
			error = al_bp_bundle_get_payload_size(bundle_object, (u32_t *) &bundle_payload_len);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				error_print("[DTNperf fatal error] in getting bundle payload size: %s\n", al_bp_strerror(error));
				server_clean_exit(1);
			}
			debug_print(DEBUG_L2,"done\n");

			// mark current time
			debug_print(DEBUG_L2,"[DEBUG_L2] marking time...");
			current = time(NULL);
			debug_print(DEBUG_L2," done\n");

			// print bundle arrival
			printf("%s : %u bytes from %s\n", ctime(&current),	bundle_payload_len,	bundle_object.spec->source.uri);

			// get bundle header and options
			debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting bundle header and options...");
			if (get_bundle_header_and_options(&bundle_object, &bundle_header, &bundle_ack_options) < 0)
			{
				printf("[DTNperf warning] in getting bundle header and options\n");
				continue;
			}

			debug_print(DEBUG_L2," done.\n");

			// get SOURCE eid
			debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting source eid...");
			error = al_bp_bundle_get_source(bundle_object, &bundle_source_addr);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				error_print("[DTNperf fatal error] in getting bundle source eid: %s\n", al_bp_strerror(error));
				server_clean_exit(1);
			}

			debug_print(DEBUG_L2," done:\n");
			debug_print(DEBUG_L2,"\tbundle_source_addr = %s\n", bundle_source_addr.uri);
			debug_print(DEBUG_L2,"\n");


			// get DEST eid
			debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting destination eid...");
			error = al_bp_bundle_get_dest(bundle_object, &bundle_dest_addr);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				error_print("[DTNperf fatal error] in getting bundle destination eid: %s\n", al_bp_strerror(error));
				server_clean_exit(1);
			}

			debug_print(DEBUG_L2," done:\n");
			debug_print(DEBUG_L2,"\tbundle_dest_eid = %s\n", bundle_dest_addr.uri);
			debug_print(DEBUG_L2,"\n");


			// get REPLY TO eid
			debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting reply to eid...");
			error = al_bp_bundle_get_replyto(bundle_object, &bundle_replyto_addr);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				error_print("error getting bundle reply to eid: %s\n",
						al_bp_strerror(error));
				debugger_destroy();
				exit(1);
			}

			debug_print(DEBUG_L2," done:\n");
			debug_print(DEBUG_L2,"\tbundle_replyto_eid = %s\n", bundle_replyto_addr.uri);
			debug_print(DEBUG_L2,"\n");

			// get bundle creation timestamp
			debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting bundle creation timestamp...");
			error = al_bp_bundle_get_creation_timestamp(bundle_object, &bundle_creation_timestamp);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				error_print("[DTNperf fatal error] in getting bundle creation timestamp: %s\n", al_bp_strerror(error));
				server_clean_exit(1);
			}

			debug_print(DEBUG_L2," done:\n");
			debug_print(DEBUG_L2,"\tbundle creation timestamp:\n"
					"\tsecs = %d\n\tseqno= %d\n",
					(int)bundle_creation_timestamp.secs, (int)bundle_creation_timestamp.seqno);
			debug_print(DEBUG_L2,"\n");
			//***********************************************************************//

			if(bundle_object.spec->metadata.metadata_len > 0)
			{
				debug_print(DEBUG_L2,"[DTNperf L2] printing METADATA blocks...\n");
				print_ext_or_metadata_blocks(bundle_object.spec->metadata.metadata_len, bundle_object.spec->metadata.metadata_val);
				debug_print(DEBUG_L2,"[DTNperf L2] done printing METADATA blocks.\n");
			}
			else
			{
				debug_print(DEBUG_L2,"[DTNperf L2] no METADATA blocks specified.\n");

			}
			//***********************************************************************//

			// get bundle payload filename
			if(perf_opt->use_file)
			{
				debug_print(DEBUG_L2,"[DEBUG_L2]\tgetting bundle payload filename...");
				error = al_bp_bundle_get_payload_file(bundle_object, &pl_filename, (u32_t *) &pl_filename_len);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in getting bundle payload filename: %s\n",
							al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2," done:\n");
			}

			//debug_print(DEBUG_L1,"======================================\n");
			//debug_print(DEBUG_L1," Bundle received at %s\n", ctime(&current));
			//debug_print(DEBUG_L1,"  source: %s\n", bundle_source_addr.uri);
			if (perf_opt->use_file)
			{
				debug_print(DEBUG_L1,"[DEBUG_L1] saved into: %s\n", pl_filename);
			}
			//debug_print(DEBUG_L1,"--------------------------------------\n");

			// check if is file transfer bundle
			debug_print(DEBUG_L2,"[DEBUG_L2]\tchecking if this is a file transfer bundle...");
			if (bundle_header == FILE_HEADER)
			{
				is_file_transfer_bundle = TRUE;
			}

			debug_print(DEBUG_L2," done.\n");
			debug_print(DEBUG_L2,"\tbundle is%sa file transfer bundle\n",
					is_file_transfer_bundle ? " " : " not ");
			debug_print(DEBUG_L2,"\n");

			int crc_ok = 1;
			if (bundle_ack_options.crc_enabled==TRUE)
			{

				FILE *pl_stream;
				char *transfer;
				int transfer_len;
				u32_t pl_size;
				uint32_t local_crc;

				local_crc = 0;
				crc_ok = 0;

				// get info about bundle size
				al_bp_bundle_get_payload_size(bundle_object, &pl_size);

				if (open_payload_stream_read(bundle_object, &pl_stream) < 0)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] i can't open the bundle file\n");
					server_clean_exit(1);
				}
				/*
				fseek(pl_stream, HEADER_SIZE + BUNDLE_OPT_SIZE, SEEK_SET);
				if (is_file_transfer_bundle)
				{
					fseek
				}*/

				transfer_len = HEADER_SIZE + BUNDLE_OPT_SIZE+sizeof(al_bp_timeval_t);
				transfer = (char*) malloc(transfer_len);
				memset(transfer, 0, transfer_len);
				if (fread(transfer, transfer_len, 1, pl_stream) != 1 && ferror(pl_stream)!=0)
				{
					error_print("[DTNperf warning] in processing file transfer bundle: %s\n", strerror(errno));
				}
				local_crc = calc_crc32_d8(local_crc, (uint8_t*) transfer, transfer_len);
				free(transfer);

				fseek(pl_stream, BUNDLE_CRC_SIZE, SEEK_CUR);

				transfer_len = pl_size-transfer_len-BUNDLE_CRC_SIZE;
				transfer = (char*) malloc(transfer_len);
				memset(transfer, 0, transfer_len);
				if (fread(transfer, transfer_len, 1, pl_stream) != 1 && ferror(pl_stream)!=0)
				{
					error_print("[DTNperf warning] in processing file transfer bundle: %s\n", strerror(errno));
				}

				local_crc = calc_crc32_d8(local_crc, (uint8_t*) transfer, transfer_len);

				debug_print(DEBUG_L1,"CRC received: %"PRIu32" GENERATED: %"PRIu32"\n", bundle_object.payload->buf.buf_crc, local_crc);

				if (local_crc != bundle_object.payload->buf.buf_crc)
					bundle_ack_options.crc_enabled=TRUE;
				else
					bundle_ack_options.crc_enabled=FALSE;

				if (bundle_ack_options.crc_enabled==FALSE)
				{
					crc_ok = 1;
				}

				free(transfer);
				close_payload_stream_read(pl_stream);
			}

			// process file transfer bundle
			if(is_file_transfer_bundle && crc_ok == 1)
			{
				debug_print(DEBUG_L2,"[DEBUG_L2]\tprocessing file transfer bundle...");

				pthread_mutex_lock(&mutexdata);
				indicator = process_incoming_file_transfer_bundle(&file_transfer_info_list,
						&bundle_object,perf_opt->file_dir, (bundle_ack_options.crc_enabled == TRUE ? &bundle_object.payload->buf.buf_crc : (uint32_t *) NULL));

				pthread_mutex_unlock(&mutexdata);
				sched_yield();

				// WRONG CRC
				//if (indicator == -2)
				//{
				//	if (debug)
				//		printf("CRC differs from the received one.\n");
				//	bundle_ack_options.crc_enabled=TRUE;
				//}
				//else
				//	bundle_ack_options.crc_enabled=FALSE;

				if (indicator < 0) // error in processing bundle
				{
					error_print("[DTNperf warning] in processing file transfer bundle: %s\n", strerror(errno));
				}
				debug_print(DEBUG_L2,"done.\n");
				if (indicator == 1)
					debug_print(DEBUG_L2,"Transfer Completed\n");
			}

			// get bundle expiration time
			if (bundle_ack_options.set_ack_expiration)
			{
				// 	al_bp_bundle_get_expiration(bundle_object, &bundle_expiration);
				//  is setted the smaller
				if( conn_opt->expiration < bundle_ack_options.ack_expiration)
					bundle_expiration = conn_opt->expiration;
				else
					bundle_expiration = bundle_ack_options.ack_expiration;
			}
			else
				bundle_expiration = conn_opt->expiration;

			// get bundle priority
			bundle_priority.ordinal = 0;
			if( bundle_ack_options.set_ack_priority)
			{
				// is setted the smaller
				if( conn_opt->priority.priority < bundle_ack_options.ack_priority.priority)
					bundle_priority.priority = conn_opt->priority.priority;
				else
					bundle_priority.priority = bundle_ack_options.ack_priority.priority;
			}
			else
				bundle_priority.priority = conn_opt->priority.priority;

			// send acks to the client only if requested by client
			// send acks to the monitor if:
			// ack requested by client AND ack-to-monitor option set AND bundle_ack_options.ack_to_mon == ATM_NORMAL
			// OR client forced server to send ack to monitor

			boolean_t send_ack_to_client = bundle_ack_options.ack_to_client;
			boolean_t send_ack_to_monitor = FALSE;
			send_ack_to_monitor = (bundle_ack_options.ack_to_client && (bundle_ack_options.ack_to_mon == ATM_NORMAL) && bundle_ack_options.ack_to_client && perf_opt->acks_to_mon)
																																	|| (bundle_ack_options.ack_to_mon == ATM_FORCE_YES);
			if (send_ack_to_client || send_ack_to_monitor)
			{
				// create bundle ack to send
				debug_print(DEBUG_L2,"[DEBUG_L2] initiating memory for bundle ack...");
				error = al_bp_bundle_create(&bundle_ack_object);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in initiating memory for bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");


				// initiate server ack payload
				// set server ack payload source
				server_ack_payload.bundle_source = bundle_source_addr;
				// set server ack payload timestamp
				server_ack_payload.bundle_creation_ts = bundle_creation_timestamp;
				// preparing the bundle ack payload
				debug_print(DEBUG_L2,"[DEBUG_L2] preparing the payload of the bundle ack...");
				error = prepare_server_ack_payload(server_ack_payload, &bundle_ack_options, &pl_buffer, &pl_buffer_size);

				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in preparing the payload of the bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");

				// setting the bundle ack payload
				debug_print(DEBUG_L2,"[DEBUG_L2] setting the payload of the bundle ack...");
				// For DTN2 implementation ack payload in in memory
				if (perf_opt->bp_implementation == BP_DTN)
				{
					error = al_bp_bundle_set_payload_mem(&bundle_ack_object, pl_buffer, pl_buffer_size);
				}
				else if (perf_opt->bp_implementation == BP_ION)
				{
					char filename_ack[256];
					FILE * fd_ack;
					u32_t filename_ack_len;
					sprintf(filename_ack,"%s_%d",SOURCE_FILE_ACK,num_ack);
					filename_ack_len = strlen(filename_ack)+1;
					fd_ack = fopen(filename_ack,"w");
					if(fd_ack == NULL)
					{
						fflush(stdout);
						error_print("[DTNperf fatal error] in creating the payload of the bundle ack: %s\n", strerror(errno));
						server_clean_exit(1);
					}
					if(fwrite(pl_buffer, pl_buffer_size, 1, fd_ack)<0){
						fflush(stdout);
						error_print("[DTNperf fatal error] in writing the payload of the bundle ack: %s\n", strerror(errno));
						//server_clean_exit(1);
					}
					if (fclose(fd_ack) != 0){
						fflush(stdout);
						error_print("[DTNperf fatal error] in closing the payload file of the bundle ack: %s\n", strerror(errno));
						server_clean_exit(1);
					}

					debug_print(DEBUG_L2,"\n[DEBUG_L2] bundle payload ack saved in: %s ... ", filename_ack);

					num_ack++;
					if (num_ack == 10000) //reset ack counter when it arrives at 10000
						num_ack = 0;
					error = al_bp_bundle_set_payload_file(&bundle_ack_object,filename_ack,filename_ack_len);
				} 
				else if (perf_opt->bp_implementation == BP_IBR) 
				{	
					error = al_bp_bundle_set_payload_mem(&bundle_ack_object, pl_buffer, pl_buffer_size);
				}
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("\n[DTNperf fatal error] in setting the payload of the bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");
				// setting the bundle ack options
				local_eid = al_bp_extB_get_local_eid(registration_descriptor);
				debug_print(DEBUG_L2,"[DEBUG_L2] setting source of the bundle ack: %s ...", local_eid.uri);
				debug_print(DEBUG_L2,"done\n");

				debug_print(DEBUG_L2,"[DEBUG_L2] setting destination of the bundle ack: %s ...", bundle_source_addr.uri);
				debug_print(DEBUG_L2,"done\n");

				debug_print(DEBUG_L2,"[DEBUG_L2] setting replyto eid of the bundle ack: %s ...", bundle_replyto_addr.uri);
				debug_print(DEBUG_L2,"done\n");

				debug_print(DEBUG_L2,"[DEBUG_L2] setting priority of the bundle ack...");

				error = al_bp_bundle_set_priority(& bundle_ack_object, bundle_priority);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in setting priority of the bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");
				/**************************************************************/
				bundle_ack_object.spec->priority.ordinal = 0;
				bundle_ack_object.spec->critical = FALSE;
				bundle_ack_object.spec->flow_label = 0;
				bundle_ack_object.spec->unreliable = FALSE;
				/**************************************************************/
				debug_print(DEBUG_L2,"[DEBUG_L2] setting expiration time of the bundle ack...");

				error = al_bp_bundle_set_expiration(& bundle_ack_object, bundle_expiration);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in setting expiration time of the bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");

				debug_print(DEBUG_L2,"[DEBUG_L2] setting delivery options of the bundle ack...");

				//bundle_ack_dopts = BP_DOPTS_CUSTODY;
				bundle_ack_dopts = 0;
				al_bp_bundle_set_delivery_opts(& bundle_ack_object, bundle_ack_dopts);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					error_print("[DTNperf fatal error] in setting delivery options of the bundle ack: %s\n", al_bp_strerror(error));
					server_clean_exit(1);
				}
				debug_print(DEBUG_L2,"done\n");

				// send the bundle ack to the client
				if (send_ack_to_client)
				{
					debug_print(DEBUG_L2,"[DEBUG_L2] sending bundle ack to client...");
					utility_error = al_bp_extB_send(registration_descriptor, bundle_ack_object, bundle_source_addr, bundle_replyto_addr);
					switch (utility_error)
					{
					case BP_EXTB_SUCCESS:
						debug_print(DEBUG_L2," bundle ack sent to client\n");
						break;
					default:
						fflush(stdout);
						error_print("[DTNperf fatal error] in sending bundle ack to client: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_strerror(al_bp_extB_errno(registration_descriptor)));
						server_clean_exit(1);
						break;
					}
				}
				printf("Send bundle to client ok\n");

				// send the bundle ack to the monitor
				if (send_ack_to_monitor)
				{
					debug_print(DEBUG_L2,"[DEBUG_L2] sending bundle ack to monitor...");
					utility_error = al_bp_extB_send(registration_descriptor, bundle_ack_object, bundle_replyto_addr, bundle_replyto_addr);
					switch (utility_error)
					{
					case BP_EXTB_SUCCESS:
						debug_print(DEBUG_L2," bundle ack sent to monitor\n");
						break;
					default:
						fflush(stdout);
						error_print("[DTNperf fatal error] in sending bundle ack to monitor: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_strerror(al_bp_extB_errno(registration_descriptor)));
						server_clean_exit(1);
						break;
					}
				}
				//free memory for bundle ack
				al_bp_bundle_free(&bundle_ack_object);
				free(pl_buffer);
				pl_buffer_size = 0;
			}
			// free memory for bundle
			al_bp_bundle_free(&bundle_object);

			free(pl_filename);
			pl_filename_len = 0;
		}

	}// while(1)

	al_bp_extB_unregister(registration_descriptor);
	al_bp_extB_destroy();

	debugger_destroy();
	exit(0);
}
// end server code

// file expiration timer thread
void * file_expiration_timer(void * opt)
{
	u32_t current_dtn_time;
	file_transfer_info_list_item_t * item, * next;

	while(!get_forced_close_server())
	{
		current_dtn_time = get_current_dtn_time();

		pthread_mutex_lock(&mutexdata);

		for(item = file_transfer_info_list.first; item != NULL; item = next)
		{
			next = item->next;
			if (item->info->last_bundle_time + item->info->expiration < current_dtn_time)
			{
				char* filename = (char*) malloc(item->info->filename_len + strlen(item->info->full_dir) +1);
				strcpy(filename, item->info->full_dir);
				strcat(filename, item->info->filename);
				if (remove(filename) < 0)
					perror("[DTNperf error] in eliminating expired file:");
				printf("Eliminated file %s because timer has expired\n", filename);
				file_transfer_info_list_item_delete(&file_transfer_info_list, item);
				free(filename);
			}
		}
		pthread_mutex_unlock(&mutexdata);
		sched_yield();
		sleep(1);
	}
	pthread_exit(NULL);
}

void print_server_usage(char * progname)
{
	error_print("\n");
	error_print("dtnperf server mode\n");
	error_print("SYNTAX: %s %s [options]\n", progname, SERVER_STRING);
	error_print("\n");
	error_print("options:\n"
			//" -a, --daemon                 Start the server as a daemon. Output is redirected to %s.\n"
			" -o, --output <file>          Change the default output file (only with -a option).\n"
			" -s, --stop                   Stop the server daemon.\n"
			// XXX no more supported
			//"     --ip-addr <addr>         IP address of the BP daemon api. Default: 127.0.0.1\n"
			//"     --ip-port <port>         IP port of the BP daemon api. Default: 5010\n"
			"     --force-eid <[DTN|IPN]>  Force scheme of registration EID.\n"
			"     --ipn-local <num>        Set ipn local number (Use only with --force-eid IPN on DTN2)\n"
			"     --fdir <dir>             Destination directory of files transferred. Default is %s .\n"
			"     --debug[=level]          Debug messages [1-2], if level is not indicated level = 2.\n"
			" -M, --memory         	       Save received bundles into memory.\n"
			" -l, --lifetime <sec>         Bundle ACKs lifetime (s). Default is 60.\n"
			" -p, --priority <val>         Bundle ACKs priority [bulk|normal|expedited|reserved]. Default: normal\n"
			"     --acks-to-mon            Send bundle ACKs in cc to the monitor.\n"
			" -h, --help                   This help.\n",
			SERVER_OUTPUT_FILE, FILE_DIR_DEFAULT);
	error_print("\n");

	debugger_destroy();
	al_bp_extB_destroy();

	exit(1);
}

void parse_server_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt)
{
	char c, done = 0;
	//boolean_t output_set = FALSE;
	dtnperf_options_t * perf_opt = perf_g_opt->perf_opt;
	dtnperf_connection_options_t * conn_opt = perf_g_opt->conn_opt;
	// kill daemon variables
	int pid;
	char cmd[256];

	while (!done)
	{
		static struct option long_options[] =
		{
				{"help", no_argument, 0, 'h'},
				{"memory", no_argument, 0, 'M'},
				{"lifetime", required_argument, 0, 'l'},
				{"debug", optional_argument, 0, 33}, 			// 33 because D is for data mode
				{"priority", required_argument, 0, 'p'},
				{"ddir", required_argument, 0, 34},
				{"fdir", required_argument, 0, 39},
				{"acks-to-mon", no_argument, 0, 35},		// server only option
				{"ip-addr", required_argument, 0, 37},
				{"ip-port", required_argument, 0, 38},
				{"force-eid", required_argument, 0, 50},
				{"ipn-local", required_argument, 0, 51},
				{"daemon", no_argument, 0, 'a'},
				{"output", required_argument, 0, 'o'},
				{"stop", no_argument, 0, 's'},
				{0,0,0,0}	// The last element of the array has to be filled with zeros.

		};
		int option_index = 0;
		c = getopt_long(argc, argv, "hvMl:p:ao:s", long_options, &option_index);

		switch (c)
		{
		case 'h':
			print_server_usage(argv[0]);
			exit(0);
			return ;

		case 'M':
			perf_opt->use_file = 0;
			perf_opt->payload_type = BP_PAYLOAD_MEM;
			break;

		case 'l':
			conn_opt->expiration = atoi(optarg);
			break;

		case 'p':
			if (!strcasecmp(optarg, "bulk"))   {
				conn_opt->priority.priority = BP_PRIORITY_BULK;
			} else if (!strcasecmp(optarg, "normal")) {
				conn_opt->priority.priority = BP_PRIORITY_NORMAL;
			} else if (!strcasecmp(optarg, "expedited")) {
				conn_opt->priority.priority = BP_PRIORITY_EXPEDITED;
			} else if (!strcasecmp(optarg, "reserved")) {
				conn_opt->priority.priority = BP_PRIORITY_RESERVED;
			} else {
				error_print("[DTNperf syntax error] Invalid priority value %s\n", optarg);
				exit(1);
			}
			break;

		case 33: // debug
			if (optarg){
				int debug = atoi(optarg);
				if(!(debug >=1 && debug <=2)){
					error_print("[DTNperf syntax error] wrong --debug argument\n");
					exit(1);
					return;
				}
				else
					perf_opt->debug=debug;
			}
			else
				perf_opt->debug = 2;
			break;

		case 34: //incoming bundles destination directory
			perf_opt->dest_dir = strdup(optarg);
			break;

		case 35: //server send acks to monitor
			perf_opt->acks_to_mon = TRUE;
			break;

		case 36: //server do not send acks
			perf_opt->no_acks = TRUE;
			break;

		case 37:
			if(perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
			{
				error_print("[DTNperf error] --ip-addr supported only in DTN2 and IBR-DTN\n");
				exit(1);
				return;
			}
			perf_opt->ip_addr = strdup(optarg);
			perf_opt->use_ip = TRUE;
			break;

		case 38:
			if(perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
			{
				error_print("[DTNperf error] --ip-port supported only in DTN2 and IBR-DTN\n");
				exit(1);
				return;
			}
			perf_opt->ip_port = atoi(optarg);
			perf_opt->use_ip = TRUE;
			break;

		case 39:
			perf_opt->file_dir = strdup(optarg);
			break;

		case 50:
			switch( find_forced_eid(strdup(optarg)) )
			{
			case 'D':
				perf_opt->eid_format_forced = 'D';
				break;
			case 'I':
				perf_opt->eid_format_forced = 'I';
				break;
			case '?':
				error_print("[DTNperf syntax error] wrong --force-eid argument\n");
				exit(1);
			}
			break;

			case 51:
				perf_opt->ipn_local_num = atoi(optarg);
				if (perf_opt->ipn_local_num <= 0)
				{
					error_print("[DTNperf syntax error] wrong --ipn_local argument\n");
					exit(1);
				}
				break;

			case 'a':
#ifdef mingw
				fprintf(stdout, "Daemon not supported in MINGW.\n");
				exit(1);
#endif
				//perf_opt->daemon = TRUE;
				fprintf(stdout, "Daemon no more supported.\n");
				exit(1);
				break;

			case 'o':
				perf_opt->server_output_file = strdup(optarg);
				//output_set = TRUE;
				break;

			case 's':
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "%s %s", argv[0], SERVER_STRING);
				pid = find_proc(cmd);
				if (pid)
				{
					printf("Closing dtnperf server pid: %d\n", pid);
#ifdef mingw
					printf("Not supported in MINGW.\n");
#else
					kill(pid, SIGINT);
#endif
				}
				else
				{
					error_print("[DTNperf error] cannot find a running instance of dtnperf server\n");
				}
				exit(0);
				break;

			case '?':
				error_print("[DTNperf error] unknown option: %c\n", optopt);
				exit(1);
				break;

			case (char)(-1):
																															done = 1;
			break;

			default:
				// getopt already prints an error message for unknown option characters
				print_server_usage(argv[0]);
				exit(1);
		}
	}
	/*if (output_set && !perf_opt->daemon)
	{
		error_print("\n[DTNperf syntax error] -o option can be used only with -a option\n");
		print_server_usage(argv[0]);
		exit(1);
	}*/
	if(perf_opt->bp_implementation == BP_DTN && perf_opt->eid_format_forced == 'I' &&
			perf_opt->ipn_local_num == 0)
	{
		error_print("\n[DTNperf syntax error] must indicate ipn local number (--ipn-local option)\n");
		print_server_usage(argv[0]);
		exit(1);
	}
}

// Ctrl+C handler
#ifdef mingw
BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch(dwType) {
	case CTRL_C_EVENT:
		printf("\nDtnperf server: received SIGINT. Exiting\n");
		server_clean_exit(0);
		break;
	case CTRL_BREAK_EVENT:
		printf("\nDtnperf server: received SIGINT. Exiting\n");
		server_clean_exit(0);
		break;
	}
	return TRUE;
}
#else
void server_handler(int sig)
{
	printf("\nDTNperf server received SIGINT: Exiting\n");
	server_clean_exit(0);
}
#endif

void server_clean_exit(int status)
{
	file_transfer_info_list_item_t * item;

	// terminate immediately all child threads
	//pthread_cancel(file_exp_timer);

	set_forced_close_server(TRUE);

	// delete all incomplete files
	for(item = file_transfer_info_list.first; item != NULL; item = item->next)
	{
		char* filename = (char*) malloc(item->info->filename_len + strlen(item->info->full_dir) +1);
		strcpy(filename, item->info->full_dir);
		strcat(filename, item->info->filename);
		if (remove(filename) < 0)
			perror("[DTNperf error] in eliminating incomplete file:");
		printf("Eliminated file %s because incomplete\n", filename);
		file_transfer_info_list_item_delete(&file_transfer_info_list, item);
		free(filename);
	}

	//al_bp_extB_unregister(registration_descriptor);
	al_bp_extB_destroy();

	debugger_destroy();
	printf("DTNperf server: Exit.\n");
	exit(status);
}

static boolean_t get_forced_close_server()
{
	int result;
	pthread_mutex_lock(&mutex_forced_close);
	result = forced_close;
	pthread_mutex_unlock(&mutex_forced_close);
	return result;
}

static void set_forced_close_server(boolean_t value)
{
	pthread_mutex_lock(&mutex_forced_close);
	forced_close = value;
	pthread_mutex_unlock(&mutex_forced_close);
}
