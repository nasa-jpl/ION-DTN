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
 *  \brief Definition of an outbound Bundle Extension Block.
 *
 * This structure holds any RFC5050-compliant extension block that is being
 * constructed for transmission from the BPA.  This structure is stored in the
 * notionally non-volatile SDR heap until the block is ready to be sent and/
 * or the containing bundle may be destroyed.
 */
typedef struct
{
	unsigned char	type;		/**	Per definitions array.	*/
	unsigned char	occurrence;	/**	Sequential count.	*/
	unsigned short	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	Object		object;		/**	Opaque scratchpad.	*/
	Object		eidReferences;	/**	SDR list (may be 0).	*/
	Object		bytes;		/**	Array in SDR heap.	*/

	/*	Internally significant data for block production.	*/

	unsigned char	tag1;		/**	Extension-specific.	*/
	unsigned char	tag2;		/**	Extension-specific.	*/
	unsigned char	tag3;		/**	Extension-specific.	*/
	unsigned short	rank;		/**	Order within spec array.*/
	int		suppressed;	/**	If suppressed.          */
} ExtensionBlock;

/**
 *  \struct AcqExtBlock
 *  \brief Definition of an inbound Bundle Extension Block.
 *
 * This structure holds any RFC5050-compliant extension block that is being
 * acquired from an underlying convergence layer.  Since the bundle
 * containing the block has not yet been committed to SDR heap storage
 * but exists only in the transient bundle acquisition work area stored in
 * volatile ION working memory, this block likewise is stored in ION working
 * memory.
 */
typedef struct
{
	unsigned char	type;		/**	Per definitions array.	*/
	unsigned char	occurrence;	/**	Sequential count.	*/
	unsigned short	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	void		*object;	/**	Opaque scratchpad.	*/
	Lyst		eidReferences;	/**	May be NULL.		*/
	unsigned char	bytes[1];	/**	Variable-length array.	*/
} AcqExtBlock;

/** Functions used in creating and transmitting an outbound extension block. */
typedef int		(*BpExtBlkOfferFn)(ExtensionBlock *, Bundle *);
typedef int		(*BpExtBlkProcessFn)(ExtensionBlock *, Bundle *, void*);
typedef void		(*BpExtBlkReleaseFn)(ExtensionBlock *);
typedef int		(*BpExtBlkCopyFn)(ExtensionBlock *, ExtensionBlock *);

/** Functions used in acquiring an inbound extension block. */
typedef int		(*BpAcqExtBlkAcquireFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpAcqExtReviewFn)(AcqWorkArea *);
typedef int		(*BpAcqExtBlkDecryptFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpAcqExtBlkParseFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpAcqExtBlkCheckFn)(AcqExtBlock *, AcqWorkArea *);
typedef int		(*BpExtBlkRecordFn)(ExtensionBlock *, AcqExtBlock *);
typedef void		(*BpAcqExtBlkClearFn)(AcqExtBlock *);

/**
 *  \struct ExtensionDef
 *  \brief Defines the callbacks used to process extension blocks.
 *
 * ExtensionDef defines the callbacks for production and acquisition
 * of a single type of extension block, identified by block type name
 * and number and occurrence number (0 for First or Only occurrence,
 * 1 for Last occurrence).
 */
typedef struct
{
	char			name[32];	/** Name of extension	*/
	unsigned char		type;		/** Block type		*/

	/*	Production callbacks.					*/

	BpExtBlkOfferFn		offer;		/** Offer 		*/
	BpExtBlkProcessFn	process[5];	/** Process		*/
	BpExtBlkReleaseFn	release;	/** Release 		*/
	BpExtBlkCopyFn		copy;		/** Copy		*/

	/*	Acquisition callbacks.					*/

	BpAcqExtBlkAcquireFn	acquire;	/** Acquire 		*/
	BpAcqExtReviewFn	review;		/** Review		*/
	BpAcqExtBlkDecryptFn	decrypt;	/** Decrypt 		*/
	BpAcqExtBlkParseFn	parse;		/** Parse 		*/
	BpAcqExtBlkCheckFn	check;		/** Check 		*/
	BpExtBlkRecordFn	record;		/** Record 		*/
	BpAcqExtBlkClearFn	clear;		/** Clear 		*/
} ExtensionDef;

/**
 *  \struct ExtensionSpec
 *  \brief Defines the canonical extension block production order.
 *
 * ExtensionSpec provides the specification for producing an outbound
 * extension block: block definition (identified by block type number),
 * three discriminator tags whose semantics are block-type-specific,
 * and list index, indicating whether the extension block is to be
 * inserted before or after the Payload block.
 *
 * listIdx is obsolete in sbsp, as *NO* blocks are ever inserted after
 * the Payload block.
 */
typedef struct
{
	unsigned char		type;		/** Block type		*/
	unsigned char		tag1;		/** Extension-specific	*/
	unsigned char		tag2;		/** Extension-specific	*/
	unsigned char		tag3;		/** Extension-specific	*/
	unsigned char		listIdx;	/** Location in bundle	*/
} ExtensionSpec;

/*****************************************************************************
 *                             FUNCTION PROTOTYPES                           *
 *****************************************************************************/

/*	Functions that operate on outbound extension blocks		*/

extern int	attachExtensionBlock(ExtensionSpec *spec, ExtensionBlock *blk,
			Bundle *bundle);
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
void		deleteExtensionBlock(Object elt, int *lengthsTotal);
void		destroyExtensionBlocks(Bundle *bundle);

/**
 * \par Function Name: findExtensionBlock
 * \par Purpose:
 * \retval Object - The discovered block.
 * \param[in]  bundle  - The bundle holding the desired block.
 * \param[in]  type    - The block identifier desired.
 * \param[in]  tag1    - A discriminator indicating the role of the block.
 * \param[in]  tag2    - A discriminator indicating the role of the block.
 * \param[in]  tag3    - A discriminator indicating the role of the block.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern Object	findExtensionBlock(Bundle *bundle, unsigned int type,
			unsigned char tag1, unsigned char tag2,
			unsigned char tag3);

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

/*	Functions that operate on inbound extension blocks		*/

extern int	acquireExtensionBlock(AcqWorkArea *wk, ExtensionDef *def,
			unsigned char *startOfBlock, unsigned int blockLength,
			unsigned char blkType, unsigned int blkProcFlags,
			Lyst *eidReferences, unsigned int dataLength);
extern int	reviewExtensionBlocks(AcqWorkArea *wk);
extern int	decryptPerExtensionBlocks(AcqWorkArea *wk);
extern int	parseExtensionBlocks(AcqWorkArea *wk);
extern int	checkPerExtensionBlocks(AcqWorkArea *wk);
extern void	deleteAcqExtBlock(LystElt elt);

/**
 * \par Function Name: discardExtensionBlock
 * \par Purpose:
 * \retval void
 * \param[in]  blk  The inbound block being discarded.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern void	discardExtensionBlock(AcqExtBlock *blk);
extern LystElt	findAcqExtensionBlock(AcqWorkArea *wk, unsigned int type,
			unsigned int occurrence);
extern int	recordExtensionBlocks(AcqWorkArea *wk);

/*	Functions that operate on extension block definitions		*/

extern void	getExtensionDefs(ExtensionDef **array, int *count);
extern
ExtensionDef	*findExtensionDef(unsigned char type);

/*	Functions that operate on extension block specifications	*/

extern void	getExtensionSpecs(ExtensionSpec **array, int *count);
extern
ExtensionSpec	*findExtensionSpec(unsigned char type, unsigned char tag1,
			unsigned char tag2, unsigned char tag3);

/*	Functions that operate on collaboration blocks			*/

#ifdef ORIGINAL_BSP
typedef struct
{
	unsigned char	type;	/** The type of correlator block	*/
	unsigned char	id;	/** Identifier of this block.		*/
	unsigned int	size;	/** The size of the allocated block.	*/
} CollabBlockHdr;

extern int 	addCollaborationBlock(Bundle *bundle, CollabBlockHdr *blkHdr);
extern int 	copyCollaborationBlocks(Bundle *newBundle, Bundle *oldBundle);
void 		destroyCollaborationBlocks(Bundle *bundle);
Object 		findCollaborationBlock(Bundle *bundle, unsigned char type,
			unsigned int id);
extern int 	updateCollaborationBlock(Object collabAddr,
			CollabBlockHdr *blkHdr);

extern int 	addAcqCollabBlock(AcqWorkArea *work, CollabBlockHdr *blkHdr);
extern void 	destroyAcqCollabBlocks(AcqWorkArea *work);
extern LystElt	findAcqCollabBlock(AcqWorkArea *work, unsigned char type,
			unsigned int id);
#endif /* ORIGINAL_BSP */

#endif /* _BEI_H_ */
