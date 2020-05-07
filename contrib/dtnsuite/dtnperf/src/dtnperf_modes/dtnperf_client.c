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
 * dtnperf_client.c
 */

#include "../dtnperf_debugger.h"

#include "dtnperf_client.h"
#include "dtnperf_monitor.h"
#include "../includes.h"
#include "../definitions.h"
#include "../bundle_tools.h"
#include "../file_transfer_tools.h"
#include "../utils.h"
#include <semaphore.h>
#include <libgen.h>
#include <sys/stat.h>
#include <al_bp_api.h>
#include <al_bp_extB.h>


#ifdef mingw
#include <windows.h>

# define timersub(a, b, result) \
		do { \
			(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
			(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
			if ((result)->tv_usec < 0) { \
				--(result)->tv_sec; \
				(result)->tv_usec += 1000000; \
			} \
		} while (0)

BOOL WINAPI ConsoleHandlerFirst(DWORD);
BOOL WINAPI ConsoleHandlerSecond(DWORD);
int wait_ctrl_c_block = 0; //for ctrl_c in windows
#endif

/* pthread_yield() is not standard,
   so use sched_yield if necessary */
#ifndef HAVE_PTHREAD_YIELD
#   ifdef HAVE_SCHED_YIELD
#       include <sched.h>
#       define pthread_yield    sched_yield
#   endif
#endif

// pthread variables
static pthread_t sender;
static pthread_t cong_ctrl;
static pthread_t cong_expir_timer;
static pthread_t wait_for_signal;
static pthread_mutex_t mutexclose;
static pthread_mutex_t mutex_forced_close;
static sem_t window;			// semaphore for congestion control (number of window)
static sem_t inflight;				// semaphore for congestion control (number of window inflight)

// variables of client threads
static send_information_t * send_info = NULL;		// array info of sent bundles
static long tot_bundles;					// for data mode
static struct timeval start, end, now;			// time variables
static struct timeval bundle_sent, ack_recvd;	// time variables
static int sent_bundles = 0;					// sent bundles counter
static unsigned long long sent_data = 0;				// sent byte counter
static int close_ack_receiver = 0;			// to signal the ack receiver to close
//static unsigned int data_written = 0;			// num of bytes written on the source file
static long int wrong_crc;

static boolean_t process_interrupted;
static boolean_t expir_timer_cong_window;


static char source_file[256];					// complete name of source file: SOURCE_FILE_pid[_numBundle]
static char * transfer_filename;			// basename of the file to transfer
static u32_t transfer_filedim;				// size of the file to transfer
static int transfer_fd;					// file descriptor

// flags to exit cleanly
static boolean_t source_file_created;

// buffer settings
static char* buffer = NULL;        	    // buffer containing data to be transmitted
static size_t bufferLen;                   // length of buffer

static boolean_t forced_close = FALSE;

// BP variables
static al_bp_extB_registration_descriptor registration_descriptor;
static al_bp_bundle_id_t * bundle_id;
static al_bp_endpoint_id_t dest_eid;
static al_bp_endpoint_id_t mon_eid;
static al_bp_bundle_object_t bundle;
static char ** file_bundle_names;


static dtnperf_options_t * perf_opt;
static dtnperf_connection_options_t * conn_opt;

extension_block_info_t * ext_blocks;
static u_int num_meta_blocks = 0;
static u_int num_ext_blocks = 0;

//useful function to parse eid
static al_bp_error_t parse_eid(al_bp_endpoint_id_t* endpoint_pointer, char* eid, char* dtn_string);
static void send_force_stop_bundle();
static void print_mode();
static void get_header_payload(u32_t* header_size, double* dtnperf_payload);
static void parse_monitor(al_bp_endpoint_id_t* endpoint_pointer);
static void parse_server(al_bp_endpoint_id_t* endpoint_pointer);
static void open_bp_connection();
static void set_close_ack_receiver(int value);
static int get_close_ack_receiver();
static boolean_t receive_ack(dtnperf_options_t *perf_opt);
static void set_forced_close_client(boolean_t value);
static boolean_t get_forced_close_client();

void set_metadata_block_information(al_bp_bundle_object_t *bundle);
void set_extension_block_information(al_bp_bundle_object_t *bundle);

/*  ----------------------------
 *          CLIENT CODE
 *  ---------------------------- */
void run_dtnperf_client(dtnperf_global_options_t * perf_g_opt)
{
	/* ------------------------
	 * variables
	 * ------------------------ */
	int pthread_status;

	//unsigned long int temp;
	//	FILE * stream; // stream for preparing payolad
	al_bp_bundle_object_t bundle_stop;

	//void * ext_buf;
	//void * meta_buf;

	al_bp_extB_error_t utility_error;
	al_bp_error_t error;

	/* ------------------------
	 * initialize variables
	 * ------------------------ */
	perf_opt = perf_g_opt->perf_opt;
	conn_opt = perf_g_opt->conn_opt;
	source_file_created = FALSE;
	tot_bundles = 0;
	process_interrupted = FALSE;
	//eid_format = 'N';

	wrong_crc = 0;

	perf_opt->log_filename = correct_dirname(perf_opt->log_filename);


	//init debug
	if (debugger_init(perf_opt->debug, perf_opt->create_log, perf_opt->log_filename) != EXIT_SUCCESS)
	{
		error_print("[DTNperf fatal error] in opening log file\n");
		client_clean_exit(1);
	}

	// Connect to BP Daemon
	debug_print(DEBUG_L2,"[DEBUG_L2] opening connection to local BP daemon...");
	open_bp_connection();
	debug_print(DEBUG_L2,"done\n");

	// Ctrl+C handler
#ifdef mingw
	signal(SIGINT,SIG_IGN);
	if (!SetConsoleCtrlHandler(ConsoleHandlerFirst,TRUE)) {
		fprintf(stderr, "Unable to install handler!\n");
		client_clean_exit(1);
	}
#else
	signal(SIGINT, client_handler);
#endif

	/* -----------------------------------------------------
	 *   initialize and parse bundle src/dest/replyto EIDs
	 * ----------------------------------------------------- */

	// parse SERVER EID
	parse_server(&dest_eid);

	al_bp_endpoint_id_t source = al_bp_extB_get_local_eid(registration_descriptor);
	debug_print(DEBUG_OFF,"local_eid = %s\n", source.uri);
	debug_print(DEBUG_L1, "[DEBUG_L1] Destination: %s\n", dest_eid.uri);

	// parse REPLY-TO (if not specified, the same as the source)
	parse_monitor(&mon_eid);

	/**************** DELETED INTERNAL MONITOR *******************/
	// checking if there is a running monitor on this endpoint
	/*
	if(perf_g_opt->mode == DTNPERF_CLIENT_MONITOR)
	{
		debug_print(DEBUG_L2,"[DEBUG_L2] checking for existing monitor on this endpoint...\n");
		error = al_bp_extB_find_registration(registration_descriptor, &mon_eid);
		if ((perf_opt->bp_implementation == BP_DTN && error == BP_SUCCESS) || //or
				(perf_opt->bp_implementation == BP_ION && (error == BP_EBUSY || error == BP_EPARSEEID)) || //or
				(perf_opt->bp_implementation == BP_IBR && error == BP_SUCCESS))
		{
			dedicated_monitor = FALSE;
			printf("there is already a monitor on this endpoint.\n");
			printf("regid 0x%x\n", (unsigned int) al_bp_extB_get_regid(registration_descriptor));
		}
		else
		{
			char temp1[256];
			monitor_parameters_t mon_params;
			dedicated_monitor = TRUE;
			mon_params.client_id = getpid();
			mon_params.perf_g_opt = perf_g_opt;
			printf("there is not a monitor on this endpoint.\n");
			// if the scheme is not "ipn", append monitor demux string to reply-to EID
			if(strncmp(perf_opt->mon_eid, "ipn", 3) != 0)
				sprintf(temp1, "%s_%d", mon_eid.uri, mon_params.client_id);
			else
				sprintf(temp1, "%s", mon_eid.uri);
			al_bp_parse_eid_string(&mon_eid, temp1);
			// start dedicated monitor
			if ((monitor_pid = fork()) == 0)
			{
				start_dedicated_monitor((void *) &mon_params);
				exit(0);
			}
			printf("dedicated (internal) monitor started\n");
			strcpy(mon_eid.uri, temp1);
		}
		debug_print(DEBUG_L2," done\n");
	}*/

	debug_print(DEBUG_L1,"[DEBUG_L1] Reply-to   : %s\n\n", mon_eid.uri);

	//create a new registration to the local router based on this eid
	debug_print(DEBUG_L1,"[DEBUG_L1] regid 0x%x\n", (unsigned int) al_bp_extB_get_regid(registration_descriptor));

	// if bundle payload > MAX_MEM_PAYLOAD, then memorize payload in file
	if (!perf_opt->use_file && perf_opt->bundle_payload > MAX_MEM_PAYLOAD)
	{
		perf_opt->use_file = 1;
		perf_opt->bundle_payload = BP_PAYLOAD_FILE;
		debug_print(DEBUG_L1,"[DTNperf warning] Payload %f > %d: using file instead of memory\n", perf_opt->bundle_payload, MAX_MEM_PAYLOAD);
	}


	u32_t header_size;
	double dtnperf_payload;
	get_header_payload(&header_size, &dtnperf_payload);

	debug_print(DEBUG_L2,"[DEBUG_L2] dtnperf header length: %u\n", header_size);
	debug_print(DEBUG_L2,"[DEBUG_L2] dtnperf payload length: %f\n", dtnperf_payload);

	/* ------------------------------------------------------------------------------
	 * select the operative-mode (between Time_mode, Data_mode and File_mode)
	 * ------------------------------------------------------------------------------ */

	print_mode();

	debug_print(DEBUG_L1,"[DEBUG_L1] Transmitting data using = %s\n", perf_opt->use_file ? "a file" : "memory");

	debug_print(DEBUG_L1,"[DEBUG_L1] Congestion control = %s:\n", perf_opt->congestion_ctrl == 'W' ? "window" : "rate");

	if(perf_opt->congestion_ctrl == 'W')
	{
		debug_print(DEBUG_L1,"\twindow is %d bundle\n", perf_opt->window);
	}
	else
	{
		debug_print(DEBUG_L1,"\trate is %f %c\n", perf_opt->rate, perf_opt->rate_unit);
	}
	debug_print(DEBUG_L1,"[DEBUG_L1] Payload is %f byte\n", perf_opt->bundle_payload);


	sent_bundles = 0;

	if (perf_opt->op_mode == 'D' || perf_opt->op_mode == 'F') // Data or File mode
	{
		debug_print(DEBUG_L2,"[DEBUG_L1] calculating how many bundles are needed...");

		if (perf_opt->op_mode == 'F') // File mode
		{
			struct stat file;
			if (stat(perf_opt->F_arg, &file) < 0)
			{
				error_print("[DTNperf fatal error] in stat (linux) of file %s : %s", perf_opt->F_arg, strerror(errno));
				client_clean_exit(1);
			}

			{
				char* namefile = basename(perf_opt->F_arg);
				transfer_filename = malloc(strlen(namefile) + 1);
				strcpy(transfer_filename, namefile);
			}

			transfer_filedim = file.st_size;
			tot_bundles += bundles_needed(transfer_filedim, get_file_fragment_size(perf_opt->bundle_payload, strlen(transfer_filename), strlen(perf_opt->mon_eid)));

			file_bundle_names = (char * *) malloc(sizeof(char *) * tot_bundles);
		}
		else // Data mode
			tot_bundles += bundles_needed(perf_opt->data_qty, perf_opt->bundle_payload);

		debug_print(DEBUG_L2," n_bundles = %ld\n", tot_bundles);
	}

	// Create the bundle object
	debug_print(DEBUG_L2,"[DEBUG_L2] creating the bundle object...");
	error = al_bp_bundle_create(&bundle);
	if (error != BP_SUCCESS)
	{
		error_print("[DTNperf fatal error] in creating bundle object\n");
		client_clean_exit(1);
	}
	debug_print(DEBUG_L2," done\n");

	debug_print(DEBUG_L2,"[DEBUG_L2] number of blocks: %d\n", perf_opt->num_blocks);

	// set extension block information
	//ext_buf = set_extension_block_information(&bundle);

	// set metadata block information
	//meta_buf = set_metadata_block_information(&bundle);
	set_metadata_block_information(&bundle);


	// Create the array for the bundle send info (only for sliding window congestion control)
	if (perf_opt->congestion_ctrl == 'W') {
		debug_print(DEBUG_L2,"[DEBUG_L2] creating structure for sending information...");

		send_info = (send_information_t*) malloc(perf_opt->window * sizeof(send_information_t));
		init_info(send_info, perf_opt->window);

		debug_print(DEBUG_L2," done\n");
	}

	// Open File Transferred
	if(perf_opt->op_mode == 'F') // File mode
	{
		// open file to transfer in read mode
		if ((transfer_fd = open(perf_opt->F_arg, O_RDONLY)) < 0)
		{
			error_print("[DTNperf fatal error] in stat (linux) of file %s : %s", perf_opt->F_arg, strerror(errno));
			client_clean_exit(2);
		}
	}

	// Setting the bundle options
	set_bp_options(&bundle, conn_opt);

	// intialize stop bundle;
	//al_bp_bundle_create(&bundle_stop);

	debug_print(DEBUG_L2,"[DEBUG_L2] entering in the transmission loop\n");

	// Run threads
	if (perf_opt->congestion_ctrl == 'W') { // sliding window congestion control
		sem_init(&window, 0, perf_opt->window);
		sem_init(&inflight, 0, 0); // no bundles on air at startup
	} else {								// rate based congestion control
		sem_init(&window, 0, 0);
		sem_init(&inflight, 0, 0); // no bundles on air at startup
	}


	sigset_t sigset;

	// blocking signals for the threads
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);

	pthread_mutex_init (&mutexclose, NULL);

	pthread_create(&sender, NULL, send_bundles, (void*)perf_g_opt);
	pthread_create(&cong_ctrl, NULL, congestion_control, (void*)perf_g_opt);
	pthread_create(&wait_for_signal, NULL, wait_for_sigint, NULL);

// this is the thread creating the problem
	pthread_join(sender, (void**)&pthread_status);
	//pthread_cancel(cong_ctrl); // terminate congestion control thread
	pthread_join(cong_ctrl, (void**)&pthread_status);

	pthread_cancel(cong_ctrl);
	pthread_mutex_destroy(&mutexclose);
	sem_destroy(&window);
	sem_destroy(&inflight);

	if (perf_opt->congestion_ctrl == 'W')
		destroy_info(send_info);

	while (get_forced_close_client()) // waiting for client_clean_exit, something was wrong
		/*pause()*/;

	// if user sent Ctrl+C to the client,
	// let the wait_for_signal thread to terminate the execution
	if (process_interrupted)
		pause();

	debug_print(DEBUG_L2,"[DEBUG_L2] out from loop\n");

	// Get the TOTAL end time
	debug_print(DEBUG_L2,"[DEBUG_L2] getting total end-time...");

	gettimeofday(&end, NULL);

	debug_print(DEBUG_L2," end.tv_sec = %u s\n", (u_int)end.tv_sec);

	// Print final report
	print_final_report(stdout);
	if(perf_opt->create_log)
		print_final_report(get_log_fp());

	if (strncmp(mon_eid.uri, al_bp_get_none_endpoint_string(), strlen(al_bp_get_none_endpoint_string())) != 0)
	{
		if (!perf_opt->no_bundle_stop)
		{
			al_bp_bundle_create(&bundle_stop);
			al_bp_endpoint_id_t none;
			al_bp_get_none_endpoint(&none);
			// fill the stop bundle
			prepare_stop_bundle(&bundle_stop, mon_eid, conn_opt->expiration, conn_opt->priority, sent_bundles);
			//debug_print(DEBUG_L2,"[DEBUG_L2] registration_descriptor: %d\n", registration_descriptor);
			al_bp_endpoint_id_t local_eid = al_bp_extB_get_local_eid(registration_descriptor);
			//debug_print(DEBUG_L1,"local_eid foud: %s\n", local_eid.uri);

			if ((utility_error = al_bp_bundle_set_source(&bundle_stop, local_eid)) != BP_EXTB_SUCCESS )
			{
				debug_print(DEBUG_L1,"[DTNperf fatal error] in setting source in stop bundle: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_extB_strerror(registration_descriptor));
				client_clean_exit(1);
			}
			print_bundle(&bundle_stop);
//			 send stop bundle to monitor
			debug_print(DEBUG_L1,"sending the stop bundle to the monitor...");
			if ((utility_error = al_bp_extB_send(registration_descriptor, bundle_stop, mon_eid, none)) != BP_EXTB_SUCCESS)
			{
				error_print("[DTNperf fatal error] in sending stop bundle: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_extB_strerror(registration_descriptor));
				client_clean_exit(1);
			}
			debug_print(DEBUG_L1,"done.\n");
			al_bp_bundle_free(&bundle_stop);
		} else
		{
			debug_print(DEBUG_L1,"[DEBUG_L1] bundle stop disabled.\n");
		}
	}
	else
	{
		debug_print(DEBUG_L1,"[DEBUG_L1] Bundle stop not needed, mon_eid = %s\n", al_bp_get_none_endpoint_string());
	}

	sleep(1);
	// Close the BP handle --
	debug_print(DEBUG_L2,"[DEBUG_L2] closing DTN handle...");
	//debug_print(DEBUG_L2,"[DEBUG_L2] registration_descriptor: %d\n", registration_descriptor);
	switch (al_bp_extB_unregister(registration_descriptor))
	{
	case BP_EXTB_ERRCLOSE:
		error_print("[DTNperf fatal error] in closing bp handle: %s\n", strerror(errno));
		client_clean_exit(1);
		break;
	case BP_EXTB_ERRUNREGISTER:
		error_print("[DTNperf fatal error] unregisted endpoint: %s\n", strerror(errno));
		client_clean_exit(1);
		break;
	default:
		break;
	}
	debug_print(DEBUG_L2," done\n");

	// deallocate memory
	if (perf_opt->op_mode == 'F')
	{
		close(transfer_fd);
	}
	if (perf_opt->use_file)
	{
		//	remove(source_file);
		source_file_created = FALSE;

		debug_print(DEBUG_L2, "[DEBUG_L2] removed file %s\n", source_file);
	}

	// free resource
	free((void*)buffer);
	free(transfer_filename);
	free(send_info);
	if(perf_opt->op_mode == 'F')
	{
		int i;
		al_bp_bundle_payload_t tmp_payload;
		tmp_payload.location = bundle.payload->location;

		if( perf_opt->bp_implementation == BP_ION)
		{
			for (i = 0; i< tot_bundles ; i++ )
			{
				// The last bundle delivered is not deleted here
				// Is deleted with the free(&bundle)
				if( strcmp(bundle.payload->filename.filename_val, file_bundle_names[i]) != 0)
				{
					tmp_payload.filename.filename_len = strlen(file_bundle_names[i]);
					tmp_payload.filename.filename_val = file_bundle_names[i];
					al_bp_free_payload(&tmp_payload);
				}
			}
		}
		else
		{
			for (i = 0; i< tot_bundles ; i++ )
			{
				remove(file_bundle_names[i]);
				free(file_bundle_names[i]);
			}
			free(file_bundle_names);
		}
	}
	//structure bundle is always free in every op mode
	al_bp_bundle_free(&bundle);
	if (perf_opt->num_blocks > 0)
	{
		/*free(ext_buf);
		free(meta_buf);*/
		//meta_buf = NULL;
		//ext_buf = NULL;
		int i;
		for ( i=0; i<perf_opt->num_blocks; i++ )
		{
			/*printf("Freeing extension block info [%d].data at 0x%08X\n",
					i, ext_blocks[i].block.data.data_val);*/
			//free(ext_blocks[i].block.data.data_val);
			ext_blocks[i].block.data.data_val = NULL;
		}
		ext_blocks = NULL;
	}

	//destroy prima del messaggio
	debugger_destroy();
	al_bp_extB_destroy();
	if (perf_opt->create_log)
		printf("\nClient log saved: %s\n", perf_opt->log_filename);

	printf("\n");

	exit(0);
}
// end client code

void set_metadata_block_information(al_bp_bundle_object_t *bundle)
{
	void * meta_buf = NULL;
	if (num_meta_blocks > 0)
	{
		al_bp_extension_block_t * meta_bp;
		meta_buf = malloc(num_meta_blocks * sizeof(al_bp_extension_block_t));
		memset(meta_buf, 0, num_meta_blocks * sizeof(al_bp_extension_block_t));
		int i=0;

		meta_bp = (al_bp_extension_block_t *)meta_buf;
		for (i = 0; i < num_meta_blocks; i++)
		{
			if (!check_metadata(&ext_blocks[i]))
			{
				continue;
			}
			meta_bp->type = ext_blocks[i].metadata_type;
			meta_bp->flags = ext_blocks[i].block.flags;
			meta_bp->data.data_len = ext_blocks[i].block.data.data_len;
			meta_bp->data.data_val = ext_blocks[i].block.data.data_val;;
			meta_bp++;
		}
		(*bundle).spec->metadata.metadata_len = num_meta_blocks;
		(*bundle).spec->metadata.metadata_val = (al_bp_extension_block_t *)meta_buf;
		print_ext_or_metadata_blocks(num_meta_blocks, (*bundle).spec->metadata.metadata_val);
	}
}

void set_extension_block_information(al_bp_bundle_object_t *bundle)
{
	void *ext_buf = NULL;
	if (num_ext_blocks > 0)
	{
		al_bp_extension_block_t * ext_bp;
		ext_buf = malloc(num_ext_blocks * sizeof(al_bp_extension_block_t));
		memset(ext_buf, 0, num_ext_blocks * sizeof(al_bp_extension_block_t));
		int i=0;

		ext_bp = (al_bp_extension_block_t *)ext_buf;
		for (i = 0; i < num_ext_blocks; i++)
		{
			if (check_metadata(&ext_blocks[i]))
			{
				continue;
			}
			ext_bp->type = ext_blocks[i].block.type;
			ext_bp->flags = ext_blocks[i].block.flags;
			ext_bp->data.data_len = ext_blocks[i].block.data.data_len;
			ext_bp->data.data_val = ext_blocks[i].block.data.data_val;;
			ext_bp++;
		}
		(*bundle).spec->blocks.blocks_len = num_ext_blocks;
		(*bundle).spec->blocks.blocks_val = (al_bp_extension_block_t *)ext_buf;
		print_ext_or_metadata_blocks(num_meta_blocks, (*bundle).spec->metadata.metadata_val);
	}
}

void print_mode()
{
	debug_print(DEBUG_L1,"[DEBUG_L1] Transmission mode = %c\n", perf_opt->op_mode);
	switch (perf_opt->op_mode)
	{
	case 'T':	// Time mode
		debug_print(DEBUG_L1,"\trequested Transmission interval %d (s)\n", perf_opt->transmission_time);
		break;
	case 'D': // Data mode
		debug_print(DEBUG_L1,"\trequested Transmission of %f bytes of data\n", perf_opt->data_qty);
		break;
	case 'F': // File mode
		debug_print(DEBUG_L1,"\trequested Transmission of file %s\n", perf_opt->F_arg);
		break;
	}
}

void get_header_payload(u32_t* header_size, double* dtnperf_payload)
{
	if(perf_opt->op_mode == 'F')
		*header_size = get_header_size(perf_opt->op_mode, strlen(perf_opt->F_arg), strlen(perf_opt->mon_eid) );
	else
		*header_size = get_header_size(perf_opt->op_mode, 0, strlen(perf_opt->mon_eid) );

	*dtnperf_payload = perf_opt->bundle_payload - *header_size;

	if (*dtnperf_payload <= 0)
	{
		fflush(stdout);
		error_print("[DTNperf warning] bundle payload too small. Extended to %d bytes\n", DEFAULT_PAYLOAD);
		perf_opt->bundle_payload = DEFAULT_PAYLOAD; //client_clean_exit(1);
	}
}

void parse_monitor(al_bp_endpoint_id_t* endpoint_pointer)
{
	al_bp_error_t error;
	if (strlen(perf_opt->mon_eid) == 0)
	{
		char* ptr;
		char* temp;
		al_bp_endpoint_id_t temp_eid = al_bp_extB_get_local_eid(registration_descriptor);
		switch (al_bp_extB_get_eid_format())
		{
		case 'D':
			ptr = strstr(&(temp_eid.uri[0]), CLI_EP_STRING);
			// copy from local EID only the uri (not the demux string)
			strncpy(perf_opt->mon_eid, &(temp_eid.uri[0]), ptr - &(temp_eid.uri[0]));
			break;
		case 'I':

			temp = (char *) malloc(sizeof(char)*AL_BP_MAX_ENDPOINT_ID);
			strcpy(temp, &(temp_eid.uri[0]));
			ptr = strtok(temp , ".");
			sprintf(temp, "%s.%s", ptr, MON_EP_NUM_SERVICE);
			memcpy(&(perf_opt->mon_eid[0]), temp, strlen(temp));
			free(temp);
			break;
		}
	}
	if (strcmp(perf_opt->mon_eid, al_bp_get_none_endpoint_string()) == 0) // se mon_eid == dtn:none DO NOT append MON_EP_STRING
		error = al_bp_parse_eid_string(&mon_eid, perf_opt->mon_eid);
	else
		error = parse_eid(&mon_eid, perf_opt->mon_eid, MON_EP_STRING);
	if (error != BP_SUCCESS)
	{
		error_print("[DTNperf fatal error] in parsing BP EID: invalid eid string '%s'\n", perf_opt->dest_eid);
		client_clean_exit(1);
	}
}

void parse_server(al_bp_endpoint_id_t* endpoint_pointer)
{
	al_bp_error_t error;
	error = parse_eid(&dest_eid, perf_opt->dest_eid, SERV_EP_STRING);
	if (error != BP_SUCCESS)
	{
		error_print("[DTNperf fatal error] in parsing BP EID: invalid eid string '%s'\n", perf_opt->dest_eid);
		client_clean_exit(1);
	}
}

void open_bp_connection()
{
	al_bp_extB_error_t utility_error;
	int pid = (int) getpid();
	char* register_string_dtn;
	// initialize the al_bp_extB
	utility_error = al_bp_extB_init(perf_opt->eid_format_forced, perf_opt->ipn_local_num);
	if (utility_error != BP_EXTB_SUCCESS)
	{
		error_print("[DTNperf fatal error] no BP protocol found.\n");
		client_clean_exit(1);
	}

	register_string_dtn = malloc(strlen(CLI_EP_STRING) + 3 + ((int)log10(pid)));
	sprintf(register_string_dtn, "%s_%d", CLI_EP_STRING, pid);
	
	// XXX No more supported
	/*if (perf_opt->use_ip)
		utility_error = al_bp_extB_register(&registration_descriptor, perf_opt->ip_addr, perf_opt->ip_port, perf_opt->ipn_local_num, pid + ((pid < 10000) ? 10000 : 0), register_string_dtn, perf_opt->bp_implementation);
	else
		utility_error = al_bp_extB_register(&registration_descriptor, NULL, 0, perf_opt->ipn_local_num, pid + ((pid < 10000) ? 10000 : 0), register_string_dtn, perf_opt->bp_implementation);*/
		
	utility_error = al_bp_extB_register(&registration_descriptor, register_string_dtn, pid + ((pid < 10000) ? 10000 : 0));

	free(register_string_dtn);

	if (utility_error != BP_EXTB_SUCCESS)
	{
		error_print("[DTNperf fatal error] in %s: %s\n",al_bp_extB_str_type_error(utility_error),  al_bp_extB_strerror(registration_descriptor));
		client_clean_exit(1);
	}
}

al_bp_error_t parse_eid(al_bp_endpoint_id_t* endpoint_pointer, char* eid, char* dtn_string)
{
	if(strncmp(eid, "ipn", 3) != 0)
		strcat(eid, dtn_string);
	return al_bp_parse_eid_string(endpoint_pointer, eid);
}

/**
 * Create and Fill Buffer File Payload
 **/
void create_fill_payload_buf(int num_bundle){
	FILE * stream, *buf_stream;
	char *buf;
	boolean_t eof_reached;
	int bytes_written;
	al_bp_error_t error;

	if(perf_opt->op_mode == 'F')// File mode
		sprintf(source_file, "%s_%d_%d", SOURCE_FILE, getpid(), num_bundle);
	else 						// Time and Data mode
		sprintf(source_file, "%s_%d", SOURCE_FILE, getpid());

	// Create the file
	if (perf_opt->use_file)
	{
		// create the file
		debug_print(DEBUG_L2,"[DEBUG_L2] creating file %s...", source_file);

		stream = fopen(source_file, "wb");

		if (stream == NULL)
		{
			error_print("[DTNperf fatal error] in creating file %s.\n \b Maybe you have not permissions\n", source_file);
			client_clean_exit(2);
		}

		source_file_created = TRUE;

		fclose(stream);

		debug_print(DEBUG_L2," done\n");
	}

	// Fill the payload
	debug_print(DEBUG_L2,"[DEBUG_L2] filling payload...");

	if (perf_opt->use_file)
		error = al_bp_bundle_set_payload_file(&bundle, source_file, strlen(source_file));
	else
		error = al_bp_bundle_set_payload_mem(&bundle, buffer, bufferLen);

	if (error != BP_SUCCESS)
	{
		error_print("[DTNperf fatal error] in setting bundle payload\n");
		client_clean_exit(1);
	}
	debug_print(DEBUG_L2," done\n");

	// open payload stream in write mode
	if (open_payload_stream_write(bundle, &stream) < 0)
	{
		error_print("[DTNperf fatal error] in opening payload stream write mode\n");
		client_clean_exit(2);
	}

	buf = (char *) malloc(perf_opt->bundle_payload);
	double temp_bundle_payload = perf_opt->bundle_payload;
	buf_stream = open_memstream(&buf, (size_t *) &temp_bundle_payload);

	// prepare the payload
	if(perf_opt->op_mode == 'F') // File mode
	{
		//open_payload_stream_write(bundle, &stream);
		error = prepare_file_transfer_payload(perf_opt, buf_stream, transfer_fd,
				transfer_filename, transfer_filedim, conn_opt->expiration , &eof_reached, &bundle.payload->buf.buf_crc, &bytes_written);
		if(error != BP_SUCCESS)
		{
			error_print("[DTNperf fatal error] in preparing file transfer payload\n");
			client_clean_exit(2);
		}
	}
	else // Time and Data mode
	{
		error = prepare_generic_payload(perf_opt, buf_stream, &bundle.payload->buf.buf_crc, &bytes_written);
		if (error != BP_SUCCESS)
		{
			error_print("[DTNperf fatal error] in preparing payload: %s\n", al_bp_strerror(error));
			client_clean_exit(1);
		}
	}

	fclose(buf_stream);

	if (perf_opt->crc==TRUE)
		debug_print(DEBUG_L1,"[DEBUG_L1] CRC = %"PRIu32"\n", bundle.payload->buf.buf_crc);

	memcpy(buf+HEADER_SIZE+BUNDLE_OPT_SIZE+sizeof(al_bp_timeval_t), &bundle.payload->buf.buf_crc, BUNDLE_CRC_SIZE);

	fwrite(buf, bytes_written, 1, stream);

	// close the stream
	close_payload_stream_write(&bundle, stream);

	debug_print(DEBUG_L1,"[DEBUG_L1] Payload prepared\n");

	/*	if((debug) && (debug_level > 0))
	{
		u32_t h_size;
		uint16_t filename_len, monitor_eid_len;
		monitor_eid_len = strlen(perf_opt->mon_eid);
		if(perf_opt->op_mode == 'F')
		{
			filename_len = strlen(transfer_filename);
			h_size =  get_header_size(perf_opt->op_mode, filename_len, monitor_eid_len);
		}
		else
			h_size = get_header_size(perf_opt->op_mode, 0, monitor_eid_len);
		printf("[debug] dtnperf header size %lu byte\n", h_size);
	}*/

	free(buf);

} // end create_fill_payload_buf

void set_close_ack_receiver(int value)
{
	pthread_mutex_lock(&mutexclose);
	close_ack_receiver = value;
	pthread_mutex_unlock(&mutexclose);
}

int get_close_ack_receiver()
{
	int result;
	pthread_mutex_lock(&mutexclose);
	result = close_ack_receiver;
	pthread_mutex_unlock(&mutexclose);
	return result;
}

/**
 * Client Threads code
 **/
void * send_bundles(void * opt)
{
	dtnperf_options_t *perf_opt = ((dtnperf_global_options_t *)(opt))->perf_opt;
	boolean_t condition;
	u32_t actual_payload;

	al_bp_extB_error_t utility_error;

	// Initialize timer
	debug_print(DEBUG_L2,"[DEBUG_L2 send thread] initializing timer...");

	gettimeofday(&start, NULL);

	debug_print(DEBUG_L2," start.tv_sec = %d s\n", (u_int)start.tv_sec);

	if (perf_opt->op_mode == 'T')		// TIME MODE
	{									// Calculate end-time
		debug_print(DEBUG_L2,"[DEBUG_L2 send thread] calculating end-time...");

		end = set (0);
		end.tv_sec = start.tv_sec + perf_opt->transmission_time;

		debug_print(DEBUG_L2," end.tv_sec = %d s\n", (u_int)end.tv_sec);
	}

	debug_print(DEBUG_L2,"[DEBUG_L2 send thread] entering loop...\n");

	if (perf_opt->op_mode == 'T') 	// TIME MODE
	{								// init variables for loop and setting condition
		now.tv_sec = start.tv_sec;
		condition = now.tv_sec <= end.tv_sec;
	}
	else							// DATA and FILE MODE
	{								// setting condition for loop
		condition = sent_bundles < tot_bundles;
	}

	//Only for DATA e TIME MODE is the payload is the same for all bundle
	if (perf_opt->op_mode == 'T' || perf_opt->op_mode == 'D')
	{
		create_fill_payload_buf(0);
	}
	else //For FILE MODE created all the payload necessary
	{
		int i=0;
		for (i=0; i<tot_bundles; i++){
			create_fill_payload_buf(i);
		}
	}

	// send bundles loop
	while (condition && !get_forced_close_client())				//LOOP
	{
		al_bp_error_t error;
		// Set Payload FILE MODE
		// Add in bundles_file_array the bundle to send for free at the end every bundle
		if (perf_opt->op_mode == 'F')
		{
			sprintf(source_file, "%s_%d_%d", SOURCE_FILE, getpid(), sent_bundles);
			if (perf_opt->use_file)
				error = al_bp_bundle_set_payload_file(&bundle, source_file, strlen(source_file));
			else
				error = al_bp_bundle_set_payload_mem(&bundle, buffer, bufferLen);
			// memorized source_file
			file_bundle_names[sent_bundles] = (char *) malloc(sizeof(char) * bundle.payload->filename.filename_len);
			strcpy(file_bundle_names[sent_bundles], bundle.payload->filename.filename_val);
		}
		// window debug
		int cur;
		sem_getvalue(&window, &cur);
		debug_print(DEBUG_L2,"\t[DEBUG_L2 send thread] window = %d\n", cur);

		sem_wait(&window); // wait to have a position on window
		if (perf_opt->op_mode == 'T')	// TIME MODE
		{								// update time and condition
			gettimeofday(&now, NULL);
			condition = now.tv_sec <= end.tv_sec;
		}
		if(!condition) // semaphore is blocking --> can be timed out!
			break;

		// Send the bundle
		debug_print(DEBUG_L1,"passing the bundle to BP...\n");

		if ((utility_error = al_bp_extB_send(registration_descriptor, bundle, dest_eid, mon_eid)) != BP_EXTB_SUCCESS)
		{
			error_print("[DTNperf fatal error] in passing the bundle to BP: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_extB_strerror(registration_descriptor));
			client_clean_exit(1);
		}

		if ((error = al_bp_bundle_get_id(bundle, &bundle_id)) != 0)
		{
			error_print("[DTNperf fatal error] in getting bundle id: %s\n", al_bp_strerror(error));
			client_clean_exit(1);
		}
		print_bundle(&bundle);
		debug_print(DEBUG_L1,"bundle passed to BP\n");
		debug_print(DEBUG_L2,"\t[DEBUG_L2 send thread] ");

		printf("bundle timestamp: %llu.%llu\n", (unsigned long long) bundle_id->creation_ts.secs, (unsigned long long) bundle_id->creation_ts.seqno);

		// put bundle id in send_info (only windowed congestion control)
		if (perf_opt->congestion_ctrl == 'W') {
			gettimeofday(&bundle_sent, NULL);
			add_info(send_info, *bundle_id, bundle_sent, perf_opt->window);
			debug_print(DEBUG_L2,"\t[DEBUG_L2 send thread] added info for bundle\n");
		}

		// Increment sent_bundles
		++sent_bundles;
		debug_print(DEBUG_L2,"\t[DEBUG_L2 send thread] now bundle passed is %d\n", sent_bundles);
		// Increment data_qty
		al_bp_bundle_get_payload_size(bundle, &actual_payload);
		sent_data += actual_payload;
		if (perf_opt->op_mode == 'T')	// TIME MODE
		{								// update time and condition
			gettimeofday(&now, NULL);
			condition = now.tv_sec <= end.tv_sec;
		}
		else							// DATA MODE
		{								// update condition
			condition = sent_bundles < tot_bundles;
		}

		if (!condition) // setted before the sem_post, so the cong_ctrl can avoid to enter on receive
			set_close_ack_receiver(1);

		if (perf_opt->congestion_ctrl == 'W')
			sem_post(&inflight); // one bundles sent, increse semaphore of on air bundles
	} // while

	debug_print(DEBUG_L2,"[DEBUG_L2 send thread] ...out from sending loop\n");
	set_close_ack_receiver(1); // to be sure, should be 1 yet
	if (perf_opt->congestion_ctrl == 'R')
	{
		pthread_cancel(cong_ctrl); // terminate congestion control thread
	}

	sem_post(&inflight); // this allow the cong ctrl thead to exit from the sem_wait

	// close thread
	pthread_exit(NULL);

} // end send_bundles

static boolean_t receive_ack(dtnperf_options_t *perf_opt)
{
	uint32_t extension_ack;
	al_bp_timestamp_t reported_timestamp;
	//al_bp_endpoint_id_t ack_sender;
	HEADER_TYPE ack_header;
	int position = -1;
	boolean_t receive_valid;
	al_bp_extB_error_t utility_error;
	al_bp_bundle_object_t ack;
	al_bp_timeval_t timeout = 1; // in seconds (-1 for infinite)
	static boolean_t printed_message = FALSE; // TRUE=message printed, FALSE=i should print the message

	//al_bp_copy_eid(&ack_sender, &dest_eid);

	al_bp_bundle_create(&ack);

	// Wait for the reply
	if (!printed_message)
	{
		debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] waiting for the reply...\n");
		printed_message = TRUE;
	}

	utility_error = al_bp_extB_receive(registration_descriptor, &ack, BP_PAYLOAD_MEM, timeout);
	if (utility_error == BP_EXTB_ERRTIMEOUT)
	{
		al_bp_bundle_free(&ack);
		return FALSE; // timed out!
	}

	receive_valid = FALSE;
	printed_message = FALSE;
	switch (utility_error)
	{
	case BP_EXTB_SUCCESS:
		receive_valid = TRUE;
		break;
	case BP_EXTB_ERRRECEIVE:
		if(count_info(send_info, perf_opt->window) == 0 && get_close_ack_receiver() == 1) // send_bundles is terminated
			return FALSE; // not found a valid ack (it will terminate outside!)
		error_print("[DTNperf fatal error] in %s: %d (%s)\n", al_bp_extB_str_type_error(utility_error), al_bp_extB_get_error(registration_descriptor), al_bp_strerror(al_bp_extB_errno(registration_descriptor)));
		client_clean_exit(1);
		break;

	default:
		error_print("[DTNperf warning] %s \n", al_bp_extB_str_type_error(utility_error));
		break;
	}

	if (receive_valid)
	{
		al_bp_error_t error;
		// Check if is actually a server ack bundle
		get_bundle_header_and_options(&ack, &ack_header, NULL);
		if (ack_header != DSA_HEADER)
		{
			debug_print(DEBUG_L2,"[DTNperf cong ctrl] wrong bundle header in getting server ack\n");
			return FALSE; // not a valid ack
		}
		gettimeofday(&ack_recvd, NULL);
		debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] ack received\n");
		// Get ack infos
		error = get_info_from_ack(&ack, NULL, &reported_timestamp, &extension_ack);
		if (error != BP_SUCCESS)
		{
			error_print("[DTNperf fatal error] in getting info from ack: %s\n", al_bp_strerror(error));
			client_clean_exit(1);
		}

		if (extension_ack & BO_CRC_ENABLED)
			wrong_crc++;

		debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] ack received timestamp: %u %u\n", reported_timestamp.secs, reported_timestamp.seqno);

		if (perf_opt->bp_implementation != BP_IBR)
			position = is_in_info(send_info, reported_timestamp, perf_opt->window);
		else {
			position = is_in_info_timestamp(send_info, reported_timestamp, perf_opt->window);
			debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] ignoring ack sequence number\n");
		}

		if (position < 0)
		{
			error_print("[DTNperf warning] ack not validated\n");
		} else {
			int cur;
			remove_from_info(send_info, position);
			debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] ack validated\n");
			sem_getvalue(&window, &cur);
			debug_print(DEBUG_L2,"\t[DEBUG_L2 cong ctrl] window = %d\n", cur+1);

		}
	} // if receive_valid
	al_bp_bundle_free(&ack);
	return (position >= 0);
}

void * congestion_control(void * opt)
{
	dtnperf_options_t *perf_opt = ((dtnperf_global_options_t *)(opt))->perf_opt;

	debug_print(DEBUG_L2,"[DEBUG_L2 cong ctrl] congestion control started, mode = %c\n", perf_opt->congestion_ctrl);

	pthread_sleep(0.5);

	if (perf_opt->congestion_ctrl == 'W') // window based congestion control
	{
		//	gettimeofday(&temp, NULL);
		pthread_create(&cong_expir_timer, NULL, congestion_window_expiration_timer, NULL);
		while (((get_close_ack_receiver() == 0) || count_info(send_info, perf_opt->window) != 0) && !get_forced_close_client()
		/*|| (gettimeofday(&temp, NULL) == 0 && ack_recvd.tv_sec - temp.tv_sec <= perf_opt->wait_before_exit)*/)
		{
			sem_wait(&inflight);
			if (get_close_ack_receiver() == 1 && count_info(send_info, perf_opt->window) == 0 && get_forced_close_client())
				break;
			if (receive_ack(perf_opt)) // if the ack is valid --> window++
				sem_post(&window);
			else // if not valid --> inflight should remain the same as before (inflight++)
				sem_post(&inflight);

		} // end while
	}
	else if (perf_opt->congestion_ctrl == 'R') // Rate based congestion control
	{
		double interval_secs;

		if (perf_opt->rate_unit == 'b') // rate is bundles per second
		{
			interval_secs = 1.0 / perf_opt->rate;
		}
		else 							// rate is bit or kbit per second
		{
			if (perf_opt->rate_unit == 'k') // Rate is kbit per second
			{
				perf_opt->rate = kilo2byte(perf_opt->rate);
			}
			else // rate is Mbit per second
			{
				perf_opt->rate = mega2byte(perf_opt->rate);
			}
			interval_secs = (double)perf_opt->bundle_payload * 8 / perf_opt->rate;
		}

		debug_print(DEBUG_L1,"[DEBUG_L1 cong ctrl] wait time for each bundle: %.4f s\n", interval_secs);

		while(get_close_ack_receiver() == 0 && !get_forced_close_client())
		{
			int cur;
			sem_post(&window);
			sem_getvalue(&window, &cur);
			debug_print(DEBUG_L2,"[DEBUG_L2 cong ctrl] increased free window size = %d\n", cur);
			pthread_sleep(interval_secs);
		}
	}
	else // wrong char for congestion control
	{
		client_clean_exit(1);
	}

	pthread_exit(NULL);
	return NULL;
} // end congestion_control

void * congestion_window_expiration_timer(void * opt)
{
	struct timeval current_time;
	al_bp_timeval_t expiration = perf_opt->bundle_ack_options.ack_expiration + conn_opt->expiration;
	expir_timer_cong_window = FALSE;

	gettimeofday(&current_time, NULL);
	if(ack_recvd.tv_sec == 0)
		ack_recvd.tv_sec = current_time.tv_sec;

	while(!get_forced_close_client())
	{
		gettimeofday(&current_time, NULL);
		if( current_time.tv_sec - ack_recvd.tv_sec >=  expiration)
		{
			expir_timer_cong_window = TRUE;
			printf("\nExpiration timer congestion window\n");
			client_clean_exit(1);
			pthread_exit(NULL);
			return NULL;
		}
		sched_yield();
		sleep(1);
	}

	pthread_exit(NULL);
	return NULL;
} // end congestion_window_expiration_timer

/*void * start_dedicated_monitor(void * params)
{
	monitor_parameters_t * parameters = (monitor_parameters_t *) params;
	run_dtnperf_monitor(parameters);
	pthread_exit(NULL);
	return NULL;
} // end start_dedicated_monitor*/

void * wait_for_sigint(void * arg)
{
	sigset_t sigset;
	int signo;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	sigwait(&sigset, &signo);

	if(expir_timer_cong_window)
	{
		printf("\nDTNperf client: Expired Timer to receive the Server's Ack\n");
	}
	else
	{
		printf("\nDTNperf client received SIGINT: Exiting\n");
	}

	set_forced_close_client(TRUE);

	if (!perf_opt->no_bundle_stop)
	{ // prepare and send bundle force-stop to the monitor
		send_force_stop_bundle();
	}

	process_interrupted = TRUE;

	// terminate all child threads
	//pthread_cancel(sender);
	//pthread_cancel(cong_ctrl);
	//pthread_cancel(cong_expir_timer);

	client_clean_exit(0);

	return NULL;
} // end wait_for_sigint

void send_force_stop_bundle()
{
	al_bp_bundle_object_t bundle_force_stop;

	if (strcmp(mon_eid.uri, al_bp_get_none_endpoint_string()) == 0) // not needed to send a force stop to dtn:none
		return;

	// create the bundle force stop
	al_bp_bundle_create(&bundle_force_stop);

	// fill the force stop bundle
	prepare_force_stop_bundle(&bundle_force_stop, mon_eid, conn_opt->expiration, conn_opt->priority);

	if(perf_opt->bp_implementation == BP_DTN)
	{
		al_bp_error_t error;
		al_bp_handle_t force_stop_handle;
		// Open a new connection to BP Daemon
		debug_print(DEBUG_L2,"[DEBUG_L2] opening a new connection to local BP daemon...");
		if (perf_opt->use_ip)
			error = al_bp_open_with_ip(perf_opt->ip_addr, perf_opt->ip_port, &force_stop_handle);
		else
			error = al_bp_open(&force_stop_handle);

		if (error != BP_SUCCESS)
		{
			error_print("[DTNperf fatal error] in opening a new bp handle: %s\n", al_bp_strerror(error));
			client_clean_exit(1);
		}
		debug_print(DEBUG_L2,"done\n");

		al_bp_bundle_set_source(&bundle_force_stop, al_bp_extB_get_local_eid(registration_descriptor));

		printf("Sending the force stop bundle to the monitor...");
		error = al_bp_bundle_send(force_stop_handle, al_bp_extB_get_regid(registration_descriptor), bundle_force_stop);
		if (error != BP_SUCCESS)
		{
			error_print("[DTNperf fatal error] in sending force stop bundle: %d (%s)\n", error, al_bp_strerror(error));
			al_bp_close(force_stop_handle);

			debugger_destroy();
			al_bp_extB_destroy();
			exit(1);
		}

		al_bp_close(force_stop_handle);

	}
	else
	{ // not DTN implementation
		al_bp_endpoint_id_t none;
		al_bp_extB_error_t utility_error;
		al_bp_get_none_endpoint(&none);
		utility_error = al_bp_extB_send(registration_descriptor, bundle_force_stop, mon_eid, none);
		if (utility_error != BP_EXTB_SUCCESS && utility_error != BP_EXTB_ERRRECEIVER) {
			error_print("[DTNperf fatal error] in sending force stop bundle: %d (%s)\n", al_bp_extB_get_error(registration_descriptor), al_bp_extB_strerror(registration_descriptor));
			debugger_destroy();
			al_bp_extB_destroy();
			exit(1);
		}
	}

	printf("done.\n");
	al_bp_bundle_free(&bundle_force_stop);
}

void print_final_report(FILE * f)
{
	double goodput, sent = 0;
	struct timeval total;
	double total_secs;
	char * gput_unit, * sent_unit;
	// check param
	if (f == NULL) return;
	timersub(&end, &start, &total);
	total_secs = (((double)total.tv_sec * 1000 *1000) + (double)total.tv_usec) / (1000 * 1000);

	if (sent_data / (1000 * 1000) >= 1)
	{
		sent = (double) sent_data / (1000 * 1000);
		sent_unit = "Mbyte";
	}
	else if (sent_data / 1000 >= 1)
	{
		sent = (double) sent_data / 1000;
		sent_unit = "Kbyte";
	}
	else
		sent_unit = "byte";

	goodput = sent_data * 8 / total_secs;
	if (goodput / (1000 * 1000) >= 1)
	{
		goodput /= 1000 * 1000;
		gput_unit = "Mbit/s";
	}
	else if (goodput / 1000 >= 1)
	{
		goodput /= 1000;
		gput_unit = "Kbit/s";
	}
	else
		gput_unit = "bit/s";

	fprintf(f, "\nBundles sent = %d ", sent_bundles);
	if (perf_opt->crc==TRUE && perf_opt->congestion_ctrl == 'W')
		fprintf(f, "(Wrong CRC = %ld) ", wrong_crc);
	fprintf(f, "total data sent = %.3f %s\n", sent, sent_unit);
	fprintf(f, "Total execution time = %.1f\n", total_secs);
	if(perf_opt->congestion_ctrl == 'W')
		fprintf(f, "Goodput = %.3f %s\n", goodput, gput_unit);
	else
		fprintf(f, "Throughput = %.3f %s\n", goodput, gput_unit);
} // end print_final_report

void print_client_usage(char* progname)
{
	error_print("\n");
	error_print("dtnperf client mode\n");
	error_print("SYNTAX: %s %s -d <dest_eid> <[-T <s> | -D <num> | -F <filename>]> [-W <size> | -R <rate>] [options]\n", progname, CLIENT_STRING);
	error_print("\n");
	error_print("options:\n"
			" -d, --destination <eid>     Destination EID (i.e. server EID).\n"
			" -T, --time <seconds>        Time-mode: seconds of transmission.\n"
			" -D, --data <num[B|k|M]>     Data-mode: amount data to transmit; B = Byte, k = kByte, M = MByte. Default 'M' (MB). According to the SI and the IEEE standards 1 MB=10^6 bytes\n"
			" -F, --file <filename>       File-mode: file to transfer\n"
			" -W, --window <size>         Window-based congestion control: size of DTNperf transmission window, i.e. max number\n"
			"								of bundles \"in flight\" (not still confirmed by a server ACK). Default: 1.\n"
			" -R, --rate <rate[k|M|b]>    Rate-based congestion control: Tx rate. k = kbit/s, M = Mbit/s, b = bundle/s. Default is kb/s\n"
			" -m, --monitor <eid>         External monitor EID.\n"
			" -C, --custody               Request of custody transfer (and of \"custody accepted\" status reports as well).\n"
			" -f, --forwarded             Request of \"forwarded\" status reports.\n"
			" -r, --received              Request of \"received\" status reports.\n"
			"     --del                   Request of \"deleted\" stautus reports.\n"
			" -N, --nofragment            Disable bundle fragmentation.\n"
			" -P, --payload <size[B|k|M]> Bundle payload size; B = Byte, k = kByte, M = MByte. Default= 'k' (kB). According to the SI and the IEEE standards 1 MB=10^6 bytes.\n"
			" -M, --memory                Store the bundle into memory instead of file (if payload < 50KB).\n"
			" -L, --log[=log_filename]    Create a log file. Default log filename is %s.\n"
			"     --log-dir <dir>         Directory where the client and the internal monitor save log and the csv files. Default \"%s\"\n"
			// XXX no more supported
			//"     --ip-addr <addr>        IP address of the BP daemon api. Default is 127.0.0.1 (DTN2 and IBR-DTN only).\n"
			//"     --ip-port <port>        IP port of the BP daemon api. Default is 5010 (DTN2 and IBR-DTN only).\n"
			"     --debug[=level]         Debug messages [1-2]; if level is not indicated level = 2.\n"
			" -l, --lifetime <time>       Bundle lifetime (s). Default is 60 s.\n"
			" -p, --priority <val>        Bundle  priority [bulk|normal|expedited|reserved]. Default is normal.\n"
			"     --ack-to-mon            Force server to send ACKs in cc to the monitor (independently of server settings).\n"
			"     --no-ack-to-mon         Force server not to send  ACKs in cc to the monitor (independently of server settings).\n"
			"     --ack-lifetime <time>   ACK lifetime (value given to the server). Default is the same as bundle lifetime\n"
			"     --ack-priority <val>    ACK priority (value given to the server) [bulk|normal|expedited|reserved]. Default is the same as bundle priority\n"
			"     --no-bundle-stop        Do not send bundles stop and force-stop to the monitor. Use it only if you know what you are doing\n"
			"     --force-eid <[DTN|IPN]> Force scheme of registration EID.\n"
			"     --ipn-local <num>       Set ipn local number (Use only on DTN2)\n"
			"     --ordinal <num>         ECOS \"ordinal priority\" [0-254]. Default: 0 (ION Only).\n"
			"     --unreliable            Set ECOS \"unreliable flag\" to True. Default: False (ION Only).\n"
			"     --critical              Set ECOS \"critical flag\" to True. Default: False (ION Only).\n"
			"     --flow <num>            ECOS \"flow\" number. Default: 0 (ION Only).\n"
			//			" -n  --num-ext-blocks <val>  Number of extension/metadata blocks\n"
			"     --mb-type <type>        Include metadata block and specify type.\n"
			"     --mb-string <string>    Extension/metadata block content.\n"
			"     --crc                   Calculate (and check on the Server) CRC of bundles.\n"
			" -h, --help                  This help.\n",
			LOG_FILENAME, LOGS_DIR_DEFAULT);
	error_print("\n");

	debugger_destroy();
	al_bp_extB_destroy();

	exit(1);
} // end print_client_usage

void parse_client_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt)
{
	char c, done = 0;
	dtnperf_options_t * perf_opt = perf_g_opt->perf_opt;
	dtnperf_connection_options_t * conn_opt = perf_g_opt->conn_opt;
	boolean_t w = FALSE, r = FALSE;
	boolean_t set_ack_priority_as_bundle = TRUE;
	boolean_t set_ack_expiration_as_bundle = TRUE;
	boolean_t data_set=false;
	char * block_buf;

	while (!done)
	{
		static struct option long_options[] =
		{
				{"help", no_argument, 0, 'h'},
				{"memory", no_argument, 0, 'M'},
				{"custody", no_argument, 0, 'C'},
				{"window", required_argument, 0, 'W'},
				{"destination", required_argument, 0, 'd'},
				{"monitor", required_argument, 0, 'm'},
				//		{"exitinterval", required_argument, 0, 'i'},			// interval before exit
				{"time", required_argument, 0, 'T'},			// time mode
				{"data", required_argument, 0, 'D'},			// data mode
				{"file", required_argument, 0, 'F'},			// file mode
				{"payload", required_argument, 0, 'P'},
				{"lifetime", required_argument, 0, 'l'},
				{"rate", required_argument, 0, 'R'},
				{"debug", optional_argument, 0, 33}, 				// 33 because D is for data mode
				{"priority", required_argument, 0, 'p'},
				{"nofragment", no_argument, 0, 'N'},
				{"received", no_argument, 0, 'r'},
				{"forwarded", no_argument, 0, 'f'},
				{"log", optional_argument, 0, 'L'},				// create log file
				{"ldir", required_argument, 0, 40},
				{"ip-addr", required_argument, 0, 37},
				{"ip-port", required_argument, 0, 38},
				{"ack-to-mon", no_argument, 0, 44},			// force server to send acks to monitor
				{"no-ack-to-mon", no_argument, 0, 45},		// force server to NOT send acks to monitor
				{"ack-lifetime", required_argument, 0, 46}	,			// set server ack expiration equal to client bundles
				{"ack-priority", required_argument, 0, 47},	// set server ack priority as indicated or equal to client bundles
				{"del", no_argument, 0, 48},					//request of bundle status deleted report
				{"no-bundle-stop", no_argument, 0, 49},		// do not send bundle stop and force stop to the monitor
				{"force-eid", required_argument, 0, 50},   // force client to register with a different scheme
				{"ipn-local", required_argument, 0, 51},   // ipn local number (DTN2 only)
				{"ordinal", required_argument, 0, 52},
				{"unreliable", no_argument, 0, 53},
				{"critical", no_argument, 0, 54},
				{"flow", required_argument, 0, 55},
				//{"num-ext-blocks", required_argument, 0, 'n'},
				{"mb-type", required_argument, 0, 56},   // set metadata extension block type
				{"mb-string", required_argument, 0, 57},          // set metadata/extension block content
				{"crc", no_argument, 0, 58},
				{0, 0, 0, 0}	// The last element of the array has to be filled with zeros.

		};

		int option_index = 0;
		c = getopt_long(argc, argv, "hvMCW:d:m:i:T:D:F:P:l:R:p:NrfL::", long_options, &option_index);

		switch (c)
		{
		case 'h':
			print_client_usage(argv[0]);
			debugger_destroy();
			al_bp_extB_destroy();
			exit(0);
			return ;

		case 'M':
			perf_opt->use_file = 0;
			perf_opt->payload_type = BP_PAYLOAD_MEM;
			break;

		case 'C':
			conn_opt->custody_transfer = 1;
			conn_opt->custody_receipts = 1;
			break;

		case 'W':
			perf_opt->congestion_ctrl = 'W';
			perf_opt->window = atoi(optarg);
			w = TRUE;
			break;

		case 'd':
			memcpy(perf_opt->dest_eid, optarg, AL_BP_MAX_ENDPOINT_ID);
			break;

		case 'm':
			memcpy(perf_opt->mon_eid, optarg, AL_BP_MAX_ENDPOINT_ID);
			break;
			/*
		case 'i':
			perf_opt->wait_before_exit = atoi(optarg)*1000;
			break;
			 */
		case 'T':
			perf_opt->op_mode = 'T';
			perf_opt->transmission_time = atoi(optarg);
			break;

		case 'D':
			perf_opt->op_mode = 'D';
			perf_opt->D_arg = strdup(optarg);
			perf_opt->data_unit = find_data_unit(perf_opt->D_arg);

			switch (perf_opt->data_unit)
			{
			case 'B':
				perf_opt->data_qty = atof(perf_opt->D_arg);
				break;
			case 'k':
				perf_opt->data_qty = kilo2byte(atof(perf_opt->D_arg));
				break;
			case 'M':
				perf_opt->data_qty = mega2byte(atof(perf_opt->D_arg));
				break;
			default:
				printf("\n[DTNperf syntax error] (-D option) invalid data unit\n");
				debugger_destroy();
				al_bp_extB_destroy();
				exit(1);
				//				printf("\nWARNING: (-D option) invalid data unit, assuming 'M' (MBytes)\n\n");
				//				perf_opt->data_qty = mega2byte(atof(perf_opt->D_arg));
				//				break;
			}
			break;

			case 'F':
				perf_opt->op_mode = 'F';
				perf_opt->F_arg = strdup(optarg);
				if(!file_exists(perf_opt->F_arg))
				{
					error_print("[DTNperf syntax error] Unable to open file %s: %s\n", perf_opt->F_arg, strerror(errno));
					debugger_destroy();
					al_bp_extB_destroy();
					exit(1);
				}
				break;

			case 'P':
				perf_opt->P_arg = optarg;
				if(perf_opt->P_arg <= 0)
				{
					printf("\n[DTNperf syntax error] (-P option) invalid data value\n");
					debugger_destroy();
					al_bp_extB_destroy();
					exit(1);
				}
				perf_opt->data_unit = find_data_unit(perf_opt->P_arg);
				switch (perf_opt->data_unit)
				{
				case 'B':
					perf_opt->bundle_payload = atof(perf_opt->P_arg);
					break;
				case 'k':
					perf_opt->bundle_payload = kilo2byte(atof(perf_opt->P_arg));
					break;
				case 'M':
					perf_opt->bundle_payload = mega2byte(atof(perf_opt->P_arg));

					break;
				default:
					printf("\n[DTNperf syntax error] (-P option) invalid data unit\n");
					debugger_destroy();
					al_bp_extB_destroy();
					exit(1);
					//				printf("\nWARNING: (-p option) invalid data unit, assuming 'K' (KBytes)\n\n");
					//				perf_opt->bundle_payload = kilo2byte(atof(perf_opt->p_arg));
					//				break;
				}
				break;

				case 'l':
					conn_opt->expiration = atoi(optarg);
					break;

				case 'R':
					perf_opt->rate_arg = strdup(optarg);
					perf_opt->rate_unit = find_rate_unit(perf_opt->rate_arg);
					perf_opt->rate = atof(perf_opt->rate_arg);
					perf_opt->congestion_ctrl = 'R';
					r = TRUE;
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
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
					}
					break;

				case 'N':
					conn_opt->disable_fragmentation = TRUE;
					break;

				case 'f':
					conn_opt->forwarding_receipts = TRUE;
					break;

				case 'r':
					conn_opt->receive_receipts = TRUE;
					break;

				case 'L':
					perf_opt->create_log = TRUE;
					if (optarg != NULL)
						perf_opt->log_filename = strdup(optarg);
					break;

				case 40:
					perf_opt->logs_dir = strdup(optarg);
					break;

				case 33: // debug
					if (optarg){
						int debug = atoi(optarg);
						if(!(debug >=1 && debug <=2)){
							error_print("[DTNperf syntax error] wrong --debug argument\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						else
							perf_opt->debug=debug;
					}
					else
						perf_opt->debug = 2;
					break;

				case 34: // incoming bundle destination directory
					perf_opt->dest_dir = strdup(optarg);
					break;

				case 37:
					if(perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
					{
						error_print("[DTNperf error] --ip-addr supported only in DTN2 and IBR-DTN\n");
						debugger_destroy();
						al_bp_extB_destroy();
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
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
						return;
					}
					perf_opt->ip_port = atoi(optarg);
					perf_opt->use_ip = TRUE;
					break;


				case 44:
					perf_opt->bundle_ack_options.ack_to_mon = ATM_FORCE_YES;
					break;

				case 45:
					perf_opt->bundle_ack_options.ack_to_mon = ATM_FORCE_NO;
					break;

				case 46:
					set_ack_expiration_as_bundle = FALSE;
					perf_opt->bundle_ack_options.set_ack_expiration = TRUE;
					perf_opt->bundle_ack_options.ack_expiration = atoi(optarg);
					break;

				case 47:
					set_ack_priority_as_bundle = FALSE;
					perf_opt->bundle_ack_options.set_ack_priority = TRUE;
					perf_opt->bundle_ack_options.ack_priority.ordinal = 0;
					if (!strcasecmp(optarg, "bulk"))   {
						perf_opt->bundle_ack_options.ack_priority.priority = BP_PRIORITY_BULK;
					} else if (!strcasecmp(optarg, "normal")) {
						perf_opt->bundle_ack_options.ack_priority.priority = BP_PRIORITY_NORMAL;
					} else if (!strcasecmp(optarg, "expedited")) {
						perf_opt->bundle_ack_options.ack_priority.priority = BP_PRIORITY_EXPEDITED;
					} else if (!strcasecmp(optarg, "reserved")) {
						perf_opt->bundle_ack_options.ack_priority.priority = BP_PRIORITY_RESERVED;
					} else {
						error_print("[DTNperf syntax error] Invalid ack priority value %s\n", optarg);
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
					}
					printf("OK\n");
					break;

				case 48:
					conn_opt->deleted_receipts = TRUE;
					break;

				case 49:
					perf_opt->no_bundle_stop = TRUE;
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
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
					}
					break;

					case 51:
						perf_opt->ipn_local_num = atoi(optarg);
						if (perf_opt->ipn_local_num <= 0)
						{
							error_print("[DTNperf syntax error] wrong --ipn_local argument\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
						}
						break;

					case 52:
						if( perf_opt->bp_implementation != BP_ION){
							error_print("[DTNperf error] --ordinal supported only in ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						conn_opt->priority.ordinal = atoi(optarg);
						if(conn_opt->priority.ordinal > 254)
						{
							error_print("[DTNperf syntax error] Invalid ordinal number %u\n", conn_opt->priority.ordinal);
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						break;

					case 53:
						if( perf_opt->bp_implementation != BP_ION){
							error_print("[DTNperf error] --unreliable supported only in ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						conn_opt->unreliable = TRUE;
						break;

					case 54:
						if( perf_opt->bp_implementation != BP_ION){
							error_print("[DTNperf error] --critical supported only in ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						conn_opt->critical = TRUE;
						break;

					case 55:
						if( perf_opt->bp_implementation != BP_ION){
							error_print("[DTNperf syntax error] --flow supported only in ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						conn_opt->flow_label = atoi(optarg);
						break;
						/*		 case 'n':
		                        perf_opt->num_blocks = atoi(optarg);
		                        ext_blocks = malloc(perf_opt->num_blocks * sizeof(extension_block_info_t));
		                        break;*/
					case 56:
						if(perf_opt->bp_implementation != BP_DTN
								&& perf_opt->bp_implementation != BP_ION)
						{
							error_print("[DTNperf syntax error] --mb-type supported only in DTN2 and ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						perf_opt->num_blocks = 1;
						ext_blocks = malloc(perf_opt->num_blocks * sizeof(extension_block_info_t));
						perf_opt->metadata_type = atoi(optarg);

						if((num_meta_blocks + 1) > perf_opt->num_blocks)
						{
							error_print("[DTNperf error] Specified only %d extension/metadata blocks\n",
									perf_opt->num_blocks);
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
						}
						if (!((perf_opt->metadata_type == METADATA_TYPE_URI) ||
								((perf_opt->metadata_type >= METADATA_TYPE_EXPT_MIN) &&
										(perf_opt->metadata_type <= METADATA_TYPE_EXPT_MAX))))
						{
							error_print("-M - metadata type code is not in use - available 1, 192-255\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
						}
						ext_blocks[num_meta_blocks].block.type = METADATA_BLOCK;
						set_metadata_type(&ext_blocks[num_meta_blocks], perf_opt->metadata_type);
						//the flags refer to the bundle protocol flags and should be defined with a proper command
						ext_blocks[num_meta_blocks].block.flags=0;
						ext_blocks[num_meta_blocks].block.data.data_len=0;
						num_meta_blocks++;
						data_set = false;
						break;

					case 57:
						if(perf_opt->bp_implementation != BP_DTN
							&& perf_opt->bp_implementation != BP_ION)
						{
							error_print("[DTNperf syntax error] --mb-string supported only in DTN2 and ION\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
							return;
						}
						if ((perf_opt->num_blocks > 0) && !data_set)
						{
							block_buf = strdup(optarg);
							set_block_buf(&ext_blocks[(num_meta_blocks + num_ext_blocks) - 1],
									block_buf, strlen(block_buf));
							data_set = true;
						}
						else if (data_set)
						{
							error_print("[DTNperf warning] Ignoring duplicate data setting\n");
						}
						else
						{
							error_print("[DTNperf error] No extension or metadata block defined to receive data\n");
							debugger_destroy();
							al_bp_extB_destroy();
							exit(1);
						}
						break;

					case 58:
						perf_opt->crc=TRUE;
						break;

					case '?':
						error_print("[DTNperf error] Unknown option: %c\n", optopt);
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
						break;

					case (char)(-1):
																																																																																	done = 1;
					break;

					default:
						// getopt already prints an error message for unknown option characters
						print_client_usage(argv[0]);
						debugger_destroy();
						al_bp_extB_destroy();
						exit(1);
		} // --switch
	} // -- while

	// if mode is client and monitor eid is not specified, client node must be also monitor.
	/*if(perf_g_opt->mode == DTNPERF_CLIENT && perf_opt->mon_eid == NULL)
	{
		perf_g_opt->mode = DTNPERF_CLIENT_MONITOR;
	}*/

	// set ack-to-client request
	if (perf_opt->congestion_ctrl == 'W')
		perf_opt->bundle_ack_options.ack_to_client = TRUE;
	else
		perf_opt->bundle_ack_options.ack_to_client = FALSE;

	// set bundle ack priority as the same of bundle one
	if (set_ack_expiration_as_bundle)
		perf_opt->bundle_ack_options.ack_expiration = conn_opt->expiration;

	// set bundle ack priority as the same of bundle one
	if (set_ack_priority_as_bundle)
		perf_opt->bundle_ack_options.ack_priority = conn_opt->priority;
	// TEMP: ordinal = 0
	perf_opt->bundle_ack_options.ack_priority.ordinal = conn_opt->priority.ordinal;

#define CHECK_SET(_arg, _what)                                          				\
		if (_arg == 0) {                                                    	 		\
			error_print("\n[DTNperf syntax error] %s must be specified\n", _what);   	\
			print_client_usage(argv[0]);                                                \
			debugger_destroy(); 														\
			al_bp_extB_destroy(); 													\
			exit(1);                                                        			\
		}

	CHECK_SET(perf_opt->dest_eid[0], "destination eid");
	CHECK_SET(perf_opt->op_mode, "-T or -D or -F");

	if (w && r)
	{
		error_print("\n[DTNperf syntax error] -w and -r options are mutually exclusive\n");   	\
		print_client_usage(argv[0]);                                               				\
		debugger_destroy();																		\
		al_bp_extB_destroy();																\
		exit(1);
	}


	// check command line options
	check_options(perf_g_opt);

} // end parse_client_options

/* ----------------------------
 * check_options
 * ---------------------------- */
void check_options(dtnperf_global_options_t * global_options)
{

	dtnperf_options_t * perf_opt = global_options->perf_opt;

	// checks on values
	if ((perf_opt->op_mode == 'D') && (perf_opt->data_qty <= 0))
	{
		error_print("\n[DTNperf syntax error] (-D option) you should send a positive number of MBytes (%f)\n\n",
				perf_opt->data_qty);
		debugger_destroy();
		al_bp_extB_destroy();
		exit(2);
	}
	if ((perf_opt->op_mode == 'T') && (perf_opt->transmission_time <= 0))
	{
		error_print("\n[DTNperf syntax error] (-T option) you should specify a positive time\n\n");
		debugger_destroy();
		al_bp_extB_destroy();
		exit(2);
	}

	// checks on options combination
	if ((perf_opt->use_file) && (perf_opt->op_mode == 'T'))
	{
		if (perf_opt->bundle_payload <= ILLEGAL_PAYLOAD)
		{
			perf_opt->bundle_payload = DEFAULT_PAYLOAD;
			error_print("\n[DTNperf warning] (a): bundle payload set to %f bytes\n", perf_opt->bundle_payload);
			error_print("(use_file && op_mode=='T' + payload <= %d)\n\n", ILLEGAL_PAYLOAD);
		}
	}
	if ((perf_opt->use_file) && (perf_opt->op_mode == 'D'))
	{
		if ((perf_opt->bundle_payload <= ILLEGAL_PAYLOAD)
				|| ((perf_opt->bundle_payload > perf_opt->data_qty)	&& (perf_opt->data_qty > 0)))
		{
			perf_opt->bundle_payload = perf_opt->data_qty;
			error_print("\n[DTNperf warning] (b): bundle payload set to %f bytes\n", perf_opt->bundle_payload);
			error_print("(use_file && op_mode=='D' + payload <= %d or > %f)\n\n", ILLEGAL_PAYLOAD, perf_opt->data_qty);
		}
	}

	if (perf_opt->bundle_payload <= ILLEGAL_PAYLOAD)
	{
		perf_opt->bundle_payload = DEFAULT_PAYLOAD;
		error_print("\n[DTNperf warning]: bundle payload set to %f bytes\n\n", perf_opt->bundle_payload);

	}
	if ((!perf_opt->use_file) && (perf_opt->bundle_payload > MAX_MEM_PAYLOAD))
	{
		perf_opt->bundle_payload = MAX_MEM_PAYLOAD;
		error_print("\n[DTNperf warning]: MAX_MEM_PAYLOAD = %d\nbundle payload set to max: %f bytes\n", MAX_MEM_PAYLOAD, perf_opt->bundle_payload);
	}

	if ((!perf_opt->use_file) && (perf_opt->op_mode == 'D'))
	{
		if (perf_opt->data_qty <= MAX_MEM_PAYLOAD)
		{
			perf_opt->bundle_payload = perf_opt->data_qty;
			error_print("\n[DTNperf warning] (c1): bundle payload set to %f bytes\n", perf_opt->bundle_payload);
			error_print("(!use_file + payload <= %d + data_qty <= %d + op_mode=='D')\n\n",
					ILLEGAL_PAYLOAD, MAX_MEM_PAYLOAD);
		}
		if (perf_opt->data_qty > MAX_MEM_PAYLOAD)
		{
			perf_opt->bundle_payload = MAX_MEM_PAYLOAD;
			error_print("(!use_file + payload <= %d + data_qty > %d + op_mode=='D')\n",
					ILLEGAL_PAYLOAD, MAX_MEM_PAYLOAD);
			error_print("\n[DTNperf warning] (c2): bundle payload set to %f bytes\n\n", perf_opt->bundle_payload);
		}
	}

	if (perf_opt->window <= 0)
	{
		error_print("\n[DTNperf syntax error] (-W option) the window must be set to a posotive integer value \n\n");
		debugger_destroy();
		al_bp_extB_destroy();
		exit(2);
	}

	//if underlying implementation is DTN2 and forced scheme is "ipn", ipn-local must be set
	if (al_bp_get_implementation() == BP_DTN && perf_opt->eid_format_forced == 'I' && perf_opt->ipn_local_num <= 0)
	{
		error_print("\n[DTNperf syntax error] (--force-eid option) To use ipn registration in DTN2 implementation,"
				"you must set the local ipn number with the option --ipn-local\n\n");
		debugger_destroy();
		al_bp_extB_destroy();
		exit(2);
	}

} // end check_options

#ifdef mingw
BOOL WINAPI ConsoleHandlerFirst(DWORD dwType)
{
	switch(dwType) {
	case CTRL_C_EVENT:
		printf("\nDTNperf client received SIGINT: exiting\n");
		client_clean_exit(0);
		break;
	case CTRL_BREAK_EVENT:
		printf("\nDTNperf client received SIGINT: exiting\n");
		client_clean_exit(0);
		break;
	default:
		break;
	}
	return TRUE;
}

void ctrlc_win_handler () {
	al_bp_handle_t force_stop_handle;

	printf("\nDTNperf client received SIGINT: Exiting\n");

	// send a foce stop to the monitor to terminate it
	if (!perf_opt->no_bundle_stop)
	{ // prepare and send bundle force-stop to the monitor

		al_bp_bundle_object_t bundle_force_stop;
		// Open a new connection to BP Daemon
		debug_print(DEBUG_L1,"[debug] opening a new connection to local BP daemon...");

		if(perf_opt->bp_implementation == BP_DTN)
		{
			if (perf_opt->use_ip)
				error = al_bp_open_with_ip(perf_opt->ip_addr, perf_opt->ip_port, &force_stop_handle);
			else
				error = al_bp_open(&force_stop_handle);
		}

		if (error != BP_SUCCESS)
		{
			fprintf(stderr, "[DTNperf fatal error] in opening a new bp handle: %s\n", al_bp_strerror(error));
			client_clean_exit(1);
		}
		debug_print(DEBUG_L1,"done\n");

		// create the bundle force stop
		al_bp_bundle_create(&bundle_force_stop);
		// fill the force stop bundle
		prepare_force_stop_bundle(&bundle_force_stop, mon_eid, conn_opt->expiration, conn_opt->priority);
		al_bp_bundle_set_source(&bundle_force_stop, local_eid);
		// send force_stop bundle to monitor
		printf("Sending the force stop bundle to the monitor...");
		if(perf_opt->bp_implementation == BP_DTN)
			error = al_bp_bundle_send(force_stop_handle, regid, bundle_force_stop);
		else if(perf_opt->bp_implementation == BP_ION)
			error = al_bp_bundle_send(handle, regid, bundle_force_stop);
		else if(perf_opt->bp_implementation == BP_IBR)
			error = al_bp_bundle_send(handle, regid, bundle_force_stop);
		if ((error) != BP_SUCCESS)
		{
			fprintf(stderr, "[DTNperf fatal error] in sending force stop bundle: %d (%s)\n", error, al_bp_strerror(error));
			al_bp_close(force_stop_handle);
			exit(1);
		}
		printf("done.\n");

		if (perf_opt->bp_implementation == BP_DTN)
			al_bp_close(force_stop_handle);


		al_bp_bundle_free(&bundle_force_stop);
	}

	process_interrupted = TRUE;

	// terminate all child threads
	pthread_cancel(sender);
	pthread_cancel(cong_ctrl);
	pthread_cancel(cong_expir_timer);

	client_clean_exit(0);
}
BOOL WINAPI ConsoleHandlerSecond(DWORD dwType)
{
	switch(dwType) {
	case CTRL_C_EVENT:
		ctrlc_win_handler();
		break;
	case CTRL_BREAK_EVENT:
		ctrlc_win_handler();
		break;
	default:
		break;
	}
	return TRUE;
}
#else
void client_handler(int sig)
{
	printf("\nDTNperf client received SIGINT: exiting\n");

	client_clean_exit(0);
} // end client_handler
#endif

static void set_forced_close_client(boolean_t value)
{
	pthread_mutex_lock(&mutex_forced_close);
	forced_close = value;
	pthread_mutex_unlock(&mutex_forced_close);
}

static boolean_t get_forced_close_client()
{
	int result;
	pthread_mutex_lock(&mutex_forced_close);
	result = forced_close;
	pthread_mutex_unlock(&mutex_forced_close);
	return result;
}

void client_clean_exit(int status)
{
	printf("Dtnperf client: exit\n");

	// terminate all child threads
	//pthread_cancel(sender);
	//pthread_cancel(cong_ctrl);
	//pthread_cancel(cong_expir_timer);

	set_forced_close_client(TRUE);

	if(source_file_created)
	{
		remove(source_file);
		debug_print(DEBUG_L2, "[DEBUG_L2] removed file %s\n", source_file);
	}

	debugger_destroy();

	//al_bp_extB_unregister(registration_descriptor);
	al_bp_extB_destroy();

//	al_bp_bundle_free(&bundle);
	/*if (perf_opt->bp_implementation == BP_IBR)
		//with IBR-DTN, al_bp_close from a signal handler is blocking,
		//and not needed since the process is being terminated
		exit(status);*/

	exit(status);
} // end client_clean_exit
