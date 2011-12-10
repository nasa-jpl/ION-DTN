/*
 *	bssrecv.c:	a test application that demonstrates the 
 *			functionality of BSS API. It receives 
 *			a stream of bundles and display its contents.
 *								
 *					
 *	Copyright (c) 2011, Space Internetworking Center,
 *	Democritus University of Thrace.
 *	Copyright (c) 2011, California Institute of Technology.	
 *
 *	All rights reserved.						
 *	
 *	Author: Sotirios-Angelos Lenas, Space Internetworking Center (SPICE)
 */

#include "bss.h"

static char*	_threadBuf(char *newValue)
{
	static char*	buffer = NULL;

	if (newValue)
	{
		buffer = newValue;
	}

	return buffer;
}

/* implementation of the callback function */

static int display(time_t time, unsigned long count, char* buf, int bufLength)
{

	/*
	 * This function is called either by the receiving thread or the BSS
 	 * receiving application. It converts buffer contents to an integer
 	 * and based on that value a number of repetitions is defined and is
 	 * displayed as asterisks on screen.
 	 */
	if (atoi(buf) == -1)
	{
		PUTS("#######ERROR########");
		return 0;
	}

	int 	reps = atoi(buf)%150; /* maximum repetitions number of each sequence */
	while (reps!=0)
	{
		putchar('*');
		reps--;
	}
	PUTS(buf);
	fflush(stdout);
	return 0;
}

static int replay (time_t fromTime, time_t toTime)
{
	time_t		curTime = 0;
	bssNav		nav;
	char*		data;
	int 		bytesRead;
	int 		pLen;
	unsigned long 	count;

	data = calloc(65536, sizeof(char));

	if (fromTime==0 || toTime==0)
	{
		PUTS("from h to == 0");
		return -1;
	}

	PUTS("----------demonstrating bssNext functionality------------");
	PUTS("---------------------------------------------------------");
	fflush(stdout);

	if(bssSeek(&nav, fromTime, &curTime, &count) < 0)
	{
		PUTS("bssSeek failed");
		return -1;
	}
	
	while(curTime < toTime)
	{
		memset(data, '\0', 65536);
		bytesRead = bssRead(nav, data, 65536);
		if(bytesRead == -1)
		{
			PUTS("bssRead failed");
			fflush(stdout);
			return -1;
		}
		/*	call the display function	*/
		display(curTime, count, data, sizeof(data));

		/*	get next frame	    */
		pLen = bssNext(&nav, &curTime, &count);
		if(pLen == -2)
		{
			PUTS("End of list");
			break;
		}
		else if (pLen < 0)
		{
			PUTS("bssNext failed");
			return -1;
		}
		microsnooze(12800);
	}
	
	PUTS("\n");

	PUTS("----------demonstrating bssPrev functionality------------");
	PUTS("---------------------------------------------------------");
	fflush(stdout);

	if(bssSeek(&nav, toTime, &curTime, &count) < 0)
	{
		PUTS("bssSeek failed");
		return -1;
	}
	
	while(curTime >= fromTime)
	{
		/*	get previous frame	*/
		pLen = bssPrev(&nav, &curTime, &count);
		if(pLen == -2)
		{
			PUTS("End of list");
			break;
		}
		else if (pLen < 0)
		{
			PUTS("bssPrev failed");
			return -1;
		}
		
		memset(data, '\0', 65536);
		bytesRead = bssRead(nav, data, 65536);
		if(bytesRead == -1)
		{
			PUTS("bssRead failed");
			fflush(stdout);
			return -1;
		}
		/*	call the display function	*/
		display(curTime, count, data, sizeof(data));
		
		microsnooze(12800);
	}

	free(data);
	return 0;
}

static int userInput(int fd, char* bssName, char* path, char* eid )
{
	char	parameters[512];
	int	paramLen;

	PUTS("Please enter DB name, path and eid separated by whitespace.");
	PUTS("e.g.: bssDB /home/user/experiments/bss ipn:2.71");
	fflush(stdout);

	if(igets(fd, parameters, sizeof parameters, &paramLen) == NULL)
	{
   		PUTS("Error in reading arguments");
		return -1;
  	}
	
	if(sscanf (parameters, "%s %s %s", bssName, path, eid) != 3)
	{
		PUTS("Wrong number of arguments");
		return -1;
	}

	return 0;
}

void handleQuit(int sig)
{
	bssExit();
	free(_threadBuf(NULL));
	exit(1);
}

#ifdef VXWORKS
int	bssrecv(int a1, int a2, int a3, int a4, int a5,
		int a6, int a7, int a8, int a9, int a10)
{
#else
int	main(int argc, char **argv)
{
#endif
	int 		choice;
	char		parameters[512];
	char		bssName[64];
	char		path[256];
	char		eid[32];
	char		toTime[TIMESTAMPBUFSZ];
	char		fromTime[TIMESTAMPBUFSZ];
	int		paramLen;
	int		cmdFile = fileno(stdin);
	int		recvLength = 65536;
	time_t		from = 0;
	time_t		to = 0;
	time_t		refTime = 0;
	char*		buffer;

	/*
         * ********************************************************
	 * In order for the BSS receiving thread to work properly
	 * BSS receiving application must always allocate a buffer
	 * of a certain size and provide its address and its length 
	 * to bssRun or bssStart function.
         * ********************************************************
	 */ 
	buffer = calloc(recvLength, sizeof(char));
	_threadBuf(buffer);
	
	isignal(SIGINT, handleQuit);
	isignal(SIGTERM, handleQuit);
	
	/* 
	 * Each BSS receiving application supports two modes of operation,
	 * the real-time and the playback mode. In real-time mode (bssStart()), 
	 * BSS receiver starts a thread, enabling the reception of a bundle  
	 * stream, and creates a database in which it stores the received frames.  
	 * In playback mode (bssOpen()), BSS receiver is able to replay the last 
	 * SOD seconds of a stream which is stored in an already existing database. 
	 * The simultaneous operation of both, real-time and playback mode, in a
	 * BSS receiving application is also supported (bssRun()). 
	 */

	do 
	{
		PUTS("\n");
		PUTS("---------------Menu-----------------");
		PUTS("1. Open BSS Receiver in playback mode");
		PUTS("2. Start BSS receiving thread");
		PUTS("3. Start BSS Receiver");
		PUTS("4. Close current playback session");
		PUTS("5. Stop BSS receiving thread");
		PUTS("6. Stop BSS Receiver");
		PUTS("7. Replay session");
		PUTS("8. Exit");

		if(igets(cmdFile, parameters, sizeof parameters, &paramLen) == NULL)
		{
   			printf("Error in reading choice");
			continue;
  		}
      				
		if(sscanf (parameters, "%d", &choice) != 1)
		{
			PUTS("Invalid choice!");
			continue;
		}
		PUTS("");
		switch(choice)
		{
			case 1: 
				
				if(userInput(cmdFile, bssName, path, eid)<0)
				{
					break;
				}
				bssOpen(bssName, path, eid);
				break;

			case 2:		
				if(userInput(cmdFile, bssName, path, eid)<0)
				{
					break;
				}
				bssStart(bssName, path, eid, _threadBuf(NULL), 
					recvLength*sizeof(char), display);
				break;

			case 3: 
				if(userInput(cmdFile, bssName, path, eid)<0)
				{
					break;
				}
				bssRun(bssName, path, eid, _threadBuf(NULL), 
					recvLength*sizeof(char), display);
				break;
			case 4: 
				PUTS("Closing current playback session...\n");
				bssClose();
				break;
				
			case 5: 
				PUTS("Stopping receiving thread...\n");
				bssStop();
				break;
			case 6: 
				PUTS("Exiting BSS receiver app...\n");
				bssExit();
				break;
			case 7: PUTS("Pls provide replay period: fromTime toTime ");
				PUTS("fromTime and toTime format: yyyy/mm/dd-hh:mm:ss");
				if(igets(cmdFile, parameters, sizeof(parameters), 
					&paramLen) == NULL)
				{
   					PUTS("Error in reading arguments");
					break;
  				}
      				
				if(sscanf (parameters, "%s %s", fromTime, toTime) != 2)
				{
					PUTS("Wrong number of arguments");
					break;
				}

				from = readTimestampLocal(fromTime, refTime) - EPOCH_2000_SEC;
				to = readTimestampLocal(toTime, refTime) - EPOCH_2000_SEC;
				
				/*	call the replay function      */
				if(replay(from, to) == -1)
				{
					PUTS("replay failed");
				}

				break;
			case 8: PUTS("Quitting program!\n");
				bssExit();
				free(_threadBuf(NULL));
				break;
			default: printf("Invalid choice!\n");
				break;
		}
	} while (choice != 8);
	return 0;
}
