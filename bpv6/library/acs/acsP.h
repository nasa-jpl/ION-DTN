/*
	acsP.h: private definitions supporting the implementation of 
	Aggregate Custody Signals (ACS) for the bundle protocol.

	Authors: Andrew Jenkins, Sebastian Kuzminsky, 
				University of Colorado at Boulder

	Copyright (c) 2008-2011, Regents of the University of Colorado.
	This work was supported by NASA contracts NNJ05HE10G, NNC06CB40C, and
	NNC07CB47C.
 											*/

#ifndef _ACSP_H_
#define _ACSP_H_

#include "bpP.h"
#include "acs.h"
#include "cteb.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#error "NDEBUG is defined"
#endif
#include <assert.h>
#define ASSERT_ACSSDR_XN    assert(sdr_in_xn(acsSdr) != 0)
#define ASSERT_ACSSDR_NOXN  assert(sdr_in_xn(acsSdr) == 0)
#define ASSERT_BPSDR_XN     assert(sdr_in_xn(bpSdr) != 0)
#define ASSERT_BPSDR_NOXN   assert(sdr_in_xn(bpSdr) == 0)

#ifndef ACS_TTL
#define ACS_TTL              (86400)   /* Bundle lifetime of an ACS */
#endif
#define MAX_ACSLOG_LEN       (512)
#define MAX_REPRACS_LEN      (MAX_ACSLOG_LEN - 50)  /* Max len of a printAcs */
#define DEFAULT_ACS_DELAY    (15)      /* Default s to wait before generating */
#define DEFAULT_ACS_SIZE     (120)     /* Default bytes of ACS payload */
#define ACS_SDR_NAME         "acs"
#define ACS_DBNAME           "acsdb"
#define ACS_SDR_DEFAULT_HEAPWORDS    10000

/* SDR hashes take "estimated number of items" and "preferred mean search
 * depth" arguments which are used to trade memory consumption for lookup
 * time.  We instead just specify directly a preferred number of lists.
 * The actual number of lists allocated depends on the sdrhash implementation
 * but is probably the least prime number greater than the next-larger power
 * of 2. */
#define ACS_CIDHASH_ROWCOUNT (1<<4)
#define ACS_BIDHASH_ROWCOUNT (1<<4)


/* A BP Aggregate Custody Signal is a list of Custody Identifiers that
 * specify bundles for which custody is accepted/rejected.  BP ACS offers
 * compression of custody signals.
 *
 *               Acs	(succeeded, no reason)
 *              /     \
 *           1+10    +2+7
 */

/* Acs... versions keep references in working (non-SDR) memory */
typedef struct
{
	/* Most of the fields here are analogous to those of BpCtSignal */
	Lyst		fills;		/* list of AcsFills. */
	unsigned char	succeeded;	/*	Boolean.		*/
	BpCtReason	reasonCode;
	DtnTime		signalTime;
} Acs;

typedef struct
{
	unsigned int	start;		/* first sequence in a range we're SACKing. */
	unsigned int	length;		/* number of contiguous sequences. */
} AcsFill;

/* SdrAcs... versions keep references in SDR memory */

/* A SdrAcsPendingCust is a custodian for which we have pending custody signals
 * that we haven't emitted yet, because we are hoping to lump many custody
 * signals in one aggregate custody signal.      */

typedef struct
{
	char 			eid[MAX_EID_LEN + 1];

	/* Information about the custodian. */
	unsigned long   acsSize;

	unsigned long   acsDelay;

	/* Data structures associated with sending this custodian ACS. */
	Object			signals;		/* SDR list of SdrAcsSignals. */
} SdrAcsPendingCust;

typedef struct
{
	unsigned char	succeeded;		/* Boolean */
	BpCtReason		reasonCode;
	Object			acsFills;		/* SDR list of SdrAcsFills. */
	Object	     	acsDue;			/* BpEvent */
	Object          pendingCustAddr; /* A pointer back to this sig's parent. */

	/* The serializedZco is a reference to a ZCO *in the BP SDR*, even though
	 * the rest of the ACS data is in the ACS SDR.  This is because the bpSend()
	 * function actually sends the ZCO, and it needs payloads it sends to be in
	 * the BP SDR. */
	Object          serializedZco;
	/* We can't use zco_get_length() to get the length of the serializedZco:
	 * it might lead to fulfillment of the Coffman conds. */
	unsigned long	serializedZcoLength;
} SdrAcsSignal;

/* These don't have references to other objects, and are thus the same
 * for SDR and working memory versions. */
typedef	AcsFill		SdrAcsFill;

typedef struct
{
	Object			pendingCusts;	/* SDR list of SdrAcsPendingCusts. */

	Object			cidHash;		/* SDR hash custodianId -> cbId. */
	Object			bidHash;		/* SDR hash bundleId    -> cbId. */

	Address			id;				/* The next unused CID. */

	unsigned int    logLevel;       /* 0:    Only log errors.
					 * else: Progressively more verbose. */
} AcsDB;
extern AcsDB	*acsConstants;


/* The ACS Custody ID database maintains many-to-one associations between
 * custody IDs and bundles. */
typedef struct
{
	unsigned int   id;
} AcsCustodyId;

/* The information required to uniquely identify a bundle.
 * This is a key for looking up the custody ID in a database. */
typedef struct
{
	char                sourceEid[MAX_EID_LEN];
	BpTimestamp         creationTime;
	unsigned int       fragmentOffset;
	unsigned int       fragmentLength;
} __attribute__((packed)) AcsBundleId;

typedef struct
{
	AcsCustodyId     custodyId;
	AcsBundleId      bundleId;
} AcsCbidEntry;

/* ACS-only functions */
extern Sdr getAcssdr(void);

extern Object getPendingCustodians(void);

extern int cmpSdrAcsSignals(Sdr acsSdr, Address lhsAddr, void *argData);

extern Object findSdrAcsSignal(Object acsSignals, BpCtReason reasonCode,
		unsigned char succeeded, Object *signalAddrPtr);

extern Object findCustodianByEid(Object custodians, const char *eid);

extern unsigned long serializeAcs(Object signalAddr, Object *serializedZco,
		unsigned long lastSerializedSize);

extern int appendToSdrAcsSignals(Object acsSignals, Object pendingCustAddr,
		BpCtReason reasonCode, unsigned char succeeded,
		const CtebScratchpad *cteb);

extern int trySendAcs(SdrAcsPendingCust *custodian, BpCtReason reasonCode,
		unsigned char succeeded, const CtebScratchpad *cteb);

extern int get_or_make_custody_id(const char *sourceEid,
		const BpTimestamp *creationTime, unsigned int fragmentOffset,
		unsigned int fragmentLength, AcsCustodyId *cid);

extern Object getOrMakeCustodianByEid(Object custodians, const char *eid);

extern int get_bundle_id(AcsCustodyId *custodyId, AcsBundleId *id);

extern int destroy_custody_id(AcsBundleId *id);

/* ACS logging
 * Call ACSLOG(level, args) where level is one of the ACSLOGLEVELs, and
 * args are arguments as for printf, a format string and arguments to print:
 *
 *  ACSLOG(ACSLOGLEVEL_ERROR, "Something is horribly wrong: 1 == %d", 2);
 *
 * Set the loglevel via acsadmin's initialize command.  If ACSLOG is called
 * when ACS is not initialized, then it will only print things at loglevels
 * <= WARN.
 *
 * For convenience, you probably want to use ACSLOG_ERROR and friends.     */

#define ACSLOGLEVEL_ERROR                    (1)
#define ACSLOGLEVEL_WARN                     (2)
#define ACSLOGLEVEL_INFO                     (4)
#define ACSLOGLEVEL_DEBUG                    (8)

#ifdef SOLARIS_COMPILER

#define ACSLOG(level, ...)						\
	if ((acsConstants == NULL && level <= ACSLOGLEVEL_WARN)		\
	|| (acsConstants != NULL && level & acsConstants->logLevel))	\
	{								\
		char	acsLogBuf[MAX_ACSLOG_LEN];			\
		snprintf(acsLogBuf, sizeof(acsLogBuf), _VA_ARGS__);	\
		putErrmsg(acsLogBuf, NULL);				\
	}

#define ACSLOG_ERROR(...)						\
	{								\
		char	acsLogBuf[MAX_ACSLOG_LEN];			\
		snprintf(acsLogBuf, sizeof(acsLogBuf), _VA_ARGS__);	\
		putErrmsg(acsLogBuf, NULL);				\
	}

#define ACSLOG_WARN(...)						\
	{								\
		char	acsLogBuf[MAX_ACSLOG_LEN];			\
		snprintf(acsLogBuf, sizeof(acsLogBuf), _VA_ARGS__);	\
		putErrmsg(acsLogBuf, NULL);				\
	}

#define ACSLOG_INFO(...)						\
	if (acsConstants != NULL					\
	&& (ACSLOGLEVEL_INFO & acsConstants->logLevel))			\
	{								\
		char	acsLogBuf[MAX_ACSLOG_LEN];			\
		snprintf(acsLogBuf, sizeof(acsLogBuf), _VA_ARGS__);	\
		putErrmsg(acsLogBuf, NULL);				\
	}

#define ACSLOG_DEBUG(...)						\
	if (acsConstants != NULL					\
	&& (ACSLOGLEVEL_DEBUG & acsConstants->logLevel))		\
	{								\
		char	acsLogBuf[MAX_ACSLOG_LEN];			\
		snprintf(acsLogBuf, sizeof(acsLogBuf), _VA_ARGS__);	\
		putErrmsg(acsLogBuf, NULL);				\
	}

#else

#define ACSLOG(level, args...)						\
	if ((acsConstants == NULL && level <= ACSLOGLEVEL_WARN)		\
	|| (acsConstants != NULL && level & acsConstants->logLevel))	\
	{								\
		char acsLogBuf[MAX_ACSLOG_LEN];				\
		snprintf(acsLogBuf, sizeof(acsLogBuf), args);		\
		putErrmsg(acsLogBuf, NULL);				\
	}

#define ACSLOG_ERROR(args...) ACSLOG(ACSLOGLEVEL_ERROR, args)
#define ACSLOG_WARN(args...)  ACSLOG(ACSLOGLEVEL_WARN, args)
#define ACSLOG_INFO(args...)  ACSLOG(ACSLOGLEVEL_INFO, args)
#define ACSLOG_DEBUG(args...) ACSLOG(ACSLOGLEVEL_DEBUG, args)

#endif

#ifdef __cplusplus
}	/* extern "C" */
#endif


#endif /* _ACSP_H_ */
