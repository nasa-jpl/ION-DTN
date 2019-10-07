/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * csv_tools.c
 */

#include "utils.h"
#include "csv_tools.h"
#include <al_bp_api.h>

void csv_print_rx_time(FILE * file, struct timeval time, struct timeval start_time)
{
	struct timeval * result = malloc(sizeof(struct timeval));
	char buf[50];
	sub_time(time, start_time, result);
	sprintf(buf, "%ld.%ld;", result->tv_sec, result->tv_usec);
	fwrite(buf, strlen(buf), 1, file);
}

void csv_print_eid(FILE * file, al_bp_endpoint_id_t eid)
{
	char buf[257];
	sprintf(buf, "%s;", eid.uri);
	fwrite(buf, strlen(buf), 1, file);
}

void csv_print_timestamp(FILE * file, al_bp_timestamp_t timestamp)
{
	char buf[50];
	sprintf(buf, "%u;%u;", timestamp.secs, timestamp.seqno);
	fwrite(buf, strlen(buf), 1, file);
}

void csv_print_status_report_timestamps_header(FILE * file)
{
	char buf[300];
	memset(buf, 0, 300);
	strcat(buf, "Dlv;");
	strcat(buf, "Ct;");
	strcat(buf, "Rcv;");
	strcat(buf, "Fwd;");
	strcat(buf, "Del;");

	// not useful for now
	// strcat(buf, "ACKED_BY_APP_TIMESTAMP");

	// status report reason
	strcat(buf, "Reason;");

	fwrite(buf, strlen(buf), 1, file);
}
void csv_print_status_report_timestamps(FILE * file, al_bp_bundle_status_report_t status_report)
{
	char buf1[256];
	char buf2[50];
	memset(buf1, 0, 256);

	if (status_report.flags & BP_STATUS_DELIVERED)
		sprintf(buf2, "%u;", status_report.delivery_ts.secs);
	else
		sprintf(buf2, " ; ");
	strcat(buf1, buf2);

	if (status_report.flags & BP_STATUS_CUSTODY_ACCEPTED)
		sprintf(buf2, "%u;", status_report.custody_ts.secs);
	else
		sprintf(buf2, " ;");
	strcat(buf1, buf2);

	if (status_report.flags & BP_STATUS_RECEIVED)
		sprintf(buf2, "%u;", status_report.receipt_ts.secs);
	else
		sprintf(buf2, " ;");
	strcat(buf1, buf2);

	if (status_report.flags & BP_STATUS_FORWARDED)
		sprintf(buf2, "%u;", status_report.forwarding_ts.secs);
	else
		sprintf(buf2, " ;");
	strcat(buf1, buf2);

	if (status_report.flags & BP_STATUS_DELETED)
		sprintf(buf2, "%u;", status_report.deletion_ts.secs);
	else
		sprintf(buf2, " ;");
	strcat(buf1, buf2);

	// not useful for now
	/*
	if (status_report.flags & BP_STATUS_ACKED_BY_APP)
		sprintf(buf2, "%u;", status_report.ack_by_app_ts.secs);
	else
		sprintf(buf2, " ;");
	strcat(buf1, buf2);
	*/

	// status report reason
	strcat(buf1, al_bp_status_report_reason_to_str(status_report.reason));
	strcat(buf1, ";");

	fwrite(buf1, strlen(buf1), 1, file);
}

void csv_print_long(FILE * file, long num)
{
	char buf[50];
	sprintf(buf, "%ld", num);
	csv_print(file, buf);
}

void csv_print_ulong(FILE * file, unsigned long num)
{
	char buf[50];
	sprintf(buf, "%lu", num);
	csv_print(file, buf);
}

void csv_print(FILE * file, char * string)
{
	char buf[256];
	memset(buf, 0, 256);
	strcat(buf, string);
	if (buf[strlen(buf) -1] != ';')
		strcat(buf, ";");
	fwrite(buf, strlen(buf), 1, file);
}

void csv_end_line(FILE * file)
{
	char c = '\n';
	fwrite(&c, 1, 1, file);
	fflush(file);
}

