/*
 * Copyright (c) 2016, Intel Corporation. All rights reserved.
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

#ifndef __ADC_SERVICE_PRIVATE_H__
#define __ADC_SERVICE_PRIVATE_H__

/** ADC Service message definitions */
#define MSG_ID_ADC_REQ                  (MSG_ID_ADC_SERVICE_BASE + 0x0)

/** ADC service internal message ID */
#define MSG_ID_ADC_GET_VAL_REQ          (MSG_ID_ADC_REQ | 0x5)
#define MSG_ID_ADC_SUBSCRIBE_REQ        (MSG_ID_ADC_REQ | 0x6)
#define MSG_ID_ADC_UNSUBSCRIBE_REQ      (MSG_ID_ADC_REQ | 0x7)

/** ADC min channel ID */
#define ADC_MIN_CHANNEL             0
/** ADC max channel ID */
#define ADC_MAX_CHANNEL             18

/**
 * Request message structure
 */
typedef struct adc_service_get_val_req_msg {
	struct cfw_message header;      /*!< header message */
	uint32_t channel;               /*!< index of ADC channel */
} adc_service_get_val_req_msg_t;

/**
 * ADC service client structure
 */
typedef struct adc_service_cli_req {
	struct cfw_message header;
	cfw_service_conn_t *adc_service_conn;   /*!< the service connection */
	uint32_t adc_channel;   /*!< Channel to read ADC value */
	uint32_t time1;         /*!< Time1 in ms. time after the gpio is set active */
	uint32_t time2;         /*!< Time2 in ms. time between the set of the gpio and the actual read of the ADC  */
	uint8_t gpio_pin;       /*!< SS GPIO pin number to toggle */
	bool active_high;       /*!< Polarity of the GPIO */
	void *priv;             /*!< Priv data from client */
} adc_service_cli_req_t;

#endif /* __ADC_SERVICE_PRIVATE_H__ */
