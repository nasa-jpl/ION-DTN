/*
 * ion_fec.c - ION Forward Error Correction wrapper for Jerasure
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

#include "ion_fec.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "jerasure.h"
#include "reed_sol.h"

/*
 * FEC context structure.
 * Holds the encoding matrix and code parameters.
 */
struct ion_fec
{
	unsigned short k;      /* Number of data blocks */
	unsigned short m;      /* Total blocks (data + parity) */
	int           *matrix; /* Encoding matrix (m-k) x k */
};

/*
 * Word size for Galois Field operations.
 * w=8 means GF(2^8), which supports up to 255 total blocks.
 * This matches zfec's behavior.
 */
#define FEC_WORD_SIZE 8

ion_fec_t *ion_fec_new(unsigned short k, unsigned short m)
{
	ion_fec_t *fec;

	if (k == 0 || m <= k || m > 255)
	{
		return NULL;
	}

	fec = (ion_fec_t *) malloc(sizeof(ion_fec_t));
	if (fec == NULL)
	{
		return NULL;
	}

	fec->k = k;
	fec->m = m;

	/*
	 * Create Reed-Solomon Vandermonde coding matrix.
	 * This is an (m-k) x k matrix used for encoding.
	 * For decoding, Jerasure will compute the inverse internally.
	 */
	fec->matrix = reed_sol_vandermonde_coding_matrix(k, m - k, FEC_WORD_SIZE);
	if (fec->matrix == NULL)
	{
		free(fec);
		return NULL;
	}

	return fec;
}

void ion_fec_free(ion_fec_t *fec)
{
	if (fec != NULL)
	{
		if (fec->matrix != NULL)
		{
			free(fec->matrix);
		}
		free(fec);
	}
}

void ion_fec_encode(const ion_fec_t *fec, const unsigned char *const *data_blocks,
		unsigned char	  **parity_blocks,
		const unsigned int *parity_block_nums, size_t num_parity_blocks,
		size_t block_size)
{
	char **data_ptrs;
	int    i;

	(void) parity_block_nums; /* Not used - Jerasure generates all parity */
	(void) num_parity_blocks; /* Should equal (m - k) */

	if (fec == NULL || data_blocks == NULL || parity_blocks == NULL)
	{
		return;
	}

	/*
	 * Jerasure's API takes non-const char** but does not modify data_blocks.
	 * Create a temporary array to avoid const cast warning.
	 */
	data_ptrs = (char **) malloc(fec->k * sizeof(char *));
	if (data_ptrs == NULL)
	{
		return;
	}
	for (i = 0; i < fec->k; i++)
	{
		data_ptrs[i] = (char *) (uintptr_t) data_blocks[i];
	}

	/*
	 * Jerasure encodes all parity blocks at once.
	 * It generates (m-k) parity blocks from k data blocks.
	 */
	jerasure_matrix_encode(fec->k, fec->m - fec->k, FEC_WORD_SIZE,
			fec->matrix, data_ptrs, (char **) parity_blocks,
			(int) block_size);

	free(data_ptrs);
}

void ion_fec_decode(const ion_fec_t	   *fec,
		const unsigned char *const *input_blocks,
		unsigned char		  **output_blocks,
		const unsigned int *block_indices, size_t block_size)
{
	int   *erasures;
	int   *erased;
	char **data_ptrs;
	char **coding_ptrs;
	int    i;
	int    num_erasures;
	int    output_idx;
	int    ret;

	if (fec == NULL || input_blocks == NULL || output_blocks == NULL
			|| block_indices == NULL)
	{
		return;
	}

	/*
	 * Allocate temporary arrays for Jerasure's decode interface.
	 * Jerasure expects separate data_ptrs and coding_ptrs arrays,
	 * plus an erasures array listing which blocks are missing.
	 */
	data_ptrs = (char **) malloc(fec->k * sizeof(char *));
	coding_ptrs = (char **) malloc((fec->m - fec->k) * sizeof(char *));
	erasures = (int *) malloc((fec->m + 1) * sizeof(int));
	erased = (int *) calloc(fec->m, sizeof(int));

	if (data_ptrs == NULL || coding_ptrs == NULL || erasures == NULL
			|| erased == NULL)
	{
		free(data_ptrs);
		free(coding_ptrs);
		free(erasures);
		free(erased);
		return;
	}

	/*
	 * Initialize all pointers to NULL.
	 * We'll fill in the ones we have from input_blocks.
	 */
	for (i = 0; i < fec->k; i++)
	{
		data_ptrs[i] = NULL;
	}
	for (i = 0; i < fec->m - fec->k; i++)
	{
		coding_ptrs[i] = NULL;
	}

	/*
	 * Map input blocks to data_ptrs or coding_ptrs based on their indices.
	 * Indices 0 to k-1 are data blocks, k to m-1 are parity blocks.
	 * Use uintptr_t cast to safely remove const qualifier - Jerasure
	 * does not modify the input data blocks.
	 */
	for (i = 0; i < fec->k; i++)
	{
		unsigned int idx = block_indices[i];
		if (idx < fec->k)
		{
			/* This is a data block */
			data_ptrs[idx] = (char *) (uintptr_t) input_blocks[i];
		}
		else if (idx < fec->m)
		{
			/* This is a parity/coding block */
			coding_ptrs[idx - fec->k] =
			                (char *) (uintptr_t) input_blocks[i];
		}
	}

	/*
	 * Build erasures array: list of missing block indices.
	 * Also allocate output buffers for erased data blocks.
	 */
	num_erasures = 0;
	output_idx = 0;
	for (i = 0; i < fec->k; i++)
	{
		if (data_ptrs[i] == NULL)
		{
			/* Data block i is missing - it's an erasure */
			erasures[num_erasures++] = i;
			erased[i] = 1;
			/* Use output buffer for this missing block */
			data_ptrs[i] = (char *) output_blocks[output_idx++];
		}
	}
	for (i = 0; i < fec->m - fec->k; i++)
	{
		if (coding_ptrs[i] == NULL)
		{
			/* Parity block is missing - mark as erasure but we don't need to recover it */
			erasures[num_erasures++] = fec->k + i;
			erased[fec->k + i] = 1;
			/* Allocate temporary buffer for this parity block */
			coding_ptrs[i] = (char *) malloc(block_size);
		}
	}
	erasures[num_erasures] = -1; /* Terminate erasures array */

	/*
	 * Call Jerasure to decode/reconstruct.
	 * This will fill in the missing data blocks in data_ptrs.
	 */
	ret = jerasure_matrix_decode(fec->k, fec->m - fec->k, FEC_WORD_SIZE,
			fec->matrix, 0, erasures, data_ptrs, coding_ptrs,
			(int) block_size);

	/*
	 * Clean up temporary parity buffers we allocated.
	 */
	for (i = 0; i < fec->m - fec->k; i++)
	{
		if (erased[fec->k + i])
		{
			free(coding_ptrs[i]);
		}
	}

	free(data_ptrs);
	free(coding_ptrs);
	free(erasures);
	free(erased);

	(void) ret; /* Jerasure returns 0 on success, -1 on failure */
}
