/*
	winion.c:	background ION handle holder for Windows.

	Author: Scott Burleigh, JPL

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ion.h"
#include <signal.h>

#define	MAX_SM_SEGMENTS	100
#define	MAX_ION_IPCS	(SEMMNS + MAX_SM_SEGMENTS)

typedef enum
{
	IonIpcNone = 0,
	IonSmSegment,
	IonSemaphore
} IonIpcType;

typedef struct
{
	IonIpcType	type;
	int		key;
	HANDLE		handle;
} IonIpc;

typedef struct
{
	IonIpc		ipcs[MAX_ION_IPCS];
	int		ipcsCount;
} IonIpcTable;

static int	scanIpcs(IonIpcTable *ipcTbl, IonIpcType type, DWORD key,
			int *match, int *firstOpening)
{
	int	i;
	IonIpc	*ipc;
	int	count = 0;

	if (match)
	{
		*match = -1;		/*	Default.		*/
	}

	if (firstOpening)
	{
		*firstOpening = -1;	/*	Default.		*/
	}

	for (ipc = ipcTbl->ipcs, i = 0; i < ipcTbl->ipcsCount; ipc++, i++)
	{
		if (match == NULL)	/*	Just counting.		*/
		{
			if (ipc->type != IonIpcNone)
			{
				count++;
			}

			continue;
		}

		/*	Looking for matching IPC.			*/

		if (ipc->type == type && ipc->key == key)
		{
			*match = i;
			return 0;	/*	Found match.		*/
		}

		/*	Not a match.  Looking for an empty table slot?	*/

		if (firstOpening == NULL)	/*	Nope.		*/
		{
			continue;
		}

		/*	If this slot is empty and no other empty slot
		 *	has been noted yet, note this one.		*/

		if (ipc->type == IonIpcNone)
		{
			if (*firstOpening == -1)
			{
				*firstOpening = i;
			}
		}
	}

	/*	Now pointing past the last IPC in the table.  If table
	 *	is not full, and we are looking for an empty table
	 *	slot and have not yet noted one, note this one.		*/

	if (i < MAX_ION_IPCS)
	{
		if (firstOpening != NULL && *firstOpening == -1)
		{
			*firstOpening = i;
		}
	}

	return count;
}

static int	noteSmSegmentIpc(IonIpcTable *ipcTbl, int key)
{
	int	match;
	int	firstOpening;
	IonIpc	*ipc;
	char	memName[32];

	oK(scanIpcs(ipcTbl, IonSmSegment, key, &match, &firstOpening));
	if (match >= 0)		/*	Found it.			*/
	{
		return 0;	/*	SM segment already noted.	*/
	}

	if (firstOpening < 0)	/*	No room for new IPC.		*/
	{
		return -1;
	}

	ipc = ipcTbl->ipcs + firstOpening;
	sprintf(memName, "%d.mmap", key);
	ipc->handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memName);
	if (ipc->handle == NULL)	/*	Not found.		*/
	{
		printf("Can't find memory segment: %u.\n",
				(unsigned int) GetLastError());
		return -1;
	}

	ipc->type = IonSmSegment;
	ipc->key = key;
	if (firstOpening == ipcTbl->ipcsCount)
	{
		ipcTbl->ipcsCount++;
	}

	return 0;
}

static int	noteSemaphoreIpc(IonIpcTable *ipcTbl, int key)
{
	int	match;
	int	firstOpening;
	IonIpc	*ipc;
	char	semaphoreName[32];

	oK(scanIpcs(ipcTbl, IonSmSegment, key, &match, &firstOpening));
	if (match >= 0)		/*	Found it.			*/
	{
		return 0;	/*	Semaphore already noted.	*/
	}

	if (firstOpening < 0)	/*	No room for new IPC.		*/
	{
		return -1;
	}

	ipc = ipcTbl->ipcs + firstOpening;
	sprintf(semaphoreName, "%d.event", key);
	ipc->handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, semaphoreName);
	if (ipc->handle == NULL)	/*	Not found.		*/
	{
		printf("Can't find semaphore %u.\n",
				(unsigned int) GetLastError());
		return -1;
	}

	ipc->type = IonSemaphore;
	ipc->key = key;
	if (firstOpening == ipcTbl->ipcsCount)
	{
		ipcTbl->ipcsCount++;
	}

	return 0;
}

static void	forgetIpc(IonIpcTable *ipcTbl, IonIpcType type, int key)
{
	int	match;
	IonIpc	*ipc;

	oK(scanIpcs(ipcTbl, type, key, &match, NULL));
	if (match < 0)		/*	Didn't find the IPC.		*/
	{
		puts("Can't detach from IPC.");
		return;
	}

	ipc = ipcTbl->ipcs + match;
	CloseHandle(ipc->handle);
	ipc->type = IonIpcNone;
}

int	main(int argc, char *argv[])
{
	IonIpcTable	ipcs;
	int		ipcsInUse;
	HANDLE		hPipe = INVALID_HANDLE_VALUE;
	int		ionRunning = 1;
 	BOOL		fConnected = FALSE;
	char		msg[1 + sizeof(DWORD)];
	DWORD		bytesRead;
 	BOOL		fSuccess = FALSE;
	DWORD		key;
	char		reply[1];
	DWORD		bytesWritten;
 
	hPipe = CreateNamedPipe("\\\\.\\pipe\\ion.pipe", PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			1, sizeof reply, sizeof msg, 0, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) 
	{
		printf("winion failed creating pipe, error code %u.\n",
				(unsigned int) GetLastError()); 
		return 0;
	}

	memset((char *) &ipcs, 0, sizeof(IonIpcTable));
	signal(SIGINT, SIG_IGN);
	while (ionRunning)
	{
		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE
				: (GetLastError() == ERROR_PIPE_CONNECTED); 
		if (!fConnected) 
		{
			printf("winion failed on connect, error code %u.\n",
					(unsigned int) GetLastError()); 
			break;
		}

		/*	Read one request to track an IPC.		*/

		fSuccess = ReadFile(hPipe, msg, sizeof msg, &bytesRead, NULL);
		if (!fSuccess || bytesRead == 0)
		{   
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				CloseHandle(hPipe);
				continue;
			}

			printf("winion failed reading msg, error code %u.\n",
					(unsigned int) GetLastError()); 
			break;
		}

		/*	Parse the message, track the IPC.		*/

		memcpy((char *) &key, msg + 1, sizeof(DWORD));
		reply[0] = 1;		/*	Default: success.	*/
		switch (msg[0])
		{
		case WIN_STOP_ION:
			ionRunning = 0;
			continue;

		case WIN_NOTE_SM:
			if (noteSmSegmentIpc(&ipcs, key) < 0)
			{
				reply[0] = 0;		/*	Fail.	*/
			}

			break;

		case WIN_NOTE_SEMAPHORE:
			if (noteSemaphoreIpc(&ipcs, key) < 0)
			{
				reply[0] = 0;		/*	Fail.	*/
			}

			break;

		case WIN_FORGET_SM:
			forgetIpc(&ipcs, IonSmSegment, key);
			break;

		case WIN_FORGET_SEMAPHORE:
			forgetIpc(&ipcs, IonSemaphore, key);
			break;

		case '?':
			ipcsInUse = scanIpcs(&ipcs, IonIpcNone, 0, NULL, NULL);
			printf("winion retaining %d IPCs.\n", ipcsInUse);
			reply[0] = 0;			/*	Dummy.	*/
			break;

		default:
			printf("Invalid message type to winion: %d.\n", msg[0]);
			reply[0] = 0;			/*	Fail.	*/
		}

		/*	Tell the client to continue.			*/
 
		fSuccess = WriteFile(hPipe, reply, sizeof reply, &bytesWritten,
				NULL);
		if (!fSuccess || bytesWritten != sizeof reply)
		{
			printf("winion failed writing reply, error code %u.\n",
					(unsigned int) GetLastError()); 
			break;
		}

		/*	Now disconnect pipe so it can be reconnected.	*/

		FlushFileBuffers(hPipe); 
		DisconnectNamedPipe(hPipe); 
	}
 
	/*	Disconnect pipe and terminate.				*/

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 

	/*	Termination of process closes all handles.		*/

	return 0;
}
