/*

	crc.h:	definitions enabling the use of CRC functions
		provided by Antara Teknik, LLC.

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t ion_CRC16_1021_X25(const char *data, uint32_t dLen,
			uint16_t crc);
#ifdef ENABLE_HIGH_SPEED
extern uint16_t ion_CRC16_1021_X25_slice(const char *data, uint32_t dLen,
			uint16_t crc);

extern uint32_t ion_CRC32_1EDC6F41_C_slice(const char *data, uint32_t dLen,
			uint32_t crc);

extern uint32_t ion_CRC32_04C11DB7_slice(const char *data, uint32_t dLen,
			uint32_t crc);
#else
#if (BZIP_2_NEEDED)
extern uint32_t ion_CRC32_04C11DB7_bzip2(const char *data, uint32_t dLen,
			uint32_t crc);
#endif
extern uint32_t ion_CRC32_04C11DB7(const char *data, uint32_t dLen,
			uint32_t crc);

extern uint32_t ion_CRC32_1EDC6F41_C(const char *data, uint32_t dLen,
			uint32_t crc);
#endif
#ifdef __cplusplus
}
#endif

#endif /* CRC_H */
