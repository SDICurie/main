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

#ifndef __CIR_STORAGE_BACKEND_H
#define __CIR_STORAGE_BACKEND_H

#include <stdint.h>
#include "cir_storage.h"

/* This header file contains the Circular storage backend API. */

/**
 * @addtogroup cbuffer
 * @{
 */

/**
 * Block Pointer structure
 */
typedef struct {
	uint32_t index;         /*!< Index of block where pointer is located */
	uint32_t offset;        /*!< Offset of pointer in the block */
} block_pointer_t;

typedef struct _cir_storage_flash_t cir_storage_flash_t;

/**
 * Circular storage information.
 * This structure shall be filled by the backend (except wp and rp)
 * and passed to the @ref cir_storage_flash_init "generic init function".
 */
typedef struct _cir_storage_flash_t {
	cir_storage_t parent; /*!< Circular buffer handle */
	uint32_t block_first; /*!< Index of circular buffer first block */
	uint32_t block_last;  /*!< Index of circular buffer last block */
	uint32_t block_size;  /*!< Block size (in bytes) */
	uint32_t last_offset; /*!< Last element offset in a block */
	block_pointer_t wp;   /*!< Write Pointer */
	block_pointer_t rp;   /*!< Read Pointer */
	int32_t (*read)(cir_storage_flash_t *, uint32_t, uint32_t, uint8_t *);  /*!< Read function */
	int32_t (*write)(cir_storage_flash_t *, uint32_t, uint32_t, uint8_t *); /*!< Write function */
	int32_t (*erase)(cir_storage_flash_t *, uint32_t, uint32_t);          /*!< Erase function */
	void (*lock)(cir_storage_flash_t *);   /*!< Lock function */
	void (*unlock)(cir_storage_flash_t *); /*!< Unlock function */
} cir_storage_flash_t;

/**
 * Generic setup of the flash circular storage.
 * Must be called by the backends init functions.
 *
 * @param storage  The storage to initialize
 *
 * @return -1  if an error occurs,
 *          0  if no error
 */
int32_t cir_storage_flash_init(cir_storage_flash_t *storage);

/** @} */

#endif /* __CIR_STORAGE_BACKEND_H */
