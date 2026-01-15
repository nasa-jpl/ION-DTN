/*
 * ltp_admin.h
 *
 * Public API for LTP (Licklider Transmission Protocol) Administration
 *
 * This header provides public interfaces for managing LTP configuration
 * that are currently accessible only through the ltpadmin CLI.
 *
 * Copyright (c) 2025, California Institute of Technology.
 * ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 */

#ifndef LTP_ADMIN_H
#define LTP_ADMIN_H

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ===========================================================================
 * LTP Configuration Management
 * ===========================================================================
 *
 * LTP (Licklider Transmission Protocol) provides reliable data transmission
 * over long-delay, high-loss links. It manages communication with remote
 * LTP engines through "spans" and receives data through "seats".
 *
 * Key Concepts:
 * - Span: Configuration for transmitting to a remote LTP engine
 * - Seat: Configuration for receiving from remote LTP engines
 * - LSO: Link Service Output (transmission daemon, one per span)
 * - LSI: Link Service Input (reception daemon, one per seat)
 *
 * Runtime Safety:
 * Unlike BP scheme operations, LTP span and seat operations are SAFE to
 * perform while LTP is running. They use SDR transactions for synchronization
 * and automatically manage LSO/LSI process lifecycles.
 *
 * See LTP_RUNTIME_SAFETY_ANALYSIS.md for detailed analysis.
 */

/* System lifecycle */

/*
 * Initialize the LTP engine.
 * Creates or recovers LTP database in SDR.
 * Must be called after ionAttach().
 *
 * Parameters:
 *   est_max_sessions - Estimated maximum number of concurrent sessions
 *
 * Returns: 0 on success, -1 on error
 */
extern int ltp_init(unsigned int est_max_sessions);

/*
 * Start the LTP engine and all its daemons.
 * Launches ltpclock, ltpdeliv, and all configured LSO/LSI processes.
 *
 * Returns: 0 on success, -1 on error
 */
extern int ltp_start(void);

/*
 * Stop the LTP engine and all its daemons.
 * Terminates ltpclock, ltpdeliv, and all LSO/LSI processes.
 */
extern void ltp_stop(void);

/* Span management (remote LTP engines) */

/*
 * Add a span to a remote LTP engine.
 * A span defines how to transmit data to a specific remote LTP engine.
 *
 * Parameters:
 *   engine_id              - Remote LTP engine identifier (typically IPN node number)
 *   max_export_sessions    - Maximum concurrent outbound sessions to this engine
 *   max_import_sessions    - Maximum concurrent inbound sessions from this engine
 *   max_segment_size       - Maximum LTP segment size in bytes
 *   aggr_size_limit        - Block aggregation size threshold in bytes
 *   aggr_time_limit        - Block aggregation time limit in seconds
 *   lso_command            - Link Service Output command (e.g., "udplso localhost:1113")
 *   queuing_latency        - Expected transmission queuing delay in seconds
 *   purge_enabled          - Enable automatic session purging (0=no, 1=yes)
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 *
 * Example:
 *   add_span(2, 100, 100, 1400, 10000, 1, "udplso localhost:1113", 1, 0);
 */
extern int add_span(uvast engine_id,
                    unsigned int max_export_sessions,
                    unsigned int max_import_sessions,
                    unsigned int max_segment_size,
                    unsigned int aggr_size_limit,
                    unsigned int aggr_time_limit,
                    char *lso_command,
                    unsigned int queuing_latency,
                    int purge_enabled);

/*
 * Update an existing span's parameters.
 *
 * Parameters: Same as add_span()
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int update_span(uvast engine_id,
                       unsigned int max_export_sessions,
                       unsigned int max_import_sessions,
                       unsigned int max_segment_size,
                       unsigned int aggr_size_limit,
                       unsigned int aggr_time_limit,
                       char *lso_command,
                       unsigned int queuing_latency,
                       int purge_enabled);

/*
 * Remove a span to a remote LTP engine.
 * Automatically stops the LSO process before removal.
 * Span must have no pending segments or open sessions.
 *
 * Parameters:
 *   engine_id - Remote LTP engine identifier
 *
 * Returns: 1 on success, 0 if not found or has pending data, -1 on error
 */
extern int remove_span(uvast engine_id);

/*
 * Start transmission on a span.
 * Starts the LSO process for the specified span.
 *
 * Parameters:
 *   engine_id - Remote LTP engine identifier
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int ltp_start_span(uvast engine_id);

/*
 * Stop transmission on a span.
 * Stops the LSO process for the specified span.
 *
 * Parameters:
 *   engine_id - Remote LTP engine identifier
 */
extern void ltp_stop_span(uvast engine_id);

/* Seat management (local receivers) */

/*
 * Add a seat (link service input).
 * A seat defines how to receive data from remote LTP engines.
 *
 * Parameters:
 *   lsi_command - Link Service Input command (e.g., "udplsi localhost:1113")
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 *
 * Example:
 *   add_seat("udplsi localhost:1113");
 */
extern int add_seat(char *lsi_command);

/*
 * Remove a seat (link service input).
 * Automatically stops the LSI process before removal.
 *
 * Parameters:
 *   lsi_command - Link Service Input command to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_seat(char *lsi_command);

/* Monitoring */

/*
 * List all configured spans.
 * Displays span engine IDs, session limits, and LSO commands.
 *
 * Note: Future versions will return structured data.
 */
extern void ltp_list_spans(void);

/*
 * List all configured seats.
 * Displays LSI commands and process IDs.
 *
 * Note: Future versions will return structured data.
 */
extern void ltp_list_seats(void);

extern void ltp_print_span_sessions(uvast engine_id);

/* Bulk removal operations */

/*
 * Remove all LTP spans.
 * Stops LSO processes and removes all configured spans.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Spans with pending data (segments, sessions) will be skipped.
 * Use ltp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Returns: Number of spans successfully removed (>= 0), -1 on fatal error
 */
extern int ltp_remove_all_spans(void);

/*
 * Remove all LTP seats.
 * Stops LSI processes and removes all configured seats.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Use ltp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Returns: Number of seats successfully removed (>= 0), -1 on fatal error
 */
extern int ltp_remove_all_seats(void);

#ifdef __cplusplus
}
#endif

#endif /* LTP_ADMIN_H */
