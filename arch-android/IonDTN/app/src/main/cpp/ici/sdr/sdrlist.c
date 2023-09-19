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
	Address		userData;
	Object		first;	/*	first element in the list	*/
	Object		last;	/*	last element in the list	*/
	size_t		length;	/*	number of elements in the list	*/
} SdrList;

typedef struct
{
	Object		list;	/*	list that this element is in	*/
	Object		prev;	/*	previous element in list	*/
	Object		next;	/*	next element in list		*/
	Object		data;	/*	data for this element		*/
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

Object	Sdr_list_create(const char *file, int line, Sdr sdrv)
{
	Object	list;
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
	sdrPut((Address) list, listBuffer);
	return list;
}

void	Sdr_list_destroy(const char *file, int line, Sdr sdrv, Object list,
		SdrListDeleteFn deleteFn, void *arg)
{
	SdrList		listBuffer;
	Object		elt;
	Object		next;
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

	sdrFetch(listBuffer, (Address) list);
	for (elt = listBuffer.first; elt != 0; elt = next)
	{
		sdrFetch(eltBuffer, (Address) elt);
		next = eltBuffer.next;
		if (deleteFn)
		{
			deleteFn(sdrv, elt, arg);
		}

		/* just in case user mistakenly accesses later... */
		sdr_list__elt_clear(&eltBuffer);
		sdrPut((Address) elt, eltBuffer);
		sdrFree(elt);
	}

	/* just in case user mistakenly accesses later... */
	sdr_list__clear(&listBuffer);
	sdrPut((Address) list, listBuffer);
	sdrFree(list);
}

Address	sdr_list_user_data(Sdr sdrv, Object list)
{
	SdrList		listBuffer;

	CHKZERO(list);
	sdrFetch(listBuffer, list);
	return listBuffer.userData;
}

void	Sdr_list_user_data_set(const char *file, int line, Sdr sdrv,
		Object list, Address data)
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
	sdrPut((Address) list, listBuffer);
}

size_t	sdr_list_length(Sdr sdrv, Object list)
{
	SdrList		listBuffer;

	CHKERR(list);
	sdrFetch(listBuffer, list);
	return listBuffer.length;
}

Object	Sdr_list_insert_first(const char *file, int line, Sdr sdrv, Object list,
		Address data)
{
	SdrList		listBuffer;
	Object		elt;
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
		oK(_iEnd(file, line, "elt"));
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element at the beginning of the list */
	sdrFetch(listBuffer, (Address) list);
	eltBuffer.prev = 0;
	eltBuffer.next = listBuffer.first;
	sdrPut((Address) elt, eltBuffer);
	if (listBuffer.first != 0)
	{
		sdrFetch(eltBuffer, (Address) listBuffer.first);
		eltBuffer.prev = elt;
		sdrPut((Address) listBuffer.first, eltBuffer);
	}
	else
	{
		listBuffer.last = elt;
	}

	listBuffer.first = elt;
	listBuffer.length += 1;
	sdrPut((Address) list, listBuffer);
	return elt;
}

Object	Sdr_list_insert_last(const char *file, int line, Sdr sdrv, Object list,
		Address data)
{
	SdrList		listBuffer;
	Object		elt;
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
		oK(_iEnd(file, line, "elt"));
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element at the end of the list */
	sdrFetch(listBuffer, (Address) list);
	eltBuffer.prev = listBuffer.last;
	eltBuffer.next = 0;
	sdrPut((Address) elt, eltBuffer);
	if (listBuffer.last != 0)
	{
		sdrFetch(eltBuffer, (Address) listBuffer.last);
		eltBuffer.next = elt;
		sdrPut((Address) listBuffer.last, eltBuffer);
	}
	else
	{
		listBuffer.first = elt;
	}

	listBuffer.last = elt;
	listBuffer.length += 1;
	sdrPut((Address) list, listBuffer);
	return elt;
}

Object	Sdr_list_insert_before(const char *file, int line, Sdr sdrv,
		Object oldElt, Address data)
{
	SdrListElt	oldEltBuffer;
	Object		list;
	SdrList		listBuffer;
	Object		elt;
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

	sdrFetch(oldEltBuffer, (Address) oldElt);
	if ((list = oldEltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		oK(_iEnd(file, line, "elt"));
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element before the specified element */
	sdrFetch(listBuffer, (Address) list);
	eltBuffer.prev = oldEltBuffer.prev;
	eltBuffer.next = oldElt;
	sdrPut((Address) elt, eltBuffer);
	if (oldEltBuffer.prev != 0)
	{
		sdrFetch(eltBuffer, (Address) oldEltBuffer.prev);
		eltBuffer.next = elt;
		sdrPut((Address) oldEltBuffer.prev, eltBuffer);
	}
	else
	{
		listBuffer.first = elt;
	}

	oldEltBuffer.prev = elt;
	sdrPut((Address) oldElt, oldEltBuffer);
	listBuffer.length += 1;
	sdrPut((Address) list, listBuffer);
	return elt;
}

Object	Sdr_list_insert_after(const char *file, int line, Sdr sdrv,
		Object oldElt, Address data)
{
	SdrListElt	oldEltBuffer;
	Object		list;
	SdrList		listBuffer;
	Object		elt;
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

	sdrFetch(oldEltBuffer, (Address) oldElt);
	if ((list = oldEltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return 0;
	}

	/* create new element */
	elt = _sdrzalloc(sdrv, sizeof(SdrListElt));
	if (elt == 0)
	{
		oK(_iEnd(file, line, "elt"));
		return 0;
	}

	sdr_list__elt_clear(&eltBuffer);
	eltBuffer.list = list;
	eltBuffer.data = data;

	/* insert new element after the specified element */
	sdrFetch(listBuffer, (Address) list);
	eltBuffer.next = oldEltBuffer.next;
	eltBuffer.prev = oldElt;
	sdrPut((Address) elt, eltBuffer);
	if (oldEltBuffer.next != 0)
	{
		sdrFetch(eltBuffer, (Address) oldEltBuffer.next);
		eltBuffer.prev = elt;
		sdrPut((Address) oldEltBuffer.next, eltBuffer);
	}
	else
	{
		listBuffer.last = elt;
	}

	oldEltBuffer.next = elt;
	sdrPut((Address) oldElt, oldEltBuffer);
	listBuffer.length += 1;
	sdrPut((Address) list, listBuffer);
	return elt;
}

Object	Sdr_list_insert(const char *file, int line, Sdr sdrv, Object list,
		Address data, SdrListCompareFn compare, void *dataBuffer)
{
	SdrList		listBuffer;
	SdrListElt	eltBuffer;
	Object		elt;

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

	sdrFetch(listBuffer, (Address) list);
	for (elt = listBuffer.last; elt != 0; elt = eltBuffer.prev)
	{
		sdrFetch(eltBuffer, (Address) elt);
		if (compare(sdrv, eltBuffer.data, dataBuffer) <= 0) break;
	}

	/*	Insert into list at this point.				*/

	if (elt == 0)
	{
		return Sdr_list_insert_first(file, line, sdrv, list, data);
	}

	return Sdr_list_insert_after(file, line, sdrv, elt, data);
}

void	Sdr_list_delete(const char *file, int line, Sdr sdrv, Object elt,
		SdrListDeleteFn deleteFn, void *arg)
{
	SdrListElt	eltBuffer;
	Object		list;
	SdrList		listBuffer;
	Object		next;
	Object		prev;

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

	sdrFetch(eltBuffer, (Address) elt);
	if ((list = eltBuffer.list) == 0)
	{
		oK(_xniEnd(file, line, "list", sdrv));
		return;
	}

	sdrFetch(listBuffer, (Address) list);
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
	sdrPut((Address) elt, eltBuffer);
	sdrFree(elt);
	if (prev)
	{
		sdrFetch(eltBuffer, (Address) prev);
		eltBuffer.next = next;
		sdrPut((Address) prev, eltBuffer);
	}
	else
	{
		listBuffer.first = next;
	}

	if (next)
	{
		sdrFetch(eltBuffer, (Address) next);
		eltBuffer.prev = prev;
		sdrPut((Address) next, eltBuffer);
	}
	else
	{
		listBuffer.last = prev;
	}

	listBuffer.length -= 1;
	sdrPut((Address) list, listBuffer);
}

Object	sdr_list_first(Sdr sdrv, Object list)
{
	SdrList		listBuffer;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(list);
	sdrFetch(listBuffer, (Address) list);
	return listBuffer.first;
}

Object	sdr_list_last(Sdr sdrv, Object list)
{
	SdrList		listBuffer;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(list);
	sdrFetch(listBuffer, (Address) list);
	return listBuffer.last;
}

Object	sdr_list_next(Sdr sdrv, Object elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(elt);
	sdrFetch(eltBuffer, (Address) elt);
	return eltBuffer.next;
}

Object	sdr_list_prev(Sdr sdrv, Object elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(elt);
	sdrFetch(eltBuffer, (Address) elt);
	return eltBuffer.prev;
}

Object	sdr_list_search(Sdr sdrv, Object fromElt, int reverse,
			SdrListCompareFn compare, void *dataBuffer)
{
	Object		eltAddr;
	SdrListElt	elt;
	int		result;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(fromElt);
	if (compare)	/* list assumed sorted; bail early if possible	*/
	{
		if (reverse)
		{
			for (eltAddr = fromElt; eltAddr != 0;
					eltAddr = elt.prev)
			{
				sdrFetch(elt, (Address) eltAddr);
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
				sdrFetch(elt, (Address) eltAddr);
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
			sdrFetch(elt, (Address) eltAddr);
			if (elt.data == (Address) dataBuffer) break;
		}
	}
	else
	{
		for (eltAddr = fromElt; eltAddr != 0; eltAddr = elt.next)
		{
			sdrFetch(elt, (Address) eltAddr);
			if (elt.data == (Address) dataBuffer) break;
		}
	}

	return eltAddr;
}

Object	sdr_list_list(Sdr sdrv, Object elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(elt);
	sdrFetch(eltBuffer, (Address) elt);
	return eltBuffer.list;
}

Address	sdr_list_data(Sdr sdrv, Object elt)
{
	SdrListElt	eltBuffer;

	CHKZERO(elt);
	sdrFetch(eltBuffer, (Address) elt);
	return eltBuffer.data;
}

Address	Sdr_list_data_set(const char *file, int line, Sdr sdrv, Object elt,
		Address new)
{
	SdrListElt	eltBuffer;
	Address		old;

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

	sdrFetch(eltBuffer, (Address) elt);
	old = eltBuffer.data;
	eltBuffer.data = new;
	sdrPut((Address) elt, eltBuffer);
	return old;
}
