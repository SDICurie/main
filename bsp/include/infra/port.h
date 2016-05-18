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

#ifndef __INFRA_PORT_H__
#define __INFRA_PORT_H__
#include "os/os.h"
#include "infra/message.h"

/**
 * @defgroup ports Communication port management
 * The ports are a messaging feature that allows to communicate between nodes
 * in a transparent maner.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/port.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * @ingroup messaging
 * @{
 */

/** Define the maximum number of port that can be allocated */
#define MAX_PORTS       50


#ifndef CONFIG_PORT_IS_MASTER
/**
 * Allocate a port table containing the number of port specified in numport.
 *
 * @param numport Size of the port table to allocate.
 *
 * @return Address of the allocated port table.
 */
void *port_alloc_port_table(int numport);

/**
 * Set the port table structure for a remote, shared mem enabled CPU.
 *
 * @param ptbl ptbl port table to assign on ports
 */
void port_set_ports_table(void *ptbl);
#endif

/**
 * Allocate a port.
 *
 * The allocation of the port Id is global to the platform.
 * Only the Main Service Manager can allocate.
 * The slave nodes send messages to the master node in order
 * to allocate and get the port Id.
 * The message handler for this port shall be set by the
 * framework (see port_set_hanlder).
 *
 * @param queue Queue used by the port
 *
 * @return Id of the allocated port
 */
uint16_t port_alloc(void *queue);


/**
 * Set the message handler for a port.
 *
 * @param port_id Port identifier.
 * @param handler Message handler to assign to the port.
 *                - First parameter: message
 *                - Second parameter: private data as provided on registration
 * @param param Private parameter to pass to the message handler
 *              in addition to the message.
 */
void port_set_handler(uint16_t port_id, void (*handler)(struct	message *,
							void *	priv),
		      void *param);

/**
 * Call the message handler attached to this port.
 *
 * This function shall be called when a message is retrieved from a queue.
 *
 * @param msg Message to process
 */
void port_process_message(struct message *msg);

/**
 * Send a message to the destination port set in the message.
 *
 * @param msg Message to send
 *
 * @return OS_ERR_TYPE error code
 *          -# E_OS_OK : a message was read
 *          -# E_OS_ERR_OVERFLOW: the queue is full (message was not posted)
 *          -# E_OS_ERR: invalid parameter
 */
int port_send_message(struct message *msg);

/**
 * Set the port identifier of the given port.
 *
 * This is done to initialize a port in the context of a slave node.
 *
 * @param port_id Port Id to initialize
 */
void port_set_port_id(uint16_t port_id);

/**
 * Set the CPU identifier of the given port.
 *
 * This is used to know if the internal or ipc messaging needs to be used.
 *
 * @param port_id Port Id
 * @param cpu_id CPU Id to associate with the port
 */
void port_set_cpu_id(uint16_t port_id, uint8_t cpu_id);

/**
 * Get the CPU identifier of the given port.
 *
 * @param port_id Port Id
 * @return CPU Id associated with the port
 */
uint8_t port_get_cpu_id(uint16_t port_id);

/**
 * Retrieve the pointer to the global port table.
 *
 * This should only be used by the master in the context of shared memory
 * communications, in order to pass the port table to a slave node with
 * memory shared with the master node.
 *
 * @return Address of the global port table
 */
void *port_get_port_table();

/**
 * Process the first message in a queue.
 *
 * This function gets the first pending message out of the queue and
 * calls the appropriate port handler.
 * If no message is pending, the call returns immediately.
 *
 * @param queue Queue to fetch the message from
 *
 * @return Message Id of the processed message or 0 if no message
 */
uint16_t queue_process_message(T_QUEUE queue);

/**
 * Process the first message in a queue with a timeout option.
 *
 * This function gets the first pending message out of the queue and
 * calls the appropriate port handler.
 * If no message is pending, the function blocks until a message is available
 * or returns on reached timeout.
 *
 * @param queue   Queue to fetch the message from
 * @param timeout Desired timeout (in ms).
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 * @param err     Pointer where to return the error code. E_OS_OK means no error.
 *
 * @return Message Id of the processed message or 0 if no message
 */
uint16_t queue_process_message_wait(T_QUEUE queue, uint32_t timeout,
				    OS_ERR_TYPE *err);

/**
 * Multi CPU support APIs.
 */

/**
 * Set the CPU Id for this (port) instance.
 *
 * @param cpu_id CPU Id to set.
 */
void set_cpu_id(uint8_t cpu_id);

/**
 * Get the CPU id for this (port) instance.
 *
 * @return CPU Id of the instance.
 */
uint8_t get_cpu_id(void);

/**
 * Multi CPU APIs.
 */

/**
 * Set the callback to be used to send a message to the given CPU.
 *
 * @param cpu_id CPU Id for which to set the handler.
 * @param handler Callback used to send a message to this CPU.
 */
void set_cpu_message_sender(uint8_t cpu_id, int (*handler)(struct message *msg));

/**
 * Set the callback to be used to request free of the message to a CPU.
 *
 * @param cpu_id CPU Id for which to set the handler.
 * @param free_handler Callback used to request message free.
 */
void set_cpu_free_handler(uint8_t cpu_id, void (*free_handler)(struct message *));
/**@} */
#endif /* __INFRA_PORT_H_ */
