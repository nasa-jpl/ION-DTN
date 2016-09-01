/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
/*****************************************************************************
 **
 ** \file lit.h
 **
 **
 ** Description: This file contains the structures and operations associated
 **              with literal definitions.
 **
 ** Notes:
 **   (1) Parameterized literals are stored in a parameterless state as they are
 **       defined as taking parameters, but the definition of course) does not
 **       include any given set or instance of parameters.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef LIT_H_
#define LIT_H_

#include "../primitives/value.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */



/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Value function for Literals.
 *
 * The value function takes a literal definition and produces an expression
 * result that captures the value of the literal.
*/
typedef value_t (*lit_val_fn)(mid_t *id);


/**
 * Describes a Literal Data Definition.
 *
 * Notably with this structure there are two ways to capture the value
 * of the literal, depending on whether the literal is parameterized
 * or not.
 *
 * If the literal is not parameterized, then its value is pre-computed
 * and store as a result in this data structure.
 *
 * If the liteal is parameterized, the MID definition (sans parameters)
 * is stored in this structure along with a function for building a
 * value based on given parameters.
 */
typedef struct {
    mid_t *id;             /**> The identifier (name) of the literal. */

    value_t value;   /**> The literal value, if not parameterized. */

    lit_val_fn calc;       /**> Function to build value from parm def. */
} lit_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

lit_t *lit_create(mid_t *id, value_t value, lit_val_fn calc);

lit_t *lit_find_by_id(Lyst list, ResourceLock *mutex, mid_t *id);

value_t lit_get_value(lit_t *lit);

void lit_lyst_clear(Lyst *list, ResourceLock *mutex, int destroy);

void lit_release(lit_t *lit);

char *lit_to_tring(lit_t *def);


#endif /* LIT_H_ */
