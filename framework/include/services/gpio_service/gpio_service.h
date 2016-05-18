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

#ifndef __GPIO_SERVICE_H__
#define __GPIO_SERVICE_H__

#include <stdint.h>
#include "cfw/cfw.h"
#include "services/services_ids.h"
#include "drivers/data_type.h"

/**
 * @defgroup gpio_service GPIO Service
 * General Purpose Input/Output (GPIO)
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/gpio_service/gpio_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/gpio_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_GPIO, SERVICES_QUARK_SE_GPIO_IMPL</tt>
 * <tr><th><b>Service Id</b>  <td><tt>SS_GPIO_SERVICE_ID | SOC_GPIO_SERVICE_ID | AON_GPIO_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

/** Message ID of service response for @ref gpio_service_configure */
#define MSG_ID_GPIO_SERVICE_CONFIGURE_RSP       ((MSG_ID_GPIO_SERVICE_BASE + \
						  0) | 0x40)
/** Message ID of service response for @ref gpio_service_set_state */
#define MSG_ID_GPIO_SERVICE_SET_STATE_RSP       ((MSG_ID_GPIO_SERVICE_BASE + \
						  1) | 0x40)
/** Message ID of service response for @ref gpio_service_get_state */
#define MSG_ID_GPIO_SERVICE_GET_STATE_RSP       ((MSG_ID_GPIO_SERVICE_BASE + \
						  2) | 0x40)
/** Message ID of service response for @ref gpio_service_listen */
#define MSG_ID_GPIO_SERVICE_LISTEN_RSP          ((MSG_ID_GPIO_SERVICE_BASE + \
						  3) | 0x40)
/** Message ID of service response for @ref gpio_service_unlisten */
#define MSG_ID_GPIO_SERVICE_UNLISTEN_RSP        ((MSG_ID_GPIO_SERVICE_BASE + \
						  4) | 0x40)
/** Message ID of service response for @ref gpio_service_get_bank_state */
#define MSG_ID_GPIO_SERVICE_GET_BANK_STATE_RSP  ((MSG_ID_GPIO_SERVICE_BASE + \
						  5) | 0x40)
/** Message ID of service response for GPIO events */
#define MSG_ID_GPIO_SERVICE_EVT                 (MSG_ID_GPIO_SERVICE_BASE | \
						 0x80)

/** GPIO interrupt types */
typedef enum {
	RISING_EDGE,
	FALLING_EDGE,
	BOTH_EDGE // Used on soc GPIO only
} gpio_service_isr_mode_t;

/** Debounce configuration */
typedef enum {
	DEB_OFF = 0,
	DEB_ON
} gpio_service_debounce_mode_t;

/** Response message structure for @ref gpio_service_configure */
typedef struct gpio_service_configure_rsp_msg {
	struct cfw_message header; /*!< Message header */
	int status;             /*!< Response status code.*/
} gpio_service_configure_rsp_msg_t;

/** Response message structure for @ref gpio_service_set_state */
typedef struct gpio_service_set_state_rsp_msg {
	struct cfw_message header; /*!< Message header */
	int status;             /*!< Response status code.*/
} gpio_service_set_state_rsp_msg_t;

/** Response message structure for @ref gpio_service_get_state */
typedef struct gpio_service_get_state_rsp_msg {
	struct cfw_message header; /*!< Message header */
	bool state;             /*!< Current state of the gpio */
	int status;             /*!< Response status code.*/
} gpio_service_get_state_rsp_msg_t;

/** Response message structure @ref gpio_service_listen */
typedef struct gpio_service_listen_rsp_msg {
	struct cfw_message header; /*!< Message header */
	uint8_t index;          /*!< Index of GPIO pin to monitor */
	int status;             /*!< Response status code.*/
} gpio_service_listen_rsp_msg_t;

/** Event message structure for @ref gpio_service_listen */
typedef struct gpio_service_listen_evt_msg {
	struct cfw_message header; /*!< Message header */
	uint8_t index;          /*!< Index of GPIO which triggered the interrupt */
	bool pin_state;         /*!< Current gpio state */
	uint32_t timestamp;     /*!< Gpio event timestamp */
	int status;             /*!< Response status code.*/
} gpio_service_listen_evt_msg_t;

/** Response message structure @ref gpio_service_unlisten */
typedef struct gpio_service_unlisten_rsp_msg {
	struct cfw_message header; /*!< Message header */
	uint8_t index;          /*!< Index of GPIO to stop monitoring */
	int status;             /*!< Response status code.*/
} gpio_service_unlisten_rsp_msg_t;

/** Response message structure @ref gpio_service_get_bank_state*/
typedef struct gpio_service_get_bank_state_rsp_msg {
	struct cfw_message header; /*!< Message header */
	uint32_t state;         /*!< State of the gpio port */
	int status;             /*!< Response status code.*/
} gpio_service_get_bank_state_rsp_msg_t;


/**
 * Configure a GPIO line.
 *
 * @param service_conn Service connection
 * @param index GPIO index in the port to configure
 * @param mode Mode of the GPIO line (0 - input, 1 - output)
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_CONFIGURE_RSP_ with attached \ref gpio_service_configure_rsp_msg_t
 */
void gpio_service_configure(cfw_service_conn_t *service_conn, uint8_t index,
			    uint8_t mode,
			    void *priv);

/**
 * Set the state of a GPIO line.
 *
 * @param service_conn Service connection
 * @param index GPIO index in the port to configure
 * @param value State to set the GPIO line (0 - low level, 1 - high level)
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_SET_RSP_ with attached \ref gpio_service_set_state_rsp_msg_t
 */
void gpio_service_set_state(cfw_service_conn_t *service_conn, uint8_t index,
			    uint8_t value,
			    void *priv);

/**
 * Get state of a GPIO line
 *
 * @param service_conn Service connection
 * @param index GPIO index in the port to read
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_GET_RSP_ with attached \ref gpio_service_get_state_rsp_msg_t
 * GPIO state will be available in the _state_ field.
 */
void gpio_service_get_state(cfw_service_conn_t *service_conn, uint8_t index,
			    void *priv);

/**
 * Get state of a GPIO bank
 *
 * @param service_conn Service connection
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_GET_BANK_RSP_ with attached \ref gpio_service_get_bank_state_rsp_msg_t
 * GPIO state will be available in the _state_ field.
 */
void gpio_service_get_bank_state(cfw_service_conn_t *service_conn, void *priv);

/**
 * Register to gpio state change.
 *
 * @param service_conn Service connection
 * @param pin GPIO index in the port to configure
 * @param mode Interrupt mode (RISING_EDGE, FALLING_EDGE)
 * @param debounce Debounce config (DEB_OFF, DEB_ON)
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_LISTEN_RSP_ with attached \ref gpio_service_listen_rsp_msg_t
 * @b Event: _MSG_ID_GPIO_SERVICE_EVT_ with attached \ref gpio_service_listen_evt_msg_t
 *
 * @msc
 *  Client,"GPIO Service","GPIO driver";
 *
 *  "GPIO Service"=>"GPIO driver" [label="call driver configuration"];
 *  "GPIO Service"<<"GPIO driver" [label="configuration status"];
 *  Client<-"GPIO Service" [label="status", URL="\ref gpio_service_listen_rsp_msg_t"];
 *  --- [label="GPIO state change"];
 *  Client<-"GPIO Service" [label="state change event", URL="\ref gpio_service_listen_evt_msg_t"];
 * @endmsc
 */
void gpio_service_listen(cfw_service_conn_t *service_conn, uint8_t pin,
			 gpio_service_isr_mode_t mode, uint8_t debounce,
			 void *priv);

/**
 * Unregister to gpio state change.
 *
 * @param service_conn Service connection
 * @param pin GPIO index in the port to configure
 * @param priv Private data pointer passed back in the response message.
 *
 * @b Response: _MSG_ID_GPIO_SERVICE_UNLISTEN_RSP_ with attached \ref gpio_service_unlisten_rsp_msg_t
 */
void gpio_service_unlisten(cfw_service_conn_t *service_conn, uint8_t pin,
			   void *priv);

/** @} */

#endif
