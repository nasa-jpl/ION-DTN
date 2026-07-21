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

/*	Private definition of SDR catalogue management structure.	*/

typedef struct
{
	char		name[MAX_SDR_NAME + 1];
	int		type;
	SdrObject	object;
} CatalogueEntry;

/*	*	*	Object catalogue management functions	*	*/

static int compareCatalogueEntries(Sdr sdrv, SdrAddress entryAddr, void *arg)
{
	CatalogueEntry	oldEntry;
	CatalogueEntry	*newEntry = (CatalogueEntry *) arg;

	sdrFetch(oldEntry, entryAddr);
	return strcmp(oldEntry.name, newEntry->name);
}

void	Sdr_catlg(const char *file, int line, Sdr sdrv, char *name, int type,
		SdrObject object)
{
	SdrMap		*map = _mapImage(sdrv);
	SdrObject	catalogue;
	CatalogueEntry	entry;
	SdrObject	elt;
	SdrObject	addr;
	int		result;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	if (object == 0 || name == NULL || strlen(name) > MAX_SDR_NAME)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return;
	}

	istrcpy(entry.name, name, sizeof entry.name);
	entry.type = type;
	entry.object = object;
	sdrFetch(catalogue, ADDRESS_OF(catalogue));
	if (catalogue == 0)
	{
		catalogue = sdr_list_create(sdrv);
		if (catalogue == 0)
		{
			oK(_iEnd(file, line, "catalogue"));
			return;
		}

		patchMap(catalogue, catalogue);
	}

	for (elt = sdr_list_first(sdrv, catalogue); elt;
			elt = sdr_list_next(sdrv, elt))
	{
		addr = (SdrObject) sdr_list_data(sdrv, elt);
		result = compareCatalogueEntries(sdrv, (SdrAddress) addr, &entry);
		if (result == 0)
		{
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
		oK(_iEnd(file, line, "addr"));
		return;
	}

	sdrPut((SdrAddress) addr, entry);
	if (elt)
	{
		if (Sdr_list_insert_before(file, line, sdrv, elt,
				    (SdrAddress) addr) == 0)
		{
			oK(_iEnd(file, line, name));
			return;
		}
	}
	else
	{
		if (Sdr_list_insert_last(file, line, sdrv, catalogue,
				    (SdrAddress) addr) == 0)
		{
			oK(_iEnd(file, line, name));
			return;
		}
	}
}

static SdrObject catlgLookup(Sdr sdrv, char *name)
{
	SdrMap		*map = _mapImage(sdrv);
	CatalogueEntry	entry;
	SdrObject	catalogue;

	XNCHKZERO(!(name == NULL || strlen(name) > MAX_SDR_NAME));
	istrcpy(entry.name, name, sizeof entry.name);
	sdrFetch(catalogue, ADDRESS_OF(catalogue));
	if (catalogue == 0)
	{
		return 0;	/*	No catalogue; can't be in it.	*/
	}

	return sdr_list_search(sdrv, sdr_list_first(sdrv, catalogue), 0,
			compareCatalogueEntries, &entry);
}

SdrObject sdr_find(Sdr sdrv, char *name, int *type)
{
	SdrObject	elt;
	CatalogueEntry	entry;

	CHKZERO(sdrFetchSafe(sdrv));
	elt = catlgLookup(sdrv, name);
	if (elt)
	{
		sdrFetch(entry, sdr_list_data(sdrv, elt));
		if (type)
		{
			*type = entry.type;
		}

		return entry.object;
	}

	return 0;
}

void	Sdr_uncatlg(const char *file, int line, Sdr sdrv, char *name)
{
	SdrObject elt;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	elt = catlgLookup(sdrv, name);
	if (elt)
	{
		sdrFree((SdrObject) sdr_list_data(sdrv, elt));
		Sdr_list_delete(file, line, sdrv, elt, NULL, NULL);
	}
}

SdrObject sdr_read_catlg(Sdr sdrv, char *name, int *type, SdrObject *object,
		SdrObject prev_elt)
{
	SdrMap		*map = _mapImage(sdrv);
	SdrObject	catalogue;
	SdrObject	elt;
	CatalogueEntry	entry;

	CHKZERO(sdrFetchSafe(sdrv));
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
		return elt;
	}

	sdrFetch(entry, sdr_list_data(sdrv, elt));
	if (name)
	{
		istrcpy(name, entry.name, sizeof entry.name);
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
