/*
 *	sdrlist.c:	spacecraft data recorder list management
 *			library.
 *
 *	Copyright (c) 2001-2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	This library implements the Simple Data Recorder system's
 *	self-delimiting strings.
 *
 *	Modification History:
 *	Date	  Who	What
 *	4-3-96	  APS	Abstracted IPC services and task control.
 *	5-1-96	  APS	Ported to sparc-sunos4.
 *	12-20-00  SCB	Revised for sparc-sunos5.
 *	6-8-07    SCB	Divided sdr.c library into separable components.
 */

#include "sdrP.h"
#include "sdrlist.h"

/*		Private definitions of SDR list structures.		*/

typedef struct
{
	SdrAddress	userData;
	SdrObject	first;	/*	first element in the list	*/
	SdrObject	last;	/*	last element in the list	*/
	size_t		length;	/*	number of elements in the list	*/
} SdrList;

typedef struct
{
	SdrObject list; /* list that this element is in */
	SdrObject prev; /* previous element in list */
	SdrObject next; /* next element in list */
	SdrObject data; /* data for this element */
} SdrListElt;

/*	*	*	List management functions	*	*	*/

static void	sdr_list__clear(SdrList *list)
{
	list->userData = 0;
	list->first = 0;
	list->last = 0;
	list->length = 0;
}

static void	sdr_list__elt_clear(SdrListElt *elt)
{
	elt->list = 0;
	elt->prev = 0;
	elt->next = 0;
	elt->data = 0;
}

SdrObject Sdr_list_create(const char *file, int line, Sdr sdrv)
{
	SdrObject list;
	SdrList	listBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	list = _sdrzalloc(sdrv, sizeof(SdrList));
	if (list == 0)
	{
		oK(_iEnd(file, line, "list"));
		return 0;
	}

	sdr_list__clear(&listBuffer);
	sdrPut((SdrAddress) list, listBuffer);
	return list;
}

void Sdr_list_destroy(const char *file, int line, Sdr sdrv, SdrObject list,
		SdrListDeleteFn deleteFn, void *arg)
{
	SdrList		listBuffer;
	SdrObject	elt;
	SdrObject	next;
	SdrListElt	eltBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (list == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return;
	}

	sdrFetch(listBuffer, (SdrAddress) list);
	for (elt = listBuffer.first; elt != 0; elt = next)
	{
		sdrFetch(eltBuffer, (SdrAddress) elt);
		next = eltBuffer.next;
		if (deleteFn)
		{
			deleteFn(sdrv, elt, arg);
		}

		/* just in case user mistakenly accesses later... */
		sdr_list__elt_clear(&eltBuffer);
		sdrPut((SdrAddress) elt, eltBuffer);
		sdrFree(elt);
	}

	/* just in case user mistakenly accesses later... */
	sdr_list__clear(&listBuffer);
	sdrPut((SdrAddress) list, listBuffer);
	sdrFree(list);
}

SdrAddress sdr_list_user_data(Sdr sdrv, SdrObject list)
{
	SdrList		listBuffer;

	CHKZERO(list);
	sdrFetch(listBuffer, list);
	return listBuffer.userData;
}

void	Sdr_list_user_data_set(const char *file, int line, Sdr sdrv,
		SdrObject list, SdrAddress data)
{
	SdrList	listBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (list == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return;
	}

	sdrFetch(listBuffer, list);
	listBuffer.userData = data;
	sdrPut((SdrAddress) list, listBuffer);
}

size_t sdr_list_length(Sdr sdrv, SdrObject list)
{
	SdrList		listBuffer;

	CHKERR(list);
	sdrFetch(listBuffer, list);
	return listBuffer.length;
}

SdrObject Sdr_list_insert_first(const char *file, int line, Sdr sdrv,
		SdrObject list, SdrAddress data)
{
	SdrList		listBuffer;
	SdrObject	elt;
	SdrListElt	eltBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (list == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		_putErrmsg(file, line, "SDR memory exhausted. Can't allocate SdrListElt.", NULL);
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element at the beginning of the list */
	sdrFetch(listBuffer, (SdrAddress) list);
	eltBuffer.prev = 0;
	eltBuffer.next = listBuffer.first;
	sdrPut((SdrAddress) elt, eltBuffer);
	if (listBuffer.first != 0)
	{
		sdrFetch(eltBuffer, (SdrAddress) listBuffer.first);
		eltBuffer.prev = elt;
		sdrPut((SdrAddress) listBuffer.first, eltBuffer);
	}
	else
	{
		listBuffer.last = elt;
	}

	listBuffer.first = elt;
	listBuffer.length += 1;
	sdrPut((SdrAddress) list, listBuffer);
	return elt;
}

SdrObject Sdr_list_insert_last(const char *file, int line, Sdr sdrv,
		SdrObject list, SdrAddress data)
{
	SdrList		listBuffer;
	SdrObject	elt;
	SdrListElt	eltBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (list == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		_putErrmsg(file, line, "SDR memory exhausted. Can't allocate SdrListElt.", NULL);
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element at the end of the list */
	sdrFetch(listBuffer, (SdrAddress) list);
	eltBuffer.prev = listBuffer.last;
	eltBuffer.next = 0;
	sdrPut((SdrAddress) elt, eltBuffer);
	if (listBuffer.last != 0)
	{
		sdrFetch(eltBuffer, (SdrAddress) listBuffer.last);
		eltBuffer.next = elt;
		sdrPut((SdrAddress) listBuffer.last, eltBuffer);
	}
	else
	{
		listBuffer.first = elt;
	}

	listBuffer.last = elt;
	listBuffer.length += 1;
	sdrPut((SdrAddress) list, listBuffer);
	return elt;
}

SdrObject Sdr_list_insert_before(const char *file, int line, Sdr sdrv,
		SdrObject oldElt, SdrAddress data)
{
	SdrListElt	oldEltBuffer;
	SdrObject	list;
	SdrList		listBuffer;
	SdrObject	elt;
	SdrListElt	eltBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (oldElt == 0)
	{
		oK(_xniEnd(file, line, "oldElt", sdrv));
		return 0;
	}

	sdrFetch(oldEltBuffer, (SdrAddress) oldElt);
	if ((list = oldEltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		/*
		* Graceful error handling on SDR exhaustion.
		* Replaced _iEnd to prevent sm_Abort() and fatal daemon crash.
		* Returning 0 allows higher-level functions to safely cancel the transaction.
		*/
		_putErrmsg(file, line, "SDR memory exhausted. Can't allocate SdrListElt.", NULL);
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element before the specified element */
	sdrFetch(listBuffer, (SdrAddress) list);
	eltBuffer.prev = oldEltBuffer.prev;
	eltBuffer.next = oldElt;
	sdrPut((SdrAddress) elt, eltBuffer);
	if (oldEltBuffer.prev != 0)
	{
		sdrFetch(eltBuffer, (SdrAddress) oldEltBuffer.prev);
		eltBuffer.next = elt;
		sdrPut((SdrAddress) oldEltBuffer.prev, eltBuffer);
	}
	else
	{
		listBuffer.first = elt;
	}

	oldEltBuffer.prev = elt;
	sdrPut((SdrAddress) oldElt, oldEltBuffer);
	listBuffer.length += 1;
	sdrPut((SdrAddress) list, listBuffer);
	return elt;
}

SdrObject Sdr_list_insert_after(const char *file, int line, Sdr sdrv,
		SdrObject oldElt, SdrAddress data)
{
	SdrListElt	oldEltBuffer;
	SdrObject	list;
	SdrList		listBuffer;
	SdrObject	elt;
	SdrListElt	eltBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (oldElt == 0)
	{
		oK(_xniEnd(file, line, "oldElt", sdrv));
		return 0;
	}

	sdrFetch(oldEltBuffer, (SdrAddress) oldElt);
	if ((list = oldEltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		/*
		* Graceful error handling on SDR exhaustion.
		* Replaced _iEnd to prevent sm_Abort() and fatal daemon crash.
		* Returning 0 allows higher-level functions to safely cancel the transaction.
		*/
		_putErrmsg(file, line, "SDR memory exhausted. Can't allocate SdrListElt.", NULL);
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element after the specified element */
	sdrFetch(listBuffer, (SdrAddress) list);
	eltBuffer.next = oldEltBuffer.next;
	eltBuffer.prev = oldElt;
	sdrPut((SdrAddress) elt, eltBuffer);
	if (oldEltBuffer.next != 0)
	{
		sdrFetch(eltBuffer, (SdrAddress) oldEltBuffer.next);
		eltBuffer.prev = elt;
		sdrPut((SdrAddress) oldEltBuffer.next, eltBuffer);
	}
	else
	{
		listBuffer.last = elt;
	}

	oldEltBuffer.next = elt;
	sdrPut((SdrAddress) oldElt, oldEltBuffer);
	listBuffer.length += 1;
	sdrPut((SdrAddress) list, listBuffer);
	return elt;
}

SdrObject Sdr_list_insert(const char *file, int line, Sdr sdrv, SdrObject list,
		SdrAddress data, SdrListCompareFn compare, void *dataBuffer)
{
	SdrList		listBuffer;
	SdrListElt	eltBuffer;
	SdrObject	elt;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	if (list == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/*	If not sorted list, just append to the end of the list.	*/

	if (compare == (SdrListCompareFn) NULL)
	{
		return Sdr_list_insert_last(file, line, sdrv, list, data);
	}

	/*	Application claims that this list is sorted.  Find
	 *	position to insert new data into list.  Start from end
	 *	of list to keep sort stable; sort sequence is implicitly
	 *	FIFO within key, i.e., we insert element AFTER the last
	 *	element in the table with the same key value.		*/

	sdrFetch(listBuffer, (SdrAddress) list);
	for (elt = listBuffer.last; elt != 0; elt = eltBuffer.prev)
	{
		sdrFetch(eltBuffer, (SdrAddress) elt);
		if (compare(sdrv, eltBuffer.data, dataBuffer) <= 0) break;
	}

	/*	Insert into list at this point.				*/

	if (elt == 0)
	{
		return Sdr_list_insert_first(file, line, sdrv, list, data);
	}

	return Sdr_list_insert_after(file, line, sdrv, elt, data);
}

void Sdr_list_delete(const char *file, int line, Sdr sdrv, SdrObject elt,
		SdrListDeleteFn deleteFn, void *arg)
{
	SdrListElt	eltBuffer;
	SdrObject	list;
	SdrList		listBuffer;
	SdrObject	next;
	SdrObject	prev;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (elt == 0)
	{
		oK(_xniEnd(file, line, "elt", sdrv));
		return;
	}

	sdrFetch(eltBuffer, (SdrAddress) elt);
	if ((list = eltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return;
	}

	sdrFetch(listBuffer, (SdrAddress) list);
	if (listBuffer.length < 1)
	{
		oK(_xniEnd(file, line, "list non-empty", sdrv));
		return;
	}

	next = eltBuffer.next;
	prev = eltBuffer.prev;
	if (deleteFn)
	{
		deleteFn(sdrv, elt, arg);
	}

	/* just in case user accesses later... */
	sdr_list__elt_clear(&eltBuffer);
	sdrPut((SdrAddress) elt, eltBuffer);
	sdrFree(elt);
	if (prev)
	{
		sdrFetch(eltBuffer, (SdrAddress) prev);
		eltBuffer.next = next;
		sdrPut((SdrAddress) prev, eltBuffer);
	}
	else
	{
		listBuffer.first = next;
	}

	if (next)
	{
		sdrFetch(eltBuffer, (SdrAddress) next);
		eltBuffer.prev = prev;
		sdrPut((SdrAddress) next, eltBuffer);
	}
	else
	{
		listBuffer.last = prev;
	}

	listBuffer.length -= 1;
	sdrPut((SdrAddress) list, listBuffer);
}

SdrObject sdr_list_first(Sdr sdrv, SdrObject list)
{
	SdrList		listBuffer;

	if (!sdrFetchSafe(sdrv))
	{
		writeMemoNote("[?] sdr_list_first called but SDR not \
accessible", itoa(list));
		printStackTrace();
		return 0;	/*	Defensive: SDR not accessible.	*/
	}

	CHKZERO(list);
	sdrFetch(listBuffer, (SdrAddress) list);
	return listBuffer.first;
}

SdrObject sdr_list_last(Sdr sdrv, SdrObject list)
{
	SdrList		listBuffer;

	if (!sdrFetchSafe(sdrv))
	{
		writeMemoNote("[?] sdr_list_last called but SDR not \
accessible", itoa(list));
		printStackTrace();
		return 0;	/*	Defensive: SDR not accessible.	*/
	}

	CHKZERO(list);
	sdrFetch(listBuffer, (SdrAddress) list);
	return listBuffer.last;
}

SdrObject sdr_list_next(Sdr sdrv, SdrObject elt)
{
	SdrListElt	eltBuffer;

	if (!sdrFetchSafe(sdrv))
	{
		writeMemoNote("[?] sdr_list_next called but SDR not \
accessible", itoa(elt));
		printStackTrace();
		return 0;	/*	Defensive: SDR not accessible.	*/
	}

	CHKZERO(elt);
	sdrFetch(eltBuffer, (SdrAddress) elt);
	return eltBuffer.next;
}

SdrObject sdr_list_prev(Sdr sdrv, SdrObject elt)
{
	SdrListElt	eltBuffer;

	if (!sdrFetchSafe(sdrv))
	{
		writeMemoNote("[?] sdr_list_prev called but SDR not \
accessible", itoa(elt));
		printStackTrace();
		return 0;	/*	Defensive: SDR not accessible.	*/
	}

	CHKZERO(elt);
	sdrFetch(eltBuffer, (SdrAddress) elt);
	return eltBuffer.prev;
}

SdrObject sdr_list_search(Sdr sdrv, SdrObject fromElt, int reverse,
			SdrListCompareFn compare, void *dataBuffer)
{
	SdrObject	eltAddr;
	SdrListElt	elt;
	int		result;

	if (!sdrFetchSafe(sdrv))
	{
		writeMemoNote("[?] sdr_list_search called but SDR not \
accessible", itoa(fromElt));
		printStackTrace();
		return 0;	/*	Defensive: SDR not accessible.	*/
	}

	CHKZERO(fromElt);
	if (compare)	/* list assumed sorted; bail early if possible	*/
	{
		if (reverse)
		{
			for (eltAddr = fromElt; eltAddr != 0;
					eltAddr = elt.prev)
			{
				sdrFetch(elt, (SdrAddress) eltAddr);
				result = compare(sdrv, elt.data, dataBuffer);
				if (result < 0)
				{
					/*	past any possible match	*/

					return 0;
				}

				if (result == 0)
				{
					return eltAddr;
				}
			}
		}
		else
		{
			for (eltAddr = fromElt; eltAddr != 0;
					eltAddr = elt.next)
			{
				sdrFetch(elt, (SdrAddress) eltAddr);
				result = compare(sdrv, elt.data, dataBuffer);
				if (result > 0)
				{
					/*	past any possible match	*/

					return 0;
				}

				if (result == 0)
				{
					return eltAddr;
				}
			}
		}

		return 0;		/*	end of list reached	*/
	}

	/*	Use "==" since no comparison function was provided.	*/

	if (reverse)
	{
		for (eltAddr = fromElt; eltAddr != 0; eltAddr = elt.prev)
		{
			sdrFetch(elt, (SdrAddress) eltAddr);
			if (elt.data == (SdrAddress) dataBuffer) break;
		}
	}
	else
	{
		for (eltAddr = fromElt; eltAddr != 0; eltAddr = elt.next)
		{
			sdrFetch(elt, (SdrAddress) eltAddr);
			if (elt.data == (SdrAddress) dataBuffer) break;
		}
	}

	return eltAddr;
}

SdrObject sdr_list_list(Sdr sdrv, SdrObject elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(elt);
	sdrFetch(eltBuffer, (SdrAddress) elt);
	return eltBuffer.list;
}

SdrAddress sdr_list_data(Sdr sdrv, SdrObject elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(elt);
	sdrFetch(eltBuffer, (SdrAddress) elt);
	return eltBuffer.data;
}

SdrAddress Sdr_list_data_set(const char *file, int line, Sdr sdrv,
		SdrObject elt, SdrAddress new)
{
	SdrListElt	eltBuffer;
	SdrAddress	old;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (elt == 0)
	{
		oK(_xniEnd(file, line, "elt", sdrv));
		return 0;
	}

	sdrFetch(eltBuffer, (SdrAddress) elt);
	old = eltBuffer.data;
	eltBuffer.data = new;
	sdrPut((SdrAddress) elt, eltBuffer);
	return old;
}
