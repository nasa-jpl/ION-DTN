/*
eclso.c

 Author: Nicola Alessi (nicola.alessi@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

Copyright (c) 2016, Alma Mater Studiorum, University of Bologna
 All rights reserved.

This file implements eclso, the Forward Error Correction Link Service
Outduct daemon

 * */
#include "eclso.h"
#include <pthread.h>	// POSIX threads
#include <string.h>		// memset
#include <stdlib.h> //rand, srand , ..

#define PARSE_PARAMETER(PAR_NO) 	(argc > (PAR_NO) ? (int) strtoll(argv[PAR_NO], NULL, 10) : -1)

int	main(int argc, char *argv[])
{
	/*declarations*/
	EclsoEnvironment 	 eclsoEnv;
	EclsoThreadParms	 thread;

	/* Thread declarations*/
	pthread_t			encodeMatrixThread;
	pthread_t			sendMatrixThread;
	pthread_t			feedbackHandlerThread;

	/*Logger*/
	loggerInit(1);
	loggerStartLog(0,"outputECLSO.log",true,true); //can be used debugPrint() shortcut instead of loggerPrint()

	parseCommandLineArgument(argc,argv,&eclsoEnv);

	initEclsoUpperLevel(argc,argv,&eclsoEnv.T,&eclsoEnv.ownEngineId,&eclsoEnv.portNbr,&eclsoEnv.ipAddress);
	initEclsoLowerLevel(argc,argv,eclsoEnv.portNbr,eclsoEnv.ipAddress,eclsoEnv.txbps);

	/* randomizing matrix id */
	if(!eclsoEnv.staticMidEnabled)
	{
		srand(time(NULL));
		eclsoEnv.globalMatrixID= rand() % (256 * sizeof(unsigned long));
		debugPrint("randomized last matrix id %d", eclsoEnv.globalMatrixID);
	}

	debugPrint("N: %d K: %d T: %d maxAggregationTime: %d seconds txbps:  %d codingThreshold: %d \nadaptiveCoding: %s feedbackRequest: %s feedbackAdaptive: %s interleaving: %s puncturing: %s randomizeMid: %s continuousMode: %s \nengineID %u",
			eclsoEnv.N,eclsoEnv.K,eclsoEnv.T,eclsoEnv.maxAggregationTime,eclsoEnv.txbps,
			eclsoEnv.codingThreshold,
			eclsoEnv.adaptiveModeEnabled ? "Enabled" : "Disabled",
			eclsoEnv.feedbackRequestEnabled ? "Enabled" : "Disabled",
			eclsoEnv.feedbackAdaptiveRcEnabled ? "Enabled" : "Disabled",
			eclsoEnv.interleavingEnabled ? "Enabled" : "Disabled",
			eclsoEnv.puncturingEnabled ? "Enabled" : "Disabled",
			eclsoEnv.staticMidEnabled ? "Enabled" : "Disabled",
			eclsoEnv.continuousModeEnabled ? "Enabled" : "Disabled",
			eclsoEnv.ownEngineId);


	/* INITIALIZATIONS */

	/*FEC*/
	fecManagerInit(eclsoEnv.adaptiveModeEnabled,eclsoEnv.feedbackAdaptiveRcEnabled,eclsoEnv.continuousModeEnabled,eclsoEnv.K,eclsoEnv.N,eclsoEnv.T);
	matrixBufferInit(FEC_ECLSO_MATRIX_BUFFER);
	timerInit(timerHandler,&thread);

	/* threads init */
	thread.eclsoEnv= &eclsoEnv;
	thread.running=true;

	sem_init(&(thread.notifyT1),0,0);
	sem_init(&(thread.notifyT2),0,0);
	sem_init(&(thread.notifyT3),0,0);

	//	Starting thread.

	if (pthread_create(&encodeMatrixThread, NULL, encode_matrix_thread, &thread))
	{
		debugPrint("ERROR: eclso can't create the encode matrix thread");
		exit(1);
	}
	if (pthread_create(&sendMatrixThread, NULL, pass_matrix_thread, &thread))
	{
		debugPrint("ERROR: eclso can't create the send matrix thread");
		exit(1);
	}

	if (eclsoEnv.feedbackRequestEnabled && pthread_create(&feedbackHandlerThread, NULL, feedback_handler_thread, &thread))
	{
		debugPrint("ERROR: eclso can't create the feedback handler thread");
		exit(1);
	}
	/* initializations ended */

	//Once entered the following function, the program should never return until active.
	if(eclsoEnv.HSLTP_mode)
		fill_matrix_thread_HSLTP_MODE(&thread);
	else
	 	fill_matrix_thread(&thread);


	//We can be here only if an error has occurred in fill_matrix_thread;
	//hence, we must wait for the termination of the other threads (join),
	//then we have to free the memory and exit.

	debugPrint("T1: join with T2");
	pthread_join(encodeMatrixThread,NULL);
	debugPrint("T1: join with T3");
	pthread_join(sendMatrixThread,NULL);
	if (eclsoEnv.feedbackRequestEnabled)
		{
		debugPrint("T1: join with T4");
		pthread_join(feedbackHandlerThread,NULL);
		}

	sem_destroy(&(thread.notifyT1));
	sem_destroy(&(thread.notifyT2));
	sem_destroy(&(thread.notifyT3));

	matrixBufferDestroy();
	fecManagerDestroy();
	timerDestroy();
	debugPrint("T1: memory cleared");
	loggerDestroy();

	return 0;
}

/*Thread functions*/
static void *fill_matrix_thread(void *parm) // thread T1
{
	//This is the main thread, denoted by "T1";
	//this thread is in charge of filling a matrix with the segments received from
	//LTP (or another upper layer)
	EclsoThreadParms	 *thread = (EclsoThreadParms *) parm;
	EclsoEnvironment 	 *eclsoEnv=		  thread->eclsoEnv;
	EclsaMatrix 		 *matrix;

	int 			segmentLength;
	char			*segment;
	segment= (char *) calloc( eclsoEnv->T -2 , sizeof(char)  );
	eclsoEnv->maxInfoSize=getBiggestFEC()->K;

	debugPrint("log: thread T1 running (fill_matrix_thread)");

	while(thread->running)
		{
		receiveSegmentFromUpperProtocol(segment,&segmentLength);

		while ( (matrix=getMatrixToFill(eclsoEnv->globalMatrixID,eclsoEnv->ownEngineId))==NULL)
			{
			debugPrint("T1: sleeping... all matrix full");
			sem_wait(&(thread->notifyT1));
			}

		sem_wait(&(matrix->lock));

		if( isMatrixEmpty(matrix) )
			{
			matrix->ID=		  		 eclsoEnv->globalMatrixID;
			matrix->globalID= 		 eclsoEnv->globalMatrixID;
			matrix->engineID=		 eclsoEnv->ownEngineId;
			matrix->maxInfoSize=	 eclsoEnv->maxInfoSize;
			matrix->feedbackEnabled= eclsoEnv->feedbackRequestEnabled;
			matrix->HSLTPModeEnabled = eclsoEnv->HSLTP_mode;

			timerStart(&(matrix->timer),
					eclsoEnv->maxAggregationTime,
					matrix,
					&(matrix->ID),
					&(matrix->engineID));
			debugPrint("T1: start to fill new matrix MID=%d EID=%d", matrix->ID, matrix->engineID);
			}

		addSegmentToEclsaMatrix(matrix,segment,segmentLength,matrix->infoSegmentAddedCount,true);

		if ( isMatrixInfoPartFull(matrix) )
			{
			timerStop(&(matrix->timer));
			eclsoEnv->globalMatrixID++;
			matrix->clearedToCodec=true;
			sem_notify(&(thread->notifyT2));
			debugPrint("T1: MID=%d full, stop timer, wake up T2!", matrix->ID);
			}

		sem_post(&(matrix->lock));
		}

free(segment);
return NULL;
}
static void *fill_matrix_thread_HSLTP_MODE(void *parm) // thread T1
{
	//This is the main thread, denoted by "T1";
	//this thread is in charge of filling a matrix with the segments received from
	//LTP (or another upper layer)
	EclsoThreadParms	 *thread = (EclsoThreadParms *) parm;
	EclsoEnvironment 	 *eclsoEnv=		  thread->eclsoEnv;
	EclsaMatrix 		 *matrix;

	int processingType;
	int 			segmentLength;
	char			*segment;

	segment= (char *) calloc( eclsoEnv->T -2 , sizeof(char)  );
	eclsoEnv->maxInfoSize=getBiggestFEC()->K;

	debugPrint("log: thread T1 running (fill_matrix_thread)");

	while(thread->running)
		{
		processingType = receiveSegmentFromUpperProtocol_HSLTP_MODE(segment,&segmentLength);
		while ( (matrix=getMatrixToFill(eclsoEnv->globalMatrixID,eclsoEnv->ownEngineId))==NULL)
			{
			debugPrint("T1: sleeping... all matrix full");
			sem_wait(&(thread->notifyT1));
			}

		sem_wait(&(matrix->lock));

		if( isMatrixEmpty(matrix) )
			{
			matrix->ID=		  		 eclsoEnv->globalMatrixID;
			matrix->globalID= 		 eclsoEnv->globalMatrixID;
			matrix->engineID=		 eclsoEnv->ownEngineId;
			matrix->maxInfoSize=	 eclsoEnv->maxInfoSize;
			matrix->feedbackEnabled= eclsoEnv->feedbackRequestEnabled;
			matrix->HSLTPModeEnabled = eclsoEnv->HSLTP_mode;
// EOB ?
			if(processingType == DEFAULT_PROC || processingType == END_OF_MATRIX)
					{
						matrix->HSLTPMatrixType = ONLY_DATA;
						timerStart(&(matrix->timer),
							eclsoEnv->maxAggregationTime,
							matrix,
							&(matrix->ID),
							&(matrix->engineID));
					debugPrint("T1: start to fill new matrix MID=%d EID=%d default proc", matrix->ID, matrix->engineID);
					}
			else if (processingType == SPECIAL_PROC)
					{
						matrix->HSLTPMatrixType = ONLY_SIGNALING;

						timerStart(&(matrix->timer),
								SPECIAL_PROC_TIMER,
								matrix,
								&(matrix->ID),
								&(matrix->engineID));
						debugPrint("T1: start to fill new matrix MID=%d EID=%d special proc", matrix->ID, matrix->engineID);
					}
			}
			else
			{
				if (processingType == SPECIAL_PROC && matrix->HSLTPMatrixType == ONLY_DATA)
					{
					matrix->HSLTPMatrixType = SIGNALING_AND_DATA;
					}
				if(processingType == DEFAULT_PROC && matrix->HSLTPMatrixType == ONLY_SIGNALING)
					{
					matrix->HSLTPMatrixType = SIGNALING_AND_DATA;
					timerStop(&(matrix->timer));
					timerStart(&(matrix->timer),
										eclsoEnv->maxAggregationTime,
										matrix,
										&(matrix->ID),
										&(matrix->engineID));
					debugPrint("T1: change to signaling and data matrix MID=%d EID=%d special proc", matrix->ID, matrix->engineID);
					}
			}
		addSegmentToEclsaMatrix(matrix,segment,segmentLength,matrix->infoSegmentAddedCount,true);
		//debugPrint("T1: segment of type %d added to the matrix", processingType);

		if ( isMatrixInfoPartFull(matrix) || processingType == END_OF_MATRIX)
		{
			timerStop(&(matrix->timer));
			eclsoEnv->globalMatrixID++;
			matrix->clearedToCodec=true;
			sem_notify(&(thread->notifyT2));
			if (isMatrixInfoPartFull(matrix))
				debugPrint("T1: MID=%d full, stop timer, wake up T2!", matrix->ID);
			else 
				debugPrint("T1: MID=%d received EOB, stop timer, wake up T2!", matrix->ID);
			}

		sem_post(&(matrix->lock));
		}

free(segment);
return NULL;
}
static void	*encode_matrix_thread(void *parm) 	// thread T2
{
	//This thread is in charge of matrix encoding
	EclsoThreadParms	 *thread = (EclsoThreadParms *) parm;
	EclsaMatrix 		 *matrix;
	EclsoEnvironment 	 *eclsoEnv= thread->eclsoEnv;

	debugPrint("log: thread T2 running (encode_matrix_thread)");

	while(thread->running)
	{
		debugPrint("T2: sleeping...");
		sem_wait(&(thread->notifyT2));
		debugPrint("T2: awake!");

		while ( (matrix=getMatrixToCode())!=NULL )
			{
			debugPrint("T2: iteration");
			sem_wait(&(matrix->lock));
			encodeEclsaMatrix(eclsoEnv,matrix);
			matrix->clearedToSend=true;
			sem_notify(&(thread->notifyT3));
			sem_post(&(matrix->lock));
			}
	}
	return NULL;
}
static void	*pass_matrix_thread(void *parm) 	//thread T3
	{
	//This thread is in charge of passing all matrix segments
	//to UDP (or to another lower layer) via the eclsa protocol
	EclsoThreadParms	 *thread = (EclsoThreadParms *) parm;
	EclsoEnvironment 	 *eclsoEnv= thread->eclsoEnv;
	EclsaMatrix 		 *matrix;
	debugPrint("log: thread T3 running (pass_matrix_thread)");
	while(thread->running)
		{
		debugPrint("T3: sleeping...");
		sem_wait(&(thread->notifyT3));
		debugPrint("T3: awake!");
		while ( (matrix=getMatrixToSend())!=NULL )
			{
			debugPrint("T3: iteration!");
			sem_wait(&(matrix->lock));
			passEclsaMatrix(eclsoEnv,matrix);
			flushEclsaMatrix(matrix);
			sem_notify(&(thread->notifyT1));
			sem_post(&(matrix->lock));
			}
		}
	return NULL;
	}
static void *feedback_handler_thread(void *parm)
{
	//This thread is in charge of managing the feedback sent by
	//eclsi on the Rx node to eclso on the Tx one.
	EclsoThreadParms	 *thread   = (EclsoThreadParms *) parm;
	EclsoEnvironment 	 *eclsoEnv = thread->eclsoEnv;
	EclsaFeedback 		 feedback;

	int bufferLength;
	void *lowerProtocolData;
	unsigned int lowerProtocolDataLenght;

	//float oldEstimatedSuccessRate;
	float matrixSuccessRate;
	float feedbackWeight;

	unsigned short minMid=eclsoEnv->globalMatrixID;
	unsigned short maxMid;

	char status;

	while(thread->running)
		{
		receivePacketFromLowerProtocol((char *)&feedback,&bufferLength,&lowerProtocolData,&lowerProtocolDataLenght);
		maxMid=eclsoEnv->globalMatrixID;

		if( !eclsoEnv->feedbackAdaptiveRcEnabled || isFeedbackInvalid(&feedback,minMid,maxMid) )
			{
			debugPrint("T4: feedback rcv MID=%d decState=%s (Rx/Tot)=%d/%d",
					feedback.matrixID,
					getCodecStatusString(feedback.codecStatus),
					feedback.receivedSegments,
					feedback.totalSegments);
			continue;
			}

		matrixSuccessRate=(float)feedback.receivedSegments/feedback.totalSegments;
		//oldEstimatedSuccessRate=eclsoEnv->estimatedSuccessRate;

		status= convertToAbstractCodecStatus(feedback.codecStatus);
		if(status == STATUS_CODEC_SUCCESS || status == STATUS_CODEC_NOT_DECODED)
			{
			//Successful decoding: set the weight of new data in the average processing
			if(feedback.totalSegments > FEEDBACK_DEFAULT_RELIABILITY_THRESHOLD)
				//standard weight
				feedbackWeight=FEEDBACK_DEFAULT_WEIGHT;
			else
				//reduce the weight if segments are few
				feedbackWeight=FEEDBACK_DEFAULT_WEIGHT * feedback.totalSegments / FEEDBACK_DEFAULT_RELIABILITY_THRESHOLD ;
			}
		else
			feedbackWeight=1; //Failed decoding: setting the weight to 1 means do not average with old data;

		debugPrint("Estimated Packet Loss Probability (EPLP) feedback weight %f",feedbackWeight);

		eclsoEnv->estimatedSuccessRate= feedbackWeight *matrixSuccessRate + (1-feedbackWeight) * eclsoEnv->estimatedSuccessRate;

		debugPrint("T4: feedback rcv & up MID=%d decState=%s matPER=%f estPER=%f (Rx/Tot)=%d/%d",
						feedback.matrixID,
						getCodecStatusString(feedback.codecStatus),
						1-matrixSuccessRate, //matrixPER
						//eclsoEnv->estimatedSuccessRate,
						//oldEstimatedSuccessRate,
						1-eclsoEnv->estimatedSuccessRate,
						//feedback.codeID,
						feedback.receivedSegments,
						feedback.totalSegments);
		minMid=feedback.matrixID+1;
		}

	return NULL;
	}

/*Common functions*/
static void parseCommandLineArgument(int argc, char *argv[], EclsoEnvironment *eclsoEnv)
{
	unsigned int		flags;
	const int parameters=9;
	if( argc < parameters )
		{
		debugPrint("wrong usage. missing some parameters.. argc=%d",argc);
		exit(1);
		}

	if( argc > parameters )
		{
		debugPrint("wrong usage.too much parameters.. argc=%d",argc);
		exit(1);
		}

	memset(eclsoEnv,0,sizeof(EclsoEnvironment));

	eclsoEnv->N = PARSE_PARAMETER(2);
	eclsoEnv->K = PARSE_PARAMETER(3);
	if( eclsoEnv->N < eclsoEnv->K )
		{
		debugPrint("ERROR: the parameter N cant be lower than K"); //todo
		exit(1);
		}
	eclsoEnv->maxAggregationTime=PARSE_PARAMETER(4);
	eclsoEnv->codingThreshold =PARSE_PARAMETER(5);

	//parsing flags
	flags=PARSE_PARAMETER(6);
	eclsoEnv->adaptiveModeEnabled = (flags & MASK_ADAPTIVE_MODE) != 0;
	eclsoEnv->feedbackRequestEnabled = (flags & MASK_FEEDBACK_REQUEST) != 0;
	eclsoEnv->feedbackAdaptiveRcEnabled = (flags & MASK_FEEDBACK_ADAPTIVE_RC) != 0;
	eclsoEnv->interleavingEnabled = (flags & MASK_INTERLEAVING) !=0;
	eclsoEnv->HSLTP_mode = (flags & MASK_HSLTP) !=0;
	eclsoEnv->staticMidEnabled = (flags & MASK_STATIC_MID) != 0;
	eclsoEnv->continuousModeEnabled = (flags & MASK_CONTINUOUS_MODE) != 0;

	eclsoEnv->txbps = PARSE_PARAMETER(7);
	eclsoEnv->estimatedSuccessRate= (float) eclsoEnv->K / eclsoEnv->N;

	if(eclsoEnv->feedbackAdaptiveRcEnabled && !eclsoEnv->feedbackRequestEnabled)
		{
		debugPrint("WARNING: feedbackRequest Disabled and fedbackAdaptive Enabled. enabling fedbackRequest...");
		eclsoEnv->feedbackRequestEnabled=true;
		}
	if(eclsoEnv->puncturingEnabled)
		{
		debugPrint("WARNING: Puncturing feature is not supported yet. disabling puncturing...");
		eclsoEnv->puncturingEnabled=false;
		}

	if(eclsoEnv->continuousModeEnabled && !isContinuousModeAvailable())
		{
		debugPrint("WARNING: Continuous mode feature is not supported by this Codec. disabling continuous mode...");
		eclsoEnv->continuousModeEnabled=false;
		}

	if(eclsoEnv->continuousModeEnabled && eclsoEnv->adaptiveModeEnabled)
		{
		debugPrint("WARNING: adaptiveCoding Enabled and continuousMode Enabled. disabling adaptiveCoding...");
		eclsoEnv->adaptiveModeEnabled=false;
		}
}

/*Single eclsa matrix functions*/
void encodeEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix)
	{
	matrix->encodingCode=getBestFEC(matrix->infoSegmentAddedCount,eclsoEnv->estimatedSuccessRate);
	void *codecMatrix= matrix->abstractCodecMatrix;
	FecElement 	*code= 		  matrix->encodingCode;
	int universalCodecStatus;

	matrix->codecStatus = encodeCodecMatrix(codecMatrix,code);
	universalCodecStatus=convertToAbstractCodecStatus(matrix->codecStatus);

	//As the encoding should never fail, the codecStatus should always be >0
	//the following check is for safety only.
	//todo se la codifica è andata a buon fine
	if(universalCodecStatus == STATUS_CODEC_SUCCESS )
			{
			//matrix->encoded=true;
			matrix->redundancySegmentAddedCount= code->N-code->K;
			}

	debugPrint("T2: Encoder info: %s "
			"MID=%d "
			"N=%d K=%d "
			"I=%d "
			"R=%d "		,getCodecStatusString(matrix->codecStatus),
						 matrix->ID,code->N,code->K,
						 matrix->infoSegmentAddedCount,
						 matrix->redundancySegmentAddedCount);

	}
void passEclsaMatrix(EclsoEnvironment *eclsoEnv, EclsaMatrix *matrix)
	{
	// Send one matrix to eclsi on the pair node (logical link);
	//pass matrix rows to eclsa and then the eclsa packet to the
	//UDP (or to another lower layer protocol).
	static 	EclsaHeader header;
	static char buffer[FEC_LOWERLEVEL_MAX_PACKET_SIZE];
	int bufferLength, segmentID;
	unsigned int i;
	bool addRedundancy= (convertToAbstractCodecStatus(matrix->codecStatus)== STATUS_CODEC_SUCCESS);

	createEclsaHeader(matrix,&header);
	//The sequence contains the segmentIDs of the segments that must be transmitted
	//(all but info padding and punctured redundancy)
	sequenceReload(&matrix->sequence,matrix->encodingCode,addRedundancy,eclsoEnv->interleavingEnabled,eclsoEnv->puncturingEnabled,matrix->infoSegmentAddedCount);

	for( i=0; i < sequenceGetLength(&(matrix->sequence)); i++ )
		{
		segmentID=matrix->sequence.values[i];
		//Create an eclsa packet and put it in "buffer"; give also bufferLength.
		createEclsaPacket(matrix,&header,segmentID,buffer,&bufferLength);
		//Send an eclsa packet to the lower layer
		sendPacketToLowerProtocol(buffer,&bufferLength,NULL);
		}
	debugPrint("T3: all segments of MID= %d sent to lower protocol", matrix->ID);
	}

/*Timer functions */
void timerHandler(unsigned int timerID,void *matrixData,void *userData)
{
/*
 To be thread-safe, a timer handler must contain the following instructions:
 sem_wait(&(matrix->lock));
 if(matrix->timer.ID == timerID)
 	 {
 	 ...
 	 }
  sem_post(&(matrix->lock));
 */
debugPrint("TT: Timer handler");
EclsoThreadParms *thread=(EclsoThreadParms *) userData;
EclsoEnvironment *eclsoEnv= thread->eclsoEnv;
EclsaMatrix *matrix= (EclsaMatrix *) matrixData;

sem_wait(&(matrix->lock));

if(!matrix->clearedToCodec && !matrix->clearedToSend && matrix->timer.ID == timerID)
	{
	debugPrint("T1: matrix filled. Status:\"Timeout\" MID=%d GID=%d",matrix->ID,matrix->globalID);
	eclsoEnv->globalMatrixID++;

	if (matrix->infoSegmentAddedCount >= eclsoEnv->codingThreshold)
		{
		matrix->clearedToCodec=true;
		sem_notify(&(thread->notifyT2));
		}
	else
		{
		debugPrint("T1: skipping encoding, infoSegmentAddedCount < codingThreshold");
		matrix->clearedToSend=true;
		sem_notify(&(thread->notifyT3));
		}

	}

sem_post(&(matrix->lock));
}

/*Synchronization */
void sem_notify(sem_t *threadSem)
{
//todo: is this the best way?
int val;
sem_getvalue(threadSem,&val);
if(val==0)
	sem_post(threadSem);
}
