/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup list Lists
 * List management utility functions
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "util/list.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/util</tt>
 * </table>
 *
 * @ingroup infra
 * @{
 */

typedef struct list {
	struct list *next;
} list_t;

typedef struct list_head_ {
	list_t *head;
	list_t *tail;
} list_head_t;

/**
 * Initialize a list structure.
 *
 * @param list List to initialize
 */
void list_init(list_head_t *list);

/**
 * Append an element to the end of a list, protected from interrupt context concurency.
 *
 * @param list    List to which element has to be added
 * @param element Element to add.
 */
void list_add(list_head_t *list, list_t *element);

/**
 * Insert an element at the begining of a list, protected from interrupt context concurency.
 *
 * @param list    List to which element has to be added
 * @param element Element to add.
 */
void list_add_head(list_head_t *list, list_t *element);

/**
 * Remove an element from the list, protected from interrupt context concurency.
 *
 * @param list    List to remove the element from
 * @param element Element to remove.
 */
void list_remove(list_head_t *list, list_t *element);

/**
 * Iterate through elements of a list.
 *
 * @param lh    List to iterate through
 * @param cb    Callback function to call for each element
 * @param param Parameter to pass to the callback in addition to the element.
 */
void list_foreach(list_head_t *lh, void (*cb)(void *, void *), void *param);

/**
 * Iterate through elements of a list, with the option
 * to remove element from the list in the callback.
 *
 * @param lh    List to iterate through
 * @param cb    Callback function to call for each element.
 *              If the callback returns non-zero value, the element is
 *              removed from the list.
 * @param param Parameter to pass to the callback in addition to the element.
 */
void list_foreach_del(list_head_t *lh, int (*cb)(void *, void *), void *param);

/**
 * Get the first element from the list, protected from interrupt context concurrency.
 *
 * @param lh List from which the element has to be retrieved.
 * @return Extracted element.
 */
list_t *list_get(list_head_t *lh);

/**
 * Check if the list is empty.
 *
 * @return 0 If not empty
 */
static inline int list_empty(list_head_t *lh)
{
	return lh->head == NULL;
}

/**
 * Find the first element in a list matching a criteria.
 *
 * @param lh   List to parse.
 * @param cb   Test callback that will be applied to each item
 * @param data Opaque data to pass to the test callback
 *
 * @return Found element or NULL
 */
list_t *list_find_first(list_head_t *lh, bool (*cb)(list_t *,
						    void *), void *data);


/** @} */
#endif /* __LIST_H__ */
