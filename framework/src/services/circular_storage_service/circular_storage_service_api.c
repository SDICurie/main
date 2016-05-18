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

#include "cfw/cfw_service.h"
#include "circular_storage_service_private.h"
#include "services/circular_storage_service/circular_storage_service.h"

/****************************************************************************************
*********************** SERVICE API IMPLEMENTATION **************************************
****************************************************************************************/
void circular_storage_service_get(cfw_service_conn_t *	conn,
				  uint32_t		key,
				  void *		priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		conn, MSG_ID_CIRCULAR_STORAGE_GET_REQ,
		sizeof(
			circular_storage_get_req_msg_t), priv);
	circular_storage_get_req_msg_t *req =
		(circular_storage_get_req_msg_t *)msg;

	req->key = key;
	cfw_send_message(msg);
}

void circular_storage_service_push(cfw_service_conn_t * conn,
				   uint8_t *		buffer,
				   void *		storage,
				   void *		priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		conn, MSG_ID_CIRCULAR_STORAGE_PUSH_REQ,
		sizeof(
			circular_storage_push_req_msg_t), priv);
	circular_storage_push_req_msg_t *req =
		(circular_storage_push_req_msg_t *)msg;

	req->buffer = buffer;
	req->storage = storage;
	cfw_send_message(msg);
}

void circular_storage_service_pop(cfw_service_conn_t *	conn,
				  void *		storage,
				  void *		priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		conn, MSG_ID_CIRCULAR_STORAGE_POP_REQ,
		sizeof(
			circular_storage_pop_req_msg_t), priv);
	circular_storage_pop_req_msg_t *req =
		(circular_storage_pop_req_msg_t *)msg;

	req->storage = storage;
	cfw_send_message(msg);
}

void circular_storage_service_peek(cfw_service_conn_t * conn,
				   void *		storage,
				   void *		priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		conn, MSG_ID_CIRCULAR_STORAGE_PEEK_REQ,
		sizeof(
			circular_storage_peek_req_msg_t), priv);
	circular_storage_peek_req_msg_t *req =
		(circular_storage_peek_req_msg_t *)msg;

	req->storage = storage;
	cfw_send_message(msg);
}

void circular_storage_service_clear(cfw_service_conn_t *conn,
				    void *		storage,
				    uint32_t		elt_count,
				    void *		priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		conn, MSG_ID_CIRCULAR_STORAGE_CLEAR_REQ,
		sizeof(
			circular_storage_clear_req_msg_t), priv);
	circular_storage_clear_req_msg_t *req =
		(circular_storage_clear_req_msg_t *)msg;

	req->elt_count = elt_count;
	req->storage = storage;
	cfw_send_message(msg);
}
