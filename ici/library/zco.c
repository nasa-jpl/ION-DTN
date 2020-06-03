/*

	zco.c:		API for using zero-copy objects to implement
			deeply stacked communication protocols.

	Author:	Scott Burleigh, JPL

	Copyright (c) 2004, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#include "platform.h"
#include "bulk.h"
#include "zco.h"

#ifndef ZCODEBUG
#define ZCODEBUG	0
#endif

static const char	*bookNames[] = { "inbound ", "outbound" };

/*	The INBOUND and OUTBOUND "books" control ZCOs' occupancy of
 *	SDR heap, bulk storage, and file system space.  Inbound and
 *	outbound ZCOs are managed separately to ensure that reaching
 *	the occupancy limit for outbound ZCOs (due to a volume of
 *	offered traffic in excess of the available outbound buffer
 *	space) never inhibits data reception and vice versa.
 *	Reference-counting objects (FileRef, BulkRef, ObjRef,
 *	ZcoFileLien, ZcoBulkLien, ZcoObjLien) have counts for both
 *	books because there may be references to them in ZCO extents
 *	posted to both books.						*/

typedef struct
{
	double		fileOccupancy;
	double		maxFileOccupancy;
	double		bulkOccupancy;
	double		maxBulkOccupancy;
	double		heapOccupancy;
	double		maxHeapOccupancy;
} ZcoBook;

typedef struct
{
	ZcoBook		books[2];
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
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	unsigned long	inode;		/*	For detecting change.	*/
	unsigned long	fileLength;	/*	For detecting EOF.	*/
	unsigned long	xmitProgress;	/*	For detecting EOF.	*/
	char		pathName[256];
	char		cleanupScript[256];
	char		okayToDestroy;	/*	Boolean.		*/
	char		unlinkOnDestroy;/*	Boolean.		*/
} FileRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	unsigned long	item;		/*	Bulk item location.	*/
	vast		length;		/*	Length of object.	*/
	char		okayToDestroy;	/*	Boolean.		*/
} BulkRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		object;		/*	Heap address of object.	*/
	vast		length;		/*	Length of object.	*/
	char		okayToDestroy;	/*	Boolean.		*/
} ObjRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of FileRef.*/
	vast		length;		/*	Length of lien on file.	*/
} ZcoFileLien;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of BulkRef.*/
	vast		length;		/*	Length of lien on item.	*/
} ZcoBulkLien;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of ObjRef.	*/
	vast		length;		/*	Length of lien on obj.	*/
} ZcoObjLien;

typedef struct
{
	ZcoMedium	sourceMedium;
	Object		location;	/*	of applicable lien	*/
	vast		offset;		/*	within file/item/object	*/
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

	ZcoAcct		acct;
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
				db.books[0].fileOccupancy = 0;
				db.books[0].maxFileOccupancy = 1.0e12;
				db.books[0].bulkOccupancy = 0;
				db.books[0].maxBulkOccupancy = 1.0e12;
				db.books[0].heapOccupancy = 0;
				db.books[0].maxHeapOccupancy = 1.0e12;
				db.books[1].fileOccupancy = 0;
				db.books[1].maxFileOccupancy = 1.0e12;
				db.books[1].bulkOccupancy = 0;
				db.books[1].maxBulkOccupancy = 1.0e12;
				db.books[1].heapOccupancy = 0;
				db.books[1].maxHeapOccupancy = 1.0e12;
				sdr_write(sdr, obj, (char*) &db, sizeof(ZcoDB));
				sdr_catlg(sdr, dbName, 0, obj);
			}
		}
	}

	return obj;
}

void	zco_status(Sdr sdr)
{
	Object		obj;
			OBJ_POINTER(ZcoDB, db);
	int		i;
	ZcoBook		*book;
	char		buffer[128];

	CHKVOID(sdr);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		writeMemo("[?] No ZCO database to print.");
		return;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	for (i = 0, book = db->books; i < 2; i++, book++)
	{
		isprintf(buffer, sizeof buffer, "[i] %s file  max: %13.0f  \
current: %13.0f", bookNames[i], book->maxFileOccupancy, book->fileOccupancy);
		writeMemo(buffer);
		isprintf(buffer, sizeof buffer, "[i] %s bulk  max: %13.0f  \
current: %13.0f", bookNames[i], book->maxBulkOccupancy, book->bulkOccupancy);
		writeMemo(buffer);
		isprintf(buffer, sizeof buffer, "[i] %s heap  max: %13.0f  \
current: %13.0f", bookNames[i], book->maxHeapOccupancy, book->heapOccupancy);
		writeMemo(buffer);
	}
}

static void	_zcoCallback(ZcoCallback *newCallback, ZcoAcct acct)
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
			notify(acct);
		}
	}
}

void	zco_register_callback(ZcoCallback notify)
{
	_zcoCallback(&notify, ZcoUnknown);
}

void	zco_unregister_callback()
{
	ZcoCallback	notify = NULL;

	_zcoCallback(&notify, ZcoUnknown);
}

static void	zco_increase_file_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->fileOccupancy += delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s file occupancy increased to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->fileOccupancy);
writeMemo(buf);
#endif
	}
}

static void	zco_reduce_file_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->fileOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s file occupancy reduced to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->fileOccupancy);
writeMemo(buf);
#endif
	}
}

double	zco_get_file_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->fileOccupancy;
	}
	else
	{
		return 0;
	}
}

void	zco_set_max_file_occupancy(Sdr sdr, double limit, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(limit >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->maxFileOccupancy = limit;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

double	zco_get_max_file_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->maxFileOccupancy;
	}
	else
	{
		return 0;
	}
}

int	zco_enough_file_space(Sdr sdr, vast length, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;
	vast	increment;

	CHKZERO(sdr);
	CHKZERO(length >= 0);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 0;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	book = &(db->books[((int) acct)]);
	increment = book->fileOccupancy + length;
	if (increment < 0)		/*	Overflow.		*/
	{
		return 0;
	}

	return (book->maxFileOccupancy - increment) > 0;
}

static void	zco_increase_bulk_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->bulkOccupancy += delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s bulk occupancy increased to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->bulkOccupancy);
writeMemo(buf);
#endif
	}
}

static void	zco_reduce_bulk_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->bulkOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s bulk occupancy reduced to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->bulkOccupancy);
writeMemo(buf);
#endif
	}
}

double	zco_get_bulk_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->bulkOccupancy;
	}
	else
	{
		return 0;
	}
}

void	zco_set_max_bulk_occupancy(Sdr sdr, double limit, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(limit >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->maxBulkOccupancy = limit;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

double	zco_get_max_bulk_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->maxBulkOccupancy;
	}
	else
	{
		return 0;
	}
}

int	zco_enough_bulk_space(Sdr sdr, vast length, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;
	vast	increment;

	CHKZERO(sdr);
	CHKZERO(length >= 0);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 0;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	book = &(db->books[((int) acct)]);
	increment = book->bulkOccupancy + length;
	if (increment < 0)		/*	Overflow.		*/
	{
		return 0;
	}

	return (book->maxBulkOccupancy - increment) > 0;
}

void	zco_increase_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(delta >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->heapOccupancy += delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s heap occupancy increased to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->heapOccupancy);
writeMemo(buf);
#endif
	}
}

void	zco_reduce_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(delta >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->heapOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
#if ZCODEBUG
char	buf[128];
sprintf(buf, "[i] %s heap occupancy reduced to " VAST_FIELDSPEC ".",
bookNames[((int) acct)], book->heapOccupancy);
writeMemo(buf);
#endif
	}
}

double	zco_get_heap_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->heapOccupancy;
	}
	else
	{
		return 0;
	}
}

void	zco_set_max_heap_occupancy(Sdr sdr, double limit, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(limit >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->maxHeapOccupancy = limit;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

double	zco_get_max_heap_occupancy(Sdr sdr, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;

	CHKZERO(sdr);
	obj = getZcoDB(sdr);
	if (obj)
	{
		GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
		book = &(db->books[((int) acct)]);
		return book->maxHeapOccupancy;
	}
	else
	{
		return 0;
	}
}

int	zco_enough_heap_space(Sdr sdr, vast length, ZcoAcct acct)
{
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;
	vast	increment;

	CHKZERO(sdr);
	CHKZERO(length >= 0);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 0;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	book = &(db->books[((int) acct)]);
	increment = book->heapOccupancy + length;
	if (increment < 0)		/*	Overflow.		*/
	{
		return 0;
	}

	return (book->maxHeapOccupancy - increment) > 0;
}

Object	zco_create_file_ref(Sdr sdr, char *pathName, char *cleanupScript,
		 ZcoAcct acct)
{
	char		pathBuf[MAXPATHLEN + 1];
	int		pathLen;
	int		scriptLen = 0;
	int		sourceFd;
	struct stat	statbuf;
	Object		fileRefObj;
	FileRef		fileRef;

	CHKZERO(sdr);
	CHKZERO(pathName);
	if (qualifyFileName(pathName, pathBuf, sizeof pathBuf) < 0)
	{
		putErrmsg("Path name unusable: length.", pathName);
		return 0;
	}

	pathName = pathBuf;
	pathLen = istrlen(pathName, sizeof pathBuf);
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
		close(sourceFd);
		return 0;
	}

	/*	Parameters verified.  Proceed with FileRef creation.	*/

	close(sourceFd);
	memset((char *) &fileRef, 0, sizeof(FileRef));
	fileRef.refCount[0] = 0;
	fileRef.refCount[1] = 0;
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
	int		pathLen;
	char		pathBuf[256];
	int		scriptLen = 0;
	int		sourceFd;
	struct stat	statbuf;
	FileRef		fileRef;

	CHKERR(sdr);
	CHKERR(fileRefObj);
	CHKERR(pathName);
	CHKERR(sdr_in_xn(sdr));
	if (qualifyFileName(pathName, pathBuf, sizeof pathBuf) < 0)
	{
		writeMemoNote("[?] Can't qualify file name of revised ZCO ref.",
				pathName);
		return 0;
	}

	pathName = pathBuf;
	pathLen = istrlen(pathName, sizeof pathBuf);
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
		close(sourceFd);
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
	if (fileRef.refCount[0] == 0 && fileRef.refCount[1] == 0)
	{
		destroyFileReference(sdr, &fileRef, fileRefObj);
		return;
	}

	fileRef.okayToDestroy = 1;
	sdr_write(sdr, fileRefObj, (char *) &fileRef, sizeof(FileRef));
}

Object	zco_create_bulk_ref(Sdr sdr, unsigned long item, vast length,
		ZcoAcct acct)
{
	Object	bulkRefObj;
	BulkRef	bulkRef;

	CHKZERO(sdr);
	CHKZERO(item);

	/*	Parameters verified.  Proceed with BulkRef creation.	*/

	memset((char *) &bulkRef, 0, sizeof(BulkRef));
	bulkRef.refCount[0] = 0;
	bulkRef.refCount[1] = 0;
	bulkRef.okayToDestroy = 0;
	bulkRef.item = item;
	bulkRef.length = length;
	bulkRefObj = sdr_malloc(sdr, sizeof(BulkRef));
	if (bulkRefObj == 0)
	{
		putErrmsg("No space for bulk reference.", NULL);
		return 0;
	}

	sdr_write(sdr, bulkRefObj, (char *) &bulkRef, sizeof(BulkRef));
	return bulkRefObj;
}

static void	destroyBulkReference(Sdr sdr, BulkRef *bulkRef,
			Object bulkRefObj)
{
	sdr_free(sdr, bulkRefObj);
	bulk_destroy(bulkRef->item);
}

void	zco_destroy_bulk_ref(Sdr sdr, Object bulkRefObj)
{
	BulkRef	bulkRef;

	CHKVOID(sdr);
	CHKVOID(bulkRefObj);
	sdr_stage(sdr, (char *) &bulkRef, bulkRefObj, sizeof(BulkRef));
	if (bulkRef.refCount[0] == 0 && bulkRef.refCount[1] == 0)
	{
		destroyBulkReference(sdr, &bulkRef, bulkRefObj);
		return;
	}

	bulkRef.okayToDestroy = 1;
	sdr_write(sdr, bulkRefObj, (char *) &bulkRef, sizeof(BulkRef));
}

Object	zco_create_obj_ref(Sdr sdr, Object object, vast length, ZcoAcct acct)
{
	Object	objRefObj;
	ObjRef	objRef;

	CHKZERO(sdr);
	CHKZERO(object);

	/*	Parameters verified.  Proceed with ObjRef creation.	*/

	memset((char *) &objRef, 0, sizeof(ObjRef));
	objRef.refCount[0] = 0;
	objRef.refCount[1] = 0;
	objRef.okayToDestroy = 0;
	objRef.object = object;
	objRef.length = length;
	objRefObj = sdr_malloc(sdr, sizeof(ObjRef));
	if (objRefObj == 0)
	{
		putErrmsg("No space for object reference.", NULL);
		return 0;
	}

	sdr_write(sdr, objRefObj, (char *) &objRef, sizeof(ObjRef));
	return objRefObj;
}

static void	destroyObjReference(Sdr sdr, ObjRef *objRef, Object objRefObj)
{
	sdr_free(sdr, objRefObj);
	sdr_free(sdr, objRef->object);
}

void	zco_destroy_obj_ref(Sdr sdr, Object objRefObj)
{
	ObjRef	objRef;

	CHKVOID(sdr);
	CHKVOID(objRefObj);
	sdr_stage(sdr, (char *) &objRef, objRefObj, sizeof(ObjRef));
	if (objRef.refCount[0] == 0 && objRef.refCount[1] == 0)
	{
		destroyObjReference(sdr, &objRef, objRefObj);
		return;
	}

	objRef.okayToDestroy = 1;
	sdr_write(sdr, objRefObj, (char *) &objRef, sizeof(ObjRef));
}

int	zco_extent_too_large(Sdr sdr, ZcoMedium source, vast length,
		ZcoAcct acct)
{
	vast	heapSpaceNeeded = sizeof(SourceExtent);
	Object	obj;
		OBJ_POINTER(ZcoDB, db);
	ZcoBook	*book;
	double	fileSpaceAvbl;
	double	bulkSpaceAvbl;
	double	heapSpaceAvbl;

	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 1;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	book = &(db->books[((int) acct)]);
	fileSpaceAvbl = book->maxFileOccupancy - book->fileOccupancy;
	if (fileSpaceAvbl < 0)
	{
		fileSpaceAvbl = 0;
	}

	bulkSpaceAvbl = book->maxBulkOccupancy - book->bulkOccupancy;
	if (bulkSpaceAvbl < 0)
	{
		bulkSpaceAvbl = 0;
	}

	heapSpaceAvbl = book->maxHeapOccupancy - book->heapOccupancy;
	if (heapSpaceAvbl < 0)
	{
		heapSpaceAvbl = 0;
	}

	switch (source)
	{
	case ZcoFileSource:
		if (length > fileSpaceAvbl)
		{
			return 1;
		}

		break;

	case ZcoBulkSource:
		if (length > bulkSpaceAvbl)
		{
			return 1;
		}

		break;

	case ZcoObjSource:
	case ZcoSdrSource:
		heapSpaceNeeded += length;
		break;

	default:
		/*	Appending an extent whose source is another
		 *	ZCO may be done in two circumstances:
		 *
		 *	1.  The new and old ZCOs are in the same
		 *	    account.  zco_clone_source_data and
		 *	    zco_clone conform to this condition.  In
		 *	    this case no new data item is inserted
		 *	    into that common account, so the new ZCO
		 *	    extent cannot be too large, and therefore
		 *	    calling this function is an error.
		 *
		 *	2.  The new and old ZCOs are in different
		 *	    accounts.  This can ONLY be done by
		 *	    zco_create (passed an initial extent
		 *	    that references the old ZCO) and
		 *	    zco_append_extent, which do their own
		 *	    private extent size checks.  So, again,
		 *	    calling this function is an error.
		 *
		 *	So we reject this function invocation.		*/

		putErrmsg("Invalid source medium.", itoa((int) source));
		return 1;
	}

	if (heapSpaceNeeded > heapSpaceAvbl)
	{
		return 1;
	}

	return 0;
}

void	zco_get_aggregate_length(Sdr sdr, Object sourceZcoObj, vast offset,
		vast length, vast *fileSpaceOccupied, vast *bulkSpaceOccupied,
		vast *heapSpaceOccupied)
{
	vast		endOfSource = offset + length;
	Zco		sourceZco;
	Object		obj;
	SourceExtent	extent;
	vast		bytesToSkip;
	vast		bytesToCount;

	CHKVOID(sdr && sourceZcoObj && offset >= 0 && length >= 0
		&& endOfSource >= 0 && fileSpaceOccupied && bulkSpaceOccupied
		&& heapSpaceOccupied);
	sdr_read(sdr, (char *) &sourceZco, sourceZcoObj, sizeof(Zco));
	if (endOfSource > (sourceZco.totalLength
			- sourceZco.aggregateCapsuleLength))
	{
		*fileSpaceOccupied = -1;
		*bulkSpaceOccupied = -1;
		*heapSpaceOccupied = -1;
		putErrmsg("Offset + length exceeds zco source data length.",
				utoa(endOfSource));
		return;
	}

	*fileSpaceOccupied = 0;
	*bulkSpaceOccupied = 0;
	*heapSpaceOccupied = 0;
	for (obj = sourceZco.firstExtent; obj; obj = extent.nextExtent)
	{
		if (length == 0)	/*	Done.			*/
		{
			return;
		}

		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		if (offset >= extent.length)
		{
			offset -= extent.length;
			continue;	/*	Count none of this one.	*/
		}

		/*	Offset has now been reduced to the number of
		 *	bytes to skip over in the first extent that
		 *	contains some portion of the source data we
		 *	want to count.					*/

		bytesToSkip = offset;
		bytesToCount = extent.length - bytesToSkip;
		if (bytesToCount > length)
		{
			bytesToCount = length;
		}

		*heapSpaceOccupied += sizeof(SourceExtent);
		switch (extent.sourceMedium)
		{
		case ZcoFileSource:
			*fileSpaceOccupied += bytesToCount;
			break;

		case ZcoBulkSource:
			*bulkSpaceOccupied += bytesToCount;
			break;

		default:
			*heapSpaceOccupied += bytesToCount;
		}

		/*	Note that all applicable content of this
		 *	extent has been counted.			*/

		offset -= bytesToSkip;
		length -= bytesToCount;
	}
}

static int	aggregateExtentTooLarge(Sdr sdr, Object location, vast offset,
			vast length, ZcoAcct acct)
{
	vast		fileSpaceNeeded = 0;
	vast		bulkSpaceNeeded = 0;
	vast		heapSpaceNeeded = 0;
	Object		obj;
			OBJ_POINTER(ZcoDB, db);
	ZcoBook		*book;
	double		fileSpaceAvbl;
	double		bulkSpaceAvbl;
	double		heapSpaceAvbl;

	zco_get_aggregate_length(sdr, location, offset, length, 
			&fileSpaceNeeded, &bulkSpaceNeeded, &heapSpaceNeeded);
	obj = getZcoDB(sdr);
	if (obj == 0)
	{
		return 1;
	}

	GET_OBJ_POINTER(sdr, ZcoDB, db, obj);
	book = &(db->books[((int) acct)]);
	fileSpaceAvbl = book->maxFileOccupancy - book->fileOccupancy;
	if (fileSpaceAvbl < 0)
	{
		fileSpaceAvbl = 0;
	}

	if (fileSpaceNeeded > fileSpaceAvbl)
	{
		return 1;
	}

	bulkSpaceAvbl = book->maxBulkOccupancy - book->bulkOccupancy;
	if (bulkSpaceAvbl < 0)
	{
		bulkSpaceAvbl = 0;
	}

	if (bulkSpaceNeeded > bulkSpaceAvbl)
	{
		return 1;
	}

	heapSpaceAvbl = book->maxHeapOccupancy - book->heapOccupancy;
	if (heapSpaceAvbl < 0)
	{
		heapSpaceAvbl = 0;
	}

	if (heapSpaceNeeded > heapSpaceAvbl)
	{
		return 1;
	}

	return 0;
}

static int	appendExtentOfExistingZco(Sdr sdr, Object zcoObj, Zco *zco,
			Zco *sourceZco, vast offset, vast length)
{
	ZcoAcct		acct = zco->acct;
	vast		lengthAppended = 0;
	Object		obj;
	SourceExtent	oldExtent;
	vast		bytesToSkip;
	vast		bytesToCopy;
	Object		extentObj;
	vast		increment;
	SourceExtent	newExtent;
	FileRef		fileRef;
	ZcoFileLien	fileLien;
	BulkRef		bulkRef;
	ZcoBulkLien	bulkLien;
	ObjRef		objRef;
	ZcoObjLien	objLien;
	SourceExtent	prevExtent;

	for (obj = sourceZco->firstExtent; obj; obj = oldExtent.nextExtent)
	{
		if (length == 0)	/*	Done.			*/
		{
			break;
		}

		sdr_read(sdr, (char *) &oldExtent, obj, sizeof(SourceExtent));
		if (offset >= oldExtent.length)
		{
			offset -= oldExtent.length;
			continue;	/*	Copy none of this one.	*/
		}

		/*	Offset has now been reduced to the number of
		 *	bytes to skip over in the first extent that
		 *	contains some portion of the source data
		 *	we want to copy.				*/

		bytesToSkip = offset;
		bytesToCopy = oldExtent.length - bytesToSkip;
		if (bytesToCopy > length)
		{
			bytesToCopy = length;
		}

		extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
		if (extentObj == 0)
		{
			putErrmsg("No space for extent.", NULL);
			return -1;
		}

		increment = sizeof(SourceExtent);
		newExtent.sourceMedium = oldExtent.sourceMedium;
		newExtent.location = oldExtent.location;
		newExtent.offset = oldExtent.offset + bytesToSkip;
		newExtent.length = bytesToCopy;
		newExtent.nextExtent = 0;
		switch (oldExtent.sourceMedium)
		{
		case ZcoFileSource:

			/*	The source extent's location is a file
			 *	lien address.				*/

			sdr_stage(sdr, (char *) &fileLien, oldExtent.location,
					sizeof(ZcoFileLien));
			fileLien.refCount[acct]++;
			sdr_write(sdr, oldExtent.location, (char *) &fileLien,
					sizeof(ZcoFileLien));
			if (fileLien.refCount[acct] == 1)
			{
				/*	Initial insertion of this
				 *	source data item into this
				 *	account, so post it.		*/

				zco_increase_file_occupancy(sdr,
						fileLien.length, acct);
				increment += sizeof(ZcoFileLien);

				/*	This is an additional
				 *	citation of the lien's file
				 *	reference object within this
				 *	account, which must be counted.	*/

				sdr_stage(sdr, (char *) &fileRef,
						fileLien.location,
						sizeof(FileRef));
				fileRef.refCount[acct]++;
				sdr_write(sdr, fileLien.location,
						(char *) &fileRef,
						sizeof(FileRef));
				if (fileRef.refCount[acct] == 1)
				{
					/*	Initial insertion of
					 *	this file reference
					 *	into this account, so
					 *	post it.		*/

					increment += sizeof(FileRef);
				}
			}

			break;

		case ZcoBulkSource:

			/*	The source extent's location is a bulk
			 *	lien address.				*/

			sdr_stage(sdr, (char *) &bulkLien, oldExtent.location,
					sizeof(ZcoBulkLien));
			bulkLien.refCount[acct]++;
			sdr_write(sdr, oldExtent.location, (char *) &bulkLien,
					sizeof(ZcoBulkLien));
			if (bulkLien.refCount[acct] == 1)
			{
				/*	Initial insertion of this
				 *	source data item into this
				 *	account, so post it.		*/

				zco_increase_bulk_occupancy(sdr,
						bulkLien.length, acct);
				increment += sizeof(ZcoBulkLien);

				/*	This is an additional
				 *	citation of the lien's bulk
				 *	reference object within this
				 *	account, which must be counted.	*/

				sdr_stage(sdr, (char *) &bulkRef,
						bulkLien.location,
						sizeof(BulkRef));
				bulkRef.refCount[acct]++;
				sdr_write(sdr, bulkLien.location,
						(char *) &bulkRef,
						sizeof(BulkRef));
				if (bulkRef.refCount[acct] == 1)
				{
					/*	Initial insertion of
					 *	this bulk reference
					 *	into this account, so
					 *	post it.		*/

					increment += sizeof(BulkRef);
				}
			}

			break;

		default:	/*	Must be ZcoObjSource.		*/

			/*	The source extent's location is an
			 *	object lien address.			*/

			sdr_stage(sdr, (char *) &objLien, oldExtent.location,
					sizeof(ZcoObjLien));
			objLien.refCount[acct]++;
			sdr_write(sdr, oldExtent.location, (char *) &objLien,
					sizeof(ZcoObjLien));
			if (objLien.refCount[acct] == 1)
			{
				/*	Initial insertion of this
				 *	source data item into this
				 *	account, so post it.		*/

				increment += (objLien.length
						+ sizeof(ZcoObjLien));

				/*	This is an additional
				 *	citation of the lien's object
				 *	reference object within this
				 *	account, which must be counted.	*/

				sdr_stage(sdr, (char *) &objRef,
						objLien.location,
						sizeof(ObjRef));
				objRef.refCount[acct]++;
				sdr_write(sdr, objLien.location,
						(char *) &objRef,
						sizeof(ObjRef));
				if (objRef.refCount[acct] == 1)
				{
					/*	Initial insertion of
					 *	this object reference
					 *	into this account, so
					 *	post it.		*/

					increment += sizeof(ObjRef);
				}
			}
		}

		zco_increase_heap_occupancy(sdr, increment, acct);
		sdr_write(sdr, extentObj, (char *) &newExtent,
				sizeof(SourceExtent));
		if (zco->firstExtent == 0)
		{
			zco->firstExtent = extentObj;
		}
		else
		{
			sdr_stage(sdr, (char *) &prevExtent, zco->lastExtent,
					sizeof(SourceExtent));
			prevExtent.nextExtent = extentObj;
			sdr_write(sdr, zco->lastExtent, (char *) &prevExtent,
					sizeof(SourceExtent));
		}

		zco->lastExtent = extentObj;
		zco->sourceLength += newExtent.length;
		zco->totalLength += newExtent.length;
		lengthAppended += newExtent.length;
		offset -= bytesToSkip;
		length -= bytesToCopy;
	}

	sdr_write(sdr, zcoObj, (char *) zco, sizeof(Zco));
	return lengthAppended;
}

static int	appendExtent(Sdr sdr, Object zcoObj, Zco *zco,
			ZcoMedium sourceMedium, Object location, vast offset,
			vast length)
{
	ZcoAcct		acct = zco->acct;
	Object		extentObj;
	vast		increment;
	SourceExtent	extent;
	Object		objRefObj;
	Object		lienObj;
	FileRef		fileRef;
	ZcoFileLien	fileLien;
	BulkRef		bulkRef;
	ZcoBulkLien	bulkLien;
	ObjRef		objRef;
	ZcoObjLien	objLien;
	SourceExtent	prevExtent;

	extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
	if (extentObj == 0)
	{
		putErrmsg("No space for extent.", NULL);
		return -1;
	}

	increment = sizeof(SourceExtent);
	if (sourceMedium == ZcoSdrSource)
	{
		/*	Must silently create ObjRef to use instead.	*/

		objRefObj = zco_create_obj_ref(sdr, location, length,
				zco->acct);
		if (objRefObj == 0)
		{
			putErrmsg("No space for object reference.", NULL);
			return -1;
		}

		location = objRefObj;
		sourceMedium = ZcoObjSource;

		/*	Flag new object reference for deletion as soon
		*	as reference counts drop to zero.		*/

		sdr_stage(sdr, (char *) &objRef, location, sizeof(ObjRef));
		objRef.okayToDestroy = 1;
		sdr_write(sdr, location, (char *) &objRef, sizeof(ObjRef));
	}

	extent.sourceMedium = sourceMedium;
	switch (sourceMedium)
	{
	case ZcoFileSource:

		/*	"location" is heap address of a file reference.
		 *	Must create a new file lien.			*/

		lienObj = sdr_malloc(sdr, sizeof(ZcoFileLien));
		if (lienObj == 0)
		{
			putErrmsg("No space for lien object.", NULL);
			return -1;
		}

		memset((char *) &fileLien, 0, sizeof(ZcoFileLien));
		fileLien.length = length;
		fileLien.location = location;
		fileLien.refCount[acct] = 1;
		sdr_write(sdr, lienObj, (char *) &fileLien,
				sizeof(ZcoFileLien));
		zco_increase_file_occupancy(sdr, length, acct);
		increment += sizeof(ZcoFileLien);

		/*	This is an additional citation of the lien's
		 *	file reference object within this account,
		 *	which must be counted.				*/

		sdr_stage(sdr, (char *) &fileRef, location, sizeof(FileRef));
		fileRef.refCount[acct]++;
		sdr_write(sdr, location, (char *) &fileRef, sizeof(FileRef));
		if (fileRef.refCount[acct] == 1)
		{
			/*	Initial insertion of this file
			 *	reference into this account, so
			 *	post it.				*/

			increment += sizeof(FileRef);
		}

		break;

	case ZcoBulkSource:

		/*	"location" is heap address of a bulk reference.
		 *	Must create a new bulk lien.			*/

		lienObj = sdr_malloc(sdr, sizeof(ZcoBulkLien));
		if (lienObj == 0)
		{
			putErrmsg("No space for lien object.", NULL);
			return -1;
		}

		memset((char *) &bulkLien, 0, sizeof(ZcoBulkLien));
		bulkLien.length = length;
		bulkLien.location = location;
		bulkLien.refCount[acct] = 1;
		sdr_write(sdr, lienObj, (char *) &bulkLien,
				sizeof(ZcoBulkLien));
		zco_increase_bulk_occupancy(sdr, length, acct);
		increment += sizeof(ZcoBulkLien);

		/*	This is an additional citation of the lien's
		 *	bulk reference object within this account,
		 *	which must be counted.				*/

		sdr_stage(sdr, (char *) &bulkRef, location, sizeof(BulkRef));
		bulkRef.refCount[acct]++;
		sdr_write(sdr, location, (char *) &bulkRef, sizeof(BulkRef));
		if (bulkRef.refCount[acct] == 1)
		{
			/*	Initial insertion of this bulk
			 *	reference into this account, so
			 *	post it.				*/

			increment += sizeof(BulkRef);
		}

		break;

	default:	/*	Source medium is heap object.		*/

		/*	"location" is heap address of an object
		 *	reference.  Must create a new object lien.	*/

		lienObj = sdr_malloc(sdr, sizeof(ZcoObjLien));
		if (lienObj == 0)
		{
			putErrmsg("No space for lien object.", NULL);
			return -1;
		}

		memset((char *) &objLien, 0, sizeof(ZcoObjLien));
		objLien.length = length;
		objLien.location = location;
		objLien.refCount[acct] = 1;
		sdr_write(sdr, lienObj, (char *) &objLien, sizeof(ZcoObjLien));
		increment += (length + sizeof(ZcoObjLien));

		/*	This is an additional citation of the lien's
		 *	object reference object within this account,
		 *	which must be counted.				*/

		sdr_stage(sdr, (char *) &objRef, location, sizeof(ObjRef));
		objRef.refCount[acct]++;
		sdr_write(sdr, location, (char *) &objRef, sizeof(ObjRef));
		if (objRef.refCount[acct] == 1)
		{
			/*	Initial insertion of this object
			 *	reference into this account, so
			 *	post it.				*/

			increment += sizeof(ObjRef);
		}
	}

	extent.location = lienObj;
	zco_increase_heap_occupancy(sdr, increment, acct);
	extent.offset = offset;
	extent.length = length;
	extent.nextExtent = 0;
	sdr_write(sdr, extentObj, (char *) &extent, sizeof(SourceExtent));
	if (zco->firstExtent == 0)
	{
		zco->firstExtent = extentObj;
	}
	else
	{
		sdr_stage(sdr, (char *) &prevExtent, zco->lastExtent,
				sizeof(SourceExtent));
		prevExtent.nextExtent = extentObj;
		sdr_write(sdr, zco->lastExtent, (char *) &prevExtent,
				sizeof(SourceExtent));
	}

	zco->lastExtent = extentObj;
	zco->sourceLength += length;
	zco->totalLength += length;
	sdr_write(sdr, zcoObj, (char *) zco, sizeof(Zco));
	return length;
}

Object	zco_create(Sdr sdr, ZcoMedium firstExtentSourceMedium,
		Object firstExtentLocation, vast firstExtentOffset,
		vast firstExtentLength, ZcoAcct acct)
{
	Object	zcoObj;
	Zco	zco;
	Zco	sourceZco;
	int	lengthDeclared = 0;
	int	result;

	CHKERR(sdr);
	if (firstExtentSourceMedium == ZcoZcoSource)
	{
		if (firstExtentLocation == 0)
		{
			putErrmsg("No source ZCO indicated.", NULL);
			printStackTrace();
			return ((Object) ERROR);
		}

		sdr_read(sdr, (char *) &sourceZco, firstExtentLocation,
				sizeof(Zco));
		if (sourceZco.acct == acct)
		{
			putErrmsg("Same account; use zco_clone here.", NULL);
			printStackTrace();
			return ((Object) ERROR);
		}

		/*	Negative firstExtentLength indicates that
		 *	the length of the first extent is declared
		 *	to be okay.					*/

		if (firstExtentLength < 0)
		{
			lengthDeclared = 1;
		}

		/*	Force first extent of new ZCO to be the
		 *	concatenation of all SourceExtents of the
		 *	source ZCO.					*/

		firstExtentOffset = 0;
		firstExtentLength = sourceZco.totalLength -
				sourceZco.aggregateCapsuleLength;
		if (!lengthDeclared)
		{
			/*	Must check length of first extent.	*/

			if (aggregateExtentTooLarge(sdr, firstExtentLocation,
				firstExtentOffset, firstExtentLength, acct))
			{
				/*	Not enough available ZCO space.	*/

				return 0;
			}
		}
	}
	else	/*	Not copying from another ZCO.			*/
	{
		if (firstExtentLocation)
		{
			if (firstExtentLength == 0)
			{
				putErrmsg("First extent length is zero.", NULL);
				printStackTrace();
				return ((Object) ERROR);
			}

			if (firstExtentLength < 0)
			{
				/*	Length is declared to be okay.	*/

				firstExtentLength = 0 - firstExtentLength;
			}
			else	/*	Must check length of extent.	*/
			{
				if (zco_extent_too_large(sdr,
						firstExtentSourceMedium,
						firstExtentLength, acct))
				{
					/*	Not enough available
					 *	ZCO space.		*/

					return 0;
				}
			}
		}
		else		/*	No first extent.		*/
		{
			if (firstExtentLength)
			{
				putErrmsg("First extent location is zero.",
						NULL);
				printStackTrace();
				return ((Object) ERROR);
			}
		}
	}

	zcoObj = sdr_malloc(sdr, sizeof(Zco));
	if (zcoObj == 0)
	{
		putErrmsg("No space for zco.", NULL);
		return ((Object) ERROR);
	}

	zco_increase_heap_occupancy(sdr, sizeof(Zco), acct);
	memset((char *) &zco, 0, sizeof(Zco));
	zco.acct = acct;
	sdr_write(sdr, zcoObj, (char *) &zco, sizeof(Zco));
	if (firstExtentLocation)
	{
		if (firstExtentSourceMedium == ZcoZcoSource)
		{
			result = appendExtentOfExistingZco(sdr, zcoObj, &zco,
					&sourceZco, firstExtentOffset,
					firstExtentLength);
		}
		else
		{
			result = appendExtent(sdr, zcoObj, &zco,
					firstExtentSourceMedium,
					firstExtentLocation, firstExtentOffset,
					firstExtentLength);
		}
		
		if (result < 0)
		{
			putErrmsg("Can't append initial extent.", NULL);
			return ((Object) ERROR);
		}
	}

	return zcoObj;
}

vast	zco_append_extent(Sdr sdr, Object zcoObj, ZcoMedium source,
		Object location, vast offset, vast length)
{
	Zco	zco;
	Zco	sourceZco;

	CHKERR(sdr);
	CHKERR(zcoObj);
	CHKERR(location);
	CHKERR(length != 0);
	sdr_stage(sdr, (char *) &zco, zcoObj, sizeof(Zco));

	/*	ZCO space occupancy will be increased; is there room?	*/

	if (length < 0)		/*	Length is declared to be okay.	*/
	{
		length = 0 - length;
	}
	else
	{
		if (zco_extent_too_large(sdr, source, length, zco.acct))
		{
			return 0;	/*	No available ZCO space.	*/
		}
	}

	if (source == ZcoZcoSource)	/*	Cloning.		*/
	{
		sdr_read(sdr, (char *) &sourceZco, location, sizeof(Zco));
		if (sourceZco.acct == zco.acct)
		{
			putErrmsg("Same account; use zco_clone_source_data \
here.", NULL);
			printStackTrace();
			return ERROR;
		}

		if ((offset + length) > (sourceZco.totalLength
				- sourceZco.aggregateCapsuleLength))
		{
			putErrmsg("Offset + length exceeds source data length.",
					utoa(offset + length));
			return ERROR;
		}

		return appendExtentOfExistingZco(sdr, zcoObj, &zco, &sourceZco,
				offset, length);
	}

	return appendExtent(sdr, zcoObj, &zco, source, location, offset,
			length);
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
	zco_increase_heap_occupancy(sdr, increment, zcoBuf.acct);
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
	zco_reduce_heap_occupancy(sdr, increment, zcoBuf.acct);
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
	zco_increase_heap_occupancy(sdr, increment, zcoBuf.acct);
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
	zco_reduce_heap_occupancy(sdr, increment, zcoBuf.acct);
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

Object	zco_header_text(Sdr sdr, Object zco, int skip, vast *length)
{
	Zco	zcoBuf;
	Capsule	capsule;

	CHKZERO(sdr);
	CHKZERO(zco);
	CHKZERO(skip >= 0);
	CHKZERO(length);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if (zcoBuf.firstHeader == 0)
	{
		writeMemo("[?] No headers.");
		return 0;
	}

	sdr_read(sdr, (char *) &capsule, zcoBuf.firstHeader, sizeof(Capsule));
	while (skip > 0)
	{
		if (capsule.nextCapsule == 0)
		{
			writeMemo("[?] No such header.");
			return 0;
		}

		sdr_read(sdr, (char *) &capsule, capsule.nextCapsule,
				sizeof(Capsule));
		skip--;
	}

	*length = capsule.length;
	return capsule.text;
}

Object	zco_trailer_text(Sdr sdr, Object zco, int skip, vast *length)
{
	Zco	zcoBuf;
	Capsule	capsule;

	CHKZERO(sdr);
	CHKZERO(zco);
	CHKZERO(skip >= 0);
	CHKZERO(length);
	sdr_read(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	if (zcoBuf.firstTrailer == 0)
	{
		writeMemo("[?] No trailers.");
		return 0;
	}

	sdr_read(sdr, (char *) &capsule, zcoBuf.firstTrailer, sizeof(Capsule));
	while (skip > 0)
	{
		if (capsule.nextCapsule == 0)
		{
			writeMemo("[?] No such trailer.");
			return 0;
		}

		sdr_read(sdr, (char *) &capsule, capsule.nextCapsule,
				sizeof(Capsule));
		skip--;
	}

	*length = capsule.length;
	return capsule.text;
}

int	zco_bond(Sdr sdr, Object zco)
{
	Zco		zcoBuf;
	ZcoAcct		acct;
	Object		capsuleObj;
	Capsule		capsule;
	Object		objRefObj;
	ObjRef		objRef;
	Object		objLienObj;
	ZcoObjLien	objLien;
	Object		extentObj;
	SourceExtent	extent;

	CHKERR(sdr);
	CHKERR(zco);
	sdr_stage(sdr, (char *) &zcoBuf, zco, sizeof(Zco));
	acct = zcoBuf.acct;

	/*	Convert all headers to source data extents.		*/

	while (zcoBuf.lastHeader)
	{
		capsuleObj = zcoBuf.lastHeader;
		sdr_read(sdr, (char *) &capsule, capsuleObj, sizeof(Capsule));
		zcoBuf.lastHeader = capsule.prevCapsule;

		/*	Create ObjRef object for capsule content.	*/

		objRefObj = sdr_malloc(sdr, sizeof(ObjRef));
		if (objRefObj == 0)
		{
			putErrmsg("Can't create ObjRef for header.", NULL);
			return -1;
		}

		memset((char *) &objRef, 0, sizeof(ObjRef));
		objRef.refCount[acct] = 1;
		objRef.object = capsule.text;
		objRef.length = capsule.length;
		objRef.okayToDestroy = 1;
		sdr_write(sdr, objRefObj, (char *) &objRef, sizeof(ObjRef));

		/*	Create object lien referencing ObjRef object.	*/

		objLienObj = sdr_malloc(sdr, sizeof(ZcoObjLien));
		if (objLienObj == 0)
		{
			putErrmsg("Can't create object lien for header.", NULL);
			return -1;
		}

		memset((char *) &objLien, 0, sizeof(ZcoObjLien));
		objLien.refCount[acct] = 1;
		objLien.location = objRefObj;
		objLien.length = objRef.length;
		sdr_write(sdr, objLienObj, (char *) &objLien,
				sizeof(ZcoObjLien));

		/*	Content of extent is the ZcoObjLien object.	*/

		extentObj = sdr_malloc(sdr, sizeof(SourceExtent));
		if (extentObj == 0)
		{
			putErrmsg("Can't convert header to extent.", NULL);
			return -1;
		}

		memset((char *) &extent, 0, sizeof(SourceExtent));
		extent.sourceMedium = ZcoObjSource;
		extent.location = objLienObj;
		extent.offset = 0;
		extent.length = objLien.length;
		extent.nextExtent = zcoBuf.firstExtent;
		sdr_write(sdr, extentObj, (char *) &extent,
				sizeof(SourceExtent));
		zcoBuf.firstExtent = extentObj;
		if (zcoBuf.lastExtent == 0)
		{
			zcoBuf.lastExtent = zcoBuf.firstExtent;
		}

		sdr_free(sdr, capsuleObj);
		zco_reduce_heap_occupancy(sdr, sizeof(Capsule), acct);

		/*	No need to reduce heap occupancy by the
		 *	length of the capsule's text, because we
		 *	would immediately add it back into the
		 *	heap occupancy of the same account as the
		 *	new object reference's length.			*/

		zco_increase_heap_occupancy(sdr, sizeof(SourceExtent)
				+ sizeof(ZcoObjLien) + sizeof(ObjRef), acct);
	}

	zcoBuf.firstHeader = 0;

	/*	Convert all trailers to source data extents.		*/

	memset((char *) &extent, 0, sizeof(SourceExtent));
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

		capsuleObj = zcoBuf.firstTrailer;
		sdr_read(sdr, (char *) &capsule, capsuleObj, sizeof(Capsule));
		zcoBuf.firstTrailer = capsule.nextCapsule;

		/*	Create ObjRef object for capsule content.	*/

		objRefObj = sdr_malloc(sdr, sizeof(ObjRef));
		if (objRefObj == 0)
		{
			putErrmsg("Can't create ObjRef for trailer.", NULL);
			return -1;
		}

		memset((char *) &objRef, 0, sizeof(ObjRef));
		objRef.refCount[acct] = 1;
		objRef.object = capsule.text;
		objRef.length = capsule.length;
		objRef.okayToDestroy = 1;
		sdr_write(sdr, objRefObj, (char *) &objRef, sizeof(ObjRef));

		/*	Create object lien referencing ObjRef object.	*/

		objLienObj = sdr_malloc(sdr, sizeof(ZcoObjLien));
		if (objLienObj == 0)
		{
			putErrmsg("Can't create lien for trailer.", NULL);
			return -1;
		}

		memset((char *) &objLien, 0, sizeof(ZcoObjLien));
		objLien.refCount[acct] = 1;
		objLien.location = objRefObj;
		objLien.length = objRef.length;
		sdr_write(sdr, objLienObj, (char *) &objLien,
				sizeof(ZcoObjLien));

		/*	Content of extent is the ZcoObjLien object.	*/

		memset((char *) &extent, 0, sizeof(SourceExtent));
		extent.sourceMedium = ZcoObjSource;
		extent.location = objLienObj;
		extent.offset = 0;
		extent.length = objLien.length;
		zcoBuf.lastExtent = extentObj;
		if (zcoBuf.firstExtent == 0)
		{
			zcoBuf.firstExtent = zcoBuf.lastExtent;
		}

		sdr_free(sdr, capsuleObj);
		zco_reduce_heap_occupancy(sdr, sizeof(Capsule), zcoBuf.acct);

		/*	No need to reduce heap occupancy by the
		 *	length of the capsule's text, because we
		 *	would immediately add it back into the
		 *	heap occupancy of the same account as the
		 *	new object reference's length.			*/

		zco_increase_heap_occupancy(sdr, sizeof(SourceExtent)
				+ sizeof(ZcoObjLien) + sizeof(ObjRef), acct);
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

static int	reviseSource(Sdr sdr, SourceExtent *extent, vast bytesToSkip,
			vast bytesExposed, char *buffer)
{
	ZcoObjLien	objLien;
	ObjRef		objRef;
	ZcoBulkLien	bulkLien;
	BulkRef		bulkRef;
	ZcoFileLien	fileLien;
	FileRef		fileRef;
	int		fd;
	int		bytesWritten;

	if (extent->sourceMedium == ZcoObjSource)
	{
		sdr_read(sdr, (char *) &objLien, extent->location,
				sizeof(ZcoObjLien));
		sdr_read(sdr, (char *) &objRef, objLien.location,
				sizeof(ObjRef));
		sdr_write(sdr, objRef.object + extent->offset + bytesToSkip,
				buffer, bytesExposed);
		return 0;
	}

	if (extent->sourceMedium == ZcoBulkSource)
	{
		sdr_read(sdr, (char *) &bulkLien, extent->location,
				sizeof(ZcoBulkLien));
		sdr_read(sdr, (char *) &bulkRef, bulkLien.location,
				sizeof(BulkRef));
		if (bulk_write(bulkRef.item, extent->offset + bytesToSkip,
				buffer, bytesExposed) < 0)
		{
			return -1;
		}

		return 0;
	}

	/*	Must be ZcoFileSource.					*/

	sdr_read(sdr, (char *) &fileLien, extent->location,
			sizeof(ZcoFileLien));
	sdr_read(sdr, (char *) &fileRef, fileLien.location, sizeof(FileRef));
	fd = iopen(fileRef.pathName, O_WRONLY, 0);
	if (fd < 0)
	{
		return -1;
	}

	if (lseek(fd, extent->offset + bytesToSkip, SEEK_SET) < 0)
	{
		close(fd);	/*	Can't position.	*/
		return -1;
	}

	bytesWritten = write(fd, buffer, bytesExposed);
	close(fd);
	if (bytesWritten != bytesExposed)
	{
		return -1;
	}

	return 0;
}

int	zco_revise(Sdr sdr, Object zcoObj, vast offset, char *buffer,
		vast length)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToRevise;
	vast		bytesRevised;
	Object		obj;
	Capsule		capsule;
	vast		bytesExposed;
	SourceExtent	extent;
	int		failed = 0;

	if (length == 0)
	{
		return 0;
	}

	CHKERR(sdr);
	CHKERR(zcoObj);
	CHKERR(offset >= 0);
	CHKERR(buffer);
	CHKERR(length > 0);
	sdr_read(sdr, (char *) &zco, zcoObj, sizeof(Zco));
	bytesToSkip = offset;
	bytesToRevise = length;
	bytesRevised = 0;

	/*	Revise header data as necessary.			*/

	for (obj = zco.firstHeader; obj; obj = capsule.nextCapsule)
	{
		if (bytesToRevise == 0)		/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		bytesExposed = capsule.length;
		if (bytesToSkip >= bytesExposed)
		{
			bytesToSkip -= bytesExposed;
			continue;	/*	Revise none of this.	*/
		}

		bytesExposed -= bytesToSkip;
		if (bytesToRevise < bytesExposed)
		{
			bytesExposed = bytesToRevise;
		}

		sdr_write(sdr, capsule.text + bytesToSkip, buffer,
				bytesExposed);
		buffer += bytesExposed;
		bytesToSkip = 0;
		bytesToRevise -= bytesExposed;
		bytesRevised += bytesExposed;
	}

	/*	Revise source data as necessary.			*/

	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		if (bytesToRevise == 0)		/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesExposed = extent.length;
		if (bytesToSkip >= bytesExposed)
		{
			bytesToSkip -= bytesExposed;
			continue;	/*	Revise none of this.	*/
		}

		bytesExposed -= bytesToSkip;
		if (bytesToRevise < bytesExposed)
		{
			bytesExposed = bytesToRevise;
		}

		if (reviseSource(sdr, &extent, bytesToSkip, bytesExposed,
				buffer) < 0)
		{
			failed = 1;	/*	File or bulk problem.	*/
		}

		bytesToSkip = 0;
		bytesToRevise -= bytesExposed;
		bytesRevised += bytesExposed;
	}

	/*	Revise trailer data as necessary.			*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		if (bytesToRevise == 0)		/*	Done.		*/
		{
			break;
		}

		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		bytesExposed = capsule.length;
		if (bytesToSkip >= bytesExposed)
		{
			bytesToSkip -= bytesExposed;
			continue;	/*	Send none of this one.	*/
		}

		bytesExposed -= bytesToSkip;
		if (bytesToRevise < bytesExposed)
		{
			bytesExposed = bytesToRevise;
		}

		sdr_write(sdr, capsule.text + bytesToSkip, buffer,
				bytesExposed);
		buffer += bytesExposed;
		bytesToSkip = 0;
		bytesToRevise -= bytesExposed;
		bytesRevised += bytesExposed;
	}

	if (failed)
	{
		return -1;
	}

	return 0;
}

Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length)
{
	Zco	fromZco;
	Object	toZcoObj;		/*	Cloned ZCO object.	*/
	Zco	toZco;
	vast	lengthCloned;

	CHKZERO(sdr);
	CHKZERO(fromZcoObj);
	CHKZERO(offset >= 0);
	CHKZERO(length > 0);
	sdr_read(sdr, (char *) &fromZco, fromZcoObj, sizeof(Zco));
	toZcoObj = zco_create(sdr, 0, 0, 0, 0, fromZco.acct);
	if (toZcoObj == (Object) ERROR)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return (Object) ERROR;
	}

	sdr_stage(sdr, (char *) &toZco, toZcoObj, sizeof(Zco));
	lengthCloned = appendExtentOfExistingZco(sdr, toZcoObj, &toZco,
			&fromZco, offset, length);
	if (lengthCloned < 0)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return (Object) ERROR;
	}

	return toZcoObj;
}

vast	zco_clone_source_data(Sdr sdr, Object toZcoObj, Object fromZcoObj,
		vast offset, vast length)
{
	Zco	toZco;
	Zco	fromZco;
	vast	lengthCloned;

	CHKERR(sdr);
	CHKERR(toZcoObj);
	CHKERR(fromZcoObj);
	CHKERR(offset >= 0);
	CHKERR(length > 0);
	sdr_stage(sdr, (char *) &toZco, toZcoObj, sizeof(Zco));
	sdr_read(sdr, (char *) &fromZco, fromZcoObj, sizeof(Zco));
	CHKERR(fromZco.acct == toZco.acct);
	if ((offset + length) > (fromZco.totalLength
			- fromZco.aggregateCapsuleLength))
	{
		putErrmsg("Offset + length exceeds source data length.",
				utoa(offset + length));
		return ERROR;
	}

	lengthCloned = appendExtentOfExistingZco(sdr, toZcoObj, &toZco,
			&fromZco, offset, length);
	if (lengthCloned < 0)
	{
		putErrmsg("Can't create clone ZCO extents.", NULL);
		return ERROR;
	}

	return lengthCloned;
}

static void	destroyExtentText(Sdr sdr, SourceExtent *extent, ZcoAcct acct)
{
	Object		lienObj;
	ZcoObjLien	objLien;
	Object		objRefObj;
	ObjRef		objRef;
	ZcoBulkLien	bulkLien;
	Object		bulkRefObj;
	BulkRef		bulkRef;
	ZcoFileLien	fileLien;
	Object		fileRefObj;
	FileRef		fileRef;

	if (extent->sourceMedium == ZcoObjSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &objLien, lienObj, sizeof(ZcoObjLien));
		objLien.refCount[acct]--;
		if (objLien.refCount[acct] == 0)
		{
			zco_reduce_heap_occupancy(sdr, objLien.length
					+ sizeof(ZcoFileLien), acct);

			/*	In addition, the object reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	obj reference object in this account.	*/

			objRefObj = objLien.location;
			sdr_stage(sdr, (char *) &objRef, objRefObj,
					sizeof(ObjRef));
			objRef.refCount[acct]--;
			if (objRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(ObjRef),
						acct);
			}

			/*	So now the object reference object may
			 *	may no longer be needed.		*/

			if (objRef.refCount[0] == 0 && objRef.refCount[1] == 0
			&& objRef.okayToDestroy)
			{
				destroyObjReference(sdr, &objRef, objRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, objRefObj, (char *) &objRef,
						sizeof(ObjRef));
			}
		}

		if (objLien.refCount[0] == 0 && objLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &objLien,
					sizeof(ZcoObjLien));
		}

		return;
	}

	if (extent->sourceMedium == ZcoBulkSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &bulkLien, lienObj,
				sizeof(ZcoBulkLien));
		bulkLien.refCount[acct]--;
		if (bulkLien.refCount[acct] == 0)
		{
			zco_reduce_bulk_occupancy(sdr, bulkLien.length, acct);
			zco_reduce_heap_occupancy(sdr, sizeof(ZcoBulkLien),
					acct);

			/*	In addition, the bulk reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	bulk reference object in this account.	*/

			bulkRefObj = bulkLien.location;
			sdr_stage(sdr, (char *) &bulkRef, bulkRefObj,
					sizeof(BulkRef));
			bulkRef.refCount[acct]--;
			if (bulkRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(BulkRef),
						acct);
			}

			/*	So now the bulk reference object may
			 *	may no longer be needed.		*/

			if (bulkRef.refCount[0] == 0 && bulkRef.refCount[1] == 0
			&& bulkRef.okayToDestroy)
			{
				destroyBulkReference(sdr, &bulkRef, bulkRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, bulkRefObj, (char *) &bulkRef,
						sizeof(BulkRef));
			}
		}

		if (bulkLien.refCount[0] == 0 && bulkLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &bulkLien,
					sizeof(ZcoBulkLien));
		}

		return;
	}

	if (extent->sourceMedium == ZcoFileSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &fileLien, lienObj,
				sizeof(ZcoFileLien));
		fileLien.refCount[acct]--;
		if (fileLien.refCount[acct] == 0)
		{
			zco_reduce_file_occupancy(sdr, fileLien.length, acct);
			zco_reduce_heap_occupancy(sdr, sizeof(ZcoFileLien),
					acct);

			/*	In addition, the file reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	file reference object in this account.	*/

			fileRefObj = fileLien.location;
			sdr_stage(sdr, (char *) &fileRef, fileRefObj,
					sizeof(FileRef));
			fileRef.refCount[acct]--;
			if (fileRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(FileRef),
						acct);
			}

			/*	So now the file reference object may
			 *	may no longer be needed.		*/

			if (fileRef.refCount[0] == 0 && fileRef.refCount[1] == 0
			&& fileRef.okayToDestroy)
			{
				destroyFileReference(sdr, &fileRef, fileRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, fileRefObj, (char *) &fileRef,
						sizeof(FileRef));
			}
		}

		if (fileLien.refCount[0] == 0 && fileLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &fileLien,
					sizeof(ZcoFileLien));
		}

		return;
	}

	putErrmsg("Extent source medium invalid", itoa(extent->sourceMedium));
}

static void	destroyFirstExtent(Sdr sdr, Object zcoObj, Zco *zco)
{
	SourceExtent	extent;

	sdr_read(sdr, (char *) &extent, zco->firstExtent, sizeof(SourceExtent));

	/*	Release the extent's content text.			*/

	destroyExtentText(sdr, &extent, zco->acct);

	/*	Destroy the extent itself.				*/

	sdr_free(sdr, zco->firstExtent);
	zco_reduce_heap_occupancy(sdr, sizeof(SourceExtent), zco->acct);

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
		zco_reduce_heap_occupancy(sdr, occupancy, zco.acct);
	}

	/*	Destroy all trailers.					*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy, zco.acct);
	}

	/*	Finally destroy the ZCO object.				*/

	sdr_free(sdr, zcoObj);
	zco_reduce_heap_occupancy(sdr, sizeof(Zco), zco.acct);
	_zcoCallback(NULL, zco.acct);
}

void	zco_destroy(Sdr sdr, Object zco)
{
	CHKVOID(sdr);
	CHKVOID(zco);
	destroyZco(sdr, zco);
}

vast	zco_length(Sdr sdr, Object zcoObj)
{
		OBJ_POINTER(Zco, zco);

	CHKZERO(sdr);
	CHKZERO(zcoObj);
	GET_OBJ_POINTER(sdr, Zco, zco, zcoObj);
	return zco->totalLength;
}

vast	zco_source_data_length(Sdr sdr, Object zcoObj)
{
		OBJ_POINTER(Zco, zco);
	int	headersLength;
	int	trailersLength;

	CHKZERO(sdr);
	CHKZERO(zcoObj);
	GET_OBJ_POINTER(sdr, Zco, zco, zcoObj);
	headersLength = zco->headersLength;
	trailersLength = zco->trailersLength;

	/*	Check for truncation.					*/

	CHKZERO(headersLength == zco->headersLength);
	CHKZERO(trailersLength == zco->trailersLength);
	return zco->sourceLength + headersLength + trailersLength;
}

ZcoAcct	zco_acct(Sdr sdr, Object zcoObj)
{
		OBJ_POINTER(Zco, zco);

	CHKZERO(sdr);
	CHKZERO(zcoObj);
	GET_OBJ_POINTER(sdr, Zco, zco, zcoObj);
	return zco->acct;
}

static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			vast bytesToSkip, vast bytesAvbl, ZcoReader *reader)
{
	ZcoObjLien	objLien;
	ObjRef		objRef;
	ZcoBulkLien	bulkLien;
	BulkRef		bulkRef;
	ZcoFileLien	fileLien;
	FileRef		fileRef;
	int		fd;
	int		bytesRead;
	struct stat	statbuf;
	unsigned long	xmitProgress = 0;

	switch (extent->sourceMedium)
	{
	case ZcoObjSource:
		sdr_read(sdr, (char *) &objLien, extent->location,
				sizeof(ZcoObjLien));
		sdr_read(sdr, (char *) &objRef, objLien.location,
				sizeof(ObjRef));
		sdr_read(sdr, buffer, objRef.object
				+ extent->offset + bytesToSkip, bytesAvbl);
		return bytesAvbl;

	case ZcoBulkSource:
		sdr_read(sdr, (char *) &bulkLien, extent->location,
				sizeof(ZcoBulkLien));
		sdr_read(sdr, (char *) &bulkRef, bulkLien.location,
				sizeof(BulkRef));
		return bulk_read(bulkRef.item, buffer,
				extent->offset + bytesToSkip, bytesAvbl);

	default:	/*	Source text of extent is a file.	*/
		if (reader->trackFileOffset)
		{
			xmitProgress = extent->offset + bytesToSkip + bytesAvbl;
		}

		sdr_read(sdr, (char *) &fileLien, extent->location,
				sizeof(ZcoFileLien));
		sdr_stage(sdr, (char *) &fileRef, fileLien.location,
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
						sdr_write(sdr,
							fileLien.location,
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
					bytesAvbl, reader) < bytesAvbl)
			{
				failed = 1;	/*	Source problem.	*/
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
					bytesAvbl, reader) < bytesAvbl)
			{
				failed = 1;	/*	Source problem.	*/
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
					bytesAvbl, reader) < bytesAvbl)
			{
				failed = 1;	/*	Source problem.	*/
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
					bytesAvbl, reader) == 0)
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
			/*	Delete the extent.  Note that the
			 *	applicable occupancy account is
			 *	updated only per the lien to which
			 *	this extent points, so the fact
			 *	that extent.length is now zero
			 *	doesn't mess up the accounting.		*/

			destroyExtentText(sdr, &extent, zcoBuf.acct);
			sdr_free(sdr, obj);
			zco_reduce_heap_occupancy(sdr, sizeof(SourceExtent),
					zcoBuf.acct);
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
