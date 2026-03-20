/*
	This is a portion of the `ionexit_test` test that verifies no ION POSIX
	Named Semaphores exist on the system.

	Going through the official POSIX semaphore interface (rather than checking
	the filesystem directly) makes the test agnostic to the machine's storage
	location for the named semaphores.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include <platform.h> // for SEM_NSEMS_MAX

int main(void) {

#if defined(__linux__)
	// The only OS that this test currently supports is Linux.
	//      e.g. SEM_NSEMS_MAX does not get defined on FreeBSD, so compilation
	//      of this test-support program needs to be skipped on that platform,
	//      until additional verification methods are added for the non-Linux
	//      platforms, or some platform-agnostic verification approach is
	//      found.

	char semName[NAME_MAX] = { '\0' };

	int result = 0;

	for (int i = 0; i < SEM_NSEMS_MAX; i++) {
		int nameResult = snprintf(semName, NAME_MAX, "/ion:GLOBAL:%d", i);
		assert(nameResult <= NAME_MAX && nameResult >= 0);

		// Call sem_open without O_CREAT, so that nonexistent semaphore can be
		// detected
		sem_t *semAttempt = sem_open(semName, 0);

		// Expect sem_open() to fail due to no semaphore of this name
		if (! (semAttempt == SEM_FAILED && errno == ENOENT) ) {
			printf("POSIX named semaphore `%s` still exists.\n", semName);
			result = -1;
		}
	}

	(void)strncpy(semName, "/ion:GLOBAL:ipcSem", NAME_MAX);
	sem_t *ipcSemAttempt = sem_open(semName, 0);

	if (! (ipcSemAttempt == SEM_FAILED && errno == ENOENT) ) {
		printf("POSIX named semaphore `%s` still exists.\n", semName);
		result = -1;
	}

	return result;

#else
	// For unsupported platforms, always return error; this test should have
	// been excluded from running on its unsupported platforms.
	printf("ionexit_test/checksemaphores: Unsupported platform\n");
	return -1;
#endif

}
