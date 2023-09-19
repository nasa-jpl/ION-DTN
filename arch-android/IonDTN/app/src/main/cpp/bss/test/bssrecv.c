/*
 *	bssrecv.c:	A test application that demonstrates the 
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

#include "bsstest.h"

static char*	_threadBuf(char *newValue)
{
	static char*	buffer = NULL;

	if (newValue)
	{
		buffer = newValue;
	}

	return buffer;
}

/* Implementation of the callback function */

static int display(time_t sec, unsigned long count, char* buf,
		unsigned long bufLength)
{
	int 	reps;

	/*
	 * This function is called either by the receiving thread or the BSS
 	 * receiving application. It converts buffer contents to an integer
 	 * and based on that value a number of repetitions is defined and is
 	 * displayed as asterisks on screen.
 	 */
	if (atoi(buf) == -1)
	{
		PUTS("#######ERROR########");
		return -1;
	}

	reps = atoi(buf)%150; /* Maximum repetitions number of each sequence */
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
	long 		bytesRead;
	long 		result;
	unsigned long 	count;

	data = calloc(RCV_LENGTH, sizeof(char));

	if (data == NULL)
	{
		PUTS("Memory allocation error");
		return -1;
	}

	if (fromTime < 0 || toTime <= fromTime || toTime <= 0)
	{
		PUTS("wrong fromTime/toTime arguments");
		free(data);
		return -1;
	}

	PUTS("----------Demonstrating bssNext() functionality------------");
	PUTS("-----------------------------------------------------------");
	fflush(stdout);

	if(bssSeek(&nav, fromTime, &curTime, &count) < 0)
	{
		PUTS("bssSeek failed");
		free(data);
		return -1;
	}
	
	while(curTime < toTime)
	{
		memset(data, '\0', RCV_LENGTH);
		bytesRead = bssRead(nav, data, RCV_LENGTH);
		if(bytesRead == -1)
		{
			PUTS("bssRead failed");
			fflush(stdout);
			free(data);
			return -1;
		}

		/*	Call the display function	*/

		oK(display(curTime, count, data, bytesRead));

		/*	Get next frame	    */
		result = bssNext(&nav, &curTime, &count);
		if(result == -2)
		{
			PUTS("End of list");
			break;
		}
		else if (result < 0)
		{
			PUTS("bssNext failed");
			free(data);
			return -1;
		}
		microsnooze(SNOOZE_INTERVAL);
	}
	
	PUTS("\n");

	PUTS("----------Demonstrating bssPrev() functionality------------");
	PUTS("-----------------------------------------------------------");
	fflush(stdout);

	if(bssSeek(&nav, toTime, &curTime, &count) < 0)
	{
		PUTS("bssSeek failed");
		free(data);
		return -1;
	}
	
	while(curTime >= fromTime)
	{
		/*	Get previous frame	*/
		result = bssPrev(&nav, &curTime, &count);
		if(result == -2)
		{
			PUTS("Start of list");
			break;
		}
		else if (result < 0)
		{
			PUTS("bssPrev failed");
			free(data);
			return -1;
		}
		
		memset(data, 0, RCV_LENGTH);
		bytesRead = bssRead(nav, data, RCV_LENGTH);
		if(bytesRead == -1)
		{
			PUTS("bssRead failed");
			fflush(stdout);
			free(data);
			return -1;
		}

		/*	Call the display function	*/

		oK(display(curTime, count, data, bytesRead));
		microsnooze(SNOOZE_INTERVAL);
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
	if (igets(fd, parameters, sizeof parameters, &paramLen) == NULL)
	{
   		PUTS("Error in reading arguments");
		return -1;
  	}
	
	if (sscanf(parameters, "%63s %255s %31s", bssName, path, eid) != 3)
	{
		PUTS("Wrong number of arguments");
		return -1;
	}

	return 0;
}

static void handleQuit(int sig)
{
	bssExit();
	if (_threadBuf(NULL)!=NULL)
	{
		free(_threadBuf(NULL));
	}

	exit(BSSRECV_EXIT_SUCCESS);
}

#if defined (ION_LWT)
int	bssrecv(saddr a1, saddr a2, saddr a3, saddr a4, saddr a5,
		saddr a6, saddr a7, saddr a8, saddr a9, saddr a10)
{
	char	menuNav[512];
	int	navLen;
	int 	choice = atoi((char *) a1);
	char	*aBssName = (char *) a2;
	char	*aPath = (char *) a3;
	char	*aEid = (char *) a4;
	char	*aToTime = (char *) a5;
	char	*aFromTime = (char *) a6;
	char	bssName[64];
	char	path[256];
	char	eid[32];
	char	fromTime[TIMESTAMPBUFSZ];
	char	toTime[TIMESTAMPBUFSZ];
	int	cmdFile = fileno(stdin);
	time_t	from = 0;
	time_t	to = 0;
	time_t	refTime = 0;
	char	*buffer;
	int	reqArgs = 0;		/*	Boolean		*/
	
	
#else
int	main(int argc, char **argv)
{

	char	menuNav[512];
	int	navLen;
	int 	choice=0;
	char	*aBssName = NULL;
	char	*aPath = NULL;
	char	*aEid = NULL;
	char	*aToTime = NULL;
	char	*aFromTime = NULL;
	char	bssName[64];
	char	path[256];
	char	eid[32];
	char	fromTime[TIMESTAMPBUFSZ];
	char	toTime[TIMESTAMPBUFSZ];
	int	cmdFile = fileno(stdin);
	time_t	from = 0;
	time_t	to = 0;
	time_t	refTime = 0;
	char	*buffer;
	int	reqArgs = 0;		/*	Boolean		*/

	if (argc > 7) argc = 7;
	switch (argc)
	{
		case 7:
			aToTime = argv[6];
		case 6:
			aFromTime = argv[5];
		case 5:
			aEid = argv[4];
		case 4:
			aPath = argv[3];
		case 3:
			aBssName = argv[2];
		case 2:
			choice = strtol(argv[1], NULL, 0);
		default:
			break;
	}
#endif
	/*
         * ********************************************************
	 * In order for the BSS receiving thread to work properly
	 * BSS receiving application must always allocate a buffer
	 * of a certain size and provide its address and its lenght 
	 * to bssRun or bssStart function.
         * ********************************************************
	 */ 
	buffer = calloc(RCV_LENGTH, sizeof(char));

	if (buffer == NULL)
	{
		PUTS("Memory allocation error - bssrecv will exit");
		exit(BSSRECV_EXIT_ERROR);
	}

	oK(_threadBuf(buffer));
	
	isignal(SIGINT, handleQuit);
	isignal(SIGTERM, handleQuit);
	
	/* 
	 * Each BSS receiving application supports two modes of operation,
	 * the real-time and the playback mode. In real-time mode (bssStart()), 
	 * BSS receiver starts a thread, enabling the reception of a bundle  
	 * stream, and creates a database in which it stores the received
	 * frames.  In playback mode (bssOpen()), BSS receiver is able to
	 * replay the last WINDOW seconds of a stream which is stored in an
	 * already existing database.  The simultaneous operation of both
	 * real-time and playback mode in a BSS receiving application is
	 * also supported (bssRun()). 
	 */

	do 
	{
		if(choice==0)
		{
			PUTS("\n");
			PUTS("---------------Menu-----------------");
			PUTS("1. Open BSS Receiver in playback mode");
			PUTS("2. Start BSS receiving thread");
			PUTS("3. Start BSS Receiver");
			PUTS("4. Close current playback session");
			PUTS("5. Stop BSS receiving thread");
			PUTS("6. Stop BSS Receiver");
			PUTS("7. Exit");

			if(igets(cmdFile, menuNav, sizeof menuNav, &navLen) == NULL)
			{
   				PUTS("Error in reading choice");
				continue;
  			}

			if(sscanf (menuNav, "%d", &choice) != 1)
			{
				PUTS("Invalid choice!");
				continue;
			}
		}
		PUTS("");
		switch(choice)
		{
			case 1:  
				if (aBssName == NULL || aPath == NULL || aEid == NULL)
				{										
					if(reqArgs == 0)
					{					
						if(userInput(cmdFile, bssName, path, eid) < 0)
						{
							break;
						}
						reqArgs = 1;
					}
					if(bssOpen(bssName, path) == -1)
					{
						PUTS("bssOpen failed");
						choice=0;
						reqArgs = 0;
						break;
					}	
				}
				else
				{
					if(bssOpen(aBssName, aPath) == -1)
					{
						PUTS("bssOpen failed");
						choice=0;
						break;
					}
				}

				if (aFromTime == NULL || aToTime == NULL)
				{			
					PUTS("Pls provide replay period: fromTime toTime ");
					PUTS("fromTime and toTime format: yyyy/mm/dd-hh:mm:ss");
					if (igets(cmdFile, menuNav, sizeof(menuNav), &navLen) == NULL)
					{
   						PUTS("Error in reading arguments");
						break;
  					}
      				
					if (sscanf(menuNav, "%19s %19s", fromTime, toTime) != 2)
					{
						PUTS("Wrong number of arguments");
						break;
					}

					from = readTimestampLocal(fromTime, refTime) - EPOCH_2000_SEC;
					if (from < 0) from = 0;
					to = readTimestampLocal(toTime, refTime) - EPOCH_2000_SEC;
				}
				else
				{
					from = readTimestampLocal(aFromTime, refTime) - EPOCH_2000_SEC;
					if (from < 0) from = 0;
					to = readTimestampLocal(aToTime, refTime) - EPOCH_2000_SEC;
				}
				
				/*	Call the replay function      */
				if(replay(from, to) == -1)
				{
					PUTS("Replay failed");
				}
				aFromTime = NULL;
				aToTime = NULL;
				choice=0;
				break;

			case 2:	
				if (aBssName == NULL || aPath == NULL || aEid == NULL)
				{										
					if(userInput(cmdFile, bssName, path, eid) < 0)
					{
						break;
					}
					if(bssStart(bssName, path, eid,_threadBuf(NULL), 
					   RCV_LENGTH*sizeof(char), display) == -1)
					{
						PUTS("bssStart failed");
					}	
				}
				else
				{
					if(bssStart(aBssName, aPath, aEid, _threadBuf(NULL), 
					   RCV_LENGTH*sizeof(char), display) == -1)
					{
						PUTS("bssStart failed");
					}
				}
				choice=0;
				break;

			case 3: 
				if (aBssName == NULL || aPath == NULL || aEid == NULL)
				{
					if(userInput(cmdFile, bssName, path, eid) < 0)
					{
						break;
					}
					if(bssRun(bssName, path, eid, _threadBuf(NULL), 
					   RCV_LENGTH*sizeof(char), display) == -1)
					{
						PUTS("bssRun failed");
					}
					
				}
				else
				{
					if(bssRun(aBssName, aPath, aEid, _threadBuf(NULL), 
					   RCV_LENGTH*sizeof(char), display) == -1)
					{
						PUTS("bssRun failed");
					}
				}
				choice=0;
				break;

			case 4: 
				PUTS("Closing current playback session...\n");
				oK(bssClose());
				aBssName=NULL;
				aPath=NULL;
				aEid=NULL;
				reqArgs=0;
				choice=0;				
				break;
				
			case 5: 
				PUTS("Stopping receiving thread...\n");
				oK(bssStop());
				aBssName=NULL;
				aPath=NULL;
				aEid=NULL;
				choice=0;
				break;

			case 6: 
				oK(bssExit());
				aBssName=NULL;
				aPath=NULL;
				aEid=NULL;
				reqArgs=0;
				choice=0;
				break;

			case 7: 
                		PUTS("Quitting program!\n");
				break;

			default:
                		PUTS("Invalid choice!\n");
				break;
		}
	} while (choice != 7);

	oK(bssExit());
	if (_threadBuf(NULL)!=NULL)
	{
		free(_threadBuf(NULL));
	}

	return 0;
}
