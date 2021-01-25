/** \file list.c
 *
 *  \brief  Implementation of a list library
 *
 ** \copyright Copyright (c) 2020, Alma Mater Studiorum, University of Bologna, All rights reserved.
 **
 ** \par License
 **
 **    This file is part of Unibo-CGR.                                            <br>
 **                                                                               <br>
 **    Unibo-CGR is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published by
 **    the Free Software Foundation, either version 3 of the License, or
 **    (at your option) any later version.                                        <br>
 **    Unibo-CGR is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **    GNU General Public License for more details.                               <br>
 **                                                                               <br>
 **    You should have received a copy of the GNU General Public License
 **    along with Unibo-CGR.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  \author Lorenzo Persampieri, lorenzo.persampieri@studio.unibo.it
 *
 *  \par Supervisor
 *       Carlo Caini, carlo.caini@unibo.it
 */

#include "list.h"

#include <stdlib.h>
#include <stdio.h>

static void sort_list_algorithm(List, compare_function);
static void erase_list(List);

/******************************************************************************
 *
 * \par Function Name:
 *      list_create
 *
 * \brief Allocate memory for a list structure
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return List
 *
 * \retval List  A pointer to the list structure
 * \retval NULL  MWITHDRAW error
 *
 * \param[in]  userData         The userData field of the new list.
 * \param[in]  delete_userData  The function to Delete the userData, or NULL
 * \param[in]  compare          The function to compare the elements of the list
 * \param[in]  delete_data_elt  The function to Delete the elements of the list
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
List list_create(void *userData, delete_function delete_userData, compare_function compare,
		delete_function delete_data_elt)
{
	List new_list = (List) MWITHDRAW(sizeof(struct list));

	if (new_list != NULL)
	{
		new_list->userData = userData;
		new_list->delete_userData = delete_userData;
		new_list->compare = compare;
		new_list->delete_data_elt = delete_data_elt;
		new_list->first = NULL;
		new_list->last = NULL;
		new_list->length = 0;
	}

	return new_list;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_userData
 *
 * \brief Get the userData field of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void*
 *
 * \retval void*   The pointer to the userData field
 * \retval NULL    userData NULL
 *
 * \param[in]  list   The list from which we want to get the userdata field
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void* list_get_userData(List list)
{
	void *userData = NULL;

	if (list != NULL)
	{
		userData = list->userData;
	}

	return userData;
}

/******************************************************************************
 *
 * \par Function Name: list_get_length
 *
 * \brief Get the length field of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return long unsigned int
 *
 * \retval ">= 0"  The length field of the list
 *
 * \param[in]  list  The list from which we want to get the length field
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
long unsigned int list_get_length(List list)
{
	long unsigned int length = 0;

	if (list != NULL)
	{
		length = list->length;
	}

	return length;
}

/******************************************************************************
 *
 * \par Function Name:
 *      move_elt_to_other_list
 *
 * \brief Move a ListElt from a list to other list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return int
 *
 * \retval   0   Success
 * \retval  -1   Parameters not initialized correctly
 *
 * \param[in]  *elt		The element we want to move
 * \param[in]	other	The list where we want to move elt
 *
 * \par Notes:
 * 		1. In success case: elt->list == other, other->first == elt
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int move_elt_to_other_list(ListElt *elt, List other)
{

	ListElt *next_elt, *prev_elt;
	List eltList;
	int result = -1;
	if (elt != NULL && other != NULL && elt->list != NULL)
	{
		eltList = elt->list;
		next_elt = elt->next;
		prev_elt = elt->prev;
		if (next_elt == NULL)
		{
			eltList->last = prev_elt;
		}
		else
		{
			next_elt->prev = prev_elt;
		}

		if (prev_elt == NULL)
		{
			eltList->first = next_elt;
		}
		else
		{
			prev_elt->next = next_elt;
		}

		eltList->length -= 1;

		elt->prev = NULL;
		if (other->first == NULL)
		{
			other->last = elt;
		}
		else
		{
			other->first->prev = elt;
		}
		elt->next = other->first;
		other->first = elt;

		other->length += 1;
		elt->list = other;

		result = 0;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      move__a_elt__before__b_elt
 *
 * \brief Move a_elt before b_elt. a_elt will be the "previous" element of b_elt
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return int
 *
 * \retval   0   Success
 * \retval  -1   Parameters not initialized correctly
 *
 * \param[in]  *a_elt	The element we want to move
 * \param[in]  *b_elt	the element we want to be preceded by a_elt
 *
 * \par Notes:
 * 		1. Note that a_elt and b_elt can be part of different lists,
 * 		   but also of the same one
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
int move__a_elt__before__b_elt(ListElt *a_elt, ListElt *b_elt)
{
	ListElt *prev_a_elt, *next_a_elt, *prev_b_elt;
	List a, b;
	int result = -1;
	if (a_elt != NULL && b_elt != NULL && a_elt != b_elt)
	{
		a = a_elt->list;
		b = b_elt->list;

		if (a != NULL && b != NULL)
		{

			prev_a_elt = a_elt->prev;
			next_a_elt = a_elt->next;
			if (next_a_elt != NULL)
			{
				next_a_elt->prev = prev_a_elt;
			}
			else
			{
				a->last = prev_a_elt;
			}
			if (prev_a_elt != NULL)
			{
				prev_a_elt->next = next_a_elt;
			}
			else
			{
				a->first = next_a_elt;
			}
			a->length -= 1;

			prev_b_elt = b_elt->prev;

			a_elt->next = b_elt;
			a_elt->prev = prev_b_elt;
			b_elt->prev = a_elt;
			if (prev_b_elt == NULL)
			{
				b->first = a_elt;
			}
			else
			{
				prev_b_elt->next = a_elt;
			}

			a_elt->list = b;

			b->length += 1;

			result = 0;
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_first_elt
 *
 * \brief Get the first element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The first element of the list
 * \retval NULL		 Empty list.
 *
 * \param[in]  list  The list from which we want to get the first element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_get_first_elt(List list)
{
	ListElt *elt = NULL;
	if (list != NULL)
	{
		elt = list->first;
	}

	return elt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_next_elt
 *
 * \brief Get the next element of elt
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The next element of elt (elt->next)
 * \retval NULL		 If elt is the last element of the list
 *
 * \param[in]  *elt  The element from which we want to get the next element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_get_next_elt(ListElt *elt)
{
	ListElt *result = NULL;
	if (elt != NULL)
	{
		result = elt->next;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_prev_elt
 *
 * \brief Get the prev element of elt
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*   The prev element of elt (elt->prev)
 *
 * \param[in]  *elt   The element from which we want to get the prev element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_get_prev_elt(ListElt *elt)
{
	ListElt *result = NULL;
	if (elt != NULL)
	{
		result = elt->prev;
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_last_elt
 *
 * \brief Get the last element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The last element of the list (list->last)
 * \retval NULL      Empty list
 *
 * \param[in]  list  The list from which we want to get the last element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_get_last_elt(List list)
{
	ListElt *elt = NULL;
	if (list != NULL)
	{
		elt = list->last;
	}

	return elt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_first_equal_element
 *
 * \brief Get the first equal element found
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The first equal element found
 * \retval NULL      Equal element not found
 *
 * \param[in]  list         The list from which we want to get the equals element
 * \param[in]  *data        The data we want to search in the list
 * \param[in]  compare      The compare function we want to use to find the data in the list
 * \param[in]  *startElt    The start element of the list from which we want to search the data,
 *                          or NULL if we want to start from list->first
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_get_first_equal_element(List list, void *data, compare_function compare,
		ListElt *startElt)
{
	ListElt *current, *from, *result = NULL;
	int equals_elements = -1;

	if (data != NULL && compare != NULL && (list != NULL || startElt != NULL))
	{
		if (startElt != NULL)
		{
			from = startElt;
		}
		else
		{
			from = list->first;
		}

		for (current = from; current != NULL && equals_elements != 0; current = current->next)
		{
			equals_elements = compare(current->data, data);

			if (equals_elements == 0)
			{
				result = current;
			}
		}
	}

	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_get_equals_elements
 *
 * \brief Get a list of equals elements of data
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return List
 *
 * \retval List  A new allocated list of equals elements of data.
 * \retval NULL  MWITHDRAW or arguments error (Arguments NULL)
 *
 * \param[in]  list    The list from which we want to get the equals elements
 * \param[in]  *data   The data we want to search in the list
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *      2. list->compare here must be != NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
List list_get_equals_elements(List list, void *data)
{
	List result = NULL;
	ListElt *current = NULL;

	if (data != NULL && list != NULL)
	{
		if (list->compare != NULL)
		{
			result = list_create(list->userData, list->delete_userData, list->compare,
					list->delete_data_elt);
			if (result != NULL)
			{
				do
				{
					current = list_get_first_equal_element(list, data, list->compare, current);
					if (current != NULL)
					{
						list_insert_last(result, current->data);
						current = current->next;
					}
				} while (current != NULL);

				if (result->length == 0)
				{
					free_list(result);
					result = NULL;
				}
			}
		}
	}
	return result;
}

/******************************************************************************
 *
 * \par Function Name:
 *      listElt_get_data
 *
 * \brief Get the data pointed from elt
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void*
 *
 * \retval void*  The data pointed from elt (elt->data)
 * \retval NULL   data not found or elt NULL
 *
 * \param[in]  *elt   The element from which we want to get the data
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void* listElt_get_data(ListElt *elt)
{
	void *data = NULL;

	if (elt != NULL)
	{
		data = elt->data;
	}

	return data;
}

/******************************************************************************
 *
 * \par Function Name:
 *      listElt_get_list
 *
 * \brief Get the list to which elt belongs
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return List
 *
 * \retval List  The list to which elt belongs (elt->list)
 * \retval NULL  List not found
 *
 * \param[in]  *elt  The element from which we want to get the list
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
List listElt_get_list(ListElt *elt)
{
	List list = NULL;

	if (elt != NULL)
	{
		list = elt->list;
	}

	return list;
}

/******************************************************************************
 *
 * \par Function Name:
 *      erase_elt
 *
 * \brief Erase all the elt's fields
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  *elt  The element from which we want to erase the fields
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void erase_elt(ListElt *elt)
{
	if (elt != NULL)
	{
		elt->data = NULL;
		elt->list = NULL;
		elt->next = NULL;
		elt->prev = NULL;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      erase_list
 *
 * \brief Erase all the list's fields
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list from which we want to erase the fields
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void erase_list(List list)
{
	if (list != NULL)
	{
		list->first = NULL;
		list->last = NULL;
		list->length = 0;
		list->userData = NULL;
		list->compare = NULL;
		list->delete_data_elt = NULL;
		list->delete_userData = NULL;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      free_list
 *
 * \brief Remove all the element belonging to the list and the list itself
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list we want to be "freed"
 *
 * \par Notes:
 *      1. The delete_userData will be called, if not NULL
 *      2. The delete_data_elt will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void free_list(List list)
{
	if (list != NULL)
	{
		free_list_elts(list);

		if (list->delete_userData != NULL)
		{
			list->delete_userData(list->userData);
		}

		erase_list(list);
		MDEPOSIT(list);

		list = NULL;
	}

}

/******************************************************************************
 *
 * \par Function Name:
 *      free_list_elts
 *
 * \brief Remove all the element belonging to the list but not the list itself
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list for which we want that all the element will be deleted
 *
 * \par Notes:
 *      1. The delete_data_elt will be called, if not NULL
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void free_list_elts(List list)
{
	ListElt *temp, *elt;

	if (list != NULL)
	{
		elt = list->first;
		if (list->delete_data_elt != NULL)
		{
			while (elt != NULL)
			{
				if (elt->data != NULL)
				{
					list->delete_data_elt(elt->data);
				}
				temp = elt;
				elt = temp->next;
				erase_elt(temp);
				MDEPOSIT(temp);
			}
		}
		else
		{
			while (elt != NULL)
			{
				temp = elt;
				elt = elt->next;
				erase_elt(temp);
				MDEPOSIT(temp);
			}
		}

		list->first = NULL;
		list->last = NULL;
		list->length = 0;
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_insert_first
 *
 * \brief Insert an element, that will point to data, as the first element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The inserted element
 * \retval NULL      MWITHDRAW error
 *
 * \param[in]  list   The list where we want to add the element
 * \param[in]  data   The data that will be pointed by the element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_insert_first(List list, void *data)
{

	ListElt *first = NULL;
	ListElt *oldFirst;
	if (list != NULL && data != NULL)
	{
		first = (ListElt*) MWITHDRAW(sizeof(ListElt));

		if (first != NULL)
		{
			first->list = list;
			oldFirst = list->first;
			first->next = oldFirst;
			first->prev = NULL;
			first->data = data;

			list->first = first;
			list->length += 1;
			if (list->last == NULL)
			{
				list->last = first;
			}
			else //the list had at least one element
			{
				oldFirst->prev = list->first;
			}
		}
	}

	return first;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_insert_last
 *
 * \brief Insert an element, that will point to data, as the last element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The inserted element
 * \retval NULL      MWITHDRAW error
 *
 * \param[in]  list  The list where we want to add the element
 * \param[in]  data  The data that will be pointed by the element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_insert_last(List list, void *data)
{
	ListElt *last = NULL;
	ListElt *oldLast;

	if (list != NULL && data != NULL)
	{

		last = (ListElt*) MWITHDRAW(sizeof(ListElt));

		if (last != NULL)
		{
			last->list = list;
			last->next = NULL;
			oldLast = list->last;
			last->prev = oldLast;
			last->data = data;

			list->last = last;
			list->length += 1;

			if (list->first == NULL)
			{
				list->first = last;
			}
			else //the list had at least one element
			{
				oldLast->next = list->last;
			}
		}
	}
	return last;

}

/******************************************************************************
 *
 * \par Function Name:
 *      list_remove_first
 *
 * \brief Remove (and MDEPOSIT_wrapper) the first element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list from which we want to remove the first element
 *
 * \par Notes:
 *      1. The delete_data_elt will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void list_remove_first(List list)
{
	ListElt *first = NULL;

	if (list != NULL && list->first != NULL)
	{
		first = list->first;
		list->first = first->next;
		list->length -= 1;

		if (list->first == NULL)
		{
			list->last = NULL;
		}
		else
		{
			list->first->prev = NULL;
		}

		if (first->data != NULL && list->delete_data_elt != NULL)
		{
			list->delete_data_elt(first->data);
		}
		erase_elt(first);
		MDEPOSIT(first);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_remove_last
 *
 * \brief Remove (and MDEPOSIT_wrapper) the last element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list from which we want to remove the last element
 *
 * \par Notes:
 *      1. The delete_data_elt will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void list_remove_last(List list)
{
	ListElt *last = NULL;

	if (list != NULL && list->last != NULL)
	{
		last = list->last;
		list->last = last->prev;
		list->length -= 1;

		if (list->last == NULL)
		{
			list->first = NULL;
		}
		else
		{
			list->last->next = NULL;
		}

		if (last->data != NULL && list->delete_data_elt != NULL)
		{
			list->delete_data_elt(last->data);
		}

		erase_elt(last);
		MDEPOSIT(last);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_insert_before
 *
 * \brief Insert an element, that will point to data, before the "target"
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*   The inserted element
 * \retval NULL       MWITHDRAW or arguments error (arguments NULL)
 *
 * \param[in]  *target   The element that we want to be preceded by the new element that point to data
 * \param[in]  *data     The data pointed by the new inserted element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_insert_before(ListElt *target, void *data)
{
	ListElt *elt = NULL;
	List list;

	if (target != NULL && data != NULL)
	{
		list = target->list;

		if (list != NULL)
		{
			elt = (ListElt*) MWITHDRAW(sizeof(ListElt));
			if (elt != NULL)
			{
				list->length += 1;

				elt->data = data;
				elt->list = list;
				elt->next = target;
				elt->prev = target->prev;

				target->prev = elt;

				if (elt->prev != NULL)
				{
					elt->prev->next = elt;
				}
				else //target was the first element of the list
				{
					list->first = elt;
				}
			}
		}
	}

	return elt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_insert_after
 *
 * \brief Insert an element, that will point to data, after the "target"
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return ListElt*
 *
 * \retval ListElt*  The inserted element
 * \retval NULL      MWITHDRAW or arguments error (arguments NULL)
 *
 * \param[in]  *target   The element that we want the new element (that point to data) is the successor.
 * \param[in]  *data     The data pointed by the new inserted element
 *
 * \par Notes:
 *      1. You must check that the return value of this function is not NULL.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_insert_after(ListElt *target, void *data)
{
	ListElt *elt = NULL;
	List list;

	if (target != NULL && data != NULL)
	{
		list = target->list;

		if (list != NULL)
		{
			elt = (ListElt*) MWITHDRAW(sizeof(ListElt));
			if (elt != NULL)
			{
				list->length += 1;

				elt->data = data;
				elt->list = list;
				elt->prev = target;
				elt->next = target->next;

				target->next = elt;

				if (elt->next != NULL)
				{
					elt->next->prev = elt;
				}
				else //target was the last element of the list
				{
					list->last = elt;
				}
			}
		}
	}

	return elt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_remove_elt
 *
 * \brief Remove the element from the list (elt->list, if any) and MDEPOSIT that element
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  *elt  The element that we want to remove
 *
 * \par Notes:
 *      1. The delete_data_elt will be called, if not NULL
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void list_remove_elt(ListElt *elt)
{
	List list;

	if (elt != NULL)
	{
		list = elt->list;
		if (list != NULL)
		{
			if (elt->prev != NULL)
			{
				elt->prev->next = elt->next;
			}
			if (elt->next != NULL)
			{
				elt->next->prev = elt->prev;
			}

			list->length -= 1;

			if (list->first == elt)
			{
				list->first = elt->next;
			}
			if (list->last == elt)
			{
				list->last = elt->prev;
			}

			if (list->delete_data_elt != NULL && elt->data != NULL)
			{
				list->delete_data_elt(elt->data);
			}
		}

		erase_elt(elt);
		MDEPOSIT(elt);
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_remove_elt_by_data
 *
 * \brief Remove the element, that its data field is equals to data (if any), from the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list   The list from which we want to remove the element
 * \param[in]  *data  The data that has to be equals by the data pointed from an element of the list
 *
 * \par Notes:
 *      1. The delete_data_elt will be called, if not NULL
 *      2. data could be freed if the found element point exactly to this data,
 *         this depends on delete_data_elt.
 *      3. If the list->compare is not NULL we will check the equality with this function,
 *         but two element are also equals if they point to the same data.
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void list_remove_elt_by_data(List list, void *data)
{
	ListElt *elt = list_search_elt_by_data(list, data);
	if (elt != NULL)
	{
		list_remove_elt(elt);
	}
}

/******************************************************************************
 *
 * \par Function Name:
 *      list_search_elt_by_data
 *
 * \brief Search the element, that its data field is equals to data (if any), in the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list    The list from which we want to find the element
 * \param[in]  *data   The data that has to be equals by the data pointed from an element of the list
 *
 * \par Notes:
 *			1. If the list->compare is not NULL we will check the equality with this function,
 *        	   but two element are also equals if they point to the same data.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
ListElt* list_search_elt_by_data(List list, void *data)
{
	ListElt *elt = NULL, *temp;
	int found = 0;

	if (list != NULL && data != NULL)
	{
		if (list->compare != NULL)
		{
			for (temp = list->first; temp != NULL && !found; temp = temp->next)
			{
				if (temp->data == data || list->compare(temp->data, data) == 0)
				{
					elt = temp;
					found = 1; //esco dal ciclo
				}
			}
		}
		else
		{
			for (temp = list->first; temp != NULL && !found; temp = temp->next)
			{
				if (temp->data == data)
				{
					elt = temp;
					found = 1; //esco dal ciclo
				}
			}
		}
	}

	return elt;
}

/******************************************************************************
 *
 * \par Function Name:
 *      sort_list_algorithm
 *
 * \brief Sort the element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list     The list from which we want to sort the element
 * \param[in]  compare  The compare function that will be used to sort the element of the list
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
static void sort_list_algorithm(List list, compare_function compare)
{
	/* BUBBLE SORT */
	/* For security (although more expensive) I decided to order the list by swapping the prev and next pointers
	 * keeping the data's pointer untouched, so any pointers external to the element x (be it ListElt or the data) of the list
	 * in any case point to the correct element
	 *
	 * NOTE: by exchanging the date field (more efficient) any pointers external to the ListElt element
	 * no longer point to the data they believe they are pointing
	 */

	ListElt *current, *upper_bound, *old_ub_next;
	ListElt *lower_elt, *higher_elt, *temp_elt;
	int sorted = 0, current_is_greater;

	upper_bound = list->last;
	if (upper_bound != NULL)
	{
		old_ub_next = upper_bound->next;
		while (!sorted)
		{
			sorted = 1;
			current = list->first;
			while (current != upper_bound && current->next != NULL)
			{
				temp_elt = current->next;
				current_is_greater = compare(current->data, temp_elt->data);
				if (current_is_greater > 0) //swap
				{
					/*								current state:										  */
					/* list->first -> ... -> lower_elt -> current -> temp_elt -> higher_elt -> ... -> list->last */

					lower_elt = current->prev;
					if (lower_elt != NULL)
					{
						lower_elt->next = temp_elt;
					}
					else
					{ /* current is the first element of the list	*/
						list->first = temp_elt;
					}

					higher_elt = temp_elt->next;
					if (higher_elt != NULL)
					{
						higher_elt->prev = current;
					}
					else
					{/*	current->next is the last element of the lists	*/
						list->last = current;
					}

					current->prev = temp_elt;
					current->next = higher_elt;

					temp_elt->prev = lower_elt;
					temp_elt->next = current;

					//swapped
					sorted = 0;
				}
				else /* not swapped */
				{
					current = temp_elt;
				}
			} //while(current!=upper_bound)

			if (old_ub_next == upper_bound->next)
			{
				upper_bound = upper_bound->prev;
			}
		} // while(!sorted)
	}
	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      sort_list
 *
 * \brief Sort the element of the list
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  list  The list from which we want to sort the element
 *
 * \par Notes:
 *      1. The compare function (list->compare) has to be != NULL.
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void sort_list(List list)
{
	if (list != NULL)
	{
		if (list->compare != NULL)
		{
			sort_list_algorithm(list, list->compare);
		}
	}

	return;
}

/******************************************************************************
 *
 * \par Function Name:
 *      remove_secondList_from_firstLists
 *
 * \brief Remove all the elements of the firstList that match with the element of the secondList
 *
 *
 * \par Date Written:
 *      07/01/20
 *
 * \return void
 *
 * \param[in]  firstList   The list from which we want to remove the elements
 * \param[in]  secondList  The list that contains the element that we want to remove from the firstList
 *
 * \par Notes:
 * 		1. The element are compared by the compare of the firstList (firstList->compared)-
 *      2. The compare function (firstList->compare) of the firstList must be not NULL.
 *      3. The delete_data_elt will be called, if not NULL
 *
 *
 * \par Revision History:
 *
 *  DD/MM/YY |  AUTHOR         |   DESCRIPTION
 *  -------- | --------------- | -----------------------------------------------
 *  07/01/20 | L. Persampieri  |  Initial Implementation and documentation.
 *****************************************************************************/
void remove_secondList_from_firstList(List firstList, List secondList)
{
	ListElt *currentFirstList, *currentSecondList, *temp;
	int found, compare_result;
	compare_function compare;
	if (firstList != NULL && secondList != NULL)
	{
		if (firstList->compare != NULL)
		{
			compare = firstList->compare;

			for (currentSecondList = secondList->first; currentSecondList != NULL;
					currentSecondList = currentSecondList->next)
			{
				found = 0;
				for (currentFirstList = firstList->first; currentFirstList != NULL && !found;
						currentFirstList = currentFirstList->next)
				{
					compare_result = compare(currentFirstList->data, currentSecondList->data);

					if (compare_result == 0)
					{
						temp = currentFirstList;
						found = 1;
					}
				}

				if (found == 1)
				{
					list_remove_elt(temp);
				}
			}
		}
	}
}
