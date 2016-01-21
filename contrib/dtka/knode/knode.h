/*
 *	knode.h:	private definitions supporting the use of
 *			DTKA at ION nodes.
 *
 *	Copyright (c) 2013, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "lyst.h"
#include "zco.h"
#include "bp.h"
#include "dtka.h"
#include "ionsec.h"

#ifndef _KNODE_H_
#define _KNODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	uvast		nodeNbr;
	unsigned int	firstPrimaryShare;	/*	share number	*/
	unsigned int	lastPrimaryShare;	/*	share number	*/
	unsigned int	firstBackupShare;	/*	share number	*/
	unsigned int	lastBackupShare;	/*	share number	*/
} DtkaAuthority;

typedef struct
{
	time_t		timestamp;
	unsigned char	hash[32];
	unsigned int	sharenum;
} DtkaBlockHeader;

typedef struct
{
	int		sourceAuthNum;
	Object		text;			/*	ZCO (an adu)	*/
} DtkaBlock;

typedef struct
{
	unsigned int	blocksAnnounced;
	DtkaBlock	blocks[2];		/*	prime, backup	*/
} DtkaShare;

typedef struct
{
	time_t		timestamp;		/*	Key field 1.	*/
	unsigned char	hash[32];		/*	Key field 2.	*/
	size_t		blksize;		/*	Key field 3.	*/
	unsigned int	sharesAnnounced;
	DtkaShare	shares[DTKA_FEC_M];
} DtkaBulletin;

typedef struct
{
	time_t		lastBulletinTime;
	time_t		nextKeyGenTime;
	unsigned int	keyGenInterval;		/*	At least 60.	*/
	unsigned int	effectiveLeadTime;	/*	At least 20.	*/
	Object		bulletins;	/*	SDR list: DtkaBulletin	*/
	DtkaAuthority	authorities[DTKA_NUM_AUTHS];
} DtkaNodeDB;

typedef struct
{
	int		clockPid;
	int		mgrPid;
} DtkaNodeVdb;

extern int		knodeInit();
extern int		knodeStart();
extern int		knodeIsStarted();
extern void		knodeStop();
extern int		knodeAttach();
extern Object		getKnodeDbObject();
extern DtkaNodeDB	*getKnodeConstants();

#ifdef __cplusplus
}
#endif

#endif
