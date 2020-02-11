/*

	platform_sm.h:	more platform-dependent porting adaptations.

	Copyright (c) 2001, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Alan Schlutsmeyer, Jet Propulsion Laboratory		*/
/*									*/
#ifndef _PLATFORM_SM_H_
#define _PLATFORM_SM_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int		wmKey;
	size_t		wmSize;
	char		*wmAddress;
	char		*wmName;
} sm_WmParms;

typedef int		sm_SemId;
#define SM_SEM_NONE	(-1)

#define SM_SEM_FIFO	0
#define SM_SEM_PRIORITY	1

#define SM_NO_KEY	(-1)

#define	ICI_PRIORITY	250

#ifndef ION4WIN		/*	No pthreads in Visual Studio.		*/

#ifdef SOLARIS_COMPILER
#define pthread_begin	pthread_begin_named
#else
/*      Required in order to overload pthread_begin macro		*/
#define GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define pthread_begin(...) GET_MACRO(__VA_ARGS__, pthread_begin5, pthread_begin4)(__VA_ARGS__)
#endif

#if defined (bionic) || defined (uClibc)
extern int		sm_BeginPthread(pthread_t *threadId,
				const pthread_attr_t *attr,
				void *(*function)(void *), void *arg);
extern int		sm_BeginPthread_named(pthread_t *threadId,
				const pthread_attr_t *attr,
				void *(*function)(void *), void *arg, const char *name);
#define pthread_begin4(w,x,y,z) sm_BeginPthread(w, x, y, z)
#define pthread_begin5(w,x,y,z,u) sm_BeginPthread_named(w, x, y, z,u)
extern void		sm_EndPthread(pthread_t threadId);
#define pthread_end(x)	sm_EndPthread(x)
#else			/*	Standard pthread functions available.	*/
#define pthread_begin4(w,x,y,z) pthread_create(w, x, y, z)
extern int		pthread_begin_named(pthread_t *thread, const pthread_attr_t *attr,
				void *(*start_routine) (void *), void *arg, const char *name);
#define pthread_begin5(w,x,y,z,u) pthread_begin_named(w,x,y,z,u)
#define pthread_end(x)		pthread_cancel(x)
#endif			/*	end of #ifdef bionic || uClibc		*/

#endif			/*	end of #ifndef ION4WIN			*/

/*      IPC services access control */
extern int		sm_ipc_init();
extern void		sm_ipc_stop();
extern int		sm_GetUniqueKey();

/*	Portable semaphore management routines.				*/

extern sm_SemId		sm_SemCreate(int key, int semType);
extern int		sm_SemTake(sm_SemId semId);
extern void		sm_SemGive(sm_SemId semId);
extern int		sm_SemUnwedge(sm_SemId semId, int timeoutSeconds);
extern void		sm_SemDelete(sm_SemId semId);
extern void		sm_SemEnd(sm_SemId semId);
extern int		sm_SemEnded(sm_SemId semId);
extern void		sm_SemUnend(sm_SemId semId);
extern sm_SemId		sm_GetTaskSemaphore(int taskId);

/*	Portable shared-memory region access routines.			*/

extern int		sm_ShmAttach(int key, size_t size, char **shmPtr,
					uaddr *id);
extern void		sm_ShmDetach(char *shmPtr);
extern void		sm_ShmDestroy(uaddr id);

/*	Portable task (process) management routines.			*/

extern int		sm_TaskIdSelf();
extern int		sm_TaskExists(int taskId);
extern void		*sm_TaskVar(void **arg);
extern void		sm_TaskSuspend();
extern void		sm_TaskDelay(int seconds);
extern void		sm_TaskYield();
extern int		sm_TaskSpawn(char *name, char *arg1, char *arg2,
				char *arg3, char *arg4, char *arg5, char *arg6,
				char *arg7, char *arg8, char *arg9, char *arg10,
				int priority, int stackSize);
extern void		sm_TaskForget(int taskId);
extern void		sm_TaskKill(int taskId, int sigNbr);
extern void		sm_TaskDelete(int taskId);
extern void		sm_TasksClear();
extern void		sm_Abort();
#if (defined(mingw) || defined(ION4WIN))
extern void		sm_WaitForWakeup(int seconds);
extern void		sm_Wakeup(DWORD);
#endif

#ifndef ION4WIN		/*	No pthreads in Visual Studio.		*/
extern void		sm_ConfigurePthread(pthread_attr_t *attr,
				size_t stackSize);
#endif			/*	end of #ifdef ION4WIN			*/

extern int		pseudoshell(char *commandLine);

#ifdef __cplusplus
}
#endif

#endif  /* _PLATFORM_SM_H_ */
