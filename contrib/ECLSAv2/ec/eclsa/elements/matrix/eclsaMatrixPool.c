/*
 eclsaMatrixPool.c

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#include "eclsaMatrixPool.h"
#include "eclsaMatrix.h"
#include <stdbool.h>
#include "../sys/eclsaLogger.h"
#include "../sys/eclsaMemoryManager.h"
#include <string.h>
#include <stdlib.h> //malloc, free

/***** PRIVATE FUNCTIONS *****/
void initPoolMatrix(EclsaMatrix *matrix, const int N, const int T);
static void destroyPoolMatrix(EclsaMatrix *matrix);
static inline EclsaMatrix* getMatrixInPosition(const unsigned int position);
static inline void setMatrixInPosition(const unsigned int position, EclsaMatrix *pointer);
static inline bool isStaticPosition(const unsigned int position);
static inline bool isDynamicPosition(const unsigned int position);
static inline bool isStaticMatrix(EclsaMatrix *matrix);
static inline bool isDynamicMatrix(EclsaMatrix *matrix);
static int getNumberAllocatedMatrix();

/*
 * Matrix Pool structure
 */
typedef struct
{
	EclsaMatrix **matrixPool;		// Marix Pool
	unsigned int fullLength;		// Length of full pool (static + dynamic)
	unsigned int staticPoolSize;	// Length of static pool (and position of first dynamic element)

	pthread_mutex_t lock;		// Pool lock
} MatrixPool;

static MatrixPool matrixPool;

void matrixPoolInit(const unsigned int staticPool, const unsigned int maxDynamicMatrix)
{
	int i;
	memset(&matrixPool,0,sizeof(MatrixPool));

	matrixPool.staticPoolSize=staticPool;
	matrixPool.fullLength=staticPool + maxDynamicMatrix;

	matrixPool.matrixPool = (EclsaMatrix**) allocateVector(sizeof(EclsaMatrix*), matrixPool.fullLength);

	/*  Recursive Mutex (useful to check if a matrix is static or dynamic)  */
	pthread_mutexattr_t recursiveAttribute;
	pthread_mutexattr_init(&recursiveAttribute);
	pthread_mutexattr_settype(&recursiveAttribute, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&(matrixPool.lock), &recursiveAttribute);

	// sets all matrixs to NULL
	for(i=0;i< matrixPool.staticPoolSize;i++)
		{
		setMatrixInPosition(i, NULL);
		}

	// XXX if removed --> initial static pool = 0 else initial static pool = staticPool
	/*for(i=0;i< matrixPool.staticPoolSize; i++)
		{
		EclsaMatrix* matrix = (EclsaMatrix*) allocateElement(sizeof(EclsaMatrix));
		setMatrixInPosition(i, matrix);
		initPoolMatrix(matrix, getBiggestFEC()->N, getBiggestFEC()->T);
		//eclsaMatrixInit(&(matrixPool.array[i]));
		//sequenceInit(&(matrixPool.array[i].sequence),getBiggestFEC()->N);
		}*/

}
void matrixPoolDestroy()
{
	int i;
	for(i=0;i< matrixPool.fullLength; i++)
		{
		if( getMatrixInPosition(i) != NULL )
			{
			EclsaMatrix* matrixPointer = getMatrixInPosition(i);
			destroyPoolMatrix(matrixPointer);
			deallocateElement(&(matrixPointer));
			}
		}
	deallocateVector(&(matrixPool.matrixPool));

	pthread_mutex_destroy(&(matrixPool.lock));
}
EclsaMatrix *getMatrixToFill(const unsigned short matrixID, const unsigned short engineID, int N, int T)
{
	static int currentIndex=0;
	int i;
	EclsaMatrix *matrix;

	pthread_mutex_lock(&(matrixPool.lock));
	//Look if the previous matrix is still the requested matrix
	matrix=getMatrixInPosition(currentIndex);
	if(matrix != NULL && matrix->ID == matrixID && matrix->engineID == engineID && matrix->abstractCodecMatrix->rows >= N && matrix->abstractCodecMatrix->cols >= T)
		{
		pthread_mutex_unlock(&(matrixPool.lock));
		return matrix;
		}

	//otherwise looks for the requested matrix
	for(i=0;i< matrixPool.fullLength;i++)
		{
		matrix=getMatrixInPosition(i);
		if(matrix != NULL && matrix->ID == matrixID && matrix->engineID == engineID && matrix->abstractCodecMatrix->rows >= N && matrix->abstractCodecMatrix->cols >= T )
			{
			pthread_mutex_unlock(&(matrixPool.lock));
			currentIndex=i;
			return matrix;
			}
		}

	//if the requested matrix is not found, return the first free matrix
	for(i=0;i< matrixPool.fullLength;i++)
		{
		matrix=getMatrixInPosition(i);

		// if matrix is NULL --> create matrix
		if ( matrix == NULL )
			{
			if ( isStaticMatrix(matrix) )
				{
				N=getBiggestFEC()->N;
				T=getBiggestFEC()->T;
				}
			matrix = (EclsaMatrix*) allocateElement(sizeof(EclsaMatrix));
			setMatrixInPosition(i, matrix);
			initPoolMatrix(matrix, N, T); //todo può fallire perchè finisco la memoria, da sistemare!
			if ( matrix != 	NULL && matrix->abstractCodecMatrix != NULL)  	// enough memory
				{
				debugPrint("Matrix created at index=%d with N=%d T=%d", i, N, T);
				debugPrint("Matrix (used/total) %d/%d", getNumberAllocatedMatrix(), matrixPool.fullLength);
				}
			else								// not enough memory
				{
				deallocateElement((Pointer*)&(matrix));
				matrix=NULL;
				setMatrixInPosition(i, NULL); // reset null pointer
				debugPrint("[WARNING] Matrix NOT created at index=%d because of full memory", i);
				}
			}

		if( matrix != NULL && isMatrixEmpty(matrix) )
			{
			currentIndex=i;
			pthread_mutex_unlock(&(matrixPool.lock));
			return matrix;
			}
		}

	//if there is not any free matrix also in dynamic pool, the pool is full. By returning NULL the thread is put to sleep
	pthread_mutex_unlock(&(matrixPool.lock));
	return NULL;
}
EclsaMatrix *getMatrixToCode()
{
	int i;
	unsigned long 	minGlobalID;
	EclsaMatrix		*matrix_i;
	EclsaMatrix 	*returnMatrix;
	bool 			found=false;

	pthread_mutex_lock(&(matrixPool.lock));

	//Find the next matrix to code (i.e. that with the lower globalID)
	for(i=0;i< matrixPool.fullLength;i++)
		{
		matrix_i=getMatrixInPosition(i);

		if( matrix_i != NULL && !matrix_i->clearedToSend && matrix_i->clearedToCodec && ( !found || matrix_i->globalID < minGlobalID))
			{
			minGlobalID= matrix_i->globalID;
			returnMatrix=matrix_i;
			found=true;
			}
		}

	pthread_mutex_unlock(&(matrixPool.lock));
	if(found)
		return returnMatrix;
	else
		return NULL;
}
EclsaMatrix *getMatrixToSend()
{
	int i;
	unsigned long 	minGlobalID;
	EclsaMatrix 	*matrix_i;
	EclsaMatrix 	*returnMatrix;
	bool 			found=false;

	pthread_mutex_lock(&(matrixPool.lock));

	//Find the next matrix to send in dynamic pool (with the lower globalID)
	for(i=0;i< matrixPool.fullLength;i++)
		{
		matrix_i=getMatrixInPosition(i);
		if(matrix_i != NULL && matrix_i->clearedToSend && ( !found || matrix_i->globalID < minGlobalID))
			{
			minGlobalID= matrix_i->globalID;
			returnMatrix=matrix_i;
			found=true;
			}
		}

	pthread_mutex_unlock(&(matrixPool.lock));
	if(found)
		return returnMatrix;
	else
		return NULL;
}
bool	forcePreviousMatrixToDecode(EclsaMatrix *currentMatrix)
{
	//todo
	EclsaMatrix *matrix_i;
	unsigned int i;
	bool found=false;
	pthread_mutex_lock(&(matrixPool.lock));
		for(i=0;i<matrixPool.fullLength;i++)
			{
			matrix_i=getMatrixInPosition(i);
			if(  matrix_i != NULL &&
				!matrix_i->clearedToCodec &&
				!matrix_i->clearedToSend &&
				 matrix_i->engineID == currentMatrix->engineID &&
				 matrix_i->ID < currentMatrix->ID)
				{
				timerStop(&(matrix_i->timer));
				matrix_i->clearedToCodec=true;
				debugPrint("T1: matrix received.Status:\"Forced\" MID=%d EID=%d N=%d K=%d I=%d/%d R=%d/%d",matrix_i->ID,matrix_i->engineID,matrix_i->encodingCode->N,matrix_i->encodingCode->K,matrix_i->infoSegmentAddedCount,matrix_i->maxInfoSize,matrix_i->redundancySegmentAddedCount,matrix_i->encodingCode->N-matrix_i->encodingCode->K);
				found=true;
				}
			}
	pthread_mutex_unlock(&(matrixPool.lock));
	return found;
}
void flushMatrixFromPool(EclsaMatrix **matrix)
{
	int i;
	EclsaMatrix *matrix_i;

	if ( matrix == NULL )
		return;

	pthread_mutex_lock(&(matrixPool.lock));

	for(i=0;i< matrixPool.fullLength;i++)
		{
		matrix_i=getMatrixInPosition(i);

		if( matrix_i != NULL && matrix_i == *matrix) // is requested to delete this matrix (can be removed only if is in the dynamic part)
			{
			flushEclsaMatrix(*matrix); // flush the matrix anyway
			if ( isDynamicMatrix(matrix_i) )
				{
				destroyPoolMatrix(matrix_i);
				deallocateElement((Pointer*)&(matrix_i));
				setMatrixInPosition(i, NULL); // reset null pointer
				debugPrint("Flushed matrix index=%d, destroyed from memory", i);
				*matrix = NULL;
				}
			else if( isStaticMatrix(matrix_i) )
				{
				debugPrint("Flushed matrix index=%d but it still remains in memory", i);
				}
			debugPrint("Matrix (used/total) %d/%d", getNumberAllocatedMatrix(), matrixPool.fullLength);
			pthread_mutex_unlock(&(matrixPool.lock));
			return;
			}
		}

	pthread_mutex_unlock(&(matrixPool.lock));
}

/***** PRIVATE FUNCTIONS *****/

static int getNumberAllocatedMatrix()
{
	int numberOfMatrix = 0;
	int i;

	pthread_mutex_lock(&(matrixPool.lock));

	for(i=0;i< matrixPool.fullLength; i++)
	{
		if (getMatrixInPosition(i) != NULL)
		{
			numberOfMatrix++;
		}
	}

	pthread_mutex_unlock(&(matrixPool.lock));

	return numberOfMatrix;
}

static inline bool isStaticMatrix(EclsaMatrix *matrix)
{
	int i;

	pthread_mutex_lock(&(matrixPool.lock));

	for(i=0;i< matrixPool.fullLength;i++)
		{
		if( matrix == getMatrixInPosition(i) ) // found
			{
			pthread_mutex_unlock(&(matrixPool.lock));
			return isStaticPosition(i);
			}
		}

	// Not found, default false
	pthread_mutex_unlock(&(matrixPool.lock));

	return false;
}

static inline bool isDynamicMatrix(EclsaMatrix *matrix)
{
	int i;

	pthread_mutex_lock(&(matrixPool.lock));

	for(i=0;i< matrixPool.fullLength;i++)
		{
		if( matrix == getMatrixInPosition(i) ) // found
			{
			pthread_mutex_unlock(&(matrixPool.lock));
			return isDynamicPosition(i);
			}
		}

	// Not found, default false
	pthread_mutex_unlock(&(matrixPool.lock));

	return false;
}

static inline bool isStaticPosition(const unsigned int position)
{
	return (position < matrixPool.staticPoolSize);
}

static inline bool isDynamicPosition(const unsigned int position)
{
	return (position >= matrixPool.staticPoolSize);
}

static inline EclsaMatrix* getMatrixInPosition(const unsigned int position)
{
	return matrixPool.matrixPool[position];
}

static inline void setMatrixInPosition(unsigned int position, EclsaMatrix *pointer)
{
	matrixPool.matrixPool[position] = pointer;
}

void initPoolMatrix(EclsaMatrix *matrix, const int N, const int T)
{
	if ( matrix == NULL)
		return;
	eclsaMatrixInit(matrix, N, T);
	if ( matrix->abstractCodecMatrix != NULL ) // init may fail (not memory)
		sequenceInit(&(matrix->sequence), getBiggestFEC()->N);
}

void destroyPoolMatrix(EclsaMatrix *matrix)
{
	eclsaMatrixDestroy(matrix);
	sequenceDestroy(&(matrix->sequence));
}
