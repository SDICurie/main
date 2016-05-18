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

#ifndef __SENSOR_SVC_H__
#define __SENSOR_SVC_H__
#include <string.h>
#include "services/sensor_service/sensor_data_format.h"
#include "services/sensor_service/sensor_service.h"
#include "sensor_svc_platform.h"
#include "sensor_svc_calibration.h"
#include "sensors/sensor_core/ipc/ipc_comm.h"

typedef sensor_service_message_general_rsp_t ss_start_sensor_scanning_rsp_t;
typedef sensor_service_message_general_rsp_t ss_stop_scanning_rsp_t;
typedef sensor_service_message_general_rsp_t ss_sensor_subscribe_data_rsp_t;
typedef sensor_service_message_general_rsp_t ss_sensor_unsubscribe_data_rsp_t;

DEFINE_LOG_MODULE(LOG_MODULE_SS_SVC, "SS_S")

/*
 * sensor service send get_calibration event to app
 */
typedef struct {
	struct cfw_message head;
	sensor_service_t handle;
	uint8_t calibration_type;
	uint8_t data_length;
	uint8_t value[0];
} ss_sensor_get_cal_data_evt_t;

/**
 * Parameters of MSG_ID_SENSOR_SERVICE_GET_PROPERTY_EVT
 */
typedef struct {
	struct cfw_message head;
	sensor_service_t handle;
	uint8_t data_length;
	uint8_t value[0];
} ss_sensor_get_property_evt_t;

#define IS_ON_BOARD_SENSOR_TYPE(type) ((((type) > \
					 ON_BOARD_SENSOR_TYPE_START) &&	   \
					((type) < \
					 ON_BOARD_SENSOR_TYPE_END)) ? 1 : 0)
#define IS_BLE_SENSOR_TYPE(type)    ((((type) > BLE_SENSOR_TYPE_START) &&   \
				      ((type) < BLE_SENSOR_TYPE_END)) ? 1 : 0)
#define IS_ANT_SENSOR_TYPE(type)    ((((type) > ANT_SENSOR_TYPE_START) &&   \
				      ((type) < ANT_SENSOR_TYPE_END)) ? 1 : 0)

/**
 * Sensor Service Message definitions
 */
#define MSG_ID_SS_BASE                              (MSG_ID_SENSOR_SERVICE_BASE	\
						     + 0x00)

/**
 * Request Message Ids
 */
#define MSG_ID_SS_START_SENSOR_SCANNING_REQ         (MSG_ID_SS_BASE | 0x01)
#define MSG_ID_SS_STOP_SENSOR_SCANNING_REQ          (MSG_ID_SS_BASE | 0x02)

#define MSG_ID_SS_SENSOR_START_REQ                  (MSG_ID_SS_BASE | 0x03)
#define MSG_ID_SS_SENSOR_STOP_REQ                   (MSG_ID_SS_BASE | 0x04)

#define MSG_ID_SS_SENSOR_CALIBRATION_REQ            (MSG_ID_SS_BASE | 0x05)

#define MSG_ID_SS_SENSOR_SUBSCRIBE_DATA_REQ         (MSG_ID_SS_BASE | 0x06)
#define MSG_ID_SS_SENSOR_UNSUBSCRIBE_DATA_REQ       (MSG_ID_SS_BASE | 0x07)

#define MSG_ID_SS_SENSOR_SET_PROPERTY_REQ            (MSG_ID_SS_BASE | 0x08)
#define MSG_ID_SS_SENSOR_GET_PROPERTY_REQ            (MSG_ID_SS_BASE | 0x09)


#define SENSOR_DEVICE_ID_REQ_MASK           (1 << SENSOR_DEVICE_ID)
#define SENSOR_PRODUCT_ID_REQ_MASK          (1 << SENSOR_PRODUCT_ID)
#define SENSOR_MANUFACTURE_ID_REQ_MASK      (1 << SENSOR_MANUFACTURER_ID)
#define SENSOR_SERIAL_NUMBER_REQ_MASK       (1 << SENSOR_SERIAL_NUMBER)
#define SENSOR_SW_VERSION_REQ_MASK          (1 << SENSOR_SW_VERSION)
#define SENSOR_HW_VERSION_REQ_MASK          (1 << SENSOR_HW_VERSION)
#define SENSOR_CALIBRATION_FLAG_MASK        (1 << SENSOR_CALIBRATION_FLAG)
#define SENSOR_SET_PROPERTY_FLAG_MASK        (1 << SENSOR_SET_PROPERTY_FLAG)
#define SENSOR_GET_PROPERTY_FLAG_MASK        (1 << SENSOR_GET_PROPERTY_FLAG)

typedef enum {
	SENSOR_DEVICE_ID,
	SENSOR_PRODUCT_ID,
	SENSOR_MANUFACTURER_ID,
	SENSOR_SERIAL_NUMBER,
	SENSOR_SW_VERSION,
	SENSOR_HW_VERSION,
	SENSOR_CALIBRATION_FLAG,
	SENSOR_SET_PROPERTY_FLAG,
	SENSOR_GET_PROPERTY_FLAG
} sensor_info_type_t;

/* Return status of sensor funcs*/
typedef enum {
	NO_ERROR,
	NO_CLIENT_SUPPORT, /*Client does't support the sensor*/
	NO_CLIENT_INFO, /*Can't find the client info*/
	NO_SENSOR_INFO /*Can't find the sensor info*/
} err_info_t;

/* Sensor service status */
typedef enum {
	SS_STATUS_SUCCESS = 0,
	SS_STATUS_CMD_REPEAT,
	SS_STATUS_ERROR        /* Generic Error */
} ss_service_status_t;

#define NEXT_STATUS_SCANNING 1
#define NEXT_STATUS_PAIRING  2

typedef struct {
	uint8_t next_status;
	void *p_param;
} open_service_param_t;

int ss_send_rsp_msg_to_client(void *p_handle, uint16_t msg_id,
			      sensor_service_ret_type status,
			      void *p_sensor_handle,
			      void *priv_from_client);

void ss_send_scan_rsp_msg_to_clients(uint32_t sensor_type_bit_map,
				     sensor_service_ret_type status,
				     void *conn_client, void *
				     priv_data_from_client);

void ss_send_scan_data_to_clients(uint8_t sensor_type, void *p_data,
				  void *conn_client,
				  void *priv_data_from_client);

void ss_send_stop_scan_rsp_msg_to_clients(uint32_t sensor_type_bit_map,
					  sensor_service_ret_type status,
					  void *
					  conn_client, void *
					  priv_data_from_client);

void ss_send_pair_rsp_msg_to_clients(sensor_service_t sensor_handle,
				     sensor_service_ret_type status,
				     void *conn_client, void *
				     priv_data_from_client);

void ss_send_subscribing_rsp_msg_to_clients(sensor_service_t sensor_handle,
					    sensor_service_ret_type status,
					    void *
					    conn_client, void *
					    priv_data_from_client);


void ss_send_subscribing_evt_msg_to_clients(sensor_service_t sensor_handle,
					    uint8_t data_type,
					    uint32_t timestamp, void *p_data,
					    uint16_t len);

void ss_send_unsubscribing_rsp_msg_to_clients(sensor_service_t sensor_handle,
					      sensor_service_ret_type status,
					      void *
					      conn_client, void *
					      priv_data_from_client);

void ss_send_cal_rsp_and_data_to_clients(sensor_service_t sensor_handle,
					 uint8_t calibration_type,
					 uint8_t length, uint8_t *cal_data,
					 sensor_service_ret_type status,
					 void *conn_client,
					 void *priv_data_from_client);

void ss_send_set_property_rsp_to_clients(sensor_service_t,
					 sensor_service_ret_type,
					 void *,
					 void *);

void ss_send_get_property_data_to_clients(sensor_service_t, uint8_t, uint8_t *,
					  sensor_service_ret_type,
					  void *,
					  void *);

void ss_send_unpair_rsp_msg_to_clients(sensor_service_t sensor_handle,
				       sensor_service_ret_type status,
				       void *conn_client, void *
				       priv_data_from_client);

void ss_send_unlink_rsp_msg_to_clients(sensor_service_t sensor_handle,
				       sensor_service_ret_type status,
				       void *conn_client, void *
				       priv_data_from_client);

#ifdef CONFIG_SENSOR_SERVICE_RESERVED_FUNC
void ss_send_device_id_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);

void ss_send_product_id_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);

void ss_send_manufacturer_id_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);

void ss_send_serial_number_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);

void ss_send_hw_version_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);

void ss_send_sw_version_rsp_msg_to_clients(
	sensor_service_t	sensor_handle,
	uint8_t *		str,
	sensor_service_ret_type status);
#endif

typedef struct {
	struct cfw_message head;
	uint16_t msg_id;
	uint8_t sensor_type;
	uint8_t sensor_id;
	uint8_t len;
	uint8_t data[0];
} sc_evt_t;


typedef sc_evt_t sc_scan_evt_t;

typedef sc_evt_t sc_subscribe_evt_t;

int svc_send_scan_cmd_to_core(uint32_t			sensor_type_bit_map,
			      struct cfw_message *	p_req);

int svc_send_start_cmd_to_core(uint8_t sensor_type, uint8_t sensor_id,
			       struct cfw_message *p_req);

int svc_send_subscribe_data_cmd_to_core(uint8_t sensor_type, uint8_t sensor_id,
					uint16_t sample_freq,
					uint16_t buf_delay,
					struct cfw_message *p_req);

int svc_send_unsubscribe_data_cmd_to_core(uint8_t sensor_type,
					  uint8_t sensor_id, uint8_t data_type,
					  struct cfw_message *p_req);

int svc_send_calibration_cmd_to_core(ss_sensor_calibration_req_t *req);

int svc_send_stop_cmd_to_core(uint8_t sensor_type, uint8_t sensor_id,
			      struct cfw_message *p_req);

void ss_sc_resp_msg_handler(sc_rsp_t *p_msg);

void ss_sc_evt_msg_handler(sc_evt_t *p_msg);

int send_request_cmd_to_core(uint8_t tran_id, uint8_t sensor_id,
			     uint32_t param1, uint32_t param2, uint8_t *addr,
			     uint8_t cmd_id,
			     struct cfw_message *p_req);

/**
 * take sensor_service as a client of sensor_core svc
 *
 * sensor service open the sensor core service, so that this
 * sensor_svc will be a client of sensor_core_svc.
 */
bool sensor_svc_open_sensor_core_svc(void *p_queue, void (*cb)(
					     void *), void *cb_param);
#endif
