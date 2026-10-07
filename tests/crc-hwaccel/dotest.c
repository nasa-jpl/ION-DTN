/*
 * crc_hwaccel_test.c - differential self-test for the CRC routines.
 *
 * Checks the public ion_CRC entry points against an independent,
 * bit-at-a-time reference for both polynomials ION carries on the bundle/CFDP
 * data path:
 *
 *   * CRC-16/X-25  (reflected 0x1021, init/xorout 0xFFFF)
 *   * CRC-32C      (reflected 0x1EDC6F41, init/xorout 0xFFFFFFFF)
 *
 * On capable x86 or AArch64 the entry points dispatch to the hardware fast
 * paths at run time; elsewhere they use the table routines.  Either way the
 * output must equal the reference, which shares no code with the table or the
 * intrinsics, so this is a genuine cross-check, not a tautology.
 *
 * Covered: the canonical "123456789" check values; every length from 0 to 512
 * plus a few large sizes; read offsets 0..15 (exercising the x86 alignment
 * loop); and accumulation across split points.
 *
 * Exit: 0 all match, 1 a mismatch (loud, with the failing case).
 */

/* platform.h must precede the system headers (ION feature-test macros). */
// clang-format off
#include "platform.h"
#include "crc.h"
#include <stdint.h>
#include <stdio.h>
// clang-format on

/* Reflected bit-at-a-time reference, independent of crc.c. */

static uint16_t ref_crc16_x25(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFF; /* init */
	uint32_t i;
	int	 b;

	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (b = 0; b < 8; b++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
		}
	}

	return (uint16_t) (crc ^ 0xFFFF); /* xorout */
}

static uint32_t ref_crc32c(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu; /* init */
	uint32_t i;
	int	 b;

	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (b = 0; b < 8; b++)
		{
			crc = (crc & 1) ? (crc >> 1) ^ 0x82F63B78u : (crc >> 1);
		}
	}

	return crc ^ 0xFFFFFFFFu; /* xorout */
}

static int failures = 0;

static void check16(const uint8_t *d, uint32_t len, const char *what)
{
	uint16_t got = ion_CRC16_1021_X25((const char *) d, len, 0);
	uint16_t exp = ref_crc16_x25(d, len);

	if (got != exp)
	{
		printf("FAIL CRC16 %s len=%u: got 0x%04X expected 0x%04X\n",
				what, len, got, exp);
		failures++;
	}

	/* The slice entry must agree too (data path uses it). */
	got = ion_CRC16_1021_X25_slice((const char *) d, len, 0);
	if (got != exp)
	{
		printf("FAIL CRC16-slice %s len=%u: got 0x%04X expected "
		       "0x%04X\n",
				what, len, got, exp);
		failures++;
	}
}

/*
 * ION declares exactly one CRC-32C entry point per build: the slice-by-16
 * ..._C_slice under ENABLE_HIGH_SPEED, else the byte-at-a-time ..._C.
 * ion_crc32c() routes to whichever exists; both dispatch to the hardware path
 * on capable CPUs at run time.
 */
#ifdef ENABLE_HIGH_SPEED
#define ion_crc32c(d, n, c) ion_CRC32_1EDC6F41_C_slice((d), (n), (c))
#else
#define ion_crc32c(d, n, c) ion_CRC32_1EDC6F41_C((d), (n), (c))
#endif

static uint32_t crc32c(const uint8_t *d, uint32_t len, uint32_t crc)
{
	return ion_crc32c((const char *) d, len, crc);
}

static void check32(const uint8_t *d, uint32_t len, const char *what)
{
	uint32_t got = crc32c(d, len, 0);
	uint32_t exp = ref_crc32c(d, len);

	if (got != exp)
	{
		printf("FAIL CRC32C %s len=%u: got 0x%08X expected 0x%08X\n",
				what, len, got, exp);
		failures++;
	}
}

int main(void)
{
	static uint8_t pool[8192 + 16];
	const uint8_t *nums = (const uint8_t *) "123456789";
	uint32_t       len;
	uint32_t       off;
	uint32_t       split;
	uint32_t       i;
	uint32_t       seed = 0x12345678u;

	/*
	 * Surface which paths engaged, so a CI log shows whether hardware
	 * acceleration was actually taken on this runner.
	 */

	printf("crc accel: %s\n", ion_crc_accel_status());

	/* Canonical check values anchor the reference itself. */

	if (ref_crc16_x25(nums, 9) != 0x906E)
	{
		printf("FAIL reference CRC16/X-25 check value\n");
		return 1;
	}

	if (ref_crc32c(nums, 9) != 0xE3069283u)
	{
		printf("FAIL reference CRC-32C check value\n");
		return 1;
	}

	if (ion_CRC16_1021_X25((const char *) nums, 9, 0) != 0x906E)
	{
		printf("FAIL ion_CRC16_1021_X25('123456789') != 0x906E\n");
		failures++;
	}

	if (crc32c(nums, 9, 0) != 0xE3069283u)
	{
		printf("FAIL ion CRC-32C('123456789') != 0xE3069283\n");
		failures++;
	}

	/* Deterministic pseudo-random fill (no rand() dependency). */

	for (i = 0; i < sizeof pool; i++)
	{
		seed = seed * 1103515245u + 12345u;
		pool[i] = (uint8_t) (seed >> 16);
	}

	/*
	 * Every length 0..512 at every offset 0..15.  The offset walks the x86 8-byte
	 * alignment loop and the first-block load; lengths cross the 16-byte fold and
	 * 8/16-byte unroll boundaries and leave tails of every size.
	 */

	for (off = 0; off < 16; off++)
	{
		for (len = 0; len <= 512; len++)
		{
			check16(pool + off, len, "sweep");
			check32(pool + off, len, "sweep");
		}
	}

	/* A few large buffers. */

	for (len = 1024; len <= 8192; len += 1021)
	{
		check16(pool, len, "large");
		check32(pool, len, "large");
	}

	/*
	 * Accumulation boundaries: CRC(a||b) must equal continuing the CRC of a over
	 * b.  The entry points take ~crc in and return ~crc out, so the prior return
	 * feeds straight in.
	 */

	for (split = 0; split <= 64; split++)
	{
		uint32_t total = 200;
		uint16_t c16;
		uint32_t c32;
		uint16_t e16 = ref_crc16_x25(pool, total);
		uint32_t e32 = ref_crc32c(pool, total);

		c16 = ion_CRC16_1021_X25((const char *) pool, split, 0);
		c16 = ion_CRC16_1021_X25((const char *) pool + split,
				total - split, c16);
		if (c16 != e16)
		{
			printf("FAIL CRC16 accum split=%u: 0x%04X != 0x%04X\n",
					split, c16, e16);
			failures++;
		}

		c32 = crc32c(pool, split, 0);
		c32 = crc32c(pool + split, total - split, c32);
		if (c32 != e32)
		{
			printf("FAIL CRC32C accum split=%u: 0x%08X != "
			       "0x%08X\n",
					split, c32, e32);
			failures++;
		}
	}

	if (failures)
	{
		printf("crc_hwaccel_test: %d FAILURE(S)\n", failures);
		return 1;
	}

	printf("crc_hwaccel_test: all CRC outputs match the reference\n");
	return 0;
}
