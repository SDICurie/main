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

#ifndef __LL_STORAGE_SERVICE_H__
#define __LL_STORAGE_SERVICE_H__

#include <stdint.h>

#include "cfw/cfw.h"
#include "services/services_ids.h"

/**
 * @defgroup ll_storage_service Low Level Storage Service
 * Perform operations on non-volatile memory.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/ll_storage_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/ll_storage_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_LL_STORAGE, SERVICES_QUARK_SE_LL_STORAGE_IMPL</tt>
 * <tr><th><b>Service Id</b>  <td><tt>LL_STOR_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

/* Message ID for service response */
#define MSG_ID_LL_STORAGE_SERVICE_ERASE_BLOCK_RSP  (0x40 | \
						    ( \
							    MSG_ID_LL_STORAGE_SERVICE_BASE \
							    + 2))
#define MSG_ID_LL_STORAGE_SERVICE_READ_RSP         (0x40 | \
						    ( \
							    MSG_ID_LL_STORAGE_SERVICE_BASE \
							    + 3))
#define MSG_ID_LL_STORAGE_SERVICE_WRITE_RSP        (0x40 | \
						    ( \
							    MSG_ID_LL_STORAGE_SERVICE_BASE \
							    + 4))

/**
 * Structure containing the response to:
 *  - \ref ll_storage_service_erase_block
 */
typedef struct ll_storage_service_erase_block_rsp_msg {
	struct cfw_message header; /*!< Message header */
	int status;             /*!< Response status code.*/
} ll_storage_service_erase_block_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - \ref ll_storage_service_erase_partition
 *  - \ref ll_storage_service_write
 */
typedef struct ll_storage_service_write_rsp_msg {
	struct cfw_message header; /*!< Message header */
	uint8_t write_type;     /*!< Type of action: ERASE/WRITE */
	uint32_t actual_size;   /*!< Actual size of written data */
	int status;             /*!< Response status code.*/
} ll_storage_service_write_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - \ref ll_storage_service_read
 */
typedef struct ll_storage_service_read_rsp_msg {
	struct cfw_message header; /*!< Message header */
	uint32_t actual_read_size; /*!< Number of bytes read */
	void *buffer;           /*!< Buffer containing data; should be freed when no more used */
	int status;             /*!< Response status code.*/
} ll_storage_service_read_rsp_msg_t;


/**
 * Low level partition erase.
 *
 * @msc
 *  Client,"Low Level Storage Service","Flash memory driver";
 *
 *  Client->"Low Level Storage Service" [label="erase request"];
 *  "Low Level Storage Service"=>"Flash memory driver" [label="erase \n function call"];
 *  "Low Level Storage Service"<<"Flash memory driver" [label="erase \n return status"];
 *  Client<-"Low Level Storage Service" [label="erase response \n message w/ status", URL="\ref ll_storage_service_write_rsp_msg_t"];
 * @endmsc
 *
 * @param conn Service client connection pointer.
 * @param partition_id ID of the partition
 * @param priv Private data pointer that will be passed in the response message
 *
 * @b Response: _MSG_ID_LL_STORAGE_SERVICE_WRITE_RSP_ message with write_type = ERASE_REQ
 */
void ll_storage_service_erase_partition(cfw_service_conn_t *	conn,
					uint16_t		partition_id,
					void *			priv);

/**
 * Low level erase block(s).
 *
 * @msc
 *  Client,"Low Level Storage Service","Flash memory driver";
 *
 *  Client->"Low Level Storage Service" [label="erase request"];
 *  "Low Level Storage Service"=>"Flash memory driver" [label="erase block\n function call"];
 *  "Low Level Storage Service"<<"Flash memory driver" [label="erase block \n return status"];
 *  Client<-"Low Level Storage Service" [label="erase response \n message w/ status", URL="\ref ll_storage_service_erase_block_rsp_msg_t"];
 * @endmsc
 *
 * @param conn Service client connection pointer.
 * @param partition_id ID of the partition
 * @param start_block First block to be erased (offset from the beginning of the partition)
 * @param number_of_blocks  Number of blocks to be erased
 * @param priv Private data pointer that will be passed in the response message
 *
 * @b Response: _MSG_ID_LL_STORAGE_SERVICE_ERASE_BLOCK_RSP_ message
 */
void ll_storage_service_erase_block(cfw_service_conn_t *conn,
				    uint16_t partition_id, uint16_t start_block,
				    uint16_t number_of_blocks,
				    void *priv);

/**
 * Low level data read.
 *
 * @msc
 *  Client,"Low Level Storage Service","Flash memory driver";
 *
 *  Client->"Low Level Storage Service" [label="read request"];
 *  "Low Level Storage Service"=>"Flash memory driver" [label="read \n function call"];
 *  "Low Level Storage Service"<<"Flash memory driver" [label="read return status \n and data"];
 *  Client<-"Low Level Storage Service" [label="read response \n message w/ status and data", URL="\ref ll_storage_service_read_rsp_msg_t"];
 * @endmsc
 *
 * @param conn Service client connection pointer.
 * @param partition_id ID of the partition
 * @param start_offset First data address to be read (offset from the beginning of the partition); \n must be 4bytes aligned
 * @param size  Number of bytes to be read
 * @param priv Private data pointer that will be passed in the response message
 *
 * @b Response: _MSG_ID_LL_STORAGE_SERVICE_READ_RSP_ message
 */
void ll_storage_service_read(cfw_service_conn_t *conn, uint16_t partition_id,
			     uint32_t start_offset, uint32_t size,
			     void *priv);

/**
 * Low level data write.
 *
 * @msc
 *  Client,"Low Level Storage Service","Flash memory driver";
 *
 *  Client->"Low Level Storage Service" [label="write request"];
 *  "Low Level Storage Service"=>"Flash memory driver" [label="write \n function call"];
 *  "Low Level Storage Service"<<"Flash memory driver" [label="write \n return status"];
 *  Client<-"Low Level Storage Service" [label="write response \n message w/ status", URL="\ref ll_storage_service_write_rsp_msg_t"];
 * @endmsc
 *
 * @param conn Cervice client connection pointer.
 * @param partition_id ID of the partition
 * @param start_offset First address to be written (offset from the beginning of the partition); \n must be 4bytes aligned
 * @param buffer Buffer containing data to be written
 * @param size  Number of bytes to be written
 * @param priv Private data pointer that will be passed in the response message
 *
 * @b Response: _MSG_ID_LL_STORAGE_SERVICE_WRITE_RSP_ with write_type = WRITE_REQ
 */
void ll_storage_service_write(cfw_service_conn_t *conn, uint16_t partition_id,
			      uint32_t start_offset, void *buffer,
			      uint32_t size,
			      void *priv);

/** @} */

#endif /* __LL_STORAGE_SERVICE_H__ */
