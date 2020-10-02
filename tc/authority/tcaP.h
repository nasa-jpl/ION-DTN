/*
 *	tcaP.h:	private definitions supporting the implementation of
 *		the Trusted Collective authority system.
 *
 *	Copyright (c) 2020, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

#include "tc.h"

#ifndef _TCA_H_
#define _TCA_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	uvast		nodeNbr;
	time_t		effectiveTime;
	Object		acknowledged;		/*	array of char	*/
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[TC_MAX_DATLEN];
} TcaRecord;

#define	TC_HDR_LEN	(sizeof(TcaRecord) - TC_MAX_DATLEN)

typedef struct
{
	uvast		nodeNbr;
	unsigned int	inService;	/*	Boolean.		*/
} TcaAuthority;

typedef struct
{
	int		blocksGroupNbr;
	int		bulletinsGroupNbr;
	int		recordsGroupNbr;
	time_t		currentCompilationTime;	/*	ctime		*/
	time_t		nextCompilationTime;	/*	ctime		*/
	unsigned int	compilationInterval;	/*	Default 3600	*/
	unsigned int	consensusInterval;	/*	"grace period"	*/
	int		hijacked;		/*	Boolean		*/

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
	int		ownAuthIdx;	/*	Own position in list.	*/
	Object		authorities;	/*	SDR list: TcaAuthority	*/
	Object		validClients;	/*	SDR list: node numbers	*/
	Object		currentRecords;	/*	SDR list: TcaRecord	*/
	Object		pendingRecords;	/*	SDR list: TcaRecord	*/
} TcaDB;

typedef struct
{
	int		recvPid;	/*	For stopping tcarecv.	*/
	int		compilePid;	/*	For stopping tcacompile.*/
} TcaVdb;

extern int		tcaInit(int blocksGroupNbr, int bulletinsGroupNbr,
				int recordsGroupNbr, int auths,
				int K, double R);
extern int		tcaStart(int blocksGroupNbr);
extern int		tcaIsStarted(int blocksGroupNbr);
extern void		tcaStop(int blocksGroupNbr);
extern int		tcaAttach(int blocksGroupNbr);
extern Object		getTcaDBObject(int blocksGroupNbr);
extern TcaVdb		*getTcaVdb(int blocksGroupNbr);

#ifdef __cplusplus
}
#endif

#endif
