/*	bpcp.h:	header for bpcp, a remote copy utility that utilizes CFDP
 *
 *	Copyright (c) 2012, California Institute of Technology.
 *	All rights reserved.
 *	Author: Samuel Jero <sj323707@ohio.edu>, Ohio University
 */
#include "cfdp.h"
#include "cfdpops.h"
#include "bputa.h"

#define BPCP_VERSION_STRING "bpcp version 1.0\nApril 2012\nAuthor: Samuel Jero <sj323707@ohio.edu>\n"

/*There are two possible modes of operation for bpcp:
 * 1)bpcp could wait for each transfer to be sent and
 *   then continue. This is with SERIAL defined.
 * 2)bpcp could continue immediately after each transfer
 *  is queued. This is with SERIAL undefined.
 * It isn't clear to me which mode is more desirable.
 * Mode 1 will result in a clearer progress display (all
 * data will be at receiver at end of program+RTT). However,
 * Mode 1 will also probably result in more small LTP
 * sessions because the LTP session must be sent before
 * each transfer is considered sent.*/
#define SERIAL 1

/*Define to enable SIGTERM and SIGINT handlers (cleanup temp files and
 * semaphores)*/
#define SIG_HANDLER 1

/*Currently CFDP's directory operations does not remove
 * temporary files. Define this in order to remove those files
 * when bpcpd exits. This provides a crappy work-around for
 * CFDP's brain damage by executing an "rm dirlist_*" on bpcpd's exit.
 * THIS WILL NOT WORK ON VXWORKS!!!!*/
#define CLEAN_ON_EXIT 1

/*Max level of file system recursion*/
#define	NUM_TMP_FILES 	128


/*Structure containing all of the Parameters
 * needed for a CFDP request*/
typedef struct
{
	CfdpHandler		faultHandlers[16];			/*CFDP fault handlers*/
	CfdpNumber		destinationEntityNbr;		/*Destination Node Number*/
	char			sourceFileNameBuf[256];		/*Buffer for name of source file*/
	char			*sourceFileName;			/*Source filename*/
	char			destFileNameBuf[256];		/*Buffer for name of destination file*/
	char			*destFileName;				/*Destination filename*/
	BpUtParms		utParms;					/*BP Parameters for this transfer*/
	MetadataList		msgsToUser;				/*Message to User Queue for this transfer*/
	MetadataList		fsRequests;				/*File System requests for this transfer*/
	CfdpTransactionId	transactionId;			/*ID for this CFDP transaction*/
	CfdpProxyTask 		proxytask;				/*Information for CFDP proxy request, if needed*/
} CfdpReqParms;

/*Enum for type of transfer*/
enum mv_type
{
	Local_Local,		/*Local to Local Transfer*/
	Local_Remote,		/*Local to Remote Transfer*/
	Remote_Local,		/*Remote to Local Transfer*/
	Remote_Remote,		/*Remote to Remote Transfer*/
};

/*Enum for remote directory status*/
enum wait_status
{
	no_req,			/*No Directory Listing Request Pending*/
	dir_req,		/*Directory Listing Request Pending*/
	dir_exists,		/*Result Status: Directory Exists*/
	nodir,			/*Result Status: No Such Directory*/
	snd_wait,		/*Waiting for a file to be sent*/
	sent,			/*Result Status: File sent*/
};

/*Message to user object*/
typedef struct
{
	Object			text;		/*Text of Message*/
	unsigned char	length;		/*Message Length*/
} MsgToUser;

/*Transfer Information*/
struct transfer{
	enum mv_type 	type;			/*Type of Transfer*/
	char			dhost[256];		/*Destination Hostname*/
	char			dfile[256];		/*Destination Filename*/
	char			shost[256];		/*Source Hostname*/
	char			sfile[256];		/*Source Filename*/
};


typedef struct
{
	char		directoryName[256];
	char		directoryDestFileName[256];
	int			directoryListingResponseCode;
} CfdpDirListingResponse;

/*Portability for directory structures*/
#ifndef _D_EXACT_NAMLEN
#define _D_EXACT_NAMLEN(d) (strlen ((d)->d_name))
#endif

/*Solaris include for working terminal width detection*/
#if defined(solaris)
#include <termios.h>
#endif
