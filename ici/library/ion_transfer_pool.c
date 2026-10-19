/*
	ion_transfer_pool.c:	POSIX shared memory transfer pool
				implementation.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#include "platform.h"
#include "ion_transfer_pool.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*	Internal: compute offset of slot header from pool base.		*/

static IonPoolSlotHdr	*slotHdr(IonTransferPool *pool, int slot_idx)
{
	char	*base = (char *) pool->base;

	return (IonPoolSlotHdr *) (void *) (base + sizeof(IonPoolHdr)
			+ (size_t) slot_idx * pool->slot_size);
}

/*	Internal: compute offset of slot data from pool base.		*/

static char	*slotData(IonTransferPool *pool, int slot_idx)
{
	return (char *) slotHdr(pool, slot_idx) + sizeof(IonPoolSlotHdr);
}

int	pool_create(const char *name, uint32_t num_slots,
		uint32_t slot_data_capacity, IonTransferPool *pool)
{
	uint32_t	slot_size;
	size_t		total_size;
	int		fd;
	void		*base;
	IonPoolHdr	*hdr;
	uint32_t	i;

	if (name == NULL || pool == NULL || num_slots == 0
			|| slot_data_capacity == 0)
	{
		return -1;
	}

	slot_size = sizeof(IonPoolSlotHdr) + slot_data_capacity;
	total_size = sizeof(IonPoolHdr) + (size_t) num_slots * slot_size;

	/*	Create the shared memory region.  O_EXCL ensures
	 *	we don't accidentally attach to a stale pool.		*/

	fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, 0660);
	if (fd < 0)
	{
		if (errno == EEXIST)
		{
			/*	Stale pool from a previous run.
			 *	Unlink it and try again.		*/

			shm_unlink(name);
			fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL,
					0660);
		}

		if (fd < 0)
		{
			perror("pool_create: shm_open");
			return -1;
		}
	}

	if (ftruncate(fd, total_size) < 0)
	{
		perror("pool_create: ftruncate");
		close(fd);
		shm_unlink(name);
		return -1;
	}

	base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (base == MAP_FAILED)
	{
		perror("pool_create: mmap");
		close(fd);
		shm_unlink(name);
		return -1;
	}

	/*	Initialize pool header.					*/

	hdr = (IonPoolHdr *) base;
	memset(hdr, 0, sizeof(IonPoolHdr));
	hdr->magic = ION_POOL_MAGIC;
	hdr->version = ION_POOL_VERSION;
	hdr->num_slots = num_slots;
	hdr->slot_size = slot_size;

	/*	Initialize all slot headers.				*/

	pool->base = base;
	pool->slot_size = slot_size;

	for (i = 0; i < num_slots; i++)
	{
		IonPoolSlotHdr	*sh = slotHdr(pool, i);

		memset(sh, 0, sizeof(IonPoolSlotHdr));
		atomic_init(&sh->state, ION_SLOT_FREE);
		sh->capacity = slot_data_capacity;
	}

	/*	Populate the pool handle.				*/

	snprintf(pool->name, sizeof(pool->name), "%s", name);
	pool->fd = fd;
	pool->total_size = total_size;
	pool->num_slots = num_slots;
	pool->is_creator = 1;

	return 0;
}

int	pool_attach(const char *name, IonTransferPool *pool)
{
	int		fd;
	struct stat	sb;
	void		*base;
	IonPoolHdr	*hdr;

	if (name == NULL || pool == NULL)
	{
		return -1;
	}

	fd = shm_open(name, O_RDWR, 0);
	if (fd < 0)
	{
		perror("pool_attach: shm_open");
		return -1;
	}

	if (fstat(fd, &sb) < 0)
	{
		perror("pool_attach: fstat");
		close(fd);
		return -1;
	}

	base = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (base == MAP_FAILED)
	{
		perror("pool_attach: mmap");
		close(fd);
		return -1;
	}

	/*	Validate the pool header.				*/

	hdr = (IonPoolHdr *) base;
	if (hdr->magic != ION_POOL_MAGIC)
	{
		fprintf(stderr, "pool_attach: bad magic 0x%08x (expected "
				"0x%08x)\n", hdr->magic, ION_POOL_MAGIC);
		munmap(base, sb.st_size);
		close(fd);
		return -1;
	}

	if (hdr->version != ION_POOL_VERSION)
	{
		fprintf(stderr, "pool_attach: version mismatch %u vs %u\n",
				hdr->version, ION_POOL_VERSION);
		munmap(base, sb.st_size);
		close(fd);
		return -1;
	}

	/*	Populate the pool handle.				*/

	snprintf(pool->name, sizeof(pool->name), "%s", name);
	pool->fd = fd;
	pool->base = base;
	pool->total_size = (size_t) sb.st_size;
	pool->num_slots = hdr->num_slots;
	pool->slot_size = hdr->slot_size;
	pool->is_creator = 0;

	return 0;
}

int	pool_slot_acquire(IonTransferPool *pool)
{
	uint32_t	i;

	for (i = 0; i < pool->num_slots; i++)
	{
		IonPoolSlotHdr	*sh = slotHdr(pool, i);
		uint32_t	expected = ION_SLOT_FREE;

		if (atomic_compare_exchange_strong_explicit(
				&sh->state, &expected, ION_SLOT_WRITING,
				memory_order_acq_rel, memory_order_relaxed))
		{
			return (int) i;
		}
	}

	return -1;	/*	No free slots; caller should retry.	*/
}

char	*pool_slot_data(IonTransferPool *pool, int slot_idx)
{
	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return NULL;
	}

	return slotData(pool, slot_idx);
}

IonPoolSlotHdr	*pool_slot_hdr(IonTransferPool *pool, int slot_idx)
{
	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return NULL;
	}

	return slotHdr(pool, slot_idx);
}

uint32_t	pool_slot_capacity(const IonTransferPool *pool)
{
	if (pool == NULL || pool->slot_size <= sizeof(IonPoolSlotHdr))
	{
		return 0;
	}

	return pool->slot_size - (uint32_t) sizeof(IonPoolSlotHdr);
}

void	pool_slot_publish(IonTransferPool *pool, int slot_idx,
		uint64_t data_len, uint64_t xfer_id)
{
	IonPoolSlotHdr	*sh;

	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return;
	}

	sh = slotHdr(pool, slot_idx);

	/*	Write metadata first, then release with store-release
	 *	so the consumer sees consistent data.			*/

	sh->data_len = data_len;
	sh->xfer_id = xfer_id;
	atomic_store_explicit(&sh->state, ION_SLOT_READY,
			memory_order_release);
}

char	*pool_slot_read(IonTransferPool *pool, int slot_idx,
		uint64_t *data_len, uint64_t *xfer_id)
{
	IonPoolSlotHdr	*sh;
	uint32_t	expected = ION_SLOT_READY;

	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return NULL;
	}

	sh = slotHdr(pool, slot_idx);

	if (atomic_compare_exchange_strong_explicit(
			&sh->state, &expected, ION_SLOT_READING,
			memory_order_acq_rel, memory_order_relaxed))
	{
		if (data_len)
		{
			*data_len = sh->data_len;
		}

		if (xfer_id)
		{
			*xfer_id = sh->xfer_id;
		}

		return slotData(pool, slot_idx);
	}

	return NULL;
}

void	pool_slot_release(IonTransferPool *pool, int slot_idx)
{
	IonPoolSlotHdr	*sh;

	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return;
	}

	sh = slotHdr(pool, slot_idx);
	atomic_store_explicit(&sh->state, ION_SLOT_FREE,
			memory_order_release);
}

void	pool_slot_force_free(IonTransferPool *pool, int slot_idx)
{
	IonPoolSlotHdr	*sh;

	if (slot_idx < 0 || (uint32_t) slot_idx >= pool->num_slots)
	{
		return;
	}

	sh = slotHdr(pool, slot_idx);
	atomic_store_explicit(&sh->state, ION_SLOT_FREE,
			memory_order_release);
}

void	pool_reset_all_slots(IonTransferPool *pool)
{
	uint32_t	i;

	for (i = 0; i < pool->num_slots; i++)
	{
		pool_slot_force_free(pool, (int) i);
	}
}

void	pool_detach(IonTransferPool *pool)
{
	if (pool->base != NULL && pool->base != MAP_FAILED)
	{
		munmap(pool->base, pool->total_size);
		pool->base = NULL;
	}

	if (pool->fd >= 0)
	{
		close(pool->fd);
		pool->fd = -1;
	}
}

void	pool_destroy(IonTransferPool *pool)
{
	pool_detach(pool);

	if (pool->is_creator && pool->name[0] != '\0')
	{
		shm_unlink(pool->name);
		pool->name[0] = '\0';
	}
}
