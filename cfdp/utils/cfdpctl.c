// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.

/**
 * @file cfdpctl.c
 * @author Sky DeBaun, Jet Propulsion Laboratory
 * @date June 2026
 * @brief Command-line interface and asynchronous event-watcher daemon for CFDP.
 *
 * This utility serves as an alternate operational interface for the CCSDS File
 * Delivery Protocol (CFDP) implementation within NASA's Interplanetary Overlay
 * Network (ION). It supersedes legacy test harnesses by providing a dual-mode
 * architecture designed for headless automation and real-time GUI integration.
 *
 * ARCHITECTURAL MODES:
 * 1. Action Mode (Foreground CLI):
 * Executes discrete protocol directives (Put, Get, Filestore, State
 * Manipulation) and exits immediately. Designed for automated regression
 * matrices.
 *
 * 2. Watch Mode (Background Daemon):
 * Runs a persistent, multi-threaded event loop blocking on the SDR event queue.
 * It autonomously responds to remote protocol requests (e.g., Proxy Gets) and
 * broadcasts a real-time JSON telemetry stream of protocol state changes over a
 * non-blocking TCP/UDS socket.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAX_LOG_OUTPUT_LEN   2048
#define MAX_CFDP_USR_MSG_LEN 255

#define CFDP_MAX_PATH_LEN    256
#define CFDP_FS_RESP_FMT_OVERHEAD \
	64 /* Safe padding for "ActionCode: -2147483648..." */
#define MAX_CFDP_FS_RESP_LEN (CFDP_MAX_PATH_LEN + CFDP_FS_RESP_FMT_OVERHEAD)

/* clang-format off */
#include "platform.h"
#include "bp.h"
#include "bputa.h"
#include "cfdp.h"
#include "cfdpops.h"
#include "ion.h"
#include "ion_atomic.h"
#include "sdrxn.h"
/* clang-format on */
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

/* Global operational state. Process-local atomic flag for thread safety. */
static ion_atomic_t g_running = ION_ATOMIC_INIT(1);

/* Global verbosity flag. Toggled to 1 via --quiet to suppress stdout. */
static int g_quiet = 0;

/* Global debug flag. Toggled to 1 via --debug to show system RPC payloads. */
static int g_debug = 0;

/* Tracks active ION SDR attachment to guard writeMemoNote() calls. */
static int g_attached = 0;

/* Global thread state managing the active TCP socket for GUI connections */
static int	       g_client_fd = -1;
static pthread_mutex_t g_client_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Cached local node identity populated from IonDB on watch startup */
static char g_own_fqnn[FQN_MAX_LENGTH] = "";
static char g_host_name[64] = "";

static int send_all_nonblocking(int fd, const char *buf, size_t len)
{
	size_t total_sent = 0;
	int    retries = 0;

	while (total_sent < len)
	{
		ssize_t sent = send(fd, buf + total_sent, len - total_sent, 0);

		if (sent < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}

			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				struct pollfd pfd;
				int	      prc;

				if (retries >= 4)
				{
					return -1;
				}

				retries++;
				for (;;)
				{
					memset(&pfd, 0, sizeof(pfd));
					pfd.fd = fd;
					pfd.events = POLLOUT;
					prc = poll(&pfd, 1, 50);
					if (prc < 0 && errno == EINTR)
					{
						continue;
					}

					break;
				}

				if (prc <= 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
				{
					return -1;
				}

				continue;
			}

			return -1;
		}

		if (sent == 0)
		{
			return -1;
		}

		retries = 0;
		total_sent += (size_t) sent;
	}

	return 0;
}

typedef struct
{
	sigset_t	mask;
	pthread_mutex_t mutex;
	int		exiting;
} SigWaitContext;

/**
 * @brief Synchronously awaits termination signals in a dedicated thread.
 *
 * Replaces async-signal-unsafe trap handlers so cfdp_interrupt() executes in
 * normal pthread context without risking ION SDR or semaphore deadlocks.
 *
 * @param[in] arg Pointer to the shared SigWaitContext synchronization state.
 *
 * @return NULL upon signal receipt or internal shutdown notification.
 */
static void *signal_wait_thread(void *arg)
{
	SigWaitContext *ctx = (SigWaitContext *) arg;

	for (;;)
	{
		int sig = 0;
		int rc;
		int should_interrupt = 0;

		rc = sigwait(&ctx->mask, &sig);
		if (rc != 0)
		{
			if (rc == EINTR)
			{
				continue;
			}

			pthread_mutex_lock(&ctx->mutex);
			if (!ctx->exiting)
			{
				ctx->exiting = 1;
				ion_atomic_set(&g_running, 0);
				should_interrupt = 1;
			}
			pthread_mutex_unlock(&ctx->mutex);

			if (should_interrupt)
			{
				cfdp_interrupt();
			}

			break;
		}

		pthread_mutex_lock(&ctx->mutex);
		if (ctx->exiting || sig == SIGUSR1)
		{
			ctx->exiting = 1;
			pthread_mutex_unlock(&ctx->mutex);
			break;
		}

		ctx->exiting = 1;
		ion_atomic_set(&g_running, 0);
		pthread_mutex_unlock(&ctx->mutex);

		cfdp_interrupt();
		break;
	}

	return NULL;
}

/**
 * @brief Wakes and joins the signal-waiter thread without ESRCH races.
 *
 * @param[in,out] ctx     Pointer to the shared signal waiter context.
 * @param[in]     sig_tid Thread identifier of the running signal_wait_thread.
 */
static void stop_signal_thread(SigWaitContext *ctx, pthread_t sig_tid)
{
	pthread_mutex_lock(&ctx->mutex);
	if (!ctx->exiting)
	{
		ctx->exiting = 1;
		pthread_kill(sig_tid, SIGUSR1);
	}
	pthread_mutex_unlock(&ctx->mutex);

	pthread_join(sig_tid, NULL);
	pthread_mutex_destroy(&ctx->mutex);
}

/**
 * @brief Safely writes to a file descriptor handling partial writes and interrupts.
 *
 * Bypasses GCC -Wunused-result warnings deterministically. It ensures the entire
 * buffer is transmitted by looping on partial writes, and safely ignores terminal
 * errors (e.g., EPIPE) where standard output is physically unavailable.
 */
static void safe_write(int fd, const char *buf, size_t len)
{
	size_t total_written = 0;
	while (total_written < len)
	{
		ssize_t res = write(fd, buf + total_written, len - total_written);
		if (res <= 0)
		{
			if (res < 0 && errno == EINTR)
			{
				continue; /* Interrupted, retry */
			}
			break; /* Zero write or hard error; stop attempting. */
		}
		total_written += (size_t) res;
	}
}

#ifdef HAVE_WORKING_FORK
/**
 * @brief Detaches the process from the controlling terminal for background run.
 *
 * Executes a UNIX double-fork so the daemon cannot acquire a controlling
 * terminal, clears umask, and redirects standard descriptors to /dev/null via
 * checked dup2() calls while preserving the working directory for multi-node
 * ION configurations and relative socket paths.
 *
 * @return 0 in the daemonized grandchild, -1 if the initial fork() fails.
 */
static int daemonize(void)
{
	pid_t		 pid;
	int		 null_fd;
	const char	*err_msg;
	struct sigaction sa;

	pid = fork();
	if (pid < 0)
	{
		err_msg = "Daemon initialization failed: "
			  "First fork() failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		return -1;
	}

	if (pid > 0)
	{
		_exit(EXIT_SUCCESS);
	}

	if (setsid() < 0)
	{
		err_msg = "Daemon initialization failed: setsid() failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		_exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) < 0 || sigaction(SIGHUP, &sa, NULL) < 0)
	{
		err_msg = "Daemon initialization failed: sigaction() failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		_exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid < 0)
	{
		err_msg = "Daemon initialization failed: "
			  "Second fork() failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		_exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{
		_exit(EXIT_SUCCESS);
	}

	umask(0);

	null_fd = open("/dev/null", O_RDWR);
	if (null_fd < 0)
	{
		err_msg = "Daemon initialization failed: "
			  "open(\"/dev/null\") failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		_exit(EXIT_FAILURE);
	}

	if (dup2(null_fd, STDIN_FILENO) < 0 || dup2(null_fd, STDOUT_FILENO) < 0 ||
			dup2(null_fd, STDERR_FILENO) < 0)
	{
		err_msg = "Daemon initialization failed: dup2() failed.\n";
		safe_write(STDERR_FILENO, err_msg, istrlen(err_msg, 256));
		if (null_fd > STDERR_FILENO)
		{
			close(null_fd);
		}
		_exit(EXIT_FAILURE);
	}

	if (null_fd > STDERR_FILENO)
	{
		close(null_fd);
	}

	return 0;
}
#endif /* HAVE_WORKING_FORK */

/**
 * @brief Dispatches synchronous log messages to stdout and the ION SDR log.
 *
 * This function utilizes POSIX write() to bypass buffered I/O, ensuring that
 * messages are immediately flushed even if the daemon crashes shortly after.
 * Logging to stdout is bypassed if the global quiet flag (g_quiet) is active.
 *
 * @note Parameter is intentionally `char *` (not `const char *`) because
 * `platform.h` declares `writeMemoNote(char *, char *)` and ION's `AM_CFLAGS`
 * enforces `-Wcast-qual -Werror`.
 *
 * @param[in] msg Null-terminated string containing the log message.
 */
static void log_event(char *msg)
{
	if (msg == NULL)
	{
		return;
	}

	if (g_attached)
	{
		writeMemoNote("[CFDPCTL]", msg);
	}

	if (!g_quiet)
	{
		safe_write(STDOUT_FILENO, msg, istrlen(msg, MAX_LOG_OUTPUT_LEN));
		safe_write(STDOUT_FILENO, "\n", 1);
	}
}

/**
 * @brief Strictly parses a bounded ASCII digit span [start, end) into a uvast.
 *
 * @param[in]  start Pointer to the first character in the span.
 * @param[in]  end   Pointer one past the last character in the span.
 * @param[out] out   Destination pointer for the parsed uvast value.
 *
 * @return 0 on successful parse, -1 if empty, non-digit, or out of range.
 */
static int parse_strict_uvast_span(const char *start, const char *end, uvast *out)
{
	char   num_buf[32];
	size_t len;
	size_t i;

	if (start == NULL || end == NULL || out == NULL || start >= end)
	{
		return -1;
	}

	len = (size_t) (end - start);
	if (len >= sizeof(num_buf))
	{
		return -1;
	}

	for (i = 0; i < len; i++)
	{
		if (!isdigit((unsigned char) start[i]))
		{
			return -1;
		}
	}

	memcpy(num_buf, start, len);
	num_buf[len] = '\0';

	if (platform_parse_uvast(num_buf, out) < 0)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief Parses a string representation of a CFDP Transaction ID into a struct.
 *
 * Transaction IDs in CFDP are pairs of the source entity ID and a sequential
 * transaction number. This function safely parses strings formatted as
 * "source_node.transaction_number" (e.g., "1.45") into ION's internal
 * compressed number format.
 *
 * @param[in]  str   String representation of the TXID.
 * @param[out] txid  Pointer to the CfdpTransactionId struct to be populated.
 *
 * @return 0 on successful parse, -1 if the string format is invalid.
 */
static int parse_txid(char *str, CfdpTransactionId *txid)
{
	const char *last_dot;
	const char *mid_dot = NULL;
	const char *p;
	uvast	    src_node = 0;
	uvast	    tx_num = 0;

	if (str == NULL || txid == NULL)
	{
		return -1;
	}

	last_dot = strrchr(str, '.');
	if (last_dot == NULL || last_dot == str || *(last_dot + 1) == '\0')
	{
		return -1;
	}

	for (p = str; p < last_dot; p++)
	{
		if (*p == '.')
		{
			if (mid_dot != NULL)
			{
				return -1;
			}
			mid_dot = p;
		}
	}

	if (mid_dot != NULL)
	{
		uvast alloc_nbr = 0;
		uvast node_nbr = 0;

		if (sizeof(uvast) < 8)
		{
			return -1;
		}

		if (parse_strict_uvast_span(str, mid_dot, &alloc_nbr) < 0 ||
				parse_strict_uvast_span(mid_dot + 1, last_dot,
						&node_nbr) < 0)
		{
			return -1;
		}

		if (alloc_nbr > (uvast) 0xFFFFFFFFUL ||
				node_nbr > (uvast) 0xFFFFFFFFUL)
		{
			return -1;
		}

		src_node = ((alloc_nbr << 16) << 16) | node_nbr;
	}
	else
	{
		if (parse_strict_uvast_span(str, last_dot, &src_node) < 0)
		{
			return -1;
		}
	}

	if (parse_strict_uvast_span(last_dot + 1,
			    (last_dot + 1) + istrlen(last_dot + 1, 33),
			    &tx_num) < 0)
	{
		return -1;
	}

	if (src_node == 0 && tx_num == 0)
	{
		return -1;
	}

	cfdp_compress_number(&txid->sourceEntityNbr, src_node);
	cfdp_compress_number(&txid->transactionNbr, tx_num);
	return 0;
}

/**
 * @brief Strictly parses integer arguments to prevent silent zeroing and overflow.
 *
 * Wraps the core platform_parse_int utility to enforce strict 32-bit bounds
 * and terminate the daemon immediately on CLI parsing errors.
 *
 * @param[in] str      The string value to convert.
 * @param[in] arg_name The name of the argument being parsed (for user error output).
 *
 * @param[out] out      Destination pointer for the parsed integer.
 *
 * @return 0 on success, -1 on parse or bounds failure.
 */
static int strict_atoi(const char *str, const char *arg_name, int *out)
{
	if (str == NULL || out == NULL || platform_parse_int(str, out) < 0)
	{
		char err_buf[256];

		isprintf(err_buf, sizeof(err_buf),
				"Error: Invalid integer value for %s: '%s'\n",
				(arg_name != NULL) ? arg_name : "argument",
				(str != NULL) ? str : "(null)");
		safe_write(STDERR_FILENO, err_buf,
				istrlen(err_buf, sizeof(err_buf)));
		return -1;
	}

	return 0;
}

/**
 * @brief Strictly parses unsigned vast arguments using the core platform safe-parser.
 *
 * Prevents negative inputs from silently wrapping to massive unsigned values.
 *
 * @param[in] str      The string value to convert.
 * @param[in] arg_name The name of the argument being parsed (for user error output).
 *
 * @param[out] out      Destination pointer for the parsed uvast value.
 *
 * @return 0 on success, -1 on parse or bounds failure.
 */
static int strict_atouvast(const char *str, const char *arg_name, uvast *out)
{
	if (str == NULL || out == NULL || platform_parse_uvast(str, out) < 0)
	{
		char err_buf[256];

		isprintf(err_buf, sizeof(err_buf),
				"Error: Invalid unsigned integer value "
				"for %s: '%s'\n",
				(arg_name != NULL) ? arg_name : "argument",
				(str != NULL) ? str : "(null)");
		safe_write(STDERR_FILENO, err_buf,
				istrlen(err_buf, sizeof(err_buf)));
		return -1;
	}

	return 0;
}

/**
 * @brief Dispatches a standalone CFDP Filestore Request (Rename or Delete).
 *
 * This executes a metadata-only CFDP transaction, transmitting zero payload bytes.
 * It wraps the filestore directive inside a Bundle Protocol (BP) envelope,
 * applying the requested Class of Service, TTL, and Custody Transfer settings.
 * If the put operation fails, it safely cleans up the SDR transaction.
 *
 * @param[in] node            Destination remote node ID.
 * @param[in] action          The CFDP action to perform (e.g., CfdpRenameFile, CfdpDeleteFile).
 * @param[in] file1           The primary file target on the remote system.
 * @param[in] file2           The secondary file target (required only for Rename actions).
 * @param[in] closure_latency Optional timeout specification for transaction closure.
 * @param[in] priority        BP Class of Service priority level.
 * @param[in] ttl             Time-to-Live (in seconds) for the underlying bundle.
 * @param[in] custody         Boolean flag enabling/disabling BP Custody Transfer.
 * @param[in] critical        Boolean flag marking the bundle for minimum latency routing.
 *
 * @return 0 on successful dispatch to the local BP agent, -1 on failure.
 */
static int action_filestore(uvast node, CfdpAction action, char *file1,
		char *file2, uvast closure_latency, int priority, int ttl,
		int custody, int critical)
{
	CfdpNumber	  destNode;
	CfdpTransactionId txid;
	MetadataList	  fsRequests;
	BpUtParms	  utParms;
	char		  log_buf[256];
	uvast		  src_id, tx_id;

	if (priority < 0 || priority > 2 || ttl <= 0 ||
			closure_latency > (uvast) UINT_MAX)
	{
		log_event("Error: Invalid priority, TTL, or closure bound.");
		return -1;
	}

	memset((char *) &utParms, 0, sizeof(BpUtParms));
	utParms.lifespan = ttl;
	utParms.classOfService = priority;
	utParms.custodySwitch = custody ? SourceCustodyRequired :
					  NoCustodyRequested;

	if (critical)
	{
		utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	}

	cfdp_compress_number(&destNode, (uvast) node);

	fsRequests = cfdp_create_fsreq_list();
	if (fsRequests == 0)
	{
		log_event("Failed to create CFDP filestore request list.");
		return -1;
	}

	if (cfdp_add_fsreq(fsRequests, action, (char *) file1, (char *) file2) < 0)
	{
		log_event("Failed to add CFDP filestore request.");
		cfdp_destroy_fsreq_list(&fsRequests);
		return -1;
	}

	if (cfdp_put(&destNode, sizeof(BpUtParms), (unsigned char *) &utParms,
			    NULL, NULL, NULL, NULL, NULL, 0, NULL,
			    (unsigned int) closure_latency, 0, fsRequests,
			    &txid) <= 0)
	{
		log_event("CFDP Filestore request failed.");

		if (fsRequests != 0)
		{
			cfdp_destroy_fsreq_list(&fsRequests);
		}

		return -1;
	}

	cfdp_decompress_number(&src_id, &txid.sourceEntityNbr);
	cfdp_decompress_number(&tx_id, &txid.transactionNbr);

	snprintf(log_buf, sizeof(log_buf),
			"Filestore Request Dispatched. TXID: " UVAST_FIELDSPEC
			"." UVAST_FIELDSPEC,
			(uvast) src_id, (uvast) tx_id);
	log_event(log_buf);

	return 0;
}

/**
 * @brief Initiates a standard CFDP File Put transaction.
 *
 * This is the primary mechanism for transferring files across the network.
 * Beyond the core payload, it can optionally append User Messages and secondary
 * Filestore Requests (like remote renames) to the same metadata block, ensuring
 * they travel with the file. Memory safety is enforced by explicitly destroying
 * metadata lists in the SDR if the dispatch fails.
 *
 * @param[in] node            Destination node ID.
 * @param[in] src             Path to the local source file.
 * @param[in] dest            Desired path for the destination file on the remote node.
 * @param[in] msg             Optional text message to append as User Metadata.
 * @param[in] rn_src          Optional filestore rename source file.
 * @param[in] rn_dest         Optional filestore rename destination file.
 * @param[in] closure_latency Timeout specification for transaction closure.
 * @param[in] priority        BP Class of Service priority level.
 * @param[in] ttl             Time-to-Live (in seconds) for the underlying bundle.
 * @param[in] custody         Boolean flag enabling/disabling BP Custody Transfer.
 * @param[in] critical        Boolean flag marking the bundle for minimum latency routing.
 *
 * @return 0 on successful initiation, -1 if the SDR/BP agent rejects the request.
 */
static int action_put(uvast node, char *src, char *dest, char *msg,
		char *rn_src, char *rn_dest, uvast closure_latency,
		int priority, int ttl, int custody, int critical)
{
	CfdpNumber	  destNode;
	CfdpTransactionId txid;
	MetadataList	  msgsToUser = 0;
	MetadataList	  fsRequests = 0;
	BpUtParms	  utParms;
	char		  log_buf[256];
	uvast		  src_id, tx_id;

	char *safe_src = (src && src[0] != '\0') ? (char *) src : NULL;
	char *safe_dest = (dest && dest[0] != '\0') ? (char *) dest : NULL;

	if (priority < 0 || priority > 2 || ttl <= 0 ||
			closure_latency > (uvast) UINT_MAX)
	{
		log_event("Error: Invalid priority, TTL, or closure bound.");
		return -1;
	}

	memset((char *) &utParms, 0, sizeof(BpUtParms));
	utParms.lifespan = ttl;
	utParms.classOfService = priority;
	utParms.custodySwitch = custody ? SourceCustodyRequired :
					  NoCustodyRequested;

	if (critical)
	{
		utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	}

	cfdp_compress_number(&destNode, (uvast) node);

	if (msg != NULL)
	{
		msgsToUser = cfdp_create_usrmsg_list();
		if (msgsToUser == 0)
		{
			log_event("Failed to create CFDP user message list.");
			return -1;
		}

		if (cfdp_add_usrmsg(msgsToUser, (unsigned char *) msg,
				    istrlen(msg, MAX_CFDP_USR_MSG_LEN)) < 0)
		{
			log_event("Failed to add CFDP user message.");
			cfdp_destroy_usrmsg_list(&msgsToUser);
			return -1;
		}
	}

	if (rn_src != NULL && rn_dest != NULL)
	{
		fsRequests = cfdp_create_fsreq_list();
		if (fsRequests == 0)
		{
			log_event("Failed to create CFDP filestore "
				  "request list.");
			if (msgsToUser != 0)
			{
				cfdp_destroy_usrmsg_list(&msgsToUser);
			}
			return -1;
		}

		if (cfdp_add_fsreq(fsRequests, CfdpRenameFile, (char *) rn_src,
				    (char *) rn_dest) < 0)
		{
			log_event("Failed to add CFDP filestore request.");
			cfdp_destroy_fsreq_list(&fsRequests);
			if (msgsToUser != 0)
			{
				cfdp_destroy_usrmsg_list(&msgsToUser);
			}
			return -1;
		}
	}

	if (cfdp_put(&destNode, sizeof(BpUtParms), (unsigned char *) &utParms,
			    safe_src, safe_dest, NULL, NULL, NULL, 0, NULL,
			    (unsigned int) closure_latency, msgsToUser,
			    fsRequests, &txid) <= 0)
	{
		log_event("CFDP Put request failed.");

		if (msgsToUser != 0)
		{
			cfdp_destroy_usrmsg_list(&msgsToUser);
		}
		if (fsRequests != 0)
		{
			cfdp_destroy_fsreq_list(&fsRequests);
		}

		return -1;
	}

	cfdp_decompress_number(&src_id, &txid.sourceEntityNbr);
	cfdp_decompress_number(&tx_id, &txid.transactionNbr);
	snprintf(log_buf, sizeof(log_buf),
			"Transfer Initiated. TXID: " UVAST_FIELDSPEC
			"." UVAST_FIELDSPEC,
			(uvast) src_id, (uvast) tx_id);
	log_event(log_buf);

	return 0;
}

/**
 * @brief Requests a Directory Listing from a remote CFDP entity.
 *
 * This command instructs the target node to execute a local filesystem listing
 * on the specified directory. Once complete, the remote node will automatically
 * initiate a reverse Put transaction to transmit the manifest back to this node,
 * saving it at `local_dest`.
 *
 * @param[in] node       Destination node ID to query.
 * @param[in] remote_dir Directory path on the remote filesystem to index.
 * @param[in] local_dest Local file path where the incoming manifest will be stored.
 * @param[in] priority   BP Class of Service priority level.
 * @param[in] ttl        Time-to-Live (in seconds) for the query bundle.
 * @param[in] custody    Boolean flag enabling/disabling BP Custody Transfer.
 * @param[in] critical   Boolean flag marking the bundle for minimum latency routing.
 *
 * @return 0 on successful dispatch, -1 if arguments are missing or SDR fails.
 */
static int action_dir_list(uvast node, char *remote_dir, char *local_dest,
		int priority, int ttl, int custody, int critical)
{
	CfdpNumber	  destNode;
	CfdpTransactionId txid;
	BpUtParms	  utParms;
	CfdpDirListTask	  task;
	char		  log_buf[256];
	uvast		  src_id, tx_id;

	if (!remote_dir || !local_dest)
	{
		log_event("Error: Remote dir and local dest required.");
		return -1;
	}

	if (priority < 0 || priority > 2 || ttl <= 0)
	{
		log_event("Error: Invalid priority (0..2) or TTL (> 0).");
		return -1;
	}

	memset((char *) &txid, 0, sizeof(CfdpTransactionId));
	memset((char *) &utParms, 0, sizeof(BpUtParms));
	utParms.lifespan = ttl;
	utParms.classOfService = priority;
	utParms.custodySwitch = custody ? SourceCustodyRequired :
					  NoCustodyRequested;

	if (critical)
	{
		utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	}

	cfdp_compress_number(&destNode, (uvast) node);

	task.directoryName = (char *) remote_dir;
	task.destFileName = (char *) local_dest;

#ifdef CFDP_DIRLIST_OPTION_STATUS_BYTES
	if (cfdp_rls_extended(&destNode, sizeof(BpUtParms),
			    (unsigned char *) &utParms, NULL, NULL, NULL, NULL,
			    0, NULL, 0, 0, 0, &task,
			    CFDP_DIRLIST_OPTION_STATUS_BYTES, &txid) < 0 ||
			txid.transactionNbr.length == 0)
#else
	if (cfdp_rls(&destNode, sizeof(BpUtParms), (unsigned char *) &utParms,
			    NULL, NULL, NULL, NULL, 0, NULL, 0, 0, 0, &task,
			    &txid) < 0 ||
			txid.transactionNbr.length == 0)
#endif
	{
		log_event("CFDP Directory Listing request failed.");
		return -1;
	}

	cfdp_decompress_number(&src_id, &txid.sourceEntityNbr);
	cfdp_decompress_number(&tx_id, &txid.transactionNbr);
	snprintf(log_buf, sizeof(log_buf),
			"Dir List Requested. TXID: " UVAST_FIELDSPEC
			"." UVAST_FIELDSPEC,
			(uvast) src_id, (uvast) tx_id);
	log_event(log_buf);

	return 0;
}

/**
 * @brief Dispatches a Proxy Get request to pull a file from a remote node.
 *
 * Because CFDP operates primarily on push mechanics (Put), retrieving a file
 * requires instructing the remote entity to become the sender. This sends a
 * proxy command payload to the remote agent, which, upon processing, will initiate
 * a reverse transfer back to our local node.
 *
 * @param[in] node            Destination node ID hosting the desired file.
 * @param[in] remote_src      Path to the source file on the remote node.
 * @param[in] local_dest      Path where the file should be saved locally.
 * @param[in] closure_latency Optional timeout specification for the reverse transaction.
 * @param[in] priority        BP Class of Service priority level.
 * @param[in] ttl             Time-to-Live (in seconds) for the proxy command.
 * @param[in] custody         Boolean flag enabling/disabling BP Custody Transfer.
 * @param[in] critical        Boolean flag marking the bundle for minimum latency routing.
 *
 * @return 0 on successful dispatch of the proxy command, -1 on failure.
 */
static int action_get(uvast node, char *remote_src, char *local_dest,
		uvast closure_latency, int priority, int ttl, int custody,
		int critical)
{
	CfdpNumber	  destNode;
	CfdpTransactionId txid;
	BpUtParms	  utParms;
	CfdpProxyTask	  task;
	char		  log_buf[256];
	uvast		  src_id, tx_id;

	if (!remote_src || !local_dest)
	{
		log_event("Error: Remote source and local dest required.");
		return -1;
	}

	if (priority < 0 || priority > 2 || ttl <= 0 ||
			closure_latency > (uvast) UINT_MAX)
	{
		log_event("Error: Invalid priority, TTL, or closure bound.");
		return -1;
	}

	memset((char *) &txid, 0, sizeof(CfdpTransactionId));
	memset((char *) &utParms, 0, sizeof(BpUtParms));
	utParms.lifespan = ttl;
	utParms.classOfService = priority;
	utParms.custodySwitch = custody ? SourceCustodyRequired :
					  NoCustodyRequested;

	if (critical)
	{
		utParms.ancillaryData.flags |= BP_MINIMUM_LATENCY;
	}

	cfdp_compress_number(&destNode, (uvast) node);

	memset((char *) &task, 0, sizeof(CfdpProxyTask));
	task.sourceFileName = (char *) remote_src;
	task.destFileName = (char *) local_dest;
	task.closureRequested = (closure_latency > 0) ? 1 : 0;
	task.unacknowledged = 1;

	if (cfdp_get(&destNode, sizeof(BpUtParms), (unsigned char *) &utParms,
			    NULL, NULL, NULL, NULL, 0, NULL, 0, 0, 0, &task,
			    &txid) < 0 ||
			txid.transactionNbr.length == 0)
	{
		log_event("CFDP Proxy Get request failed.");
		return -1;
	}

	cfdp_decompress_number(&src_id, &txid.sourceEntityNbr);
	cfdp_decompress_number(&tx_id, &txid.transactionNbr);
	snprintf(log_buf, sizeof(log_buf),
			"Proxy Get Requested. TXID: " UVAST_FIELDSPEC
			"." UVAST_FIELDSPEC,
			(uvast) src_id, (uvast) tx_id);
	log_event(log_buf);

	return 0;
}

/**
 * @brief Intercepts an active CFDP transaction to modify its processing state.
 *
 * Used primarily for operator intervention (pausing massive transfers, cancelling
 * broken transactions, or pulling specific status reports). The transaction ID
 * must be explicitly known and provided.
 *
 * @param[in] txid_str String representation of the transaction ID (e.g., "1.42").
 * @param[in] action   Integer mapping to the state alteration directive:
 * 0 (Suspend), 1 (Resume), 2 (Cancel), 3 (Status Report).
 *
 * @return 0 on successful state change in the SDR, -1 on failure or parsing error.
 */
static int action_state_change(char *txid_str, int action)
{
	CfdpTransactionId txid;
	int		  result = -1;

	if (txid_str == NULL)
	{
		log_event("Error: Missing Transaction ID. Use --txid");
		return -1;
	}

	if (parse_txid(txid_str, &txid) < 0)
	{
		log_event("Invalid TXID format. Use src.txnum (e.g., 2.45)");
		return -1;
	}

	if (action == 0)
	{
		result = cfdp_suspend(&txid);
	}
	else if (action == 1)
	{
		result = cfdp_resume(&txid);
	}
	else if (action == 2)
	{
		result = cfdp_cancel(&txid);
	}
	else if (action == 3)
	{
		result = cfdp_report(&txid);
	}

	if (result <= 0)
	{
		log_event("State change or report request failed.");
		return -1;
	}

	log_event("Request dispatched successfully.");
	return 0;
}

/**
 * @brief Appends a labeled string field to a log buffer without overflow.
 *
 * @param[in,out] buf   Destination log buffer.
 * @param[in]     cap   Total byte capacity of @p buf.
 * @param[in,out] pos   Current write offset in @p buf, updated on append.
 * @param[in]     label Prefix label (e.g., " | Src: ").
 * @param[in]     value Field value string; ignored if NULL or empty.
 */
static void append_log_field(char *buf, size_t cap, size_t *pos,
		const char *label, const char *value)
{
	int    written;
	size_t rem;

	if (value == NULL || value[0] == '\0' || cap == 0 || *pos >= cap - 1)
	{
		return;
	}

	rem = cap - *pos;
	written = snprintf(buf + *pos, rem, "%s%s", label, value);
	if (written < 0)
	{
		return;
	}

	if ((size_t) written >= rem)
	{
		*pos = cap - 1;
	}
	else
	{
		*pos += (size_t) written;
	}
}

/**
 * @brief Escapes and appends raw bytes to a JSON string buffer safely.
 *
 * @param[in,out] dst     Destination buffer.
 * @param[in]     dst_cap Total byte capacity of @p dst.
 * @param[in,out] pos     Current write offset in @p dst, updated on append.
 * @param[in]     src     Source byte buffer.
 * @param[in]     src_len Number of valid bytes in @p src.
 */
static void append_sanitized_json(char *dst, size_t dst_cap, size_t *pos,
		const unsigned char *src, size_t src_len)
{
	size_t i;
	size_t j;

	if (dst == NULL || dst_cap == 0 || pos == NULL || src == NULL)
	{
		return;
	}

	j = *pos;
	if (j >= dst_cap)
	{
		return;
	}

	for (i = 0; i < src_len; i++)
	{
		unsigned char c = src[i];

		if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t')
		{
			if (j + 2 >= dst_cap)
			{
				break;
			}

			dst[j++] = '\\';
			if (c == '\n')
			{
				dst[j++] = 'n';
			}
			else if (c == '\r')
			{
				dst[j++] = 'r';
			}
			else if (c == '\t')
			{
				dst[j++] = 't';
			}
			else
			{
				dst[j++] = (char) c;
			}
		}
		else
		{
			if (j + 1 >= dst_cap)
			{
				break;
			}

			dst[j++] = (c >= 32 && c <= 126) ? (char) c : '_';
		}
	}

	dst[j] = '\0';
	*pos = j;
}

/**
 * @brief Serializes a CFDP SDR event into JSON and broadcasts it via TCP/UDS.
 *
 * This function handles the messy extraction and string escaping required to take
 * a raw CFDP event from the ION database and convert it into a valid JSON payload
 * for external UI consumption. It protects against malformed strings in User
 * Messages and Status Reports breaking the JSON structure.
 *
 * @note This is currently a one-way telemetry stream using manual string formatting.
 * A future architectural update is required to swap `isprintf` for a formal POSIX
 * JSON library to support bi-directional UI command integration.
 *
 * @param[in] type             The specific enum code of the CFDP event triggered.
 * @param[in] txid             Pointer to the transaction ID associated with the event.
 * @param[in] cond             Transaction condition code (e.g., NoError, Cancelled).
 * @param[in] progress         Current byte offset of the ongoing transfer.
 * @param[in] fileSize         Total expected size of the file being transferred.
 * @param[in] fileStatus       Current disposition of the file in the local filestore.
 * @param[in] deliveryCode     Final delivery status (Complete, Incomplete).
 * @param[in] closureRequested Flag indicating if the sender demanded a closure report.
 * @param[in] msgsToUser       List pointer containing arbitrary text messages.
 * @param[in] fsResponses      List pointer of filestore action responses.
 * @param[in] offset           Segment offset for data events.
 * @param[in] length           Segment length for data events.
 */
typedef struct
{
	/* Event output buffers populated by cfdp_get_event() */
	char srcFile[256];
	char destFile[256];
	char statusBuf[256];
	char segMetaBuf[64];

	/* Message and filestore response scratch buffers */
	unsigned char textBuf[256];
	char	      f1[256];
	char	      f2[256];
	char	      fs_msg[256];
	char	      fs_resp_buf[512];
	char	      safe_msg_buf[512];
	char	      proxy_rpc_buf[512];

	/* JSON serialization and terminal log formatting buffers */
	char safe_src_buf[512];
	char safe_dest_buf[512];
	char safe_status_buf[512];
	char safe_fs_resp_buf[1024];
	char json_buf[4096];
	char final_log[MAX_LOG_OUTPUT_LEN];
} WatchWorkspace;

static void serialize_and_send_event(WatchWorkspace *ws, CfdpEventType type,
		time_t eventTime, CfdpTransactionId *txid, CfdpCondition cond,
		uvast progress, uvast fileSize, CfdpFileStatus fileStatus,
		CfdpDeliveryCode deliveryCode, unsigned int closureRequested,
		CfdpTransactionId *origTxid, MetadataList *msgsToUser,
		MetadataList *fsResponses, uvast offset, unsigned int length)
{
	uvast  src_id = 0;
	uvast  tx_num = 0;
	int    has_client = 0;
	size_t msg_pos = 0;
	size_t rpc_pos = 0;

	if (ws == NULL || txid == NULL || msgsToUser == NULL ||
			fsResponses == NULL)
	{
		return;
	}

	ws->fs_resp_buf[0] = '\0';
	ws->safe_msg_buf[0] = '\0';
	ws->proxy_rpc_buf[0] = '\0';

	/* 1. Always decode primary TXID (needed for standard terminal log) */
	cfdp_decompress_number(&src_id, &txid->sourceEntityNbr);
	cfdp_decompress_number(&tx_num, &txid->transactionNbr);

	/* 2. EXTRACT, SORT, AND SANITIZE ALL MESSAGES */
	if (*msgsToUser != 0)
	{
		int msg_len = 0;

		/* Drain messages until the list is empty */
		while (cfdp_get_usrmsg(msgsToUser, ws->textBuf, &msg_len) == 0 &&
				msg_len > 0)
		{
			int safe_len = msg_len;

			if (safe_len > (int) sizeof(ws->textBuf))
			{
				safe_len = (int) sizeof(ws->textBuf);
			}

			/* Intercept binary proxy RPCs ("cfdp" + type < 32) */
			if (safe_len >= 5 && ws->textBuf[4] < 32 &&
					strncmp((const char *) ws->textBuf,
							"cfdp", 4) == 0)
			{
				append_sanitized_json(ws->proxy_rpc_buf,
						sizeof(ws->proxy_rpc_buf),
						&rpc_pos,
						(const unsigned char *) "[Proxy RPC: ",
						12);
				append_sanitized_json(ws->proxy_rpc_buf,
						sizeof(ws->proxy_rpc_buf),
						&rpc_pos, ws->textBuf,
						(size_t) safe_len);
				append_sanitized_json(ws->proxy_rpc_buf,
						sizeof(ws->proxy_rpc_buf),
						&rpc_pos,
						(const unsigned char *) "] ", 2);
			}
			else
			{
				if (msg_pos > 0)
				{
					append_sanitized_json(ws->safe_msg_buf,
							sizeof(ws->safe_msg_buf),
							&msg_pos,
							(const unsigned char *) " ",
							1);
				}

				append_sanitized_json(ws->safe_msg_buf,
						sizeof(ws->safe_msg_buf),
						&msg_pos, ws->textBuf,
						(size_t) safe_len);
			}
		}
	}

	if (*fsResponses != 0)
	{
		size_t fs_pos = 0;

		while (*fsResponses != 0)
		{
			CfdpAction action = (CfdpAction) -1;
			int	   fs_status = 0;
			int	   fs_written;
			size_t	   fs_rem;

			memset(ws->f1, 0, sizeof(ws->f1));
			memset(ws->f2, 0, sizeof(ws->f2));
			memset(ws->fs_msg, 0, sizeof(ws->fs_msg));

			if (cfdp_get_fsresp(fsResponses, &action, &fs_status,
					    ws->f1, ws->f2, ws->fs_msg) < 0 ||
					(int) action == -1)
			{
				break;
			}

			if (fs_pos >= sizeof(ws->fs_resp_buf) - 1)
			{
				continue;
			}

			fs_rem = sizeof(ws->fs_resp_buf) - fs_pos;
			fs_written = snprintf(ws->fs_resp_buf + fs_pos, fs_rem,
					"%sActionCode: %d, Status: %d, "
					"Target: %s",
					(fs_pos > 0) ? "; " : "", (int) action,
					fs_status, ws->f1);
			if (fs_written > 0)
			{
				if ((size_t) fs_written >= fs_rem)
				{
					fs_pos = sizeof(ws->fs_resp_buf) - 1;
				}
				else
				{
					fs_pos += (size_t) fs_written;
				}
			}
		}
	}

	/* 3. Build JSON and send to GUI (if connected) */
	pthread_mutex_lock(&g_client_mutex);
	has_client = (g_client_fd >= 0);
	pthread_mutex_unlock(&g_client_mutex);

	if (has_client)
	{
		uvast orig_src_id = 0;
		uvast orig_tx_num = 0;
		int   json_len;

		ws->json_buf[0] = '\0';
		ws->safe_src_buf[0] = '\0';
		ws->safe_dest_buf[0] = '\0';
		ws->safe_status_buf[0] = '\0';
		ws->safe_fs_resp_buf[0] = '\0';

		/* Decode Originating Proxy TXID if present */
		if (origTxid != NULL)
		{
			cfdp_decompress_number(&orig_src_id,
					&origTxid->sourceEntityNbr);
			cfdp_decompress_number(&orig_tx_num,
					&origTxid->transactionNbr);
		}

		if (ws->srcFile[0] != '\0')
		{
			size_t src_pos = 0;

			append_sanitized_json(ws->safe_src_buf,
					sizeof(ws->safe_src_buf), &src_pos,
					(const unsigned char *) ws->srcFile,
					istrlen(ws->srcFile, CFDP_MAX_PATH_LEN));
		}

		if (ws->destFile[0] != '\0')
		{
			size_t dest_pos = 0;

			append_sanitized_json(ws->safe_dest_buf,
					sizeof(ws->safe_dest_buf), &dest_pos,
					(const unsigned char *) ws->destFile,
					istrlen(ws->destFile, CFDP_MAX_PATH_LEN));
		}

		if (ws->statusBuf[0] != '\0')
		{
			size_t status_pos = 0;

			append_sanitized_json(ws->safe_status_buf,
					sizeof(ws->safe_status_buf), &status_pos,
					(const unsigned char *) ws->statusBuf,
					istrlen(ws->statusBuf, 256));
		}

		if (ws->fs_resp_buf[0] != '\0')
		{
			size_t fs_json_pos = 0;

			append_sanitized_json(ws->safe_fs_resp_buf,
					sizeof(ws->safe_fs_resp_buf),
					&fs_json_pos,
					(const unsigned char *) ws->fs_resp_buf,
					istrlen(ws->fs_resp_buf,
							sizeof(ws->fs_resp_buf)));
		}

		/* Build the expanded JSON Payload using escaped buffers */
		json_len = snprintf(ws->json_buf, sizeof(ws->json_buf),
				"{\n"
				"  \"eventType\": %d,\n"
				"  \"time\": " VAST_FIELDSPEC ",\n"
				"  \"transactionId\": \"" UVAST_FIELDSPEC
				"." UVAST_FIELDSPEC "\",\n"
				"  \"sourceFileName\": \"%s\",\n"
				"  \"destFileName\": \"%s\",\n"
				"  \"origTransactionId\": \"" UVAST_FIELDSPEC
				"." UVAST_FIELDSPEC "\",\n"
				"  \"progress\": " UVAST_FIELDSPEC ",\n"
				"  \"offset\": " UVAST_FIELDSPEC ",\n"
				"  \"length\": %u,\n"
				"  \"fileSize\": " UVAST_FIELDSPEC ",\n"
				"  \"condition\": %d,\n"
				"  \"deliveryCode\": %d,\n"
				"  \"fileStatus\": %d,\n"
				"  \"closureRequested\": %u,\n"
				"  \"statusReport\": \"%s\",\n"
				"  \"messageToUser\": \"%s\",\n"
				"  \"fsResponse\": \"%s\"\n"
				"}\n",
				(int) type, (vast) eventTime, (uvast) src_id,
				(uvast) tx_num, ws->safe_src_buf,
				ws->safe_dest_buf, (uvast) orig_src_id,
				(uvast) orig_tx_num, (uvast) progress,
				(uvast) offset, length, (uvast) fileSize,
				(int) cond, (int) deliveryCode, (int) fileStatus,
				closureRequested, ws->safe_status_buf,
				ws->safe_msg_buf, ws->safe_fs_resp_buf);

		if (json_len > 0 && (size_t) json_len < sizeof(ws->json_buf))
		{
			/* Broadcast JSON telemetry to attached client. */
			pthread_mutex_lock(&g_client_mutex);
			if (g_client_fd >= 0)
			{
				if (send_all_nonblocking(g_client_fd, ws->json_buf,
						    (size_t) json_len) < 0)
				{
					close(g_client_fd);
					g_client_fd = -1;
				}
			}
			pthread_mutex_unlock(&g_client_mutex);
		}
		else
		{
			log_event("Warning: Dropped truncated or invalid "
				  "JSON telemetry.");
		}
	}

	/* 4. Standard terminal log (Executes regardless of client state) */
	if (!g_quiet && type != 5)
	{
		size_t pos = 0;
		int    written;

		ws->final_log[0] = '\0';
		written = snprintf(ws->final_log, sizeof(ws->final_log),
				"Event %d for TX " UVAST_FIELDSPEC
				"." UVAST_FIELDSPEC " (Prog: " UVAST_FIELDSPEC
				")",
				(int) type, (uvast) src_id, (uvast) tx_num,
				(uvast) progress);
		if (written > 0)
		{
			if ((size_t) written >= sizeof(ws->final_log))
			{
				pos = sizeof(ws->final_log) - 1;
			}
			else
			{
				pos = (size_t) written;
			}
		}
		else
		{
			ws->final_log[0] = '\0';
		}

		append_log_field(ws->final_log, sizeof(ws->final_log), &pos,
				" | Src: ", ws->srcFile);
		append_log_field(ws->final_log, sizeof(ws->final_log), &pos,
				" | Dest: ", ws->destFile);
		append_log_field(ws->final_log, sizeof(ws->final_log), &pos,
				" | Msg: ", ws->safe_msg_buf);
		if (g_debug)
		{
			append_log_field(ws->final_log, sizeof(ws->final_log),
					&pos, " | ", ws->proxy_rpc_buf);
		}

		append_log_field(ws->final_log, sizeof(ws->final_log), &pos,
				" | Report: ", ws->statusBuf);
		append_log_field(ws->final_log, sizeof(ws->final_log), &pos,
				" | FS_Resp: ", ws->fs_resp_buf);

		log_event(ws->final_log);
	}
}

/**
 * @brief Independent p-thread handling incoming TCP/UDS socket connections.
 *
 * This thread isolates the blocking `accept()` network calls from the primary
 * CFDP event loop. This ensures that the daemon can continuously drain the
 * ION database queues in real-time without stalling while waiting for a GUI
 * client to connect or disconnect.
 *
 * @param[in] arg Pointer to the active server socket file descriptor.
 *
 * @return NULL upon graceful thread termination.
 */
static void *gui_listener_thread(void *arg)
{
	int server_fd = *(int *) arg;
	int logged_exhaustion = 0;

	while (ion_atomic_get(&g_running))
	{
		struct pollfd pfd;
		int	      poll_rc;
		int	      new_client;
		int	      flags;

		memset(&pfd, 0, sizeof(pfd));
		pfd.fd = server_fd;
		pfd.events = POLLIN;

		poll_rc = poll(&pfd, 1, 200);
		if (!ion_atomic_get(&g_running))
		{
			break;
		}

		if (poll_rc == 0)
		{
			continue;
		}

		if (poll_rc < 0)
		{
			if (errno == EINTR || errno == EAGAIN ||
					errno == EWOULDBLOCK)
			{
				continue;
			}

			log_event("Listener poll() failed; stopping.");
			break;
		}

		new_client = accept(server_fd, NULL, NULL);
		if (new_client < 0)
		{
			int err = errno;

			if (!ion_atomic_get(&g_running))
			{
				break;
			}

			if (err == EINTR || err == EAGAIN || err == EWOULDBLOCK ||
					err == ECONNABORTED)
			{
				continue;
			}

			if (err == EMFILE || err == ENFILE || err == ENOBUFS ||
					err == ENOMEM)
			{
				if (!logged_exhaustion)
				{
					log_event("Accept failed: resource "
						  "exhausted, backing off.");
					logged_exhaustion = 1;
				}

				microsnooze(100000);
				continue;
			}

			log_event("Listener accept() failed; stopping.");
			break;
		}

		if (!ion_atomic_get(&g_running))
		{
			close(new_client);
			break;
		}

		logged_exhaustion = 0;

		/* Enforce non-blocking I/O for deterministic event loops */
		flags = fcntl(new_client, F_GETFL, 0);
		if (flags < 0 || fcntl(new_client, F_SETFL, flags | O_NONBLOCK) < 0)
		{
			log_event("Failed to set O_NONBLOCK on GUI socket.");
			close(new_client);
			continue;
		}

		log_event("GUI connected.");

		pthread_mutex_lock(&g_client_mutex);
		if (g_client_fd >= 0)
		{
			/* Gracefully sever the old connection */
			const char *disconnect_msg = "{\n"
						     "  \"eventType\": -1,\n"
						     "  \"statusReport\": "
						     "\"Disconnecting "
						     "for new client.\"\n"
						     "}\n";
			(void) send_all_nonblocking(g_client_fd, disconnect_msg,
					istrlen(disconnect_msg, 256));
			close(g_client_fd);
			g_client_fd = -1;
		}
		if (g_own_fqnn[0] != '\0')
		{
			char hello_msg[256];
			int  hello_len;

			hello_len = snprintf(hello_msg, sizeof(hello_msg),
					"{\n"
					"  \"eventType\": 0,\n"
					"  \"ownEntityId\": \"%s\",\n"
					"  \"host\": \"%s\"\n"
					"}\n",
					g_own_fqnn, g_host_name);
			if (hello_len > 0 &&
					(size_t) hello_len < sizeof(hello_msg))
			{
				(void) send_all_nonblocking(new_client,
						hello_msg, (size_t) hello_len);
			}
		}
		g_client_fd = new_client;
		pthread_mutex_unlock(&g_client_mutex);
	}

	return NULL;
}

/**
 * @brief Safely unlinks a stale UNIX domain socket after verifying its type.
 *
 * @note Intentional lstat() + S_ISSOCK() guard before unlink(). Do NOT remove
 * lstat() to silence static-analyzer TOCTOU warnings: POSIX provides no atomic
 * socket-only unlink, and --sock is user-supplied. Calling unlink() blindly on
 * EADDRINUSE would delete regular files if passed via a CLI typo.
 *
 * @param[in] path Filesystem path to the candidate UNIX domain socket.
 *
 * @return 0 if unlinked (or already absent), -1 if non-socket or unlink fails.
 */
static int unlink_verified_socket(const char *path)
{
	struct stat st;

	if (path == NULL || lstat(path, &st) < 0 || !S_ISSOCK(st.st_mode))
	{
		return -1;
	}

	if (unlink(path) < 0 && errno != ENOENT)
	{
		return -1;
	}

	return 0;
}

/**
 * @brief Spools up the persistent, multi-threaded event daemon.
 *
 * This function initializes the network socket (TCP or UNIX Domain Socket) to
 * listen for UI clients, spawns the `gui_listener_thread`, and then enters a
 * continuous blocking loop on `cfdp_get_event()`. It reads state changes from
 * the SDR, serializes them, and performs necessary SDR memory cleanup for
 * processed lists (like user messages or filestore responses).
 *
 * @param[in] endpoint The socket binding parameter. If it contains a slash
 * ('/'), it creates a UNIX domain socket (recommended for local UI clients).
 * Otherwise, it is treated as a TCP port (binding INADDR_LOOPBACK only) or
 * explicit "<ipv4>:<port>" specification. Note: the telemetry stream is
 * unauthenticated; avoid binding non-loopback addresses on untrusted networks.
 *
 * @return 0 on graceful termination, -1 on critical startup failure.
 */
static int watch_mode(const char *endpoint)
{
	int		      server_fd;
	int		      is_uds;
	pthread_t	      listener_tid;
	pthread_t	      sig_tid;
	SigWaitContext	      sig_ctx;
	WatchWorkspace	     *ws;
	CfdpEventType	      type;
	time_t		      event_time;
	int		      reqNbr;
	CfdpTransactionId     txid;
	CfdpTransactionId     origTxid;
	uvast		      fileSize;
	uvast		      offset;
	uvast		      progress;
	unsigned int	      length;
	unsigned int	      recBounds;
	unsigned int	      segMetaLen;
	unsigned int	      closureRequested;
	CfdpContinuationState contState;
	CfdpCondition	      cond;
	CfdpFileStatus	      fileStatus;
	CfdpDeliveryCode      deliveryCode;

	if (endpoint == NULL)
	{
		return -1;
	}

	ws = (WatchWorkspace *) MTAKE(sizeof(WatchWorkspace));
	if (ws == NULL)
	{
		log_event("Failed to allocate watch workspace.");
		return -1;
	}
	memset(ws, 0, sizeof(WatchWorkspace));

	is_uds = (strchr(endpoint, '/') != NULL);
	if (is_uds)
	{
		struct sockaddr_un addr;
		size_t		   path_len = (size_t) istrlen(endpoint,
				sizeof(addr.sun_path));
		mode_t		   old_mask;
		int		   bind_res;

		if (path_len >= sizeof(addr.sun_path))
		{
			log_event("UNIX socket path too long.");
			MRELEASE(ws);
			return -1;
		}

		server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (server_fd < 0)
		{
			log_event("Failed to open UNIX socket.");
			MRELEASE(ws);
			return -1;
		}

		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		memcpy(addr.sun_path, endpoint, path_len + 1);

		/* Create UDS with 0600 perms (avoid daemon umask(0)) */
		old_mask = umask(0177);
		bind_res = bind(server_fd, (struct sockaddr *) &addr,
				sizeof(addr));
		umask(old_mask);

		if (bind_res < 0)
		{
			if (errno == EADDRINUSE)
			{
				int test_fd;
				int conn_err = 0;

				test_fd = socket(AF_UNIX, SOCK_STREAM, 0);
				if (test_fd >= 0)
				{
					if (connect(test_fd, (struct sockaddr *) &addr,
							    sizeof(addr)) < 0)
					{
						conn_err = errno;
					}

					close(test_fd);
				}

				if (conn_err == ECONNREFUSED)
				{
					if (unlink_verified_socket(endpoint) < 0)
					{
						log_event("Refusing to unlink "
							  "non-socket or "
							  "unremovable "
							  "endpoint.");
						close(server_fd);
						MRELEASE(ws);
						return -1;
					}

					old_mask = umask(0177);
					bind_res = bind(server_fd,
							(struct sockaddr *) &addr,
							sizeof(addr));
					umask(old_mask);

					if (bind_res < 0)
					{
						log_event("Failed to bind "
							  "UNIX socket after "
							  "clearing stale "
							  "endpoint.");
						close(server_fd);
						MRELEASE(ws);
						return -1;
					}
				}
				else
				{
					log_event("Failed to bind: UNIX "
						  "socket is actively "
						  "in use by another "
						  "daemon.");
					close(server_fd);
					MRELEASE(ws);
					return -1;
				}
			}
			else
			{
				log_event("Failed to bind UNIX socket. "
					  "Invalid path or permissions.");
				close(server_fd);
				MRELEASE(ws);
				return -1;
			}
		}
	}
	else
	{
		/* TCP socket initialization block */
		struct sockaddr_in addr;
		uvast		   port_val;
		unsigned short	   port;
		int		   opt = 1;
		const char	  *colon;

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		colon = strchr(endpoint, ':');
		if (colon != NULL)
		{
			char   ip_buf[INET_ADDRSTRLEN];
			size_t ip_len = (size_t) (colon - endpoint);

			if (ip_len == 0 || ip_len >= sizeof(ip_buf))
			{
				log_event("Invalid TCP bind address.");
				MRELEASE(ws);
				return -1;
			}

			memcpy(ip_buf, endpoint, ip_len);
			ip_buf[ip_len] = '\0';
			if (inet_pton(AF_INET, ip_buf, &addr.sin_addr) != 1)
			{
				log_event("Invalid IPv4 bind address.");
				MRELEASE(ws);
				return -1;
			}

			if (platform_parse_uvast(colon + 1, &port_val) < 0 ||
					port_val == 0 || port_val > 65535)
			{
				log_event("Invalid TCP port number.");
				MRELEASE(ws);
				return -1;
			}

			if (ntohl(addr.sin_addr.s_addr) != INADDR_LOOPBACK)
			{
				log_event("Warning: binding unauthenticated "
					  "TCP telemetry to non-loopback.");
			}
		}
		else
		{
			if (platform_parse_uvast(endpoint, &port_val) < 0 ||
					port_val == 0 || port_val > 65535)
			{
				log_event("Invalid TCP port number.");
				MRELEASE(ws);
				return -1;
			}

			/* Default to loopback to avoid exposing live state */
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		}

		port = (unsigned short) port_val;
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0)
		{
			log_event("Failed to open TCP socket.");
			MRELEASE(ws);
			return -1;
		}

		/* Prevent "Address already in use" on rapid test restarts */
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
				sizeof(opt));

		addr.sin_port = htons(port);

		if (bind(server_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		{
			log_event("Failed to bind TCP socket.");
			close(server_fd);
			MRELEASE(ws);
			return -1;
		}
	}

	if (listen(server_fd, 1) < 0)
	{
		log_event("Failed to listen on socket.");
		close(server_fd);
		if (is_uds)
		{
			(void) unlink_verified_socket(endpoint);
		}
		MRELEASE(ws);
		return -1;
	}

	{
		int srv_flags = fcntl(server_fd, F_GETFL, 0);

		if (srv_flags < 0 ||
				fcntl(server_fd, F_SETFL,
						srv_flags | O_NONBLOCK) < 0)
		{
			log_event("Failed to set O_NONBLOCK on "
				  "server socket.");
			close(server_fd);
			if (is_uds)
			{
				(void) unlink_verified_socket(endpoint);
			}
			MRELEASE(ws);
			return -1;
		}
	}

	memset(&sig_ctx, 0, sizeof(sig_ctx));
	sigemptyset(&sig_ctx.mask);
	sigaddset(&sig_ctx.mask, SIGINT);
	sigaddset(&sig_ctx.mask, SIGTERM);
	sigaddset(&sig_ctx.mask, SIGUSR1);
	if (pthread_mutex_init(&sig_ctx.mutex, NULL) != 0)
	{
		log_event("Failed to initialize signal mutex.");
		close(server_fd);
		if (is_uds)
		{
			(void) unlink_verified_socket(endpoint);
		}
		MRELEASE(ws);
		return -1;
	}

	if (pthread_create(&sig_tid, NULL, signal_wait_thread, &sig_ctx) != 0)
	{
		log_event("Failed to start signal handler thread.");
		pthread_mutex_destroy(&sig_ctx.mutex);
		close(server_fd);
		if (is_uds)
		{
			(void) unlink_verified_socket(endpoint);
		}
		MRELEASE(ws);
		return -1;
	}

	{
		Sdr	  sdr = getIonsdr();
		SdrObject iondbObj = getIonDbObject();

		g_own_fqnn[0] = '\0';
		g_host_name[0] = '\0';
		if (sdr != NULL && iondbObj != 0 && sdr_begin_xn(sdr))
		{
			IonDB iondb;

			sdr_read(sdr, (char *) &iondb, iondbObj, sizeof(IonDB));
			sdr_exit_xn(sdr);
			if (iondb.ownFqnn > 0)
			{
				putFqn(g_own_fqnn, iondb.ownFqnn);
			}
		}
		if (gethostname(g_host_name, sizeof(g_host_name) - 1) == 0)
		{
			char *dot;

			g_host_name[sizeof(g_host_name) - 1] = '\0';
			dot = strchr(g_host_name, '.');
			if (dot != NULL)
			{
				*dot = '\0';
			}
		}
	}

	log_event("Watcher daemon listening for GUI connections...");

	if (pthread_create(&listener_tid, NULL, gui_listener_thread,
			    &server_fd) != 0)
	{
		log_event("Failed to start GUI listener thread.");
		stop_signal_thread(&sig_ctx, sig_tid);
		close(server_fd);
		if (is_uds)
		{
			(void) unlink_verified_socket(endpoint);
		}
		MRELEASE(ws);
		return -1;
	}

	/* Main CFDP SDR Event Pump */
	while (ion_atomic_get(&g_running))
	{
		MetadataList msgsToUser = 0;
		MetadataList fsResponses = 0;

		closureRequested = 0;
		memset(ws->srcFile, 0, sizeof(ws->srcFile));
		memset(ws->destFile, 0, sizeof(ws->destFile));
		memset(ws->statusBuf, 0, sizeof(ws->statusBuf));
		memset(ws->segMetaBuf, 0, sizeof(ws->segMetaBuf));

		if (cfdp_get_event(&type, &event_time, &reqNbr, &txid,
				    ws->srcFile, ws->destFile, &fileSize,
				    &msgsToUser, &offset, &length, &recBounds,
				    &contState, &segMetaLen, ws->segMetaBuf,
				    &cond, &progress, &fileStatus,
				    &deliveryCode, &origTxid, ws->statusBuf,
				    &fsResponses, &closureRequested) < 0)
		{
			if (msgsToUser != 0)
			{
				cfdp_destroy_usrmsg_list(&msgsToUser);
			}
			if (fsResponses != 0)
			{
				cfdp_destroy_fsresp_list(&fsResponses);
			}
			log_event("cfdp_get_event failed.");
			break;
		}

		/* Trap: Breaks loop immediately if ION's SDR is destroyed */
		if ((int) type == -1)
		{
			if (msgsToUser != 0)
			{
				cfdp_destroy_usrmsg_list(&msgsToUser);
			}
			if (fsResponses != 0)
			{
				cfdp_destroy_fsresp_list(&fsResponses);
			}
			break;
		}

		if (type == CfdpNoEvent)
		{
			if (msgsToUser != 0)
			{
				cfdp_destroy_usrmsg_list(&msgsToUser);
			}
			if (fsResponses != 0)
			{
				cfdp_destroy_fsresp_list(&fsResponses);
			}
			continue;
		}

		serialize_and_send_event(ws, type, event_time, &txid, cond,
				progress, fileSize, fileStatus, deliveryCode,
				closureRequested, &origTxid, &msgsToUser,
				&fsResponses, offset, length);

		/* Safely check for GUI disconnects */
		pthread_mutex_lock(&g_client_mutex);
		if (g_client_fd >= 0)
		{
			char check_buf;
			ssize_t ret = recv(g_client_fd, &check_buf, 1, MSG_PEEK);

			/*
			 * ret == 0 is a graceful close; ret < 0 other than
			 * EAGAIN, EWOULDBLOCK, or EINTR is a dead socket.
			 */
			if (ret == 0 ||
					(ret < 0 && errno != EAGAIN &&
							errno != EWOULDBLOCK &&
							errno != EINTR))
			{
				log_event("GUI disconnected.");
				close(g_client_fd);
				g_client_fd = -1;
			}
		}
		pthread_mutex_unlock(&g_client_mutex);

		if (msgsToUser != 0)
		{
			cfdp_destroy_usrmsg_list(&msgsToUser);
		}
		if (fsResponses != 0)
		{
			cfdp_destroy_fsresp_list(&fsResponses);
		}
	}

	ion_atomic_set(&g_running, 0);

	/* Join poll()-gated listener thread before closing server_fd */
	pthread_join(listener_tid, NULL);
	close(server_fd);

	if (is_uds)
	{
		(void) unlink_verified_socket(endpoint);
	}

	pthread_mutex_lock(&g_client_mutex);
	if (g_client_fd >= 0)
	{
		close(g_client_fd);
		g_client_fd = -1;
	}
	pthread_mutex_unlock(&g_client_mutex);

	/* Wake and join signal waiter thread */
	stop_signal_thread(&sig_ctx, sig_tid);

	MRELEASE(ws);
	return 0;
}

#ifdef HAVE_WORKING_FORK
/**
 * @brief Builds a sanitized PID file path under /tmp for a watch endpoint.
 *
 * @param[in]  endpoint Socket endpoint string (port, ip:port, or UDS path).
 * @param[out] pid_path Destination buffer for the PID file path.
 * @param[in]  cap      Capacity of @p pid_path in bytes.
 */
static void build_pid_path(const char *endpoint, char *pid_path, size_t cap)
{
	char	    tag[128];
	size_t	    i = 0;
	const char *p = endpoint;

	if (p == NULL || *p == '\0')
	{
		istrcpy(pid_path, "/tmp/cfdpctl.pid", cap);
		return;
	}

	while (*p == '/')
	{
		p++;
	}

	while (*p != '\0' && i < sizeof(tag) - 1)
	{
		unsigned char c = (unsigned char) *p;

		if (isalnum(c) || c == '.' || c == '-' || c == '_')
		{
			tag[i++] = (char) c;
		}
		else
		{
			tag[i++] = '_';
		}

		p++;
	}

	tag[i] = '\0';
	if (tag[0] == '\0')
	{
		istrcpy(pid_path, "/tmp/cfdpctl.pid", cap);
		return;
	}

	isprintf(pid_path, (int) cap, "/tmp/cfdpctl_%s.pid", tag);
}
#endif /* HAVE_WORKING_FORK */

/**
 * @brief Primary entry point and argument tokenizer.
 *
 * Configures the signal handlers, handles potential background daemonization,
 * attaches to the local ION SDR, and routes execution to either a foreground
 * action command (put, get, ls, etc.) or the persistent background watcher daemon.
 *
 * @param[in] argc Argument count from the environment.
 * @param[in] argv Argument vector containing runtime parameters.
 *
 * @return 0 on successful execution and safe teardown, 1 on argument failure
 * or if the ION node is unreachable.
 */
static int cfdpctl_main(int argc, char *argv[])
{
	int   mode_watch = 0, mode_put = 0, mode_suspend = 0, mode_resume = 0;
	int   mode_cancel = 0, mode_ls = 0, mode_get = 0, mode_rm = 0;
	int   mode_mv = 0, mode_report = 0, mode_daemon = 0;
	int   mode_count = 0;
	uvast node = 0, closure_latency = 0;

	int priority = 1;
	int ttl = 300;
	int custody = 0;
	int critical = 0;

	char	   *src = NULL, *dest = NULL, *msg = NULL, *txid_str = NULL;
	const char *endpoint = "5555";
	char	   *rn_src = NULL, *rn_dest = NULL;
	int	    c, status = 0;
	int	    mask_blocked = 0;
	struct sigaction sa;
	sigset_t	 orig_mask;
#ifdef HAVE_WORKING_FORK
	int  pid_created = 0;
	char pid_path[256] = { 0 };
#endif

	struct option long_options[] = { { "watch", no_argument, &mode_watch, 1 },
		{ "daemon", no_argument, &mode_daemon, 1 },
		{ "quiet", no_argument, &g_quiet, 1 },
		{ "debug", no_argument, &g_debug, 1 },
		{ "put", no_argument, &mode_put, 1 },
		{ "get", no_argument, &mode_get, 1 },
		{ "ls", no_argument, &mode_ls, 1 },
		{ "rm", no_argument, &mode_rm, 1 },
		{ "mv", no_argument, &mode_mv, 1 },
		{ "suspend", no_argument, &mode_suspend, 1 },
		{ "resume", no_argument, &mode_resume, 1 },
		{ "cancel", no_argument, &mode_cancel, 1 },
		{ "report", no_argument, &mode_report, 1 },
		{ "custody", no_argument, &custody, 1 },
		{ "critical", no_argument, &critical, 1 },
		{ "src", required_argument, 0, 's' },
		{ "dest", required_argument, 0, 'd' },
		{ "node", required_argument, 0, 'n' },
		{ "msg", required_argument, 0, 'm' },
		{ "sock", required_argument, 0, 'p' },
		{ "txid", required_argument, 0, 't' },
		{ "closure", required_argument, 0, 'c' },
		{ "priority", required_argument, 0, 'P' },
		{ "ttl", required_argument, 0, 'T' },
		{ "rn-src", required_argument, 0, 'R' },
		{ "rn-dest", required_argument, 0, 'D' }, { 0, 0, 0, 0 } };

	/* Reset file-scope state for RTEMS single-address-space re-entrancy */
	ion_atomic_set(&g_running, 1);
	g_quiet = 0;
	g_debug = 0;
	g_attached = 0;
	pthread_mutex_lock(&g_client_mutex);
	g_client_fd = -1;
	g_own_fqnn[0] = '\0';
	g_host_name[0] = '\0';
	pthread_mutex_unlock(&g_client_mutex);
	optind = 1;

	sigemptyset(&orig_mask);

	while ((c = getopt_long(argc, argv, "s:d:n:m:p:t:c:P:T:R:D:bq",
				long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 0:
			break;
		case 's':
			src = optarg;
			break;
		case 'd':
			dest = optarg;
			break;
		case 'n':
			if (strict_atouvast(optarg, "--node", &node) < 0)
			{
				return EXIT_FAILURE;
			}
			break;
		case 'm':
			msg = optarg;
			break;
		case 'p':
			endpoint = optarg;
			break;
		case 't':
			txid_str = optarg;
			break;
		case 'c':
			if (strict_atouvast(optarg, "--closure",
					    &closure_latency) < 0)
			{
				return EXIT_FAILURE;
			}
			if (closure_latency > (uvast) UINT_MAX)
			{
				const char *err_msg = "Error: --closure exceeds "
						      "unsigned int range.\n";
				safe_write(STDERR_FILENO, err_msg,
						istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
				return EXIT_FAILURE;
			}
			break;
		case 'P':
			if (strict_atoi(optarg, "--priority", &priority) < 0)
			{
				return EXIT_FAILURE;
			}
			if (priority < 0 || priority > 2)
			{
				const char *err_msg = "Error: --priority must be "
						      "in range [0, 2].\n";
				safe_write(STDERR_FILENO, err_msg,
						istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
				return EXIT_FAILURE;
			}
			break;
		case 'T':
			if (strict_atoi(optarg, "--ttl", &ttl) < 0)
			{
				return EXIT_FAILURE;
			}
			if (ttl <= 0)
			{
				const char *err_msg = "Error: --ttl must be "
						      "greater than 0.\n";
				safe_write(STDERR_FILENO, err_msg,
						istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
				return EXIT_FAILURE;
			}
			break;
		case 'R':
			rn_src = optarg;
			break;
		case 'D':
			rn_dest = optarg;
			break;
		case 'b':
			mode_daemon = 1;
			break;
		case 'q':
			g_quiet = 1;
			break;
		case '?':
		default:
		{
			const char *err_msg = "Error: Unrecognized or "
					      "incomplete CLI option.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			return EXIT_FAILURE;
		}
		}
	}

	if (optind < argc)
	{
		char err_buf[256];
		isprintf(err_buf, sizeof(err_buf),
				"Error: Unexpected positional argument: '%s'\n",
				argv[optind]);
		safe_write(STDERR_FILENO, err_buf,
				istrlen(err_buf, sizeof(err_buf)));
		return EXIT_FAILURE;
	}

	mode_count = mode_watch + mode_put + mode_get + mode_ls + mode_rm +
			mode_mv + mode_suspend + mode_resume + mode_cancel +
			mode_report;
	if (mode_count == 0)
	{
		const char *err_msg =
				"Specify mode: --watch, --put, --get, --ls, "
				"--rm, --mv, --suspend, --resume, --cancel, "
				"or --report\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		return EXIT_FAILURE;
	}
	if (mode_count > 1)
	{
		const char *err_msg = "Error: Operational modes are mutually "
				      "exclusive; specify only one mode.\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		return EXIT_FAILURE;
	}
	if (mode_daemon && !mode_watch)
	{
		const char *err_msg = "Error: --daemon (-b) is only valid "
				      "with --watch.\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		return EXIT_FAILURE;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGPIPE, &sa, NULL) < 0)
	{
		const char *err_msg = "Failed to ignore SIGPIPE.\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		return EXIT_FAILURE;
	}

	if (mode_daemon)
	{
#ifdef HAVE_WORKING_FORK
		if (daemonize() < 0)
		{
			return EXIT_FAILURE;
		}
#else
		log_event("Daemon mode requested, but fork() is not "
			  "supported on this platform. Running in "
			  "foreground.");
#endif
	}

	if (mode_watch)
	{
		sigset_t watch_mask;

		sigemptyset(&watch_mask);
		sigaddset(&watch_mask, SIGINT);
		sigaddset(&watch_mask, SIGTERM);
		sigaddset(&watch_mask, SIGUSR1);
		if (pthread_sigmask(SIG_BLOCK, &watch_mask, &orig_mask) != 0)
		{
			const char *err_msg = "Failed to block watch signals.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			return EXIT_FAILURE;
		}
		mask_blocked = 1;
	}

	if (cfdp_attach() < 0)
	{
		const char *err_msg =
				"Failed to attach. Is the ION node running?\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		if (mask_blocked)
		{
			pthread_sigmask(SIG_SETMASK, &orig_mask, NULL);
		}
		return EXIT_FAILURE;
	}

	g_attached = 1;

#ifdef HAVE_WORKING_FORK
	/* Write PID file after cfdp_attach() succeeds */
	if (mode_watch)
	{
		int pid_fd;

		build_pid_path(endpoint, pid_path, sizeof(pid_path));

		/* Use O_EXCL and 0600 perms to prevent TOCTOU in /tmp */
		pid_fd = open(pid_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (pid_fd >= 0)
		{
			char pid_buf[32];

			isprintf(pid_buf, sizeof(pid_buf), "%d\n",
					(int) getpid());
			safe_write(pid_fd, pid_buf,
					istrlen(pid_buf, sizeof(pid_buf)));
			close(pid_fd);
			pid_created = 1;
		}
		else
		{
			log_event("Warning: Failed to write PID file "
				  "securely (file may already exist).");
		}
	}
#endif

	if (mode_watch)
	{
		status = watch_mode(endpoint);
	}
	else if (mode_get)
	{
		if ((src && src[0] == '\0') || (dest && dest[0] == '\0'))
		{
			const char *err_msg = "Error: Path arguments cannot "
					      "be empty.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if (!src || !dest || node == 0)
		{
			char err_buf[256];
			isprintf(err_buf, sizeof(err_buf),
					"Usage: %s --get --src <remote> "
					"--dest <local> --node <id>\n",
					argv[0]);
			safe_write(STDERR_FILENO, err_buf,
					istrlen(err_buf, sizeof(err_buf)));
			status = EXIT_FAILURE;
		}
		else
		{
			status = action_get(node, src, dest, closure_latency,
					priority, ttl, custody, critical);
		}
	}
	else if (mode_put)
	{
		if ((src != NULL) != (dest != NULL))
		{
			const char *err_msg = "Error: --src and --dest must be "
					      "specified together.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if ((rn_src != NULL) != (rn_dest != NULL))
		{
			const char *err_msg = "Error: --rn-src and --rn-dest "
					      "must be specified together.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if ((src && src[0] == '\0') || (dest && dest[0] == '\0') ||
				(rn_src && rn_src[0] == '\0') ||
				(rn_dest && rn_dest[0] == '\0'))
		{
			const char *err_msg = "Error: Path arguments cannot "
					      "be empty.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if (node == 0 || (!src && !msg && !rn_src))
		{
			char err_buf[256];
			isprintf(err_buf, sizeof(err_buf),
					"Usage: %s --put [--src <p> "
					"--dest <p>] [--msg <t>] "
					"[--rn-src <p> --rn-dest <p>] "
					"--node <id>\n",
					argv[0]);
			safe_write(STDERR_FILENO, err_buf,
					istrlen(err_buf, sizeof(err_buf)));
			status = EXIT_FAILURE;
		}
		else
		{
			status = action_put(node, src, dest, msg, rn_src,
					rn_dest, closure_latency, priority, ttl,
					custody, critical);
		}
	}
	else if (mode_ls)
	{
		if ((src && src[0] == '\0') || (dest && dest[0] == '\0'))
		{
			const char *err_msg = "Error: Path arguments cannot "
					      "be empty.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if (!src || !dest || node == 0)
		{
			char err_buf[256];
			isprintf(err_buf, sizeof(err_buf),
					"Usage: %s --ls --src <remote_dir> "
					"--dest <local> --node <id>\n",
					argv[0]);
			safe_write(STDERR_FILENO, err_buf,
					istrlen(err_buf, sizeof(err_buf)));
			status = EXIT_FAILURE;
		}
		else
		{
			status = action_dir_list(node, src, dest, priority, ttl,
					custody, critical);
		}
	}
	else if (mode_rm)
	{
		if (dest && dest[0] == '\0')
		{
			const char *err_msg = "Error: Path argument cannot "
					      "be empty.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if (!dest || node == 0)
		{
			char err_buf[256];
			isprintf(err_buf, sizeof(err_buf),
					"Usage: %s --rm --dest <remote_file> "
					"--node <id>\n",
					argv[0]);
			safe_write(STDERR_FILENO, err_buf,
					istrlen(err_buf, sizeof(err_buf)));
			status = EXIT_FAILURE;
		}
		else
		{
			status = action_filestore(node, CfdpDeleteFile, dest,
					NULL, closure_latency, priority, ttl,
					custody, critical);
		}
	}
	else if (mode_mv)
	{
		if ((src && src[0] == '\0') || (dest && dest[0] == '\0'))
		{
			const char *err_msg = "Error: Path arguments cannot "
					      "be empty.\n";
			safe_write(STDERR_FILENO, err_msg,
					istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
			status = EXIT_FAILURE;
		}
		else if (!src || !dest || node == 0)
		{
			char err_buf[256];
			isprintf(err_buf, sizeof(err_buf),
					"Usage: %s --mv --src <old> "
					"--dest <new> --node <id>\n",
					argv[0]);
			safe_write(STDERR_FILENO, err_buf,
					istrlen(err_buf, sizeof(err_buf)));
			status = EXIT_FAILURE;
		}
		else
		{
			status = action_filestore(node, CfdpRenameFile, src,
					dest, closure_latency, priority, ttl,
					custody, critical);
		}
	}
	else if (mode_suspend)
	{
		status = action_state_change(txid_str, 0);
	}
	else if (mode_resume)
	{
		status = action_state_change(txid_str, 1);
	}
	else if (mode_cancel)
	{
		status = action_state_change(txid_str, 2);
	}
	else if (mode_report)
	{
		status = action_state_change(txid_str, 3);
	}
	else
	{
		const char *err_msg =
				"Specify mode: --watch, --put, --get, --ls, "
				"--rm, --mv, --suspend, --resume, --cancel, "
				"or --report\n";
		safe_write(STDERR_FILENO, err_msg,
				istrlen(err_msg, MAX_LOG_OUTPUT_LEN));
		status = EXIT_FAILURE;
	}

	g_attached = 0;
	cfdp_detach();

#ifdef HAVE_WORKING_FORK
	/* Remove PID file on exit only if this instance created it */
	if (mode_watch && pid_created)
	{
		unlink(pid_path);
	}
#endif

	if (mask_blocked)
	{
		pthread_sigmask(SIG_SETMASK, &orig_mask, NULL);
	}

	return (status == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#if defined(ION_LWT)
int cfdpctl(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5, saddr a6,
		saddr a7, saddr a8, saddr a9, saddr a10)
{
	char  prog_name[] = "cfdpctl";
	saddr raw_args[10];
	char *argv[12];
	int   argc = 1;
	int   i;

	raw_args[0] = a1;
	raw_args[1] = a2;
	raw_args[2] = a3;
	raw_args[3] = a4;
	raw_args[4] = a5;
	raw_args[5] = a6;
	raw_args[6] = a7;
	raw_args[7] = a8;
	raw_args[8] = a9;
	raw_args[9] = a10;

	argv[0] = prog_name;
	for (i = 0; i < 10; i++)
	{
		if (raw_args[i] == 0)
		{
			break;
		}

		argv[argc++] = (char *) raw_args[i];
	}

	argv[argc] = NULL;
	return cfdpctl_main(argc, argv);
}
#else
int main(int argc, char *argv[])
{
	return cfdpctl_main(argc, argv);
}
#endif
