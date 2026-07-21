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

#ifndef TCCP_H
#define TCCP_H

#include "tcc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TCC_PRIMARY	(0)
#define TCC_BACKUP	(1)

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
	SdrObject	text; /* ZCO (an adu) */
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
	SdrObject	shares;			/* sdr_list of TccShares */
} TccBulletin;

typedef struct
{
	size_t		length;
	SdrObject	data;
} TccContent;

typedef struct
{
	uvast		fqnn;
	unsigned int	firstPrimaryShare;	/*	share number	*/
	unsigned int	lastPrimaryShare;	/*	share number	*/
	unsigned int	firstBackupShare;	/*	share number	*/
	unsigned int	lastBackupShare;	/*	share number	*/
} TccAuthority;

typedef struct
{
	int		blocksGroupNbr;
	time_t		lastBulletinTime;

	/*	fec_K is the mandated diffusion, i.e., the number
	 *	of blocks into which each bulletin is divided for
	 *	transmission.
	 *
	 *	fec_R is the mandated redundancy, i.e., the percentage
	 *	of  blocks issued per bulletin that will be parity
	 *	blocks rather than extents of the bulletin itself.
	 *
	 *	fec_M is the total number of distinct blocks that will
	 *	be produced for each bulletin.
	 *
	 *		fec_M = fec_K + (fec_R * fec_K)
	 *
	 *	fec_N is the total number of blocks that will be
	 *	transmitted per bulletin, accounting for 50% overlap
	 *	in transmissions among the authorities of the trusted
	 *	collective.  For each share of each bulletin, one
	 *	authority is assigned prime responsibility for
	 *	publishing that share and some other authority is
	 *	assigned backup responsibility for publishing that
	 *	same share.
	 *
	 *		fec_N = fec_M * 2
	 *
	 *	fec_Q is the number of blocks that will be transmitted
	 *	by each authority per bulletin.
	 *
	 *		fec_Q = fec_N / sdr_list_length(authorities)	*/

	int		fec_K;
	double		fec_R;
	int		fec_M;
	int		fec_N;
	int		fec_Q;
	SdrObject	authorities; /* SDR list: TccAuthority */
	SdrObject	bulletins;   /* SDR list: TccBulletin */
	SdrObject	contents;    /* SDR list: TccContent */
} TccDB;

typedef struct
{
	SdrObject dbs; /* SDR list: TccDB */
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

extern int		tccInit(int blocksGroupNbr, int auths, int K, double R);
extern int		tccStart(int blocksGroupNbr);
extern int		tccIsStarted(int blocksGroupNbr);
extern void		tccStop(int blocksGroupNbr);
extern int		tccAttach(int blocksGroupNbr);
extern SdrObject	getTccDBObj(int blocksGroupNbr);
extern TccVdb		*getTccVdb(int blocksGroupNbr);

#ifdef __cplusplus
}
#endif

#endif /* TCCP_H */
