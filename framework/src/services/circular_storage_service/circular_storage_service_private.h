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

#ifndef __CIRCULAR_STORAGE_SERVICE_PRIVATE_H__
#define __CIRCULAR_STORAGE_SERVICE_PRIVATE_H__

#include <stdint.h>

#include "cfw/cfw.h"
#include "services/services_ids.h"

#define MSG_ID_CIRCULAR_STORAGE_PUSH_REQ               ( \
		MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE + 5)
#define MSG_ID_CIRCULAR_STORAGE_POP_REQ                ( \
		MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE + 6)
#define MSG_ID_CIRCULAR_STORAGE_PEEK_REQ               ( \
		MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE + 7)
#define MSG_ID_CIRCULAR_STORAGE_CLEAR_REQ              ( \
		MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE + 8)
#define MSG_ID_CIRCULAR_STORAGE_GET_REQ                ( \
		MSG_ID_CIRCULAR_STORAGE_SERVICE_BASE + 9)

typedef struct circular_storage_get_req_msg {
	struct cfw_message header;
	uint32_t key;
} circular_storage_get_req_msg_t;

typedef struct circular_storage_push_req_msg {
	struct cfw_message header;
	void *storage;
	uint8_t *buffer;
} circular_storage_push_req_msg_t;

typedef struct circular_storage_pop_req_msg {
	struct cfw_message header;
	void *storage;
} circular_storage_pop_req_msg_t;

typedef struct circular_storage_peek_req_msg {
	struct cfw_message header;
	void *storage;
} circular_storage_peek_req_msg_t;

typedef struct circular_storage_clear_req_msg {
	struct cfw_message header;
	uint32_t elt_count;
	void *storage;
} circular_storage_clear_req_msg_t;

#endif /* __CIRCULAR_STORAGE_SERVICE_PRIVATE_H__ */
