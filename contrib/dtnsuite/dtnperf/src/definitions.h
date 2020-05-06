/********************************************************
 **  Authors: Michele Rodolfi, michele.rodolfi@studio.unibo.it
 **           Anna d'Amico, anna.damico@studio.unibo.it
 **           Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * definitions.h
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include <stdint.h>

// dtnperf version
#define DTNPERF_VERSION "3.6.2"

// dtnperf server mode string
#define SERVER_STRING "--server"

// dtnperf client mode string
#define CLIENT_STRING "--client"

// dtnperf monitor mode string
#define MONITOR_STRING "--monitor"

// dir where are saved incoming bundles
#define BUNDLE_DIR_DEFAULT "/tmp/"

// dir where are saved transfered files
#define FILE_DIR_DEFAULT "~/dtnperf/files/"

// dir where are saved client and monitor logs
#define LOGS_DIR_DEFAULT "."

// source file for bundle in client with use_file option
#define SOURCE_FILE "/tmp/dtnperfbuf"

// source file for bundle ack in server [ONLY ION implementation]
#define SOURCE_FILE_ACK "/tmp/dtnperfack"

// default client log filename
#define LOG_FILENAME "dtnperf_client.log"

// output file: stdin and stderr redirect here if daemon is TRUE;
#define SERVER_OUTPUT_FILE "dtnperf_server.log"
#define MONITOR_OUTPUT_FILE "dtnperf_monitor.log"

// default filename of the unique csv logfile for the monitor with oneCSVonly option
#define MONITOR_UNIQUE_CSV_FILENAME "monitor_unique_log.csv"

/*
 * FIXED SIZE HEADERS
 */
// header type
#define HEADER_TYPE uint32_t

// header size
#define HEADER_SIZE sizeof(HEADER_TYPE)

// header of bundles sent in time mode
#define TIME_HEADER 0x1

// header of bundles sent in data mode
#define DATA_HEADER 0x2

// header of bundles sent in file mode
#define FILE_HEADER 0x4

// header of dtnperf server bundle ack
#define DSA_HEADER 0x8

// header of force stop bundle sent by client to monitor
#define FORCE_STOP_HEADER 0x10

// header of stop bundle sent by client to monitor
#define STOP_HEADER 0x20

// bundle options type
#define BUNDLE_OPT_TYPE uint16_t

// bundle options size
#define BUNDLE_OPT_SIZE sizeof(uint16_t)

// bundle crc size
#define BUNDLE_CRC_SIZE sizeof(uint32_t)

// congestion control options
// window based
#define BO_ACK_CLIENT_YES 0x8000
// rate based
#define BO_ACK_CLIENT_NO 0x0000
// congestion control option mask (first 2 bits)
#define BO_ACK_CLIENT_MASK 0xC000

// acks to monitor options
// default behavior
#define BO_ACK_MON_NORMAL 0x0000
// force server to send acks to monitor
#define BO_ACK_MON_FORCE_YES 0x2000
// force server to not send acks to monitor
#define BO_ACK_MON_FORCE_NO 0x1000
// acks to monitor options mask
#define BO_ACK_MON_MASK 0x3000

// set ack expiration time as this bundle one
#define BO_SET_EXPIRATION 0x0080

// ack priority options
// set ack priority bit
#define BO_SET_PRIORITY 0x0040
// priorities indicated
#define BO_PRIORITY_BULK 0x0000
#define BO_PRIORITY_NORMAL 0x0010
#define BO_PRIORITY_EXPEDITED 0x0020
#define BO_PRIORITY_RESERVED 0x0030
// priority mask
#define BO_PRIORITY_MASK 0x0030

// crc options (BO HEADER)
// IF IN BUNDLE FROM CLIENT TO SERVER
// BIT 4 -> 0 : CRC DISABLED, 1: CRC ENABLED
// ELSE IF IN BUNDLE FROM SERVER TO CLIENT
// BIT 4 -> 0 : CRC CHECK OK (OR NOT ENABLED), 1: CRC CHECK FAILED
#define BO_CRC_DISABLED 0x0000
#define BO_CRC_ENABLED	0x0800

// max payload (in bytes) if bundles are stored into memory
#define MAX_MEM_PAYLOAD 50000

// illegal number of bytes for the bundle payload
#define ILLEGAL_PAYLOAD 0

// default value (in bytes) for bundle payload
#define DEFAULT_PAYLOAD 50000

// DTN none
#define DTNNONE_STRING "dtn:none"

// CBHE scheme
#define CBHE_SCHEME_STRING "ipn"

// DTN scheme
#define DTN_SCHEME_STRING "dtn"

// server endpoint demux string
#define SERV_EP_STRING "/dtnperf:/dest"
#define SERV_EP_NUM_SERVICE "2000"

// client endpoint demux string
#define CLI_EP_STRING "/dtnperf:/src"

// monitor endpoint demux string
#define MON_EP_STRING "/dtnperf:/mon"
#define MON_EP_NUM_SERVICE "1000"

// generic payload pattern
#define PL_PATTERN "0123456789"

// unix time of 1/1/2000
#define DTN_EPOCH 946684800

/**
 * Metadata type code numbers used in Metadata Blocks - see RFC 6258
 */
typedef enum {
	METADATA_TYPE_URI		        = 0x01,  ///< Metadata block carries URI
	METADATA_TYPE_EXPT_MIN			= 0xc0,  ///< Low end of experimental range
	METADATA_TYPE_EXPT_MAX			= 0xff,  ///< High end of experimental range
} metadata_type_code_t;

/**
 * Valid type codes for bundle blocks.
 * (See http://www.dtnrg.org/wiki/AssignedNamesAndNumbers)
 * THis is copied from servlib/bundling/BundleProtcocol.h
 */
typedef enum {
    PRIMARY_BLOCK               = 0x000, ///< INTERNAL ONLY -- NOT IN SPEC
    PAYLOAD_BLOCK               = 0x001, ///< Defined in RFC5050
    BUNDLE_AUTHENTICATION_BLOCK = 0x002, ///< Defined in RFC6257
    PAYLOAD_SECURITY_BLOCK      = 0x003, ///< Defined in RFC6257
    CONFIDENTIALITY_BLOCK       = 0x004, ///< Defined in RFC6257
    PREVIOUS_HOP_BLOCK          = 0x005, ///< Defined in RFC6259
    METADATA_BLOCK              = 0x008, ///< Defined in RFC6258
    EXTENSION_SECURITY_BLOCK    = 0x009, ///< Defined in RFC6257
    SESSION_BLOCK               = 0x00c, ///< NOT IN SPEC YET
    AGE_BLOCK                   = 0x00a, ///< draft-irtf-dtnrg-bundle-age-block-01
    QUERY_EXTENSION_BLOCK       = 0x00b, ///< draft-irtf-dtnrg-bpq-00
    SEQUENCE_ID_BLOCK           = 0x010, ///< NOT IN SPEC YET
    OBSOLETES_ID_BLOCK          = 0x011, ///< NOT IN SPEC YET
    API_EXTENSION_BLOCK         = 0x100, ///< INTERNAL ONLY -- NOT IN SPEC
    UNKNOWN_BLOCK               = 0x101, ///< INTERNAL ONLY -- NOT IN SPEC
} bundle_block_type_t;


#endif /* DEFINITIONS_H_ */
