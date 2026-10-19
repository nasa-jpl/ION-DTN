'''
bridge_pool.py: ctypes wrapper for ION shared memory transfer pools.

Calls libici.so pool_* functions via ctypes to ensure correct atomic
operations on slot state fields.  No reimplementation of lock-free
logic in Python.

Author: iondev33

Copyright (c) 2026, California Institute of Technology.
ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
'''

import ctypes
import ctypes.util
import logging
import time

LOGGER = logging.getLogger(__name__)

# Mirror of IonTransferPool struct from ici/include/ion_transfer_pool.h
class IonTransferPool(ctypes.Structure):
    _fields_ = [
        ('name', ctypes.c_char * 256),
        ('fd', ctypes.c_int),
        ('base', ctypes.c_void_p),
        ('total_size', ctypes.c_size_t),
        ('num_slots', ctypes.c_uint32),
        ('slot_size', ctypes.c_uint32),
        ('is_creator', ctypes.c_int),
    ]


_libici = None


def _load_libici():
    '''Load libici.so and configure function signatures.'''
    global _libici
    if _libici is not None:
        return _libici

    _libici = ctypes.CDLL('libici.so')

    # int pool_attach(const char *name, IonTransferPool *pool)
    _libici.pool_attach.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(IonTransferPool)
    ]
    _libici.pool_attach.restype = ctypes.c_int

    # int pool_slot_acquire(IonTransferPool *pool)
    _libici.pool_slot_acquire.argtypes = [
        ctypes.POINTER(IonTransferPool)
    ]
    _libici.pool_slot_acquire.restype = ctypes.c_int

    # char *pool_slot_data(IonTransferPool *pool, int slot_idx)
    _libici.pool_slot_data.argtypes = [
        ctypes.POINTER(IonTransferPool),
        ctypes.c_int
    ]
    _libici.pool_slot_data.restype = ctypes.c_void_p

    # void pool_slot_publish(IonTransferPool *pool, int slot_idx,
    #                        uint64_t data_len, uint64_t xfer_id)
    _libici.pool_slot_publish.argtypes = [
        ctypes.POINTER(IonTransferPool),
        ctypes.c_int,
        ctypes.c_uint64,
        ctypes.c_uint64
    ]
    _libici.pool_slot_publish.restype = None

    # char *pool_slot_read(IonTransferPool *pool, int slot_idx,
    #                      uint64_t *data_len, uint64_t *xfer_id)
    _libici.pool_slot_read.argtypes = [
        ctypes.POINTER(IonTransferPool),
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_uint64),
        ctypes.POINTER(ctypes.c_uint64)
    ]
    _libici.pool_slot_read.restype = ctypes.c_void_p

    # void pool_slot_release(IonTransferPool *pool, int slot_idx)
    _libici.pool_slot_release.argtypes = [
        ctypes.POINTER(IonTransferPool),
        ctypes.c_int
    ]
    _libici.pool_slot_release.restype = None

    # void pool_slot_force_free(IonTransferPool *pool, int slot_idx)
    _libici.pool_slot_force_free.argtypes = [
        ctypes.POINTER(IonTransferPool),
        ctypes.c_int
    ]
    _libici.pool_slot_force_free.restype = None

    # void pool_reset_all_slots(IonTransferPool *pool)
    _libici.pool_reset_all_slots.argtypes = [
        ctypes.POINTER(IonTransferPool)
    ]
    _libici.pool_reset_all_slots.restype = None

    # void pool_detach(IonTransferPool *pool)
    _libici.pool_detach.argtypes = [
        ctypes.POINTER(IonTransferPool)
    ]
    _libici.pool_detach.restype = None

    return _libici


class Pool:
    '''Python wrapper for an ION shared memory transfer pool.

    Attaches to an existing pool created by tcp4clo or tcp4cli.
    All slot state transitions use the C library's atomic operations.
    '''

    def __init__(self, name, max_retries=30):
        '''Attach to a shared memory pool.

        :param name: POSIX shm name (e.g., "/ion_tcp4_tx_loopback")
        :param max_retries: Number of attach attempts (1s between retries)
        :raises RuntimeError: If pool cannot be attached after max_retries
        '''
        self._lib = _load_libici()
        self._pool = IonTransferPool()
        self._name = name

        name_bytes = name.encode('utf-8') if isinstance(name, str) else name

        for attempt in range(max_retries):
            if self._lib.pool_attach(name_bytes,
                                     ctypes.byref(self._pool)) == 0:
                LOGGER.info('Attached to pool %s (slots=%d, slot_size=%d)',
                            name, self._pool.num_slots, self._pool.slot_size)
                return
            if attempt < max_retries - 1:
                LOGGER.debug('Pool %s not ready, retry %d/%d',
                             name, attempt + 1, max_retries)
                time.sleep(1)

        raise RuntimeError(f'Cannot attach to pool {name} '
                           f'after {max_retries} retries')

    @property
    def num_slots(self):
        return self._pool.num_slots

    @property
    def slot_size(self):
        return self._pool.slot_size

    def acquire(self):
        '''Acquire a FREE slot for writing (FREE -> WRITING).

        :return: Slot index (>= 0) on success, -1 if no slots available.
        '''
        return self._lib.pool_slot_acquire(ctypes.byref(self._pool))

    def slot_data_ptr(self, slot_idx):
        '''Return a ctypes pointer to the slot's data region.'''
        return self._lib.pool_slot_data(ctypes.byref(self._pool), slot_idx)

    def publish(self, slot_idx, data_len, xfer_id):
        '''Publish a slot (WRITING -> READY).'''
        self._lib.pool_slot_publish(ctypes.byref(self._pool),
                                    slot_idx, data_len, xfer_id)

    def read(self, slot_idx):
        '''Read a READY slot (READY -> READING).

        :return: Tuple of (data_bytes, xfer_id), or (None, 0) if not READY.
        '''
        data_len = ctypes.c_uint64()
        xfer_id = ctypes.c_uint64()
        ptr = self._lib.pool_slot_read(ctypes.byref(self._pool), slot_idx,
                                       ctypes.byref(data_len),
                                       ctypes.byref(xfer_id))
        if not ptr:
            return None, 0

        data = ctypes.string_at(ptr, data_len.value)
        return data, xfer_id.value

    def release(self, slot_idx):
        '''Release a slot (READING -> FREE).'''
        self._lib.pool_slot_release(ctypes.byref(self._pool), slot_idx)

    def force_free(self, slot_idx):
        '''Force a slot to FREE regardless of current state.'''
        self._lib.pool_slot_force_free(ctypes.byref(self._pool), slot_idx)

    def reset_all_slots(self):
        '''Force all slots to FREE (crash recovery).'''
        self._lib.pool_reset_all_slots(ctypes.byref(self._pool))

    def write_and_publish(self, slot_idx, data, xfer_id):
        '''Write data into slot and publish (WRITING -> READY).

        :param slot_idx: Slot index from acquire()
        :param data: Bundle bytes to write
        :param xfer_id: Transfer ID for this bundle
        :raises ValueError: If data exceeds slot capacity
        '''
        dst = self.slot_data_ptr(slot_idx)
        if not dst:
            raise RuntimeError(f'Invalid slot data pointer for slot {slot_idx}')

        ctypes.memmove(dst, data, len(data))
        self.publish(slot_idx, len(data), xfer_id)

    def close(self):
        '''Detach from the pool (munmap). Does NOT destroy the pool.'''
        self._lib.pool_detach(ctypes.byref(self._pool))
        LOGGER.info('Detached from pool %s', self._name)
