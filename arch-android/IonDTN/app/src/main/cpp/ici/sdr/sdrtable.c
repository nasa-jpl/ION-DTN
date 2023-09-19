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

/*		Private definition of SDR table structure.		*/

typedef struct
{
	Address		userData;
	int		rowSize;
	int		rowCount;
	Object		rows;	/*	an array			*/
} SdrTable;

/*	*	*	Table management functions	*	*	*/

Object	Sdr_table_create(const char *file, int line, Sdr sdrv, int rowSize,
		int rowCount)
{
	SdrTable	tableBuffer;
	Object		table;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (rowSize < 1 || rowCount < 1)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
		return 0;
	}

	tableBuffer.userData = 0;
	tableBuffer.rowSize = rowSize;
	tableBuffer.rowCount = rowCount;
	tableBuffer.rows = _sdrmalloc(sdrv, rowSize * rowCount);
	if (tableBuffer.rows == 0)
	{
		oK(_iEnd(file, line, "tableBuffer.rows"));
		return 0;
	}

	table = _sdrzalloc(sdrv, sizeof tableBuffer);
	if (table == 0)
	{
		oK(_iEnd(file, line, "table"));
		return 0;
	}

	sdrPut((Address) table, tableBuffer);
	return table;
}

Address	sdr_table_user_data(Sdr sdrv, Object table)
{
	SdrTable	tableBuffer;

	CHKZERO(sdrFetchSafe(sdrv));
	CHKZERO(table);
	sdrFetch(tableBuffer, (Address) table);
	return tableBuffer.userData;
}

void	Sdr_table_user_data_set(const char *file, int line, Sdr sdrv,
		Object table, Address data)
{
	SdrTable	tableBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (table == 0)
	{
		oK(_xniEnd(file, line, "table", sdrv));
		return;
	}

	sdrFetch(tableBuffer, (Address) table);
	tableBuffer.userData = data;
	sdrPut((Address) table, tableBuffer);
}

void	sdr_table_dimensions(Sdr sdrv, Object table, int *rowSize,
		int *rowCount)
{
	SdrTable	tableBuffer;

	CHKVOID(table);
	CHKVOID(rowSize);
	CHKVOID(rowCount);
	sdrFetch(tableBuffer, (Address) table);
	*rowSize = tableBuffer.rowSize;
	*rowCount = tableBuffer.rowCount;
}

void	sdr_table_stage(Sdr sdrv, Object table)
{
	SdrTable	tableBuffer;

	CHKVOID(table);
	sdrFetch(tableBuffer, (Address) table);
	sdr_stage(sdrv, NULL, tableBuffer.rows, 0);
}

Address	sdr_table_row(Sdr sdrv, Object table, unsigned int rowNbr)
{
	SdrTable	tableBuffer;
	Address		addr;

	CHKERR(table);
	sdrFetch(tableBuffer, (Address) table);
	CHKERR(rowNbr < tableBuffer.rowCount);
	addr = ((Address) (tableBuffer.rows)) + (rowNbr * tableBuffer.rowSize);
	return addr;
}

void	Sdr_table_destroy(const char *file, int line, Sdr sdrv, Object table)
{
	SdrTable	tableBuffer;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return;
	}

	joinTrace(sdrv, file, line);
	if (table == 0)
	{
		oK(_xniEnd(file, line, _apiErrMsg(), sdrv));
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
