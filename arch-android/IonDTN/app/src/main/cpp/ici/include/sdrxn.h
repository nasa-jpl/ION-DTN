/*

	sdrxn.h:	definitions supporting use of the SDR
			transaction mechanism.

	Author: Scott Burleigh, JPL
	
	Modification History:
	Date      Who	What
	06-05-07  SCB	Initial abstraction from original SDR API.

	Copyright (c) 2001-2007 California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
	acknowledged.

									*/
#ifndef _SDRXN_H_
#define _SDRXN_H_

#include "psm.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NONE
#define NONE		(-1)
#endif

#define	MAX_SDR_NAME	(31)

typedef struct sdrv_str	*Sdr;	/*	Local view of an SDR.		*/

/*			SDR configuration flags				*/
#define	SDR_IN_DRAM	1	/*	Write to & read from memory.	*/
#define	SDR_IN_FILE	2	/*	Write file; read file if nec.	*/
#define	SDR_REVERSIBLE	4	/*	Transactions may be reversed.	*/
#define	SDR_BOUNDED	8	/*	Object boundaries defended.	*/

/*		SDR system administration functions.			*/

#define sdr_initialize(wmSize, wmPtr, wmKey, wmName)	Sdr_initialize(wmSize, \
wmPtr, wmKey, wmName)
extern int		Sdr_initialize(size_t wmSize, char *wmPtr, int wmKey,
					char *wmName);
			/*	Initializes the overall sdr system.
				In particular, attaches to shared
				memory for sdr library operation
				(allocating that shared memory as
				necessary, unless wmPtr is non-NULL)
				and creates a semaphor to serialize
				access to the sdrs array.  If the
	 			sdr system is to share a common pool
				of pre-allocated memory with one or
				more other systems, provide the shared
				memory key and partition name of that
				common memory pool; otherwise use
				SM_NO_KEY for wmKey and NULL for
				wmName.					*/

extern void		sdr_wm_usage(PsmUsageSummary *summary);
                        /*	Loads PsmUsageSummary structure with
			 	snapshot of usage of the PSM-managed
				dynamic working memory privately
				allocated to the SDR system for its
				internal operations.  To print the
		      		snapshot, use psm_report().		*/ 

extern void		sdr_shutdown();
			/*	Ends all access to all SDRs and
				releases all resources used by the
				sdr system.				*/

/*		SDR data space administration functions.		*/

extern int		sdr_load_profile(char *name, int configFlags,
				size_t heapWords, int heapKey, size_t logSize,
				int logKey, char *pathName, char *restartCmd);
			/*	Loads the profile for an SDR into the
				sdrs list.  The profile of an SDR must
				be reloaded (into shared memory) on a
				given processor after each time the
				processor reboots, in order to
				re-establish access to the SDR from
				multiple tasks on that processor.

				At the time the profile is loaded,
				we check to see whether or not the
				named SDR already exists.  If it
				does, then we automatically back
				out the current transaction (if the
				SDR is configured for transaction
				reversibility and the transaction
				log contains any log entries).  If
				it does not, then we create and
				initialize the SDR.

			 	"name" is the name of the SDR.
				The "configFlags" value must be some
				logical disjunction of configuration
				flags as defined above.  "heapWords"
				is the size of the usable heap portion
				of the SDR's data space in "words"
				(long integers).  The SDR's total data
				space size in bytes is given by
			       	(heapWords * word size) + map size.

				On creation of the SDR, where the
				SDR_IN_DRAM option is selected, if
				"heapKey" is SM_NO_KEY then a region of
				shared memory of length equal to total
				data space size will automatically be
				allocated and shared using a dynamically
				selected shared memory key; otherwise
				"heapKey" must be a shared memory key
				identifying a pre-allocated region
				of shared memory of length equal to
				the total data space size, shared
				using the indicated key.

				"logSize" is the amount of memory
				that will be made available for the
				transaction reversibility log.  If
				SDR_REVERSIBLE is not set in the
				"configFlags" then this value is
				ignored; otherwise, if "logSize"
				is zero then the transaction
				reversibility log will be written
				to a file rather than to memory.

				On creation of the SDR, where the
				SDR_REVERSIBLE option is selected
				and logSize is greater than zero, if
				"logKey" is SM_NO_KEY then a region of
				shared memory of length equal to log
				size will automatically be allocated
				and shared using a dynamically
				selected shared memory key; otherwise
				"logKey" must be a shared memory key
				identifying a pre-allocated region
				of shared memory of length equal to
				the log size, shared using the
				indicated key.

				If SDR_IN_FILE is selected, or if
				SDR_REVERSIBLE is selected and
				"logSize" is zero, then the path
				name of the directory into which
				the log file and/or db file will
				be written must be supplied in
				pathName.  The name of the log file
				(if applicable) will be
				"<sdrname>.sdrlog".  The name of the
				db file (if applicable) will be
				"<sdrname>.sdr".  On creation
				of the SDR, where the SDR_IN_FILE
				option is selected, a file of the
				indicated name and of the size given
				by total data space size will be
				created and filled with zeros.

				If a cleanup task must be run whenever
				a transaction is reversed, the command
				to execute this task must be provided
				in "restartCmd".  If restartCmd is NULL
				then no cleanup task will be run upon
				transaction reversal.			*/

extern int		sdr_reload_profile(char *name, int configFlags,
				size_t heapWords, int heapKey, size_t logSize,
				int logKey, char *pathName, char *restartCmd);
			/*	For use when the state of an SDR is
			 *	thought to be inconsistent, perhaps
			 *	due to crash of a program that had
			 *	a transaction open.  Unloads the
			 *	profile for the SDR, forcing the
			 *	reversal of any transaction that is
			 *	currently in progress when the SDR's
			 *	profile is re-loaded.  Then calls
			 *	sdr_load_profile() to re-load the
			 *	profile for the SDR.  Same return
			 *	values as sdr_load_profile.		*/

#define sdr_start_using(name)	Sdr_start_using(name)
extern Sdr		Sdr_start_using(char *name);
			/*	Locates SDR by name and returns a
				handle that can be used for all
				functions on that SDR.			*/

extern char		*sdr_name(Sdr sdr);
			/*	Returns the name of the SDR.		*/

extern size_t		sdr_heap_size(Sdr sdr);
			/*	Returns total size of heap, in bytes.	*/

extern void		sdr_stop_using(Sdr sdr);
			/*	Ends access to the SDR via this
				Sdr handle; other users of the SDR
				are unaffected.				*/

extern void		sdr_abort(Sdr sdr);
			/*	Terminates the task.  In flight
				configuration, also terminates all
				use of the sdr system by all tasks.	*/

extern void		sdr_destroy(Sdr sdr);
			/*	Ends all access to this SDR, erases
				the SDR from memory and file system,
				and unloads the SDR's profile from
				the sdrs list.				*/

/*		Basic, low-level SDR transaction functions.		*/

extern int		sdr_begin_xn(Sdr sdr);
extern int		sdr_in_xn(Sdr sdr);		/*	Boolean	*/
extern int		sdr_heap_is_halted(Sdr sdr);	/*	Boolean	*/
extern void		sdr_exit_xn(Sdr sdr);
extern void		sdr_cancel_xn(Sdr sdr);
extern int		sdr_end_xn(Sdr sdr);

/*		Low-level SDR I/O functions.				*/

typedef uaddr		SdrAddress;
#define	Address		SdrAddress

/*	Both SdrObjects and SdrAddresses are absolute offsets from
 	the start of an SDR heap; they are functionally equivalent
	to pointers in DRAM.  They are differentiated to enable
	compile-time type checking to detect some possible SDR access
	errors: an SdrObject is the address of some block of SDR
	data space allocated by sdr_malloc() in the sdrmgt library,
	while an SdrAddress can point to any location in the data
	space (i.e., it can point anywhere inside an object).		*/

typedef uaddr		SdrObject;
#define	Object		SdrObject

extern void		*sdr_pointer(Sdr sdr, Address address);
extern Address		sdr_address(Sdr sdr, void *pointer);

#ifndef HEAP_PTRS
#define	HEAP_PTRS	0
#endif

#if (HEAP_PTRS)
#define OBJ_POINTER(typenm, varnm)\
	typenm	*varnm
#define	GET_OBJ_POINTER(sdrp, typenm, varnm, addr)\
	varnm = (typenm *) sdr_pointer(sdrp, addr)
#else
#define OBJ_POINTER(typenm, varnm)\
	typenm	varnm##BUF; typenm	*varnm = &varnm##BUF
#define	GET_OBJ_POINTER(sdrp, typenm, varnm, addr)\
	sdr_read(sdrp, (char *) &varnm##BUF, addr, sizeof(typenm))
#endif

#define FLD_OFFSET(fld, object) (((char *) fld) - ((char *) object))

#define sdr_write(sdr, into, from, size) \
Sdr_write(__FILE__, __LINE__, sdr, into, from, size)
extern void		Sdr_write(const char *file, int line,
				Sdr sdr, Address into, char *from, size_t size);

#define sdr_poke(sdr, address, variable) \
Sdr_write(__FILE__, __LINE__, sdr, address, \
(char *) &variable, sizeof variable)

#define sdr_set(sdr, pointer, variable) \
Sdr_write(__FILE__, __LINE__, sdr, sdr_address(sdr, pointer), \
(char *) &variable, sizeof variable)

extern void		sdr_read(Sdr sdr, char *into, Address from,
				size_t size);

#define sdr_peek(sdr, variable, address) \
sdr_read(sdr, (char *) &variable, address, sizeof variable)

#define sdr_get(sdr, variable, pointer) \
sdr_read(sdr, (char *) &variable, sdr_address(sdr, pointer), sizeof variable)

#define xniEnd(arg)	_xniEnd(__FILE__, __LINE__, arg, sdrv)
extern int		_xniEnd(const char *, int, const char *, Sdr);
#define XNCHKERR(e)	if (!(e) && xniEnd(#e)) return -1
#define XNCHKZERO(e)	if (!(e) && xniEnd(#e)) return 0
#define XNCHKNULL(e)	if (!(e) && xniEnd(#e)) return NULL
#define XNCHKVOID(e)	if (!(e) && xniEnd(#e)) return

#ifdef __cplusplus
}
#endif

#endif  /* _SDRXN_H_ */
