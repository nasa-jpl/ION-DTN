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
 * This structure holds any BPv7-compliant extension block that is being
 * constructed for transmission from the BPA.  This structure is stored in the
 * notionally non-volatile SDR heap until the block is ready to be sent and/
 * or the containing bundle may be destroyed.
 */
typedef struct
{
	BpBlockType	type;		/**	Per definitions array.	*/
	unsigned char	number;		//	To be developed.
	unsigned char	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	Object		object;		/**	Opaque scratchpad.	*/
	Object		bytes;		/**	Array in SDR heap.	*/

	/*	Internally significant data for block production.	*/

	unsigned char	tag1;		/**	Extension-specific.	*/
	unsigned char	tag2;		/**	Extension-specific.	*/
	unsigned char	tag3;		/**	Extension-specific.	*/
	unsigned short	rank;		/**	Order within spec array.*/
	BpCrcType	crcType;
	int		suppressed;	/**	If suppressed.          */
} ExtensionBlock;

/**
 *  \struct AcqExtBlock
 *  \brief Definition of an inbound Bundle Extension Block.
 *
 * This structure holds any BPv7-compliant extension block that is being
 * acquired from an underlying convergence layer.  Since the bundle
 * containing the block has not yet been committed to SDR heap storage
 * but exists only in the transient bundle acquisition work area stored in
 * volatile ION working memory, this block likewise is stored in ION working
 * memory.
 */
typedef struct
{
	BpBlockType	type;		/**	Per definitions array.	*/
	unsigned char	number;		//	To be developed.
	unsigned char	blkProcFlags;	/**	Per BP spec.		*/
	unsigned int	dataLength;	/**	Block content.		*/
	unsigned int	length;		/**	Length of bytes array.	*/
	unsigned int	size;		/**	Size of scratchpad obj.	*/
	void		*object;	/**	Opaque scratchpad.	*/
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
 * and number.
 */
typedef struct
{
	char			name[32];	/** Name of extension	*/
	BpBlockType		type;		/** Block type		*/

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
extern Object	findExtensionBlock(Bundle *bundle, BpBlockType type,
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
 * \param[out] blockData     - The serialized block.
 * \par Notes:
 * \par Revision History:
 *  MM/DD/YY  AUTHOR        IONWG#    DESCRIPTION
 *  --------  ------------  ------ ----------------------------------------
 *            S. Burleigh		   Initial Implementation
 */
extern int	serializeExtBlk(ExtensionBlock *blk, char *blockData);

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
			BpBlockType blkType, unsigned int blkNumber,
			unsigned int blkProcFlags, unsigned int dataLength);
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
extern LystElt	findAcqExtensionBlock(AcqWorkArea *wk, BpBlockType type);
extern int	recordExtensionBlocks(AcqWorkArea *wk);

/*	Functions that operate on extension block definitions		*/

extern void	getExtensionDefs(ExtensionDef **array, int *count);
extern
ExtensionDef	*findExtensionDef(BpBlockType type);

/*	Functions that operate on extension block specifications	*/

extern void	getExtensionSpecs(ExtensionSpec **array, int *count);
extern
ExtensionSpec	*findExtensionSpec(BpBlockType type, unsigned char tag1,
			unsigned char tag2, unsigned char tag3);

#endif /* _BEI_H_ */
