/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
/* Utility API */
#include "sensor_svc.h"
#include "sensor_svc_utils.h"
#include "sensor_svc_calibration.h"
/* Properties Service will allow to read/store sensor data to flash */
#include "services/properties_service/properties_service.h"


/*Flag for doing erase first*/
#define ERASE_FLASH_FIRST 0

/*
 * nonpersist_service_id = 1 ,false, false
 * */
#define PERSIST_SERVICE_ID 2
struct props_data_common {
	uint8_t sensor_type;
	uint8_t dev_id;
	int data[3];
};

static struct props_data_common props_data[2];

/*Gloable struct to store everything*/
struct props_clb_dev_s {
	uint8_t persist_service_id;
	uint8_t prop_id;
};

const struct props_clb_dev_s props_clb_dev[] = {
	{ PERSIST_SERVICE_ID, SENSOR_ACCELEROMETER },
	{ PERSIST_SERVICE_ID, SENSOR_GYROSCOPE },
};

static cfw_service_conn_t *props_service_conn = NULL;
static cfw_client_t *props_client;

static uint8_t set_clb_to_sensor_core(uint8_t prop_index, uint8_t clb_type)
{
	uint8_t sensor_type = props_data[prop_index].sensor_type;
	uint8_t dev_id = props_data[prop_index].dev_id;
	int frame_size = 0;

	switch (props_clb_dev[prop_index].prop_id) {
	case SENSOR_ACCELEROMETER:
		frame_size = 3 * sizeof(int);
		break;
	case SENSOR_GYROSCOPE:
		frame_size = 3 * sizeof(short);
		break;
	}

	/* Allocate sensor setting calibration request message */
	ss_sensor_calibration_req_t *p_msg =
		(ss_sensor_calibration_req_t *)message_alloc(
			sizeof(*p_msg) + frame_size, NULL);
	if (p_msg == NULL)
		return -1;

	p_msg->sensor = GET_SENSOR_HANDLE(sensor_type, dev_id);
	p_msg->calibration_type = clb_type;
	p_msg->clb_cmd = REBOOT_AUTO_CALIBRATION_CMD;
	p_msg->data_length = frame_size;
	/* Fill cal param*/
	memcpy(p_msg->value, props_data[prop_index].data, frame_size);
	svc_send_calibration_cmd_to_core(p_msg);
	bfree(p_msg);
	return 0;
}

static int find_clb_dev(uint8_t sensor_type)
{
	for (int i = 0;
	     i < sizeof(props_clb_dev) / sizeof(struct props_clb_dev_s);
	     i++) {
		if (props_clb_dev[i].prop_id == sensor_type)
			return i;
	}
	return -1;
}

void sensor_clb_write_flash(uint8_t sensor_type, uint8_t dev_id, void *data,
			    uint32_t len)
{
	int i = find_clb_dev(sensor_type);

	if (!props_service_conn || i < 0) {
		return;
	}

	memcpy(props_data[i].data, data, len);
	props_data[i].sensor_type = sensor_type;
	props_data[i].dev_id = dev_id;
	properties_service_write(props_service_conn,
				 props_clb_dev[i].persist_service_id,
				 props_clb_dev[i].prop_id,
				 (void *)&props_data[i], sizeof(props_data[i]),
				 NULL);
}

static void props_handle_msg(struct cfw_message *msg, void *data)
{
	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_PROP_SERVICE_ADD_RSP:
	{
		int add_ret = ((properties_service_add_rsp_msg_t *)msg)->status;
		if (add_ret == DRV_RC_OK) {
			int priv_msg = (int)msg->priv;
			for (int i = 0;
			     i < sizeof(props_clb_dev) /
			     sizeof(struct props_clb_dev_s);
			     i++) {
				if (priv_msg == props_clb_dev[i].prop_id) {
					properties_service_read(
						props_service_conn,
						props_clb_dev[i].
						persist_service_id,
						props_clb_dev[i
						].prop_id, msg->priv);
					break;
				}
			}
		} else
			pr_debug(LOG_MODULE_SS_SVC,
				 "Fail to add properties in svc_property init");
	}
	break;
	case MSG_ID_PROP_SERVICE_READ_RSP:
	{
		int read_ret =
			((properties_service_read_rsp_msg_t *)msg)->status;
		if (read_ret == DRV_RC_OK) {
			int priv_msg = (int)msg->priv;
			for (int i = 0;
			     i < sizeof(props_clb_dev) /
			     sizeof(struct props_clb_dev_s);
			     i++) {
				if (priv_msg == props_clb_dev[i].prop_id) {
					int props_size =
						((
							 properties_service_read_rsp_msg_t
							 *)msg)->property_size;
					struct props_data_common *props =
						(struct props_data_common *)((
										     properties_service_read_rsp_msg_t
										     *)
									     msg)
						->start_of_values;

					if (props_size ==
					    sizeof(props_data[i])) {
						memcpy(&props_data[i], props,
						       props_size);
						set_clb_to_sensor_core(i, 0);
					}
					break;
				}
			}
		} else
			pr_debug(LOG_MODULE_SS_SVC,
				 "Fail to read properties in svc_property init");
	}
	break;
	case MSG_ID_PROP_SERVICE_WRITE_RSP:
	{
		int wr_ret =
			((properties_service_write_rsp_msg_t *)msg)->status;
		if (wr_ret != DRV_RC_OK)
			pr_debug(LOG_MODULE_SS_SVC, "Fail to write properties");
	}
	break;
	case MSG_ID_PROP_SERVICE_REMOVE_RSP:
	{
		int rm_ret =
			((properties_service_remove_rsp_msg_t *)msg)->status;
		if (rm_ret != DRV_RC_OK)
			pr_debug(LOG_MODULE_SS_SVC, "Fail to remove properties");
	}
	break;
	default:
		break;
	}
	cfw_msg_free(msg);
}


/*props service call back*/
static void props_handle_connect_cb(cfw_service_conn_t *conn, void *param)
{
	if (conn == NULL)
		return;

	props_service_conn = conn;
#if defined(ERASE_FLASH_FIRST) && (ERASE_FLASH_FIRST == 1)
	for (int i = 0;
	     i < sizeof(props_clb_dev) / sizeof(struct props_clb_dev_s);
	     i++)
		sensor_clb_clean_flash(props_clb_dev[i].sensor_type,
				       props_clb_dev[i].dev_id);
#else
	for (int i = 0;
	     i < sizeof(props_clb_dev) / sizeof(struct props_clb_dev_s);
	     i++) {
		int priv = props_clb_dev[i].prop_id;
		properties_service_add(props_service_conn,
				       props_clb_dev[i].persist_service_id,
				       props_clb_dev[i].prop_id, true,
				       (uint8_t *)&props_data[i],
				       sizeof(props_data[i]),
				       (void *)priv);
	}
#endif
}

void sensor_svc_clb_init(T_QUEUE queue)
{
	if (!props_service_conn) {
		props_client = cfw_client_init(queue, props_handle_msg, NULL);
		/* Open properties service when available */
		cfw_open_service_helper(props_client, PROPERTIES_SERVICE_ID,
					props_handle_connect_cb, NULL);
	}
}
