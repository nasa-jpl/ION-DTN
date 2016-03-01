/*
	bulk.c:	dummy implementation of "bulk" data I/O functions.
		To be supplanted by mission-specific bulk data access
		functions in flight deployments.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2015, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "platform.h"
#include "ion.h"
#include "bulk.h"

#define	BULK_MAX_FILE_NAME_LEN	(32)

static void	getFileName(unsigned long item, char *fileNameBuffer)
{
	isprintf(fileNameBuffer, MAXPATHLEN, "%s%cionbulk.%lu",
			getIonWorkingDirectory(), ION_PATH_DELIMITER, item);
}

int	bulk_create(unsigned long item)
{
	char	fileName[MAXPATHLEN];
	int	fd;

	getFileName(item, fileName);
	fd = iopen(fileName, O_RDWR | O_CREAT, 0666);
	if (fd < 0)
	{
		putSysErrmsg("bulk_create failed on open.",  fileName);
		return -1;
	}

	close(fd);
	return fd;
}

int	bulk_write(unsigned long item, vast offset, char *buffer, vast length)
{
	char	fileName[MAXPATHLEN];
	int	fd;
	int	result;

	getFileName(item, fileName);
	fd = iopen(fileName, O_WRONLY, 0);
	if (fd < 0)
	{
		putSysErrmsg("bulk_write failed on open.",  fileName);
		return -1;
	}

	if (offset > 0)
	{
		if (lseek(fd, offset, SEEK_SET) == (off_t) -1)
		{
			putSysErrmsg("bulk_write failed on lseek.",  fileName);
			close(fd);
			return -1;
		}
	}

	result = write(fd, buffer, length);
	close(fd);
	if (result < 0)
	{
		putSysErrmsg("bulk_write failed on write.",  fileName);
		return -1;
	}

	return result;
}

int	bulk_read(unsigned long item, char *buffer, vast offset, vast length)
{
	char	fileName[MAXPATHLEN];
	int	fd;
	int	result;

	getFileName(item, fileName);
	fd = iopen(fileName, O_RDONLY, 0);
	if (fd < 0)
	{
		putSysErrmsg("bulk_read failed on open.",  fileName);
		return -1;
	}

	if (offset > 0)
	{
		if (lseek(fd, offset, SEEK_SET) == (off_t) -1)
		{
			putSysErrmsg("bulk_read failed on lseek.",  fileName);
			close(fd);
			return -1;
		}
	}

	result = read(fd, buffer, length);
	close(fd);
	if (result < length)
	{
		putSysErrmsg("bulk_read failed on read.",  fileName);
		return -1;
	}

	return result;
}

void	bulk_destroy(unsigned long item)
{
	char	fileName[MAXPATHLEN];

	getFileName(item, fileName);
	unlink(fileName);
}
