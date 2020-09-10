/*
 *	tccP.h:		private definitions supporting the
 *			implementation of the Trusted Collective
 *			client system.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "lyst.h"
#include "zco.h"
#include "tcc.h"

#ifndef _TCCP_H_
#define _TCCP_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	time_t		timestamp;
	unsigned char	hash[32];
	unsigned int	sharenum;
} TccBlockHeader;

typedef struct
{
	int		sourceAuthNum;
	Object		text;			/*	ZCO (an adu)	*/
} TccBlock;

typedef struct
{
	unsigned int	blocksAnnounced;
	TccBlock	blocks[2];		/*	prime, backup	*/
} TccShare;

typedef struct
{
	time_t		timestamp;		/*	Key field 1.	*/
	unsigned char	hash[32];		/*	Key field 2.	*/
	size_t		blksize;		/*	Key field 3.	*/
	unsigned int	sharesAnnounced;
	TccShare	shares[TCC_FEC_M];
} TccBulletin;

typedef struct
{
	size_t		length;
	Object		data;
} TccContent;

typedef struct
{
	uvast		nodeNbr;
	unsigned int	firstPrimaryShare;	/*	share number	*/
	unsigned int	lastPrimaryShare;	/*	share number	*/
	unsigned int	firstBackupShare;	/*	share number	*/
	unsigned int	lastBackupShare;	/*	share number	*/
} TccAuthority;

typedef struct
{
	int		blocksGroupNbr;
	time_t		lastBulletinTime;
	Object		authorities;	/*	SDR list: TccAuthority	*/
	Object		bulletins;	/*	SDR list: TccBulletin	*/
	Object		contents;	/*	SDR list: TccContent	*/
} TccDB;

typedef struct
{
	Object		dbs;		/*	SDR list: TccDB		*/
} TccMDB;

typedef struct
{
	int		blocksGroupNbr;
	int		tccPid;
	sm_SemId	contentSemaphore;
} TccVdb;

typedef struct
{
	PsmAddress	vdbs;		/*	SmList: TccVdb		*/
} TccMVdb;

extern int		tccInit(int blocksGroupNbr, int numAuths);
extern int		tccStart(int blocksGroupNbr);
extern int		tccIsStarted(int blocksGroupNbr);
extern void		tccStop(int blocksGroupNbr);
extern int		tccAttach(int blocksGroupNbr);
extern Object		getTccDBObj(int blocksGroupNbr);
extern TccVapp		*getTccVdb(int blocksGroupNbr);

#ifdef __cplusplus
}
#endif

#endif	/*	_TCCP_H	*/
