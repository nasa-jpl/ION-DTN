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
	Object		text;		/*	header or trailer	*/
	unsigned int	length;
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
	Object		location;
	int		objLength;
} SdrRef;

typedef struct
{
	ZcoMedium	sourceMedium;
	Object		location;	/*	of FileRef or SdrRef	*/
	unsigned int	offset;		/*	within file or object	*/
	unsigned int	length;
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
	unsigned int	headersLength;		/*	within extents	*/
	unsigned int	sourceLength;		/*	within extents	*/
	unsigned int	trailersLength;		/*	within extents	*/

	Object		firstTrailer;		/*	Capsule		*/
	Object		lastTrailer;		/*	Capsule		*/

	unsigned int	aggregateCapsuleLength;
	unsigned int	totalLength;		/*	incl. capsules	*/
	unsigned int	occupancy;		/*	incl. source	*/
} Zco;

static char	*_badArgsMemo()
{
	return "Missing/invalid argument(s).";
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
	GET_OBJ_POINTER(sdr, FileRef, fileRef, fileRefObj);
	return istrcpy(buffer, fileRef->pathName, buflen);
}

unsigned int	zco_file_ref_occupancy(Sdr sdr, Object fileRefObj)
{
	/*	For now, all file reference objects are the same size.	*/

	return sizeof(FileRef);
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

Object	zco_create(Sdr sdr, ZcoMedium firstExtentSourceMedium,
		Object firstExtentLocation,
		unsigned int firstExtentOffset,
		unsigned int firstExtentLength)
{
	Zco	zco;
	Object	zcoObj;

	CHKZERO(sdr);
	CHKZERO(!(firstExtentLocation == 0 && firstExtentLength != 0));
	CHKZERO(!(firstExtentLength == 0 && firstExtentLocation != 0));
	memset((char *) &zco, 0, sizeof(Zco));
	zco.occupancy = sizeof(Zco);
	zcoObj = sdr_malloc(sdr, sizeof(Zco));
	if (zcoObj == 0)
	{
		putErrmsg("No space for zco.", NULL);
		return 0;
	}

	sdr_write(sdr, zcoObj, (char *) &zco, sizeof(Zco));
	if (firstExtentLength)
	{
		if (zco_append_extent(sdr, zcoObj, firstExtentSourceMedium,
				firstExtentLocation, firstExtentOffset,
				firstExtentLength) < 0)
		{
			putErrmsg("Can't append initial extent.", NULL);
			return 0;
		}
	}

	return zcoObj;
}

static int	appendExtent(Sdr sdr, Object zco, ZcoMedium sourceMedium,
			int cloning, Object location, unsigned int offset,
			unsigned int length)
{
	Zco		zcoBuf;
	SourceExtent	extent;
	FileRef		fileRef;
	Object		sdrRefObj;
	SdrRef		sdrRef;
	Object		extentObj;
	SourceExtent	prevExtent;

	extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
	if (extentObj == 0)
	{
		putErrmsg("No space for extent.", NULL);
		return -1;
	}

	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	extent.sourceMedium = sourceMedium;
	if (sourceMedium == ZcoFileSource)
	{
		sdr_stage(sdr, (char *) &fileRef, location, sizeof(FileRef));
		fileRef.refCount++;
		sdr_write(sdr, location, (char *) &fileRef, sizeof(FileRef));
		extent.location = location;
	}
	else if (cloning)
	{
		sdr_stage(sdr, (char *) &sdrRef, location, sizeof(SdrRef));
		sdrRef.refCount++;
		sdr_write(sdr, location, (char *) &sdrRef, sizeof(SdrRef));

		/*	Occupancy is intentionally overstated here,
		 *	because there's no obvious way to make sure
		 *	it's not understated without exposing SdrRefs
		 *	to application code.				*/

		zcoBuf.occupancy += sizeof(SdrRef);
		zcoBuf.occupancy += length;
		extent.location = location;
	}
	else	/*	Initial reference to some object in SDR heap.	*/
	{
		sdrRefObj = sdr_malloc(sdr, sizeof(SdrRef));
		if (sdrRefObj == 0)
		{
			putErrmsg("No space for SDR reference.", NULL);
			return -1;
		}

		sdrRef.refCount = 1;
		sdrRef.location = location;
		sdrRef.objLength = length;
		sdr_write(sdr, sdrRefObj, (char *) &sdrRef, sizeof(SdrRef));
		zcoBuf.occupancy += sizeof(SdrRef);
		zcoBuf.occupancy += length;
		extent.location = sdrRefObj;
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

	zcoBuf.occupancy += sizeof(SourceExtent);
	zcoBuf.lastExtent = extentObj;
	zcoBuf.sourceLength += length;
	zcoBuf.totalLength += length;
	sdr_write(sdr, zco, (char *) &zcoBuf, sizeof(Zco));
	return 0;
}

int	zco_append_extent(Sdr sdr, Object zco, ZcoMedium source,
		Object location, unsigned int offset, unsigned int length)
{
	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(location);
	CHKERR(length);
	return appendExtent(sdr, zco, source, 0, location, offset, length);
}

int	zco_prepend_header(Sdr sdr, Object zco, char *text,
		unsigned int length)
{
	Capsule	header;
	Object	capsuleObj;
	Zco	zcoBuf;

	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(text);
	CHKERR(length);
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
	zcoBuf.occupancy += length;
	header.nextCapsule = zcoBuf.firstHeader;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	sdr_write(sdr, capsuleObj, (char *) &header, sizeof(Capsule));
	zcoBuf.occupancy += sizeof(Capsule);
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

	CHKVOID(sdr);
	CHKVOID(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if (zcoBuf.firstHeader == 0)
	{
		writeMemo("[?] No header to discard.");
		return;
	}

	sdr_read(sdr, (char *) &capsule, zcoBuf.firstHeader, sizeof(Capsule));
	sdr_free(sdr, zcoBuf.firstHeader);	/*	Lose capsule.	*/
	zcoBuf.occupancy -= sizeof(Capsule);
	sdr_free(sdr, capsule.text);		/*	Lose header.	*/
	zcoBuf.occupancy -= capsule.length;
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

int	zco_append_trailer(Sdr sdr, Object zco, char *text,
		unsigned int length)
{
	Capsule	trailer;
	Object	capsuleObj;
	Zco	zcoBuf;

	CHKERR(sdr);
	CHKERR(zco);
	CHKERR(text);
	CHKERR(length);
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
	zcoBuf.occupancy += length;
	trailer.prevCapsule = zcoBuf.lastTrailer;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	sdr_write(sdr, capsuleObj, (char *) &trailer, sizeof(Capsule));
	zcoBuf.occupancy += sizeof(Capsule);
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
	sdr_free(sdr, zcoBuf.lastTrailer);	/*	Lose capsule.	*/
	zcoBuf.occupancy -= sizeof(Capsule);
	sdr_free(sdr, capsule.text);		/*	Lose header.	*/
	zcoBuf.occupancy -= capsule.length;
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

Object	zco_clone(Sdr sdr, Object zco, unsigned int offset,
		unsigned int length)
{
	Object		newZco;		/*	Cloned ZCO object.	*/
	Zco		zcoBuf;
	Object		obj;
	SourceExtent	extent;
	unsigned int	bytesToSkip;
	unsigned int	bytesToCopy;

	CHKZERO(sdr);
	CHKZERO(zco);
	CHKZERO(length);
	newZco = zco_create(sdr, 0, 0, 0, 0);
	if (newZco == 0)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return 0;
	}

	/*	Set up reading of old ZCO.				*/

	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if ((offset + length) >
			(zcoBuf.totalLength - zcoBuf.aggregateCapsuleLength))
	{
		putErrmsg("Offset + length exceeds zco source data length",
				utoa(offset + length));
		return 0;
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

		if (appendExtent(sdr, newZco, extent.sourceMedium, 1,
				extent.location, extent.offset + bytesToSkip,
				bytesToCopy) < 0)
		{
			putErrmsg("Can't add extent to cloned ZCO.", NULL);
			return 0;
		}

		/*	Note consumption of all applicable content
		 *	of this extent.					*/

		offset -= bytesToSkip;
		length -= bytesToCopy;
	}

	return newZco;
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

		/*	Redress overstatement of SDR heap occupancy
		 *	here.						*/

		zco->occupancy -= sizeof(SdrRef);
		zco->occupancy -= extent->length;
	}
	else
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

	/*	Erase the extent from the ZCO.				*/

	zco->occupancy -= sizeof(SourceExtent);
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
		sdr_free(sdr, obj);
	}

	/*	Destroy all trailers.					*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		sdr_free(sdr, obj);
	}

	/*	Finally destroy the ZCO object.				*/

	sdr_free(sdr, zcoObj);
}

void	zco_destroy(Sdr sdr, Object zco)
{
	CHKVOID(sdr);
	CHKVOID(zco);
	destroyZco(sdr, zco);
}

unsigned int	zco_length(Sdr sdr, Object zco)
{
	Zco	zcoBuf;

	CHKZERO(sdr);
	CHKZERO(zco);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	return zcoBuf.totalLength;
}

unsigned int	zco_source_data_length(Sdr sdr, Object zco)
{
	Zco	zcoBuf;

	CHKZERO(sdr);
	CHKZERO(zco);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	return zcoBuf.sourceLength + zcoBuf.headersLength
			+ zcoBuf.trailersLength;
}

unsigned int	zco_occupancy(Sdr sdr, Object zco)
{
	Zco	zcoBuf;

	CHKZERO(sdr);
	CHKZERO(zco);
	CHKZERO(sdr_in_xn(sdr));
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	return zcoBuf.occupancy;
}

#if 0
void	zco_concatenate(Sdr sdr, Object aggregateZco, Object atomicZco)
{
	Zco		agZco;
	Zco		atZco;
	SourceExtent	extent;

	if (sdr == NULL || aggregateZco == 0 || atomicZco == 0)
	{
		putErrmsg(_badArgsMemo(), NULL);
		return;
	}

	sdr_stage(sdr, (char *) &agZco, aggregateZco, sizeof(Zco));
	sdr_stage(sdr, (char *) &atZco, atomicZco, sizeof(Zco));
	if (atZco.firstHeader != 0 || atZco.firstTrailer != 0
	|| atZco.headersLength != 0 || atZco.trailersLength != 0
	|| agZco.firstHeader != 0 || agZco.firstTrailer != 0
	|| agZco.headersLength != 0 || agZco.trailersLength != 0)
	{
		putErrmsg("Can't concatenate unless both ZCOs are stripped.",
				NULL);
		return;
	}

	if (agZco.firstExtent == 0)
	{
		agZco.firstExtent = atZco.firstExtent;
	}
	else	/*	Adjust last extent to chain to first new one.	*/
	{
		sdr_stage(sdr, (char *) &extent, agZco.lastExtent,
				sizeof(SourceExtent));
		extent.nextExtent = atZco.firstExtent;
		sdr_write(sdr, agZco.lastExtent, (char *) &extent,
				sizeof(SourceExtent));
	}

	agZco.lastExtent = atZco.lastExtent;
	agZco.sourceLength += atZco.sourceLength;
	sdr_write(sdr, aggregateZco, (char *) &agZco, sizeof(Zco));
	atZco.firstExtent = 0;
	atZco.lastExtent = 0;
	atZco.sourceLength = 0;
	sdr_write(sdr, atomicZco, (char *) &atZco, sizeof(Zco));
	zco_destroy(sdr, atomicZco);
}
#endif
static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			unsigned int bytesToSkip, unsigned int bytesAvbl,
			ZcoReader *reader, ZcoMedium sourceMedium)
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

int	zco_transmit(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	Zco		zco;
	unsigned int	bytesToSkip;
	unsigned int	bytesToTransmit;
	int		bytesTransmitted;
	Object		obj;
	Capsule		capsule;
	unsigned int	bytesAvbl;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
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

int	zco_receive_headers(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	Zco		zco;
	unsigned int	bytesToSkip;
	unsigned int	bytesToReceive;
	int		bytesReceived;
	unsigned int	bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
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

void	zco_delimit_source(Sdr sdr, Object zco, unsigned int offset,
		unsigned int length)
{
	Zco		zcoBuf;
	unsigned int	trailersOffset;
	unsigned int	totalSourceLength;

	CHKVOID(sdr);
	CHKVOID(zco);
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

int	zco_receive_source(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	Zco		zco;
	unsigned int	bytesToSkip;
	unsigned int	bytesToReceive;
	int		bytesReceived;
	unsigned int	bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
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

int	zco_receive_trailers(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	Zco		zco;
	unsigned int	bytesToSkip;
	unsigned int	bytesToReceive;
	int		bytesReceived;
	unsigned int	bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

	CHKERR(sdr);
	CHKERR(reader);
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
	unsigned int	sourceLengthToSave;
	Object		obj;
	Object		nextExtent;
	SourceExtent	extent;
	int		extentModified;
	unsigned int	headerTextLength;
	unsigned int	trailerTextLength;

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
