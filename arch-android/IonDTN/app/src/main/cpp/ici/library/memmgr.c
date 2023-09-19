/*
 *	memmgr.c:	functions for enabling multiple dynamic
 *			memory management systems to coexist in
 *			a single address space.
 *									
 *	Copyright (c) 2001, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	Modification History:
 *	Date	  Who	What
 *	02-19-01  SCB	Initial implementation.
 *
 */

#include "platform.h"
#include "memmgr.h"

typedef struct
{
	char		*name;
	MemAllocator	take;
	MemDeallocator	release;
	MemAtoPConverter AtoP;
	MemPtoAConverter PtoA;
} MemManager;

#define MEMMGR_LIMIT	(8)
#define MEMMGR_MAX_NAME	(15)

/*
 *	private standard memory allocator -- allocates and initializes
 */

static void	*memmgr_malloc(const char *fileName, int lineNbr, size_t size)
{
	return acquireSystemMemory(size);
}

/*
 *	private standard memory deallocator
 */

static void	memmgr_free(const char *fileName, int lineNbr, void *address)
{
	TRACK_FREE(address);
	free(address);
}

/*
 *	private standard address-to-pointer converter
 */

static void	*memmgr_atop(uaddr pointer)
{
	return (void *) pointer;
}

/*
 *	private standard pointer-to-address converter
 */

static uaddr	memmgr_ptoa(void * address)
{
	return (uaddr) address;
}

/*	Default null memory management functions.			*/

static void	*null_malloc(const char *fileName, int lineNbr, size_t size)
{
	return NULL;
}

static void	null_free(const char *fileName, int lineNbr, void *address)
{
	return;
}

static void	*null_atop(uaddr pointer)
{
	return NULL;
}

static uaddr	null_ptoa(void * address)
{
	return 0;
}

/*
 *	private global variables
 */

static int	_mem_mgrs(int nbr, char *name, MemAllocator take,
			MemDeallocator release, MemAtoPConverter AtoP,
			MemPtoAConverter PtoA, MemManager **mgrp)
{
	static char		mem_mgr_name_buffer[(MEMMGR_MAX_NAME + 1)
					* MEMMGR_LIMIT];
	static MemManager	mem_mgrs[MEMMGR_LIMIT] =	{
		{ "std", memmgr_malloc, memmgr_free, memmgr_atop, memmgr_ptoa }
								};
	static int 		mem_mgr_count = 1;
	int			i;
	MemManager		*mgr;
	char			*nameArea;

	if (nbr < 0)	/*	Add or lookup memory manager.		*/
	{
		CHKERR(name);
		for (i = 0, mgr = mem_mgrs; i < mem_mgr_count; i++, mgr++)
		{
			if (strcmp(mgr->name, name) != 0)
			{
				continue;
			}

			/*	Found the named memory manager.		*/

			if (mgrp)	/*	Lookup.			*/
			{
				*mgrp = mgr;
				return i;
			}

			/*	Re-adding existing memory manager.	*/

			if (mgr->take != take
			|| mgr->release != release
			|| mgr->AtoP != AtoP
			|| mgr->PtoA != PtoA)
			{
				putErrmsg("memmgr definition clash.", name);
				return -1;
			}

			return i;	/*	Already added.		*/
		}

		/*	This is an unknown memory manager name.		*/

		if (mgrp)		/*	Lookup.			*/
		{
			*mgrp = NULL;
			return -1;	/*	Not found.		*/
		}

		/*	Adding new memory manager.			*/

		if (i == MEMMGR_LIMIT)
		{
			putErrmsg("Too many memory managers.", NULL);
			return -1;
		}

		if (take == NULL)
		{
			take = null_malloc;
		}

		if (release == NULL)
		{
			release = null_free;
		}

		if (AtoP == NULL)
		{
			AtoP = null_atop;
		}

		if (PtoA == NULL)
		{
			PtoA = null_ptoa;
		}

		/*	Copy the name, in case it's not a literal.	*/

		nameArea = mem_mgr_name_buffer
				+ (mem_mgr_count * (MEMMGR_MAX_NAME + 1));
		istrcpy(nameArea, name, MEMMGR_MAX_NAME);

		/*	Insert memory manager into array.		*/

		mgr->name = nameArea;
		mgr->take = take;
		mgr->release = release;
		mgr->AtoP = AtoP;
		mgr->PtoA = PtoA;
		mem_mgr_count++;
		return i;
	}

	/*	Just want pointer to numbered memory manager.		*/

	CHKERR(mgrp);
	if (nbr < mem_mgr_count)
	{
		*mgrp = mem_mgrs + nbr;
		return nbr;
	}

	*mgrp = NULL;
	return -1;
}

/*
 *	public functions
 */

int	memmgr_add(char *name, MemAllocator afn, MemDeallocator ffn,
		MemAtoPConverter apfn, MemPtoAConverter pafn)
{
	/*	If any existing memory manager of this name exists,
		return its idx.  Else add a new memory manager of
		this name in the first unused mem_mgr structure, and
		return its idx.						*/

	return _mem_mgrs(-1, name, afn, ffn, apfn, pafn, NULL);
}

int	memmgr_find(char *name)
{
	MemManager	*mgr;

	return _mem_mgrs(-1, name, NULL, NULL, NULL, NULL, &mgr);
}

char	*memmgr_name(int mgrId)
{
	MemManager	*mgr;

	CHKNULL(mgrId >= 0);
	if (_mem_mgrs(mgrId, NULL, NULL, NULL, NULL, NULL, &mgr) < 0)
	{
		return NULL;
	}

	return mgr->name;
}

MemAllocator	memmgr_take(int mgrId)
{
	MemManager	*mgr;

	CHKNULL(mgrId >= 0);
	if (_mem_mgrs(mgrId, NULL, NULL, NULL, NULL, NULL, &mgr) < 0)
	{
		return NULL;
	}

	return mgr->take;
}

MemDeallocator	memmgr_release(int mgrId)
{
	MemManager	*mgr;

	CHKNULL(mgrId >= 0);
	if (_mem_mgrs(mgrId, NULL, NULL, NULL, NULL, NULL, &mgr) < 0)
	{
		return NULL;
	}

	return mgr->release;
}

MemAtoPConverter memmgr_AtoP(int mgrId)
{
	MemManager	*mgr;

	CHKNULL(mgrId >= 0);
	if (_mem_mgrs(mgrId, NULL, NULL, NULL, NULL, NULL, &mgr) < 0)
	{
		return NULL;
	}

	return mgr->AtoP;
}

MemPtoAConverter memmgr_PtoA(int mgrId)
{
	MemManager	*mgr;

	CHKNULL(mgrId >= 0);
	if (_mem_mgrs(mgrId, NULL, NULL, NULL, NULL, NULL, &mgr) < 0)
	{
		return NULL;
	}

	return mgr->PtoA;
}

int	memmgr_open(int memKey, size_t memSize, char **memPtr, uaddr *smId,
		char *partitionName, PsmPartition *partition,
		int *memMgr, MemAllocator afn, MemDeallocator ffn,
		MemAtoPConverter apfn, MemPtoAConverter pafn)
{
	PsmMgtOutcome	outcome;

	CHKERR(memPtr);
	CHKERR(memMgr);
	CHKERR(partition);
	switch (sm_ShmAttach(memKey, memSize, memPtr, smId))
	{
	case 0:		/*	Attaching to an existing segment.	*/
	case 1:		/*	Have acquired a new segment.		*/
		break;

	default:	/*	Attach failed.				*/
		putErrmsg("Can't open memory region.", NULL);
		return -1;
	}

	/*	Initialize it if necessary.				*/

	if (psm_manage(*memPtr, memSize, partitionName, partition, &outcome) < 0
	|| outcome == Refused)
	{
		putErrmsg("Can't manage memory region.", NULL);
		return -1;
	}

	/*	Establish memory manager for privately managed
	 *	objects allocated out of this shared-memory region,
	 *	such as Lysts.  Such objects reside in shared memory
	 *	but are usable only by the process that created them,
	 *	because they are accessed using raw memory pointers
	 *	rather than PsmAddress references (which are offsets
	 *	within the shared memory partition that are usable
	 *	no matter what a process thinks the base address of
	 *	the shared-memory region is).				*/

	*memMgr = memmgr_add(partitionName, afn, ffn, apfn, pafn);

	/*	Initialize SDR global control header if necessary.	*/

	if (psm_get_root(*partition) == 0)	/*	No catalog yet.	*/
	{
		if (psm_add_catlg(*partition) < 0)
		{
			putErrmsg("Can't add catalog to memory region.", NULL);
			return -1;
		}
	}

	return 0;
}

void	memmgr_destroy(uaddr smId, PsmPartition *partition)
{
	char	*memptr;

	CHKVOID(partition);
	if (*partition == NULL)	/*	Nothing to do.			*/
	{
		return;
	}

	memptr = psm_space(*partition);
	psm_erase_root(*partition);
	psm_erase(*partition);
	*partition = NULL;
	sm_ShmDetach(memptr);
	sm_ShmDestroy(smId);
}
