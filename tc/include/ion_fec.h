/*
 * ion_fec.h - ION Forward Error Correction wrapper for Jerasure
 *
 * Copyright (c) 2024, California Institute of Technology.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.
 */

#ifndef ION_FEC_H
#define ION_FEC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opaque FEC context structure.
 */
typedef struct ion_fec ion_fec_t;

/*
 * Create an erasure coder context.
 *
 * @param k  Number of data blocks required to reconstruct original data
 * @param m  Total number of blocks (data + parity), must be > k
 * @return   FEC context on success, NULL on failure (memory allocation)
 *
 * The coder can generate (m - k) parity blocks from k data blocks.
 * Original data can be reconstructed from any k of the m total blocks.
 */
ion_fec_t *ion_fec_new(unsigned short k, unsigned short m);

/*
 * Free an erasure coder context.
 *
 * @param fec  FEC context to free (may be NULL)
 */
void ion_fec_free(ion_fec_t *fec);

/*
 * Encode data blocks to generate parity blocks.
 *
 * @param fec              FEC context
 * @param data_blocks      Array of k pointers to data blocks
 * @param parity_blocks    Array of (m-k) pointers to parity block buffers (output)
 * @param parity_block_nums  Array of parity block indices (k to m-1)
 * @param num_parity_blocks  Number of parity blocks to generate (m - k)
 * @param block_size       Size of each block in bytes
 *
 * This generates parity blocks from the k data blocks.
 * The parity_block_nums array specifies which parity blocks to generate
 * (indices from k to m-1).
 */
void ion_fec_encode(const ion_fec_t *fec, const unsigned char *const *data_blocks,
		unsigned char     **parity_blocks,
		const unsigned int *parity_block_nums, size_t num_parity_blocks,
		size_t block_size);

/*
 * Decode/reconstruct missing data blocks.
 *
 * @param fec            FEC context
 * @param input_blocks   Array of k pointers to available blocks (data or parity)
 * @param output_blocks  Array of pointers to output buffers for reconstructed blocks
 * @param block_indices  Array of k block indices indicating which block is in each input slot
 * @param block_size     Size of each block in bytes
 *
 * The input_blocks array must contain exactly k blocks. Each block can be either
 * a data block (index 0 to k-1) or a parity block (index k to m-1).
 * The block_indices array identifies which block is at each position.
 *
 * Missing data blocks (those with indices 0 to k-1 that are not in input_blocks)
 * will be reconstructed and written to output_blocks.
 */
void ion_fec_decode(const ion_fec_t        *fec,
		const unsigned char *const *input_blocks,
		unsigned char             **output_blocks,
		const unsigned int *block_indices, size_t block_size);

#ifdef __cplusplus
}
#endif

#endif /* ION_FEC_H */
