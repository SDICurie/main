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

#include <stddef.h>
#include <stdbool.h>

#include "cir_storage.h"
#include "cir_storage_backend.h"

#define BASE_PTR(storage,index) index*storage->block_size + sizeof(block_info_t)
#define READ_PTR(storage)       storage->rp.offset
#define WRITE_PTR(storage)      storage->wp.offset
#define READ_BLOCK(storage)     storage->rp.index
#define WRITE_BLOCK(storage)    storage->wp.index

/**
 * Each circular storage spans accross several blocks of FLASH, each starting
 * with a header allowing to identify incompatible legacy storage blocks.
 */
typedef struct _block_header {
	uint32_t magic:16; /** A magic word */
	uint32_t size:16;  /** The size of each stored element */
} block_header_t;

/**
 * Right after each block header, a status struct allows to know if the block
 * contains the write and read pointers.
 */
typedef struct _block_status {
	uint32_t write;
	uint32_t read;
} block_status_t;

/** The block info is the concatenation of the block header and status */
typedef struct _block_info {
	block_header_t header;
	block_status_t status;
} block_info_t;

#define CIR_STORAGE_FLASH_MAGIC 0xABCD

#define BLOCK_UNUSED  0xFFFFFFFF
#define BLOCK_CURRENT 0xAAAAAAAA
#define BLOCK_USED    0x00000000

#define WRITE_STATUS_OFFSET 0
#define READ_STATUS_OFFSET  sizeof(uint32_t)

typedef struct _elt_status {
	uint32_t status;
} elt_status_t;

#define ELT_EMPTY     0xFFFFFFFF
#define ELT_WRITTEN   0xBBBBBBBB
#define ELT_READ      0x00000000

static int32_t write_header(cir_storage_flash_t *storage, uint32_t index) {

	block_header_t header = {
		.magic = CIR_STORAGE_FLASH_MAGIC
	};
	header.size = storage->parent.elt_size;
	return storage->write(storage,
			      index*storage->block_size,
	                      sizeof(header),
	                      (uint8_t *)&header);
}

static int32_t write_status(cir_storage_flash_t *storage,
                            uint32_t index,
                            uint32_t status,
                            uint32_t offset) {

	return storage->write(storage,
			      index*storage->block_size + sizeof(block_header_t) + offset,
	                      sizeof(status),
	                      (uint8_t *)&status);
};

int32_t cir_storage_flash_init(cir_storage_flash_t *storage)
{
	block_info_t info;
	if (storage->parent.buffer_size%storage->block_size ||
		(storage->parent.elt_size + sizeof(uint32_t)
			> storage->block_size - sizeof(block_info_t))) {
		return -1;
	}

	/* Doing this calculation only once will save some flash space */
	uint32_t elt_space = storage->parent.elt_size + sizeof(elt_status_t);
	uint32_t block_space = storage->block_size - sizeof(block_info_t);
	storage->last_offset =
		sizeof(block_info_t) + elt_space*(block_space/elt_space-1);

	WRITE_PTR(storage) = 0;
	READ_PTR(storage) = 0;

	/* Retrieve write and read pointers */
	uint32_t block_index = storage->block_first;
	while ((block_index <=  storage->block_last)
		&& ((READ_PTR(storage) == 0) || (WRITE_PTR(storage) == 0))) {

		uint32_t block_offset = block_index*storage->block_size;
		if (storage->read(storage,
				  block_offset,
				  sizeof(info),
				  (uint8_t *)&info) != 0) {
			return -1;
		}
		/* Check header first */
		if (((uint32_t) info.header.magic == CIR_STORAGE_FLASH_MAGIC)
			&& (info.header.size == storage->parent.elt_size)) {
			bool has_wp = (info.status.write == BLOCK_CURRENT);
			bool has_rp = (info.status.read == BLOCK_CURRENT);
			if (has_wp || has_rp) {
				/* Look for pointers in this block */
				uint32_t elt_offset = block_offset + sizeof(block_info_t);
				while ((elt_offset < (block_offset + storage->block_size))
					&& ((has_wp && (WRITE_PTR(storage) == 0))
				     || (has_rp && (READ_PTR(storage) == 0)))) {
				    uint32_t elt_status;
				    if (storage->read(storage,
						      elt_offset,
				                      sizeof(elt_status),
				                      (uint8_t *)&elt_status) != 0) {
				        return -1;
				    }
					if ((has_wp && (WRITE_PTR(storage) == 0))
					 && (elt_status == ELT_EMPTY)) {
						/* The first empty element is the write pointer */
						WRITE_PTR(storage) = elt_offset;
						WRITE_BLOCK(storage) = block_index;
					}
					if ((has_rp && (READ_PTR(storage) == 0))
					 && (elt_status != ELT_READ)) {
						/* The first non-read element is the read pointer */
						READ_PTR(storage) = elt_offset;
						READ_BLOCK(storage) = block_index;
					}
					elt_offset += sizeof(elt_status) + storage->parent.elt_size;
				}
			}
		} else {
			/* Not an existing circular buffer */
			break;
		}
		block_index++;
	}
	if ((READ_PTR(storage) != 0) && (WRITE_PTR(storage) != 0)) {
		/* Existing storage detected */
		return 0;
	}
	/* Storage first init */

	/* Erase the first block */
	if (storage->erase(storage, storage->block_first, 1) != 0) {
		return -1;
	}

	/* Store our header first */
	if (write_header(storage, storage->block_first) != 0) {
		return -1;
	}

	/* Write the block status */
	block_status_t status = {
		.write = BLOCK_CURRENT,
		.read = BLOCK_CURRENT
	};
	if (storage->write(
			storage,
			storage->block_first*storage->block_size + sizeof(block_header_t),
			sizeof(status),
			(uint8_t *)&status) != 0) {
		return -1;
	}

	/* Set our pointers in the first block, after the info block */
	READ_PTR(storage) = BASE_PTR(storage,storage->block_first);
	WRITE_PTR(storage) = READ_PTR(storage);
	WRITE_BLOCK(storage) = storage->block_first;
	READ_BLOCK(storage) = storage->block_first;
	return 0;
}

cir_storage_err_t cir_storage_push(cir_storage_t *self, uint8_t *buf)
{
	cir_storage_flash_t *storage = (cir_storage_flash_t *)self;
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;

	storage->lock(storage);
	elt_status_t elt_status = { ELT_WRITTEN };

	/* Update the status of the next element */
	if (storage->write(storage, WRITE_PTR(storage), sizeof(elt_status), (uint8_t *)&elt_status) != 0) {
		ret = CBUFFER_STORAGE_ERROR;
		goto exit;
	}

	/* Write the element */
	if (storage->write(storage, WRITE_PTR(storage) + sizeof(elt_status), self->elt_size, buf) != 0) {
		ret = CBUFFER_STORAGE_ERROR;
		goto exit;
	}

	/* Increase and adjust write pointer */
	if (WRITE_PTR(storage)%storage->block_size == storage->last_offset) {
		/* Mark current block as non-current for write pointer */
		if (write_status(storage, WRITE_BLOCK(storage), BLOCK_USED, WRITE_STATUS_OFFSET) != 0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}

		/* Switch to the next block */
		WRITE_BLOCK(storage) = (WRITE_BLOCK(storage) == storage->block_last)
			? storage->block_first : WRITE_BLOCK(storage) + 1;

		/* Erase the new block */
		if (storage->erase(storage, WRITE_BLOCK(storage), 1) !=0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}
		/* Write new header to block */
		if (write_header(storage, WRITE_BLOCK(storage)) !=0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}

		/* Mark New block as current for write pointer */
		if (write_status(storage, WRITE_BLOCK(storage), BLOCK_CURRENT, WRITE_STATUS_OFFSET) != 0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}

		/* Set write pointer */
		WRITE_PTR(storage) = BASE_PTR(storage,WRITE_BLOCK(storage));

		/* If read pointer is in the erased block, we move it on the next one */
		if (WRITE_BLOCK(storage) == READ_BLOCK(storage)) {
			READ_BLOCK(storage) = (READ_BLOCK(storage) == storage->block_last)
				? storage->block_first : READ_BLOCK(storage) + 1;
			/* Skip block info */
			READ_PTR(storage) = BASE_PTR(storage,READ_BLOCK(storage));
			/* Update block header */
			if (write_status(storage, READ_BLOCK(storage), BLOCK_CURRENT, READ_STATUS_OFFSET) != 0) {
				ret = CBUFFER_STORAGE_ERROR;
				goto exit;
			}
		}
	} else {
		WRITE_PTR(storage) += sizeof(elt_status) + storage->parent.elt_size;
	}

exit:
	storage->unlock(storage);
	return ret;
}

/* Must be called with the storage mutex locked */
static cir_storage_err_t clear_one_element(cir_storage_flash_t * storage)
{
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;

	elt_status_t elt_status = { ELT_READ };

	/* Mark the element as read */
	if (storage->write(storage, READ_PTR(storage),sizeof(elt_status), (uint8_t *)&elt_status) != 0) {
		ret = CBUFFER_STORAGE_ERROR;
		goto exit;
	}
	if (READ_PTR(storage)%storage->block_size == storage->last_offset) {
		/* Mark current block as not current for read pointer */
		if (write_status(storage, READ_BLOCK(storage), BLOCK_USED, READ_STATUS_OFFSET) != 0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}
		/* Switch to next block */
		READ_BLOCK(storage) = (READ_BLOCK(storage) == storage->block_last) ?
			storage->block_first : READ_BLOCK(storage) + 1;
		/* Mark new block as current for read pointer */
		if (write_status(storage, READ_BLOCK(storage), BLOCK_CURRENT, READ_STATUS_OFFSET) != 0) {
			ret = CBUFFER_STORAGE_ERROR;
			goto exit;
		}
		READ_PTR(storage) = BASE_PTR(storage,READ_BLOCK(storage));
	} else {
		/* Advance the read pointer of one element */
		READ_PTR(storage) += sizeof(elt_status) + storage->parent.elt_size;
	}

exit:
	return ret;
}

/* Must be called with the storage mutex locked */
static cir_storage_err_t read_one_element(cir_storage_flash_t *storage, uint8_t *buf)
{
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;

	/* nothing to read */
	if (READ_PTR(storage) == WRITE_PTR(storage)) {
		ret = CBUFFER_STORAGE_EMPTY_ERROR;
		goto exit;
	}

	if (storage->read(storage, READ_PTR(storage) + sizeof(elt_status_t),
		              storage->parent.elt_size, buf) != 0) {
		ret = CBUFFER_STORAGE_ERROR;
		goto exit;
	}

exit:
	return ret;
}

cir_storage_err_t cir_storage_pop(cir_storage_t *self, uint8_t *buf)
{
	cir_storage_flash_t *storage = (cir_storage_flash_t *)self;
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;

	storage->lock(storage);

	ret = read_one_element(storage, buf);
	if (ret != CBUFFER_STORAGE_SUCCESS) {
		goto exit;
	}

	/* Clear data */
	ret = clear_one_element(storage);

exit:
	storage->unlock(storage);
	return ret;
}


cir_storage_err_t cir_storage_peek(cir_storage_t * self, uint8_t *buf)
{
	cir_storage_flash_t *storage = (cir_storage_flash_t *)self;
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;

	storage->lock(storage);

	ret = read_one_element(storage, buf);

	storage->unlock(storage);
	return ret;
}

cir_storage_err_t cir_storage_clear(cir_storage_t * self, uint32_t elt_count)
{
	cir_storage_flash_t *storage = (cir_storage_flash_t *)self;
	cir_storage_err_t ret = CBUFFER_STORAGE_SUCCESS;
	int n = elt_count;

	storage->lock(storage);

	while ((ret == CBUFFER_STORAGE_SUCCESS)
		&& (READ_PTR(storage) != WRITE_PTR(storage))
		&& ((elt_count == 0) || (n > 0))) {
		ret = clear_one_element(storage);
		n--;
	}
	storage->unlock(storage);
	return ret;
}
