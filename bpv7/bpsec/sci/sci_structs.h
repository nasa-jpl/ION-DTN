/*
 * sci_structs.h
 *
 *  Created on: Mar 25, 2022
 *      Author: ebirrane
 */

#ifndef _SCI_STRUCTS_H_
#define _SCI_STRUCTS_H_

#include "ion.h"
#include "bp.h"
#include "bpP.h"



/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/

#define BPSEC_SC_NAME_MAX_LEN 32

typedef enum {
	SC_ROLE_SOURCE   = 0x01,
	SC_ROLE_VERIFIER = 0x02,
	SC_ROLE_ACCEPTOR = 0x04
} sc_role;

typedef enum {
	SC_STAT_OK          = 0x00,
	SC_STAT_BAD_CONTEXT = 0x01,
	SC_STAT_BAD_PARM    = 0x02,
	SC_STAT_PROC_ERROR_ = 0x03,
	SC_STAT_BAD_ID      = 0x04,
	SC_STAT_BAD_SVC     = 0x05
} sc_status;


typedef enum {
	SC_ACT_SIGN     = 0x01,
	SC_ACT_VERIFY   = 0x02,
	SC_ACT_ENCRYPT  = 0x04,
	SC_ACT_DECRYPT  = 0x08
} sc_action;

enum sc_services {
    SC_SVC_BIBINT  = 0x01,
    SC_SVC_BCBCONF = 0x02
};



/**
 * Different types of security value.
 *
 * SC_VAL_TYPE_PARM:   A security parameter. This may refer to a policy
 *                     parameter stored on a local BPA, a parameter from a
 *                     security block read from a received bundle, or a
 *                     parameter that has been generated at a BPA forhandling
 *                     a specific security block.
 *
 * SC_VAL_TYPE_RESULT: A security result. This may refer to a result calculated
 *                     in working memory while verifying a received security
 *                     block, or a result stored in the SDR as part of building
 *                     a security block.
*/
typedef enum {
    SC_VAL_TYPE_PARM     = 0x01,
    SC_VAL_TYPE_RESULT   = 0x02,
    SC_VAL_TYPE_UNKNOWN  = 0xFF
} sc_val_type;



/**
 * Different storage locations of a SC value.
 *
 * SC_VAL_STORE_MEM: The SC value is stored in working memory. This storage
 *                   location is only useful for individual applications which
 *                   operate on values as part of security checks.
 *
 * SC_VAL_STORE_SM:  The SC value is stored in shared memory. These values are
 *                   often used to store policy parameters that need to be accessed
 *                   by multiple ION applications.
 *
 * SC_VAL_STORE_SDR: The SC value is stored on the SDR. These values often
 *                   represent values that are written to the SDR as part of
 *                   an outbound security block.
 */

typedef enum {
    SC_VAL_STORE_MEM     = 0x01,
    SC_VAL_STORE_SM      = 0x02,
    SC_VAL_STORE_SDR     = 0x04
} sc_val_store;



/*****************************************************************************
 *                                Structures                                 *
 *****************************************************************************/


/**
 * SC Value
 *
 * This structure captures a security context value (parameter or result).
 * SC values are complicated by the fact that parameters can be defined by
 * multiple actors in the BP library - some exist in shared memory while
 * others exist on the heap and others exist in the SDR.
 *
 * The SC value structure helps to insulate these different types of values
 * from the applications the operate on them.
 *
 * Values are uniquely identified by the 3-tuple of (context, type, value id)
 * where context is the security context identifier, type determines whether
 * the value represents a parameter or result, and value id is the identifier
 * within the context for that type.
 *
 * The fields of the SC Value, and their interpretation, are as follows.
 *
 * Type (scValType)
 * ---
 * The type of the value - either a security parameter or a security result.
 *
 *
 * Storage Location (scValLoc)
 * ---
 * The location of the value in ION. This is important, as the underlying
 * structures used by ION differ based on the storage location.
 *
 *
 * Identifier (scValId)
 * ---
 * Identifiers are defined by a given security context and are unique for that
 * security context. However, these identifiers are NOT globally unique -
 * different security contexts may have different definitions of the same
 * identifier value. For example, Result Id 1 for security context 1 may be
 * different than Result Id 1 for security context 2.
 *
 *
 * Length (scValLength)
 * ---
 * The length of the value itself in bytes. This refers to the length of the
 * SC value as it exists in the scRawValue union, not any type of serialized
 * or deserialied representation of the value.
 *
 *
 * Value (scRawValue)
 * ---
 * The value of the SC value, stored in its most convenient form for quick
 * use by the security context.  The format of the value is known only to the
 * security context which generates (and uses) the value. All other holders of
 * this value should treat this data as opaque.
 *
 * The value field is a union representing the various ways in which a value
 * may be stored by ION. The field of the union to be examined is determined
 * by the scValLoc enumeration.
 *
 * asAddr - The PSM Address of the shared memory object holding the raw
 *          value. This field should only be used when the location enumeration
 *          is given as: SC_VAL_STORE_SM.
 *
 * asPtr  - The local, working memory address of the object holding the raw
 *          value. This field should only be used when the location enumeration
 *          is given as: SC_VAL_STORE_MEM.
 *
 * asPtr  - The Address of the SDR Object holding the raw value. This field
 *          should only be used when the location enumertion is given as:
 *          SC_VAL_STORE_SDR.
 *
 */

typedef struct
{
    sc_val_type  scValType;    /** Whether this value is a result or a parameter. */
    sc_val_store scValLoc;     /** Where this SCI value is stored.                */
    int           scValId;     /** Value id. Unique within a security context.    */
    int           scValLength; /** Length of the value data.                      */

    union {
        PsmAddress asAddr;
        void       *asPtr;
        Object     asSdr;
    } scRawValue;              /** Value data.    Actual (not CBOE encoded)       */
} sc_value;



/**
 * One of the more complicated parts of dealing with a security context
 * is the generation and handling of values (Parameters and results).
 *
 * The sc_value_map structure provides a mechanism for associating
 * parameter and result definitions of a security context with the
 * function callbacks used to process them in a variety of ways.
 *
 * The contents of this map are as follows.
 *
 *
 * Value String Name (scValName)
 * ---
 * The string name of a value identifies instances of that value in
 * string contexts, such as when reading a policy parameter from a
 * human-readable configuration file.
 *
 *
 * Value Identifier (scValId)
 * ---
 * The security-context defined unique integer identifier for this value.
 *
 *
 * Value Type (scValType)
 * ---
 * The type of value (parameter or result).
 *
 * This is needed because value identifiers may be reused across parameters
 * and results in a single security context. For example, a security context
 * can define a parameter with scValId 1 and a result with scValId 1. A
 * security context MAY NOT define 2 parameters that both have scValId 1, or
 * two different result types with scValId 1.
 *
 *
 * Processing Callbacks
 * ---
 * A security context associates individual callbacks for common processing
 * functions associated with a value, as follows.
 *
 * bpsec_scvm_strDecode  - The security-context specific function to initialize
 *                         the data portion of a value from a string
 *                         representation.
 *
 * bpsec_scvm_clear      - The security-context specific function to clear the
 *                         contents of an SCI value. The security context which
 *                         initialized the data value is needed to understand
 *                         how to clear out the data value because, sometimes,
 *                         values may represent complex structures and not
 *                         primitive types.
 *
 * bpsec_scvm_strEncode  - The security-context specific function to generate a
 *                         string representation of the value. This string
 *                         representation is allocated using the ION heap and
 *                         must be released by the calling function.
 *
 * bpsec_scvm_cborDecode - The security-context specific function to initialize
 *                         a value from a security parameter or security results
 *                         read from an inbound security block. Values in blocks
 *                         are always encoded in CBOR.
 *
 * bpsec_scvm_cborEncode - The security-context specific function to generate a
 *                         version of the value encoded to be included in an
 *                         outbound security block. Values in blocks are always
 *                         encoded in CBOR.
 *
 */

typedef int      (*bpsec_scvm_strDecode)  (PsmPartition wm, sc_value *val, unsigned int len, char *value);
typedef void     (*bpsec_scvm_clear)      (PsmPartition wm, sc_value *val);
typedef char*    (*bpsec_scvm_strEncode)  (PsmPartition wm, sc_value *val);
typedef int      (*bpsec_scvm_cborDecode) (PsmPartition wm, sc_value *val, unsigned int len, uint8_t *value);
typedef uint8_t* (*bpsec_scvm_cborEncode) (PsmPartition wm, sc_value *val, unsigned int *len);


typedef struct
{
    char                 *scValName;
    int                   scValId;
    sc_val_type           scValType;
    bpsec_scvm_strDecode  scValFromStr;
    bpsec_scvm_clear      scValClear;
    bpsec_scvm_strEncode  scValToStr;
    bpsec_scvm_cborEncode scValToCBOR;
    bpsec_scvm_cborDecode scValFromCBOR;
} sc_value_map;


/**
 * Security Context State
 *
 * A BPSec security block consists of a set of one or more security operations
 * sharing some common security context.
 *
 * The security context structure captures the information that a BPA must
 * track when processing a security block, to include the information shared
 * for all security operations, and information related to the current security
 * operation being processed.
 *
 * A BPA instantiates one of these SC states for every security block that
 * must be created or otherwise processed.  It is this state that is used to
 * generate (or process) security results for each target of a security block.
 *
 * A state is initialized when starting to process a security block. Once a
 * security operation is processed, the results of that operation will be
 * available in the results field of the state.
 *
 * When a state is incremented, metrics in the state structure are updated and
 * any results associated with the current security operation are reset to hold
 * results for the next security operation in the security block.
 *
 * Because this processing happens in the context of a single thread of
 * control (within the BPA), any memory specific to the state itself is kept
 * in local memory. However, the parameters and results may, themselves, exist
 * in shared memory, local memory, or the SDR as a function of whether they are
 * sources from security policy, an inbound security block, or an outbound
 * security block.
 *
 * Within ION:
 *   - Policy configurations (such as policy parameters) exist in shared memory
 *   - Inbound security block information exists in local memory
 *   - Outbound security block information exists on the SDR.
 *
 */
// TODO migrate security role enums from policy into this space.
// TODO Document why we carry a raw key.
typedef struct
{
    PsmPartition  scStWm;       /** Partition associated with this state.   */
    Sdr           sdr;          /** SDR associated with outbound blocks.    */
    unsigned char scSecBlkNum;  /** Security block being processed.         */
    int           scStId;       /** Id of the SC using this state.          */
    sc_role       scRole;       /** Security role (source, verifier, acceptor */
    sc_action     scStAction;   /** The security service action.            */
    int           scStSize;     /** Max size for data into update function. */
    sc_status     scStStatus;   /** Processing status.                      */
    int           scStCurTgt;   /** The current target being processed.     */
    int           scStTotTgts;  /** Number of targets in security block.    */
    EndpointId    scStSource;   /** Security source for the security block. */
    Lyst          scStParms;    /** (sc_value*) Security context parms.     */
    Lyst          scStResults;  /** (sc_value*) Current op results.         */
    sc_value      scRawKey;     /** Current unwrapped key value, is applicable. */
} sc_state;





#endif /* _SCI_STRUCTS_H_ */
