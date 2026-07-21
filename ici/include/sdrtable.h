/*

	sdrtable.h:	definitions supporting use of SDR self-
			delimited tables.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who	What
	06-05-07  SCB	Initial abstraction from original SDR API.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/

#ifndef SDRTABLE_H
#define SDRTABLE_H

#include "sdrmgt.h"

#ifdef __cplusplus
extern "C" {
#endif

/*	Functions for operating on self-delimited tables in SDR.	*/

#define sdr_table_create(sdr, rowSize, rowCount) \
Sdr_table_create(__FILE__, __LINE__, sdr, rowSize, rowCount)
extern SdrObject Sdr_table_create(const char *file, int line, Sdr sdr,
		int rowSize, int rowCount);

extern SdrAddress sdr_table_user_data(Sdr sdr, SdrObject table);

#define sdr_table_user_data_set(sdr, table, userData) \
Sdr_table_user_data_set(__FILE__, __LINE__, sdr, table, userData)
extern void Sdr_table_user_data_set(const char *file, int line, Sdr sdr,
		SdrObject table, SdrAddress userData);

extern void sdr_table_dimensions(Sdr sdr, SdrObject table, int *rowSize,
		int *rowCount);

extern void sdr_table_stage(Sdr sdr, SdrObject table);

extern SdrAddress sdr_table_row(Sdr sdr, SdrObject table, unsigned int rowNbr);

#define sdr_table_destroy(sdr, table) \
Sdr_table_destroy(__FILE__, __LINE__, sdr, table)
extern void Sdr_table_destroy(const char *file, int line, Sdr sdr,
		SdrObject table);
#ifdef __cplusplus
}
#endif

#endif /* SDRTABLE_H */
