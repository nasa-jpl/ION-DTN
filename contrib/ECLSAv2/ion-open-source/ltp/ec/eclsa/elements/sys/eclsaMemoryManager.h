/*
 eclsaMemoryManager.h

 Author: Andrea Bisacchi (andrea.bisacchi5@studio.unibo.it)
 Project Supervisor: Carlo Caini (carlo.caini@unibo.it)

 Copyright (c) 2018, Alma Mater Studiorum, University of Bologna
 All rights reserved.

todo

 * */

#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_


#include <stdbool.h>
#include <stddef.h>

typedef void* Pointer;
typedef Pointer Vector;
typedef Vector* Matrix;

typedef enum {
	BYTE 		= 1,
	KILOBYTE 	= BYTE 		* 1024,
	MEGABYTE 	= KILOBYTE 	* 1024,
	GIGABYTE 	= MEGABYTE 	* 1024
} TypeOfBytes;

void initMemoryManager(const size_t size, const TypeOfBytes typeOfBytes);
void destroyMemoryManager(const bool autoFreeLeak);

Pointer allocateElement(const size_t size);
void deallocateelement(Pointer* elementPointer);

Vector allocateVector(const size_t elementSize, const size_t length);
void deallocatevector(Vector* vectorPointer);

Matrix allocateMatrix(const size_t elementSize, const unsigned int rows, const unsigned int cols);
void deallocatematrix(Matrix* matrixPointer, const unsigned int rows);

#define deallocateElement(pointer) 			deallocateelement((Pointer*)(pointer));
#define deallocateVector(pointer) 			deallocatevector((Vector*)(pointer));
#define deallocateMatrix(pointer, rows)		deallocatematrix((Matrix*)(pointer), (rows));

#endif /* MEMORYMANAGER_H_ */
