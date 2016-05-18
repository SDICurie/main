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

#ifndef __IPC_H__
#define __IPC_H__

#include "infra/ipc_requests.h"
#include "infra/message.h"

/**
 * @defgroup ipc IPC layer
 * Inter-Processor Communication (IPC)
 * @ingroup infra
 */

/**
 * @defgroup ipc_mbx IPC Mailbox interface
 * Inter-Processor Communication API used for ARC/QRK.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/ipc.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * ARC/QRK Inter-Processor Communication uses:
 * - mailboxes to wake up / acknowledge the other core
 * - shared memory to transport message data.
 *
 * To send a message or an answer to the other core:
 * - A core posts a message to the MailBox
 * - The other core receives an interrupt
 *   + It can call a callback to handle the message in interrupt context.
 *   + It can read the Mailbox by polling outside any interrupt context.
 * - The message must be freed (by receiver, if not used to send an answer).
 *
 * IPC is automatically initialized by @ref bsp_init.
 *
 * @ingroup ipc
 * @{
 */

/**
 * Initialize IPC component.
 *
 * @param tx_channel IPC Tx mailbox.
 * @param rx_channel IPC Rx mailbox.
 * @param tx_ack_channel IPC Tx acknowledge mailbox.
 * @param rx_ack_channel IPC Rx acknowledge mailbox.
 * @param remote_cpu_id Remote CPU id of this IPC
 */
void ipc_init(int tx_channel, int rx_channel, int tx_ack_channel,
	      int rx_ack_channel,
	      uint8_t remote_cpu_id);

/**
 * Request a synchronous IPC call.
 *
 * This method blocks until the command gets an answer.
 *
 * @param request_id Synchronous request id
 * @param param1 First param of the request
 * @param param2 Second param of the request
 * @param ptr Third param of the request
 *
 * @return the synchronous command response.
 */
int ipc_request_sync_int(int request_id, int param1, int param2, void *ptr);

/**
 * Notify an IPC panic.
 *
 * This method notifies a local panic through IPC. It waits for the completion
 * of an possible pending request.
 * It can be called from an interrupt context and does not wait for a response.
 *
 * @param core_id Id of the core that notifies the panic
 *
 */
void ipc_request_notify_panic(int core_id);

/**
 * Poll for pending IPC request.
 *
 * In polling mode, this method has to be called in order to handle
 * IPC requests.
 */
void ipc_handle_message();

/**
 * Synchronous callback run on received IPC request
 *
 * This callback is run in the context of an interrupt.
 *
 * @param cpu_id Id of the core that sends the request
 * @param request Request ID
 * @param param1 First parameter of the request
 * @param param2 Second parameter of the request
 * @param ptr    Third parameter of the request
 *
 * @return 0 if success, -1 otherwise
 */
int ipc_sync_callback(uint8_t cpu_id, int request, int param1, int param2,
		      void *ptr);

/**
 * Set a user callback to call when an incoming IPC synchronous request is not
 * handled by default handlers.
 *
 * This function will assert if you set this handler more than once (to avoid
 * conflicts).
 *
 * @param user_cb Callback to call.
 */
void ipc_sync_set_user_callback(int (*user_cb)(uint8_t cpu_id, int request,
					       int param1, int param2,
					       void *ptr));

/**
 * Called by port implementation when a message has to be sent to a
 * CPU connected through the mailbox IPC mechanism.
 *
 * @param message Message to send to the other core
 *
 * @return 0 if success, -1 otherwise
 */
int ipc_async_send_message(struct message *message);

/**
 * Called by port implementation when a message that comes from a
 * different CPU than the current one has to be freed.
 *
 * @param message Message to be freed.
 */
void ipc_async_free_message(struct message *message);

/**
 * Initialize the asynchrounous message send/free mechanism for remote
 * message sending and freeing.
 *
 * Asynchrounous message send/free is needed as a message can be sent/freed in
 * the context of an interrupt, but synchronous IPC requests cannot be called
 * from an interrupt context.
 *
 * @param queue Queue on which context the IPC requests have to be called.
 */
void ipc_async_init(T_QUEUE queue);

/**
 * Setup main queue and IPC for the application.
 *
 * The implementation is machine-specific.
 */
T_QUEUE ipc_setup(void);


/** @} */
#endif
