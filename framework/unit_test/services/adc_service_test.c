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

#include <microkernel.h>
#include "cfw/cfw.h"

#include "cfw/cproxy.h"
#include "services/adc_service/adc_service.h"
#include "util/cunit_test.h"
#include "service_tests.h"

#define TEST_CHANNEL    4
#define NO_OF_REQUEST   6
#define SS_GPIO_SW_FG_VOLT_EN 14
#define GPIO_SET_LOW 0
#define GPIO_SET_HIGH 1
#define ADC_GET_TIME 4000
#define GPIO_SET_TIME 2000

#define ADC_CU_ASSERT(msg, cdt)	\
	do { CU_ASSERT(msg, cdt); \
	     if (!(cdt)) { return; } } while (0)

static DRIVER_API_RC adc_read_resp = DRV_RC_INVALID_OPERATION;
static cfw_service_conn_t *adc_service_handle = NULL;
static uint8_t req_cnt[NO_OF_REQUEST] = { 1 };
static uint8_t resp_cnt = 0;
static uint8_t adc_evt_counter;
static void *adc_svc_client[2];
static int8_t idx;
static void adc_handle_msg(struct cfw_message *msg, void *data)
{
	DRIVER_API_RC ret = DRV_RC_FAIL;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_ADC_SERVICE_GET_VAL_RSP:
		adc_read_resp = ((adc_service_get_val_rsp_msg_t *)msg)->status;
		if (adc_read_resp == DRV_RC_OK) {
			if (((adc_service_get_val_rsp_msg_t *)msg)->reason ==
			    ADC_EVT_RX) {
				CU_ASSERT(
					"SS_ADC value is negative",
					((adc_service_get_val_rsp_msg_t *)msg)
					->value >= 0);
			}
			cu_print(
				"Received Response for Request: %d with adc value: %d\n",
				*(uint8_t *)msg->priv,
				((adc_service_get_val_rsp_msg_t *)msg)->value);
			CU_ASSERT("Response received not in request order",
				  ++resp_cnt == *(uint8_t *)msg->priv);
		} else if (adc_read_resp == DRV_RC_FAIL) {
			cu_print(
				"Received Error response for Request: %d with adc value: %d\n",
				*(uint8_t *)msg->priv,
				((adc_service_get_val_rsp_msg_t *)msg)->value);
			CU_ASSERT("Response received not in request order",
				  ++resp_cnt == *(uint8_t *)msg->priv);
		}
		break;
	case MSG_ID_ADC_SERVICE_SUBSCRIBE_RSP:
		adc_svc_client[idx++] =
			((adc_service_subscribe_rsp_msg_t *)msg)->
			adc_sub_conn_handle;
		ret = ((adc_service_subscribe_rsp_msg_t *)msg)->status;
		CU_ASSERT("ADC subscribe fails", ret == DRV_RC_OK);
		break;
	case MSG_ID_ADC_SERVICE_UNSUBSCRIBE_RSP:
		ret = ((adc_service_subscribe_rsp_msg_t *)msg)->status;
		CU_ASSERT("ADC Unsubscribe fails", ret == DRV_RC_OK);
		adc_evt_counter++;
		break;
	case MSG_ID_ADC_SERVICE_GET_VAL_EVT:
		ret = ((adc_service_get_evt_msg_t *)msg)->status;
		CU_ASSERT("ADC get value fails", ret == DRV_RC_OK);
		adc_evt_counter++;
		break;
	default:
		cu_print("default cfw handler\n");
		break;
	}
	cfw_msg_free(msg);
}

void adc_service_subscribe_test()
{
	uint64_t timeout = 0;

	cu_print(
		"##################################################################\n");
	cu_print(
		"# Purpose of ADC service subscribe test :                        #\n");
	cu_print(
		"# subscribe the adc service and get the adc value for given time##\n");
	cu_print(
		"##################################################################\n");

	adc_service_subscribe(adc_service_handle, TEST_CHANNEL,
			      SS_GPIO_SW_FG_VOLT_EN, GPIO_SET_HIGH,
			      ADC_GET_TIME, GPIO_SET_TIME, NULL);
	adc_service_subscribe(adc_service_handle, 10,
			      SS_GPIO_SW_FG_VOLT_EN, GPIO_SET_HIGH,
			      ADC_GET_TIME, GPIO_SET_TIME, NULL);

	while ((++timeout) <= 0xFFFFFF) {
		queue_process_message(get_test_queue());
		if (adc_evt_counter == NO_OF_REQUEST)
			break;
	}
	ADC_CU_ASSERT("Timeout expired, not adc read response",
		      timeout <= 0xFFFFFF);
	ADC_CU_ASSERT("Response not received for all request\n",
		      adc_evt_counter == NO_OF_REQUEST);
	timeout = 0;
	adc_service_unsubscribe(adc_service_handle, adc_svc_client[0]);
	adc_service_unsubscribe(adc_service_handle, adc_svc_client[1]);
	while ((++timeout) <= 0xFFFFFF) {
		queue_process_message(get_test_queue());
		if (adc_evt_counter == NO_OF_REQUEST + 1)
			break;
	}
	ADC_CU_ASSERT("Timeout expired, not adc read response",
		      timeout <= 0xFFFFFF);
	ADC_CU_ASSERT("Response not received for all request\n",
		      adc_evt_counter == NO_OF_REQUEST + 1);
}

void adc_service_test()
{
	uint64_t timeout = 0;

	cu_print("##################################################\n");
	cu_print("# Purpose of ADC service test :                  #\n");
	cu_print("#            Retrieve value sampled on channel X #\n");
	cu_print("##################################################\n");

	SRV_WAIT(!cfw_service_registered(SS_ADC_SERVICE_ID), 0xFFFFFF);
	ADC_CU_ASSERT("Timeout expired, cfw service is not registered",
		      cfw_service_registered(SS_ADC_SERVICE_ID));

	cu_print("ADC Service registered, open it\n");
	adc_service_handle = cproxy_connect(SS_ADC_SERVICE_ID, adc_handle_msg,
					    NULL);
	ADC_CU_ASSERT("Timeout expired, unable to open ADC service",
		      adc_service_handle);

	/* Handle Error Scenario for adc get value with invalid channel number */
	adc_service_get_value(adc_service_handle, 100, (void *)&(req_cnt[0]));

	for (uint8_t i = 1; i < NO_OF_REQUEST; i++) {
		req_cnt[i] = i + 1;
		adc_service_get_value(adc_service_handle, TEST_CHANNEL,
				      (void *)&req_cnt[i]);
	}

	while ((++timeout) <= 0xFFFFFF) {
		queue_process_message(get_test_queue());
		if (resp_cnt == NO_OF_REQUEST)
			break;
	}
	ADC_CU_ASSERT("Timeout expired, not adc read response",
		      timeout <= 0xFFFFFF);
	ADC_CU_ASSERT("Response not received for all request\n",
		      resp_cnt == NO_OF_REQUEST);
}
