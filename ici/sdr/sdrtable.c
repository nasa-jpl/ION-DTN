/*
 *	sdrtable.c:	simple data recorder table management
 *			library.
 *
 *	Copyright (c) 2001-2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	This library implements the Simple Data Recorder system's
 *	self-delimiting tables.
 *
 *	Modification History:
 *	Date	  Who	What
 *	4-3-96	  APS	Abstracted IPC services and task control.
 *	5-1-96	  APS	Ported to sparc-sunos4.
 *	12-20-00  SCB	Revised for sparc-sunos5.
 *	6-8-07 	  SCB	Divided sdr.c library into separable components.
 */

#include "sdrP.h"
#include "sdrtable.h"

static const char	*badAddressMsg = "can't access item at address zero";

/*		Private definition of SDR table structure.		*/

typedef struct
{
	Address		userData;
	int		rowSize;
	int		rowCount;
	Object		rows;	/*	an array			*/
} SdrTable;

/*	*	*	Table management functions	*	*	*/

Object	Sdr_table_create(char *file, int line, Sdr sdrv, int rowSize,
		int rowCount)
{
	SdrTable	tableBuffer;
	Object		table;

	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (rowSize < 1 || rowCount < 1)
	{
		errno = EINVAL;
		_putErrmsg(file, line, apiErrMsg, NULL);
		crashXn(sdrv);
		return 0;
	}

	tableBuffer.userData = 0;
	tableBuffer.rowSize = rowSize;
	tableBuffer.rowCount = rowCount;
	tableBuffer.rows = _sdrmalloc(sdrv, rowSize * rowCount);
	if (tableBuffer.rows == 0)
	{
		_putErrmsg(file, line, "can't create table rows", NULL);
		return 0;
	}

	table = _sdrzalloc(sdrv, sizeof tableBuffer);
	if (table == 0)
	{
		_putErrmsg(file, line, "can't create table object", NULL);
		return 0;
	}

	sdrPut((Address) table, tableBuffer);
	return table;
}

Address	sdr_table_user_data(Sdr sdrv, Object table)
{
	SdrState	*sdr;
	SdrTable	tableBuffer;

	if (table == 0)
	{
		errno = EINVAL;
		putErrmsg(badAddressMsg, "table");
		crashXn(sdrv);
		return 0;
	}

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	sdrFetch(tableBuffer, (Address) table);
	releaseSdr(sdr);
	return tableBuffer.userData;
}

void	Sdr_table_user_data_set(char *file, int line, Sdr sdrv, Object table,
		Address data)
{
	SdrTable	tableBuffer;

	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
		return;
	}

	joinTrace(sdrv, file, line);
	if (table == 0)
	{
		errno = EINVAL;
		_putErrmsg(file, line, badAddressMsg, "table");
		crashXn(sdrv);
		return;
	}

	sdrFetch(tableBuffer, (Address) table);
	tableBuffer.userData = data;
	sdrPut((Address) table, tableBuffer);
}

void	sdr_table_dimensions(Sdr sdrv, Object table, int *rowSize,
		int *rowCount)
{
	SdrState	*sdr;
	SdrTable	tableBuffer;

	if (table == 0 || rowSize == NULL || rowCount == NULL)
	{
		errno = EINVAL;
		putErrmsg(apiErrMsg, NULL);
		crashXn(sdrv);
		return;
	}

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	sdrFetch(tableBuffer, (Address) table);
	releaseSdr(sdr);
	*rowSize = tableBuffer.rowSize;
	*rowCount = tableBuffer.rowCount;
}

void	sdr_table_stage(Sdr sdrv, Object table)
{
	SdrState	*sdr = sdrv->sdr;
	SdrTable	tableBuffer;

	if (table == 0)
	{
		errno = EINVAL;
		putErrmsg(badAddressMsg, "table");
		crashXn(sdrv);
		return;
	}

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	sdrFetch(tableBuffer, (Address) table);
	sdr_stage(sdrv, NULL, tableBuffer.rows, 0);
	releaseSdr(sdr);
}

Address	sdr_table_row(Sdr sdrv, Object table, unsigned int rowNbr)
{
	SdrState	*sdr;
	SdrTable	tableBuffer;
	Address		addr;

	if (table == 0)
	{
		errno = EINVAL;
		putErrmsg(badAddressMsg, "table");
		crashXn(sdrv);
		return -1;
	}

	REQUIRE(sdrv);
	sdr = sdrv->sdr;
	takeSdr(sdr);
	sdrFetch(tableBuffer, (Address) table);
	releaseSdr(sdr);
	if (rowNbr >= tableBuffer.rowCount)
	{
		errno = EINVAL;
		putErrmsg("Invalid table row number.", utoa(rowNbr));
		crashXn(sdrv);
		return -1;
	}

	addr = ((Address) (tableBuffer.rows)) + (rowNbr * tableBuffer.rowSize);
	return addr;
}

void	Sdr_table_destroy(char *file, int line, Sdr sdrv, Object table)
{
	SdrTable	tableBuffer;

	if (sdr_in_xn(sdrv) == 0)
	{
		_putErrmsg(file, line, notInXnMsg, NULL);
		return;
	}

	joinTrace(sdrv, file, line);
	if (table == 0)
	{
		errno = EINVAL;
		_putErrmsg(file, line, badAddressMsg, "table");
		crashXn(sdrv);
		return;
	}

	sdrFetch(tableBuffer, (Address) table);
	sdrFree(tableBuffer.rows);

	/* just in case user mistakenly accesses later... */
	tableBuffer.rowSize = 0;
	tableBuffer.rowCount = 0;
	tableBuffer.rows = 0;
	sdrPut((Address) table, tableBuffer);
	sdrFree(table);
}
