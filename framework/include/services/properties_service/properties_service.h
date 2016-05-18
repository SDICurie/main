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

#ifndef __PROPERTIES_SERVICE_H__
#define __PROPERTIES_SERVICE_H__

#include "cfw/cfw.h"
#include "drivers/data_type.h"
#include "infra/properties_storage.h"
#include "services/services_ids.h"

/**
 * @defgroup properties_service Properties Service
 * Properties Service (storage of named variables)
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/properties_service/properties_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/properties_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_PROPERTIES, SERVICES_QUARK_SE_PROPERTIES_IMPL, and more in Kconfig</tt>
 * <tr><th><b>Service Id</b>  <td><tt>PROPERTIES_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

#define MSG_ID_PROP_SERVICE_ADD_RSP     ((MSG_ID_PROP_SERVICE_BASE + 1) | 0x40)
#define MSG_ID_PROP_SERVICE_READ_RSP    ((MSG_ID_PROP_SERVICE_BASE + 3) | 0x40)
#define MSG_ID_PROP_SERVICE_WRITE_RSP   ((MSG_ID_PROP_SERVICE_BASE + 4) | 0x40)
#define MSG_ID_PROP_SERVICE_REMOVE_RSP  ((MSG_ID_PROP_SERVICE_BASE + 2) | 0x40)

/**
 * Structure containing the response to:
 *  - @ref properties_service_read
 */
typedef struct properties_service_read_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Status of the read operation, use it to check for errors */
	uint16_t property_size;         /*!< Size of property value */
	uint8_t start_of_values[];      /*!< Property value */
} properties_service_read_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref properties_service_add
 */
typedef struct properties_service_add_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Response status code.*/
} properties_service_add_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref properties_service_remove
 */
typedef struct properties_service_remove_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Response status code.*/
} properties_service_remove_rsp_msg_t;

/**
 * Structure containing the response to:
 *  - @ref properties_service_write
 */
typedef struct properties_service_write_rsp_msg {
	struct cfw_message header;      /*!< Message header */
	int status;                     /*!< Response status code.*/
} properties_service_write_rsp_msg_t;

/**
 * Read a property.
 *
 * This function returns immediately and the response will be sent through an
 * incoming cfw message with ID MSG_ID_PROP_SERVICE_READ_RSP of type
 * properties_service_read_rsp_msg.
 *
 * @param conn service Client connection pointer.
 * @param service_id Service_id client is interested in
 * @param property_id Property_id the client is interested in
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_PROP_SERVICE_READ_PROP_RSP_ with attached \ref properties_service_read_rsp_msg_t
 */
void properties_service_read(cfw_service_conn_t *conn, uint16_t service_id,
			     uint16_t property_id,
			     void *priv);

/**
 * Add a property.
 *
 * @param conn service Client connection pointer.
 * @param service_id Service_id client is interested in
 * @param property_id Property_id the client is interested in
 * @param factory_rest_persistent Set to true if the property needs to persist upon a factory reset
 * @param buffer Buffer containing the value of the new property
 * @param size Size of the new property's value
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_PROP_SERVICE_ADD_PROP_RSP_ with attached \ref properties_service_add_rsp_msg_t
 */
void properties_service_add(cfw_service_conn_t *conn, uint16_t service_id,
			    uint16_t property_id, bool factory_rest_persistent,
			    void *buffer, uint16_t size,
			    void *priv);

/**
 * Write or add a property.
 *
 * @param conn Service client connection pointer.
 * @param service_id Service_id client is interested in
 * @param property_id Property_id the client is interested in
 * @param buffer Buffer containing the value of the new property
 * @param size Size of the new property's value
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_PROP_SERVICE_WRITE_PROP_RSP_ with attached \ref properties_service_write_rsp_msg_t
 */
void properties_service_write(cfw_service_conn_t *conn, uint16_t service_id,
			      uint16_t property_id, void *buffer, uint16_t size,
			      void *priv);

/**
 * Remove property
 *
 * @param conn service Client connection pointer.
 * @param service_id Service_id client is interested in
 * @param property_id Property_id the client is interested in
 * @param priv Private data pointer that will be passed back in the response
 *
 * @b Response: _MSG_ID_PROP_SERVICE_REMOVE_PROP_RSP_ with attached \ref properties_service_remove_rsp_msg_t
 */
void properties_service_remove(cfw_service_conn_t *conn, uint16_t service_id,
			       uint16_t property_id,
			       void *priv);

/** @} */

#endif /* __PROPERTIES_SERVICE_H__ */
