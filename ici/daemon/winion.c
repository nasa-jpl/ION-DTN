/*
	winion.c:	background ION handle holder for Windows.

	Author: Scott Burleigh, JPL

	Copyright (c) 2011, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.
									*/
#include "ion.h"
#include <signal.h>

#define	MAX_ION_IPCS	200

typedef enum
{
	IonSmSegment = 1,
	IonSemaphore
} IonIpcType;

typedef struct
{
	IonIpcType	type;
	int		key;
	HANDLE		handle;
} IonIpc;

static int	noteSmSegmentIpc(IonIpc *ipc, int key)
{
	char	memName[32];

	sprintf(memName, "%d.mmap", key);
	ipc->handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memName);
	if (ipc->handle == NULL)	/*	Not found.		*/
	{
		printf("Can't find memory segment: %u.\n",
				(unsigned int) GetLastError());
		return -1;
	}

	return 0;
}

static int	noteSemaphoreIpc(IonIpc *ipc, int key)
{
	char	semaphoreName[32];

	sprintf(semaphoreName, "%d.event", key);
	ipc->handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, semaphoreName);
	if (ipc->handle == NULL)	/*	Not found.		*/
	{
		printf("Can't find semaphore %u.\n",
				(unsigned int) GetLastError());
		return -1;
	}

	return 0;
}

static int	noteIpc(IonIpc *ipc, IonIpcType type, int key)
{
	int	result;

	if (type == IonSmSegment)
	{
		result = noteSmSegmentIpc(ipc, key);
	}
	else
	{
		result = noteSemaphoreIpc(ipc, key);
	}

	if (result == 0)
	{
		ipc->type = type;
		ipc->key = key;
	}

	return result;
}

int	main(int argc, char *argv[])
{
	IonIpc		ipcs[MAX_ION_IPCS];
	int		ipcsCount = 0;
	HANDLE		hPipe = INVALID_HANDLE_VALUE;
	int		ionRunning = 1;
 	BOOL		fConnected = FALSE;
	char		msg[5];
	DWORD		bytesRead;
 	BOOL		fSuccess = FALSE;
	IonIpcType	type;
	DWORD		key;
	int		i;
	char		reply[1] = { '\0' };
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

	memset((char *) ipcs, 0, sizeof ipcs);
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

		/*	Read one request to retain a handle.		*/

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

		/*	Parse the message, open and retain the handle.	*/

		reply[0] = 1;				/*	Okay.	*/
		switch (msg[0])
		{
		case 0:					/*	Stop.	*/
			ionRunning = 0;
			continue;

		case 1:
			type = IonSmSegment;
			break;

		case 2:
			type = IonSemaphore;
			break;

		case '?':
			printf("winion retaining %d IPCs.\n", ipcsCount);
			reply[0] = 0;			/*	Dummy.	*/
			break;

		default:
			reply[0] = 0;			/*	Fail.	*/
		}

		if (reply[0])	/*	Valid IPC type.			*/
		{
			memcpy((char *) &key, msg + 1, sizeof(DWORD));
			for (i = 0; i < ipcsCount; i++)
			{
				if (ipcs[i].type == type && ipcs[i].key == key)
				{
					break;
				}
			}

			if (i == ipcsCount)	/*	New IPC.	*/
			{
				if (i == MAX_ION_IPCS)
				{
					reply[0] = 0;	/*	Fail.	*/
				}
				else
				{
					if (noteIpc(&ipcs[i], type, (int) key))
					{
						reply[0] = 0;
					}
					else
					{
						ipcsCount++;
					}
				}
			}
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
