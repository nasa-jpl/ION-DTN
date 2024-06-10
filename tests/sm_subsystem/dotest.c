/* Test for posix named semaphores and non-unique keys being returned by sm_GetUniqueKey
 *
 * Author: Shawn Ostermann @ OHIO
 * Date: August 2022			*/

#include <cfdp.h>
#include <bputa.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include "check.h"
#include "testutil.h"

static int debug = 0;

/* if not zero, all test iteration limits are multiplied by this */
static float exhaustive_test_multiplier = 1.0000;  /* can be less than 1 */


/* signal handler for exited processes */
int reaper_exit_val_total = 0;
static void reaper(int sig)
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


static void wait_for_children() {
	if (debug) fprintf(stderr,"Parent process %d Waiting for children to finish...\n", getpid());
	while (1) {
		int status;
		int pid=wait(&status);
		if (pid == -1) {
			if (errno == ECHILD) {
				break;  /* no more children */
			} else if (errno == EINTR) {
				continue; /* got interrupted, try again */
			}
			perror("semaphore_churn: wait()");
			break;
		}	
	}
	if (debug) fprintf(stderr,"Done waiting for children to finish...\n");
	return;
}


static int check_unique_keys_guts(unsigned iterations, int sm_unique_key[], int num_uniq_keys)
{
	int l, k;
	unsigned count_non_unique = 0;
	unsigned key_min = 0xffffffff;
	unsigned key_max = 0;

	for (l=0; l < iterations; ++l) {
		unsigned NEW_unique_key = sm_GetUniqueKey();

		if (debug>1) if (l < 10) { printf("process %d (0x%08x) got unique key %d (0x%08x) (diff 0x%08lx)\n", getpid(), getpid(), NEW_unique_key, NEW_unique_key, (unsigned long) sm_unique_key[0] - (unsigned long) NEW_unique_key); fflush(stdout);}

		for (k=0; k < num_uniq_keys; ++k) {
			if (NEW_unique_key == sm_unique_key[k]) {
				printf("      ******  on loop iteration %d, process %d got unique key %u (0x%08x) again\n", 
					l, getpid(), sm_unique_key[k], sm_unique_key[k]);
				++count_non_unique;
			} 
		}

		if (NEW_unique_key < key_min) key_min = NEW_unique_key;
		if (NEW_unique_key > key_max) key_max = NEW_unique_key;
	}
	printf("\tProcess %d completed %d requests (min:%u max:%u) and received %d non-unique keys\n",
		getpid(), iterations, key_min, key_max, count_non_unique);

	return(count_non_unique);
}

static int check_unique_keys()
{
	int num_uniq_keys = 5;
	sm_SemId unique_sm_id[num_uniq_keys];
	int sm_unique_key[num_uniq_keys];
	int non_unique_count;
	unsigned iterations = 50000;
	int nprocs = 10;
	int p;
	static uaddr id;
	static char *ptr;

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	printf("  check_unique_keys() Running %u iterations in each of %d processes\n\n", iterations, nprocs);

	// generate 2 'unique' keys to ensure that we never get them again below
	sm_unique_key[0] = sm_GetUniqueKey();
	sm_unique_key[1] = sm_unique_key[0] + 0x00001000;   /* not the way it's supposed to work... */
	sm_unique_key[2] = sm_unique_key[0] + 0x00a40000;   /* not the way it's supposed to work... */
	sm_unique_key[3] = sm_unique_key[0] + 0x00005000;   /* not the way it's supposed to work... */
	sm_unique_key[4] = sm_unique_key[0] + 0x00005010;   /* not the way it's supposed to work... */

	printf("\tProcessid %d (0x%08x) generated 'unique' key:  %u (0x%08x) (used for a semaphore) \n", 
	getpid(), getpid(), sm_unique_key[0], sm_unique_key[0]);
	printf("\tProcessid %d (0x%08x) \"invented\" 'unique' key: %u (0x%08x) (used for a semaphore) \n", 
	getpid(), getpid(), sm_unique_key[1], sm_unique_key[1]);
	printf("\tProcessid %d (0x%08x) \"invented\" 'unique' key: %u (0x%08x) (used for a semaphore) \n", 
	getpid(), getpid(), sm_unique_key[2], sm_unique_key[2]);
	printf("\tProcessid %d (0x%08x) \"invented\" 'unique' key: %u (0x%08x) (used for a shared memory region) \n", 
	getpid(), getpid(), sm_unique_key[3], sm_unique_key[3]);
	printf("\tProcessid %d (0x%08x) \"invented\" 'unique' key: %u (0x%08x) (used for a semaphore) \n", 
	getpid(), getpid(), sm_unique_key[4], sm_unique_key[4]);
	printf("\n");

	// prepare to count exit values
	reaper_exit_val_total = 0;

	// generate an ION semaphore with that unique key
	if ((unique_sm_id[0] = sm_SemCreate(sm_unique_key[0], SM_SEM_FIFO)) == SM_SEM_NONE) {
		printf("Creation of target semaphore1 failed\n");
		return(0);
	}

	// generate an ION semaphore with that unique key Plus a little (cheating - but should check anyway)
	if ((unique_sm_id[1] = sm_SemCreate(sm_unique_key[1], SM_SEM_FIFO)) == SM_SEM_NONE) {
		printf("Creation of target unique_sem2 failed\n");
		return(0);
	}

	// generate an ION semaphore with that unique key Plus a little (cheating - but should check anyway)
	if ((unique_sm_id[2] = sm_SemCreate(sm_unique_key[2], SM_SEM_FIFO)) == SM_SEM_NONE) {
		printf("Creation of target unique_sem3 failed\n");
		return(0);
	}

	// generate an ION shared memory segment with that unique key Plus a little (cheating - but should check anyway)
	if ((unique_sm_id[3] = sm_ShmAttach(sm_unique_key[3], 1000, &ptr, &id)) == SM_SEM_NONE) {
		printf("Creation of target shared memory unique_sem4 failed for key %d (0x%x)\n", sm_unique_key[3], sm_unique_key[3]);
		return(0);
	}

	// generate an ION semaphore with that unique key Plus a little (cheating - but should check anyway)
	// for thorough testing, created in a subprocess */
	if (fork() == 0) {
		if ((unique_sm_id[4] = sm_SemCreate(sm_unique_key[4], SM_SEM_FIFO)) == SM_SEM_NONE) {
			printf("Creation of target unique_sem3 failed\n");
			exit(99);
		}
		exit(0);
	}

	// generate MANY more and see if we ever get the same key back...
	for (p=0; p < nprocs; ++p) {
		if (fork() == 0) {
			non_unique_count = check_unique_keys_guts(iterations, sm_unique_key, num_uniq_keys);
			_exit((non_unique_count) != 0);  
		}
	}
	/* and run it in the CURRENT process, too */
	non_unique_count = check_unique_keys_guts(iterations, sm_unique_key, num_uniq_keys);
	reaper_exit_val_total += non_unique_count;

	if (debug) fprintf(stderr,"Parent process %d Waiting for children to finish...\n", getpid());
	// Note - ION cleans these processes up out from under us, so we have "no children"
	// We depend, instead, on the "reaper" code above to catch their signals
	wait_for_children();
	if (debug) fprintf(stderr,"Done waiting for children to finish...\n");

	if (reaper_exit_val_total == 0)
		printf("  passed - check_unique_keys\n");
	else {
		printf("  failed - check_unique_keys - unique key was reused at least %d times\n", reaper_exit_val_total); 
	}

	return(reaper_exit_val_total == 0);
}

// global counter and its critical section protection
int semnum = -1;
int counter = 0;



static int
thread_adder_guts (int *pcounter, unsigned iterations)
{
	int iter;
	if (debug) fprintf (stderr,"I am a worker adding to shared variable at address %p (protected by semaphore %d)\n", pcounter, semnum);

    for (iter = 0; iter < iterations; ++iter)
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
	unsigned iterations = *((int *)parg);

	thread_adder_guts(&counter, iterations);
    pthread_exit (NULL);
}


static int multi_thread_semtest()
{
    int i;
    struct timeval time_begin, time_end;
	counter = 0;
	#define MAXTHREADS 100
	pthread_t threads[MAXTHREADS];	
	unsigned nthreads = 10;
	unsigned iterations = 300000;
	long int critical_sections;
	int correct;

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	printf("  multi_thread_semtest() Running %u iterations in each of %d threads\n", iterations, nthreads);

	semnum = sm_SemCreate (-1, 0);

   	gettimeofday (&time_begin, NULL);

    for (i = 0; i < nthreads; i++)
        {
            if (pthread_create (&threads[i], NULL, thread_adder, (void *)&iterations) != 0) {
				perror("pthread_create");
			}
        }

    for (i = 0; i < nthreads; i++)
        {
            int exitValue;
            pthread_join (threads[i], (void *)&exitValue);
			if (exitValue != 0)
				fprintf (stderr,"Thread %d returned with exit value %d\n", i, exitValue);
        }

    gettimeofday (&time_end, NULL);

    critical_sections = nthreads * iterations;
	correct = (counter == (critical_sections));
    fprintf (stderr,"\n  Main thread done, counter: %'d   %s\n", counter,
            correct? "CORRECT" : "WRONG!!!!!!!!!!!!");

    double elapsed_sec
        = (time_end.tv_sec + (time_end.tv_usec / 1000000.0))
          - (time_begin.tv_sec + (time_begin.tv_usec / 1000000.0));

    fprintf (stderr,"  Elapsed time: %.3lf seconds\n", elapsed_sec);
    fprintf (stderr,"  Critical sections/second: %'.0lf\n",
            (double)((double)critical_sections / elapsed_sec));
    fprintf (stderr,"  Microseconds/critical section: %.3lf\n",
            (elapsed_sec * 1000000.0) / critical_sections);

	sm_SemDelete(semnum);

    return (correct);
}

static int multi_process_semtest()
{
    int i;
	int exitval;
	int correct;
    struct timeval time_begin, time_end;
	uaddr shmid;
	int nprocs = 10;
	unsigned iterations = 300000;

	struct shmem {
		int semnum;
		int counter;
	} *pshmemInt = NULL;

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	printf("  multi_process_semtest() Running %u iterations in each of %d processes\n", iterations, nprocs);

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

    for (i = 0; i < nprocs; i++)
        {
            if (fork() == 0) {
				/* child */
				thread_adder_guts(&pshmemInt->counter, iterations);
				_exit(0);
			}
        }

    wait_for_children();

    gettimeofday (&time_end, NULL);

    long int critical_sections = nprocs * iterations;
	correct = (pshmemInt->counter == (critical_sections));
    fprintf (stderr,"\n  Main thread done, counter: %'d   %s\n", pshmemInt->counter,
            correct ? "CORRECT" : "WRONG!!!!!!!!!!!!");

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

    return (correct);
}


int sem_errors()
{
	int sem;
	int ret;
	int correct = 1;
	int ended = 0;

	printf("\n**** NOTE: several of these operations may generate internal errors (from assertions) - that's expected\n\n");

	printf("semerrors(): testing sm_SemCreate\n");
	sem = sm_SemCreate(-1,0);
	if (sem == -1) {
		correct = 0;
		fprintf(stderr,"semerrors(): sm_SemCreate failed with return value %d\n", sem);
	}
	printf("semerrors(): test semaphore is id %d\n", sem);

	printf("semerrors(): deleting that semaphore\n");
	sm_SemDelete(sem);

	printf("semerrors(): trying to sm_SemTake(%d) that closed semaphore\n", sem);
	ret = sm_SemTake(sem);
	if (ret != 0) {
		printf("    CORRECT: sm_SemTake failed with return value %d\n", ret);
	} else {
		correct = 0;
		printf("    ** ERROR: sm_SemTake did NOT fail but had return value %d\n", ret);
	}

	printf("semerrors(): trying to sm_SemGive(%d) that closed semaphore\n", sem);
	sm_SemGive(sem);  /* this will fail, but there is no return value (only an assertion failure) */

	printf("semerrors(): trying to sm_SemUnend(%d) that closed semaphore, then check with sm_SemEnded()\n", sem);
	sm_SemUnend(sem);
	ended = sm_SemEnded(sem);
	if (ended != 0) {
		correct = 0;
		printf("    ** ERROR: after sm_SemUnend() on closed sem, sm_SemEnded() did NOT return 0 (but returned %d)\n",
			 ended);
	}

	printf("semerrors(): trying to sm_SemEnd(%d) that closed semaphore, then check with sm_SemEnded()\n", sem);
	sm_SemEnd(sem);
	ended = sm_SemEnded(sem);
	if (ended != 1) {
		correct = 0;
		printf("    ** ERROR: after sm_SemEnd() on closed sem, sm_SemEnded() did NOT return 1 (but returned %d)\n",
			 ended);
	}

	printf("semerrors(): trying to sm_SemEnded(%d) that closed semaphore\n", sem);
	ret = sm_SemEnded(sem); // return value ignored on purpose 

	printf("semerrors(): trying to sm_SemUnwedge(%d) that closed semaphore\n", sem);
	ret = sm_SemUnwedge(sem,1);
	if (ret != 0) {
		printf("    CORRECT: sm_SemUnwedge failed with return value %d\n", ret);
	} else {
		// Unfortunately, this test is slightly non-deterministic because of the way 
		// that SemUnwedge cleans up semaphores that may or may NOT be in use, so
		// we will NOT call this an error
		// correct = 0;
		printf("    ** ERROR: sm_SemUnwedge did NOT fail but had return value %d (error ignored)\n", ret);
	}

	printf("semerrors(): trying to sm_SemDelete(%d) that closed semaphore\n", sem);
	sm_SemDelete(sem);

	fprintf(stderr,"\n  %s errors seen\n", correct?"NO":"SOME");

	return(correct);
}


int do_churn(int p, unsigned iterations)
{
	int numsems_each = 10;
	int i, s, d;
	int semlist[numsems_each];

	for (i=0; i < iterations; ++i) {
		for (s=0; s < numsems_each; ++s) {
			int sem = sm_SemCreate(-1,0);

			if (sem == SM_SEM_NONE) {
				fprintf(stderr,"\n!!!!!!!!!! Process %d failed to create semaphore in do_churn()\n", getpid());
				for (d=0; d < s; ++d) {
					sm_SemDelete(semlist[d]);
				}
				return(1);
			}
			semlist[s] = sem;
		}
		for (s=0; s < numsems_each; ++s) {
			sm_SemTake(semlist[s]);
			sm_SemGive(semlist[s]);
		}
		for (s=0; s < numsems_each; ++s) {
			sm_SemDelete(semlist[s]);
		}
	}
	if (debug) fprintf(stderr,"Child %d exits\n", getpid());
	fflush(stderr);
	return(0);
}


int semaphore_churn()
{
	int nprocs = 10;
	unsigned iterations = 15000;
	int p;
	int pids[nprocs];

	if (exhaustive_test_multiplier > 0)
		iterations *= exhaustive_test_multiplier;

	printf("  semaphore_churn() Running %u iterations in each of %d processes\n", iterations, nprocs);

	reaper_exit_val_total = 0; /* watch child process exit values */
	for (p = 0; p < nprocs; ++p) {
		if (debug) fprintf(stderr,"Making child process %d\n", p);
		fflush(stdout); fflush(stderr);
		if ((pids[p] = fork()) == 0) {
			int ret;
			if (debug) fprintf(stderr,"I am child %d (pid %d) and my parent is pid %d\n", p, getpid(), getppid());
			/* child */
			ret = do_churn(p, iterations);
			_exit(ret);
		}
	}

	wait_for_children();

	fprintf(stderr,"\n  %d errors seen\n", reaper_exit_val_total);

	return(reaper_exit_val_total == 0);
}

/* need a platform-independant, synchronous version */
int synch_command(char *cmd, int maxseconds)
{
	int pid = pseudoshell(cmd);
	if (pid == ERROR) {
		printf("couldn't run command '%s'\n", cmd);
		return(0);
	}
	while (sm_TaskExists(pid)) {
		if (--maxseconds < 1) {
			printf("shell command '%s' is taking too long, giving up\n", cmd);
			sm_TaskDelete(pid);
			return(0);
		}
		printf("Waiting for '%s' to finish...\n", cmd);
		sm_TaskDelay(1);
	}
	return(1);
}

int do_killm()
{
	return(synch_command("killm",60));
}


int main(int argc, char **argv)
{
	int passed = 1;
	time_t time_start, time_stop;

	/* check up first, just in case */
	if (!do_killm())
		exit(1);

	/* Start ION */
	printf("Starting ION...\n"); fflush(stdout);
	ionstart_default_config("loopback-ltp/loopback.ionrc", NULL, NULL,	NULL, NULL,	NULL);
	printf("DONE Starting ION...\n"); fflush(stdout);

	// don't let ION clean up my child processes
	signal(SIGCHLD,reaper);

	if (exhaustive_test_multiplier != 1) {
		printf("\nPerforming more/less exhaustive tests (multiplied by %f)\n\n", exhaustive_test_multiplier);
	}

	// run each of the scenarios...
	printf("\n####################################################\n");
	printf("Semaphore churn test...\n\n");
	time(&time_start);
	if (!semaphore_churn())
		passed = 0;
	time(&time_stop);
	printf("\nElapsed time: %ld seconds\n", time_stop - time_start);

	printf("\n####################################################\n");
	printf("Testing simple critical sections with multiple threads in one process ...\n\n");
	time(&time_start);
	if (!multi_thread_semtest())
		passed = 0;
	time(&time_stop);
	printf("\nElapsed time: %ld seconds\n", time_stop - time_start);

	printf("\n####################################################\n");
	printf("Testing simple critical sections with multiple child processes ...\n\n");
	time(&time_start);
	if (!multi_process_semtest())
		passed = 0;
	time(&time_stop);
	printf("\nElapsed time: %ld seconds\n", time_stop - time_start);

	printf("\n####################################################\n");
	printf("Testing get_unique_key()\n\n");
	time(&time_start);
	if (!check_unique_keys())
		passed = 0;
	time(&time_stop);
	printf("\nElapsed time: %ld seconds\n", time_stop - time_start);

	printf("\n####################################################\n");
	printf("Testing semaphore error handling ...\n\n");
	printf("Warning - may leave semaphore table inconsistent \n\n");
	time(&time_start);
	if (!sem_errors())
		passed = 0;
	time(&time_stop);
	printf("\nElapsed time: %ld seconds\n", time_stop - time_start);

	printf("\n####################################################\n");

#ifdef POSIX_NAMED_SEMAPHORES
	void _semPrintTable();
	_semPrintTable();
#endif /* POSIX_NAMED_SEMAPHORES */

	if (passed)
		printf("**** PASSED - dotest()\n");
	else
		printf("**** FAILED - dotest()\n");

	return (passed?0:1);
}
