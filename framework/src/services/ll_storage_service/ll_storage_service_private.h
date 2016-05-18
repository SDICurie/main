/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
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

#ifndef __LL_STORAGE_SERVICE_PRIVATE_H__
#define __LL_STORAGE_SERVICE_PRIVATE_H__

#include <stdint.h>

#include "cfw/cfw.h"
#include "services/services_ids.h"
#include "services/ll_storage_service/ll_storage_service.h"

/*  */
#define MSG_ID_LL_ERASE_BLOCK_REQ      (~0x40 &	\
					MSG_ID_LL_STORAGE_SERVICE_ERASE_BLOCK_RSP)
#define MSG_ID_LL_READ_PARTITION_REQ   (~0x40 &	\
					MSG_ID_LL_STORAGE_SERVICE_READ_RSP)
#define MSG_ID_LL_WRITE_PARTITION_REQ  (~0x40 &	\
					MSG_ID_LL_STORAGE_SERVICE_WRITE_RSP)

/*MX25U12835F is 128Mb bits serial Flash memory,
 *
 * Program command is executed on byte basis, or page (256 bytes) basis, or word basis
 * Erase command is executed on sector (4K-byte), block (32K-byte), or large block (64K-byte), or whole chip basis.
 * The erase block size that will be considered in the sector size */

/* Quark SE embedded 384KB Flash memory,
 *
 * Program command is executed on 32-bits double word basis
 * Erase command is executed on pages (2K-byte), whole flash basis.
 * The erase block size that will be considered is the page size */

#define ERASE_REQ                                       0
#define WRITE_REQ                                       1


/**
 * Structure containing the request to erase a block.
 */
typedef struct ll_storage_erase_block_req_msg {
	struct cfw_message header;
	uint16_t partition_id;
	uint32_t st_blk;
	uint32_t no_blks;
} ll_storage_erase_block_req_msg_t;

/**
 * Structure containing the request to write a partition.
 */
typedef struct ll_storage_write_partition_req_msg {
	struct cfw_message header;
	uint16_t partition_id;
	uint32_t st_offset;
	uint32_t size;
	uint8_t write_type;
	void *buffer;
} ll_storage_write_partition_req_msg_t;

/**
 * Structure containing the request to read a partition.
 */
typedef struct ll_storage_read_partition_req_msg {
	struct cfw_message header;
	uint16_t partition_id;
	uint32_t st_offset;
	uint32_t size;
} ll_storage_read_partition_req_msg_t;


#endif /* __LL_STORAGE_SERVICE_PRIVATE_H__ */
