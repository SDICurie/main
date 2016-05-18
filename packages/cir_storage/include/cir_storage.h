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

#ifndef __CIR_STORAGE_H
#define __CIR_STORAGE_H

#include <stdint.h>

/* This header file contains the Circular storage client API. */

/**
 * @defgroup cbuffer Circular buffer storage
 * The Circular storage library allows client to store and retrieve data from a circular buffer on a flash medium.
 *
 * The library relies on specific backends to address the different flash types.
 *
 * It exposes two APIs:
 * * a client API to push, pop, peek and clear elements from an existing storage, identified by its storage handle,
 * * a backend API used to initialize a storage.
 *   in order to initialize a circular buffer, cir_storage_flash_init() function
 *   should be called with a proper implementation of cir_storage_flash_t
 * @ingroup packages
 * @{
 */

/** Circular buffer error code. */
typedef enum {
	CBUFFER_STORAGE_SUCCESS = 0, /*!< Operation succeed */
	CBUFFER_STORAGE_BOUNDS_ERROR, /*!< Bounds error */
	CBUFFER_STORAGE_EMPTY_ERROR, /*!< Empty circular buffer error */
	CBUFFER_STORAGE_WRITE_ERROR, /*!< Writing error from backend function*/
	CBUFFER_STORAGE_READ_ERROR, /*!< Reading error from backend function*/
	CBUFFER_STORAGE_ERASE_ERROR, /*!< Erase error from backend function*/
	CBUFFER_STORAGE_ERROR       /*!< Default circular buffer error */
} cir_storage_err_t;

/**
 * Handle of the circular buffer
 */
typedef struct _cir_storage_t {
	uint32_t buffer_size;   /*!< Total size in bytes */
	uint32_t elt_size;      /*!< Element size in bytes */
} cir_storage_t;

/**
 * Push an element in the circular buffer.
 * @param self the pointer on the circular buffer.
 * @param buf pointer to the data to push.
 * @return cbuffer_storage_err_t error code.
 *  CBUFFER_STORAGE_BOUNDS_ERROR: buffer length to push is larger
 *                                  than buffer size.
 *  CBUFFER_STORAGE_WRITE_ERROR: Writing step failed, element is not pushed.
 *  CBUFFER_STORAGE_SUCCESS: circular buffer push succeed.
 */
cir_storage_err_t cir_storage_push(cir_storage_t *self, uint8_t *buf);

/**
 * Pop the oldest element from the circular buffer
 * Popped element is removed from the circular buffer.
 * @param self the pointer on the circular buffer.
 * @param buf pointer to the buffer to fill.
 * @return cbuffer_storage_err_t error code.
 *  CBUFFER_STORAGE_BOUNDS_ERROR: element to pop is larger than buffer size.
 *  CBUFFER_STORAGE_EMPTY_ERROR: circular buffer is empty. Pop is not possible.
 *  CBUFFER_STORAGE_READ_ERROR: Reading step failed, element is not popped.
 *  CBUFFER_STORAGE_SUCCESS: circular buffer pop succeed.
 */
cir_storage_err_t cir_storage_pop(cir_storage_t *self, uint8_t *buf);

/**
 * Read bytes from the circular buffer.
 * @param self the pointer on the circular buffer.
 * @param buf pointer to the buffer to fill.
 * @return cbuffer_storage_err_t error code.
 *  CBUFFER_STORAGE_READ_ERROR: Reading step failed, element is not peeked.
 *  CBUFFER_STORAGE_SUCCESS: circular buffer peek succeed.
 */
cir_storage_err_t cir_storage_peek(cir_storage_t *self, uint8_t *buf);

/**
 * Clear data stored in the circular buffer.
 * @param self the pointer on the circular buffer.
 * @param elt_count number of elements to clear, 0 to clear all
 * @return cbuffer_storage_err_t error code.
 *  CBUFFER_STORAGE_ERASE_ERROR: clear step failed.
 *  CBUFFER_STORAGE_SUCCESS: circular buffer clear succeed.
 */
cir_storage_err_t cir_storage_clear(cir_storage_t *self, uint32_t elt_count);

/** @} */

#endif /* __CIR_STORAGE_H */
