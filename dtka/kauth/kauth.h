/*
 *	kauth.h:	private definitions supporting the implementation
 *			of DTKA key authority programs.
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

#ifndef _KAUTH_H_
#define _KAUTH_H_

#ifdef __cplusplus
extern "C" {
#endif

/*	*	*	Database structure	*	*	*	*/

typedef struct
{
	uvast		nodeNbr;
	time_t		effectiveTime;
	char		acknowledged[DTKA_NUM_AUTHS];
	time_t		assertionTime;
	unsigned short	datLength;
	unsigned char	datValue[DTKA_MAX_DATLEN];
} DtkaRecord;

#define	DTKA_HDR_LEN	(sizeof(DtkaRecord) - DTKA_MAX_DATLEN)

typedef struct
{
	uvast		nodeNbr;
	unsigned int	inService;	/*	Boolean.		*/
} DtkaAuthority;

typedef struct
{
	time_t		currentCompilationTime;	/*	ctime		*/
	time_t		nextCompilationTime;	/*	ctime		*/
	unsigned int	compilationInterval;	/*	Default 3600	*/
	unsigned int	consensusInterval;	/*	60 sec or more	*/
	int		hijacked;		/*	Boolean		*/
	Object		currentRecords;	/*	SDR list: DtkaRecord	*/
	Object		pendingRecords;	/*	SDR list: DtkaRecord	*/
	DtkaAuthority	authorities[DTKA_NUM_AUTHS];
	int		ownAuthIdx;	/*	Own position in array.	*/
} DtkaAuthDB;

typedef struct
{
	int		recvPid;	/*	For stopping karecv.	*/
	int		compilePid;	/*	For stopping kacompile.	*/
} DtkaAuthVdb;

extern int		kauthInit();
extern int		kauthStart();
extern int		kauthIsStarted();
extern void		kauthStop();
extern int		kauthAttach();
extern Object		getKauthDbObject();
extern DtkaAuthDB	*getKauthConstants();

#ifdef __cplusplus
}
#endif

#endif
