/*
 *	smlist.c:	shared-memory linked list management.
 *			Adapted from SDR linked-list management,
 *			which in turn was lifted from Jeff
 *			Biesiadecki's list library.
 *
 *	Copyright (c) 2001, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	04-26-03  SCB	One mutex per list.
 *	12-11-00  SCB	Initial implementation.
 */

#include "platform.h"
#include "smlist.h"

/* define a list */
typedef struct
{
	PsmAddress	userData;
	PsmAddress	first;	/*	first element in the list	*/
	PsmAddress	last;	/*	last element in the list	*/
	size_t		length;	/*	number of elements in the list	*/
	sm_SemId	lock;	/*	mutex for list			*/
} SmList;

/* define an element of a list */
typedef struct
{
	PsmAddress	list;	/* address of list that element is in	*/
	PsmAddress	prev;	/* address of previous element in list	*/
	PsmAddress	next;	/* address of next element in list	*/
	PsmAddress	data;	/* address of data for this element	*/
} SmListElt;

static char	*_noListMsg()
{
	return "Can't access list at address zero.";
}

static char	*_cannotLockMsg()
{
	return "Can't lock list.";
}

static char	*_noSpaceForEltMsg()
{
	return "Can't allocate space for list element.";
}

static void	eraseList(SmList *list)
{
	list->userData = 0;
	list->first = 0;
	list->last = 0;
	list->length = 0;
	return;
}

static void	eraseListElt(SmListElt *elt)
{
	elt->list = 0;
	elt->prev = 0;
	elt->next = 0;
	elt->data = 0;
	return;
}

static int	lockSmlist(SmList *list)
{
	return sm_SemTake(list->lock);
}

static void	unlockSmlist(SmList *list)
{
	sm_SemGive(list->lock);
}

PsmAddress	Sm_list_create(const char *fileName, int lineNbr,
			PsmPartition partition)
{
	sm_SemId	lock;
	PsmAddress	list;
	SmList		*listBuffer;

	lock = sm_SemCreate(SM_NO_KEY, SM_SEM_FIFO);
	if (lock < 0)
	{
		putErrmsg("Can't create semaphore for list.", NULL);
		return 0;
	}

	list = Psm_zalloc(fileName, lineNbr, partition, sizeof(SmList));
	if (list == 0)
	{
		sm_SemDelete(lock);
		putErrmsg("Can't allocate space for list header.", NULL);
		return 0;
	}

	listBuffer = (SmList *) psp(partition, list);
	eraseList(listBuffer);
	listBuffer->lock = lock;
	return list;
}

void	sm_list_unwedge(PsmPartition partition, PsmAddress list, int interval)
{
	SmList	*listBuffer;

	CHKVOID(partition);
	listBuffer = (SmList *) psp(partition, list);
	CHKVOID(listBuffer);
	sm_SemUnwedge(listBuffer->lock, interval);
}

static int	wipeList(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress list,
			SmListDeleteFn deleteFn, void *arg, int destroy)
{
	SmList		*listBuffer;
	PsmAddress	elt;
	PsmAddress	next;
	SmListElt	*eltBuffer;

	listBuffer = (SmList *) psp(partition, list);
	if (lockSmlist(listBuffer) < 0)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return -1;
	}

	for (elt = listBuffer->first; elt != 0; elt = next)
	{
		eltBuffer = (SmListElt *) psp(partition, elt);
		CHKERR(eltBuffer);
		next = eltBuffer->next;
		if (deleteFn)
		{
			deleteFn(partition, elt, arg);
		}

		/* clear in case user mistakenly accesses later... */
		eraseListElt(eltBuffer);
		Psm_free(fileName, lineNbr, partition, elt);
	}

	eraseList(listBuffer);
	if (destroy)
	{
		sm_SemEnd(listBuffer->lock);
		microsnooze(50000);
		sm_SemDelete(listBuffer->lock);
		listBuffer->lock = SM_SEM_NONE;
		Psm_free(fileName, lineNbr, partition, list);
	}
	else
	{
		unlockSmlist(listBuffer);
	}

	return 0;
}

int	Sm_list_clear(const char *fileName, int lineNbr, PsmPartition partition,
		PsmAddress list, SmListDeleteFn deleteFn, void *arg)
{
	CHKERR(partition);
	CHKERR(list);
	return wipeList(fileName, lineNbr, partition, list, deleteFn, arg, 0);
}

int	Sm_list_destroy(const char *fileName, int lineNbr,
		PsmPartition partition, PsmAddress list,
		SmListDeleteFn deleteFn, void *arg)
{
	CHKERR(partition);
	CHKERR(list);
	return wipeList(fileName, lineNbr, partition, list, deleteFn, arg, 1);
}

PsmAddress	sm_list_user_data(PsmPartition partition, PsmAddress list)
{
	SmList		*listBuffer;
	PsmAddress	userData;

	CHKZERO(partition);
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		return 0;
	}

	userData = listBuffer->userData;
	unlockSmlist(listBuffer);
	return userData;
}

int	sm_list_user_data_set(PsmPartition partition, PsmAddress list,
			PsmAddress data)
{
	SmList	*listBuffer;

	CHKERR(partition);
	CHKERR(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKERR(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return -1;
	}

	listBuffer->userData = data;
	unlockSmlist(listBuffer);
	return 0;
}

size_t	sm_list_length(PsmPartition partition, PsmAddress list)
{
	SmList	*listBuffer;
	int	length;

	CHKERR(partition);
	CHKERR(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKERR(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return -1;
	}

	length = listBuffer->length;
	unlockSmlist(listBuffer);
	return length;
}

static PsmAddress	finishInsertingFirst(const char *fileName, int lineNbr,
				PsmPartition partition, PsmAddress list,
				SmList *listBuffer, PsmAddress data)
{
	PsmAddress	elt;
	SmListElt	*eltBuffer;

	elt = Psm_zalloc(fileName, lineNbr, partition, sizeof(SmListElt));
	if (elt == 0)
	{
		unlockSmlist(listBuffer);
		putErrmsg(_noSpaceForEltMsg(), NULL);
		return 0;
	}

	eltBuffer = (SmListElt *) psp(partition, elt);
	eraseListElt(eltBuffer);
	eltBuffer->list = list;
	eltBuffer->data = data;
	eltBuffer->prev = 0;
	eltBuffer->next = listBuffer->first;
	if (listBuffer->first != 0)
	{
		eltBuffer = (SmListElt *) psp(partition, listBuffer->first);
		CHKZERO(eltBuffer);
		eltBuffer->prev = elt;
	}
	else
	{
		listBuffer->last = elt;
	}

	listBuffer->first = elt;
	listBuffer->length += 1;
	unlockSmlist(listBuffer);
	return elt;
}

PsmAddress	Sm_list_insert_first(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress list,
			PsmAddress data)
{
	SmList	*listBuffer;

	if (list == 0)
	{
		putErrmsg(_noListMsg(), NULL);
		return 0;
	}

	listBuffer = (SmList *) psp(partition, list);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return 0;
	}

	return finishInsertingFirst(fileName, lineNbr, partition, list,
			listBuffer, data);
}

PsmAddress	Sm_list_insert_last(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress list,
			PsmAddress data)
{
	SmList		*listBuffer;
	PsmAddress	elt;
	SmListElt	*eltBuffer;

	CHKZERO(partition);
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return 0;
	}

	elt = Psm_zalloc(fileName, lineNbr, partition, sizeof(SmListElt));
	if (elt == 0)
	{
		unlockSmlist(listBuffer);
		putErrmsg(_noSpaceForEltMsg(), NULL);
		return 0;
	}

	eltBuffer = (SmListElt *) psp(partition, elt);
	eraseListElt(eltBuffer);
	eltBuffer->list = list;
	eltBuffer->data = data;
	eltBuffer->prev = listBuffer->last;
	if (listBuffer->last != 0)
	{
		eltBuffer = (SmListElt *) psp(partition, listBuffer->last);
		CHKZERO(eltBuffer);
		eltBuffer->next = elt;
	}
	else
	{
		listBuffer->first = elt;
	}

	listBuffer->last = elt;
	listBuffer->length += 1;
	unlockSmlist(listBuffer);
	return elt;
}

PsmAddress	Sm_list_insert_before(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress oldElt,
			PsmAddress data)
{
	SmListElt	*oldEltBuffer;
	PsmAddress	list;
	SmList		*listBuffer;
	PsmAddress	elt;
	SmListElt	*eltBuffer;

	CHKZERO(partition);
	CHKZERO(oldElt);
	oldEltBuffer = (SmListElt *) psp(partition, oldElt);
	CHKZERO(oldEltBuffer);
	if ((list = oldEltBuffer->list) == 0)
	{
		putErrmsg(_noListMsg(), NULL);
		return 0;
	}

	listBuffer = (SmList *) psp(partition, list);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return 0;
	}

	elt = Psm_zalloc(fileName, lineNbr, partition, sizeof(SmListElt));
	if (elt == 0)
	{
		unlockSmlist(listBuffer);
		putErrmsg(_noSpaceForEltMsg(), NULL);
		return 0;
	}

	eltBuffer = (SmListElt *) psp(partition, elt);
	eraseListElt(eltBuffer);
	eltBuffer->list = list;
	eltBuffer->data = data;
	eltBuffer->prev = oldEltBuffer->prev;
	eltBuffer->next = oldElt;
	if (oldEltBuffer->prev != 0)
	{
		eltBuffer = (SmListElt *) psp(partition, oldEltBuffer->prev);
		CHKZERO(eltBuffer);
		eltBuffer->next = elt;
	}
	else
	{
		listBuffer->first = elt;
	}

	oldEltBuffer->prev = elt;
	listBuffer->length += 1;
	unlockSmlist(listBuffer);
	return elt;
}

static PsmAddress	finishInsertingAfter(const char *fileName, int lineNbr,
				PsmPartition partition, PsmAddress oldElt,
				SmListElt *oldEltBuffer, PsmAddress list,
				SmList *listBuffer, PsmAddress data)
{
	PsmAddress	elt;
	SmListElt	*eltBuffer;

	elt = Psm_zalloc(fileName, lineNbr, partition, sizeof(SmListElt));
	if (elt == 0)
	{
		unlockSmlist(listBuffer);
		putErrmsg(_noSpaceForEltMsg(), NULL);
		return 0;
	}

	eltBuffer = (SmListElt *) psp(partition, elt);
	eraseListElt(eltBuffer);
	eltBuffer->list = list;
	eltBuffer->data = data;
	eltBuffer->next = oldEltBuffer->next;
	eltBuffer->prev = oldElt;
	if (oldEltBuffer->next != 0)
	{
		eltBuffer = (SmListElt *) psp(partition, oldEltBuffer->next);
		CHKZERO(eltBuffer);
		eltBuffer->prev = elt;
	}
	else
	{
		listBuffer->last = elt;
	}

	oldEltBuffer->next = elt;
	listBuffer->length += 1;
	unlockSmlist(listBuffer);
	return elt;
}

PsmAddress	Sm_list_insert_after(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress oldElt,
			PsmAddress data)
{
	SmListElt	*oldEltBuffer;
	PsmAddress	list;
	SmList		*listBuffer;

	CHKZERO(partition);
	CHKZERO(oldElt);
	oldEltBuffer = (SmListElt *) psp(partition, oldElt);
	CHKZERO(oldEltBuffer);
	if ((list = oldEltBuffer->list) == 0)
	{
		putErrmsg(_noListMsg(), NULL);
		return 0;
	}

	listBuffer = (SmList *) psp(partition, list);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return 0;
	}

	return finishInsertingAfter(fileName, lineNbr, partition, oldElt,
			oldEltBuffer, list, listBuffer, data);
}

PsmAddress	Sm_list_insert(const char *fileName, int lineNbr,
			PsmPartition partition, PsmAddress list,
			PsmAddress data, SmListCompareFn compare, void *argData)
{
	SmList		*listBuffer;
	PsmAddress	elt;
	SmListElt	*eltBuffer;

	if (compare == (SmListCompareFn) NULL)
	{
		/*	List is assumed to be unsorted.  We simply
			add the new element at the end of the list.	*/

		return Sm_list_insert_last(fileName, lineNbr, partition,
				list, data);
	}

	/*	Using user-specified comparison function.  List is
		assumed to be in sorted order.				*/

	CHKZERO(partition);
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return 0;
	}

	/*	Find position to insert new data into list.
		Start from end of list to keep sort stable;
		sort sequence is implicitly FIFO within key,
		i.e., we insert element AFTER the last element
		in the table with the same key value.			*/

	for (elt = listBuffer->last; elt != 0; elt = eltBuffer->prev)
	{
		eltBuffer = (SmListElt *) psp(partition, elt);
		CHKZERO(eltBuffer);
		if (compare(partition, eltBuffer->data, argData) <= 0)
		{
			break;
		}
	}

	/* insert into list */

	if (elt == 0)
	{
		return finishInsertingFirst(fileName, lineNbr, partition, list,
				listBuffer, data);
	}

	return finishInsertingAfter(fileName, lineNbr, partition, elt,
			eltBuffer, list, listBuffer, data);
}

int	Sm_list_delete(const char *fileName, int lineNbr,
		PsmPartition partition, PsmAddress elt, SmListDeleteFn deleteFn,
		void *arg)
{
	SmListElt	*eltBuffer;
	PsmAddress	list;
	SmList		*listBuffer;
	PsmAddress	next;
	PsmAddress	prev;

	CHKERR(partition);
	CHKERR(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKERR(eltBuffer);
	if ((list = eltBuffer->list) == 0)
	{
		putErrmsg(_noListMsg(), NULL);
		return -1;
	}

	listBuffer = (SmList *) psp(partition, list);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg(_cannotLockMsg(), NULL);
		return -1;
	}

	if (listBuffer->length < 1)
	{
		unlockSmlist(listBuffer);
		putErrmsg("list element can't be deleted, list is empty", NULL);
		return -1;
	}

	next = eltBuffer->next;
	prev = eltBuffer->prev;
	if (deleteFn)
	{
		deleteFn(partition, elt, arg);
	}

	/* clear in case user accesses later... */
	eraseListElt(eltBuffer);
	Psm_free(fileName, lineNbr, partition, elt);
	if (prev)
	{
		eltBuffer = (SmListElt *) psp(partition, prev);
		CHKERR(eltBuffer);
		eltBuffer->next = next;
	}
	else
	{
		listBuffer->first = next;
	}

	if (next)
	{
		eltBuffer = (SmListElt *) psp(partition, next);
		CHKERR(eltBuffer);
		eltBuffer->prev = prev;
	}
	else
	{
		listBuffer->last = prev;
	}

	listBuffer->length -= 1;
	unlockSmlist(listBuffer);
	return 0;
}

PsmAddress	sm_list_first(PsmPartition partition, PsmAddress list)
{
	SmList		*listBuffer;
	PsmAddress	first;

	CHKZERO(partition);
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg("Can't get first element.", NULL);
		return 0;
	}

	first = listBuffer->first;
	unlockSmlist(listBuffer);
	return first;
}

PsmAddress	sm_list_last(PsmPartition partition, PsmAddress list)
{
	SmList		*listBuffer;
	PsmAddress	last;

	CHKZERO(partition);
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		putErrmsg("Can't get last element.", NULL);
		return 0;
	}

	last = listBuffer->last;
	unlockSmlist(listBuffer);
	return last;
}

PsmAddress	sm_list_next(PsmPartition partition, PsmAddress elt)
{
	SmListElt	*eltBuffer;

	CHKERR(partition);
	CHKERR(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKERR(eltBuffer);
	return eltBuffer->next;
}

PsmAddress	sm_list_prev(PsmPartition partition, PsmAddress elt)
{
	SmListElt	*eltBuffer;

	CHKERR(partition);
	CHKERR(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKERR(eltBuffer);
	return eltBuffer->prev;
}

PsmAddress	sm_list_search(PsmPartition partition, PsmAddress fromElt,
			SmListCompareFn compare, void *arg)
{
	SmListElt	*eltBuffer;
	PsmAddress	list;
	SmList		*listBuffer;
	PsmAddress	elt;
	int		result;

	CHKZERO(partition);
	if (fromElt == 0)
	{
		return 0;	/*	Obviously not found in list.	*/
	}

	CHKZERO(fromElt);
	eltBuffer = (SmListElt *) psp(partition, fromElt);
	CHKZERO(eltBuffer);
	list = eltBuffer->list;
	CHKZERO(list);
	listBuffer = (SmList *) psp(partition, list);
	CHKZERO(listBuffer);
	if (lockSmlist(listBuffer) == ERROR)
	{
		return 0;
	}

	/*	Forward linear search starting at specified element.	*/

	if (compare != (SmListCompareFn)NULL)
	{
		/*	Using user-specified comparison function, so
			list is assumed to be in sorted order.  We
			abandon the search as soon we're past any
			possible match.					*/

		elt = fromElt;
		while (1)
		{
			result = compare(partition, eltBuffer->data, arg);
			if (result > 0)	/*	past any possible match	*/
			{
				elt = 0;
				break;
			}

			if (result == 0)	/*	found 1st match	*/
			{
				break;
			}

			elt = eltBuffer->next;
			if (elt == 0)		/*	end of list	*/
			{
				break;
			}

			eltBuffer = (SmListElt *) psp(partition, elt);
		}

		unlockSmlist(listBuffer);
		return elt;
	}

	/*	List is assumed to be unsorted.  We examine every
		element of the list, until we either get an exact
		"==" match or run out of elements.			*/

	elt = fromElt;
	while (1)
	{
		if (eltBuffer->data == (PsmAddress) arg)
		{
			break;
		}

		elt = eltBuffer->next;
		if (elt == 0)		/*	end of list	*/
		{
			break;
		}

		eltBuffer = (SmListElt *) psp(partition, elt);
	}

	unlockSmlist(listBuffer);
	return elt;
}

PsmAddress	sm_list_list(PsmPartition partition, PsmAddress elt)
{
	SmListElt	*eltBuffer;

	CHKZERO(partition);
	CHKZERO(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKZERO(eltBuffer);
	return eltBuffer->list;
}

PsmAddress	sm_list_data(PsmPartition partition, PsmAddress elt)
{
	SmListElt	*eltBuffer;

	CHKZERO(partition);
	CHKZERO(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKZERO(eltBuffer);
	return eltBuffer->data;
}

PsmAddress	sm_list_data_set(PsmPartition partition, PsmAddress elt,
			PsmAddress new)
{
	SmListElt	*eltBuffer;
	PsmAddress	old;

	CHKZERO(partition);
	CHKZERO(elt);
	eltBuffer = (SmListElt *) psp(partition, elt);
	CHKZERO(eltBuffer);
	old = eltBuffer->data;
	eltBuffer->data = new;
	return old;
}
