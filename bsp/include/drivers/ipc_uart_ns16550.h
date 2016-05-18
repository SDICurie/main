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

#ifndef _IPC_UART_NS16550_H_
#define _IPC_UART_NS16550_H_

#include "infra/device.h"

/**
 * @defgroup ipc_uart_ns16550 IPC UART NS16550 Driver
 * NS16550 IPC UART driver.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/ipc_uart_ns16550.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/ipc</tt>
 * <tr><th><b>Config flag</b> <td><tt>IPC_UART_NS16550</tt>
 * </table>
 *
 * This driver manages the communication between BLE Core and Quark through UART.
 * It also addresses power management.
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 * IPC UART power management driver.
 */
extern struct driver ipc_uart_ns16550_driver;

/**
 * IPC UART driver management structure
 */
struct ipc_uart_info {
	struct device *uart_dev;        /*!< UART device to use */
	uint32_t irq_vector;            /*!< IRQ number */
	uint32_t irq_mask;              /*!< IRQ mask */
	void (*tx_cb)(bool wake_state, void *); /*!< Callback to be called to set wake state when TX is starting or ending */
	void *tx_cb_param;              /*!< tx_cb function parameter */
	struct pm_wakelock rx_wl;       /*!< UART RX wakelock */
	struct pm_wakelock tx_wl;       /*!< UART TX wakelock */
};

/** IPC UART return codes */
enum IPC_UART_RESULT_CODES {
	IPC_UART_ERROR_OK = 0,
	IPC_UART_ERROR_DATA_TO_BIG,
	IPC_UART_TX_BUSY /**< A transmission is already ongoing, message is NOT sent */
};

/**
 * Channel list
 */
enum ipc_channels {
	RPC_CHANNEL=0, /**< RPC channel */
	IPC_UART_MAX_CHANNEL
};

/**
 * Channel state
 */
enum ipc_channel_state {
	IPC_CHANNEL_STATE_CLOSED,
	IPC_CHANNEL_STATE_OPEN
};

/**
 * Definitions valid for NONE sync IPC UART headers
 * |len|channel|cpu_id|request|payload|
 *
 * len = len(request)+len(payload)
 */

/**
 * @note this structure must be self-aligned and self-packed
 */
struct ipc_uart_header {
	uint16_t len;       /**< Length of IPC message, (request + payload) */
	uint8_t channel;    /**< Channel number of IPC message. */
	uint8_t src_cpu_id; /**< CPU id of IPC sender. */
};

/**
 * IPC channel description
 */
struct ipc_uart_channels {
	uint16_t index; /**< Channel number */
	uint16_t state; /**< @ref ipc_channel_state */
	int (*cb)(int chan, int request, int len, void *data);
	/**< Callback of the channel.
	 * @param chan Channel index used
	 * @param request Request id (defined in ipc_requests.h)
	 * @param len Payload size
	 * @param data Pointer to data
	 */
};

/**
 * Send of PDU buffer over IPC UART.
 *
 * This constructs an IPC message header and triggers the sending of it and message buffer. If a transmission
 * is already ongoing, it will fail. In this case upper layer needs to queue the message buffer.
 *
 * @param dev IPC UART device to use
 * @param handle Opened IPC UART channel handle
 * @param len Length of message to send
 * @param p_data Message buffer to send
 *
 * @return
 *  - IPC_UART_ERROR_OK TX has been initiated
 *  - IPC_UART_TX_BUSY a transmission is already going, message needs to be queued
 *
 * @note This function needs to be executed with (UART) irq off to avoid pre-emption from IPC UART isr
 * causing state variable corruption. It also called from IPC UART isr to send the next IPC message.
 */
int ipc_uart_ns16550_send_pdu(struct td_device *dev, void *handle, int len,
			      void *p_data);

/**
 * Register a callback function being called on TX start/end.
 *
 * @param dev IPC UART device to use
 * @param cb Callback to be called to set wake state when TX is starting or ending
 *           - 1st parameter is the wake state
 *           - 2nd parameter is the parameter given on callback registration
 * @param param Parameter passed to cb when being called
 *
 */
void ipc_uart_ns16550_set_tx_cb(struct td_device *dev, void (*cb)(bool,
								  void *),
				void *param);

/**
 * Disable an IPC UART device
 *
 * This disables all the IPC channels opened on this device.
 *
 * @param dev IPC UART device to use
 */
void ipc_uart_ns16550_disable(struct td_device *dev);

/**
 * Open an IPC UART channel and define the callback function for receiving IPC messages.
 *
 * @param channel IPC channel ID to use
 * @param cb      Callback to handle messages
 *                - 1st parameter `chan` is the IPC UART channel handle
 *                - 2nd parameter `request` is the IPC message type
 *                - 3nd parameter `len` is the length of the message
 *                - 4th parameter `data` points to the message content
 *
 * @return
 *         - Pointer to channel structure if success,
 *         - NULL if opening fails.
 */
void *ipc_uart_channel_open(int channel,
			    int (*cb)(int chan, int request, int len,
				      void *data));

/** @} */

#endif /* _IPC_UART_NS16550_H_ */
