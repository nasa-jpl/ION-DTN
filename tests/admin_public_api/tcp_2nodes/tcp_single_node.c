/*
 * tcp_single_node.c
 *
 * Configures a single ION node for multi-node operation via TCPCL.
 *
 * IMPORTANT: Multi-node operation requires SEPARATE PROCESSES.
 *
 * ION's architecture uses process-wide static variables and a single SDR system
 * per process. Therefore, you CANNOT run multiple ION nodes in the same process.
 * Each node must be instantiated by running this program in a separate process
 * (e.g., in different terminals or as separate daemon processes).
 *
 * Why separate processes are required:
 * - SDR (Space Data Router) system is process-wide (sdr_initialize)
 * - IPC system maintains process-level state (sm_ipc_init)
 * - Static variables track single node attachment (_ionsdr, _ionwm, _bpvdb)
 * - Shared memory segments are identified by unique keys per node
 *
 * Usage:
 *   ./tcp_single_node <node_num> <wmKey> <sdrName> <work_dir> \\
 *                     <local_ip> <tcp_port> <peer_node> <peer_ip> <peer_tcp_port>
 *
 * Parameters:
 *   node_num       - Node number (IPN node ID)
 *   wmKey          - Shared memory key for ION working memory (must be unique per node)
 *   sdrName        - SDR database name (must be unique per node)
 *   work_dir       - Working directory for this node
 *   local_ip       - IP address to bind to (use localhost or specific IP)
 *   tcp_port       - TCP port for this node to listen on
 *   peer_node      - Peer node number
 *   peer_ip        - Peer's IP address
 *   peer_tcp_port  - Peer's TCP port to connect to
 *
 * Note: wmKey is required for daemon processes to attach to shared memory.
 * SDR heap and log keys are auto-allocated for proper cleanup.
 *
 * Example for 2 nodes on localhost:
 *   Terminal 1: ./tcp_single_node 1 65536 ion1 /tmp/ion_node1 localhost 4556 2 localhost 4557
 *   Terminal 2: ./tcp_single_node 2 65537 ion2 /tmp/ion_node2 localhost 4557 1 localhost 4556
 *
 * Example for 2 nodes on different hosts:
 *   Host1: ./tcp_single_node 1 65536 ion1 /tmp/ion_node1 192.168.1.10 4556 2 192.168.1.20 4556
 *   Host2: ./tcp_single_node 2 65537 ion2 /tmp/ion_node2 192.168.1.20 4556 1 192.168.1.10 4556
 *
 * Compile:
 *   gcc -o tcp_single_node tcp_single_node.c -lici -lbp -lpthread -lm -lrt
 */

#include "ion.h"
#include "ion_admin.h"
#include "ionsec.h"
#include "bp.h"
#include "bp_admin.h"
#include "rfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

/* Color codes */
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RED "\x1b[31m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_RESET "\x1b[0m"

static void print_usage(const char *prog_name)
{
	printf("\n");
	printf(COLOR_CYAN "========================================\n");
	printf("  ION Single Node Configuration Tool\n");
	printf("========================================" COLOR_RESET "\n\n");

	printf(COLOR_MAGENTA "IMPORTANT: Multi-node operation requires SEPARATE PROCESSES!" COLOR_RESET "\n\n");
	printf("Each ION node must run in its own process. You cannot instantiate\n");
	printf("multiple nodes in a single process due to ION's architecture:\n");
	printf("  - Process-wide SDR (Space Data Router) system\n");
	printf("  - Process-level IPC state management\n");
	printf("  - Static variables for node attachment\n\n");

	printf("Usage:\n");
	printf("  %s <node_num> <wmKey> <sdrName> <work_dir> \\\n", prog_name);
	printf("                         <local_ip> <tcp_port> <peer_node> <peer_ip> <peer_tcp_port>\n\n");

	printf("Parameters:\n");
	printf("  node_num       - Node number (IPN node ID)\n");
	printf("  wmKey          - Shared memory key (must be unique per node)\n");
	printf("  sdrName        - SDR database name (must be unique per node)\n");
	printf("  work_dir       - Working directory for this node\n");
	printf("  local_ip       - IP address to bind to (localhost or specific IP)\n");
	printf("  tcp_port       - TCP port for this node to listen on\n");
	printf("  peer_node      - Peer node number\n");
	printf("  peer_ip        - Peer's IP address\n");
	printf("  peer_tcp_port  - Peer's TCP port to connect to\n\n");

}

static void print_step(const char *msg)
{
	printf(COLOR_BLUE "==> %s" COLOR_RESET "\n", msg);
}

static void print_info(const char *msg)
{
	printf(COLOR_YELLOW "    [INFO] %s" COLOR_RESET "\n", msg);
}

static void print_success(const char *msg)
{
	printf(COLOR_GREEN "    [SUCCESS] %s" COLOR_RESET "\n", msg);
}

static void print_error(const char *msg)
{
	printf(COLOR_RED "    [ERROR] %s" COLOR_RESET "\n", msg);
}

int main(int argc, char *argv[])
{
	IonParms parms;
	time_t now;
	char buffer[512];
	char program_dir[256];

	/* Command line arguments */
	int node_num;
	int wmKey;
	char *sdrName;
	char *work_dir;
	char *local_ip;
	int tcp_port;
	int peer_node;
	char *peer_ip;
	int peer_tcp_port;

	char local_addr[128];
	char peer_addr[128];

	/* Parse arguments */
	if (argc != 10)
	{
		print_usage(argv[0]);
		return 1;
	}

	node_num = atoi(argv[1]);
	wmKey = atoi(argv[2]);
	sdrName = argv[3];
	work_dir = argv[4];
	local_ip = argv[5];
	tcp_port = atoi(argv[6]);
	peer_node = atoi(argv[7]);
	peer_ip = argv[8];
	peer_tcp_port = atoi(argv[9]);

	/* Validate inputs */
	if (node_num <= 0)
	{
		print_error("node_num must be positive");
		return 1;
	}
	if (wmKey <= 0)
	{
		print_error("wmKey must be positive");
		return 1;
	}
	if (strlen(sdrName) == 0)
	{
		print_error("sdrName cannot be empty");
		return 1;
	}
	if (strlen(local_ip) == 0)
	{
		print_error("local_ip cannot be empty");
		return 1;
	}
	if (strlen(peer_ip) == 0)
	{
		print_error("peer_ip cannot be empty");
		return 1;
	}
	if (tcp_port <= 0 || tcp_port > 65535)
	{
		print_error("tcp_port must be between 1 and 65535");
		return 1;
	}
	if (peer_tcp_port <= 0 || peer_tcp_port > 65535)
	{
		print_error("peer_tcp_port must be between 1 and 65535");
		return 1;
	}

	/* Format addresses */
	snprintf(local_addr, sizeof(local_addr), "%s:%d", local_ip, tcp_port);
	snprintf(peer_addr, sizeof(peer_addr), "%s:%d", peer_ip, peer_tcp_port);

	printf("\n");
	printf(COLOR_CYAN "========================================\n");
	printf("  ION Node %d Configuration\n", node_num);
	printf("========================================" COLOR_RESET "\n\n");

	printf(COLOR_MAGENTA "MULTI-NODE NOTICE:" COLOR_RESET " This process will instantiate Node %d.\n", node_num);
	printf("Each node MUST run in a separate process.\n\n");

	printf("Configuration:\n");
	printf("  Node number:      %d\n", node_num);
	printf("  WM Key:           %d\n", wmKey);
	printf("  SDR Name:         %s\n", sdrName);
	printf("  Working dir:      %s\n", work_dir);
	printf("  Local address:    %s\n", local_addr);
	printf("  Peer node:        %d\n", peer_node);
	printf("  Peer address:     %s\n\n", peer_addr);

	/* Get program directory for ION_NODE_LIST_DIR */
	if (getcwd(program_dir, sizeof(program_dir)) == NULL)
	{
		print_error("Cannot get current directory");
		return 1;
	}

	print_step("Setup environment");
	snprintf(buffer, sizeof(buffer), "Program directory: %s", program_dir);
	print_info(buffer);

	/* Create working directory */
	if (mkdir(work_dir, 0755) != 0 && errno != EEXIST)
	{
		snprintf(buffer, sizeof(buffer), "Cannot create directory %s", work_dir);
		print_error(buffer);
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Working directory: %s", work_dir);
	print_info(buffer);

	/* Change to working directory */
	if (chdir(work_dir) != 0)
	{
		snprintf(buffer, sizeof(buffer), "Cannot chdir to %s", work_dir);
		print_error(buffer);
		return 1;
	}
	print_success("Changed to working directory");

	/* Set environment variables */
	if (setenv("ION_NODE_LIST_DIR", program_dir, 1) != 0)
	{
		print_error("Failed to set ION_NODE_LIST_DIR");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "ION_NODE_LIST_DIR=%s", program_dir);
	print_success(buffer);

	if (setenv("ION_NODE_WDNAME", work_dir, 1) != 0)
	{
		print_error("Failed to set ION_NODE_WDNAME");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "ION_NODE_WDNAME=%s", work_dir);
	print_success(buffer);

	/* Initialize ION */
	print_step("Initialize ION");
	memset(&parms, 0, sizeof(IonParms));
	parms.wmKey = wmKey;  /* Explicit key needed for daemon attachment */
	parms.wmSize = 5000000;
	parms.wmAddress = NULL;
	istrcpy(parms.sdrName, sdrName, sizeof(parms.sdrName));
	parms.sdrWmSize = 1000000;
	parms.configFlags = SDR_IN_DRAM | SDR_BOUNDED;
	parms.heapWords = 250000;
	parms.heapKey = SM_NO_KEY;  /* Auto-allocate for private memory (will be destroyed) */
	parms.logSize = 0;
	parms.logKey = SM_NO_KEY;  /* Auto-allocate for private memory (will be destroyed) */
	istrcpy(parms.pathName, "/tmp", sizeof(parms.pathName));

	snprintf(buffer, sizeof(buffer), "ionInitialize(nodeNum=%d, wmKey=%d, sdrName=%s)",
	         node_num, wmKey, sdrName);
	print_info(buffer);

	if (ionInitialize(&parms, node_num) < 0)
	{
		print_error("ionInitialize failed");
		return 1;
	}
	print_success("ION initialized");

	if (ionAttach() < 0)
	{
		print_error("ionAttach failed");
		return 1;
	}
	print_success("ION attached");

	if (ion_register_node(1) < 0)
	{
		print_error("ion_register_node failed");
		return 1;
	}
	print_success("Node registered in region 1");

	/* Initialize ION Security */
	print_step("Initialize ION Security");
	if (secInitialize() < 0)
	{
		print_error("secInitialize failed");
		return 1;
	}
	print_success("ION Security initialized");

	if (secAttach() < 0)
	{
		print_error("secAttach failed");
		return 1;
	}
	print_success("ION Security attached (empty database)");

	/* Start RFX */
	print_step("Start RFX");
	rfx_start();
	sleep(2);
	if (!rfx_system_is_started())
	{
		print_error("RFX system did not start");
		return 1;
	}
	print_success("RFX started");

	/* Add contacts and ranges */
	print_step("Add contacts and ranges");
	now = time(NULL);

	/* Self contact - start 2 seconds in future to allow RFX to stabilize */
	if (ion_add_contact(now + 2, now + 86400, node_num, node_num, 1000000, 1.0) < 0)
	{
		snprintf(buffer, sizeof(buffer), "Failed to add self contact (Node %d -> Node %d)",
				node_num, node_num);
		print_error(buffer);
		print_error("Check ion.log for details");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Contact added (Node %d -> Node %d)", node_num, node_num);
	print_success(buffer);

	if (ion_add_range(now + 2, now + 86400, node_num, node_num, 1) < 0)
	{
		print_error("Failed to add self range");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Range added (Node %d -> Node %d)", node_num, node_num);
	print_success(buffer);

	/* Peer contact */
	if (ion_add_contact(now + 2, now + 86400, node_num, peer_node, 1000000, 1.0) < 0)
	{
		print_error("Failed to add peer contact");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Contact added (Node %d -> Node %d)", node_num, peer_node);
	print_success(buffer);

	if (ion_add_range(now + 2, now + 86400, node_num, peer_node, 1) < 0)
	{
		print_error("Failed to add peer range");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Range added (Node %d -> Node %d)", node_num, peer_node);
	print_success(buffer);

	/* Initialize BP */
	print_step("Initialize Bundle Protocol");
	if (bp_init() < 0)
	{
		print_error("bp_init failed");
		return 1;
	}
	print_success("BP initialized");

	if (bp_attach() < 0)
	{
		print_error("bp_attach failed");
		return 1;
	}
	print_success("BP attached");

	/* Configure IPN scheme */
	print_step("Configure IPN scheme");
	if (add_scheme("ipn", "ipnfw", "ipnadminep") <= 0)
	{
		print_error("Failed to add ipn scheme");
		return 1;
	}
	print_success("IPN scheme added");

	/* Add endpoints */
	snprintf(buffer, sizeof(buffer), "ipn:%d.0", node_num);
	if (add_endpoint(buffer, EnqueueBundle, NULL) <= 0)
	{
		print_error("Failed to add endpoint .0");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Endpoint ipn:%d.0 added", node_num);
	print_success(buffer);

	snprintf(buffer, sizeof(buffer), "ipn:%d.1", node_num);
	if (add_endpoint(buffer, EnqueueBundle, NULL) <= 0)
	{
		print_error("Failed to add endpoint .1");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Endpoint ipn:%d.1 added", node_num);
	print_success(buffer);

	/* Configure TCP */
	print_step("Configure TCP convergence layer");
	if (add_protocol("tcp", 10) <= 0)
	{
		print_error("Failed to add tcp protocol");
		return 1;
	}
	print_success("TCP protocol added");

	/* Add TCP induct */
	if (add_induct("tcp", local_addr, "tcpcli") <= 0)
	{
		print_error("Failed to add TCP induct");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "TCP induct added on %s", local_addr);
	print_success(buffer);

	/* Add TCP outduct to peer */
	if (add_outduct("tcp", peer_addr, "tcpclo", 0) <= 0)
	{
		print_error("Failed to add TCP outduct");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "TCP outduct added to %s", peer_addr);
	print_success(buffer);

	/* Add egress plan to peer */
	snprintf(buffer, sizeof(buffer), "ipn:%d.0", peer_node);
	if (add_plan(buffer, 0) <= 0)
	{
		print_error("Failed to add plan");
		return 1;
	}
	snprintf(buffer, sizeof(buffer), "Egress plan added for ipn:%d.0", peer_node);
	print_success(buffer);

	/* Attach outduct to plan */
	snprintf(buffer, sizeof(buffer), "ipn:%d.0", peer_node);
	if (add_planduct(buffer, "tcp", peer_addr) <= 0)
	{
		print_error("Failed to attach outduct to plan");
		return 1;
	}
	print_success("Outduct attached to plan");

	/* Start BP */
	print_step("Start Bundle Protocol");
	if (bp_start() < 0)
	{
		print_error("bp_start failed");
		return 1;
	}
	print_success("BP started");

	sleep(2);

	/* Display status */
	printf("\n");
	printf(COLOR_GREEN "========================================\n");
	printf("  Node %d Successfully Started\n", node_num);
	printf("========================================" COLOR_RESET "\n\n");

	printf("Node Information:\n");
	printf("  Node number:      %d\n", node_num);
	printf("  Endpoints:        ipn:%d.0, ipn:%d.1\n", node_num, node_num);
	printf("  TCP listening:    %s\n", local_addr);
	printf("  Can send to:      ipn:%d.* via %s\n", peer_node, peer_addr);
	printf("  Log file:         %s/ion.log\n", work_dir);
	printf("  ion_nodes file:   %s/ion_nodes\n\n", program_dir);

	printf(COLOR_CYAN "To interact with this node from other terminals:\n" COLOR_RESET);
	printf("  export ION_NODE_LIST_DIR=%s\n", program_dir);
	printf("  export ION_NODE_WDNAME=%s\n", work_dir);

	printf(COLOR_GREEN "[SUCCESS] Node %d configured and running\n" COLOR_RESET, node_num);

	/* For automated testing, let node run briefly then exit cleanly */
	printf(COLOR_YELLOW "[INFO] Node will run for 10 seconds to verify stability...\n" COLOR_RESET);
	sleep(10);

	/* Clean shutdown */
	printf(COLOR_YELLOW "\n[INFO] Shutting down Node %d...\n" COLOR_RESET, node_num);

	print_step("Stop Bundle Protocol");
	bp_stop();
	sleep(2);
	print_success("BP stopped");

	print_step("Stop RFX");
	rfx_stop();
	sleep(1);
	print_success("RFX stopped");

	/*
	 * Terminate ION and clean up resources.
	 *
	 * Note: ionTerminate(1) destroys the SDR and detaches from ION working memory.
	 * However, in multi-node scenarios where parms.wmKey is explicitly set (required
	 * for daemon attachment), some IPC resources like the semBase semaphore may not
	 * be fully destroyed. The launcher will clean up any remaining IPC resources
	 * after both nodes exit.
	 */
	print_step("Terminate ION (remove SDR and IPC)");
	ionTerminate(1);  /* 1 = full cleanup including IPC removal */
	sleep(1);
	print_success("ION terminated");

	printf(COLOR_GREEN "\n[SUCCESS] Node %d shutdown cleanly\n" COLOR_RESET, node_num);

	return 0;
}
