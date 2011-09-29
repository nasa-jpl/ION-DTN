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
	time_t		inode;		/*	to detect change	*/
	unsigned long	fileLength;
	unsigned long	xmitProgress;
	char		pathName[256];
	char		cleanupScript[256];
} FileRef;

typedef struct
{
	ZcoMedium	sourceMedium;
	Object		location;	/*	of FileRef or text	*/
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

	unsigned int	totalLength;		/*	incl. capsules	*/
	unsigned int	occupancy;		/*	excludes refs	*/
	int		refCount;
} Zco;

typedef struct
{
	Object		zcoObj;
	unsigned int	headersLengthCopied;	/*	within extents	*/
	unsigned int	sourceLengthCopied;	/*	within extents	*/
	unsigned int	trailersLengthCopied;	/*	within extents	*/
	unsigned int	lengthCopied;		/*	incl. capsules	*/
} ZcoReference;

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

static Object	addReference(Sdr sdr, Object zcoObj)
{
	ZcoReference	ref;
	Object		zrObj;
	Zco		zco;

	memset((char *) &ref, 0, sizeof(ZcoReference));
	ref.zcoObj = zcoObj;
	zrObj = sdr_malloc(sdr, sizeof(ZcoReference));
	if (zrObj == 0)
	{
		putErrmsg("No space for reference.", NULL);
		return 0;
	}

	sdr_write(sdr, zrObj, (char *) &ref, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, zcoObj, sizeof(Zco));
	zco.refCount++;
	sdr_write(sdr, zcoObj, (char *) &zco, sizeof(Zco));
	return zrObj;
}

Object	zco_create(Sdr sdr, ZcoMedium firstExtentSourceMedium,
		Object firstExtentLocation,
		unsigned int firstExtentOffset,
		unsigned int firstExtentLength)
{
	Object	zcoObj;
	Zco	zco;
	Object	zrObj;

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
	zrObj = addReference(sdr, zcoObj);
	if (zrObj == 0)
	{
		putErrmsg("Can't create initial zco reference.", NULL);
		return 0;
	}

	if (firstExtentLength)
	{
		if (zco_append_extent(sdr, zrObj, firstExtentSourceMedium,
			firstExtentLocation, firstExtentOffset,
				firstExtentLength) < 0)
		{
			putErrmsg("Can't append initial extent.", NULL);
			return 0;
		}
	}

	return zrObj;
}

int	zco_append_extent(Sdr sdr, Object zcoRef, ZcoMedium sourceMedium,
		Object location, unsigned int offset, unsigned int length)
{
	ZcoReference	ref;
	SourceExtent	extent;
	Object		extentObj;
	Zco		zco;
	SourceExtent	prevExtent;
	FileRef		fileRef;

	CHKERR(sdr);
	CHKERR(zcoRef);
	CHKERR(location);
	CHKERR(length);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	extent.sourceMedium = sourceMedium;
	extent.location = location;
	extent.offset = offset;
	extent.length = length;
	extent.nextExtent = 0;
	extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
	if (extentObj == 0)
	{
		putErrmsg("No space for extent.", NULL);
		return -1;
	}

	sdr_write(sdr, extentObj, (char *) &extent, sizeof(SourceExtent));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if (zco.firstExtent == 0)
	{
		zco.firstExtent = extentObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &prevExtent, zco.lastExtent,
				sizeof(SourceExtent));
		prevExtent.nextExtent = extentObj;
		sdr_write(sdr, zco.lastExtent, (char *) &prevExtent,
				sizeof(SourceExtent));
	}

	zco.occupancy += sizeof(SourceExtent);
	if (sourceMedium == ZcoFileSource)
	{
		sdr_stage(sdr, (char *) &fileRef, location, sizeof(FileRef));
		fileRef.refCount++;
		sdr_write(sdr, location, (char *) &fileRef, sizeof(FileRef));
	}
	else
	{
		zco.occupancy += length;
	}

	zco.lastExtent = extentObj;
	zco.sourceLength += length;
	zco.totalLength += length;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	return 0;
}

int	zco_prepend_header(Sdr sdr, Object zcoRef, char *text,
		unsigned int length)
{
	Capsule		header;
	ZcoReference	ref;
	Object		capsuleObj;
	Zco		zco;

	CHKERR(sdr);
	CHKERR(zcoRef);
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
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	zco.occupancy += length;
	header.nextCapsule = zco.firstHeader;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	sdr_write(sdr, capsuleObj, (char *) &header, sizeof(Capsule));
	zco.occupancy += sizeof(Capsule);
	if (zco.firstHeader == 0)
	{
		zco.lastHeader = capsuleObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &header, zco.firstHeader,
				sizeof(Capsule));
		header.prevCapsule = capsuleObj;
		sdr_write(sdr, zco.firstHeader, (char *) &header,
				sizeof(Capsule));
	}

	zco.firstHeader = capsuleObj;
	zco.totalLength += length;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	return 0;
}

void	zco_discard_first_header(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;
	Object		obj;
	Capsule		capsule;

	CHKVOID(sdr);
	CHKVOID(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if (zco.firstHeader == 0)
	{
		writeMemo("[?] No header to discard.");
		return;
	}

	sdr_read(sdr, (char *) &capsule, zco.firstHeader, sizeof(Capsule));
	sdr_free(sdr, zco.firstHeader);	/*	Lose the capsule.	*/
	zco.occupancy -= sizeof(Capsule);
	sdr_free(sdr, capsule.text);	/*	Lose the header.	*/
	zco.occupancy -= capsule.length;
	zco.totalLength -= capsule.length;
	zco.firstHeader = capsule.nextCapsule;
	if (capsule.nextCapsule == 0)
	{
		zco.lastHeader = 0;
	}
	else
	{
		obj = capsule.nextCapsule;
		sdr_stage(sdr, (char *) &capsule, obj, sizeof(Capsule));
		capsule.prevCapsule = 0;
		sdr_write(sdr, obj, (char *) &capsule, sizeof(Capsule));
	}

	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
}

int	zco_append_trailer(Sdr sdr, Object zcoRef, char *text,
		unsigned int length)
{
	ZcoReference	ref;
	Capsule		trailer;
	Object		capsuleObj;
	Zco		zco;

	CHKERR(sdr);
	CHKERR(zcoRef);
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
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	zco.occupancy += length;
	trailer.prevCapsule = zco.lastTrailer;
	capsuleObj = sdr_malloc(sdr, sizeof(Capsule));
	if (capsuleObj == 0)
	{
		putErrmsg("No space for capsule.", NULL);
		return -1;
	}

	sdr_write(sdr, capsuleObj, (char *) &trailer, sizeof(Capsule));
	zco.occupancy += sizeof(Capsule);
	if (zco.lastTrailer == 0)
	{
		zco.firstTrailer = capsuleObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &trailer, zco.lastTrailer,
				sizeof(Capsule));
		trailer.nextCapsule = capsuleObj;
		sdr_write(sdr, zco.lastTrailer, (char *) &trailer,
				sizeof(Capsule));
	}

	zco.lastTrailer = capsuleObj;
	zco.totalLength += length;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	return 0;
}

void	zco_discard_last_trailer(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;
	Object		obj;
	Capsule		capsule;

	CHKVOID(sdr);
	CHKVOID(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if (zco.lastTrailer == 0)
	{
		writeMemo("[?] No trailer to discard.");
		return;
	}

	sdr_read(sdr, (char *) &capsule, zco.lastTrailer, sizeof(Capsule));
	sdr_free(sdr, zco.lastTrailer);	/*	Lose the capsule.	*/
	zco.occupancy -= sizeof(Capsule);
	sdr_free(sdr, capsule.text);	/*	Lose the header.	*/
	zco.occupancy -= capsule.length;
	zco.totalLength -= capsule.length;
	zco.lastTrailer = capsule.prevCapsule;
	if (capsule.prevCapsule == 0)
	{
		zco.firstTrailer = 0;
	}
	else
	{
		obj = capsule.prevCapsule;
		sdr_stage(sdr, (char *) &capsule, obj, sizeof(Capsule));
		capsule.nextCapsule = 0;
		sdr_write(sdr, obj, (char *) &capsule, sizeof(Capsule));
	}

	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
}

Object	zco_add_reference(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	return addReference(sdr, ref.zcoObj);
}

Object	zco_clone(Sdr sdr, Object zcoRef, unsigned int offset,
		unsigned int length)
{
	Object		newZco;		/*	A ZCO reference.	*/
	ZcoReference	ref;
	Zco		zco;
	Object		obj;
	SourceExtent	extent;
	unsigned int	bytesToSkip;
	unsigned int	bytesToCopy;
	Object		extentLocation;
	unsigned int	extentOffset;
	char		buffer[1024];
	unsigned int	bytesCopied;
	unsigned int	chunkSize;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	CHKZERO(length);
	newZco = zco_create(sdr, 0, 0, 0, 0);
	if (newZco == 0)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return 0;
	}

	/*	Set up reading of old ZCO.				*/

	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if ((offset + length) > zco.totalLength)
	{
		putErrmsg("Offset + length exceeds zco length",
				utoa(offset + length));
		return 0;
	}

	/*	Copy subset of old ZCO's extents to new ZCO.		*/

	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
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

		if (extent.sourceMedium == ZcoFileSource)
		{
			/*	This is just another reference to
				the FileRef to which the original
				ZCO's extent referred.			*/

			extentLocation = extent.location;
			extentOffset = extent.offset + bytesToSkip;
		}
		else
		{
			/*	We have to create a copy of the SDR
			 *	object that will persist after the
			 *	original source ZCO is destroyed.	*/

			extentLocation = sdr_malloc(sdr, bytesToCopy);
			if (extentLocation == 0)
			{
				putErrmsg("Can't clone SDR extent.", NULL);
				return 0;
			}

			bytesCopied = 0;
			while (bytesCopied < bytesToCopy)
			{
				chunkSize = bytesToCopy - bytesCopied;
				if (chunkSize > sizeof buffer)
				{
					chunkSize = sizeof buffer;
				}

				sdr_read(sdr, buffer, extent.location
						+ bytesToSkip + bytesCopied,
						chunkSize);
				sdr_write(sdr, extentLocation + bytesCopied,
						buffer, chunkSize);
				bytesCopied += chunkSize;
			}

			extentOffset = 0;
		}

		if (zco_append_extent(sdr, newZco, extent.sourceMedium,
				extentLocation, extentOffset, bytesToCopy) < 0)
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
	FileRef	fileRef;

	if (medium == ZcoSdrSource)
	{
		sdr_free(sdr, extent->location);
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
	Zco		zco;
	Object		obj;
	Capsule		capsule;

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

void	zco_destroy_reference(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;

	CHKVOID(sdr);
	CHKVOID(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_free(sdr, zcoRef);
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	zco.refCount--;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	if (zco.refCount == 0)	/*	Time to destroy ZCO.		*/
	{
		destroyZco(sdr, ref.zcoObj);
	}
}

unsigned int	zco_length(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	return zco.totalLength;
}

unsigned int	zco_source_data_length(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	return zco.sourceLength;
}

unsigned int	zco_occupancy(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	CHKZERO(sdr_in_xn(sdr));
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	return zco.occupancy + sizeof(ZcoReference);
}

unsigned int	zco_nbr_of_refs(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;

	CHKZERO(sdr);
	CHKZERO(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	return zco.refCount;
}

#if 0
void	zco_concatenate(Sdr sdr, Object aggregateZcoRef, Object atomicZcoRef)
{
	ZcoReference	agRef;
	ZcoReference	atRef;
	Zco		aggregateZco;
	Zco		atomicZco;
	SourceExtent	extent;

	if (sdr == NULL || aggregateZcoRef == 0 || atomicZcoRef == 0)
	{
		putErrmsg(_badArgsMemo(), NULL);
		return;
	}

	sdr_read(sdr, (char *) &agRef, aggregateZcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &atRef, atomicZcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &aggregateZco, agRef.zcoObj, sizeof(Zco));
	sdr_stage(sdr, (char *) &atomicZco, atRef.zcoObj, sizeof(Zco));
	if (atomicZco.refCount != 1 || aggregateZco.refCount != 1)
	{
		putErrmsg("Can't concatenate: multiple references.", NULL);
		return;
	}

	if (atomicZco.firstHeader != 0 || atomicZco.firstTrailer != 0
	|| atomicZco.headersLength != 0 || atomicZco.trailersLength != 0
	|| aggregateZco.firstHeader != 0 || aggregateZco.firstTrailer != 0
	|| aggregateZco.headersLength != 0 || aggregateZco.trailersLength != 0)
	{
		putErrmsg("Can't concatenate unless both ZCOs are stripped.",
				NULL);
		return;
	}

	if (aggregateZco.firstExtent == 0)
	{
		aggregateZco.firstExtent = atomicZco.firstExtent;
	}
	else	/*	Adjust last extent to chain to first new one.	*/
	{
		sdr_stage(sdr, (char *) &extent, aggregateZco.lastExtent,
				sizeof(SourceExtent));
		extent.nextExtent = atomicZco.firstExtent;
		sdr_write(sdr, aggregateZco.lastExtent, (char *) &extent,
				sizeof(SourceExtent));
	}

	aggregateZco.lastExtent = atomicZco.lastExtent;
	aggregateZco.sourceLength += atomicZco.sourceLength;
	sdr_write(sdr, agRef.zcoObj, (char *) &aggregateZco, sizeof(Zco));
	atomicZco.firstExtent = 0;
	atomicZco.lastExtent = 0;
	atomicZco.sourceLength = 0;
	sdr_write(sdr, atRef.zcoObj, (char *) &atomicZco, sizeof(Zco));
	zco_destroy_reference(sdr, atomicZcoRef);
}
#endif
static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			unsigned int bytesToSkip, unsigned int bytesAvbl,
			ZcoReader *reader, ZcoMedium sourceMedium)
{
	FileRef		fileRef;
	int		fd;
	int		bytesRead;
	struct stat	statbuf;
	unsigned long	xmitProgress = 0;

	if (sourceMedium == ZcoSdrSource)
	{
		sdr_read(sdr, buffer, extent->location
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
#if 0
Object	zco_copy(Sdr sdr, Object zcoRef, ZcoMedium sourceMedium,
		char *buffer, unsigned int bufferLength, unsigned int offset,
		unsigned int length)
{
	ZcoReference	ref;
	Zco		zco;
	ZcoReader	reader;
	Zco		newZco;
	FileRef		fileRef;
	FILE		*file = NULL;
	Object		fileRefObj;
	Object		newZcoObj;
	Object		obj;
	SourceExtent	extent;
	unsigned int	bytesAvbl;
	unsigned int	bytesToCopy;
	unsigned int	bytesToSkip;
	unsigned int	copyLength;
	Object		textObj;

	if (sdr == NULL || zcoRef == 0 || buffer == NULL || bufferLength == 0
	|| length == 0)
	{
		putErrmsg(_badArgsMemo(), NULL);
		return 0;
	}

	/*	Set up reading of old ZCO.				*/

	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if ((offset + length) > zco.totalLength)
	{
		putErrmsg("Offset + length exceeds zco length",
				utoa(offset + length));
		return 0;
	}

	reader.reference = 0;		/*	Not needed.		*/
	reader.fileRefObj = 0;
	reader.file = NULL;

	/*	Set up writing of new ZCO.				*/

	memset((char *) &newZco, 0, sizeof(Zco));
	newZco.refCount = 0;
	if (sourceMedium == ZcoFileSource)
	{
		if (*buffer == 0)
		{
			putErrmsg("No file name for new zco", NULL);
			return 0;
		}

		if (strlen(buffer) > 250)
		{
			putErrmsg("zco file name too long, exceeds 250",
					itoa(strlen(buffer)));
			return 0;
		}

		file = fopen(buffer, "w");
		if (file == NULL)
		{
			putSysErrmsg("Can't open new file for ZCO", buffer);
			return 0;
		}

		fileRef.refCount = 0;
		fileRef.okayToDestroy = 0;
		istrcpy(fileRef.pathName, buffer, sizeof fileRef.pathName);
		isprintf(fileRef.cleanupScript, sizeof fileRef.cleanupScript,
				"rm %s", buffer);
	}

	newZcoObj = sdr_malloc(sdr, sizeof(Zco));
	if (newZcoObj == 0)
	{
		if (file)
		{
			fclose(file);
		}

		putErrmsg("No space for first extent.", NULL);
		return 0;
	}

	sdr_write(sdr, newZcoObj, (char *) &newZco, sizeof(Zco));
	if (sourceMedium == ZcoFileSource)
	{
		fileRefObj = sdr_malloc(sdr, sizeof(FileRef));
		if (fileRefObj == 0)
		{
			fclose(file);
			putErrmsg("No space for file ref extent.", NULL);
			return 0;
		}

		sdr_write(sdr, fileRefObj, (char *) &fileRef, sizeof(FileRef));
		if (zco_append_extent(sdr, newZcoObj, sourceMedium, fileRefObj,
				0, length) < 0)
		{
			fclose(file);
			putErrmsg("Can't append extent.", NULL);
			return 0;
		}
	}

	/*	Now copy extents of source data as requested.		*/

	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		if (length == 0)	/*	Done.			*/
		{
			break;
		}

		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (offset >= bytesAvbl)
		{
			offset -= bytesAvbl;
			continue;	/*	Use none of this one.	*/
		}

		/*	Offset has now been reduced to the number of
		 *	bytes to skip over in the first extent that
		 *	contains some portion of the source data we
		 *	want to copy.					*/

		bytesToSkip = offset;
		bytesAvbl -= offset;
		if (length < bytesAvbl)
		{
			bytesAvbl = length;
		}

		/*	bytesAvbl is now the actual number of bytes
		 *	we want to copy out of this extent.  That
		 *	number may exceed the size of the temporary
		 *	buffer we're using for copying the data, so
		 *	we copy in chunks as necessary.			*/

		bytesToCopy = bytesAvbl;
		while (bytesToCopy > 0)
		{
			/*	Get chunk of source data into buffer.	*/

			copyLength = bufferLength;
			if (copyLength > bytesToCopy)
			{
				copyLength = bytesToCopy;
			}

			copyFromSource(sdr, buffer, &extent, bytesToSkip,
					copyLength, &reader, sourceMedium);
			bytesToCopy -= copyLength;
			bytesToSkip += copyLength;

			/*	Now write it to ZCO extent.		*/

			switch (sourceMedium)
			{
			case ZcoSdrSource:
				textObj = sdr_malloc(sdr, copyLength);
				if (textObj == 0)
				{
					putErrmsg("No space for heap extent.",
							NULL);
					return 0;
				}

				sdr_write(sdr, textObj, buffer, copyLength);
				if (zco_append_extent(sdr, newZcoObj,
						sourceMedium, textObj,
						0, copyLength) < 0)
				{
					putErrmsg("Can't append extent.",
							NULL);
					return 0;
				}

				break;

			default:
				if (fwrite(buffer, 1, copyLength, file) <
						copyLength)
				{
					fclose(file);
					putSysErrmsg("Can't finish copying zco \
data to file", NULL);
					destroyZco(sdr, newZcoObj);
					return 0;
				}
			}
		}

		offset = 0;
		length -= bytesAvbl;
	}

	if (file)
	{
		fclose(file);
	}

	return newZcoObj;
}
#endif
/*	Functions for transmission via underlying protocol layer.	*/

void	zco_start_transmitting(Sdr sdr, Object zcoRef, ZcoReader *reader)
{
	CHKVOID(zcoRef);
	CHKVOID(reader);
	reader->reference = zcoRef;
	reader->trackFileOffset = 0;
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
	ZcoReference	ref;
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
	CHKERR(length);
	sdr_stage(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	bytesToSkip = ref.lengthCopied;
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
		ref.lengthCopied += bytesAvbl;
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
		ref.lengthCopied += bytesAvbl;
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
		ref.lengthCopied += bytesAvbl;
		bytesToTransmit -= bytesAvbl;
		bytesTransmitted += bytesAvbl;
	}

	/*	Update ZcoReference if necessary.			*/

	if (bytesTransmitted > 0)
	{
		sdr_write(sdr, reader->reference, (char *) &ref,
				sizeof(ZcoReference));
	}

	if (failed)
	{
		return 0;
	}

	return bytesTransmitted;
}

void	zco_stop_transmitting(Sdr sdr, ZcoReader *reader)
{
	if (reader)
	{
		reader->trackFileOffset = 0;
	}

	return;		/*	For backward compatibility.		*/
}

/*	Functions for delivery to overlying protocol or application
 *	layer.								*/

void	zco_start_receiving(Sdr sdr, Object zcoRef, ZcoReader *reader)
{
	CHKVOID(zcoRef);
	CHKVOID(reader);
	reader->reference = zcoRef;
	reader->trackFileOffset = 0;
}

int	zco_receive_headers(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	ZcoReference	ref;
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
	CHKERR(length > 0);
	sdr_stage(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	bytesToSkip = ref.headersLengthCopied;
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

		/*	Reading a header implicitly asserts an
		 *	increase in aggregate header length and a
		 *	decrease in source data length.			*/

		zco.headersLength += bytesAvbl;
		zco.sourceLength -= bytesAvbl;

		/*	Note bytes copied.				*/

		ref.headersLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}
	}

	/*	Update Zco and ZcoReference if necessary.		*/

	if (bytesReceived > 0)
	{
		sdr_write(sdr, reader->reference, (char *) &ref,
				sizeof(ZcoReference));
		sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

void	zco_restore_source(Sdr sdr, ZcoReader *reader, unsigned int length)
{
	ZcoReference	ref;
	Zco		zco;
	unsigned int	delta;

	CHKVOID(sdr);
	CHKVOID(reader);
	CHKVOID(length > 0);
	sdr_stage(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if (length > zco.headersLength)
	{
		delta = length - zco.headersLength;
		putErrmsg("Would make aggregate header length negative.",
				utoa(delta));
		return;
	}

	zco.headersLength -= length;
	zco.sourceLength += length;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	ref.headersLengthCopied -= length;
	sdr_write(sdr, reader->reference, (char *) &ref, sizeof(ZcoReference));
}

void	zco_delimit_source(Sdr sdr, ZcoReader *reader, unsigned int length)
{
	ZcoReference	ref;
	Zco		zco;
	unsigned int	delta;

	CHKVOID(sdr);
	CHKVOID(reader);
	CHKVOID(length > 0);
	sdr_read(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	if (length < ref.sourceLengthCopied)
	{
		delta = ref.sourceLengthCopied - length;
		putErrmsg("Have already read source data past this length.",
				utoa(delta));
		return;
	}

	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	if (length > zco.sourceLength)
	{
		delta = length - zco.sourceLength;
		putErrmsg("Trying to *increase* source data length.",
				utoa(delta));
		return;
	}

	/*	Delimiting source pairs a decrease in source data
	 *	length with an increase in aggregate trailer length.	*/

	delta = zco.sourceLength - length;
	zco.sourceLength = length;
	zco.trailersLength += delta;
	sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
}

int	zco_receive_source(Sdr sdr, ZcoReader *reader, unsigned int length,
		char *buffer)
{
	ZcoReference	ref;
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
	CHKERR(length);
	sdr_stage(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	bytesToSkip = zco.headersLength + ref.sourceLengthCopied;
	bytesToReceive = length;
	bytesReceived = 0;
	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}

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
				failed = 1;
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

		ref.sourceLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
	}

	/*	Update ZcoReference if necessary.			*/

	if (bytesReceived > 0)
	{
		sdr_write(sdr, reader->reference, (char *) &ref,
				sizeof(ZcoReference));
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
	ZcoReference	ref;
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
	CHKERR(length > 0);
	sdr_stage(sdr, (char *) &ref, reader->reference, sizeof(ZcoReference));
	sdr_read(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	bytesToSkip = zco.headersLength + zco.sourceLength
			+ ref.trailersLengthCopied;
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
				failed = 1;
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

		ref.trailersLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
		{
			break;
		}
	}

	/*	Update ZcoReference if necessary.			*/

	if (bytesReceived > 0)
	{
		sdr_write(sdr, reader->reference, (char *) &ref,
				sizeof(ZcoReference));
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

void	zco_stop_receiving(Sdr sdr, ZcoReader *reader)
{
	if (reader)
	{
		reader->trackFileOffset = 0;
	}

	return;		/*	For backward compatibility.		*/
}

void	zco_strip(Sdr sdr, Object zcoRef)
{
	ZcoReference	ref;
	Zco		zco;
	unsigned int	sourceLengthToSave;
	Object		obj;
	Object		nextExtent;
	SourceExtent	extent;
	int		extentModified;
	unsigned int	headerTextLength;
	unsigned int	trailerTextLength;

	CHKVOID(sdr);
	CHKVOID(zcoRef);
	sdr_read(sdr, (char *) &ref, zcoRef, sizeof(ZcoReference));
	sdr_stage(sdr, (char *) &zco, ref.zcoObj, sizeof(Zco));
	sourceLengthToSave = zco.sourceLength;
	for (obj = zco.firstExtent; obj; obj = nextExtent)
	{
		sdr_stage(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		nextExtent = extent.nextExtent;
		extentModified = 0;
		headerTextLength = 0;

		/*	First strip off any identified header text.	*/

		if (extent.length <= zco.headersLength)
		{
			/*	Entire extent is header text.		*/

			headerTextLength = extent.length;
		}
		else if (zco.headersLength > 0)
		{
			/*	Extent includes some header text.	*/

			headerTextLength = zco.headersLength;
		}

		if (headerTextLength > 0)
		{
			zco.headersLength -= headerTextLength;
			zco.totalLength -= headerTextLength;
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
			zco.trailersLength -= trailerTextLength;
			zco.totalLength -= trailerTextLength;

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
					&zco);
			sdr_free(sdr, obj);
			if (obj == zco.firstExtent)
			{
				zco.firstExtent = extent.nextExtent;
			}
		}
		else	/*	Just update extent's offset and length.	*/
		{
			sdr_write(sdr, obj, (char *) &extent,
					sizeof(SourceExtent));
		}

		sdr_write(sdr, ref.zcoObj, (char *) &zco, sizeof(Zco));
	}
}
