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

#include "services/adc_service/adc_service.h"
#include "cfw/cfw_service.h"
#include "adc_service_private.h"

/****************************************************************************************
*********************** SERVICE API IMPLEMENATION **************************************
****************************************************************************************/
void adc_service_get_value(cfw_service_conn_t *c, uint32_t channel, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		c, MSG_ID_ADC_GET_VAL_REQ,
		sizeof(
			adc_service_get_val_req_msg_t), priv);
	adc_service_get_val_req_msg_t *req =
		(adc_service_get_val_req_msg_t *)msg;

	req->channel = channel;
	cfw_send_message(msg);
}

void adc_service_subscribe(cfw_service_conn_t *c, uint32_t channel,
			   uint8_t gpio_pin,
			   bool active_high, uint32_t time1, uint32_t time2,
			   void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		c, MSG_ID_ADC_SUBSCRIBE_REQ,
		sizeof(
			adc_service_cli_req_t), priv);
	adc_service_cli_req_t *req = (adc_service_cli_req_t *)msg;

	req->adc_service_conn = c;
	req->adc_channel = channel;
	req->gpio_pin = gpio_pin;
	req->active_high = active_high;
	req->time1 = time1;
	req->time2 = time2;
	cfw_send_message(msg);
}

void adc_service_unsubscribe(cfw_service_conn_t *c, void *adc_svc_client)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		c, MSG_ID_ADC_UNSUBSCRIBE_REQ,
		sizeof(
			adc_service_cli_req_t), adc_svc_client);

	cfw_send_message(msg);
}
