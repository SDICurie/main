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

#ifndef __ZEPHYR_COMMON_
#define __ZEPHYR_COMMON_

#include "os/os.h"    /* framework-specific types */
#include <board.h>

#define INT_SIZE (sizeof(unsigned int) * 8)

#define DECLARE_BLK_ALLOC(var, type, count) \
	type var ## _elements[count]; \
	unsigned int var ## _track_alloc[count / INT_SIZE + 1] = { 0 };	\
	unsigned int var ## _alloc_max = 0; \
	unsigned int var ## _alloc_cur = 0; \
	type *var ## _alloc(void) { \
		int i; \
		uint32_t flags = irq_lock(); \
		for (i = 0; i < count; i++) { \
			int wi = i / INT_SIZE; \
			int bi = INT_SIZE - 1 - (i % INT_SIZE);	\
			if ((var ## _track_alloc[wi] & 1 << bi) == 0) {	\
				var ## _track_alloc[wi] |= 1 << (bi); \
				var ## _alloc_cur++; \
				if (var ## _alloc_cur >	\
				    var ## _alloc_max) var ## _alloc_max = \
						var ## _alloc_cur; \
				irq_unlock(flags); \
				return &var ## _elements[i]; \
			} \
		} \
		irq_unlock(flags); \
		return NULL; \
	} \
	void var ## _free(type * ptr) {	\
		int index = ptr - var ## _elements; \
		uint32_t flags;	\
		if (index < count) { \
			flags = irq_lock(); \
			if ((var ## _track_alloc[index / \
						 INT_SIZE] & 1 << \
			     (INT_SIZE - 1 - (index % INT_SIZE))) == \
			    0) { \
				return;	\
			} \
			var ## _track_alloc[index / \
					    INT_SIZE] &= \
				~(1 << (INT_SIZE - 1 - (index % INT_SIZE))); \
			irq_unlock(flags); \
			var ## _alloc_cur--; \
		} \
	} \
	bool var ## _used(type * ptr) {	\
		int index = ptr - var ## _elements; \
		if ((index >= 0) && (index < count)) { \
			if ((var ## _track_alloc[index / \
						 INT_SIZE] & \
			     (1 << (INT_SIZE - 1 - (index % INT_SIZE)))) != \
			    0) { \
				return true; \
			} \
		} \
		return false; \
	}

/* Internal init function, use os_init() instead */
void os_init_sync(void);
void os_init_timer(void);

/**
 * \brief Copy error code to caller's variable, or panic if caller did not specify
 * an error variable
 *
 * \param [out] err : pointer to caller's error variable - may be _NULL
 * \param [in] localErr : error detected by the function
 *
 */
void error_management(OS_ERR_TYPE *err, OS_ERR_TYPE localErr);


/**
 * \enum T_EXEC_LEVEL
 *  \brief Identifier for the level of execution
 */
typedef enum {
	E_EXEC_LVL_ISR = NANO_CTX_ISR,
	E_EXEC_LVL_FIBER = NANO_CTX_FIBER,
	E_EXEC_LVL_TASK = NANO_CTX_TASK,
	E_EXEC_LVL_UNKNOWN
} T_EXEC_LEVEL;


/**
 * \brief Returns the current execution level
 *   _getExecLevel returns the current execution level: task, fiber or isr
 */
#define _getExecLevel() ((T_EXEC_LEVEL)sys_execution_context_type_get())

#endif /* __ZEPHYR_COMMON_ */
