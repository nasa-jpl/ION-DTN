/*
 * Copyright (c) Antara Teknik, LLC. All rights reserved.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/*! \file array.h
    \brief Helper functions for dealing with fixed size arrays
 */

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \def TARA_ARRAY_TYPE
    \brief Declares a fixed size array with an underlying type
    \param type The underlying type of the list
    \param name The name of the list
    \param sz The size of the array

    The declared array type will be tara_<name>_array_t.
 */
#define TARA_ARRAY_TYPE(type, name, sz) \
    typedef struct tara_##name##_array  \
    {                                   \
        size_t size;                    \
        type data[sz];                  \
    } tara_##name##_array_t

/*! \fn tara_array_init
    \brief Initializes the array for first use
    \param array The array to initialize
 */
#define tara_array_init(array) (array).size = 0;

/*! \fn tara_array_clear
    \brief Clears all elements from the array
    \param array The array to clear
 */
#define tara_array_clear(array) (array).size = 0;

/*! \fn tara_array_max_size
    \brief Returns the maximum size of the array
    \param array The array to check
    \return The maximum size of the array
 */
#define tara_array_max_size(array) (sizeof (array).data / sizeof (array).data[0])

/*! \fn tara_array_size
    \brief Returns the current size of the array
    \param array The array to check
    \return The current size of the array
 */
#define tara_array_size(array) (array).size

/*! \fn tara_array_element_size
    \brief Returns the size of the array's element type
    \param array The array to check
    \return The size of the array's element type
 */
#define tara_array_element_size(array) (sizeof (array).data[0])

/*! \fn tara_array_resize
    \brief Resizes the array up to max_size
    \param array The array to resize
    \return The new size of the array

    Resizing does not modify the elements, only where the end of the array is.
 */
#define tara_array_resize(array, new_size) \
    ((array).size = (new_size), \
     (array).size = (array).size <= tara_array_max_size(array) ? (array).size : tara_array_max_size(array), \
     (array).size)

/*! \fn tara_array_empty
    \brief Checks if the array is empty
    \param array The array to check
    \return TRUE if empty, and FALSE if not
 */
#define tara_array_empty(array) (0 == (array).size)

/*! \fn tara_array_full
    \brief Checks if the array is full
    \param array The array to check
    \return TRUE if full, and FALSE if not
 */
#define tara_array_full(array) (tara_array_max_size(array) <= (array).size)

/*! \fn tara_array_begin
    \brief Returns a pointer to the first element in the array
    \param array The array to access
    \return A pointer to the first element, or NULL if empty
 */
#define tara_array_begin(array) (array).data

/*! \fn tara_array_end
    \brief Returns a pointer past the last element of the array
    \param array The array to access
    \return A pointer past the last element, or NULL if empty
 */
#define tara_array_end(array) (array).data + (array).size

/*! \fn tara_array_index
    \brief Returns a pointer to the element at the index
    \param array The array to access
    \param idx The index to access
    \return A pointer to the element, or NULL if index is outside the bounds
 */
#define tara_array_index(array, idx) \
    (idx < 0 || idx >= (array).size ? NULL : (array).data + idx)

/*! \fn tara_array_append
    \brief Appends an element to the array
    \param array The array to append to
    \return A pointer to the new element, or NULL if the array is full
 */
#define tara_array_append(array) \
    (tara_array_max_size(array) <= (array).size ? NULL : &(array).data[(array).size++])

/*! \fn tara_array_find
    \brief Finds the first element that matches the value
    \param array The array to check
    \param element A pointer to the element to start the search at
    \param value The value to compare against
    On exit the element paramter point to the matching element, or is the end pointer
 */
#define tara_array_find(array, element, value) \
    for (; element != tara_array_end(array) && (value) != *element; ++element)

/*! \fn tara_array_find_member
    \brief Finds the first element's member that matches the value
    \param array The array to check
    \param element A pointer to the element to start the search at
    \param memebr The member of to check
    \param value The value to compare against
    On exit the element paramter point to the matching element, or is the end pointer
 */
#define tara_array_find_member(array, element, member, value) \
    for (; element != tara_array_end(array) && (value) != element->member; ++element)

/*! \fn tara_array_remove
    \brief Remove an element from the array
    \param array The array to remove the element from
    \param element A pointer to the element to remove

    Elements past the removed element are moved up to fill in the empty location, so any pointers
    to elements may be invalidated
 */
#define tara_array_remove(array, element) \
    (tara_array_begin(array) > (element) || tara_array_end(array) <= (element)   \
        ? NULL : memmove((element), (element) + 1, (char *)&(array).data[--(array).size] - (char *)(element)))

/*! \fn tara_array_remove_range
    \brief Remove a range of elements from the array
    \param array The array to remove the elements from
    \param begin A pointer to the first element to remove
    \param end A pointer past the last element to remove

    Elements past the removed elements are moved up to fill in the empty locations, so any pointers
    to elements may be invalidated
 */
#define tara_array_remove_range(array, begin, end) \
    memmove((begin), (end), (char *)((array).data + (array).size) - (char *)(end)); \
    (array).size -= ((end) - (begin));

/*! \def TARA_ARRAY_ITERATE
    \brief Defines a for-loop for iterating over an array
    \param type The underlying type of the array
    \param array The array to iterate over
    \param element A pointer to the element for that iteration

    The define is used as follows:

        TARA_ARRAY_ITERATE(int, my_array, e)
        {
            printf("%d\n", *e);
        }
 */
#define TARA_ARRAY_ITERATE(type, array, element) \
    for (type * element = tara_array_begin(array); element != tara_array_end(array); ++element)

#ifdef __cplusplus
}
#endif
