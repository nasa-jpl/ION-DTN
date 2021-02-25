/************************************************************
 * \file scalar.c
 *
 * \brief  This file provides a full implementation of a library
 *         to manage the CgrScalar type.
 *
 * \par Ported from ION 3.7.0 by
 *      Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 * 
 * \par Supervisor
 *      Carlo Caini, carlo.caini@unibo.it
 *
 ***************************************************************/

#include "scalar.h"

#include <stdlib.h>

/******************************************************************************
 *
 * \par Function Name:
 *      loadCgrScalar
 *
 * \brief Set a CgrScalar equals to an integer
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[out]  *s   The CgrScalar that we want to initialize
 * \param[in]    i   The integer used to initialize "s"
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void loadCgrScalar(CgrScalar *s, long int i)
{
	if (s != NULL)
	{
		if (i < 0)
		{
			i = 0 - i;
		}

		s->gigs = 0;
		s->units = i;
		while (s->units >= ONE_GIG)
		{
			s->gigs++;
			s->units -= ONE_GIG;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      increaseCgrScalar
 *
 * \brief Increase a CgrScalar by an integer.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in, out]  *s   The CgrScalar that we want to increase, initially the augend
 *                       at the end the sum
 * \param[in]        i   The addend
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void increaseCgrScalar(CgrScalar *s, long int i)
{
	if (s != NULL)
	{
		if (i < 0)
		{
			i = 0 - i;
		}

		while (i >= ONE_GIG)
		{
			i -= ONE_GIG;
			s->gigs++;
		}

		s->units += i;
		while (s->units >= ONE_GIG)
		{
			s->gigs++;
			s->units -= ONE_GIG;
		}
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      reduceCgrScalar
 *
 * \brief Reduce a CgrScalar by an integer.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in, out]  *s   The CgrScalar that we want to reduce, initially the minuend
 *                       at the end the difference
 * \param[in]        i   The subtrahend
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void reduceCgrScalar(CgrScalar *s, long int i)
{
	if (s != NULL)
	{
		if (i < 0)
		{
			i = 0 - i;
		}

		while (i >= ONE_GIG)
		{
			i -= ONE_GIG;
			s->gigs--;
		}

		while (i > s->units)
		{
			s->units += ONE_GIG;
			s->gigs--;
		}

		s->units -= i;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      multiplyCgrScalar
 *
 * \brief Multiply a CgrScalar by an integer.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in, out]  *s   The CgrScalar that we want to multiply, initially the multiplicand
 *                       at the end the product
 * \param[in]        i   The multiplier
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void multiplyCgrScalar(CgrScalar *s, long int i)
{
	double product;

	if (s != NULL)
	{
		if (i < 0)
		{
			i = 0 - i;
		}

		product = (double) (((s->gigs * ONE_GIG) + (s->units)) * i);
		s->gigs = (long int) (product / ONE_GIG);
		s->units = (long int) (product - ((double) (s->gigs * ONE_GIG)));
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      divideCgrScalar
 *
 * \brief Divide a CgrScalar by an integer.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in, out]  *s   The CgrScalar that we want to divide, initially the dividend
 *                       at the end the quotient
 * \param[in]        i   The divider
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void divideCgrScalar(CgrScalar *s, long int i)
{
	double quotient;

	if (s != NULL && i != 0)
	{
		if (i < 0)
		{
			i = 0 - i;
		}

		quotient = (double) (((s->gigs * ONE_GIG) + s->units) / i);
		s->gigs = (int) (quotient / ONE_GIG);
		s->units = (int) (quotient - (((double) (s->gigs)) * ONE_GIG));
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      copyCgrScalar
 *
 * \brief Copy the fields of a CgrScalar to another CgrScalar.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[out]  *to    The CgrScalar to which we want to copy the fields of "from"
 * \param[in]   *from  The CgrScalar for which we want to "duplicate" the fields.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void copyCgrScalar(CgrScalar *to, CgrScalar *from)
{
	if (to != NULL && from != NULL)
	{
		to->gigs = from->gigs;
		to->units = from->units;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      addToCgrScalar
 *
 * \brief Add into a CgrScalat the quantity contained into another CgrScalar.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in,out]  *s           The CgrScalar for which we want to add the quantity "increment",
 *                              initially the augend at the end the sum
 * \param[in]      *increment   The CgrScalar that contains the quantity to increment in "s" (the addend)
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void addToCgrScalar(CgrScalar *s, CgrScalar *increment)
{
	if (s != NULL && increment != NULL)
	{
		increaseCgrScalar(s, increment->units);
		s->gigs += increment->gigs;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      subtractFromCgrScalar
 *
 * \brief Decrement a CgrScalar by the quantity contained in another CgrScalar
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return void
 *
 * \param[in, out]  *s           The CgrScalar that we want to decrement, initially the minuend
 *                               at the end the difference
 * \param[in]       *decrement   The CgrScalar that contains the quantity to decrement from s (the subtrahend)
 *
 * \warning At the end "s" could be negative.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
void subtractFromCgrScalar(CgrScalar *s, CgrScalar *decrement)
{
	if (s != NULL && decrement != NULL)
	{
		reduceCgrScalar(s, decrement->units);
		s->gigs -= decrement->gigs;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      CgrScalarIsValid
 *
 * \brief Check that "s" is a non-negative scalar.
 *
 *
 * \par Date Written:
 *      XX/XX/XX
 *
 * \return int
 *
 * \retval   1  s is a non-negative scalar, so it's valid.
 * \retval   0  s is a negative scalar
 * \retval  -1  s is NULL
 *
 * \param[in]  *s   The CgrScalar for which we want to check the validity.
 *
 * \par Revision History:
 *
 *  DD/MM/YY | AUTHOR          |  DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  30/01/20 | L. Persampieri  |  Ported function.
 *******************************************************************************/
int CgrScalarIsValid(CgrScalar *s)
{
	int result = -1;
	if (s != NULL)
	{
		result = (s->gigs >= 0);
	}

	return result;
}
