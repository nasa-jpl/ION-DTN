/********************************************************
    Authors:
    Lorenzo Mustich, lorenzo.mustich@studio.unibo.it
    Carlo Caini (DTNproxy project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

 ********************************************************/

/*
 * definitions.h

 * Define cosnstans used by DTNproxy
 *
 */
 #include <unistd.h>
 #include <sys/syscall.h>
 #include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef DTNPROXY_SRC_DEFINITIONS_H_
#define DTNPROXY_SRC_DEFINITIONS_H_

/**------------------------------------------------------------------------
 * Constants for DTNperf --client -F (compatibility with DTNPerf-server)
 * (This constants are used in DTNperf)
 **-----------------------------------------------------------------------*/

// source file
#define SOURCE_FILE "/tmp/dtnperfbuf"
// output file: stdin and stderr redirect here if daemon is TRUE;
#define SERVER_OUTPUT_FILE "dtnperf_server.log"
#define MONITOR_OUTPUT_FILE "dtnperf_monitor.log"
// dir where are saved incoming bundles
#define BUNDLE_DIR_DEFAULT "/tmp/"
// dir where are saved transfered files
#define FILE_DIR_DEFAULT "~/dtnperf/files/"
// dir where are saved client and monitor logs
#define LOGS_DIR_DEFAULT "."
// default filename of the unique csv logfile for the monitor with oneCSVonly option
#define MONITOR_UNIQUE_CSV_FILENAME "monitor_unique_log.csv"

/*
 * FIXED SIZE HEADERS
 */
// Header type
#define HEADER_TYPE uint32_t
// Header size
#define HEADER_SIZE sizeof(HEADER_TYPE)
// bundle options size
#define BUNDLE_OPT_SIZE sizeof(uint16_t)
// Bundle crc size
#define BUNDLE_CRC_SIZE sizeof(uint32_t)
// Bundle options type
#define BUNDLE_OPT_TYPE uint16_t
// header of bundles sent in file mode
#define FILE_HEADER 0x4
// default value (in bytes) for bundle payload
#define DEFAULT_PAYLOAD 50000
// window based
#define BO_ACK_CLIENT_YES 0x8000
// rate based
#define BO_ACK_CLIENT_NO 0x0000
// default behavior
#define BO_ACK_MON_NORMAL 0x0000
// force server to send acks to monitor
#define BO_ACK_MON_FORCE_YES 0x2000
// force server to not send acks to monitor
#define BO_ACK_MON_FORCE_NO 0x1000
// ack priority options
// set ack priority bit
#define BO_SET_PRIORITY 0x0040
// priorities indicated
#define BO_PRIORITY_BULK 0x0000
#define BO_PRIORITY_NORMAL 0x0010
#define BO_PRIORITY_EXPEDITED 0x0020
#define BO_PRIORITY_RESERVED 0x0030
// set ack expiration time as this bundle one
#define BO_SET_EXPIRATION 0x0080
// crc options (BO HEADER)
#define BO_CRC_DISABLED 0x0000

/**------------------------
 * Constants of DTNproxy
 **-------------------------*/
// Bundle constants
#define BUNDLE_EXPIRATION 180
// Max files num in thread directories
#define MAX_BUFFER_SIZE 10
//Max number of senders in tcp send side
#define N_SENDERS 1000
//Max number of receivers in tcp receive side
#define N_RECEIVERS 10
//Return value wrong tcp ipv4 address
#define WRONG_TCP_IP_ADDRESS 99

/**
 * String length
 */
// Filename length
#define FILE_NAME 512
// IP address length
#define IP_LENGTH 16
//Max number of tentative in tcp send side
#define NUMBER_ATTEMPTS 5
//debug leavel
#define DEBUGLEAVEL 0

/**
 * Default filename
 */
// Default log filename
#define LOG_FILENAME "dtnproxy.log"
// Default TCP-DTN dir
#define TCP_DTN_DIR "/tmp/tcpToBundle/"
// Default DTN-TCP dir
#define DTN_TCP_DIR "/tmp/bundleToTcp/"

#endif /* DTNPROXY_SRC_DEFINITIONS_H_ */
