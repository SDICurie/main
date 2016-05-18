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

#ifndef __INFRA_MESSAGE_H_
#define __INFRA_MESSAGE_H_

#include "os/os.h"
#include "util/list.h"

/**
 * @defgroup messaging Messaging infrastructure
 * Messaging infrastructure
 *
 * The inter-component messaging system make use of the "port" paradigm
 * that contains enough information to reach the destination of a message.
 *
 * A port is uniquely identified by an Id. The internal port structure
 * contains information about the core that created the port and thus
 * the way to route the message to the destination.
 *
 * Messaging scheme can be either IPC if the message crosses a CPU boundary
 * or internal queuing if the message stays on the same CPU.
 *
 * In order to avoid message copies in case the initiator and the destination
 * of a message share the same memory space, the address of the message
 * is exchanged and a mechanism to free a message is available.
 *
 * In case the message is copied through the IPC, the IPC layer should free
 * the message once the message is sent.
 *
 * @ingroup infra
 */

/**
 * @defgroup message Message
 * Message structure defintion
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/message.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * @ingroup messaging
 * @{
 */

/* Message ranges */
#define INFRA_MSG_TCMD_BASE             0xFF00


/**
 * Message types definition.
 */
typedef enum {
	TYPE_REQ,
	TYPE_RSP,
	TYPE_EVT,
	TYPE_INT
} msg_type_t;

/**
 * Message class definition.
 */
typedef enum {
	CLASS_NORMAL,
	CLASS_NO_WAKE,
	CLASS_NO_WAKE_REPLACE,
	CLASS_REPLACE
} msg_class_t;

/**
 * Message flags definitions.
 */
struct msg_flags {
	uint16_t f_prio : 8;            /*!< Priority */
	uint16_t f_class : 3;           /*!< Class */
	uint16_t f_type : 2;            /*!< Type */
	uint16_t f_queue_head : 1;      /*!< Insert at the queue head */
	uint16_t f_is_job : 1;          /*!< Message is a job */
};

/**
 * Message header structure definition.
 * Data message follows the header structure.
 * Header must be packed in order to ensure that
 * it can be simply exchanged between embedded cores.
 */
struct message {
	/** The message flags */
	struct msg_flags flags;
	/** Message identifier */
	uint16_t id;
	/** Message destination port */
	uint16_t dst_port_id;
	/** Message source port */
	uint16_t src_port_id;
	/** Message length */
	uint16_t len;
};

/** Retrieve message identifier */
#define MESSAGE_ID(msg)     (msg)->id
/** Retrieve message source port */
#define MESSAGE_SRC(msg)    (msg)->src_port_id
/** Retrieve message destination port */
#define MESSAGE_DST(msg)    (msg)->dst_port_id
/** Retrieve message length */
#define MESSAGE_LEN(msg)    (msg)->len
/** Retrieve f_type flag of message */
#define MESSAGE_TYPE(msg)   (msg)->flags.f_type
/** Retrieve f_prio flag of message */
#define MESSAGE_PRIO(msg)   (msg)->flags.f_prio
/** Retrieve f_class flag of message */
#define MESSAGE_CLASS(msg)  (msg)->flags.f_class
/** Retrieve f_queue_head flag of message */
#define MESSAGE_QUEUE_HEAD(msg)  (msg)->flags.f_queue_head

/**
 * Allocate a message
 *
 * @param size Size of the message to allocate.
 *             This includes the header and the following data.
 * @param err Pointer where to return the return code.
 *            If `err` is NULL, the function will panic in case of allocation
 *            failure.
 *
 * @return Address of the allocated message or
 *         NULL if allocation failed and `err` != NULL
 */
struct message *message_alloc(int size, OS_ERR_TYPE *err);

/**
 * Free an allocated message
 *
 * @param message Message allocated with message_alloc()
 */
void message_free(struct message *message);

/** @} */
#endif /* __INFRA_MESSAGE_H_ */
