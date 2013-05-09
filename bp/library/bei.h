/******************************************************************************
 	_bei.h:	Private header file used to encapsulate structures, constants,
 	        and function prototypes that deal with BP extension blocks.

	Author: Scott Burleigh, JPL

	Modification History:
	Date      Who   What

	Copyright (c) 2010, California Institute of Technology.
	ALL RIGHTS RESERVED.  U.S. Government Sponsorship acknowledged.
*******************************************************************************/

#ifndef _BEI_H_
#define _BEI_H_

/*****************************************************************************
 **
 ** File Name: _bei.h
 **
 ** Subsystem: BP
 **
 ** Description: This file provides all structures, variables, and function
 **              definitions necessary to define and manipulate extension
 **              blocks within the ION BP Agent.
 **
 ** Assumptions:
 **      1.
 **
 ** Modification History:
 **
 **  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 **  --------  ------------  -------  ---------------------------------------
 **  Original  S. Burleigh            Initial Implementation
 **  04/15/10  E. Birrane     105     Segmented into _bei.h file from bpP.h
 *****************************************************************************/

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

/*****************************************************************************
 *                             CONSTANT DEFINITIONS                          *
 *****************************************************************************/

/**
 * Processing Directives
 * These directives identify processing callbacks for areas of the bundle
 * protocol agent that handle data flow through parts of the extension
 * block lifecycle.
 */
#define	PROCESS_ON_FORWARD		(0)
#define	PROCESS_ON_TAKE_CUSTODY		(1)
#define	PROCESS_ON_ENQUEUE		(2)
#define	PROCESS_ON_DEQUEUE		(3)
#define	PROCESS_ON_TRANSMIT		(4)

/*****************************************************************************
 *                                DATA STRUCTURES                            *
 *****************************************************************************/

/**
 *  \struct ExtensionBlock
 *  \brief Definition of an SDR-contained Bundle Extension Block.
 *
 * This structure holds any RFC5050-compliant extension block that is being
 * constructed for sending out from the BPA.  This structure is stored in the
 * SDR and may be persistently stored until the block is ready to be sent and/
 * or the containing bundle may be removed.
 */
typedef struct
{
	unsigned char	rank;		/**	Order within def array.	*/
	unsigned char	type;		/**	Per definitions array.	*/
	unsigned short	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	Object		object;		/**	Opaque scratchpad.	*/
	Object		eidReferences;	/**	SDR list (may be 0).	*/
	Object		bytes;		/**	Array in SDR heap.	*/
	int		suppressed;	/**	If suppressed.          */
} ExtensionBlock;

/**
 *  \struct AcqExtBlock
 *  \brief Definition of a memory-only Bundle Extension Block.
 *
 * This structure holds any RFC5050-compliant extension block that is being
 * acquired from an underlying convergence layer. These blocks are stored in
 * memory as sent from the personal space manager (PSM).
 */
typedef struct
{
	unsigned char	type;		/**	Per extensions array.	*/
	unsigned short	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	void		*object;	/**	Opaque scratchpad.	*/
	Lyst		eidReferences;	/**	May be NULL.		*/
	unsigned char	bytes[1];	/**	Variable-length array.	*/
} AcqExtBlock;

/** Functions used on the creation/send side of an extension block. */
typedef int		(*BpExtBlkOfferFn)(ExtensionBlock *, Bundle *);
typedef void		(*BpExtBlkReleaseFn)(ExtensionBlock *);
typedef int		(*BpExtBlkRecordFn)(ExtensionBlock *, AcqExtBlock *);
typedef int		(*BpExtBlkCopyFn)(ExtensionBlock *, ExtensionBlock *);
typedef int		(*BpExtBlkProcessFn)(ExtensionBlock *, Bundle *, void*);

/** Functions used on the receive side of an extension block. */
typedef int		(*BpAcqExtBlkAcquireFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpAcqExtBlkCheckFn)(AcqExtBlock *, AcqWorkArea *);
typedef void		(*BpAcqExtBlkClearFn)(AcqExtBlock *);

/**
 *  \struct ExtensionDef
 *  \brief Defines the callbacks used to process extension blocks.
 *
 * ExtensionDefs hold the callbacks, type, and position of the meta-data
 * used to define and process extension blocks within the bundle.
 */
typedef struct
{
	char			name[32];	/** Name of extension	*/
	unsigned char		type;		/** Block type		*/
	unsigned char		listIdx;	/** Extension location	*/
	BpExtBlkOfferFn		offer;		/** Offer 		*/
	BpExtBlkReleaseFn	release;	/** Release 		*/
	BpAcqExtBlkAcquireFn	acquire;	/** Acquire 		*/
	BpAcqExtBlkCheckFn	check;		/** Check 		*/
	BpExtBlkRecordFn	record;		/** Record 		*/
	BpAcqExtBlkClearFn	clear;		/** Clear 		*/
	BpExtBlkCopyFn		copy;		/** Copy		*/
	BpExtBlkProcessFn	process[5];	/** ProcessingDirectives*/
} ExtensionDef;

typedef struct
{
	unsigned char	type;	/** The type of correlator block	*/
	unsigned char	id;	/** Identifier of this block.		*/
	unsigned int	size;	/** The size of the allocated block.	*/
} CollabBlockHdr;

/*****************************************************************************
 *                             FUNCTION PROTOTYPES                           *
 *****************************************************************************/

/* Functions that operate on Bundle objects */

extern int 	addCollaborationBlock(Bundle *bundle, CollabBlockHdr *blkHdr);
extern int	attachExtensionBlock(ExtensionDef *def, ExtensionBlock *blk,
			Bundle *bundle);
extern int 	copyCollaborationBlocks(Bundle *newBundle, Bundle *oldBundle);

/**
 * \par Function Name: copyExtensionBlock
 * \par Purpose:
 * \retval int
 * \param[out] newBundle The Bundle receiving the copied blocks
 * \param[in]  oldBundle The Bundle containing the original blocks.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern int	copyExtensionBlocks(Bundle *newBundle, Bundle *oldBundle);
void		deleteExtensionBlock(Object elt, unsigned int listIdx);
void 		destroyCollaborationBlocks(Bundle *bundle);
void		destroyExtensionBlocks(Bundle *bundle);
Object 		findCollaborationBlock(Bundle *bundle, unsigned char type,
			unsigned int id);

/**
 * \par Function Name: findExtensionBlock
 * \par Purpose:
 * \retval Object - The discovered block.
 * \param[in]  bundle  - The bundle holding the desired block.
 * \param[in]  type    - The block identifier desired.
 * \param[in]  idx 	   - Search before or after the payload.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern Object	findExtensionBlock(Bundle *bundle, unsigned int type,
			unsigned int idx);
extern int	insertExtensionBlock(ExtensionDef *def, ExtensionBlock *newBlk,
			Object blkAddr, Bundle *bundle, unsigned char listIdx);
extern int	patchExtensionBlocks(Bundle *bundle);
extern int	processExtensionBlocks(Bundle *bundle, int fnIdx,
			void *context);

/**
 * \par Function Name: restoreExtensionBlock
 * \par Purpose:
 * \retval void
 * \param[in]  blk  The block being restored
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern void	restoreExtensionBlock(ExtensionBlock *blk);

/**
 * \par Function Name: scratchExtensionBlock
 * \par Purpose:
 * \retval void
 * \param[in]  blk  The block being scratched.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern void	scratchExtensionBlock(ExtensionBlock *blk);

/**
 * \par Function Name: serializeExtBlk
 * \par Purpose:
 * \retval void
 * \param[in]  blk           - The block being serialized
 * \param[in]  eidReferences - EIDs referenced in this block.
 * \param[out] blockData     - The serialized block.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern int	serializeExtBlk(ExtensionBlock *blk, Lyst eidReferences,
			char *blockData);

/**
 * \par Function Name: suppressExtensionBlock
 * \par Purpose:
 * \retval void
 * \param[in]  blk  The block being suppressed
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern void	suppressExtensionBlock(ExtensionBlock *blk);
extern int 	updateCollaborationBlock(Object collabAddr,
			CollabBlockHdr *blkHdr);

/* Functions that operate on Acquisition Work Area objects */

extern int	acquireExtensionBlock(AcqWorkArea *work, ExtensionDef *def,
			unsigned char *startOfBlock, unsigned int blockLength,
			unsigned char blkType, unsigned int blkProcFlags,
			Lyst *eidReferences, unsigned int dataLength);
extern int 	addAcqCollabBlock(AcqWorkArea *work, CollabBlockHdr *blkHdr);
extern int	checkExtensionBlocks(AcqWorkArea *work);
extern void	deleteAcqExtBlock(LystElt elt, unsigned int listIdx);
extern void 	destroyAcqCollabBlocks(AcqWorkArea *work);

/**
 * \par Function Name: discardExtensionBlock
 * \par Purpose:
 * \retval void
 * \param[in]  blk  The block being discarded.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern void	discardExtensionBlock(AcqExtBlock *blk);
extern LystElt	findAcqCollabBlock(AcqWorkArea *work, unsigned char type,
			unsigned int id);
extern int	recordExtensionBlocks(AcqWorkArea *work);

/* Extension definitions access functions */

extern void	getExtensionDefs(ExtensionDef **array, int *count);
extern
ExtensionDef	*findExtensionDef(unsigned char type, unsigned char idx);

#endif // _BEI_H_
