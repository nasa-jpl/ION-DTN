/*
 * tcp_2nodes_launcher.c
 *
 * Test launcher for running two ION nodes on localhost using TCP convergence layer.
 * This program spawns two separate processes, each running tcp_single_node, to
 * demonstrate multi-node ION operation.
 *
 * IMPORTANT: Multi-node operation requires SEPARATE PROCESSES.
 * Each ION node must run in its own process because ION uses process-wide static
 * variables and a single SDR system per process.
 *
 * ARCHITECTURE:
 * - Node 1: wmKey=65536, sdrName=ion1, workDir=tcp_2nodes/node1, TCP port 4556
 * - Node 2: wmKey=65537, sdrName=ion2, workDir=tcp_2nodes/node2, TCP port 4557
 * - Each node runs in its own process (forked from this launcher)
 * - Nodes communicate via TCPCL on localhost
 *
 * USAGE:
 *   ./dotest
 *
 * The launcher will:
 * 1. Create working directories for both nodes
 * 2. Fork and exec node 1 process (using tcp_single_node)
 * 3. Fork and exec node 2 process (using tcp_single_node)
 * 4. Wait for both nodes to complete
 * 5. Report results
 */

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

/*
 * Suppress format-truncation warnings for GCC 7+.
 * This code has explicit length checks before all snprintf() calls,
 * but GCC's static analysis doesn't recognize the runtime checks and
 * issues false-positive warnings. The snprintf calls are safe because:
 * 1. All paths use MAXPATHLEN-sized buffers
 * 2. strlen() checks verify the concatenated length won't exceed buffer size
 * 3. Any overflow is caught and reported before snprintf is called
 */
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

/* Color codes for output */
#define COLOR_GREEN "\x1b[32m"

/* Color codes for output */
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

/* Test configuration */
#define NODE1_NUM "1"
#define NODE1_WMKEY "65536"
#define NODE1_SDR "ion1"
#define NODE1_WORKDIR "node1"
#define NODE1_IP "localhost"
#define NODE1_PORT "4556"

#define NODE2_NUM "2"
#define NODE2_WMKEY "65537"
#define NODE2_SDR "ion2"
#define NODE2_WORKDIR "node2"
#define NODE2_IP "localhost"
#define NODE2_PORT "4557"

/* Process tracking */
static pid_t node1_pid = -1;
static pid_t node2_pid = -1;

/*
 * Signal handler for cleanup
 */
void cleanup_handler(int sig)
{
	printf(COLOR_YELLOW "\n[LAUNCHER] Received signal %d, cleaning up...\n" COLOR_RESET, sig);

	if (node1_pid > 0) {
		kill(node1_pid, SIGTERM);
	}
	if (node2_pid > 0) {
		kill(node2_pid, SIGTERM);
	}

	exit(1);
}

/*
 * Clean up old test artifacts
 */
int cleanup_old_test(void)
{
	int ret;

	printf(COLOR_CYAN "[LAUNCHER] Cleaning up old test artifacts...\n" COLOR_RESET);

	/* Remove old node directories */
	ret = system("rm -rf node1 node2 ion_nodes 2>/dev/null");
	if (ret != 0) {
		/* Not a fatal error - directories might not exist */
		printf(COLOR_YELLOW "[LAUNCHER] Warning: cleanup command returned %d\n" COLOR_RESET, ret);
	}

	return 0;
}

/*
 * Create a directory if it doesn't exist
 */
int create_directory(const char *path)
{
	struct stat st = {0};

	if (stat(path, &st) == -1) {
		if (mkdir(path, 0755) != 0) {
			perror("mkdir");
			return -1;
		}
		printf(COLOR_CYAN "[LAUNCHER] Created directory: %s\n" COLOR_RESET, path);
	} else {
		printf(COLOR_CYAN "[LAUNCHER] Directory exists: %s\n" COLOR_RESET, path);
	}

	return 0;
}

/*
 * Get the directory where this executable is located
 */
int get_executable_dir(char *buf, size_t size)
{
	ssize_t len = -1;

	/* Try platform-specific methods to get executable path */
#ifdef __linux__
	len = readlink("/proc/self/exe", buf, size - 1);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
	len = readlink("/proc/curproc/file", buf, size - 1);
#elif defined(__sun) || defined(__CYGWIN__)
	len = readlink("/proc/self/path/a.out", buf, size - 1);
#elif defined(__APPLE__) && defined(__MACH__)
	/* macOS doesn't have procfs, use _NSGetExecutablePath */
	uint32_t bufsize = (uint32_t)size;
	if (_NSGetExecutablePath(buf, &bufsize) == 0) {
		len = strlen(buf);
	}
#else
	/* Fallback: try common paths in order */
	len = readlink("/proc/self/exe", buf, size - 1);
	if (len == -1) {
		len = readlink("/proc/curproc/file", buf, size - 1);
	}
	if (len == -1) {
		len = readlink("/proc/self/path/a.out", buf, size - 1);
	}
#endif

	if (len == -1) {
		perror("readlink");
		return -1;
	}
	buf[len] = '\0';

	/* Get directory name */
	char *dir = dirname(buf);
	memmove(buf, dir, strlen(dir) + 1);

	return 0;
}

/*
 * Spawn a node process
 */
pid_t spawn_node(const char *tcp_single_node_path,
		const char *node_name,
		const char *node_num,
		const char *wmkey,
		const char *sdr,
		const char *workdir,
		const char *local_ip,
		const char *local_port,
		const char *peer_num,
		const char *peer_ip,
		const char *peer_port)
{
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {
		/* Child process - exec tcp_single_node */
		printf(COLOR_BLUE "[%s] Starting node %s...\n" COLOR_RESET, node_name, node_num);

		execl(tcp_single_node_path, "tcp_single_node",
				node_num, wmkey, sdr, workdir,
				local_ip, local_port,
				peer_num, peer_ip, peer_port,
				NULL);

		/* If exec fails */
		perror("execl");
		fprintf(stderr, COLOR_RED "[%s] Failed to exec: %s\n" COLOR_RESET,
				node_name, tcp_single_node_path);
		exit(1);
	}

	/* Parent process */
	printf(COLOR_CYAN "[LAUNCHER] Spawned %s with PID %d\n" COLOR_RESET, node_name, pid);
	return pid;
}

/*
 * Main launcher
 */
int main(void)
{
	int status1 = 0, status2 = 0;
	int exit_code = 0;
	char exe_dir[MAXPATHLEN];
	char tcp_single_node_path[MAXPATHLEN];
	char cwd[MAXPATHLEN];
	char node1_abs_path[MAXPATHLEN];
	char node2_abs_path[MAXPATHLEN];

	/* Setup signal handlers */
	signal(SIGINT, cleanup_handler);
	signal(SIGTERM, cleanup_handler);

	printf(COLOR_BLUE "========================================\n");
	printf("ION Two-Node TCP Test Launcher\n");
	printf("========================================\n" COLOR_RESET);

	/* Get current working directory for absolute paths */
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd");
		return 1;
	}
	printf(COLOR_CYAN "[LAUNCHER] Current directory: %s\n" COLOR_RESET, cwd);

	/* Get the directory where this executable is located */
	if (get_executable_dir(exe_dir, sizeof(exe_dir)) != 0) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Failed to get executable directory\n" COLOR_RESET);
		return 1;
	}
	printf(COLOR_CYAN "[LAUNCHER] Executable directory: %s\n" COLOR_RESET, exe_dir);

	/* Construct path to tcp_single_node */
	if (strlen(exe_dir) + 17 > sizeof(tcp_single_node_path)) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Path too long: %s\n" COLOR_RESET, exe_dir);
		return 1;
	}
	snprintf(tcp_single_node_path, sizeof(tcp_single_node_path),
			"%s/tcp_single_node", exe_dir);

	/* Check if tcp_single_node exists */
	if (access(tcp_single_node_path, X_OK) != 0) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] tcp_single_node not found at: %s\n" COLOR_RESET,
				tcp_single_node_path);
		fprintf(stderr, COLOR_RED "[LAUNCHER] Make sure tcp_single_node is built\n" COLOR_RESET);
		return 1;
	}
	printf(COLOR_CYAN "[LAUNCHER] Found tcp_single_node: %s\n" COLOR_RESET, tcp_single_node_path);

	/* Clean up old test artifacts */
	cleanup_old_test();

	/* Create working directories and get absolute paths */
	if (create_directory(NODE1_WORKDIR) != 0 ||
			create_directory(NODE2_WORKDIR) != 0) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Failed to create working directories\n" COLOR_RESET);
		return 1;
	}

	/* Construct absolute paths for node working directories */
	if (strlen(cwd) + strlen(NODE1_WORKDIR) + 2 > sizeof(node1_abs_path)) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Path too long for node1\n" COLOR_RESET);
		return 1;
	}
	if (strlen(cwd) + strlen(NODE2_WORKDIR) + 2 > sizeof(node2_abs_path)) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Path too long for node2\n" COLOR_RESET);
		return 1;
	}
	snprintf(node1_abs_path, sizeof(node1_abs_path), "%s/%s", cwd, NODE1_WORKDIR);
	snprintf(node2_abs_path, sizeof(node2_abs_path), "%s/%s", cwd, NODE2_WORKDIR);

	printf(COLOR_CYAN "[LAUNCHER] Node 1 absolute path: %s\n" COLOR_RESET, node1_abs_path);
	printf(COLOR_CYAN "[LAUNCHER] Node 2 absolute path: %s\n" COLOR_RESET, node2_abs_path);

	/* Spawn Node 1 */
	printf(COLOR_CYAN "\n[LAUNCHER] Spawning Node 1...\n" COLOR_RESET);
	node1_pid = spawn_node(tcp_single_node_path, "NODE1",
			NODE1_NUM, NODE1_WMKEY, NODE1_SDR, node1_abs_path,
			NODE1_IP, NODE1_PORT,
			NODE2_NUM, NODE2_IP, NODE2_PORT);
	if (node1_pid < 0) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Failed to spawn Node 1\n" COLOR_RESET);
		return 1;
	}

	/* Give Node 1 a moment to initialize */
	sleep(2);

	/* Spawn Node 2 */
	printf(COLOR_CYAN "\n[LAUNCHER] Spawning Node 2...\n" COLOR_RESET);
	node2_pid = spawn_node(tcp_single_node_path, "NODE2",
			NODE2_NUM, NODE2_WMKEY, NODE2_SDR, node2_abs_path,
			NODE2_IP, NODE2_PORT,
			NODE1_NUM, NODE1_IP, NODE1_PORT);
	if (node2_pid < 0) {
		fprintf(stderr, COLOR_RED "[LAUNCHER] Failed to spawn Node 2\n" COLOR_RESET);
		kill(node1_pid, SIGTERM);
		return 1;
	}

	/* Wait for both nodes to complete */
	printf(COLOR_CYAN "\n[LAUNCHER] Waiting for nodes to complete...\n" COLOR_RESET);

	waitpid(node1_pid, &status1, 0);
	waitpid(node2_pid, &status2, 0);

	/* Check results */
	printf(COLOR_BLUE "\n========================================\n");
	printf("Test Results\n");
	printf("========================================\n" COLOR_RESET);

	if (WIFEXITED(status1)) {
		int exit_status = WEXITSTATUS(status1);
		if (exit_status == 0) {
			printf(COLOR_GREEN "[NODE1] Completed successfully\n" COLOR_RESET);
		} else {
			printf(COLOR_RED "[NODE1] Failed with exit code %d\n" COLOR_RESET, exit_status);
			exit_code = 1;
		}
	} else if (WIFSIGNALED(status1)) {
		int sig = WTERMSIG(status1);
		if (sig == SIGTERM || sig == SIGINT) {
			printf(COLOR_YELLOW "[NODE1] Stopped by signal (user interrupt)\n" COLOR_RESET);
		} else {
			printf(COLOR_RED "[NODE1] Terminated by signal %d\n" COLOR_RESET, sig);
			exit_code = 1;
		}
	} else {
		printf(COLOR_RED "[NODE1] Terminated abnormally\n" COLOR_RESET);
		exit_code = 1;
	}

	if (WIFEXITED(status2)) {
		int exit_status = WEXITSTATUS(status2);
		if (exit_status == 0) {
			printf(COLOR_GREEN "[NODE2] Completed successfully\n" COLOR_RESET);
		} else {
			printf(COLOR_RED "[NODE2] Failed with exit code %d\n" COLOR_RESET, exit_status);
			exit_code = 1;
		}
	} else if (WIFSIGNALED(status2)) {
		int sig = WTERMSIG(status2);
		if (sig == SIGTERM || sig == SIGINT) {
			printf(COLOR_YELLOW "[NODE2] Stopped by signal (user interrupt)\n" COLOR_RESET);
		} else {
			printf(COLOR_RED "[NODE2] Terminated by signal %d\n" COLOR_RESET, sig);
			exit_code = 1;
		}
	} else {
		printf(COLOR_RED "[NODE2] Terminated abnormally\n" COLOR_RESET);
		exit_code = 1;
	}

	if (exit_code == 0) {
		printf(COLOR_GREEN "\n[LAUNCHER] All tests passed!\n" COLOR_RESET);
	} else {
		printf(COLOR_RED "\n[LAUNCHER] Some tests failed.\n" COLOR_RESET);
	}

	/*
	 * Clean up any leftover IPC resources (shared memory and semaphores).
	 *
	 * Issue: Some IPC resources may not be destroyed by ionTerminate() in multi-node
	 * scenarios. This happens because:
	 * 1. parms.wmKey must be explicit (not SM_NO_KEY) for daemon attachment
	 * 2. When wmKey is explicit, the SDR system doesn't mark resources as "private"
	 * 3. ionTerminate(1) calls sdr_destroy() which detaches but may not destroy all IPC
	 * 4. Resources like semBase (SDR locking semaphore) can be left behind
	 *
	 * Result: semBase semaphore and possibly shared memory segments remain.
	 * Workaround: Explicitly clean up all IPC resources after test completion.
	 *
	 * Note: We don't use sm_ipc_stop() because it's too aggressive and can interfere
	 * with the shutdown process, creating more leftover resources.
	 */
	printf(COLOR_CYAN "\n[LAUNCHER] Cleaning up leftover IPC resources...\n" COLOR_RESET);
	int ret1 = system("ipcs -m | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -m 2>/dev/null");
	int ret2 = system("ipcs -s | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -s 2>/dev/null");
	if (ret1 != 0 || ret2 != 0) {
		printf(COLOR_YELLOW "[LAUNCHER] Note: Some IPC cleanup commands returned non-zero status\n" COLOR_RESET);
	}
	printf(COLOR_CYAN "[LAUNCHER] IPC cleanup completed\n" COLOR_RESET);

	return exit_code;
}
