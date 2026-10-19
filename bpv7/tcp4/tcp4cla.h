/*
	tcp4cla.h:	common definitions for TCPCLv4 convergence
			layer adapter modules.

			The TCP4 CLA communicates with an external
			TCPCLv4 agent process via POSIX shared memory
			pools (data plane) and Unix domain socket
			control channels (control plane).

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef TCP4CLA_H
#define TCP4CLA_H

#include "bpP.h"
#include "ion_transfer_pool.h"
#include "ion_ctrl_channel.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	Default configuration constants.
 *
 *	TCP4CLA_SLOT_SIZE and TCP4CLA_NUM_SLOTS can be overridden at
 *	compile time via -D flags or by configure options:
 *
 *	  ./configure --enable-tcpclv4-external --with-tcp4-slot-size=5    # 5MB slots
 *	  ./configure --enable-tcpclv4-external --with-tcp4-num-slots=8    # 8 slots per pool
 *
 *	Memory usage per duct (TX + RX pools):
 *	  1MB × 8:     16 MB  (default)
 *	  5MB × 8:     80 MB  (recommended for 50MB bundles)
 *	  10MB × 4:    80 MB  (alternative)
 *	  50MB × 8:   800 MB  (dedicated bulk transfer)		*/

#ifndef TCP4CLA_SLOT_SIZE
#  define TCP4CLA_SLOT_SIZE		(1 << 20)	/* 1MB (default) */
#endif

#ifndef TCP4CLA_NUM_SLOTS
#  define TCP4CLA_NUM_SLOTS		8		/* 8 slots (default) */
#endif

/*	Deprecated aliases for backward compatibility.			*/
#define TCP4CLA_BUFSZ			TCP4CLA_SLOT_SIZE
#define TCP4CLA_DEFAULT_NUM_SLOTS	TCP4CLA_NUM_SLOTS
#define TCP4CLA_DEFAULT_SLOT_SIZE	TCP4CLA_SLOT_SIZE

/*	Compile-time sanity checks.					*/

#if TCP4CLA_SLOT_SIZE < (64 * 1024)
#  error "TCP4CLA_SLOT_SIZE must be at least 64KB"
#endif

#if TCP4CLA_NUM_SLOTS < 2
#  error "TCP4CLA_NUM_SLOTS must be at least 2"
#endif

/*	Note: Can't reliably check for pool size > 1GB in preprocessor
 *	without risking integer overflow, so we skip this check.
 *	Users should be aware that large slot sizes require sufficient
 *	shared memory. See README.md for system configuration.		*/

/*	BP bundle header overhead margin.
 *	When auto-configuring maxPayloadLen, we must account for BP
 *	bundle headers (primary block, payload block headers, extensions).
 *	Typical overhead is 90-300 bytes; we use 1KB margin for safety.	*/
#define TCP4CLA_BP_HEADER_MARGIN	1024	/* 1KB safety margin	*/

#define TCP4CLA_KEEPALIVE_INTERVAL	15	/* seconds		*/
#define TCP4CLA_KEEPALIVE_TIMEOUT	45	/* seconds (3x interval)*/
#define TCP4CLA_CTRL_POLL_MS		1000	/* 1s poll timeout	*/
#define TCP4CLA_MAX_INFLIGHT		64
#define TCP4CLA_SOCK_DIR		"/tmp/ion_tcp4"

/*	Configuration parsed from duct name.				*/

typedef struct
{
	char		duct_name[MAX_CL_DUCT_NAME_LEN + 1];
	char		tx_pool_name[256];
	char		rx_pool_name[256];
	char		tx_ctrl_path[256];
	char		rx_ctrl_path[256];
	uint32_t	num_slots;
	uint32_t	slot_data_capacity;
} Tcp4Config;

/*	In-flight transfer tracking for async disposition.
 *	Used by tcp4clo to correlate XFER_SUCCESS/FAILURE
 *	back to the original bundleZco for bpHandleXmit*.		*/

typedef struct
{
	Object		bundleZco;
	uint64_t	xfer_id;
	int		in_use;
} Tcp4InFlight;

/*	Parse duct name and populate config with derived
 *	pool names and control socket paths.
 *
 *	Pool names:  /ion_tcp4_tx_<ductName>, /ion_tcp4_rx_<ductName>
 *	Ctrl paths:  /tmp/ion_tcp4/tx_<ductName>.sock,
 *		     /tmp/ion_tcp4/rx_<ductName>.sock
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	tcp4_parse_config(const char *ductName, Tcp4Config *cfg);

/*	In-flight transfer table management.
 *
 *	These functions are NOT thread-safe; callers must hold
 *	the inflight mutex when operating on the table.			*/

extern void	tcp4_inflight_init(Tcp4InFlight *table, int size);

extern int	tcp4_inflight_add(Tcp4InFlight *table, int size,
			Object bundleZco, uint64_t xfer_id);

extern Object	tcp4_inflight_remove(Tcp4InFlight *table, int size,
			uint64_t xfer_id);

/*	Fail all in-flight transfers by calling
 *	bpHandleXmitFailure() for each.  Used during
 *	agent crash recovery.						*/

extern void	tcp4_inflight_fail_all(Tcp4InFlight *table, int size);

/*	Extract a bundle ZCO into a shared memory pool slot.
 *
 *	Uses zco_start_transmitting() + zco_transmit() to copy
 *	bundle bytes directly into pool_slot_data(), achieving
 *	one copy from SDR to shared memory.
 *
 *	Returns bytes written (>= 0) on success, -1 on error.		*/

extern int	tcp4_extract_bundle(Sdr sdr, Object bundleZco,
			IonTransferPool *pool, int slot_idx);

#ifdef __cplusplus
}
#endif

#endif /* TCP4CLA_H */
