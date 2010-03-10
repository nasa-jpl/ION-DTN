/*
 *	sdrcatlg.c:	simple data recorder catalogue management
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
#include "sdr.h"

static const char	*catlgMsg = "can't catalog item in SDR";

/*	Private definition of SDR catalogue management structure.	*/

typedef struct
{
	char		name[MAX_SDR_NAME + 1];
	int		type;
	Object		object;
} CatalogueEntry;

/*	*	*	Object catalogue management functions	*	*/

static int	compareCatalogueEntries(Sdr sdrv, Address entryAddr, void *arg)
{
	CatalogueEntry	oldEntry;
	CatalogueEntry	*newEntry = (CatalogueEntry *) arg;

	sdrFetch(oldEntry, entryAddr);
	return strcmp(oldEntry.name, newEntry->name);
}

void	Sdr_catlg(char *file, int line, Sdr sdrv, char *name, int type,
		Object object)
{
	Object		catalogue;
	CatalogueEntry	entry;
	Object		elt;
	Object		addr;
	int		result;

	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
		return;
	}

	if (name == NULL || strlen(name) > MAX_SDR_NAME || object == 0)
	{
		errno = EINVAL;
		_putErrmsg(file, line, apiErrMsg, NULL);
		crashXn(sdrv);
		return;
	}

	strcpy(entry.name, name);
	entry.type = type;
	entry.object = object;
	sdrFetch(catalogue, ADDRESS_OF(catalogue));
	if (catalogue == 0)
	{
		catalogue = sdr_list_create(sdrv);
		if (catalogue == 0)
		{
			_putErrmsg(file, line, "Can't create SDR catalogue.",
					NULL);
			crashXn(sdrv);
			return;
		}

		patchMap(catalogue, catalogue);
	}

	for (elt = sdr_list_first(sdrv, catalogue); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		addr = (Object) sdr_list_data(sdrv, elt);
		result = compareCatalogueEntries(sdrv, (Address) addr, &entry);
		if (result == 0)
		{
			errno = EINVAL;
			_putErrmsg(file, line, "item is already in catalog",
					name);
			crashXn(sdrv);
			return;
		}

		if (result > 0)	/*	Past any possible match.	*/
		{
			break;
		}
	}

	/*	Insert catalogue entry before current elt, if any.	*/

	addr = _sdrzalloc(sdrv, sizeof entry);
	if (addr == 0)
	{
		_putErrmsg(file, line, "can't create catalog entry", NULL);
		return;
	}

	sdrPut((Address) addr, entry);
	if (elt)
	{
		if (Sdr_list_insert_before(file, line, sdrv, elt,
					(Address) addr) == 0)
		{
			_putErrmsg(file, line, catlgMsg, name);
			return;
		}
	}
	else
	{
		if (Sdr_list_insert_last(file, line, sdrv, catalogue,
					(Address) addr) == 0)
		{
			_putErrmsg(file, line, catlgMsg, name);
			return;
		}
	}
}

static Object	catlgLookup(Sdr sdrv, char *name)
{
	CatalogueEntry	entry;
	Object		catalogue;

	if (name == NULL || strlen(name) > MAX_SDR_NAME)
	{
		errno = EINVAL;
		putErrmsg(apiErrMsg, "name");
		crashXn(sdrv);
		return 0;
	}

	REQUIRE(sdrv);
	strcpy(entry.name, name);
	sdrFetch(catalogue, ADDRESS_OF(catalogue));
	if (catalogue == 0)
	{
		return 0;	/*	No catalogue; can't be in it.	*/
	}

	return sdr_list_search(sdrv, sdr_list_first(sdrv, catalogue), 0,
			compareCatalogueEntries, &entry);
}

Object	sdr_find(Sdr sdrv, char *name, int *type)
{
	SdrState	*sdr;
	Object		elt;
	CatalogueEntry	entry;

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	elt = catlgLookup(sdrv, name);
	if (elt)
	{
		sdrFetch(entry, sdr_list_data(sdrv, elt));
		releaseSdr(sdr);
		if (type)
		{
			*type = entry.type;
		}

		return entry.object;
	}

	releaseSdr(sdr);
	return 0;
}

void	Sdr_uncatlg(char *file, int line, Sdr sdrv, char *name)
{
	Object	elt;

	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
		return;
	}

	elt = catlgLookup(sdrv, name);
	if (elt)
	{
		sdrFree((Object) sdr_list_data(sdrv, elt));
		Sdr_list_delete(file, line, sdrv, elt, NULL, NULL);
	}
}

Object	sdr_read_catlg(Sdr sdrv, char *name, int *type, Object *object,
		Object prev_elt)
{
	SdrState	*sdr;
	Object		catalogue;
	Object		elt;
	CatalogueEntry	entry;

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	if (prev_elt == 0)
	{
		sdrFetch(catalogue, ADDRESS_OF(catalogue));
		elt = sdr_list_first(sdrv, catalogue);
	}
	else
	{
		elt = sdr_list_next(sdrv, prev_elt);
	}

	if (elt == 0)
	{
		releaseSdr(sdr);
		return elt;
	}

	sdrFetch(entry, sdr_list_data(sdrv, elt));
	if (name)
	{
		strcpy(name, entry.name);
	}

	if (type)
	{
		*type = entry.type;
	}

	if (object)
	{
		*object = entry.object;
	}

	return elt;
}
