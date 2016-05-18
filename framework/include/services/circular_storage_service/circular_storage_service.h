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

#ifndef __CIRCULAR_STORAGE_SERVICE_H__
#define __CIRCULAR_STORAGE_SERVICE_H__

#include <stdint.h>

#include "storage.h"
#include "services/services_ids.h"
#include "cir_storage.h"

/**
 * @defgroup circular_storage_service Circular Storage Service
 * Manage circular storage on non-volatile memory.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/circular_storage_service/circular_storage_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/circular_storage_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_CIRCULAR_STORAGE</tt>
 * <tr><th><b>Service Id</b>  <td><tt>CIRCULAR_STORAGE_SERVICE_ID</tt>
 * </table>
 *
 * The circular Storage Service manages a circular list of fixed-size elements.\n
 * The location and size of elements are defined by static configuration and initialized
 * during initialization time (circular_storage_service_init). \n
 * The client can retreive a storage configuration using a key \ref circular_storage_service_get \n
 * and can:
 * - push a new element with \ref circular_storage_service_push
 * - pop the first element with \ref circular_storage_service_pop
 * - read the first element with \ref circular_storage_service_peek
 * - clear several or all the elements with \ref circular_storage_service_clear
 *
 * @ingroup services
 * @{
 */

#define MSG_ID_CIRCULAR_STORAGE_SERVICE_PUSH_RSP      (( \
							       MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE \
							       + 5) | 0x40)
#define MSG_ID_CIRCULAR_STORAGE_SERVICE_POP_RSP       (( \
							       MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE \
							       + 6) | 0x40)
#define MSG_ID_CIRCULAR_STORAGE_SERVICE_PEEK_RSP      (( \
							       MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE \
							       + 7) | 0x40)
#define MSG_ID_CIRCULAR_STORAGE_SERVICE_CLEAR_RSP     (( \
							       MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE \
							       + 8) | 0x40)
#define MSG_ID_CIRCULAR_STORAGE_SERVICE_GET_RSP      ((	\
							      MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE \
							      + 9) | 0x40)

/**
 * Circular storage structure
 */
struct cir_storage {
	uint32_t key;
	uint16_t partition_id;
	uint32_t first_block;
	uint32_t block_count;
	uint32_t element_size;
	cir_storage_t *storage;
};

/**
 * Circular storage configuration structure
 */
struct circular_storage_configuration {
	struct cir_storage *cir_storage_list;  /*!< List of configurations */
	uint8_t cir_storage_count;             /*!< Number of configurations available */
	flash_partition_t *partitions;         /*!< List of partitions */
	uint8_t no_part;                       /*!< Number of partitions available */
};

/* Project circular storage configurations */
extern struct circular_storage_configuration cir_storage_config;

/**
 * Structure containing the response to:
 *  - @ref circular_storage_service_get
 */
typedef struct circular_storage_service_get_rsp_msg {
	struct cfw_message header;      /*!< message header */
	void *storage;            /*!< storage pointer */
	int status;                     /*!< response status code.*/
} circular_storage_service_get_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref circular_storage_service_push
 */
typedef struct circular_storage_service_push_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Response status code.*/
} circular_storage_service_push_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref circular_storage_service_pop
 */
typedef struct circular_storage_service_pop_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	uint8_t *buffer;                /*!< Buffer containing data */
	int status;                     /*!< Response status code.*/
} circular_storage_service_pop_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref circular_storage_service_peek
 */
typedef struct circular_storage_service_peek_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	uint8_t *buffer;                /*!< Buffer containing data */
	int status;                     /*!< Response status code.*/
} circular_storage_service_peek_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref circular_storage_service_clear
 */
typedef struct circular_storage_service_clear_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Response status code.*/
} circular_storage_service_clear_rsp_msg_t;

/**
 * Flash storage get
 * Request to retreive the storage configuration by giving the configuration key.
 *
 * @param conn Service client connection pointer.
 * @param key Configuration key.
 * @param priv Private data pointer that will be passed sending answer.
 *
 * @b Response: _MSG_ID_CIRCULAR_STORAGE_SERVICE_GET_RSP_ with attached \ref circular_storage_service_get_rsp_msg_t
 */
void circular_storage_service_get(cfw_service_conn_t *conn, uint32_t key,
				  void *priv);

/**
 * Flash storage push.
 *
 * @param conn Service client connection pointer.
 * @param buffer Buffer containing data to be written
 * @param storage  Pointer on the storage struct as returned by get
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_CIRCULAR_STORAGE_SERVICE_PUSH_RSP_ with attached \ref circular_storage_service_push_rsp_msg_t
 */
void circular_storage_service_push(cfw_service_conn_t *conn, uint8_t *buffer,
				   void *storage,
				   void *priv);

/**
 * Flash storage pop.
 *
 * @param conn Service client connection pointer.
 * @param storage  Pointer on the storage struct as returned by get
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_CIRCULAR_STORAGE_SERVICE_POP_RSP_ with attached \ref circular_storage_service_pop_rsp_msg_t
 */
void circular_storage_service_pop(cfw_service_conn_t *conn, void *storage,
				  void *priv);

/**
 * Flash storage peek.
 *
 * @param conn Service client connection pointer.
 * @param storage  Pointer on the storage struct as returned by get
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_CIRCULAR_STORAGE_SERVICE_PEEK_RSP_ with attached \ref circular_storage_service_peek_rsp_msg_t
 */
void circular_storage_service_peek(cfw_service_conn_t *conn, void *storage,
				   void *priv);

/**
 * Flash storage clear data.
 *
 * @param conn Service client connection pointer.
 * @param storage  Pointer on the storage struct as returned by get
 * @param elt_count Number of elements to clear, 0 to clear all
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_CIRCULAR_STORAGE_SERVICE_CLEAR_RSP_ with attached \ref circular_storage_service_clear_rsp_msg_t
 */
void circular_storage_service_clear(cfw_service_conn_t *conn, void *storage,
				    uint32_t elt_count,
				    void *priv);

/** @} */

#endif /* __CIRCULAR_STORAGE_SERVICE_H__ */
