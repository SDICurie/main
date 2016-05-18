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

#include "cfw/cfw_service.h"

#include "machine.h"
#include "drivers/ss_adc.h"
#include "drivers/gpio.h"
#include "drivers/data_type.h"
#include "infra/device.h"
#include "infra/log.h"
#include "infra/time.h"
#include "services/adc_service/adc_service.h"
#include "adc_service_private.h"

/****************************************************************************************
************************** SERVICE INITIALIZATION **************************************
****************************************************************************************/

/**
 * \brief adc service client structure
 */
typedef struct adc_service_request {
	adc_service_cli_req_t adc_svc_cli;    /*!< adc subcribe client object */
	bool time1_state;                     /*!< State of the timer expiry (timer1 or timer2) */
	T_TIMER adc_timer;                    /*!< Timer to set gpio and to read adc value*/
} adc_service_request_t;

typedef struct adc_tracked_gpio_list_ {
	list_t list; /*! Linking stucture */
	uint8_t tr_gpio_pin;
	uint16_t tr_total_clients;
	uint16_t tr_gpio_counter;
}adc_tracked_gpio_list_t;

list_head_t adc_gpio_tracked_list;

static void adc_handle_message(struct cfw_message *msg, void *param);
static void adc_client_connected(conn_handle_t *instance);
static void adc_client_disconnected(conn_handle_t *instance);
static void adc_timer_handler(void *priv_data);
static DRIVER_API_RC adc_svc_toggle_gpio(adc_service_request_t *adc_svc_req);
static void *adc_service_subscribe_request(cfw_service_conn_t *c,
					   uint8_t channel, uint8_t gpio_pin,
					   bool active_high, uint32_t time1,
					   uint32_t time2,
					   void *priv);
static inline void adc_svc_ss_adc_read(adc_service_request_t *adc_svc_req);

static struct td_device *adc_dev;
static struct td_device *ss_dev;

static service_t adc_service = {
	.service_id = SS_ADC_SERVICE_ID,
	.client_connected = adc_client_connected,
	.client_disconnected = adc_client_disconnected,
};

static void adc_service_init(int id, void *queue)
{
	adc_dev = &pf_device_ss_adc;
	list_init(&adc_gpio_tracked_list);
	cfw_register_service(queue, &adc_service, adc_handle_message, NULL);
}

CFW_DECLARE_SERVICE(ADC, SS_ADC_SERVICE_ID, adc_service_init);

/*******************************************************************************
 *********************** SERVICE IMPLEMENTATION ********************************
 ******************************************************************************/
static void adc_client_connected(conn_handle_t *instance)
{
	pr_debug(LOG_MODULE_MAIN, "%s: \n", __func__);
}

static void adc_client_disconnected(conn_handle_t *instance)
{
	pr_debug(LOG_MODULE_MAIN, "%s: \n", __func__);
}

static bool adc_gpio_tracked_cb(list_t *element, void *gpio_pin)
{
	if (((adc_tracked_gpio_list_t *)element)->tr_gpio_pin ==
	    *(uint8_t *)gpio_pin) {
		return true;
	}
	return false;
}

static void handle_get_value(struct cfw_message *msg)
{
	DRIVER_API_RC status = DRV_RC_FAIL;
	adc_service_get_val_req_msg_t *req =
		(adc_service_get_val_req_msg_t *)msg;
	adc_service_get_val_rsp_msg_t *resp =
		(adc_service_get_val_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_ADC_SERVICE_GET_VAL_RSP,
			sizeof(*resp));

	if ((req->channel < ADC_MIN_CHANNEL) ||
	    (req->channel > ADC_MAX_CHANNEL)) {
		pr_debug(LOG_MODULE_MAIN, "Unknown channel number %d\n",
			 req->channel);
		resp->status = status;
		goto out;
	}

	uint16_t result_value;
	status = ss_adc_read(req->channel, &result_value);

	if (status != DRV_RC_OK) {
		pr_debug(LOG_MODULE_MAIN, "fail to read ADC");
		resp->status = status;
		goto out;
	}

	resp->value = result_value;
	resp->status = DRV_RC_OK;
	resp->reason = ADC_EVT_RX;

out:
	cfw_send_message(resp);
	cfw_msg_free(msg);
}

static void handle_subscribe(struct cfw_message *msg)
{
	adc_service_cli_req_t *req = (adc_service_cli_req_t *)msg;
	adc_service_subscribe_rsp_msg_t *resp =
		(adc_service_subscribe_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_ADC_SERVICE_SUBSCRIBE_RSP,
			sizeof(*resp));

	if ((req->adc_channel < ADC_MIN_CHANNEL) ||
	    (req->adc_channel > ADC_MAX_CHANNEL)) {
		pr_debug(LOG_MODULE_MAIN, "Unknown channel number %d\n",
			 req->adc_channel);
		resp->status = DRV_RC_FAIL;
		goto out;
	}
	resp->adc_sub_conn_handle = adc_service_subscribe_request(
		req->adc_service_conn, req->adc_channel,
		req->gpio_pin,
		req->active_high, req->time1, req->time2, NULL);
	resp->status = DRV_RC_OK;

out:
	cfw_send_message(resp);
	cfw_msg_free(msg);
}

static void handle_unsubscribe(struct cfw_message *msg)
{
	adc_service_request_t *adc_svc_cli_handle =
		(adc_service_request_t *)msg->priv;
	adc_service_subscribe_rsp_msg_t *resp =
		(adc_service_subscribe_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_ADC_SERVICE_UNSUBSCRIBE_RSP,
			sizeof(*resp));

	if (!adc_svc_cli_handle) {
		resp->status = DRV_RC_FAIL;
		goto out;
	}
	adc_tracked_gpio_list_t *gpio_list =
		(adc_tracked_gpio_list_t *)list_find_first(
			&adc_gpio_tracked_list,
			adc_gpio_tracked_cb,
			&adc_svc_cli_handle->adc_svc_cli.gpio_pin);
	if (gpio_list->tr_total_clients > 0) {
		gpio_list->tr_total_clients--;
	}
	if (!gpio_list->tr_total_clients) {
		/* Deconfigure gpio if no client is using it,
		 * else other subscribe may get wrong data */
		gpio_deconfig(ss_dev, adc_svc_cli_handle->adc_svc_cli.gpio_pin);
		list_remove(&adc_gpio_tracked_list, (list_t *)&gpio_list->list);
		bfree(gpio_list);
	}
	timer_delete(adc_svc_cli_handle->adc_timer);
	bfree(adc_svc_cli_handle);
	adc_svc_cli_handle = NULL;
	resp->status = DRV_RC_OK;
out:
	cfw_send_message(resp);
	cfw_msg_free(msg);
}

static void adc_handle_message(struct cfw_message *msg, void *param)
{
	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_ADC_GET_VAL_REQ:
		handle_get_value(msg);
		break;
	case MSG_ID_ADC_SUBSCRIBE_REQ:
		handle_subscribe(msg);
		break;
	case MSG_ID_ADC_UNSUBSCRIBE_REQ:
		handle_unsubscribe(msg);
		break;
	default:
		cfw_print_default_handle_error_msg(LOG_MODULE_MAIN,
						   CFW_MESSAGE_ID(
							   msg));
		break;
	}
}

static void *adc_service_subscribe_request(cfw_service_conn_t *c,
					   uint8_t channel,
					   uint8_t gpio_pin, bool active_high,
					   uint32_t time1, uint32_t time2,
					   void *priv)
{
	adc_service_request_t *adc_svc_req = (adc_service_request_t *)
					     cfw_alloc_message(sizeof(
								       adc_service_request_t));

	adc_svc_req->adc_svc_cli.adc_service_conn = c;
	adc_svc_req->adc_svc_cli.adc_channel = channel;
	adc_svc_req->adc_svc_cli.gpio_pin = gpio_pin;
	adc_svc_req->adc_svc_cli.active_high = active_high;
	adc_svc_req->adc_svc_cli.time1 = time1;
	adc_svc_req->adc_svc_cli.time2 = time2;
	adc_svc_req->adc_svc_cli.priv = priv;
	/*Configure the gpio if clients requires otherwise dont configure it */
	if (adc_svc_req->adc_svc_cli.time1) {
		adc_svc_req->time1_state = true;
		gpio_cfg_data_t in_pin_cfg =
		{ .gpio_type = GPIO_OUTPUT, .int_type = EDGE,
		  .int_polarity = ACTIVE_LOW,
		  .int_debounce = DEBOUNCE_OFF,
		  .int_ls_sync = LS_SYNC_OFF,
		  .gpio_cb = NULL,
		  .gpio_cb_arg = NULL, };
		if (adc_svc_req->adc_svc_cli.gpio_pin < SS_GPIO_8B0_BITS) {
			ss_dev = &pf_device_ss_gpio_8b0;
		} else {
			adc_svc_req->adc_svc_cli.gpio_pin -= SS_GPIO_8B0_BITS;
			ss_dev = &pf_device_ss_gpio_8b1;
		}
		/*Loop to track whether gpio is already subscribed or not */
		adc_tracked_gpio_list_t *gpio_list =
			(adc_tracked_gpio_list_t *)list_find_first(
				&adc_gpio_tracked_list,
				adc_gpio_tracked_cb,
				&adc_svc_req->adc_svc_cli.gpio_pin);
		if (gpio_list == NULL) {
			gpio_list =
				(adc_tracked_gpio_list_t *)balloc(sizeof(*
									 gpio_list),
								  NULL);
			gpio_list->tr_gpio_pin =
				adc_svc_req->adc_svc_cli.gpio_pin;
			list_add(&adc_gpio_tracked_list,
				 (list_t *)&gpio_list->list);
		}
		gpio_list->tr_total_clients++;

		if (gpio_list->tr_total_clients == 1) {
			/*Configure gpio only for the first subscribe.
			 * Subscribe for the second time may raise an error while toggle */
			gpio_set_config(ss_dev,
					adc_svc_req->adc_svc_cli.gpio_pin,
					&in_pin_cfg);
		}
		adc_svc_req->adc_timer = timer_create(
			adc_timer_handler, adc_svc_req,
			adc_svc_req->adc_svc_cli.
			time1, false, true, NULL);
	} else {
		adc_svc_req->time1_state = false;
		adc_svc_req->adc_timer = timer_create(
			adc_timer_handler, adc_svc_req,
			adc_svc_req->adc_svc_cli.
			time2, false, true, NULL);
	}
	return (void *)adc_svc_req;
}

static void adc_timer_handler(void *priv_data)
{
	adc_service_request_t *adc_svc_req = (adc_service_request_t *)priv_data;

	if (adc_svc_req->time1_state) {
		/* Toggle gpio.If fails to toggle the adc gpio then we need to restart the timer1 */
		if (DRV_RC_OK != adc_svc_toggle_gpio(adc_svc_req)) {
			timer_start(adc_svc_req->adc_timer,
				    adc_svc_req->adc_svc_cli.time1,
				    NULL);
			return;
		}
		adc_svc_req->time1_state = false;
		/*Start same timer with time2. */
		timer_start(adc_svc_req->adc_timer,
			    adc_svc_req->adc_svc_cli.time2,
			    NULL);
	} else {
		/* Read adc value. */
		adc_svc_ss_adc_read(adc_svc_req);
		/* If 'timer1 == 0', that means the client don't want to toggle any gpio,
		 * so we should directly start the timer on time2 and do not config any gpio.
		 * or
		 * If we have valid time1 and if it fails to toggle gpio then we need to restart timer2.
		 */
		if (adc_svc_req->adc_svc_cli.time1 == 0 ||
		    (DRV_RC_OK != adc_svc_toggle_gpio(adc_svc_req))) {
			timer_start(adc_svc_req->adc_timer,
				    adc_svc_req->adc_svc_cli.time2,
				    NULL);
			return;
		}
		/* Restart timer with time1. */
		timer_start(adc_svc_req->adc_timer,
			    adc_svc_req->adc_svc_cli.time1,
			    NULL);
		adc_svc_req->time1_state = true;
	}
}

static DRIVER_API_RC adc_svc_toggle_gpio(adc_service_request_t *adc_svc_req)
{
	DRIVER_API_RC ret = DRV_RC_FAIL;

	adc_tracked_gpio_list_t *gpio_list =
		(adc_tracked_gpio_list_t *)list_find_first(
			&adc_gpio_tracked_list,
			adc_gpio_tracked_cb,
			&adc_svc_req->adc_svc_cli.gpio_pin);

	if (!gpio_list)
		return ret;
	if (adc_svc_req->time1_state == true) {
		/*
		 * Dont set gpio if already SET by other subscribers.
		 * we need to allow same gpio for multiple subscriber
		 */
		if (!gpio_list->tr_gpio_counter) {
			ret =
				gpio_write(ss_dev,
					   adc_svc_req->adc_svc_cli.gpio_pin,
					   adc_svc_req->adc_svc_cli.active_high);
		}
		gpio_list->tr_gpio_counter++;
	} else {
		/*
		 * Unset gpio if no other subscriber is using.
		 * we need to allow same gpio for multiple subscriber
		 */
		gpio_list->tr_gpio_counter--;
		if (!gpio_list->tr_gpio_counter)
			ret = gpio_write(
				ss_dev, adc_svc_req->adc_svc_cli.gpio_pin,
				!(adc_svc_req->adc_svc_cli.active_high));
	}
	return ret;
}

static inline void adc_svc_ss_adc_read(adc_service_request_t *adc_svc_req)
{
	DRIVER_API_RC status = DRV_RC_FAIL;
	uint16_t read_data; /*!< value read by ADC */

	adc_service_get_evt_msg_t *msg =
		(adc_service_get_evt_msg_t *)cfw_alloc_rsp_message_for_client(
			adc_svc_req->adc_svc_cli.adc_service_conn,
			MSG_ID_ADC_SERVICE_GET_VAL_EVT, sizeof(*msg),
			adc_svc_req->adc_svc_cli.priv);

	CFW_MESSAGE_TYPE((struct cfw_message *)msg) = TYPE_EVT;

	status = ss_adc_read(adc_svc_req->adc_svc_cli.adc_channel, &read_data);
	if (status != DRV_RC_OK) {
		goto send;
	}

	/* Send adc value to client */
	msg->adc_value = read_data;
send:
	msg->status = status;
	msg->timestamp = get_uptime_ms();
	cfw_send_message(msg);
}
