/*

	llcv.c:	API for using in-memory linked lists ("lysts") as
		inter-thread communication flows.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "llcv.h"

int	llcv_lyst_is_empty(Llcv llcv)
{
	CHKZERO(llcv);
	return (lyst_length(llcv->list) == 0 ? 1 : 0);
}

int	llcv_lyst_not_empty(Llcv llcv)
{
	CHKZERO(llcv);
	return (lyst_length(llcv->list) > 0 ? 1 : 0);
}

Llcv	llcv_open(Lyst list, Llcv llcv)
{
	CHKNULL(list);
	CHKNULL(llcv);
	if (llcv->list != NULL && pthread_mutex_lock(&llcv->mutex) == 0)
	{
		oK(pthread_mutex_unlock(&llcv->mutex));
		return llcv;		/*	Already initialized.	*/
	}

	if (pthread_mutex_init(&llcv->mutex, NULL))
	{
		putSysErrmsg("can't open llcv, mutex init failed", NULL);
		return NULL;
	}

	if (pthread_cond_init(&llcv->cv, NULL))
	{
		pthread_mutex_destroy(&llcv->mutex);
		putSysErrmsg("can't open llcv, condition init failed", NULL);
		return NULL;
	}

	llcv->list = list;
	return llcv;
}

void	llcv_lock(Llcv llcv)
{
	CHKVOID(llcv);
	oK(pthread_mutex_lock(&llcv->mutex));
}

void	llcv_unlock(Llcv llcv)
{
	CHKVOID(llcv);
	oK(pthread_mutex_unlock(&llcv->mutex));
}

int	llcv_wait(Llcv llcv, LlcvPredicate condition, int usec)
{
	int		result = 0;
	struct timeval	interval;
	struct timeval	workTime;
	struct timespec	deadline;

	CHKERR(llcv);
	CHKERR(condition);
	CHKERR(usec >= LLCV_BLOCKING);

	/*	Lock the mutex to assure stable state when starting
	 *	to wait.						*/

	if (pthread_mutex_lock(&llcv->mutex))
	{
		putSysErrmsg("can't wait llcv, mutex lock failed", NULL);
		return -1;
	}

	if (condition(llcv) == 0)
	{
		switch (usec)
		{
		case LLCV_POLL:
			errno = ETIMEDOUT;
			result = -1;
			break;

		case LLCV_BLOCKING:
			/*	Atomically, unlock the mutex and wait
		 	*	for some thread to signal on the cv,
		 	*	and then re-lock the mutex.		*/

			result = pthread_cond_wait(&llcv->cv, &llcv->mutex);
			if (result)
			{
				errno = result;
				putSysErrmsg("llcv wait got pthread_cond error",
						NULL);
				result = -1;
			}

			break;

		default:
			getCurrentTime(&workTime);
			interval.tv_sec = usec / 1000000;
			interval.tv_usec = usec % 1000000;
			workTime.tv_sec += interval.tv_sec;
			workTime.tv_usec += interval.tv_usec;
			if (workTime.tv_usec > 1000000)
			{
				workTime.tv_usec -= 1000000;
				workTime.tv_sec += 1;
			}

			deadline.tv_sec = workTime.tv_sec;
			deadline.tv_nsec = workTime.tv_usec * 1000;

			/*	Atomically, unlock the mutex and wait
		 	*	for some thread to signal on the cv,
		 	*	and then re-lock the mutex.		*/

			result = pthread_cond_timedwait(&llcv->cv, &llcv->mutex,
					&deadline);
			if (result)
			{
				errno = result;
				if (errno != ETIMEDOUT)
				{
					putSysErrmsg("llcv wait got \
pthread_cond error", itoa(usec));
				}

				result = -1;
			}
		}
	}

	/*	Either the condition is now true or a condition wait
	 *	failed or timed out.  Unlock the mutex to enable
	 *	further processing.					*/

	pthread_mutex_unlock(&llcv->mutex);
	return result;	/*	Wait is concluded, one way or another.	*/
}

void	llcv_signal(Llcv llcv, LlcvPredicate condition)
{
	CHKVOID(llcv);
	CHKVOID(condition);

	/*	Lock the mutex to assure stable state when signaling.	*/

	if (pthread_mutex_lock(&llcv->mutex))
	{
		writeMemo("[?] Can't signal llcv, mutex lock failed.");
		return;
	}

	/*	Signal, but only if the condition is true.		*/

	if (condition(llcv))
	{
		if (pthread_cond_signal(&llcv->cv))
		{
			writeMemo("[?] Can't signal llcv, cond signal failed.");
		}
	}

	/*	Unlock the mutex to enable further processing.		*/

	pthread_mutex_unlock(&llcv->mutex);
}

void	llcv_signal_while_locked(Llcv llcv, LlcvPredicate condition)
{
	CHKVOID(llcv);
	CHKVOID(condition);

	/*	Signal, but only if the condition is true.		*/

	if (condition(llcv))
	{
		if (pthread_cond_signal(&llcv->cv))
		{
			writeMemo("[?] Can't signal llcv, cond signal failed.");
		}
	}
}

void	llcv_close(Llcv llcv)
{
	int	result;

	if (llcv)
	{
		if (pthread_mutex_lock(&llcv->mutex) != 0)
		{
			return;	/*	Already closed.			*/
		}

		result = pthread_cond_destroy(&llcv->cv);
		oK(pthread_mutex_unlock(&llcv->mutex));
		if (result != 0 || pthread_mutex_destroy(&llcv->mutex) != 0)
		{
			writeMemo("[?] Can't close llcv.");
			return;
		}

		memset((char *) llcv, 0, sizeof(struct llcv_str));
	}
}
