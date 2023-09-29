/* Test for posix named semaphores and non-unique keys being returned by sm_GetUniqueKey
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

static int debug = 0;

/* if not zero, all test iteration limits are multiplied by this */
static unsigned exhaustive_test_multiplier = 10;


#ifdef SVR4_SEMAPHORES
	#define SEMAPHORE_SYSTEM "SVR4"	
#elif defined(RTOS_SHM)
	#error WONT COMPILE: NOT TESTED ON PLATFORM RTOS
	#define SEMAPHORE_SYSTEM "RTOS"	
#elif defined(MINGW_SHM)
	#error WONT COMPILE: NOT TESTED ON PLATFORM MINGW
	#define SEMAPHORE_SYSTEM "MINGW"	
#elif defined(VXWORKS_SEMAPHORES)
	#error WONT COMPILE: NOT TESTED ON PLATFORM VXWORKS
	#define SEMAPHORE_SYSTEM "VXWORKS"	
#elif defined(POSIX_SEMAPHORES)
	#error WONT COMPILE: NOT TESTED ON PLATFORM POSIX_SEMAPHORES
	#define SEMAPHORE_SYSTEM "Posix Semaphores"	
#elif defined(POSIX_NAMED_SEMAPHORES)
	#define SEMAPHORE_SYSTEM "Posix Named Semaphores"	
#else
	#error WONT COMPILE: CANNOT DETERMINE SEMAPHORE PLATFORM FOR TEST
#endif


int reaper_exit_val_total = 0;
static void
reaper(int sig)
{
    int status;
    int pid;

    while ((pid=waitpid((pid_t)-1, &status, WNOHANG)) > 0) {
		if (debug) fprintf(stderr,"Child process %d exited with status %u\n", pid, WEXITSTATUS(status));
		reaper_exit_val_total += WEXITSTATUS(status);
    }
    /* schedule myself again */
    signal(SIGCHLD,reaper);
}


static int check_unique_keys_guts(int iterations, int sm_unique_key1, int sm_unique_key2)
{
	int l;
	int non_unique1 = 0;
	int non_unique2 = 0;

	for (l=0; l < iterations; ++l) {
		int NEW_unique_key = sm_GetUniqueKey();

		if (debug) if (l < 10) { printf("process %d (0x%08x) got unique key %d (0x%08x) (diff 0x%08lx)\n", getpid(), getpid(), NEW_unique_key, NEW_unique_key, (unsigned long) sm_unique_key2 - (unsigned long) NEW_unique_key); fflush(stdout);}

		if (NEW_unique_key == sm_unique_key1) {
			printf("      ******  on loop iteration %d, process %d got unique key %d (0x%08x) again\n", 
				l, getpid(), sm_unique_key1, sm_unique_key1);
			++non_unique1;
		} 
		if (NEW_unique_key == sm_unique_key2) {
			printf("      ******  on loop iteration %d, process %d  got (invented) unique key %d (0x%08x) again\n", 
				l, getpid(), sm_unique_key2, sm_unique_key2);
			++non_unique2;
		}
	}
	printf("  Process %d completed %d requests and received %d non-unique keys\n",
		getpid(), iterations, non_unique1 + non_unique2);

	return(non_unique1 + non_unique2);
}

static int check_unique_keys()
{
	sm_SemId unique_sem1;
	sm_SemId unique_sem2;
	int sm_unique_key1;
	int sm_unique_key2;
	int non_unique_count;
	int iterations = 0x1000000;
	int nprocs = 10;
	int p;

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	// check semaphores
	sm_unique_key1 = sm_GetUniqueKey();
	sm_unique_key2 = sm_unique_key1 + 0x00a40000;   /* not the way it's supposed to work... */

	printf("Processid %d (0x%08x) generated 'unique' key:  %d (0x%08x)\n", 
	getpid(), getpid(), sm_unique_key1, sm_unique_key1);
	printf("Processid %d (0x%08x) \"invented\" 'unique' key: %d (0x%08x)\n", 
	getpid(), getpid(), sm_unique_key2, sm_unique_key2);

	// prepare to count exit values
	reaper_exit_val_total = 0;

	// generate an ION semaphore with that unique key
	if ((unique_sem1 = sm_SemCreate(sm_unique_key1, SM_SEM_FIFO)) == SM_SEM_NONE) {
		printf("Creation of target semaphore1 failed\n");
		return(0);
	}

	// generate an ION semaphore with that unique key Plus a little (cheating - but should check anyway)
	if ((unique_sem2 = sm_SemCreate(sm_unique_key2, SM_SEM_FIFO)) == SM_SEM_NONE) {
		printf("Creation of target semaphore2 failed\n");
		return(0);
	}

	// generate MANY more and see if we ever get the same key back...
	for (p=0; p < nprocs; ++p) {
		if (fork() == 0) {
			non_unique_count = check_unique_keys_guts(iterations, sm_unique_key1, sm_unique_key2);
			_exit((non_unique_count) != 0);  
		}
	}

	if (debug) fprintf(stderr,"Parent process %d Waiting for children to finish...\n", getpid());
	// Note - ION cleans these processes up out from under us, so we have "no children"
	// We depend, instead, on the "reaper" code above to catch their signals
	while (wait(0) != -1) {
		// ignore
		if (debug) fprintf(stderr,"Waiting again for children\n");
	}
	if (debug) fprintf(stderr,"Done waiting for children to finish...\n");

	if (reaper_exit_val_total == 0)
		printf("**** PASSED - check_unique_keys\n");
	else {
		printf("**** FAILED - check_unique_keys - unique key was reused at least %d times\n", reaper_exit_val_total); 
	}

	return(reaper_exit_val_total == 0);
}

// counter protection
int semnum = -1;
int counter = 0;
int iterations = 100000;
int nthreads = 10;
#define MAXTHREADS 100
pthread_t threads[MAXTHREADS];


static int
thread_adder_guts (int *pcounter)
{
	if (debug) fprintf (stderr,"I am a worker adding to shared variable at address %p (protected by semaphore %d)\n", pcounter, semnum);

    for (int iter = 0; iter < iterations; ++iter)
        {
                // begin critical section
                sm_SemTake (semnum);
            ++(*pcounter);  // fails if executed by multiple threads concurrently!
                sm_SemGive (semnum);
                // end critical section
        }

    return(0);
}
static void *
thread_adder (void *parg)
{
	thread_adder_guts(&counter);
    pthread_exit (NULL);
}


static int multi_thread_semtest(void)
{
    int i;
    struct timeval time_begin, time_end;
	counter = 0;

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	semnum = sm_SemCreate (-1, 0);

   	gettimeofday (&time_begin, NULL);

    for (i = 0; i < nthreads; i++)
        {
			int workernum = i;
            pthread_create (&threads[i], NULL, thread_adder, (void *)&workernum);
        }

    for (i = 0; i < nthreads; i++)
        {
            int exitValue;
            pthread_join (threads[i], (void *)&exitValue);
			if (exitValue != 0)
				fprintf (stderr,"Thread %d returned with exit value %d\n", i, exitValue);
        }

    gettimeofday (&time_end, NULL);

    long int critical_sections = nthreads * iterations;
    fprintf (stderr,"Main thread done, counter: %'d   %s\n", counter,
            (counter == (critical_sections)) ? "CORRECT" : "WRONG!!!!!!!!!!!!");

    double elapsed_sec
        = (time_end.tv_sec + (time_end.tv_usec / 1000000.0))
          - (time_begin.tv_sec + (time_begin.tv_usec / 1000000.0));

    fprintf (stderr,"  Elapsed time: %.3lf seconds\n", elapsed_sec);
    fprintf (stderr,"  Critical sections/second: %'.0lf\n",
            (double)((double)critical_sections / elapsed_sec));
    fprintf (stderr,"  Microseconds/critical section: %.3lf\n",
            (elapsed_sec * 1000000.0) / critical_sections);

	// sm_SemDelete(semnum);

    return (1);
}

static int multi_process_semtest(void)
{
    int i;
	int exitval;
	int pid;
    struct timeval time_begin, time_end;
	uaddr shmid;

	struct shmem {
		int semnum;
		int counter;
	} *pshmemInt = NULL;

	/* create shared memory to store counter */
	int fdshm = sm_ShmAttach(-1, sizeof(*pshmemInt), (void *) &pshmemInt, &shmid);
	if (fdshm == -1) 
		return(-1);
	if (debug) fprintf(stderr,"Shared memory pointer for counter is %p\n", pshmemInt);		
	pshmemInt->counter = 0;			

	/* semaphore created in CHILD process to verify that shared semaphores work correctly */
	if (fork() == 0) { 
		/* child */
		int sem = sm_SemCreate (-1, 0);
		if (debug) fprintf(stderr,"I am the child and I created ION semaphore %d\n", sem);
		pshmemInt->semnum = sem;  // stash semnum in shared memory
		_exit(0); // ION semaphore number is exit value
	}
	wait(&exitval);
	semnum = pshmemInt->semnum;  // pull semnum from shared memory	

   	gettimeofday (&time_begin, NULL);

    for (i = 0; i < nthreads; i++)
        {
            if (fork() == 0) {
				/* child */
				thread_adder_guts(&pshmemInt->counter);
				_exit(0);
			}
        }

    while ((pid=wait(&exitval)) != -1) {
		if (debug) fprintf (stderr,"multi_process_semtest: process %d returned with exit value %d\n", pid, exitval);
	}
	if (pid == -1) {
		if (errno != ECHILD)
		perror("multi_process_semtest: wait()");
	}

    gettimeofday (&time_end, NULL);

    long int critical_sections = nthreads * iterations;
    fprintf (stderr,"Main thread done, counter: %'d   %s\n", pshmemInt->counter,
            (pshmemInt->counter == (critical_sections)) ? "CORRECT" : "WRONG!!!!!!!!!!!!");

    double elapsed_sec
        = (time_end.tv_sec + (time_end.tv_usec / 1000000.0))
          - (time_begin.tv_sec + (time_begin.tv_usec / 1000000.0));

    fprintf (stderr,"  Elapsed time: %.3lf seconds\n", elapsed_sec);
    fprintf (stderr,"  Critical sections/second: %'.0lf\n",
            (double)((double)critical_sections / elapsed_sec));
    fprintf (stderr,"  Microseconds/critical section: %.3lf\n",
            (elapsed_sec * 1000000.0) / critical_sections);

	sm_ShmDetach((char *)pshmemInt);
	sm_SemDelete(semnum);

    return (1);
}


void do_churn(int p, int iterations)
{
	int numsems_each = 10;
	int i, s;
	int semlist[numsems_each];

	for (i=0; i < iterations; ++i) {
		for (s=0; s < numsems_each; ++s) {
if (debug) fprintf(stderr,"%%%% Calling sm_SemCreate() [%d] in process %d (%d)\n", s, p, getpid());
			int sem = sm_SemCreate(-1,0);
if (debug) fprintf(stderr,"%%%%     sm_SemCreate() [%d] returns to process %d (%d)\n", s, p, getpid());

			if (sem == SM_SEM_NONE) {
				fprintf(stderr,"\n!!!!!!!!!! Process %d failed to create semaphore\n", getpid());
				return;
			}
			semlist[s] = sem;
		}
		for (s=0; s < numsems_each; ++s) {
if (debug) fprintf(stderr,"%%%% Calling SemTake(%d) [%d] in process %d (%d)\n", semlist[s], s, p, getpid());
			sm_SemTake(semlist[s]);
if (debug) fprintf(stderr,"%%%% Calling SemGive(%d) [%d] in process %d (%d)\n", semlist[s], s, p, getpid());
			sm_SemGive(semlist[s]);
		}
		for (s=0; s < numsems_each; ++s) {
if (debug) fprintf(stderr,"%%%% Calling SemDelete(%d) [%d] in process %d (%d)\n", semlist[s], s, p, getpid());
			sm_SemDelete(semlist[s]);
		}
	}
	if (debug) fprintf(stderr,"Child %d exits\n", getpid());
	fflush(stderr);
}


int semaphore_churn()
{
	int numprocs = 10;
	int iterations = 10000;
	int p;
	int pids[numprocs];

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	for (p = 0; p < numprocs; ++p) {
		if (debug) fprintf(stderr,"Making child process %d\n", p);
		if ((pids[p] = fork()) == 0) {
			if (debug) fprintf(stderr,"I am child %d (pid %d) and my parent is pid %d\n", p, getpid(), getppid());
			/* child */
			do_churn(p, iterations);
			_exit(0);			
		}
	}

	if (debug) fprintf(stderr,"Parent process %d Waiting for children to finish...\n", getpid());
	while (wait(0) != -1) {
		fprintf(stderr,"Waiting some more");
	}
	if (debug) fprintf(stderr,"Done waiting for children to finish...\n");

	fprintf(stderr,"No errors seen\n");

	return(1);
}



int main(int argc, char **argv)
{
	int passed = 1;

	printf("\nCompiled for underlying semaphore system: %s\n\n", SEMAPHORE_SYSTEM);

	/* check up first, just in case */
	(void) system("killm");
	sleep(2);

	/* Start ION */
	printf("Starting ION...\n"); fflush(stdout);
	ionstart_default_config("loopback-ltp/loopback.ionrc", NULL, NULL,	NULL, NULL,	NULL);
	printf("DONE Starting ION...\n"); fflush(stdout);

	// don't let ION clean up my child processes
	signal(SIGCHLD,reaper);

	if (exhaustive_test_multiplier > 1) {
		printf("\nPerforming more exhaustive tests (multiplied by %u)\n\n", exhaustive_test_multiplier);
	}

	// run each of the scenarios...
	
	fprintf(stderr,"\n####################################################\n");
	fprintf(stderr,"Semaphore churn test...\n\n");
	if (!semaphore_churn())
		passed = 0;

	fprintf(stderr,"\n####################################################\n");
	fprintf(stderr,"Testing simple critical sections with multiple threads in one process ...\n\n");
	if (!multi_thread_semtest())
		passed = 0;

	fprintf(stderr,"\n####################################################\n");
	fprintf(stderr,"Testing simple critical sections with multiple child processes ...\n\n");
	if (!multi_process_semtest())
		passed = 0;

	fprintf(stderr,"\n####################################################\n");
	fprintf(stderr,"Testing get_unique_key()\n\n");
	if (!check_unique_keys())
		passed = 0;

	fprintf(stderr,"\n####################################################\n");
	if (passed)
		printf("**** PASSED - dotest()\n");
	else
		printf("**** FAILED - dotest()\n");

	return (passed?0:1);
}
