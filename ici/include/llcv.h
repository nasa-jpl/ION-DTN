/*

	llcv.h:	definitions enabling the use of in-memory doubly-
		linked lists (Lysts) as inter-thread communication
		flows.
			
		Like sdrpipes and sdrcvs, llcvs transmit variable-
		length messages without flow control: the writing
		rate is completely decoupled from the reading rate.

		An llcv comprises a Lyst, a mutex, and a condition
		variable.  The Lyst may be in either private
		or shared memory, but the Lyst itself is not shared
		with other processes.  The reader thread waits on
		the condition variable until signaled by a writer
		that some condition is now true.  The standard
		lyst_* API functions are used to populate and
		drain the linked list; in order to protect linked
		list integrity, each thread must call llcv_lock
		before operating on the Lyst and llcv_unlock
		afterwards.  The other llcv functions merely effect
		flow signaling in a way that makes it unnecessary for
		the reader to poll or busy-wait on the Lyst.

	Copyright (c) 2003, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
/*	Author: Scott Burleigh, Jet Propulsion Laboratory		*/
/*									*/
/*	Mutex locking fixes by John Huff, Ohio University, 2018.	*/
/*									*/
#ifndef _LLCV_H_
#define _LLCV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"
#include "lyst.h"

typedef struct llcv_str
{
	Lyst			list;
	pthread_mutex_t		mutex;
	int 			mutex_initialized;
	pthread_mutex_t* 	mutex_address;
	pthread_cond_t		cv;
} *Llcv;

typedef int	(*LlcvPredicate)(Llcv);

/*	llcv_wait timeout values					*/
#define	LLCV_POLL	(0)
#define LLCV_BLOCKING	(-1)

extern int	llcv_lyst_is_empty(Llcv Llcv);
			/*	A built-in "convenience" predicate.
			 *	Returns true if the length of the
			 *	Llcv's encapsulated Lyst is zero,
			 *	false otherwise.			*/

extern int	llcv_lyst_not_empty(Llcv Llcv);
			/*	A built-in "convenience" predicate.
			 *	Returns true if the length of the
			 *	Llcv's encapsulated Lyst is non-
			 *	zero, false otherwise.			*/

extern Llcv	llcv_open(Lyst list, Llcv llcv);
			/*	Opens an Llcv, initializing as
			 *	necessary.  The list argument must
			 *	point to an existing Lyst, which
			 *	may reside in either private or
			 *	shared dynamic memory.  The llcv
			 *	argument must point to an existing
			 *	Llcv management object, which may
			 *	reside in either static or dynamic
			 *	(private or shared) memory -- but
			 *	*NOT* in stack space.  Returns NULL
			 *	on any error.				*/

extern void	llcv_lock(Llcv llcv);
			/*	Locks the Lyst so that it may be
			 *	updated or examined safely by the
			 *	calling thread.  Fails silently
			 *	on any error.				*/

extern void	llcv_unlock(Llcv llcv);
			/*	Unlocks the Lyst so that another
			 *	thread may lock and update or
			 *	examine it.  Fails silently on
			 *	any error.				*/

extern int	llcv_wait(Llcv llcv, LlcvPredicate cond, int microseconds);
			/*	Returns when the Lyst encapsulated
			 *	within the Llcv meets the indicated
			 *	condition.  If microseconds is non-
			 *	negative, will alternatively return
			 *	-1 and set errno to ETIMEDOUT when
			 *	the indicated number of microseconds
			 *	has passed.  Negative values of the
			 *	microseconds argument other than
			 *	LLCV_BLOCKING are illegal.  Returns
			 *	-1 on any error.			*/

extern void	llcv_signal(Llcv llcv, LlcvPredicate cond);
			/*	Signals to the waiting reader (if
			 *	any) that the Lyst encapsulated in
			 *	the Llcv now meets the indicated
			 *	condition -- but only if it in fact
			 *	does meet that condition.		*/

extern void	llcv_signal_while_locked(Llcv llcv, LlcvPredicate cond);
			/*	Same as llcv_signal() except does not
			 *	lock the Llcv's mutex before signalling
			 *	or unlock afterwards.  For use when
			 *	the Llcv is already locked, to prevent
			 *	deadlock.				*/

extern void	llcv_close(Llcv llcv);
			/*	Destroys the Llcv management object's
			 *	mutex and condition variable.  Fails
			 *	silently (and has no effect) if a
			 *	reader is currently waiting on the Llcv.*/

#ifdef __cplusplus
}
#endif

#endif  /* _LLCV_H_ */
