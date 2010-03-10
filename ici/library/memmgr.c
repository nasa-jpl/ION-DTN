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

/*
 *	private global variables
 */

#define MEMMGR_LIMIT	(8)
#define MEMMGR_MAX_NAME	(15)

static char		mem_mgr_name_buffer[(MEMMGR_MAX_NAME + 1)
						* MEMMGR_LIMIT];

static void 		*memmgr_malloc(char *file, int line, size_t size);
static void		memmgr_free(char *file, int line, void * address);
static void		*memmgr_atop(unsigned long pointer);
static unsigned long	memmgr_ptoa(void * address);
static MemManager	mem_mgrs[MEMMGR_LIMIT] =
{
	{ "std", memmgr_malloc, memmgr_free, memmgr_atop, memmgr_ptoa }
};

static int 		mem_mgr_count = 1;

/*
 *	private standard memory allocator -- allocates and initializes
 */

static void	*memmgr_malloc(char *fileName, int lineNbr, size_t size)
{
	return acquireSystemMemory(size);
}

/*
 *	private standard memory deallocator
 */

static void	memmgr_free(char *fileName, int lineNbr, void *address)
{
	free(address);
}

/*
 *	private standard address-to-pointer converter
 */

static void	*memmgr_atop(unsigned long pointer)
{
	return (void *) pointer;
}

/*
 *	private standard pointer-to-address converter
 */

static unsigned long	memmgr_ptoa(void * address)
{
	return (unsigned long) address;
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

	int	i;
	char	*nameArea;

	REQUIRE(name);
	REQUIRE(afn);
	REQUIRE(ffn);
	REQUIRE(apfn);
	REQUIRE(pafn);
	for (i = 0; i < mem_mgr_count; i++)
	{
		if (mem_mgrs[i].name == NULL)
		{
			break;		/*	Not in list.		*/
		}

		if (strcmp(mem_mgrs[i].name, name) == 0)
		{
			return i;	/*	Found it.		*/
		}
	}

	if (i == MEMMGR_LIMIT)
	{
		errno = EINVAL;
		putErrmsg("too many memory managers", NULL);
		return -1;
	}

	/*	Make a copy of the name, in case it's not a literal.	*/

	nameArea = mem_mgr_name_buffer
			+ (mem_mgr_count * (MEMMGR_MAX_NAME + 1));
	strncpy(nameArea, name, MEMMGR_MAX_NAME);
	*(nameArea + MEMMGR_MAX_NAME) = '\0';

	/*	Insert memory manager into array.			*/

	mem_mgrs[i].name = nameArea;
	mem_mgrs[i].take = afn;
	mem_mgrs[i].release = ffn;
	mem_mgrs[i].AtoP = apfn;
	mem_mgrs[i].PtoA = pafn;
	mem_mgr_count++;
	return i;
}

int	memmgr_find(char *name)
{
	int	i;

	REQUIRE(name);
	for (i = 0; i < mem_mgr_count; i++)
	{
		if (mem_mgrs[i].name == NULL)
		{
			break;		/*	Not in list.		*/
		}

		if (strcmp(mem_mgrs[i].name, name) == 0)
		{
			return i;
		}
	}

	return -1;		/*	Memory manager not found.	*/
}

char	*memmgr_name(int mgrId)
{
	REQUIRE(mgrId >= 0);
	REQUIRE(mgrId < mem_mgr_count);
	return mem_mgrs[mgrId].name;
}

MemAllocator	memmgr_take(int mgrId)
{
	REQUIRE(mgrId >= 0);
	REQUIRE(mgrId < mem_mgr_count);
	return mem_mgrs[mgrId].take;
}

MemDeallocator	memmgr_release(int mgrId)
{
	REQUIRE(mgrId >= 0);
	REQUIRE(mgrId < mem_mgr_count);
	return mem_mgrs[mgrId].release;
}

MemAtoPConverter memmgr_AtoP(int mgrId)
{
	REQUIRE(mgrId >= 0);
	REQUIRE(mgrId < mem_mgr_count);
	return mem_mgrs[mgrId].AtoP;
}

MemPtoAConverter memmgr_PtoA(int mgrId)
{
	REQUIRE(mgrId >= 0);
	REQUIRE(mgrId < mem_mgr_count);
	return mem_mgrs[mgrId].PtoA;
}

int	memmgr_open(int memKey, long memSize, char **memPtr, int *smId,
		char *partitionName, PsmPartition *partition,
		int *memMgr, MemAllocator afn, MemDeallocator ffn,
		MemAtoPConverter apfn, MemPtoAConverter pafn)
{
	REQUIRE(memPtr);
	REQUIRE(memMgr);
	REQUIRE(partition);
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

	if (psm_manage(*memPtr, memSize, partitionName, partition) == Refused)
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

	if (memMgr != NULL)
	{
		*memMgr = memmgr_add(partitionName, afn, ffn, apfn, pafn);
	}

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

void	memmgr_destroy(int smId, PsmPartition *partition)
{
	char	*memptr;

	REQUIRE(partition);
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
