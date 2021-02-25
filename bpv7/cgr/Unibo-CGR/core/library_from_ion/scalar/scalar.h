/***************************************************************
 * \file scalar.h
 *
 * \brief  This file defines the type CgrScalar and some functions
 *         to manage it.
 *
 * \par Ported from ION 3.7.0 by
 *      Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 * 
 * \par Supervisor
 *      Carlo Caini, carlo.caini@unibo.it
 *
 ***************************************************************/

#ifndef LIBRARY_CGR_SCALAR
#define LIBRARY_CGR_SCALAR
#define	ONE_GIG			(1 << 30)

typedef struct
{
	long int gigs;
	long int units;
} CgrScalar;

#ifdef __cplusplus
extern "C"
{
#endif

extern void loadCgrScalar(CgrScalar*, long int);
extern void increaseCgrScalar(CgrScalar*, long int);
extern void reduceCgrScalar(CgrScalar*, long int);
extern void multiplyCgrScalar(CgrScalar*, long int);
extern void divideCgrScalar(CgrScalar*, long int);
extern void copyCgrScalar(CgrScalar *to, CgrScalar *from);
extern void addToCgrScalar(CgrScalar*, CgrScalar*);
extern void subtractFromCgrScalar(CgrScalar*, CgrScalar*);
extern int CgrScalarIsValid(CgrScalar*);

#ifdef __cplusplus
}
#endif

#endif
