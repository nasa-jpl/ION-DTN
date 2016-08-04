/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Davide Pallotti, davide.pallotti@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * dtnperf_monitor.c
 */

#include <al_bp_api.h>

#include "dtnperf_monitor.h"
#include "../includes.h"
#include "../utils.h"
#include "../bundle_tools.h"
#include "../csv_tools.h"

/*
 * Global Variables
 */

// pthread variables
pthread_t session_exp_timer;
pthread_mutex_t mutexdata;

session_list_t * session_list;
al_bp_handle_t handle;
al_bp_reg_id_t regid;
al_bp_endpoint_id_t local_eid;

// oneCSVonly flag
boolean_t oneCSVonly;

//rtPrint flag
boolean_t rtPrint;

// flags to exit cleanly
boolean_t dedicated_monitor;
boolean_t bp_handle_open;


/*  ----------------------------
 *          MONITOR CODE
 *  ---------------------------- */
void run_dtnperf_monitor(monitor_parameters_t * parameters)
{
	/* ------------------------
	 * variables
	 * ------------------------ */
	dtnperf_options_t * perf_opt = parameters->perf_g_opt->perf_opt;

	al_bp_error_t error;
	al_bp_reg_info_t reginfo;
	al_bp_bundle_object_t bundle_object;
	al_bp_bundle_status_report_t * status_report;
	al_bp_endpoint_id_t bundle_source_addr;
	al_bp_timestamp_t bundle_creation_timestamp;
	al_bp_timeval_t bundle_expiration;
	al_bp_endpoint_id_t relative_source_addr;
	al_bp_timestamp_t relative_creation_timestamp;
	HEADER_TYPE bundle_header;

	session_t * session;
	bundle_type_t bundle_type;
	struct timeval current, start;
//	struct stat file_stat;
	char * command;
	char temp[256];
	char * filename;
	int filename_len;
	char * full_filename;
	FILE * file;
	uint32_t extension_ack;

//	int stat_res;

	/* ------------------------
	 * initialize variables
	 * ------------------------ */
	boolean_t debug = perf_opt->debug;
	int debug_level = perf_opt->debug_level;

	oneCSVonly = perf_opt->oneCSVonly;
	rtPrint = perf_opt->rtPrint;

	memset(&local_eid, 0, sizeof(local_eid));
	dedicated_monitor = parameters->dedicated_monitor;
	bp_handle_open = FALSE;

	perf_opt->logs_dir = correct_dirname(perf_opt->logs_dir);

	status_report = NULL;
	session_list = session_list_create();

	// set out buffer size if daemon
	if (perf_opt->daemon)
	{
		setlinebuf(stdout);
		setlinebuf(stderr);
	}

	// create dir where dtnperf monitor will save logs
	// command should be: mkdir -p "logs_dir"
	if(debug && debug_level > 0)
		printf("[debug] initializing shell command...");
	command = malloc(sizeof(char) * (10 + strlen(perf_opt->logs_dir)));
	sprintf(command, "mkdir -p %s", perf_opt->logs_dir);
	if(debug && debug_level > 0)
		printf("done. Shell command = %s\n", command);

	// execute shell command
	if(debug && debug_level > 0)
		printf("[debug] executing shell command...");
	if (system(command) < 0)
	{
		perror("[DTNperf fatal error] in opening monitor logs dir");
		monitor_clean_exit(-1);
	}
	free(command);
	if(debug && debug_level > 0)
		printf("done\n");

	if(oneCSVonly)
		printf("Opening monitor in Unique session mode. CTRL^C to exit\n");

	// signal handlers
	signal(SIGINT, monitor_handler);
	signal(SIGUSR1, monitor_handler);
	signal(SIGUSR2, monitor_handler);

	//open the connection to the bundle protocol router
	if(debug && debug_level > 0)
		printf("[debug] opening connection to bundle protocol router...");
	if (perf_opt->use_ip)
		error = al_bp_open_with_ip(perf_opt->ip_addr, perf_opt->ip_port, &handle);
	else
		error = al_bp_open(&handle);
	if (error != BP_SUCCESS)
	{
		fflush(stdout);
		fprintf(stderr, "[DTNperf fatal error] in opening bp handle: %s\n", al_bp_strerror(error));
		monitor_clean_exit(1);
	}
	else
	{
		bp_handle_open = TRUE;
	}
	if(debug && debug_level > 0)
		printf("done\n");

	//build a local eid
	if(debug && debug_level > 0)
	{
		printf("[debug] building a local eid in format ");
		if (perf_opt->eid_format_forced == 'D' && perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
			printf("forced DTN...");
		else if (perf_opt->eid_format_forced == 'I' && perf_opt->bp_implementation != BP_ION)
			printf("forced IPN...");
		else
			printf("standard...");
	}
	if(perf_opt->bp_implementation == BP_ION && (perf_opt->eid_format_forced == 'N' || perf_opt->eid_format_forced == 'I'))
		// Use ION implementation with standard eid scheme
		error = al_bp_build_local_eid(handle, &local_eid, MON_EP_NUM_SERVICE, CBHE_SCHEME);
	else if(perf_opt->bp_implementation == BP_DTN && (perf_opt->eid_format_forced == 'N' || perf_opt->eid_format_forced == 'D'))
		// Use DTN2 implementation with standard eid scheme
	{
		if (parameters->dedicated_monitor)
			sprintf(temp, "%s_%d", MON_EP_STRING, parameters->client_id);
		else
			sprintf(temp, "%s", MON_EP_STRING);
		error = al_bp_build_local_eid(handle, &local_eid, temp, DTN_SCHEME);
	}
	else if(perf_opt->bp_implementation == BP_IBR && (perf_opt->eid_format_forced == 'N' || perf_opt->eid_format_forced == 'D'))
		// Use IBR-DTN implementation with standard eid scheme
	{
		if (parameters->dedicated_monitor)
			sprintf(temp, "%s_%d", MON_EP_STRING, parameters->client_id);
		else
			sprintf(temp, "%s", MON_EP_STRING);
		error = al_bp_build_local_eid(handle, &local_eid, temp, DTN_SCHEME);
	}
	else if(perf_opt->bp_implementation == BP_ION && perf_opt->eid_format_forced == 'D')
		// Use ION implementation with forced DTN scheme
	{
		if (parameters->dedicated_monitor)
			sprintf(temp, "%s_%d", MON_EP_STRING, parameters->client_id);
		else
			sprintf(temp, "%s", MON_EP_STRING);
		error = al_bp_build_local_eid(handle, &local_eid, temp, DTN_SCHEME);
	}
	else if(perf_opt->bp_implementation == BP_DTN && perf_opt->eid_format_forced == 'I')
		// Use DTN2 implementation with forced IPN scheme
	{
		//in this case the api al_bp_build_local_eid() wants ipn_local_number.service_number
		sprintf(temp, "%d.%s", perf_opt->ipn_local_num, MON_EP_NUM_SERVICE);
		error = al_bp_build_local_eid(handle, &local_eid, temp, CBHE_SCHEME);
	}
	else if (perf_opt->bp_implementation == BP_IBR && perf_opt->eid_format_forced == 'I')
		// Use IBR-DTN implementation with forced IPN scheme
	{
		error = al_bp_build_local_eid(handle, &local_eid, MON_EP_NUM_SERVICE, CBHE_SCHEME);
	}

	if(debug && debug_level > 0)
		printf("done\n");

	if (debug)
		printf("local_eid = %s\n", local_eid.uri);
	if (error != BP_SUCCESS)
	{
		fflush(stdout);
		fprintf(stderr, "[DTNperf fatal error] in building local EID: '%s'\n", al_bp_strerror(error));
		monitor_clean_exit(1);
	}

	// checking if there is already a registration
	if(debug && debug_level > 0)
		printf("[debug] checking for existing registration...");
	error = al_bp_find_registration(handle, &local_eid, &regid);
	if ( (error == BP_SUCCESS && perf_opt->bp_implementation == BP_DTN)
			|| (perf_opt->bp_implementation == BP_ION && (error == BP_EBUSY || error == BP_EPARSEEID))
			|| (perf_opt->bp_implementation == BP_IBR && error == BP_SUCCESS))
	{
		fflush(stdout);
		fprintf(stderr, "[DTNperf fatal error] existing a registration with the same eid.\n");
		fprintf(stderr, "regid 0x%x\n", (unsigned int) regid);
		monitor_clean_exit(1);
	}
	if ((debug) && (debug_level > 0))
		printf(" done\n");

	//create a new registration to the local router based on this eid
	if(debug && debug_level > 0)
		printf("[debug] registering to local daemon...");
	memset(&reginfo, 0, sizeof(reginfo));
	al_bp_copy_eid(&reginfo.endpoint, &local_eid);
	reginfo.flags = BP_REG_DEFER;
	reginfo.regid = BP_REGID_NONE;
	reginfo.expiration = 0;
	if ((error = al_bp_register(&handle, &reginfo, &regid)) != 0)
	{
		fflush(stdout);
		fprintf(stderr, "[DTNperf fatal error] in creating registration: %d (%s)\n",
				error, al_bp_strerror(al_bp_errno(handle)));
		monitor_clean_exit(1);
	}
	if ((debug) && (debug_level > 0))
		printf(" done\n");
	if (debug)
		printf("regid 0x%x\n", (unsigned int) regid);

	// start expiration timer thread
	if (!oneCSVonly) //no need of timer in case of unique session
	{
		pthread_mutex_init (&mutexdata, NULL);
		pthread_create(&session_exp_timer, NULL, session_expiration_timer, (void *) parameters);
	}

	// start infinite loop
	while(1)
	{
		// reset variables
		bundle_type = NONE;

		// create a bundle object
		if ((debug) && (debug_level > 0))
			printf("[debug] initiating memory for bundles...\n");
		error = al_bp_bundle_create(&bundle_object);
		if (error != BP_SUCCESS)
		{
			fflush(stdout);
			fprintf(stderr, "[DTNperf fatal error] in initiating memory for bundles: %s\n", al_bp_strerror(error));
			monitor_clean_exit(1);
		}
		if(debug && debug_level > 0)
			printf("done\n");


		// wait until receive a bundle
		if ((debug) && (debug_level > 0))
			printf("[debug] waiting for bundles...\n");
		error = al_bp_bundle_receive(handle, bundle_object, BP_PAYLOAD_MEM, -1);
		if(error == BP_ERECVINT || error == BP_ETIMEOUT)
		{
			if(error == BP_ERECVINT )
				fprintf(stderr, "[DTNperf warning] bundle reception interrupted\n");
			if(error == BP_ETIMEOUT )
				fprintf(stderr, "[DTNperf warning] bundle reception timeout expired\n");

			// free memory for bundle
			al_bp_bundle_free(&bundle_object);
		}
		else
		{
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				fprintf(stderr, "[DTNperf fatal error] in getting recv reply: %d (%s)\n",
						error, al_bp_strerror(al_bp_errno(handle)));
				monitor_clean_exit(1);
			}
			if ((debug) && (debug_level > 0))
				printf(" bundle received\n");

			// mark current time
			if ((debug) && (debug_level > 0))
				printf("[debug] marking time...");
			gettimeofday(&current, NULL);
			if ((debug) && (debug_level > 0))
				printf(" done\n");

			// get SOURCE eid
			if ((debug) && (debug_level > 0))
				printf("[debug]\tgetting source eid...");
			error = al_bp_bundle_get_source(bundle_object, &bundle_source_addr);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				fprintf(stderr, "[DTNperf fatal error] in getting bundle source eid: %s\n",
						al_bp_strerror(error));
				monitor_clean_exit(1);
			}
			if ((debug) && (debug_level > 0))
			{
				printf(" done:\n");
				printf("\tbundle_source_addr = %s\n", bundle_source_addr.uri);
				printf("\n");
			}

			// get bundle CREATION TIMESTAMP
			if ((debug) && (debug_level > 0))
				printf("[debug]\tgetting bundle creation timestamp...");
			error = al_bp_bundle_get_creation_timestamp(bundle_object, &bundle_creation_timestamp);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				fprintf(stderr, "[DTNperf fatal error] in getting bundle creation timestamp: %s\n",
						al_bp_strerror(error));
				monitor_clean_exit(1);
			}
			if ((debug) && (debug_level > 0))
			{
				printf(" done:\n");
				printf("\tbundle creation timestamp:\n"
						"\tsecs = %d\n\tseqno= %d\n",
						(int)bundle_creation_timestamp.secs, (int)bundle_creation_timestamp.seqno);
				printf("\n");
			}

			// get bundle EXPIRATION TIME only in DTN2 and IBR-DTN
			if(perf_opt->bp_implementation == BP_DTN || perf_opt->bp_implementation == BP_IBR)
			{
				if ((debug) && (debug_level > 0))
				printf("[debug]\tgetting bundle expiration time...");

				error = al_bp_bundle_get_expiration(bundle_object, &bundle_expiration);
				if (error != BP_SUCCESS)
				{
					fflush(stdout);
					fprintf(stderr, "[DTNperf fatal error] in getting bundle expiration time: %s\n",
							al_bp_strerror(error));
					monitor_clean_exit(1);
				}
				if ((debug) && (debug_level > 0))
				{
					printf(" done:\n");
					printf("\tbundle expiration: %u\n", bundle_expiration);
					printf("\n");
				}
			}

			// check if bundle is a status report
			if ((debug) && (debug_level > 0))
				printf("[debug] check if bundle is a status report...\n");
			error = al_bp_bundle_get_status_report(bundle_object, &status_report);
			if (error != BP_SUCCESS)
			{
				fflush(stdout);
				fprintf(stderr, "[DTNperf fatal error] in checking if bundle is a status report: %d (%s)\n",
						error, al_bp_strerror(al_bp_errno(handle)));
				continue;
			}
			if ((debug) && (debug_level > 0))
				printf(" %s\n", status_report == NULL ? "no" : "yes");

			// check for other bundle types
			if (status_report != NULL)
				bundle_type = STATUS_REPORT;
			else
			{
				get_bundle_header_and_options(&bundle_object, & bundle_header, NULL);
				if (bundle_header == FORCE_STOP_HEADER)
				{
					bundle_type = CLIENT_FORCE_STOP;
					if ((debug) && (debug_level > 0))
						printf("[debug] Monitor: bundle force stop arrived \n");
				}
				else if (bundle_header == STOP_HEADER)
				{
					bundle_type = CLIENT_STOP;
					if ((debug) && (debug_level > 0))
						printf("[debug] Monitor: bundle stop arrived\n");
				}
				else if (bundle_header == DSA_HEADER)
				{
					bundle_type = SERVER_ACK;
					if ((debug) && (debug_level > 0))
						printf("[debug] Monitor: server ack arrived\n");
				}
				else // unknown bundle type
				{
					fprintf(stderr, "[DTNperf warning] unknown bundle type\n");
					continue;
				}
			}

			// retrieve or open log file
			pthread_mutex_lock(&mutexdata);

			session = NULL;
			extension_ack=0;
			switch (bundle_type)
			{
			case STATUS_REPORT:
				al_bp_copy_eid(&relative_source_addr, &(status_report->bundle_id.source));
				relative_creation_timestamp = status_report->bundle_id.creation_ts;
				if (rtPrint)
					printRealtimeStatusReport(perf_opt->rtPrintFile, bundle_source_addr, status_report);
				break;

			case SERVER_ACK:
				get_info_from_ack(&bundle_object, &relative_source_addr, &relative_creation_timestamp, &extension_ack);
				break;

			case CLIENT_STOP:
			case CLIENT_FORCE_STOP:
				al_bp_copy_eid(&relative_source_addr, &bundle_source_addr);
				relative_creation_timestamp = bundle_creation_timestamp;
				break;

			default:
				break;
			}

			session = session_get(session_list, relative_source_addr);

			if (session == NULL) // start a new session
			{
				// mark start time
				start = current;
				//if oneCSVonly (unique session)
				if (oneCSVonly)
				{
					sprintf(temp, "%u_%s", relative_creation_timestamp.secs, perf_opt->uniqueCSVfilename);
					full_filename = (char *) malloc(strlen(perf_opt->logs_dir) + strlen(temp) + 2);
					sprintf(full_filename, "%s/%s", perf_opt->logs_dir, temp);
				}
				else //normal sessions behavior
				{	//if source eid of Status Report is CBHE Format
					if(strncmp(relative_source_addr.uri,"ipn",3) == 0)
					{
						filename_len = strlen(relative_source_addr.uri) - strlen("ipn:") + 15;
					}
					else
					{
						filename_len = strlen(relative_source_addr.uri) - strlen("dtn://") + 15;
					}
					filename = (char *) malloc(filename_len);
					memset(filename, 0, filename_len);
					sprintf(filename, "%u_", relative_creation_timestamp.secs);
					strncpy(temp, relative_source_addr.uri, strlen(relative_source_addr.uri) + 1);

					if(strncmp(relative_source_addr.uri,"ipn",3) == 0)
					{
						strtok(temp, ":");
						strcat(filename, strtok(NULL, "\0"));
					}
					else
					{
						char * ptr;
						strtok(temp, "/");
						strcat(filename, strtok(NULL, "/"));
						// remove .dtn suffix from the filename
						ptr = strstr(filename, ".dtn");
						if (ptr != NULL)
							ptr[0] = '\0';
					}
					strcat(filename, ".csv");
					full_filename = (char *) malloc(strlen(perf_opt->logs_dir) + strlen(filename) + 2);
					sprintf(full_filename, "%s/%s", perf_opt->logs_dir, filename);
				}

				//open file in append mode
				file = fopen(full_filename, "a");
				if (oneCSVonly) //unique session, unique file
					session = unique_session_create(full_filename, file, start,
							relative_creation_timestamp.secs);
				else // standard behavior
					session = session_create(relative_source_addr, full_filename, file, start,
						relative_creation_timestamp.secs, bundle_expiration);
				session_put(session_list, session);

				// write header in csv log file
				fprintf(file,"RX_TIME;Report_SRC;Report_TST;Report_SQN;"
								"Report_Type;Bndl_SRC;Bndl_TST;Bndl_SQN;"
								"Bndl_FO;Bndl_FL;");

				csv_print_status_report_timestamps_header(file);

				csv_print(file, "CRC");

				csv_end_line(file);

				session->wrong_crc=0;

			}

			// update session infos
			session->last_bundle_time = bundle_creation_timestamp.secs;
			if(perf_opt->bp_implementation == BP_ION)
			{
				session->expiration = perf_opt->expiration_session;
			}
			else
			{
				if(perf_opt->expiration_session > bundle_expiration)
					session->expiration = bundle_expiration;
			}
			if ((debug) && (debug_level > 0))
				printf("[debug] session expiration = %u s\n", session->expiration);

			file = session->file;
			memcpy(&start, session->start, sizeof(struct timeval));

			if (bundle_type == STATUS_REPORT && (status_report->flags & BP_STATUS_DELIVERED))
			{
				session->delivered_count++;
			}

			if (bundle_type == SERVER_ACK)
			{
				if (extension_ack & BO_CRC_ENABLED)
					session->wrong_crc++;
			}

			pthread_mutex_unlock(&mutexdata);

			// print rx time in csv log
			csv_print_rx_time(file, current, start);

			// print bundle source in csv log
			csv_print_eid(file, bundle_source_addr);

			//print bundle creation timestamp in csv log
			csv_print_timestamp(file, bundle_creation_timestamp);

			// print bundle type in csv log
			switch (bundle_type)
			{
			case CLIENT_STOP:
				csv_print(file, "CLIENT_STOP");
				break;
			case CLIENT_FORCE_STOP:
				csv_print(file, "CLIENT_FORCE_STOP");
				break;
			case SERVER_ACK:
				csv_print(file, "SERVER_ACK");
				break;
			case STATUS_REPORT:
				csv_print(file, "STATUS_REPORT");
				break;
			default:
				csv_print(file, "UNKNOWN");
				break;
			}

			// print relative source and timestamp
			if (bundle_type == SERVER_ACK || bundle_type == STATUS_REPORT)
			{
				csv_print_eid(file, relative_source_addr);
				csv_print_timestamp(file, relative_creation_timestamp);
			}
			else
				csv_print(file, " ; ;");

			// print status report infos in csv log
			if (bundle_type == STATUS_REPORT)
			{
				csv_print_ulong(file, status_report->bundle_id.frag_offset);
				csv_print_ulong(file, status_report->bundle_id.orig_length);
				csv_print_status_report_timestamps(file, * status_report);
			}
			else
				csv_print(file, " ; ; ; ; ; ; ; ;");


			if (bundle_type == SERVER_ACK)
			{
				if (extension_ack & BO_CRC_ENABLED)
					csv_print(file, "1;");
				else
					csv_print(file, "0;");
			} 
			else
				csv_print(file, " ;");

			// end line in csv log
			csv_end_line(file);

			// close file (only if monitor is not in unique session mode)
			if (bundle_type == CLIENT_STOP && !oneCSVonly)
			{
				int total_to_receive;
				get_info_from_stop(&bundle_object, &total_to_receive);
				pthread_mutex_lock(&mutexdata);
				session->total_to_receive = total_to_receive;
				if(perf_opt->bp_implementation == BP_ION)
					session->wait_after_stop = perf_opt->expiration_session;
				else
					session->wait_after_stop = bundle_expiration;
				gettimeofday(session->stop_arrival_time, NULL);
				if ((debug) && (debug_level > 0))
					printf("[debug] bundle stop arrived: closing session in %u s MAX\n", session->wait_after_stop);
				pthread_mutex_unlock(&mutexdata);
			}
			else if (bundle_type == CLIENT_FORCE_STOP && !oneCSVonly)
			{
				printf("DTNperf monitor: received forced end session bundle\n");
				session_close(session_list, session);
			}

			// free memory for bundle
			al_bp_bundle_free(&bundle_object);
		}

	} // end loop

	session_list_destroy(session_list);
	al_bp_close(handle);
	if(parameters->dedicated_monitor == TRUE)
		al_bp_unregister(handle,regid,local_eid);
	bp_handle_open = FALSE;
}
// end monitor code

// session expiration timer thread
void * session_expiration_timer(void * opt)
{
	u32_t current_dtn_time;
	struct timeval current_time;
	session_t * session, * next;

	while(1)
	{
		current_dtn_time = get_current_dtn_time();
		gettimeofday(&current_time, NULL);
		boolean_t close = FALSE;

		pthread_mutex_lock(&mutexdata);

		for(session = session_list->first; session != NULL; session = next)
		{
			next = session->next;
			// all status reports has been received: close session
			if (session->total_to_receive > 0 && session->delivered_count == session->total_to_receive &&
					// wait 3 seconds before closing the session
					session->stop_arrival_time->tv_sec + 3 < current_time.tv_sec)
			{
				// close monitor if dedicated
				if (dedicated_monitor)
				{
					close = TRUE;
				}
				else
				{
					session_close(session_list, session);
				}
			}

			// stop bundle arrived but not all the status reports have arrived and the timer has expired
			else if (session->stop_arrival_time->tv_sec != 0 &&
					session->total_to_receive > 0 && session->stop_arrival_time->tv_sec + session->wait_after_stop < current_time.tv_sec)
			{
				fprintf(stdout, "DTNperf monitor: Session Expired: Bundle stop arrived, but not all the status reports did\n");

				// close monitor if dedicated
				if (dedicated_monitor)
				{
					close = TRUE;
				}
				else
				{
					fprintf(stdout, "\tsaved log file: %s\n\n", session->full_filename);
					if (fclose(session->file) < 0)
						perror("[DTNperf warning] closing expired file:");
					session_del(session_list, session);
				}
			}
			// stop bundle is not yet arrived and the last bundle has expired
			else if (session->stop_arrival_time->tv_sec == 0 &&
					session->last_bundle_time + session->expiration + 2 < current_dtn_time && (session->last_bundle_time + session->expiration != 0))
			{
				fprintf(stdout, "DTNperf monitor: Session Expired: Bundle stop did not arrive\n");

				// close monitor if dedicated
				if (dedicated_monitor)
				{
					close = TRUE;
				}
				else
				{
					fprintf(stdout,"\tsaved log file: %s\n\n", session->full_filename);
					if (fclose(session->file) < 0)
						perror("[DTNperf warning] closing expired file:");
					session_del(session_list, session);
				}
			}
		}
		pthread_mutex_unlock(&mutexdata);
		sched_yield();

		// exit if dedicated monitor
		if (close)
		{
			kill(getpid(), SIGUSR2);
			break;
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

void monitor_clean_exit(int status)
{
	session_t * session;

	// terminate all child thread
	if (!oneCSVonly)
		pthread_cancel(session_exp_timer);

	// close all log files and delete all sessions
	if (dedicated_monitor)
	{
		session = session_list->first;
		session_close(session_list, session);
	}
	else
	{
		while (session_list->first != NULL)
		{
			session = session_list->first;
			session_close(session_list, session);
		}
	}

	session_list_destroy(session_list);

	//with IBR-DTN, al_bp_close from a signal handler is blocking, 
	//and not needed since the process is being terminated
	if (al_bp_get_implementation() != BP_IBR) //perf_opt is not available here
	{
		// close bp_handle
		if (bp_handle_open)
		{
			al_bp_close(handle);
			//al_bp_unregister(handle,regid,local_eid);
		}
	}
	printf("Dtnperf Monitor: exit.\n");
	exit(status);
}

void session_close(session_list_t * list, session_t * session)
{
	fclose(session->file);
	fprintf(stdout, "DTNperf monitor: saved log file: %s\n\n", session->full_filename);
	session_del(session_list, session);
}

void print_monitor_usage(char * progname)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "DtnPerf3 monitor mode\n");
	fprintf(stderr, "SYNTAX: %s %s [options]\n", progname, MONITOR_STRING);
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n"
			" -a, --daemon                  Start the monitor as a daemon. Output is redirected to %s.\n"
			" -o, --output <file>           Change the default output file (Only with -a option).\n"
			" -s, --stop                    Stop the monitor daemon.\n"
			" -e, --session-expiration <s>  Max idle time of log files (s). Default: 120.\n"
			"     --ip-addr <addr>          Ip address of the bp daemon api. Default: 127.0.0.1 (DTN2 and IBR-DTN only)\n"
			"     --ip-port <port>          Ip port of the bp daemon api. Default: 5010 (DTN2 and IBR-DTN only)\n"
			"     --force-eid <[DTN|IPN]    Force the scheme of the registration EID.\n"
			"     --ipn-local <num>         Set the ipn local number (Use only with --force-eid IPN on DTN2)\n"
			"     --ldir <dir>              Logs directory. Default: %s .\n"
			"     --oneCSVonly              Generate a unique csv file\n"
			"     --rt-print[=filename]     Print realtime human readable status report information\n"
			"                               Print to stdout if filename is omitted \n"
			"     --debug[=level]           Debug messages [0-2], if level is not indicated level = 1.\n"
			" -v, --verbose                 Print information messages during execution.\n"
			" -h, --help                    This help.\n",
			MONITOR_OUTPUT_FILE, LOGS_DIR_DEFAULT);
	fprintf(stderr, "\n");
	exit(1);
}

void parse_monitor_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt)
{
	char c, done = 0;
	boolean_t output_set = FALSE;
	dtnperf_options_t * perf_opt = perf_g_opt->perf_opt;
	// kill daemon variables
	int pid;
	char cmd[256];
	FILE *f;

		while (!done)
		{
			static struct option long_options[] =
			{
					{"help", no_argument, 0, 'h'},
					{"verbose", no_argument, 0, 'v'},
					{"debug", optional_argument, 0, 33},
					{"ldir", required_argument, 0, 40},
					{"ip-addr", required_argument, 0, 37},
					{"ip-port", required_argument, 0, 38},
					{"force-eid", required_argument, 0, 50},
					{"ipn-local", required_argument, 0, 51},
					{"oneCSVonly", no_argument, 0, 52},
					{"rt-print", optional_argument, 0, 53},
					{"session-expiration", required_argument, 0,'e'},
					{"daemon", no_argument, 0, 'a'},
					{"output", required_argument, 0, 'o'},
					{"stop", no_argument, 0, 's'},
					{0,0,0,0}	// The last element of the array has to be filled with zeros.

			};
			int option_index = 0;
			c = getopt_long(argc, argv, "hve:ao:s", long_options, &option_index);

			switch (c)
			{
			case 'h':
				print_monitor_usage(argv[0]);
				exit(0);
				return ;

			case 'v':
				perf_opt->verbose = TRUE;
				break;

			case 'e':
				perf_opt->expiration_session = atoi(optarg);
				break;

			case 33: // debug
				perf_opt->debug = TRUE;
				if (optarg != NULL){
					int debug_level = atoi(optarg);
					if (debug_level >= 1 && debug_level <= 2)
						perf_opt->debug_level = atoi(optarg) -1;
					else {
						fprintf(stderr, "[DTNperf syntax error] wrong --debug argument\n");
						exit(1);
						return;
					}
				}
				else
					perf_opt->debug_level = 2;
				break;

			case 37:
				if(perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
				{
					fprintf(stderr, "[DTNperf error] --ip-addr supported only in DTN2 and IBR-DTN\n");
					exit(1);
					return;
				}
				perf_opt->ip_addr = strdup(optarg);
				perf_opt->use_ip = TRUE;
				break;

			case 38:
				if(perf_opt->bp_implementation != BP_DTN && perf_opt->bp_implementation != BP_IBR)
				{
					fprintf(stderr, "[DTNperf syntax error] --ip-port supported only in DTN2 and IBR-DTN\n");
					exit(1);
					return;
				}
				perf_opt->ip_port = atoi(optarg);
				perf_opt->use_ip = TRUE;
				break;

			case 40:
				perf_opt->logs_dir = strdup(optarg);
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
						fprintf(stderr, "[DTNperf syntax error] wrong --force-eid argument\n");
						exit(1);
				}
				break;

			case 51:
				perf_opt->ipn_local_num = atoi(optarg);
				if (perf_opt->ipn_local_num <= 0)
				{
					fprintf(stderr, "[DTNperf syntax error] wrong --ipn_local argument\n");
					exit(1);
				}
				break;

			case 52:
				perf_opt->oneCSVonly = TRUE;
				break;

			case 53:
				perf_opt->rtPrint = TRUE;
				if (optarg == NULL)
					break;
				f = fopen(optarg, "w+");
				if (f == NULL)
				{
					fprintf(stderr, "[DTNperf error] impossible to open file %s: %s\n", optarg, strerror(errno));
				}
				else
					perf_opt->rtPrintFile = f;
				break;

			case 'a':
				perf_opt->daemon = TRUE;
				break;

			case 'o':
				perf_opt->monitor_output_file = strdup(optarg);
				output_set = TRUE;
				break;

			case 's':
				memset(cmd, 0, sizeof(cmd));
				sprintf(cmd, "%s %s", argv[0], MONITOR_STRING);
				pid = find_proc(cmd);
				if (pid)
				{
					printf("Closing dtnperf monitor pid: %d\n", pid);
					kill(pid, SIGINT);
				}
				else
				{
					fprintf(stderr, "[DTNperf error] cannot find a running instance of dtnperf monitor\n");
				}
				exit(0);
				break;

			case '?':
				break;

			case (char)(-1):
				done = 1;
				break;

			default:
				// getopt already prints an error message for unknown option characters
				print_monitor_usage(argv[0]);
				exit(1);
			}
		}
		if (output_set && !perf_opt->daemon)
		{
			fprintf(stderr, "\n[DTNperf syntax error] -o option can be used only with -a option\n");   \
			print_monitor_usage(argv[0]);                                               \
			exit(1);
		}
}

session_list_t * session_list_create()
{
	session_list_t * list;
	list = (session_list_t *) malloc(sizeof(session_list_t));
	list->first = NULL;
	list->last = NULL;
	list->count = 0;
	return list;
}
void session_list_destroy(session_list_t * list)
{
	free(list);
}

session_t * session_create(al_bp_endpoint_id_t client_eid, char * full_filename, FILE * file, struct timeval start,
		u32_t bundle_timestamp_secs, u32_t bundle_expiration_time)
{
	session_t * session;
	session = (session_t *) malloc(sizeof(session_t));
	session->start = (struct timeval *) malloc(sizeof(struct timeval));
	session->stop_arrival_time = (struct timeval *) malloc(sizeof(struct timeval));
	session->stop_arrival_time->tv_sec = 0;
	session->stop_arrival_time->tv_usec = 0;
	al_bp_copy_eid(&(session->client_eid), &client_eid);
	session->full_filename = strdup(full_filename);
	session->file = file;
	memcpy(session->start, &start, sizeof(struct timeval));
	session->last_bundle_time = bundle_timestamp_secs;
	session->expiration = bundle_expiration_time;
	session->delivered_count = 0;
	session->total_to_receive = 0;
	session->wait_after_stop = 0;
	session->next = NULL;
	session->prev = NULL;
	return session;
}

session_t * unique_session_create(char * full_filename, FILE * file, struct timeval start, u32_t bundle_timestamp_secs)
{
	session_t * session;
	session = (session_t *) malloc(sizeof(session_t));
	session->start = (struct timeval *) malloc(sizeof(struct timeval));
	session->stop_arrival_time = (struct timeval *) malloc(sizeof(struct timeval));
	session->full_filename = strdup(full_filename);
	session->file = file;
	memcpy(session->start, &start, sizeof(struct timeval));
	session->last_bundle_time = bundle_timestamp_secs;
	session->expiration = 0; //unique session never expires
	session->delivered_count = 0;
	session->total_to_receive = 0; //this is useless
	session->wait_after_stop = 0; //this is useless
	session->next = NULL;
	session->prev = NULL;
	return session;
}

void session_destroy(session_t * session)
{
	free(session->start);
	free(session->stop_arrival_time);
	free(session->full_filename);
	free(session);
}

void session_put(session_list_t * list, session_t * session)
{
	if (list->first == NULL) // empty list
	{
		list->first = session;
		list->last = session;
	}
	else
	{
		session->prev = list->last;
		list->last->next = session;
		list->last = session;
	}
	list->count ++;

}

session_t *  session_get(session_list_t * list, al_bp_endpoint_id_t client)
{
	session_t * item = list->first;
	// if oneCSVonly return the unique session (NULL if it isn't created yet)
	if (oneCSVonly)
		return item;

	while (item != NULL)
		{
			if (strcmp(item->client_eid.uri, client.uri) == 0)
			{
				return item;
			}
			item = item->next;
		}
		return NULL;
}

void session_del(session_list_t * list, session_t * session)
{
	if (session->next == NULL && session->prev == NULL) // unique element of the list
	{
		list->first = NULL;
		list->last = NULL;
	}
	else if (session->next == NULL) // last item of list
	{
		list->last = session->prev;
		session->prev->next = NULL;
	}
	else if (session->prev == NULL) // first item of the list
	{
		list->first = session->next;
		session->next->prev = NULL;
	}
	else // generic element of list
	{
		session->next->prev = session->prev;
		session->prev->next = session->next;
	}
	session_destroy(session);
	list->count --;

}

void printRealtimeStatusReport(FILE *f, al_bp_endpoint_id_t sr_source, al_bp_bundle_status_report_t * status_report)
{
	if (status_report->flags & BP_STATUS_RECEIVED)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_RECEIVED, status_report->receipt_ts);
	if (status_report->flags & BP_STATUS_CUSTODY_ACCEPTED)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_CUSTODY_ACCEPTED, status_report->custody_ts);
	if (status_report->flags & BP_STATUS_FORWARDED)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_FORWARDED, status_report->forwarding_ts);
	if (status_report->flags & BP_STATUS_DELIVERED)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_DELIVERED, status_report->delivery_ts);
	if (status_report->flags & BP_STATUS_DELETED)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_DELETED, status_report->deletion_ts);
	if (status_report->flags & BP_STATUS_ACKED_BY_APP)
		printSingleRealtimeStatusReport(f, sr_source, status_report, BP_STATUS_ACKED_BY_APP, status_report->ack_by_app_ts);
	fflush(f);

}
void printSingleRealtimeStatusReport(FILE *f, al_bp_endpoint_id_t sr_source, al_bp_bundle_status_report_t * status_report,
		al_bp_status_report_flags_t type, al_bp_timestamp_t timestamp)
{
	char bundle_id[256];
	bundle_id_sprintf(bundle_id, &(status_report->bundle_id));
	fprintf(f, "SR %s at %u, from node %s, referring to bundle (%s) %s\n", al_bp_status_report_flag_to_str(type),
			timestamp.secs, sr_source.uri, bundle_id,
			al_bp_status_report_reason_to_str(status_report->reason));
}

void monitor_handler(int signo)
{
	if (dedicated_monitor)
	{
		if (signo == SIGUSR1)
		{
			printf("\nDtnperf monitor: received signal from client. Exiting\n");
			monitor_clean_exit(0);
		}
		else if (signo == SIGUSR2)
		{
			monitor_clean_exit(0);
		}
	}
	else
	{
		if (signo == SIGINT)
		{
			printf("\nDtnperf monitor: received SIGINT. Exiting\n");
			monitor_clean_exit(0);
		}
	}
}
