/*
 *	sdrstring.c:	simple data recorder string management
 *			library.
 *
 *	Copyright (c) 2001-2007, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 *
 *	This library implements the Simple Data Recorder system's
 *	self-delimiting strings.
 *
 *	Modification History:
 *	Date	  Who	What
 *	4-3-96	  APS	Abstracted IPC services and task control.
 *	5-1-96	  APS	Ported to sparc-sunos4.
 *	12-20-00  SCB	Revised for sparc-sunos5.
 *	6-8-07    SCB	Divided sdr.c library into separable components.
 */

#include "sdrP.h"
#include "sdrstring.h"

/*		Private definition of SDR string structure.		*/

typedef unsigned char	SdrStringBuffer[SDRSTRING_BUFSZ];
			/*	SdrString is a low-overhead string
				representation specially designed for
				efficient storage of small strings in
				the SDR.  To avoid time-consuming SDR
				I/O to find the end of a string when
				retrieving it from the SDR, we do not
				NULL-terminate SdrStrings.  Instead
				the first character contains the length
				of the string, which cannot exceed 255.	*/

/*	*	*	String management functions	*	*	*/

Object	Sdr_string_create(const char *file, int line, Sdr sdrv, char *from)
{
	size_t		length = 0;
	Object		string;
	SdrStringBuffer stringBuf;

	if (!sdr_in_xn(sdrv))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (from == NULL || (length = strlen(from)) > 255)
	{
		oK(_xniEnd(file, line, from, sdrv));
		return 0;
	}

	string = _sdrmalloc(sdrv, length + 1);
	if (string == 0)
	{
		oK(_iEnd(file, line, from));
		return 0;
	}

	stringBuf[0] = length;
	memcpy(stringBuf + 1, from, length);
	_sdrput(file, line, sdrv, (Address) string, (char *) stringBuf,
			length + 1, SystemPut);
	return string;
}

Object	Sdr_string_dup(const char *file, int line, Sdr sdrv, Object from)
{
	Address		addr = (Address) from;
	unsigned char	length;
	SdrStringBuffer stringBuf;
	Object		string;

	if (!sdr_in_xn(sdrv))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	if (from == 0)
	{
		oK(_xniEnd(file, line, "from", sdrv));
		return 0;
	}

	sdrFetch(length, addr);
	_sdrfetch(sdrv, (char *) (stringBuf + 1), addr + 1, length);
	string = _sdrmalloc(sdrv, length + 1);
	if (string == 0)
	{
		oK(_iEnd(file, line, (char *) (stringBuf + 1)));
		return 0;
	}

	stringBuf[0] = length;
	_sdrput(file, line, sdrv, (Address) string, (char *) stringBuf,
			length + 1, SystemPut);
	return string;
}

int	sdr_string_length(Sdr sdrv, Object string)
{
	unsigned char	length;

	CHKERR(string);
	sdrFetch(length, (Address) string);
	return length;
}

int	sdr_string_read(Sdr sdrv, char *into, Object string)
{
	Address		addr = (Address) string;
	unsigned char	length;

	CHKERR(sdrFetchSafe(sdrv));
	CHKERR(into);
	CHKERR(string);
	sdrFetch(length, addr);
	_sdrfetch(sdrv, into, addr + 1, length);
	*(into + length) = '\0';
	return length;
}
