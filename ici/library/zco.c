/*

	zco.c:		API for using zero-copy objects to implement
			deeply stacked communication protocols.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "platform.h"
#include "zco.h"

typedef struct
{
	vast		heapOccupancy;
	vast		maxHeapOccupancy;
	vast		fileOccupancy;
	vast		maxFileOccupancy;
} ZcoDB;

typedef struct
{
	Object		text;		/*	header or trailer	*/
	vast		length;
	Object		prevCapsule;
	Object		nextCapsule;
} Capsule;

typedef struct
{
	int		refCount;
	short		okayToDestroy;
	short		unlinkOnDestroy;
	unsigned long	inode;		/*	to detect change	*/
	unsigned long	fileLength;
	unsigned long	xmitProgress;
	char		pathName[256];
	char		cleanupScript[256];
} FileRef;

typedef struct
{
	int		refCount;
	vast		objLength;
	Object		location;
} SdrRef;

typedef struct
{
	ZcoMedium	sourceMedium;
	Object		location;	/*	of FileRef or SdrRef	*/
	vast		offset;		/*	within file or object	*/
	vast		length;
	Object		nextExtent;
} SourceExtent;

typedef struct
{
	Object		firstHeader;		/*	Capsule		*/
	Object		lastHeader;		/*	Capsule		*/

	/*	Note that prepending headers and appending trailers
	 *	increases the lengths of the linked list for headers
	 *	and trailers but DOES NOT affect the headersLength
	 *	and trailersLength fields.  These fields indicate only
	 *	how much of the concatenated content of all extents
	 *	in the linked list of extents is currently believed
	 *	to constitute ADDITIONAL opaque header and trailer
	 *	information, just as sourceLength indicates how much
	 *	of the concatenated content of all extents is believed
	 *	to constitute source data.  The total length of the
	 *	ZCO is the sum of the lengths of the extents (some of
	 *	which sum is source data and some of which may be
	 *	opaque header and trailer information) plus the sum
	 *	of the lengths of all explicitly attached headers and
	 *	trailers.						*/

	Object		firstExtent;		/*	SourceExtent	*/
	Object		lastExtent;		/*	SourceExtent	*/
	vast		headersLength;		/*	within extents	*/
	vast		sourceLength;		/*	within extents	*/
	vast		trailersLength;		/*	within extents	*/

	Object		firstTrailer;		/*	Capsule		*/
	Object		lastTrailer;		/*	Capsule		*/

	vast		aggregateCapsuleLength;
	vast		totalLength;		/*	incl. capsules	*/
} Zco;

static char	*_badArgsMemo()
{
	return "Missing/invalid argument(s).";
}

static Object	getZcoDB(Sdr sdr)
{
	static Object	obj = 0;
	char		*dbName = "zcodb";
	int		objType;
	ZcoDB		db;

	if (obj == 0)		/*	Not located yet.		*/
	{
		obj = sdr_find(sdr, dbName, &objType);
		if (obj == 0)	/*	Doesn't exist yet.		*/
		{
			obj = sdr_malloc(sdr, sizeof(ZcoDB));
			if (obj)	/*	Must initialize.	*/
			{
				db.heapOccupancy = 0;
				db.maxHeapOccupancy = LONG_MAX;
				db.fileOccupancy = 0;
				db.maxFileOccupancy = LONG_MAX;
				sdr_write(sdr, obj, (char*) &db, sizeof(ZcoDB));
				sdr_catlg(sdr, dbName, 0, obj);
			}
		}
	}

	return obj;
}

static void	_zcoCallback(ZcoCallback *newCallback)
{
	static ZcoCallback	notify = NULL;

	if (newCallback)
	{
		notify = *newCallback;
	}
	else
	{
		if (notify != NULL)
		{
			notify();
		}
	}
}

void	zco_register_callback(ZcoCallback notify)
{
	_zcoCallback(&notify);
}

void	zco_unregister_callback()
{
	ZcoCallback	notify = NULL;

	_zcoCallback(&notify);
}

void	zco_increase_heap_occupancy(Sdr sdr, vast delta)
{
	Object	obj;
	ZcoDB	db;

	CHKVOID(sdr);
	CHKVOID(delta >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.heapOccupancy += delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

void	zco_reduce_heap_occupancy(Sdr sdr, vast delta)
{
	Object	obj;
	ZcoDB	db;

	CHKVOID(sdr);
	CHKVOID(delta >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.heapOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

vast	zco_get_heap_occupancy(Sdr sdr)
{
	Object	obj;
	ZcoDB	db;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
		return db.heapOccupancy;
	}
	else
	{
		return 0;
	}
}

void	zco_set_max_heap_occupancy(Sdr sdr, vast limit)
{
	Object	obj;
	ZcoDB	db;

	CHKVOID(sdr);
	CHKVOID(limit >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.maxHeapOccupancy = limit;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

vast	zco_get_max_heap_occupancy(Sdr sdr)
{
	Object	obj;
	ZcoDB	db;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
		return db.maxHeapOccupancy;
	}
	else
	{
		return 0;
	}
}

int	zco_enough_heap_space(Sdr sdr, vast length)
{
	Object	obj;
	ZcoDB	db;
	vast	increment;

	CHKZERO(sdr);
	CHKZERO(length >= 0);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
	increment = db.heapOccupancy + length;
	if (increment < 0)		/*	Overflow.		*/
	{
		return 0;
	}

	return (db.maxHeapOccupancy - increment) > 0;
}

int	zco_enough_file_space(Sdr sdr, vast length)
{
	Object	obj;
	ZcoDB	db;
	vast	increment;

	CHKZERO(sdr);
	CHKZERO(length >= 0);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
	increment = db.fileOccupancy + length;
	if (increment < 0)		/*	Overflow.		*/
	{
		return 0;
	}

	return (db.maxFileOccupancy - increment) > 0;
}

static void	zco_increase_file_occupancy(Sdr sdr, vast delta)
{
	Object	obj;
	ZcoDB	db;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.fileOccupancy += delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

static void	zco_reduce_file_occupancy(Sdr sdr, vast delta)
{
	Object	obj;
	ZcoDB	db;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.fileOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

vast	zco_get_file_occupancy(Sdr sdr)
{
	Object	obj;
	ZcoDB	db;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
		return db.fileOccupancy;
	}
	else
	{
		return 0;
	}
}

void	zco_set_max_file_occupancy(Sdr sdr, vast limit)
{
	Object	obj;
	ZcoDB	db;

	CHKVOID(sdr);
	CHKVOID(limit >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		db.maxFileOccupancy = limit;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

vast	zco_get_max_file_occupancy(Sdr sdr)
{
	Object	obj;
	ZcoDB	db;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
		return db.maxFileOccupancy;
	}
	else
	{
		return 0;
	}
}

Object	zco_create_file_ref(Sdr sdr, char *pathName, char *cleanupScript)
{
	int		pathLen;
	char		pathBuf[256];
	int		cwdLen;
	int		scriptLen = 0;
	int		sourceFd;
	struct stat	statbuf;
	Object		fileRefObj;
	FileRef		fileRef;

	CHKZERO(sdr);
	CHKZERO(pathName);
	pathLen = strlen(pathName);
	if (*pathName != ION_PATH_DELIMITER)
	{
		/*	Might not be an absolute path name.		*/

		if (igetcwd(pathBuf, sizeof pathBuf) == NULL)
		{
			putErrmsg("Can't get cwd.", NULL);
			return 0;
		}

		if (pathBuf[0] == ION_PATH_DELIMITER)
		{
			/*	Path names *do* start with the path
			 *	delimiter, so it's a POSIX file system,
			 *	so pathName is *not* an absolute
			 *	path name, so the absolute path name
			 *	must instead be computed by appending
			 *	the relative path name to the name of
			 *	the current working directory.		*/

			cwdLen = strlen(pathBuf);
			if ((cwdLen + 1 + pathLen + 1) > sizeof pathBuf)
			{
				putErrmsg("Absolute path name too long.",
						pathName);
				return 0;
			}

			pathBuf[cwdLen] = ION_PATH_DELIMITER;
			cwdLen++;	/*	cwdname incl. delimiter	*/
			istrcpy(pathBuf + cwdLen, pathName,
					sizeof pathBuf - cwdLen);
			pathName = pathBuf;
			pathLen += cwdLen;
		}
	}

	if (cleanupScript)
	{
		scriptLen = strlen(cleanupScript);
	}

	if (scriptLen > 255 || pathLen < 1 || pathLen > 255)
	{
		putErrmsg(_badArgsMemo(), NULL);
		return 0;
	}

	sourceFd = iopen(pathName, O_RDONLY, 0);
	if (sourceFd == -1)
	{
		putSysErrmsg("Can't open source file", pathName);
		return 0;
	}

	if (fstat(sourceFd, &statbuf) < 0)
	{
		putSysErrmsg("Can't stat source file", pathName);
		return 0;
	}

	/*	Parameters verified.  Proceed with FileRef creation.	*/

	close(sourceFd);
	memset((char *) &fileRef, 0, sizeof(FileRef));
	fileRef.refCount = 0;
	fileRef.okayToDestroy = 0;
	fileRef.unlinkOnDestroy = 0;
	fileRef.inode = statbuf.st_ino;
	fileRef.fileLength = statbuf.st_size;
	fileRef.xmitProgress = 0;
	memcpy(fileRef.pathName, pathName, pathLen);
	fileRef.pathName[pathLen] = '\0';
	if (cleanupScript)
	{
		if (scriptLen == 0)
		{
			fileRef.unlinkOnDestroy = 1;
		}
		else
		{
			memcpy(fileRef.cleanupScript, cleanupScript, scriptLen);
		}
	}

	fileRef.cleanupScript[scriptLen] = '\0';
	fileRefObj = sdr_malloc(sdr, sizeof(FileRef));
	if (fileRefObj == 0)
	{
		putErrmsg("No space for file reference.", NULL);
		return 0;
	}

	zco_increase_heap_occupancy(sdr, sizeof(FileRef));
	sdr_write(sdr, fileRefObj, (char *) &fileRef, sizeof(FileRef));
	return fileRefObj;
}

int	zco_revise_file_ref(Sdr sdr, Object fileRefObj, char *pathName,
		char *cleanupScript)
{
	int	pathLen;
	char	pathBuf[256];
	int	cwdLen;
	int	scriptLen = 0;
	int	sourceFd;
	struct stat	statbuf;
	FileRef	fileRef;

	CHKERR(sdr);
	CHKERR(fileRefObj);
	CHKERR(pathName);
	CHKERR(sdr_in_xn(sdr));
	pathLen = strlen(pathName);
	if (*pathName != ION_PATH_DELIMITER)
	{
		/*	Might not be an absolute path name.		*/

		if (igetcwd(pathBuf, sizeof pathBuf) == NULL)
		{
			putErrmsg("Can't get cwd.", NULL);
			return -1;
		}

		if (pathBuf[0] == ION_PATH_DELIMITER)
		{
			/*	Path names *do* start with the path
			 *	delimiter, so it's a POSIX file system,
			 *	so pathName is *not* an absolute
			 *	path name, so the absolute path name
			 *	must instead be computed by appending
			 *	the relative path name to the name of
			 *	the current working directory.		*/

			cwdLen = strlen(pathBuf);
			if ((cwdLen + 1 + pathLen + 1) > sizeof pathBuf)
			{
				putErrmsg("Absolute path name too long.",
						pathName);
				return -1;
			}

			pathBuf[cwdLen] = ION_PATH_DELIMITER;
			cwdLen++;	/*	cwdname incl. delimiter	*/
			istrcpy(pathBuf + cwdLen, pathName,
					sizeof pathBuf - cwdLen);
			pathName = pathBuf;
			pathLen += cwdLen;
		}
	}

	if (cleanupScript)
	{
		scriptLen = strlen(cleanupScript);
	}

	if (scriptLen > 255 || pathLen < 1 || pathLen > 255)
	{
		putErrmsg(_badArgsMemo(), NULL);
		return -1;
	}

	sourceFd = iopen(pathName, O_RDONLY, 0);
	if (sourceFd == -1)
	{
		putSysErrmsg("Can't open source file", pathName);
		return -1;
	}

	if (fstat(sourceFd, &statbuf) < 0)
	{
		putSysErrmsg("Can't stat source file", pathName);
		return 0;
	}

	/*	Parameters verified.  Proceed with FileRef revision.	*/

	close(sourceFd);
	sdr_stage(sdr, (char *) &fileRef, fileRefObj, sizeof(FileRef));
	fileRef.inode = statbuf.st_ino;
	memcpy(fileRef.pathName, pathName, pathLen);
	fileRef.pathName[pathLen] = '\0';
	if (cleanupScript)
	{
		if (scriptLen == 0)
		{
			fileRef.unlinkOnDestroy = 1;
		}
		else
		{
			fileRef.unlinkOnDestroy = 0;
			memcpy(fileRef.cleanupScript, cleanupScript, scriptLen);
		}
	}
	else
	{
		fileRef.unlinkOnDestroy = 0;
	}

	fileRef.cleanupScript[scriptLen] = '\0';
	sdr_write(sdr, fileRefObj, (char *) &fileRef, sizeof(FileRef));
	return 0;
}

char	*zco_file_ref_path(Sdr sdr, Object fileRefObj, char *buffer, int buflen)
{
	OBJ_POINTER(FileRef, fileRef);

	CHKNULL(sdr);
	CHKNULL(fileRefObj);
	CHKNULL(buffer);
	CHKNULL(buflen > 0);
	GET_OBJ_POINTER(sdr, FileRef, fileRef, fileRefObj);
	return istrcpy(buffer, fileRef->pathName, buflen);
}

int	zco_file_ref_xmit_eof(Sdr sdr, Object fileRefObj)
{
	OBJ_POINTER(FileRef, fileRef);

	CHKZERO(sdr);
	CHKZERO(fileRefObj);
	GET_OBJ_POINTER(sdr, FileRef, fileRef, fileRefObj);
	return (fileRef->xmitProgress == fileRef->fileLength);
}

static void	destroyFileReference(Sdr sdr, FileRef *fileRef,
			Object fileRefObj)
{
	/*	Destroy the file reference.  Invoke file cleanup
	 *	script if provided.					*/

	sdr_free(sdr, fileRefObj);
	zco_reduce_heap_occupancy(sdr, sizeof(FileRef));
	if (fileRef->unlinkOnDestroy)
	{
		oK(unlink(fileRef->pathName));
	}
	else
	{
		if (fileRef->cleanupScript[0] != '\0')
		{
			if (pseudoshell(fileRef->cleanupScript) < 0)
			{
				writeMemoNote("[?] Can't run file reference's \
cleanup script", fileRef->cleanupScript);
			}
		}
	}

	_zcoCallback(NULL);
}

void	zco_destroy_file_ref(Sdr sdr, Object fileRefObj)
{
	FileRef	fileRef;

	CHKVOID(sdr);
	CHKVOID(fileRefObj);
	sdr_stage(sdr, (char *) &fileRef, fileRefObj, sizeof(FileRef));
	if (fileRef.refCount == 0)
	{
		destroyFileReference(sdr, &fileRef, fileRefObj);
		return;
	}

	fileRef.okayToDestroy = 1;
	sdr_write(sdr, fileRefObj, (char *) &fileRef, sizeof(FileRef));
}

static int	extentTooLarge(Sdr sdr, ZcoMedium source, vast length)
{
	vast	heapNeeded;
	Object	obj;
	ZcoDB	db;

	if (source == ZcoZcoSource)
	{
		/*	We don't regulate the cloning of existing
		 *	ZCOs, only the creation of new ZCO extents
		 *	in heap or file space.				*/

		return 0;
	}

	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 1;
	}

	heapNeeded = sizeof(SourceExtent);
	sdr_read(sdr, (char *) &db, obj, sizeof(ZcoDB));
	if (source == ZcoFileSource)
	{
		if (db.fileOccupancy + length > db.maxFileOccupancy)
		{
			return 1;
		}
	}
	else
	{
		heapNeeded += length;
	}

	if (db.heapOccupancy + heapNeeded > db.maxHeapOccupancy)
	{
		return 1;
	}

	return 0;
}

static int	appendExtent(Sdr sdr, Object zco, ZcoMedium sourceMedium,
			int cloning, Object location, vast offset,
			vast length)
{
	Object		extentObj;
	vast		increment;
	Zco		sourceZco;
	Object		obj;
	SourceExtent	extent;
	vast		cumulativeOffset;
	Zco		zcoBuf;
	FileRef		fileRef;
	Object		sdrRefObj;
	SdrRef		sdrRef;
	SourceExtent	prevExtent;

	extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
	if (extentObj == 0)
	{
		putErrmsg("No space for extent.", NULL);
		return -1;
	}

	zco_increase_heap_occupancy(sdr, sizeof(SourceExtent));
	increment = length;

	/*	Adjust parameters if extent clone is requested.		*/

	if (sourceMedium == ZcoZcoSource)
	{
		/*	The new extent is to be a clone of some extent
			of the ZCO at "location".			*/

		sdr_read(sdr, (char *) &sourceZco, location, sizeof(Zco));
		cumulativeOffset = 0;
		extent.length = 0;
		for (obj = sourceZco.firstExtent; obj; obj = extent.nextExtent)
		{
			sdr_read(sdr, (char *) &extent, obj,
					sizeof(SourceExtent));
			if (cumulativeOffset < offset)
			{
				cumulativeOffset += extent.length;
				continue;
			}

			break;
		}

		/*	Offset and length must match exactly.		*/

		if (cumulativeOffset != offset || extent.length != length)
		{
			putErrmsg("No extent to clone.", NULL);
			return -1;
		}

		/*	Found existing extent to clone.			*/

		cloning = 1;
		sourceMedium = extent.sourceMedium;
		location = extent.location;
		offset = extent.offset;
	}

	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	extent.sourceMedium = sourceMedium;
	if (sourceMedium == ZcoFileSource)
	{
		/*	FileRef object already exists, so its size
		 *	is already counted in ZCO *heap* occupancy.	*/

		sdr_stage(sdr, (char *) &fileRef, location, sizeof(FileRef));
		fileRef.refCount++;
		sdr_write(sdr, location, (char *) &fileRef, sizeof(FileRef));
		extent.location = location;
		zco_increase_file_occupancy(sdr, increment);
	}
	else if (cloning)
	{
		sdr_stage(sdr, (char *) &sdrRef, location, sizeof(SdrRef));
		sdrRef.refCount++;
		sdr_write(sdr, location, (char *) &sdrRef, sizeof(SdrRef));
		extent.location = location;
		zco_increase_heap_occupancy(sdr, increment);
	}
	else	/*	Initial reference to some object in SDR heap.	*/
	{
		sdrRefObj = sdr_malloc(sdr, sizeof(SdrRef));
		if (sdrRefObj == 0)
		{
			putErrmsg("No space for SDR reference.", NULL);
			return -1;
		}

		increment += sizeof(SdrRef);
		sdrRef.objLength = length;
		sdrRef.location = location;
		sdrRef.refCount = 1;
		sdr_write(sdr, sdrRefObj, (char *) &sdrRef, sizeof(SdrRef));
		extent.location = sdrRefObj;
		zco_increase_heap_occupancy(sdr, increment);
	}

	extent.offset = offset;
	extent.length = length;
	extent.nextExtent = 0;
	sdr_write(sdr, extentObj, (char *) &extent, sizeof(SourceExtent));
	if (zcoBuf.firstExtent == 0)
	{
		zcoBuf.firstExtent = extentObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &prevExtent, zcoBuf.lastExtent,
				sizeof(SourceExtent));
		prevExtent.nextExtent = extentObj;
		sdr_write(sdr, zcoBuf.lastExtent, (char *) &prevExtent,
				sizeof(SourceExtent));
	}

	zcoBuf.lastExtent = extentObj;
	zcoBuf.sourceLength += length;
	zcoBuf.totalLength += length;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	return 0;
}

Object	zco_create(Sdr sdr, ZcoMedium firstExtentSourceMedium,
		Object firstExtentLocation, vast firstExtentOffset,
		vast firstExtentLength)
{
	Object	zcoObj;
	Zco	zco;

	CHKERR(sdr);
	if (firstExtentLocation)	/*	First extent provided.	*/
	{
		if (firstExtentLength <= 0)
		{
			putErrmsg("First extent length <= zero.", NULL);
			printStackTrace();
			return ((Object) ERROR);
		}

		if (extentTooLarge(sdr, firstExtentSourceMedium,
				firstExtentLength))
		{
			return 0;	/*	No available ZCO space.	*/
		}
	}
	else				/*	No first extent.	*/
	{
		if (firstExtentLength)
		{
			putErrmsg("First extent location is zero.", NULL);
			printStackTrace();
			return ((Object) ERROR);
		}
	}

	zcoObj = sdr_malloc(sdr, sizeof(Zco));
	if (zcoObj == 0)
	{
		putErrmsg("No space for zco.", NULL);
		return ((Object) ERROR);
	}

	zco_increase_heap_occupancy(sdr, sizeof(Zco));
	memset((char *) &zco, 0, sizeof(Zco));
	sdr_write(sdr, zcoObj, (char *) &zco, sizeof(Zco));
	if (firstExtentLength)
	{
		if (appendExtent(sdr, zcoObj, firstExtentSourceMedium, 0,
				firstExtentLocation, firstExtentOffset,
				firstExtentLength) < 0)
		{
			putErrmsg("Can't append initial extent.", NULL);
			return ((Object) ERROR);
		}
	}

	return zcoObj;
}

vast	zco_append_extent(Sdr sdr, Object zco, ZcoMedium source,
		Object location, vast offset, vast length)
{
	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(location);
	CHKERR(length > 0);
	if (extentTooLarge(sdr, source, length))
	{
		return 0;		/*	No available ZCO space.	*/
	}

	if (appendExtent(sdr, zco, source, 0, location, offset, length) < 0)
	{
		return ERROR;
	}

	return length;
}

int	zco_prepend_header(Sdr sdr, Object zco, char *text, vast length)
{
	vast	increment;
	Capsule	header;
	Object	capsuleObj;
	Zco	zcoBuf;

	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(text);
	CHKERR(length > 0);
	increment = length;
	header.length = length;
	header.text = sdr_malloc(sdr, length);
	if (header.text == 0)
	{
		putErrmsg("No space for header text.", NULL);
		return -1;
	}

	sdr_write(sdr, header.text, text, length);
	header.prevCapsule = 0;
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	header.nextCapsule = zcoBuf.firstHeader;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	increment += sizeof(Capsule);
	zco_increase_heap_occupancy(sdr, increment);
	sdr_write(sdr, capsuleObj, (char *) &header, sizeof(Capsule));
	if (zcoBuf.firstHeader == 0)
	{
		zcoBuf.lastHeader = capsuleObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &header, zcoBuf.firstHeader,
				sizeof(Capsule));
		header.prevCapsule = capsuleObj;
		sdr_write(sdr, zcoBuf.firstHeader, (char *) &header,
				sizeof(Capsule));
	}

	zcoBuf.firstHeader = capsuleObj;
	zcoBuf.aggregateCapsuleLength += length;
	zcoBuf.totalLength += length;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	return 0;
}

void	zco_discard_first_header(Sdr sdr, Object zco)
{
	Zco	zcoBuf;
	Object	obj;
	Capsule	capsule;
	vast	increment;

	CHKVOID(sdr);
	CHKVOID(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if (zcoBuf.firstHeader == 0)
	{
		writeMemo("[?] No header to discard.");
		return;
	}

	sdr_read(sdr, (char *) &capsule, zcoBuf.firstHeader, sizeof(Capsule));
	sdr_free(sdr, capsule.text);		/*	Lose header.	*/
	increment = capsule.length;
	sdr_free(sdr, zcoBuf.firstHeader);	/*	Lose capsule.	*/
	increment += sizeof(Capsule);
	zco_reduce_heap_occupancy(sdr, increment);
	zcoBuf.aggregateCapsuleLength -= capsule.length;
	zcoBuf.totalLength -= capsule.length;
	zcoBuf.firstHeader = capsule.nextCapsule;
	if (capsule.nextCapsule == 0)
	{
		zcoBuf.lastHeader = 0;
	}
	else
	{
		obj = capsule.nextCapsule;
		sdr_stage(sdr, (char *) &capsule, obj, sizeof(Capsule));
		capsule.prevCapsule = 0;
		sdr_write(sdr, obj, (char *) &capsule, sizeof(Capsule));
	}

	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
}

int	zco_append_trailer(Sdr sdr, Object zco, char *text, vast length)
{
	Capsule	trailer;
	vast	increment;
	Object	capsuleObj;
	Zco	zcoBuf;

	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(text);
	CHKERR(length > 0);
	increment = length;
	trailer.length = length;
	trailer.text = sdr_malloc(sdr, length);
	if (trailer.text == 0)
	{
		putErrmsg("No space for trailer text.", NULL);
		return -1;
	}

	sdr_write(sdr, trailer.text, text, length);
	trailer.nextCapsule = 0;
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	trailer.prevCapsule = zcoBuf.lastTrailer;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	increment += sizeof(Capsule);
	zco_increase_heap_occupancy(sdr, increment);
	sdr_write(sdr, capsuleObj, (char *) &trailer, sizeof(Capsule));
	if (zcoBuf.lastTrailer == 0)
	{
		zcoBuf.firstTrailer = capsuleObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &trailer, zcoBuf.lastTrailer,
				sizeof(Capsule));
		trailer.nextCapsule = capsuleObj;
		sdr_write(sdr, zcoBuf.lastTrailer, (char *) &trailer,
				sizeof(Capsule));
	}

	zcoBuf.lastTrailer = capsuleObj;
	zcoBuf.aggregateCapsuleLength += length;
	zcoBuf.totalLength += length;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	return 0;
}

void	zco_discard_last_trailer(Sdr sdr, Object zco)
{
	Zco	zcoBuf;
	vast	increment;
	Object	obj;
	Capsule	capsule;

	CHKVOID(sdr);
	CHKVOID(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if (zcoBuf.lastTrailer == 0)
	{
		writeMemo("[?] No trailer to discard.");
		return;
	}

	sdr_read(sdr, (char *) &capsule, zcoBuf.lastTrailer, sizeof(Capsule));
	sdr_free(sdr, capsule.text);		/*	Lose header.	*/
	increment = capsule.length;
	sdr_free(sdr, zcoBuf.lastTrailer);	/*	Lose capsule.	*/
	increment += sizeof(Capsule);
	zco_reduce_heap_occupancy(sdr, increment);
	zcoBuf.aggregateCapsuleLength -= capsule.length;
	zcoBuf.totalLength -= capsule.length;
	zcoBuf.lastTrailer = capsule.prevCapsule;
	if (capsule.prevCapsule == 0)
	{
		zcoBuf.firstTrailer = 0;
	}
	else
	{
		obj = capsule.prevCapsule;
		sdr_stage(sdr, (char *) &capsule, obj, sizeof(Capsule));
		capsule.nextCapsule = 0;
		sdr_write(sdr, obj, (char *) &capsule, sizeof(Capsule));
	}

	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
}

int	zco_bond(Sdr sdr, Object zco)
{
	Zco		zcoBuf;
	Object		extentObj;
	SourceExtent	extent;
	Object		sdrRefObj;
	SdrRef		sdrRef;
	Object		capsuleObj;
	Capsule		capsule;

	CHKERR(sdr);
	CHKERR(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));

	/*	Convert all headers to source data extents.		*/

	while (zcoBuf.lastHeader)
	{
		extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
		if (extentObj == 0)
		{
			putErrmsg("Can't convert header to extent.", NULL);
			return -1;
		}

		extent.sourceMedium = ZcoSdrSource;
		sdrRefObj = sdr_malloc(sdr, sizeof(SdrRef));
		if (sdrRefObj == 0)
		{
			putErrmsg("Can't create SdrRef for header.", NULL);
			return -1;
		}

		capsuleObj = zcoBuf.lastHeader;
		sdr_read(sdr, (char *) &capsule, capsuleObj, sizeof(Capsule));
		zcoBuf.lastHeader = capsule.prevCapsule;

		/*	Create SdrRef object for capsule content.	*/

		sdrRef.refCount = 1;
		sdrRef.objLength = capsule.length;
		sdrRef.location = capsule.text;
		sdr_write(sdr, sdrRefObj, (char *) &sdrRef, sizeof(SdrRef));

		/*	Content of extent is the SdrRef object.		*/

		extent.location = sdrRefObj;
		extent.offset = 0;
		extent.length = sdrRef.objLength;
		extent.nextExtent = zcoBuf.firstExtent;
		sdr_write(sdr, extentObj, (char *) &extent,
				sizeof(SourceExtent));
		zcoBuf.firstExtent = extentObj;
		if (zcoBuf.lastExtent == 0)
		{
			zcoBuf.lastExtent = zcoBuf.firstExtent;
		}

		sdr_free(sdr, capsuleObj);
		zco_increase_heap_occupancy(sdr, (sizeof(SourceExtent)
				+ sizeof(SdrRef)) - sizeof(Capsule));
	}

	zcoBuf.firstHeader = 0;

	/*	Convert all trailers to source data extents.		*/

	if (zcoBuf.lastExtent)
	{
		sdr_stage(sdr, (char *) &extent, zcoBuf.lastExtent,
				sizeof(SourceExtent));
	}

	while (zcoBuf.firstTrailer)
	{
		extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
		if (extentObj == 0)
		{
			putErrmsg("Can't convert trailer to extent.", NULL);
			return -1;
		}

		if (zcoBuf.lastExtent)
		{
			extent.nextExtent = extentObj;
			sdr_write(sdr, zcoBuf.lastExtent, (char *) &extent,
					sizeof(SourceExtent));
		}

		extent.sourceMedium = ZcoSdrSource;
		sdrRefObj = sdr_malloc(sdr, sizeof(SdrRef));
		if (sdrRefObj == 0)
		{
			putErrmsg("Can't create SdrRef for trailer.", NULL);
			return -1;
		}

		capsuleObj = zcoBuf.firstTrailer;
		sdr_read(sdr, (char *) &capsule, capsuleObj, sizeof(Capsule));
		zcoBuf.firstTrailer = capsule.nextCapsule;

		/*	Create SdrRef object for capsule content.	*/

		sdrRef.refCount = 1;
		sdrRef.objLength = capsule.length;
		sdrRef.location = capsule.text;
		sdr_write(sdr, sdrRefObj, (char *) &sdrRef, sizeof(SdrRef));

		/*	Content of extent is the SdrRef object.		*/

		extent.location = sdrRefObj;
		extent.offset = 0;
		extent.length = sdrRef.objLength;
		zcoBuf.lastExtent = extentObj;
		if (zcoBuf.firstExtent == 0)
		{
			zcoBuf.firstExtent = zcoBuf.lastExtent;
		}

		sdr_free(sdr, capsuleObj);
		zco_increase_heap_occupancy(sdr, (sizeof(SourceExtent)
				+ sizeof(SdrRef)) - sizeof(Capsule));
	}

	if (zcoBuf.lastExtent)
	{
		extent.nextExtent = 0;
		sdr_write(sdr, zcoBuf.lastExtent, (char *) &extent,
				sizeof(SourceExtent));
	}

	zcoBuf.lastTrailer = 0;
	zcoBuf.sourceLength += zcoBuf.aggregateCapsuleLength;
	zcoBuf.aggregateCapsuleLength = 0;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	return 0;
}

static int	cloneExtents(Sdr sdr, Object toZco, Object fromZco, vast offset,
			vast length)
{
	Zco		zcoBuf;
	Object		obj;
	SourceExtent	extent;
	vast		bytesToSkip;
	vast		bytesToCopy;

	/*	Set up reading of old ZCO.				*/

	sdr_read(sdr, (char *) &zcoBuf, fromZco, sizeof(Zco));
	if ((offset + length) >
			(zcoBuf.totalLength - zcoBuf.aggregateCapsuleLength))
	{
		putErrmsg("Offset + length exceeds zco source data length",
				utoa(offset + length));
		return -1;
	}

	/*	Copy subset of old ZCO's extents to new ZCO.		*/

	for (obj = zcoBuf.firstExtent; obj; obj = extent.nextExtent)
	{
		if (length == 0)	/*	Done.			*/
		{
			break;
		}

		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		if (offset >= extent.length)
		{
			offset -= extent.length;
			continue;	/*	Use none of this one.	*/
		}

		/*	Offset has now been reduced to the number of
		 *	bytes to skip over in the first extent that
		 *	contains some portion of the source data we
		 *	want to copy.					*/

		bytesToSkip = offset;
		bytesToCopy = extent.length - bytesToSkip;
		if (bytesToCopy > length)
		{
			bytesToCopy = length;
		}

		/*	Because all extents point to reference objects
		 *	(either file references or SDR heap references)
		 *	no actual copying of data is required at all.	*/

		if (appendExtent(sdr, toZco, extent.sourceMedium, 1,
				extent.location, extent.offset + bytesToSkip,
				bytesToCopy) < 0)
		{
			putErrmsg("Can't append cloned extent to ZCO.", NULL);
			return -1;
		}

		/*	Note consumption of all applicable content
		 *	of this extent.					*/

		offset -= bytesToSkip;
		length -= bytesToCopy;
	}

	return 0;
}

Object	zco_clone(Sdr sdr, Object zco, vast offset, vast length)
{
	Object	newZco;			/*	Cloned ZCO object.	*/

	CHKZERO(sdr);
	CHKZERO(zco);
	CHKZERO(length > 0);
	newZco = zco_create(sdr, 0, 0, 0, 0);
	if (newZco == (Object) ERROR
	|| cloneExtents(sdr, newZco, zco, offset, length) < 0)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return (Object) ERROR;
	}

	return newZco;
}

vast	zco_clone_source_data(Sdr sdr, Object toZco, Object fromZco,
		vast offset, vast length)
{
	CHKZERO(sdr);
	CHKZERO(toZco);
	CHKZERO(fromZco);
	CHKZERO(length > 0);
	if (cloneExtents(sdr, toZco, fromZco, offset, length) < 0)
	{
		putErrmsg("Can't create clone ZCO extents.", NULL);
		return ERROR;
	}

	return length;
}

static void	destroyExtentText(Sdr sdr, SourceExtent *extent,
			ZcoMedium medium, Zco *zco)
{
	SdrRef	sdrRef;
	FileRef	fileRef;

	if (medium == ZcoSdrSource)
	{
		sdr_stage(sdr, (char *) &sdrRef, extent->location,
				sizeof(SdrRef));
		sdrRef.refCount--;
		if (sdrRef.refCount == 0)
		{
			sdr_free(sdr, sdrRef.location);
			sdr_free(sdr, extent->location);
		}
		else	/*	Just update the SDR reference count.	*/
		{
			sdr_write(sdr, extent->location, (char *) &sdrRef,
					sizeof(SdrRef));
		}

		zco_reduce_heap_occupancy(sdr, extent->length);
	}

	if (medium == ZcoFileSource)
	{
		sdr_stage(sdr, (char *) &fileRef, extent->location,
				sizeof(FileRef));
		fileRef.refCount--;
		if (fileRef.refCount == 0 && fileRef.okayToDestroy)
		{
			destroyFileReference(sdr, &fileRef, extent->location);
		}
		else	/*	Just update the file reference count.	*/
		{
			sdr_write(sdr, extent->location, (char *) &fileRef,
					sizeof(FileRef));
		}

		zco_reduce_file_occupancy(sdr, extent->length);
	}
}

static void	destroyFirstExtent(Sdr sdr, Object zcoObj, Zco *zco)
{
	SourceExtent	extent;

	sdr_read(sdr, (char *) &extent, zco->firstExtent, sizeof(SourceExtent));

	/*	Release the extent's content text.			*/

	destroyExtentText(sdr, &extent, extent.sourceMedium, zco);

	/*	Destroy the extent itself.				*/

	sdr_free(sdr, zco->firstExtent);
	zco_reduce_heap_occupancy(sdr, sizeof(SourceExtent));

	/*	Erase the extent from the ZCO.				*/

	zco->firstExtent = extent.nextExtent;
	zco->totalLength -= extent.length;
	if (extent.length > zco->headersLength)
	{
		extent.length -= zco->headersLength;
		zco->headersLength = 0;
	}
	else
	{
		zco->headersLength -= extent.length;
		extent.length = 0;
	}

	if (extent.length > zco->sourceLength)
	{
		extent.length -= zco->sourceLength;
		zco->sourceLength = 0;
	}
	else
	{
		zco->sourceLength -= extent.length;
		extent.length = 0;
	}

	if (extent.length > zco->trailersLength)
	{
		extent.length -= zco->trailersLength;
		zco->trailersLength = 0;
	}
	else
	{
		zco->trailersLength -= extent.length;
		extent.length = 0;
	}
}

static void	destroyZco(Sdr sdr, Object zcoObj)
{
	Zco	zco;
	Object	obj;
	Capsule	capsule;
	vast	occupancy;

	sdr_read(sdr, (char *) &zco, zcoObj, sizeof(Zco));

	/*	Destroy all source data extents.			*/

	while (zco.firstExtent)
	{
		destroyFirstExtent(sdr, zcoObj, &zco);
	}

	/*	Destroy all headers.					*/

	for (obj = zco.firstHeader; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy);
	}

	/*	Destroy all trailers.					*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy);
	}

	/*	Finally destroy the ZCO object.				*/

	sdr_free(sdr, zcoObj);
	zco_reduce_heap_occupancy(sdr, sizeof(Zco));
	_zcoCallback(NULL);
}

void	zco_destroy(Sdr sdr, Object zco)
{
	CHKVOID(sdr);
	CHKVOID(zco);
	destroyZco(sdr, zco);
}

vast	zco_length(Sdr sdr, Object zco)
{
	Zco	zcoBuf;

	CHKZERO(sdr);
	CHKZERO(zco);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	return zcoBuf.totalLength;
}

vast	zco_source_data_length(Sdr sdr, Object zco)
{
	Zco	zcoBuf;
	int	headersLength;
	int	trailersLength;

	CHKZERO(sdr);
	CHKZERO(zco);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	headersLength = zcoBuf.headersLength;
	trailersLength = zcoBuf.trailersLength;

	/*	Check for truncation.					*/

	CHKZERO(headersLength == zcoBuf.headersLength);
	CHKZERO(trailersLength == zcoBuf.trailersLength);
	return zcoBuf.sourceLength + headersLength + trailersLength;
}

static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			vast bytesToSkip, vast bytesAvbl, ZcoReader *reader,
			ZcoMedium sourceMedium)
{
	SdrRef		sdrRef;
	FileRef		fileRef;
	int		fd;
	int		bytesRead;
	struct stat	statbuf;
	unsigned long	xmitProgress = 0;

	if (sourceMedium == ZcoSdrSource)
	{
		sdr_read(sdr, (char *) &sdrRef, extent->location,
				sizeof(SdrRef));
		sdr_read(sdr, buffer, sdrRef.location
				+ extent->offset + bytesToSkip, bytesAvbl);
		return bytesAvbl;
	}
	else	/*	Source text of ZCO is a file.			*/
	{
		if (reader->trackFileOffset)
		{
			xmitProgress = extent->offset + bytesToSkip + bytesAvbl;
		}

		sdr_stage(sdr, (char *) &fileRef, extent->location,
				sizeof(FileRef));
		fd = iopen(fileRef.pathName, O_RDONLY, 0);
		if (fd >= 0)
		{
			if (fstat(fd, &statbuf) < 0)
			{
				close(fd);	/*	Can't check.	*/
			}
			else if (statbuf.st_ino != fileRef.inode)
			{
				close(fd);	/*	File changed.	*/
			}
			else if (lseek(fd, extent->offset + bytesToSkip,
					SEEK_SET) < 0)
			{
				close(fd);	/*	Can't position.	*/
			}
			else
			{
				bytesRead = read(fd, buffer, bytesAvbl);
				close(fd);
				if (bytesRead == bytesAvbl)
				{
					/*	Update xmit progress.	*/

					if (xmitProgress > fileRef.xmitProgress)
					{
						fileRef.xmitProgress
							= xmitProgress;
						sdr_write(sdr, extent->location,
							(char *) &fileRef,
							sizeof(FileRef));
					}

					return bytesAvbl;
				}
			}
		}

		/*	On any problem reading from file, write fill
		 *	and return read length zero.			*/

		memset(buffer, ZCO_FILE_FILL_CHAR, bytesAvbl);
		return 0;
	}
}

/*	Functions for transmission via underlying protocol layer.	*/

void	zco_start_transmitting(Object zco, ZcoReader *reader)
{
	CHKVOID(zco);
	CHKVOID(reader);
	memset((char *) reader, 0, sizeof(ZcoReader));
	reader->zco = zco;
}

void	zco_track_file_offset(ZcoReader *reader)
{
	if (reader)
	{
		reader->trackFileOffset = 1;
	}
}

vast	zco_transmit(Sdr sdr, ZcoReader *reader, vast length, char *buffer)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToTransmit;
	vast		bytesTransmitted;
	Object		obj;
	Capsule		capsule;
	vast		bytesAvbl;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
	CHKERR(length >= 0);
	if (length == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &zco, reader->zco, sizeof(Zco));
	bytesToSkip = reader->lengthCopied;
	bytesToTransmit = length;
	bytesTransmitted = 0;

	/*	Transmit any untransmitted header data.			*/

	for (obj = zco.firstHeader; obj; obj = capsule.nextCapsule)
	{
		if (bytesToTransmit == 0)	/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		bytesAvbl = capsule.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Send none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToTransmit < bytesAvbl)
		{
			bytesAvbl = bytesToTransmit;
		}

		if (buffer)
		{
			sdr_read(sdr, buffer, capsule.text + bytesToSkip,
					bytesAvbl);
			buffer += bytesAvbl;
		}

		bytesToSkip = 0;
		reader->lengthCopied += bytesAvbl;
		bytesToTransmit -= bytesAvbl;
		bytesTransmitted += bytesAvbl;
	}

	/*	Transmit any untransmitted source data.			*/

	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		if (bytesToTransmit == 0)	/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Send none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToTransmit < bytesAvbl)
		{
			bytesAvbl = bytesToTransmit;
		}

		if (buffer)
		{
			if (copyFromSource(sdr, buffer, &extent, bytesToSkip,
				bytesAvbl, reader, extent.sourceMedium) == 0)
			{
				failed = 1;	/*	File problem.	*/
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;
		reader->lengthCopied += bytesAvbl;
		bytesToTransmit -= bytesAvbl;
		bytesTransmitted += bytesAvbl;
	}

	/*	Transmit any untransmitted trailer data.		*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		if (bytesToTransmit == 0)	/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		bytesAvbl = capsule.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Send none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToTransmit < bytesAvbl)
		{
			bytesAvbl = bytesToTransmit;
		}

		if (buffer)
		{
			sdr_read(sdr, buffer, capsule.text + bytesToSkip,
					bytesAvbl);
			buffer += bytesAvbl;
		}

		bytesToSkip = 0;
		reader->lengthCopied += bytesAvbl;
		bytesToTransmit -= bytesAvbl;
		bytesTransmitted += bytesAvbl;
	}

	if (failed)
	{
		return 0;
	}

	return bytesTransmitted;
}

/*	Functions for delivery to overlying protocol or application
 *	layer.								*/

void	zco_start_receiving(Object zco, ZcoReader *reader)
{
	CHKVOID(zco);
	CHKVOID(reader);
	memset((char *) reader, 0, sizeof(ZcoReader));
	reader->zco = zco;
}

vast	zco_receive_headers(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToReceive;
	vast		bytesReceived;
	vast		bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
	CHKERR(length >= 0);
	if (length == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &zco, reader->zco, sizeof(Zco));
	bytesToSkip = reader->headersLengthCopied;
	bytesToReceive = length;
	bytesReceived = 0;
	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Take none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToReceive < bytesAvbl)
		{
			bytesAvbl = bytesToReceive;
		}

		if (buffer)
		{
			if (copyFromSource(sdr, buffer, &extent, bytesToSkip,
				bytesAvbl, reader, extent.sourceMedium) == 0)
			{
				failed = 1;	/*	File problem.	*/
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

		reader->headersLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

void	zco_delimit_source(Sdr sdr, Object zco, vast offset, vast length)
{
	Zco	zcoBuf;
	vast	trailersOffset;
	vast	totalSourceLength;

	CHKVOID(sdr);
	CHKVOID(zco);
	CHKVOID(offset >= 0);
	CHKVOID(length >= 0);
	trailersOffset = offset + length;
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	totalSourceLength = zcoBuf.totalLength - zcoBuf.aggregateCapsuleLength;
	if (trailersOffset > totalSourceLength)
	{
		putErrmsg("Source extends beyond end of ZCO.", NULL);
		return;
	}

	zcoBuf.headersLength = offset;
	zcoBuf.sourceLength = length;
	zcoBuf.trailersLength = totalSourceLength - trailersOffset;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
}

vast	zco_receive_source(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToReceive;
	vast		bytesReceived;
	vast		bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
	CHKERR(length >= 0);
	if (length == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &zco, reader->zco, sizeof(Zco));
	bytesToSkip = zco.headersLength + reader->sourceLengthCopied;
	bytesToReceive = length;
	bytesReceived = 0;
	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Take none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToReceive < bytesAvbl)
		{
			bytesAvbl = bytesToReceive;
		}

		if (buffer)
		{
			if (copyFromSource(sdr, buffer, &extent, bytesToSkip,
				bytesAvbl, reader, extent.sourceMedium) == 0)
			{
				failed = 1;	/*	File problem.	*/
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

		reader->sourceLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

vast	zco_receive_trailers(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToReceive;
	vast		bytesReceived;
	vast		bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
	CHKERR(length >= 0);
	if (length == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &zco, reader->zco, sizeof(Zco));
	bytesToSkip = zco.headersLength + zco.sourceLength
			+ reader->trailersLengthCopied;
	bytesToReceive = length;
	bytesReceived = 0;
	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	Take none of this one.	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToReceive < bytesAvbl)
		{
			bytesAvbl = bytesToReceive;
		}

		if (buffer)
		{
			if (copyFromSource(sdr, buffer, &extent, bytesToSkip,
				bytesAvbl, reader, extent.sourceMedium) == 0)
			{
				failed = 1;	/*	File problem.	*/
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

		reader->trailersLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

void	zco_strip(Sdr sdr, Object zco)
{
	Zco		zcoBuf;
	vast		sourceLengthToSave;
	Object		obj;
	Object		nextExtent;
	SourceExtent	extent;
	int		extentModified;
	vast		headerTextLength;
	vast		trailerTextLength;

	CHKVOID(sdr);
	CHKVOID(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	sourceLengthToSave = zcoBuf.sourceLength;
	for (obj = zcoBuf.firstExtent; obj; obj = nextExtent)
	{
		sdr_stage(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		nextExtent = extent.nextExtent;
		extentModified = 0;
		headerTextLength = 0;

		/*	First strip off any identified header text.	*/

		if (extent.length <= zcoBuf.headersLength)
		{
			/*	Entire extent is header text.		*/

			headerTextLength = extent.length;
		}
		else if (zcoBuf.headersLength > 0)
		{
			/*	Extent includes some header text.	*/

			headerTextLength = zcoBuf.headersLength;
		}

		if (headerTextLength > 0)
		{
			zcoBuf.headersLength -= headerTextLength;
			zcoBuf.totalLength -= headerTextLength;
			extent.offset += headerTextLength;
			extent.length -= headerTextLength;
			extentModified = 1;
		}

		/*	Now strip off remaining text that is known
		 *	not to be source data (must be trailers).	*/

		if (extent.length <= sourceLengthToSave)
		{
			/*	Entire extent is source text.		*/

			sourceLengthToSave -= extent.length;
		}
		else	/*	Extent is partly (or all) trailer text.	*/
		{
			trailerTextLength = extent.length - sourceLengthToSave;
			sourceLengthToSave = 0;
			zcoBuf.trailersLength -= trailerTextLength;
			zcoBuf.totalLength -= trailerTextLength;

			/*	Extent offset is unaffected.		*/

			extent.length -= trailerTextLength;
			extentModified = 1;
		}

		/*	Adjust nextExtent as necessary if it is known
		 *	that there is no more source data.		*/

		if (sourceLengthToSave == 0)
		{
			if (extent.nextExtent != 0)
			{
				extent.nextExtent = 0;
				extentModified = 1;
			}
		}

		/*	Don't update extents unnecessarily.		*/

		if (extentModified == 0)
		{
			continue;
		}

		/*	Extent and Zco must both be rewritten.		*/

		if (extent.length == 0)
		{
			/*	Delete the extent.			*/

			destroyExtentText(sdr, &extent, extent.sourceMedium,
					&zcoBuf);
			sdr_free(sdr, obj);
			zco_reduce_heap_occupancy(sdr, sizeof(SourceExtent));
			if (obj == zcoBuf.firstExtent)
			{
				zcoBuf.firstExtent = extent.nextExtent;
			}
		}
		else	/*	Just update extent's offset and length.	*/
		{
			sdr_write(sdr, obj, (char *) &extent,
					sizeof(SourceExtent));
		}

		sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	}
}
