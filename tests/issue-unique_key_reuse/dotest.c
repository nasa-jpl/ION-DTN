/* Test fix for non-unique semaphores being returned by sm_GetUniqueKey
 *
 * Author: Shawn Ostermann @ OHIO
 * Date: August 2022			*/

#include <cfdp.h>
#include <bputa.h>
#include <stdlib.h>
#include <platform.h>
#include <sys/ipc.h>
#include "check.h"
#include "testutil.h"

#ifdef SVR4_SEMAPHORES
		static int Sem_Key_exists(int sem_key) {
			int semid;
			return(0);
			if ((semid = semget(sem_key, 0, 0)) == -1) {
				perror("semget");
				if (errno == ENOENT) {
					printf("No semaphore with key 0x%08x exists\n", sem_key);
					return(0);
				} else {
					perror("semget: semget failed");
					exit(1);
				}
			}
			printf("A semaphore with key 0x%08x DOES exist\n", sem_key);
			return(1);
		}
#elif defined(RTOS_SHM)
	#error WONT COMPILE: NOT TESTED ON PLATFORM RTOS
#elif defined(MINGW_SHM)
	#error WONT COMPILE: NOT TESTED ON PLATFORM MINGW
#elif defined(VXWORKS_SEMAPHORES)
	#error WONT COMPILE: NOT TESTED ON PLATFORM VXWORKS
#elif defined(POSIX_SEMAPHORES)
	#error WONT COMPILE: NOT TESTED ON PLATFORM POSIX
#else
	#error WONT COMPILE: CANNOT DETERMINE SEMAPHORE PLATFORM FOR TEST
#endif

// test2 - parent makes semaphores, one of the CHILDREN clobbers it
static int check_parent_process_and_children()
{
	sm_SemId target_semid;
	int target_sem_unique_key;
	int sem_child_key;

	// disable ION exit signal handling
	signal(SIGCHLD, SIG_DFL);

	// generate a TARGET semaphore key in the parent process
	target_sem_unique_key = sm_GetUniqueKey();  // make a new one for THIS test
	printf("For multi-process testing, Processid %d (%08x) generated 'unique' key:  %d (%08x)\n", getpid(), getpid(), target_sem_unique_key, target_sem_unique_key);

	// verify that the key doesn't exist before we even start...
	if (Sem_Key_exists(target_sem_unique_key)) {
		printf("Test setup failure, 'unique' key already exists: 0x%08x\n", target_sem_unique_key);
		return(0);
	}

	// generate an ION semaphore with that unique key
	target_semid = sm_SemCreate(target_sem_unique_key, SM_SEM_FIFO);

		system("echo; echo; ipcs -s; echo; echo");


	if (   !   Sem_Key_exists(target_sem_unique_key)) {
		printf("Test setup failure, FAILED to generate unique target key 0x%08x\n", target_sem_unique_key);
		return(0);
	}
	if (target_semid == -1) {
		printf("Creating of target semaphore failed\n");
		exit(-1);
	}
	printf("For multi-process testing, looking for duplicate semid:  %d\n", target_semid);

	int non_unique = 0;
	int iterations = 100000;
	pid_t parentpid = getpid();
	printf("Dotest running as process %u\n", parentpid);
	printf("Process %d - beginning test looping through %d processes\n", getpid(), iterations);
	for (int l=0; l < iterations; ++l) {
		int child_pid;
		if ((l % 10000) == 0)
			printf("  iteration %d...\n", l);
		if ((child_pid=fork()) == 0) {  
			// child process
			// for efficiency, ignore children whose PID isn't the same
			// in the low order bits, because it can't happen anyway
			unsigned pidmask = 0xffff;  // the one used in GetUniqueKey()
			if ((getpid() & pidmask) != (parentpid & pidmask)) {
				exit(0);  // this child exits, can't cause the problem with this pid
			}
			sm_SemId semid = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
			if (semid == -1) {
				printf("###### semaphore creation failed on loop iteration %d (%08x)\n", l, l);
				exit(1);
			}
			sem_child_key = 0;
			if (sem_child_key == target_semid) {
				printf("******  on loop iteration %d (%08x), I got target semaphore ID %d again\n", l, l, semid);
				exit(2);
			} else {
				// printf("Process %d on iteration %d got Ion semid: %d\n", getpid(), l, semid);
				sm_SemDelete(semid);
			}
			// printf("Child process %d (parent %d) is exiting\n", getpid(), getppid());
			exit(0);
		}

		// parent
		int ret, child_status;
		// printf("Waiting for child %d\n", child_pid);
		ret = waitpid(child_pid,&child_status,0);
		if (ret == -1){
			perror("waitpid failed");
			exit(1);
		} else {
			if ( WIFEXITED(child_status) )
			{
				int exit_status = WEXITSTATUS(child_status);  
				if (exit_status == 2) {
					// duplicate found
					++non_unique;
					printf("Child process found duplicate key\n");
				}
				if (exit_status != 0)     
					printf("Exit status of child %d was %d\n", ret, exit_status);
			}
		}
	}

	// sm_SemDelete(target_semid);

	printf("Process %d completed loop %d times and found %d non-unique semaphores\n",
	getpid(), iterations, non_unique);
	if (non_unique == 0)
		printf("**** PASSED part 2 \n");
	else
		printf("**** FAILED part 2 - semaphore key 0x%08x was reused %d times\n", target_sem_unique_key, non_unique);

	return(non_unique == 0);
}


static int check_one_process_many_sems()
{
	sm_SemId target_semid;
	int sem_unique_key;

	// check semaphores
	sem_unique_key = sm_GetUniqueKey();
	printf("Processid %d (0x%08x) generated 'unique' key:  %d (0x%08x)\n", 
	getpid(), getpid(), sem_unique_key, sem_unique_key);

	system("sleep 1; echo; echo; ipcs -s; echo; echo");


	// generate an ION semaphore with that unique key
	target_semid = sm_SemCreate(sem_unique_key, SM_SEM_FIFO);
	if (target_semid == -1) {
		printf("Creation of target semaphore failed\n");
		exit(-1);
	}
	printf("Processid %d created single-process target semaphore with key 0x%08x and semid %d\n", getpid(), sem_unique_key, target_semid);

	// generate 1,000,000 more and see if we ever get the same semaphore back...
	int non_unique = 0;
	int iterations = 1000000;
	for (int l=0; l < iterations; ++l) {
		if ((l % 100000) == 0)
			printf("  iteration %d...\n", l);
		sm_SemId semid = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
		if (semid == -1) {
			printf("###### semaphore creation failed on loop iteration %d (%08x)\n", l, l);
			break;
		}
		if (semid == target_semid) {
			printf("******  on loop iteration %d (%08x), I got target semaphore ID %d again\n", l, l, semid);
			++non_unique;
		} else {
			sm_SemDelete(semid);
		}
	}
	printf("Process %d completed loop %d times and found %d non-unique semaphores\n",
	getpid(), iterations, non_unique);
	if (non_unique == 0)
		printf("**** PASSED part 1\n");
	else
		printf("**** FAILED part 1 - semaphore key 0x%08x was reused %d times\n", sem_unique_key, non_unique);

	return(non_unique == 0);
}



int main(int argc, char **argv)
{
	int passed;

	ionstop();

	/* Start ION */
	printf("Starting ION...\n");
	_xadmin("ionadmin", "", "cfdp.ipn.bp.ltp.udp/config.ionrc"); 		sleep(1);
	_xadmin("ltpadmin", "", "cfdp.ipn.bp.ltp.udp/config.ltprc");		sleep(1);
	_xadmin("bpadmin", "", "cfdp.ipn.bp.ltp.udp/config.bprc");			sleep(1);
	_xadmin("ipnadmin", "", "cfdp.ipn.bp.ltp.udp/config.ipnrc");		sleep(1);
	_xadmin("bpadmin", "", "cfdp.ipn.bp.ltp.udp/loopbackstart.bprc");	sleep(5);


	// run each of the 3 scenarios...
	passed = check_one_process_many_sems();
	passed |= check_parent_process_and_children();


	if (passed)
		printf("**** PASSED\n");
	else
		printf("**** FAILED\n");

	return (passed?0:1);
}
