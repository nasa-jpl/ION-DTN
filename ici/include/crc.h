/*

	crc.h:	definitions enabling the use of CRC functions
		provided by Antara Teknik, LLC.

	Copyright (c) 2019, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#ifndef _CRC_H_
#define _CRC_H_

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t	ion_CRC16_1021_X25(const char *data, uint32_t dLen,
			uint16_t crc);

extern uint32_t ion_CRC32_04C11DB7_bzip2(const char *data, uint32_t dLen,
			uint32_t crc);

extern uint32_t ion_CRC32_04C11DB7(const char *data, uint32_t dLen,
			uint32_t crc);

extern uint32_t ion_CRC32_1EDC6F41_C(const char *data, uint32_t dLen,
			uint32_t crc);

#ifdef __cplusplus
}
#endif

#endif  /* _CRC_H_ */
