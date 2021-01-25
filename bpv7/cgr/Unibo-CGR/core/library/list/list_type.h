/** \file list_type.h
 *
 * \brief This file provides the definition of a List element and of
 *        a ListElt element.
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

#ifndef SOURCES_LIBRARY_LIST_LIST_TYPE_H_
#define SOURCES_LIBRARY_LIST_LIST_TYPE_H_

typedef struct nodeElt ListElt;
typedef struct list *List;

typedef int (*compare_function)(void*, void*);
typedef void (*delete_function)(void*);

struct nodeElt
{
	/**
	 * \brief The list to which this ListElt belongs
	 */
	List list;
	/**
	 * \brief The previous element in the list
	 */
	ListElt *prev;
	/**
	 * \brief The next element in the list
	 */
	ListElt *next;
	void *data;
};

struct list
{
	/**
	 * \brief The data to which this list belongs (back-reference)
	 */
	void *userData;
	/**
	 * \brief The first element in the list
	 */
	ListElt *first;
	/**
	 * \brief The last element in the list
	 */
	ListElt *last;
	/**
	 * \brief Number of elements in the list
	 */
	long unsigned int length;
	/**
	 * \brief Compare the data field of the ListElt in the list
	 */
	compare_function compare;
	/**
	 * \brief The function called to delete the data field of the ListElts in the list
	 */
	delete_function delete_data_elt;
	/**
	 * \brief The function called to delete the userData of the list
	 */
	delete_function delete_userData;
};

#endif /* SOURCES_LIBRARY_LIST_LIST_TYPE_H_ */
