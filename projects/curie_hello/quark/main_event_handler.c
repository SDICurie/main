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

#include "cfw/cfw.h"
#include "infra/log.h"
#include "infra/xloop.h"
#include "infra/panic.h"
#include "services/gpio_service/gpio_service.h"
#include "services/adc_service/adc_service.h"
#include "main_event_handler.h"

#define NOTIF_DELAY_MS                        5000
#define LED_GPIO_PIN                          8
#define TEST_CHANNEL                          10 /* To sense AIN_10 pin */
#define ADC_READ_PERIOD                       200 /* ADC read every 200 ms */
#define LOWER_LIMIT                           700
#define HIGHER_LIMIT                          3800
#define LED_BLINK_ONE_SEC                     1000
#define LED_BLINK_500_MS                      500
#define LED_BLINK_TWO_SEC                     2000

/* adc_svc_client may be used to unsubscribe request at a latter stage */
static void *adc_svc_client;
static uint16_t blink_freq = LED_BLINK_ONE_SEC;
static cfw_client_t *cfw_handle;
static cfw_service_conn_t *gpio_service_conn = NULL;    /* GPIO service handler */
static cfw_service_conn_t *adc_service_conn = NULL;     /* ADC service handler */
static bool led_blink_done = false;
static xloop_t *loop;
static void led_blink_func(xloop_job_t *job);

static xloop_job_t led_job = {
	.run = led_blink_func,
};

static void led_blink_func(xloop_job_t *job)
{
	if (!led_blink_done) {
		gpio_service_set_state(gpio_service_conn, LED_GPIO_PIN, 1, NULL);
		led_blink_done = true;
	} else {
		gpio_service_set_state(gpio_service_conn, LED_GPIO_PIN, 0, NULL);
		led_blink_done = false;
	}
	xloop_post_job_delayed(job->loop, &led_job, blink_freq);
}

static void service_opened_cb(cfw_service_conn_t *handle, void *param)
{
	int service_id = (int)param;

	switch (service_id) {
	case SOC_GPIO_SERVICE_ID:
		gpio_service_conn = handle;
		/* Configure GPIO */
		gpio_service_configure(gpio_service_conn, LED_GPIO_PIN, 1, NULL);
		xloop_post_job_delayed(loop, &led_job, blink_freq);
		break;
	case SS_ADC_SERVICE_ID:
		pr_debug(LOG_MODULE_MAIN, "ADC service open");
		adc_service_conn = handle;
		/* Subscribe request to ADC service without using external GPIO
		 * After subscribing, an ADC read will be performed every ADC_READ_PERIOD */
		adc_service_subscribe(adc_service_conn, TEST_CHANNEL,
				      0, 0, 0, ADC_READ_PERIOD, NULL);
		break;
	default:
		break;
	}
}

static void cfw_message_handler(struct cfw_message *msg, void *param)
{
	DRIVER_API_RC ret = DRV_RC_FAIL;
	static uint16_t adc_value;

	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_GPIO_SERVICE_CONFIGURE_RSP:
	case MSG_ID_GPIO_SERVICE_SET_STATE_RSP:
		break;
	case MSG_ID_ADC_SERVICE_SUBSCRIBE_RSP:
		pr_debug(LOG_MODULE_MAIN, "ADC subscribe Response");
		adc_svc_client =
			((adc_service_subscribe_rsp_msg_t *)msg)->
			adc_sub_conn_handle;
		ret = ((adc_service_subscribe_rsp_msg_t *)msg)->status;
		if (DRV_RC_OK != ret) {
			pr_error(LOG_MODULE_MAIN, "ADC subscribe fails %d", ret);
			panic(-1);
		}
		break;
	case MSG_ID_ADC_SERVICE_GET_VAL_EVT:
		ret = ((adc_service_get_evt_msg_t *)msg)->status;
		if (DRV_RC_OK == ret) {
			adc_value =
				((adc_service_get_evt_msg_t *)msg)->adc_value;
			pr_debug(LOG_MODULE_MAIN, "adc_value %d", adc_value);
			if (adc_value < LOWER_LIMIT)
				blink_freq = LED_BLINK_ONE_SEC;
			else if (adc_value > HIGHER_LIMIT)
				blink_freq = LED_BLINK_500_MS;
			else
				blink_freq = LED_BLINK_TWO_SEC;
		} else {
			pr_error(LOG_MODULE_MAIN, "ADC get value fails %d",
				 ret);
		}
		break;
	default:
		pr_info(LOG_MODULE_MAIN, "Discarded message: %x",
			CFW_MESSAGE_ID(msg));
	}
	cfw_msg_free(msg);
}

static int hello_func(void *param)
{
	pr_info(LOG_MODULE_MAIN, "Hello QUARK");
	return 0;
}

int main_event_handler_init(xloop_t *l)
{
	loop = l;

	cfw_handle = cfw_client_init(loop->queue, cfw_message_handler, NULL);

	cfw_open_service_helper(cfw_handle, SOC_GPIO_SERVICE_ID,
				service_opened_cb, (void *)SOC_GPIO_SERVICE_ID);

	cfw_open_service_helper(cfw_handle, SS_ADC_SERVICE_ID,
				service_opened_cb, (void *)SS_ADC_SERVICE_ID);

	xloop_post_func_periodic(loop, hello_func, NULL, NOTIF_DELAY_MS);
	pr_debug(LOG_MODULE_MAIN, "main event handler initialized");
	return 0;
}
