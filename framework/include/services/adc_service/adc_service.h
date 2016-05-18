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

#ifndef __ADC_SERVICE_H__
#define __ADC_SERVICE_H__

#include <stdint.h>

#include "cfw/cfw.h"

#include "services/services_ids.h"

#include "drivers/data_type.h"

/**
 * @defgroup adc_service ADC Service
 * Analog to Digital Converter (ADC).
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/adc_service/adc_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/adc_service/</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_QUARK_SE_ADC, SERVICES_QUARK_SE_ADC_IMPL</tt>
 * <tr><th><b>Service Id</b>  <td><tt>SS_ADC_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

/** ADC Service message definitions */
#define MSG_ID_ADC_SERVICE_RSP                          ( \
		MSG_ID_ADC_SERVICE_BASE + 0x40)
#define MSG_ID_ADC_SERVICE_EVT                          ( \
		MSG_ID_ADC_SERVICE_BASE + 0x80)

/** Message ID for service response */
#define MSG_ID_ADC_SERVICE_GET_VAL_RSP          (MSG_ID_ADC_SERVICE_RSP | 0x5)
#define MSG_ID_ADC_SERVICE_SUBSCRIBE_RSP        (MSG_ID_ADC_SERVICE_RSP | 0x6)
#define MSG_ID_ADC_SERVICE_UNSUBSCRIBE_RSP      (MSG_ID_ADC_SERVICE_RSP | 0x7)

/** Message ID of service response for ADC events */
#define MSG_ID_ADC_SERVICE_GET_VAL_EVT                  (MSG_ID_ADC_SERVICE_EVT)

/** ID for ADC RX interrupt */
#define ADC_EVT_RX                      0
/** ID for ADC ERR interrupt */
#define ADC_EVT_ERR                     1

/**
 * @ref adc_service_subscribe response message structure
 */
typedef struct {
	struct cfw_message header;              /*!< Header message */
	void *adc_sub_conn_handle;              /*!< Subscribe connection handle */
	int status;                             /*!< Response status code.*/
} adc_service_subscribe_rsp_msg_t;

/**
 * Event message structure for @ref adc_service_subscribe
 */
typedef struct {
	struct cfw_message header;
	uint16_t adc_value;                     /*!< Value read by ADC */
	uint32_t timestamp;                     /*!< Timestamp on read */
	int status;
} adc_service_get_evt_msg_t;

/**
 * @ref adc_service_get_value response message structure
 */
typedef struct adc_service_get_val_rsp_msg {
	struct cfw_message header;              /*!< Header message */
	uint32_t value;                         /*!< Value read by ADC */
	uint8_t reason;                         /*!< Reason of event (ADC_EVT_RX or ADC_EVT_ERR). */
	int status;                             /*!< Response status code.*/
} adc_service_get_val_rsp_msg_t;

/**
 * Get ADC sample value for a channel.
 *
 * @msc
 * Client,"ADC Service","ADC Driver";
 *
 * Client->"ADC Service" [label="ADC get value request"];
 * "ADC Service"=>"ADC Driver" [label="ADC read"];
 * "ADC Service"<<="ADC Driver" [label="ADC Value/Error"];
 * Client<-"ADC Service" [label="ADC get value response", URL="\ref adc_service_get_val_rsp_msg_t"];
 *
 * @endmsc
 *
 * @param service_conn Service connection handle.
 * @param channel ADC specific channel [0..18].
 * @param priv Private data that will be passed back in the response message.
 *
 * @b Response: _MSG_ID_ADC_SERVICE_GET_VAL_RSP_ with attached \ref adc_service_get_val_rsp_msg_t
 * ADC value will be available in _value_ field.
 * Panic if issue is encountered during allocation of message.
 */
void adc_service_get_value(cfw_service_conn_t *service_conn, uint32_t channel,
			   void *priv);

/**
 * Subscribe request from clients to ADC service for periodic conversions.
 *
 * @msc
 * Client,"ADC Service","ADC Driver","timer";
 *
 * Client=>"ADC Service" [label="ADC subscribe request(channel, gpio, time)"];
 * "ADC Service"=>"timer" [label="set timer (time)"];
 * "ADC Service"<<="timer" [label="timeout"];
 * "ADC Service"=>"ADC Driver" [label="Read adc value"];
 * "ADC Service"<<="ADC Driver" [label="adc value"];
 * Client<<="ADC Service" [label="ADC value notification", URL="\ref adc_service_get_evt_msg_t"];
 * "ADC Service"<<="timer" [label="timeout"];
 * "ADC Service"=>"ADC Driver" [label="Read adc value"];
 * "ADC Service"<<="ADC Driver" [label="adc value"];
 * Client<<="ADC Service" [label="ADC value notification", URL="\ref adc_service_get_evt_msg_t"];
 *
 * @endmsc
 *
 * @param service_conn Service connection handle.
 * @param channel ADC specific channel [0..18]
 * @param gpio_pin SS GPIO pin number to toggle.
 * @param active_high If the GPIO is active high.
 * @param time1 Time in ms after the gpio is set active. If 0, gpio_pin argument is ignored and no GPIO is toggled.
 * @param time2 Time in ms between the set of the gpio and the actual read of the ADC.
 * @param priv Private data that will be passed back in the response and the event messages.
 *
 * @b Response: _MSG_ID_ADC_SERVICE_SUBSCRIBE_RSP_ with attached \ref adc_service_subscribe_rsp_msg_t
 */
void adc_service_subscribe(cfw_service_conn_t *service_conn, uint32_t channel,
			   uint8_t gpio_pin, bool active_high, uint32_t time1,
			   uint32_t time2,
			   void *priv);

/**
 * Unsubscribe request from clients to ADC service.
 *
 * @param service_conn Service connection handle.
 * @param adc_svc_client Subscribe connection handle ad received in \ref adc_service_subscribe_rsp_msg_t.
 *
 * @b Response: _MSG_ID_ADC_SERVICE_UNSUBSCRIBE_RSP_
 */
void adc_service_unsubscribe(cfw_service_conn_t *	service_conn,
			     void *			adc_svc_client);

/** @} */

#endif /* __ADC_SERVICE_H__ */
