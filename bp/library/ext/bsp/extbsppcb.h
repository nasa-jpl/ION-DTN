/*
 * extbsppcb.h
 *
 *  Created on: Jul 25, 2011
 *      Author: brownrp1
 */

#ifndef EXTBSPPCB_H_
#define EXTBSPPCB_H_

#include "extbsputil.h"

/** PCB Constants */
#define PCB_SESSION_KEY_LENGTH 128
#define PCB_ENCRYPTION_CHUNK_SIZE 4096
#define PCB_ZCO_ENCRYPT_FILENAME "pcbencrypteddatafile"
#define PCB_ZCO_DECRYPT_FILENAME "pcbdecrypteddatafile"

// Option enabling payload encryption or not
#define DEBUG_ENCRYPT_PAYLOAD 1
// Option enabling session key encryption or not
#define DEBUG_ENCRYPT_SESSION 1

// If bsp debugging is turned on, then turn on pcb
#if BSP_DEBUGGING == 1
#define PCB_DEBUGGING 1
#endif

#ifndef PCB_DEBUGGING
#define PCB_DEBUGGING 0  /** Whether to enable (1) or disable (0) debugging */
#endif

#define PCB_DEBUG_LVL_PROC 1 /** Function entry/exit and above debugging */
#define PCB_DEBUG_LVL_INFO 2 /** Info information and above debugging */
#define PCB_DEBUG_LVL_WARN 3 /** Warning and above debugging */
#define PCB_DEBUG_LVL_ERR  4 /** Error and above debugging */

#define PCB_DEBUG_LVL   PCB_DEBUG_LVL_PROC

#define GMSG_BUFLEN     256
#if PCB_DEBUGGING == 1

/**
 * \def PCB_DEBUG
 * Constructs an error string message and sends it to putErrmsg. There are
 * four levels of debugging specified:
 * 1: Function entry/exit logging.  This logs the entry and exit of all major
 *    functions in the PCB library and is useful for confirming control flow
 *    through the PCB module.
 * 2: Information logging.  Information statements are peppered through the
 *    code to provide insight into the state of the module at processing
 *    points considered useful by BSP module software engineers.
 * 3: Warning logging.  Warning statements are used to flag unexpected 
 *    values that, based on context, may not constitute errors.
 * 4: Error logging.  Errors are areas in the code where some sanity check
 *    or other required condition fails to be met by the software. 
 * 
 * Error logging within the PCB module is of the form:
 * <id> <function name>: <message>
 * Where id is one of:
 * + (function entry)
 * - (function exit)
 * i (information statement)
 * ? (warning statement)
 * x (error statement)
 * 
 * Debugging can be turned off at compile time by removing the
 * PCB_DEBUGGING #define.
 */

   #define PCB_DEBUG(level, format,...) if(level >= PCB_DEBUG_LVL) \
{_isprintf(gMsg, GMSG_BUFLEN, format, __VA_ARGS__); printf("%s\n", gMsg);}

   #define PCB_DEBUG_PROC(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define PCB_DEBUG_INFO(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define PCB_DEBUG_WARN(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define PCB_DEBUG_ERR(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_ERR,format, __VA_ARGS__)
#else
   #define PCB_DEBUG(level, format,...) if(level >= PCB_DEBUG_LVL) \
{}

   #define PCB_DEBUG_PROC(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_PROC,format, __VA_ARGS__)

   #define PCB_DEBUG_INFO(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_INFO,format, __VA_ARGS__)

   #define PCB_DEBUG_WARN(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_WARN,format, __VA_ARGS__)

   #define PCB_DEBUG_ERR(format,...) \
           PCB_DEBUG(PCB_DEBUG_LVL_ERR,format, __VA_ARGS__)

#endif

/* Special object used to take the bundle payload,
 * Separate it into its parts and put it back together
 * after decryption. This helps cut down on passing the
 * large number of parameters into functions to do this.
 */

typedef struct BspPayloadReplaceKit {
    char *headerBuff;
    char *payloadBuff;
    char *trailerBuff;
    unsigned int headerLen;
    unsigned int payloadLen;
    unsigned int trailerLen;
    Object oldBundle;
    Object newBundle;
} BspPayloadReplaceKit;

/*****************************************************************************
 *                     PCB EXTENSIONS INTERFACE FUNCTIONS                    *
 *****************************************************************************/

/******************************************************************************
 *
 * \par Function Name: bsp_pcbAcquire
 *
 * \par Purpose: This callback is called when a serialized PCB block is
 *               encountered during bundle reception.  This callback will
 *               deserialize the block into a scratchpad object.
 *
 * \retval int -- 1 - The block was deserialized into a structure in the
 *                    scratchpad
 *                0 - The block was deserialized but does not appear valid.
 *               -1 - There was a system error.
 *
 * \param[in,out]  blk  The block whose serialized bytes will be deserialized
 *                      in the block's scratchpad.
 * \param[in]      wk   The work area associated with this bundle acquisition.
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int bsp_pcbAcquire(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbClear
 *
 * \par Purpose: This callback removes all memory allocated by the BSP module
 *               during the block's acquisition process. This function is the
 *               same for both PRE and POST payload blocks.
 *
 * \retval void
 *
 * \param[in,out]  blk  The block whose memory pool objects must be released.
 *
 * \par Notes:
 *      1. The block's memory pool objects have been allocated as specified
 *         by the BSP module.
 *      2. The length field associated with each pointer field is accurate
 *      3. A length of 0 implies no memory is allocated to the associated
 *         data field.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

void bsp_pcbClear(AcqExtBlock *blk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCopy
 *
 * \par Purpose: This callback copies the scratchpad object of a PCB
 * 		 block to a new block that is a copy of the original.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  newBlk The new copy of this extension block.
 * \param[in]      oldBlk The original extension block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 *
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  04/02/12  S. Burleigh   Initial Implementation.
 *****************************************************************************/

int  bsp_pcbCopy(ExtensionBlock *newBlk, ExtensionBlock *oldBlk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbOffer
 *
 * \par Purpose: This callback determines whether a PCB block is necessary for
 *               this particular bundle, based on local security policies.
 *               However, at this point we may not have enough information
 *               (such as EIDs) to query the security policy. Therefore, the
 *               offer callback ALWAYS adds the PCB block.  When we do the
 *               process on dequeue callback we will have enough information
 *               to determine whether or not the PCB extension block should
 *               be populated or scratched.
 *
 * \retval int 0 - The block was successfully processed.
 *            -1 - There was a system error.
 *
 * \param[in,out]  blk    The block that may/may not be added to the bundle.
 * \param[in]      bundle The bundle that might hold this block.
 *
 * \par Notes:
 *      1. All block memory is allocated using sdr_malloc.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int  bsp_pcbOffer(ExtensionBlock *blk, Bundle *bundle);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCheck
 *
 * \par Purpose: This callback checks a PCB block, upon bundle receipt
 *               to decrypt the payload data in the bundle
 *               For PCB, a long term key is looked up to decrypt a session
 *               key present in the security result.  The decrypted session
 *               key is used to decrypt the payload.  If no long term key is found
 *               and we aren't the security destination, this block is quietly ignored.
 *
 * \retval int 0 - The block check was inconclusive
 *             1 - The block check failed.
 *             2 - The block check succeed or there was no long term key and we aren't the
 *                 security destination
 *            -1 - There was a system error.
 *
 * \param[in]  blk  The acquisition block being checked.
 * \param[in]  wk   The working area holding other acquisition blocks and the
 *                  rest of the received bundle.
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int  bsp_pcbCheck(AcqExtBlock *blk, AcqWorkArea *wk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbProcessOnDequeue
 *
 * \par Purpose: This callback constructs the post-payload PCB block for
 *               inclusion in a bundle at the end of the dequeue phase
 *
 * \retval int 0 - The post-payload block was successfully created
 *            -1 - There was a system error.
 *
 * \param[in\out]  blk  The block whose abstract security block structure
 *                           will be populated and then serialized into the
 *                           block's bytes array.
 * \param[in]      bundle    The bundle holding the block.
 *
 * \par Notes:
 *      1. No other blocks will be added to the bundle, and no existing blocks
 *         in the bundle will be modified.  This must be the last block
 *         modification performed on the bundle before it is transmitted.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int bsp_pcbProcessOnDequeue(ExtensionBlock *post_blk,
                                Bundle *bundle,
                                void *ctxt);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbRelease
 *
 * \par Purpose: This callback removes memory allocated by the BSP module
 *               from a particular extension block.
 *
 * \retval void
 *
 * \param[in\out]  blk  The block whose allocated memory pools must be
 *                      released.
 *
 * \par Notes:
 *      1. It is OK to simply free the entire scratchpad object, without
 *         explicitly freeing the result data part of the ASB.
 *         bsp_pcbProcessOnDequeue
 *         calculates the security result and inserts it directly into the
 *         bytes array of the block, which is freed by the main ION library.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

void bsp_pcbRelease(ExtensionBlock *blk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbPrepReplaceKit
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a number of parameters about the existing bundle payload
 *               and initializes a BspPayloadReplaceKit object.
 *
 * \retval BspPayloadReplaceKit * Newly created/initialized replace kit object
 *
 * \param[in] bundle      The address to the old payload where the whole bundle
 *                        is residing
 * \param[in] payloadLen  Length of payload in the bundle payload object
 * \param[in] headerLen   Length of header in the bundle payload object
 * \param[in] trailerLen  Length of trailer in the bundle payload object
 *
 * \par Notes:
 *      1. This function is simply to prep an object that makes managing the surgery
 *         on the payload (taking header/trailer off, decrypting payload,
 *         putting it back) easier.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  09/13/11  R. Brown        Initial Implementation. 
 *****************************************************************************/

BspPayloadReplaceKit *bsp_pcbPrepReplaceKit(Object bundle, unsigned int payloadLen,
                                            unsigned int headerLen, unsigned int trailerLen);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbIsolatePayload
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a BspPayloadReplaceKit object and strips of the old
 *               payload object's header and trailer, saved for placement onto
 *               a new payload object later. In the process, only the true payload
 *               data in the bundle payload object will remain.
 *
 * \retval       The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in] bpSdr       ion's sdr object
 * \param[in] *bprk       Pointer to the replaceKit object
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  09/13/11  R. Brown        Initial Implementation. 
 *****************************************************************************/

int bsp_pcbIsolatePayload(Sdr bpSdr, BspPayloadReplaceKit *bprk);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbConstructDecryptedPayload
 *
 * \par Purpose: This is a helper function for the check callback.
 *               It takes a BspPayloadReplaceKit object and reconstructs the
 *               new payload object, right after using the given key to encrypt the
 *               old payload data.
 *
 * \retval       The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in] bpSdr       ion's sdr object
 * \param[in] *bprk       Pointer to the replaceKit object

 * \param[in] sessionKeyValue   Pointer to the key to use to encrypt the old data
 * \param[in] sessionKeyLen     Length of the key
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  09/13/11  R. Brown        Initial Implementation. 
 *****************************************************************************/
int bsp_pcbConstructDecryptedPayload(Sdr bpSdr, BspPayloadReplaceKit *bprk, 
                                     unsigned char *sessionKeyValue, unsigned int sessionKeyLen);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCryptPayload
 *
 * \par Purpose: Encrypts/decrypts given payload data and puts it into
 *               a new file. The references to that data will be put into the
 *               given resultZco object. 
 *
 * \retval int - The value indicating outcome: 0 for success,
 *                                            -1 for failure
 *
 * \param[in]  resultZco     - The object which the function will point the
 *                             new data to.
 * \param[in]  payloadData   - The payload data
 * \param[in]  fname         - Part of the filename indicating whether its
 *                             encryption or decryption
 * \param[in]  payloadLen    - Length of the payload data
 * \param[in]  keyValue      - The key to use.
 *
 * \par Notes:
 *      1. Currently, only arc4 is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown       Initial Implementation. (based off E. Birrane's BAB)
 *****************************************************************************/

int bsp_pcbCryptPayload(Object *resultZco, Object payloadData, char *fname,
                                   unsigned int payloadLen,
                                   unsigned char *keyValue,
                                   unsigned int keyLen);

/******************************************************************************
 *
 * \par Function Name: bsp_pcbCryptSessionKey
 *
 * \par Purpose: Encrypts/decrypts given session key using the given long term
 *               key
 *
 * \retval unsigned char * - The pointer to the now encrypted/decrypted key
 *
 * \param[in]  sessionKey    - The pointer that contains the session key string
 * \param[in]  sessionKeyLen - The length of the session key
 * \param[in]  ltKeyValue    - The longterm key to use to encrypt/decrypt session key
 * \param[in]  ltKeyLen      - Length of the long term key
 *
 * \par Notes:
 *      1. Currently, only arc4 is implemented.
 *      2. This function should be updated for a more parameterized security
 *         result based on different ciphersuites.
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation.
 *****************************************************************************/

unsigned char *bsp_pcbCryptSessionKey(unsigned char *sessionKey,
                                      unsigned int sessionKeyLen,  
                                      unsigned char *ltKeyValue,
                                      unsigned int ltKeyLen); 

/******************************************************************************
 *
 * \par Function Name: bsp_pcbGenSessionKey
 *                                    
 * \par Purpose: Generates a random session key string
 *
 * \retval unsigned char * - A pointer to the session key string generated
 *
 * \param[in]  sessionKeyLen - Pointer to put the length of the session key in
 *
 * \par Notes:
 * \par Revision History:
 *
 *  MM/DD/YY  AUTHOR        SPR#    DESCRIPTION
 *  --------  ------------  -----------------------------------------------
 *  08/1/11  R. Brown        Initial Implementation.
 *****************************************************************************/
                                      
unsigned char *bsp_pcbGenSessionKey(unsigned int *sessionKeyLen);

#endif /* EXTBSPPCB_H_ */
