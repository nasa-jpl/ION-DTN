/*
 eclsaMemoryManager.c

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */


#include "eclsaMemoryManager.h"

#include "eclsaLogger.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct {
	Pointer		pointer;
	size_t		sizeAllocated;
} CouplePointerSize;

typedef struct {
	unsigned long long 	maxMemorySize;				// Max size of memory allocable
	unsigned long long 	currentSize;				// Current space allocation

	pthread_mutex_t		lock;						// Lock for multiple-access security
	CouplePointerSize*	arrayAllocatedPointers;		// It contains the couples currently allocated
	size_t				sizeArrayAllocatedPointers;	// The size of the array
} MemoryManager;


/*****   PRIVATE FUNCTIONS   *****/
static void autoFreeMemory();
static void removePointerFromCouple(Pointer pointer);
static bool addCouple(Pointer pointer, const size_t size);
static void setCouple(CouplePointerSize *couple, Pointer pointer, size_t size);
static void clearCouple(CouplePointerSize *couple);
static bool haveEnoughSpace(const size_t space);
static Pointer _allocateNElements(const size_t count, const size_t elementSize);
static void _deallocMatrixVectors(Matrix matrix, const unsigned int rows);
static bool _allocateMatrixVectors(Matrix matrix, const size_t elementSize, const unsigned int rows, const unsigned int cols);
static size_t getByteSize(const TypeOfBytes typeOfBytes);


/*****   PRIVATE VARIABLES   *****/
static MemoryManager memoryManager;
static bool isInitialized = false;



/*****   PUBLIC FUNCTIONS   *****/


void initMemoryManager(const size_t size, const TypeOfBytes typeOfBytes)
{
	int i;
	if ( isInitialized )
		return;

	pthread_mutex_init(&(memoryManager.lock), NULL);

	memoryManager.currentSize = 0;
	memoryManager.maxMemorySize = size * getByteSize(typeOfBytes);

	memoryManager.sizeArrayAllocatedPointers = floor(1*getByteSize(MEGABYTE) / sizeof(CouplePointerSize)) * sizeof(CouplePointerSize) ;
	memoryManager.arrayAllocatedPointers = calloc(memoryManager.sizeArrayAllocatedPointers, sizeof(CouplePointerSize));

	for(i=0; i < memoryManager.sizeArrayAllocatedPointers;i++)
		clearCouple(&(memoryManager.arrayAllocatedPointers[i]));

	isInitialized = true;
}

void destroyMemoryManager(const bool autoFreeLeak)
{
	if ( !isInitialized )
		return;

	if ( memoryManager.currentSize != 0 )
		debugPrint("MemoryManager: WARNING! Some memory-leak may be present in program!");

	if ( autoFreeLeak )
		autoFreeMemory();

	pthread_mutex_destroy(&(memoryManager.lock));

	memoryManager.currentSize = 0;
	memoryManager.maxMemorySize = 0;

	free(memoryManager.arrayAllocatedPointers);
	memoryManager.sizeArrayAllocatedPointers = 0;

	isInitialized = false;
}

Pointer allocateElement(const size_t elementSize)
{
	if ( !isInitialized || elementSize == 0 )
		return NULL;

	return (Pointer) _allocateNElements(1, elementSize);
}

void deallocateelement(Pointer* elementPointer)
{
	if ( !isInitialized || elementPointer == NULL || *elementPointer == NULL)
		return;

	free((void*)*elementPointer);
	removePointerFromCouple(*elementPointer);

	*elementPointer = NULL;
}

Vector allocateVector(const size_t elementSize, const size_t length)
{
	if ( !isInitialized || elementSize == 0 || length == 0 )
		return NULL;

	return (Vector) _allocateNElements(length, elementSize);
}

void deallocatevector(Vector* vectorPointer)
{
	if ( !isInitialized || vectorPointer == NULL || *vectorPointer == NULL )
		return;

	deallocateElement((Pointer*)vectorPointer);
}

Matrix allocateMatrix(const size_t elementSize, const unsigned int rows, const unsigned int cols)
{
	bool error = false;
	Matrix returnMatrix = NULL;

	if ( !isInitialized || elementSize == 0 || rows == 0 || cols == 0 )
		return NULL;

	returnMatrix = (Vector*) allocateVector(sizeof(Vector), rows);	// allocate the rows vector

	if ( returnMatrix == NULL ) 									// not enough space in memory!
		error = true;
	else
		error = ! (_allocateMatrixVectors(returnMatrix, elementSize, rows, cols));

	if ( error )
		{
		// clean memory
		_deallocMatrixVectors(returnMatrix, rows);
		deallocateVector((Vector*)&returnMatrix);

		returnMatrix = NULL;
		}
	return returnMatrix;
}

void deallocatematrix(Matrix* matrixPointer, const unsigned int rows)
{
	//int i;
	Matrix matrix = NULL;

	if ( !isInitialized || matrixPointer == NULL || *matrixPointer == NULL )
		return;

	matrix = *matrixPointer;

	_deallocMatrixVectors(matrix, rows);
	deallocateVector((Vector*)&matrix);

	*matrixPointer = NULL;
}



/*****   PRIVATE FUNCTIONS   *****/


static void autoFreeMemory()
{
	int i;

	for(i=0; i < memoryManager.sizeArrayAllocatedPointers;i++)
		if ( memoryManager.arrayAllocatedPointers[i].pointer != NULL )
			deallocateElement((Pointer*)&(memoryManager.arrayAllocatedPointers[i].pointer));
}

static void removePointerFromCouple(Pointer pointer)
{
	int i;

	if ( pointer == NULL )
		return;

	pthread_mutex_lock(&(memoryManager.lock));

	for(i=0; i < memoryManager.sizeArrayAllocatedPointers;i++)
		{
		if ( memoryManager.arrayAllocatedPointers[i].pointer == pointer ) // found element, proceed to remove it
			{
			memoryManager.currentSize -= memoryManager.arrayAllocatedPointers[i].sizeAllocated;
			clearCouple(&(memoryManager.arrayAllocatedPointers[i]));
			pthread_mutex_unlock(&(memoryManager.lock));
			return;
			}
		}
	pthread_mutex_unlock(&(memoryManager.lock));
}

// true if couple added (buffer not full)
static bool addCouple(Pointer pointer, const size_t size)
{
	int i;

	if ( pointer == NULL || size == 0 )
		return false;

	pthread_mutex_lock(&(memoryManager.lock));

	for(i=0; i < memoryManager.sizeArrayAllocatedPointers;i++)
		{
		if ( memoryManager.arrayAllocatedPointers[i].pointer == NULL ) // found a free position, add couple!
			{
			setCouple(&(memoryManager.arrayAllocatedPointers[i]), pointer, size);
			memoryManager.currentSize += size;
			pthread_mutex_unlock(&(memoryManager.lock));
			return true;
			}
		}

	pthread_mutex_unlock(&(memoryManager.lock));
	return false;
}

static void setCouple(CouplePointerSize *couple, Pointer pointer, size_t size)
{
	if ( couple == NULL )
		return;

	couple->pointer =		pointer;
	couple->sizeAllocated =	size;
}

static void clearCouple(CouplePointerSize *couple)
{
	if ( couple == NULL )
		return;
	setCouple(couple, NULL, 0);
}

static bool haveEnoughSpace(const size_t space)
{
	return ( memoryManager.currentSize + space <= memoryManager.maxMemorySize );
}

static Pointer _allocateNElements(const size_t count, const size_t elementSize)
{
	Pointer returnPointer = NULL;
	size_t allocateSize = count * elementSize;

	if ( count == 0 || elementSize == 0 )
		return NULL;

	if ( !haveEnoughSpace(allocateSize) )
		return NULL;

	returnPointer = (Pointer) calloc(count, elementSize);

	if ( returnPointer != NULL && !addCouple(returnPointer, allocateSize) ) 	// have allocated but array is full
			deallocateElement(&returnPointer); 									// have to deallocate, can't save his size

	return returnPointer;
}

// Returns true if success
static bool _allocateMatrixVectors(Matrix matrix, const size_t elementSize, const unsigned int rows, const unsigned int cols)
{
	unsigned int i;

	if ( matrix == NULL || elementSize == 0 || rows == 0 || cols == 0 )
		return false;

	for (i=0; i < rows ;i++)
		{
		matrix[i] = allocateVector(elementSize, cols);	 		// allocate each row-element as a vector long cols

		if ( matrix[i] == NULL ) 								// not enough space in memory!
			return false;
		}

	return true;
}

static void _deallocMatrixVectors(Matrix matrix, const unsigned int rows)
{
	unsigned int i;

	if (matrix == NULL || rows == 0 )
		return;

	for (i=0; i < rows; i++)
		{
		deallocateVector(&(matrix[i]));
		}
}

static size_t getByteSize(const TypeOfBytes typeOfBytes)
{
	return (size_t) typeOfBytes;
}
