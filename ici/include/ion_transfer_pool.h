/*
	ion_transfer_pool.h:	POSIX shared memory transfer pool for
				inter-process bundle data transfer.

				Provides a fixed-size pool of transfer
				slots in shared memory with lock-free
				state transitions for single-producer /
				single-consumer operation.

	Author: iondev33

	Copyright (c) 2026, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
								*/

#ifndef ION_TRANSFER_POOL_H
#define ION_TRANSFER_POOL_H

#include <stdint.h>
#include <stddef.h>

/*	The pool's slot state is a lock-free single-producer/single-consumer
 *	field in POSIX shared memory crossing a process boundary (an ION
 *	convergence-layer daemon and the external TCPCLv4 agent).  Its state
 *	transitions need a compare-and-swap with acquire/release ordering.
 *	ION's shared-memory atomic abstraction (ion_ipc_atomic_t in
 *	ion_atomic.h) does not yet expose a compare-and-swap or ordered
 *	operations -- only relaxed load, store, add and exchange -- so the
 *	pool uses C11 <stdatomic.h> directly.  That requires a C11 compiler
 *	with atomics; fail here, loudly, rather than later with a confusing
 *	error on one of ION's non-C11 fallback tiers.  Issue 1683 tracks
 *	extending the abstraction so the pool can drop this dependency.		*/

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L \
		|| defined(__STDC_NO_ATOMICS__)
#error "ion_transfer_pool needs C11 atomics; build the TCPCLv4 external CLA \
(--enable-tcpclv4-external) with a C11 compiler."
#endif

#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/*	Pool magic number: "TCP4" in ASCII = 0x54435034.		*/

#define ION_POOL_MAGIC		0x54435034
#define ION_POOL_VERSION	1

/*	Slot state machine (lock-free, single-producer/single-consumer):
 *
 *	FREE(0) -> WRITING(1) -> READY(2) -> READING(3) -> FREE(0)
 *
 *	Producer transitions: FREE -> WRITING -> READY
 *	Consumer transitions: READY -> READING -> FREE			*/

#define ION_SLOT_FREE		0
#define ION_SLOT_WRITING	1
#define ION_SLOT_READY		2
#define ION_SLOT_READING	3

/*	Pool header: first 64 bytes of the shared memory region.	*/

typedef struct
{
	uint32_t	magic;		/* ION_POOL_MAGIC		*/
	uint32_t	version;	/* ION_POOL_VERSION		*/
	uint32_t	num_slots;	/* Number of transfer slots	*/
	uint32_t	slot_size;	/* Total per slot: hdr + data	*/
	uint8_t		_pad[48];	/* Pad to 64 bytes		*/
} IonPoolHdr;

/*	Slot header: 64 bytes, cache-line aligned.
 *	The _Atomic qualifier on state enables lock-free
 *	compare-and-swap and store/load operations.			*/

typedef struct
{
	_Atomic uint32_t	state;		/* ION_SLOT_*		*/
	uint32_t		_pad0;
	uint64_t		xfer_id;	/* Transfer ID		*/
	uint64_t		data_len;	/* Actual data bytes	*/
	uint64_t		capacity;	/* Max data bytes	*/
	uint8_t			_pad1[32];	/* Pad to 64 bytes	*/
} IonPoolSlotHdr;

/*	Pool handle returned by pool_create / pool_attach.		*/

typedef struct
{
	char		name[256];	/* POSIX shm name		*/
	int		fd;		/* shm file descriptor		*/
	void		*base;		/* mmap base pointer		*/
	size_t		total_size;	/* Total mapped size		*/
	uint32_t	num_slots;	/* Copied from header		*/
	uint32_t	slot_size;	/* Copied from header		*/
	int		is_creator;	/* 1 if we created the pool	*/
} IonTransferPool;

/*	Create a new shared memory pool.
 *
 *	name:		POSIX shm name (e.g., "/ion_tcp4_tx_myduct")
 *	num_slots:	Number of transfer slots
 *	slot_data_capacity: Maximum data bytes per slot
 *	pool:		Output pool handle
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	pool_create(const char *name, uint32_t num_slots,
			uint32_t slot_data_capacity,
			IonTransferPool *pool);

/*	Attach to an existing shared memory pool.
 *
 *	Returns 0 on success, -1 on error.				*/

extern int	pool_attach(const char *name, IonTransferPool *pool);

/*	Acquire a FREE slot for writing.  Performs atomic
 *	CAS: FREE -> WRITING.
 *
 *	Returns slot index (>= 0) on success, -1 if no
 *	slots are available.  Caller should retry with
 *	backoff on -1.							*/

extern int	pool_slot_acquire(IonTransferPool *pool);

/*	Return a pointer to the slot's data region.			*/

extern char	*pool_slot_data(IonTransferPool *pool, int slot_idx);

/*	Return the per-slot data capacity (bytes) of an attached or
 *	created pool -- the same for every slot.  Returns 0 if pool
 *	is NULL or uninitialized.					*/

extern uint32_t	pool_slot_capacity(const IonTransferPool *pool);

/*	Return a pointer to the slot header.				*/

extern IonPoolSlotHdr	*pool_slot_hdr(IonTransferPool *pool, int slot_idx);

/*	Publish a slot: set data_len and xfer_id, then atomically
 *	transition WRITING -> READY.					*/

extern void	pool_slot_publish(IonTransferPool *pool, int slot_idx,
			uint64_t data_len, uint64_t xfer_id);

/*	Begin reading a READY slot: atomically READY -> READING.
 *
 *	Returns pointer to data region on success (sets *data_len
 *	and *xfer_id).  Returns NULL if slot is not in READY state.	*/

extern char	*pool_slot_read(IonTransferPool *pool, int slot_idx,
			uint64_t *data_len, uint64_t *xfer_id);

/*	Release a slot after reading: READING -> FREE.			*/

extern void	pool_slot_release(IonTransferPool *pool, int slot_idx);

/*	Force a slot to FREE regardless of current state.
 *	Used during crash recovery after agent disconnect.		*/

extern void	pool_slot_force_free(IonTransferPool *pool, int slot_idx);

/*	Force all slots in the pool to FREE (crash recovery).		*/

extern void	pool_reset_all_slots(IonTransferPool *pool);

/*	Detach from the pool (munmap).  Does NOT destroy
 *	the shared memory region.					*/

extern void	pool_detach(IonTransferPool *pool);

/*	Destroy the pool: munmap and shm_unlink.
 *	Only the creator should call this.				*/

extern void	pool_destroy(IonTransferPool *pool);

#ifdef __cplusplus
}
#endif

#endif /* ION_TRANSFER_POOL_H */
