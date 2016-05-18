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


#ifndef __GPIO_SERVICE_PRIVATE_H__
#define __GPIO_SERVICE_PRIVATE_H__

#include "services/gpio_service/gpio_service.h"

/** service internal message ID for gpio_service_configure */
#define MSG_ID_GPIO_CONFIGURE_REQ   MSG_ID_GPIO_SERVICE_BASE
/** service internal message ID for gpio_service_set_state */
#define MSG_ID_GPIO_SET_REQ         (MSG_ID_GPIO_SERVICE_BASE + 1)
/** service internal message ID for gpio_service_get_state */
#define MSG_ID_GPIO_GET_REQ         (MSG_ID_GPIO_SERVICE_BASE + 2)
/** service internal message ID for gpio_service_listen */
#define MSG_ID_GPIO_LISTEN_REQ      (MSG_ID_GPIO_SERVICE_BASE + 3)
/** service internal message ID for gpio_service_unlisten */
#define MSG_ID_GPIO_UNLISTEN_REQ    (MSG_ID_GPIO_SERVICE_BASE + 4)
/** service internal message ID for gpio_service_get_bank_state */
#define MSG_ID_GPIO_GET_BANK_REQ    (MSG_ID_GPIO_SERVICE_BASE + 5)

/** Request message structure for gpio_service_configure */
typedef struct gpio_configure_req_msg {
	struct cfw_message header;
	gpio_service_isr_mode_t mode; /*!< requested mode for each gpio: 0 - gpio is an output; 1 - gpio is an input */
	uint8_t index;            /*!< index of the gpio to configure in the port */
} gpio_configure_req_msg_t;

/** Request message structure for gpio_service_set_state */
typedef struct gpio_set_req_msg {
	struct cfw_message header;
	uint8_t index; /*!< index of the gpio in the port */
	uint8_t state; /*!< state of the gpio to set */
} gpio_set_req_msg_t;

/** Request message structure for gpio_service_get_state */
typedef struct gpio_get_req_msg {
	struct cfw_message header;
	uint8_t index; /*!< index of the gpio in the port */
} gpio_get_req_msg_t;

/** Request message structure for gpio_service_listen */
typedef struct gpio_listen_req_msg {
	struct cfw_message header;
	uint8_t index;                     /*!< index of GPIO pin to monitor */
	gpio_service_isr_mode_t mode;      /*!< interrupt mode */
	gpio_service_debounce_mode_t debounce; /*!< debounce mode */
} gpio_listen_req_msg_t;

/** Request message structure for gpio_service_unlisten */
typedef struct gpio_unlisten_req_msg {
	struct cfw_message header;
	uint8_t index; /*!< index of GPIO to monitor */
} gpio_unlisten_req_msg_t;

/** Request message structure for gpio_service_get_bank_state */
typedef struct gpio_get_bank_req_msg {
	struct cfw_message header;
} gpio_get_bank_req_msg_t;

#endif
