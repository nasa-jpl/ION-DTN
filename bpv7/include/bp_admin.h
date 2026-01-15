/*
 * bp_admin.h - Minimum set of administrative functions that work with
 *              existing bp_attach and bp_detach to
 *              to programmatically configure and start/stop the BP
 *              system without using the bpadmin utility.
 */

#ifndef BP_ADMIN_H
#define BP_ADMIN_H

#include "bp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PREREQUISITES:
 * - Must call bp_attach() first (from bp.h)
 * - These functions configure the BP system
 * - Can be called from same process as bp_send/receive
 */

 /*
 * ===========================================================================
 * Managing BP Schemes: Configuration vs. State
 * ===========================================================================
 *
 * When managing Bundle Protocol (BP) schemes in ION, it is crucial to
 * distinguish between operations that modify the system's configuration
 * and those that change a scheme's operational state. Performing these
 * operations at the wrong time can lead to system instability.
 *
 * ---------------------------------------------------------------------------
 * Configuration Changes (add/remove)
 * ---------------------------------------------------------------------------
 *
 * Functions like addScheme() and removeScheme() are structural configuration
 * changes. They create or permanently destroy a scheme's underlying data
 * structures, including its forward queue and endpoint lists.
 *
 * WARNING: These operations are not safe to perform while the main BP agent
 * (e.g., bptransit) is running. Doing so creates a race condition where the
 * BP agent might try to access a data structure that is being simultaneously
 * deleted, which can lead to a crash or memory corruption.
 *
 * Correct Usage: Scheme configuration changes should ONLY be made when the
 * main BP agent is stopped (i.e., between a call to bpStop() and bpStart()).
 *
 * ---------------------------------------------------------------------------
 * State Changes (start/stop)
 * ---------------------------------------------------------------------------
 *
 * Functions like bpStartScheme() and bpStopScheme() are operational state
 * changes. They are designed to be used while the main BP agent is running.
 *
 * o bpStopScheme():
 * This function safely terminates a scheme's specific forwarder process
 * (e.g., ipnfw) but leaves its data structures intact. The main bptransit
 * daemon can still route bundles to the scheme's forward queue, where
 * they will wait safely until the scheme is restarted. This temporarily
 * suspends forwarding for the scheme without causing system instability.
 *
 * o bpStartScheme():
 * This function starts a scheme's forwarder process. It is the intended
 * method for dynamically activating or restarting a single scheme
 * without stopping the entire BP node. The newly started process will
 * begin servicing its forward queue, processing any bundles that may
 * have accumulated.
 *
 * ---------------------------------------------------------------------------
 * Summary
 * ---------------------------------------------------------------------------
 *
 * In summary, use bpStart() and bpStop() to manage the lifecycle of the
 * entire BP agent. Use addScheme() and removeScheme() only when the agent
 * is stopped. Use bpStartScheme() and bpStopScheme() to dynamically manage
 * the operational state of individual schemes while the agent is running.
 */

/* Endpoint Structure */
typedef enum
{
	DiscardBundle = 0,
	EnqueueBundle
} BpRecvRule;

/* System lifecycle - only if you're doing EVERYTHING programmatically */

/*
 * Initialize the Bundle Protocol subsystem.
 * Creates or recovers BP database in SDR.
 * Must be called after ionAttach().
 *
 * Returns: 0 on success, -1 on error
 */
extern int bp_init(void);

/*
 * Start the BP agent and all its daemons.
 * Launches bpclock, bptransit, and other BP system tasks.
 *
 * Returns: 0 on success, -1 on error
 */
extern int bp_start(void);

/*
 * Stop the BP agent and all its daemons.
 * Terminates all BP system tasks and cleans up resources.
 */
extern void bp_stop(void);

/*
 * Initialize the IPN (InterPlanetary Network) routing database.
 * Creates or recovers IPN-specific routing tables in SDR.
 * Must be called after bp_init() for IPN scheme support.
 *
 * Returns: 0 on success, -1 on error
 */
extern int ipn_init(void);

/*
 * Add a new URI scheme to the BP node.
 *
 * Parameters:
 *   name      - Scheme name (e.g., "ipn", "dtn")
 *   fwdCmd    - Forwarder daemon command
 *   admAppCmd - Admin application command
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_scheme(char *name, char *fwdCmd, char *admAppCmd);

/*
 * Remove a URI scheme from the BP node.
 * Scheme must not have any endpoints registered.
 *
 * Parameters:
 *   name - Scheme name to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_scheme(char *name);

/* Scheme lifecycle control */

/*
 * Start a scheme's forwarder daemon.
 * Launches the forwarder for an existing scheme.
 *
 * Parameters:
 *   name - Scheme name to start
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int bp_start_scheme(char *name);

/*
 * Stop a scheme's forwarder daemon.
 * Terminates the forwarder for the specified scheme.
 *
 * Parameters:
 *   name - Scheme name to stop
 */
extern void bp_stop_scheme(char *name);

/*
 * Register an endpoint for receiving bundles.
 *
 * Parameters:
 *   eid      - Endpoint ID string (e.g., "ipn:1.1")
 *   recvRule - Reception rule (DiscardBundle or EnqueueBundle)
 *   script   - Optional script to run on bundle arrival (NULL if none)
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_endpoint(char *eid, BpRecvRule recvRule, char *script);

/*
 * Unregister an endpoint.
 * Removes the endpoint and stops any associated application.
 *
 * Parameters:
 *   eid - Endpoint ID string to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_endpoint(char *eid);

/* Protocol management */

/*
 * Add a convergence layer protocol to the BP node.
 *
 * Parameters:
 *   protocol_name  - Protocol name (e.g., "tcp", "udp", "ltp")
 *   protocol_class - Protocol class:
 *                    0 = Scheduled (contact graph routing)
 *                    1 = Continuous (always available)
 *                    2 = Best-effort (unreliable, e.g., UDP)
 *                    10 = Reliable (e.g., TCP, LTP)
 *                    26 = Any (accepts any traffic class)
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_protocol(char *protocol_name, int protocol_class);

/*
 * Remove a convergence layer protocol from the BP node.
 * Protocol must not have any inducts or outducts.
 *
 * Parameters:
 *   protocol_name - Protocol name to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_protocol(char *protocol_name);

/*
 * Start a protocol's convergence layer adapter.
 *
 * Parameters:
 *   protocol_name - Protocol name to start
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int bp_start_protocol(char *protocol_name);

/*
 * Stop a protocol's convergence layer adapter.
 *
 * Parameters:
 *   protocol_name - Protocol name to stop
 */
extern void bp_stop_protocol(char *protocol_name);

/* Induct management */

/*
 * Add an induct (data reception duct) for a protocol.
 *
 * Parameters:
 *   protocol_name - Protocol name (must already exist)
 *   duct_name     - Induct name/identifier (e.g., local address/port)
 *   cli_command   - Command to start convergence layer input task
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_induct(char *protocol_name, char *duct_name, char *cli_command);

/*
 * Remove an induct from a protocol.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Induct name to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_induct(char *protocol_name, char *duct_name);

/*
 * Start an induct's convergence layer input task.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Induct name to start
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int bp_start_induct(char *protocol_name, char *duct_name);

/*
 * Stop an induct's convergence layer input task.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Induct name to stop
 */
extern void bp_stop_induct(char *protocol_name, char *duct_name);

/* Outduct management */

/*
 * Add an outduct (data transmission duct) for a protocol.
 *
 * Parameters:
 *   protocol_name       - Protocol name (must already exist)
 *   duct_name           - Outduct name/identifier (e.g., remote address/port)
 *   clo_command         - Command to start convergence layer output task
 *   max_payload_length  - Maximum payload size in bytes (0 = use default)
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_outduct(char *protocol_name, char *duct_name,
                       char *clo_command, unsigned int max_payload_length);

/*
 * Remove an outduct from a protocol.
 * Outduct must not be attached to any plan.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Outduct name to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_outduct(char *protocol_name, char *duct_name);

/*
 * Start an outduct's convergence layer output task.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Outduct name to start
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int bp_start_outduct(char *protocol_name, char *duct_name);

/*
 * Stop an outduct's convergence layer output task.
 *
 * Parameters:
 *   protocol_name - Protocol name
 *   duct_name     - Outduct name to stop
 */
extern void bp_stop_outduct(char *protocol_name, char *duct_name);

/* Egress plan management */

/*
 * Add an egress plan for routing bundles to a destination.
 *
 * Parameters:
 *   eid          - Destination endpoint ID pattern (e.g., "ipn:2.*")
 *   nominal_rate - Expected transmission rate in bytes/sec (0 = unlimited)
 *
 * Returns: 1 on success, 0 if duplicate, -1 on error
 */
extern int add_plan(char *eid, unsigned int nominal_rate);

/*
 * Remove an egress plan.
 * Plan must not have any outducts attached.
 *
 * Parameters:
 *   eid - Destination endpoint ID pattern to remove
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int remove_plan(char *eid);

/*
 * Start an egress plan.
 *
 * Parameters:
 *   eid - Destination endpoint ID pattern
 *
 * Returns: 1 on success, 0 if not found, -1 on error
 */
extern int bp_start_plan(char *eid);

/*
 * Stop an egress plan.
 *
 * Parameters:
 *   eid - Destination endpoint ID pattern
 */
extern void bp_stop_plan(char *eid);

/* Planduct (plan-outduct attachment) management */

/*
 * Attach an outduct to an egress plan.
 * This creates a "planduct" - a routing directive that tells BP
 * to use a specific outduct when forwarding to a destination.
 *
 * Parameters:
 *   eid           - Destination endpoint ID pattern
 *   protocol_name - Protocol name of the outduct
 *   duct_name     - Outduct name
 *
 * Returns: 1 on success, 0 if outduct already attached or not found, -1 on error
 */
extern int add_planduct(char *eid, char *protocol_name, char *duct_name);

/*
 * Detach an outduct from its egress plan.
 *
 * Parameters:
 *   protocol_name - Protocol name of the outduct
 *   duct_name     - Outduct name to detach
 *
 * Returns: 1 on success, 0 if outduct not attached, -1 on error
 */
extern int remove_planduct(char *protocol_name, char *duct_name);

/* Monitoring */

/*
 * List all registered URI schemes.
 * Displays scheme names, code numbers, admin endpoints,
 * and process IDs for forwarder and admin daemons.
 *
 * Note: Future versions will return structured data.
 */
extern void bp_list_schemes(void);

/*
 * List all registered endpoints across all schemes.
 * Displays endpoint IDs, receive rules, application PIDs,
 * and associated scripts.
 *
 * Note: Future versions will return structured data.
 */
extern void bp_list_endpoints(void);

/*
 * List all convergence layer protocols.
 * Displays protocol names and their classes
 * (Scheduled, Unscheduled, or OnDemand).
 *
 * Note: Future versions will return structured data.
 */
extern void bp_list_protocols(void);

/*
 * Print statistics for all bundle states.
 * Displays counts and totals for sourced, received, forwarded,
 * delivered, expired, and other bundle state transitions.
 */
extern void report_all_state_stats(void);

/* Bulk removal operations */

/*
 * Remove all endpoints registered under a scheme.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Endpoints with pending bundles or open applications will be skipped.
 * Use bp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Parameters:
 *   scheme - Scheme name ("ipn" or "dtn")
 *
 * Returns: Number of endpoints successfully removed (>= 0), -1 on fatal error
 */
extern int bp_remove_all_endpoints(char *scheme);

/*
 * Remove all inducts for a protocol.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Use bp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Parameters:
 *   protocol - Protocol name (e.g., "ltp", "tcp", "udp")
 *
 * Returns: Number of inducts successfully removed (>= 0), -1 on fatal error
 */
extern int bp_remove_all_inducts(char *protocol);

/*
 * Remove all outducts for a protocol.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Outducts attached to plans will be skipped.
 * Use bp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Parameters:
 *   protocol - Protocol name (e.g., "ltp", "tcp", "udp")
 *
 * Returns: Number of outducts successfully removed (>= 0), -1 on fatal error
 */
extern int bp_remove_all_outducts(char *protocol);

/*
 * Remove all IPN egress plans.
 * Useful for runtime reconfiguration or cleanup.
 *
 * Plans with pending bundles in queues will be skipped.
 * Use bp_stop() + ionTerminate(1) for complete shutdown instead.
 *
 * Returns: Number of plans successfully removed (>= 0), -1 on fatal error
 */
extern int ipn_remove_all_plans(void);

#ifdef __cplusplus
}
#endif

#endif /* BP_ADMIN_H */
