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

#ifndef __SENSOR_SVC_API_H__
#define __SENSOR_SVC_API_H__

#include "cfw/cfw.h"

#include "services/services_ids.h"
#include "services/sensor_service/sensor_data_format.h"

/**
 * @defgroup sensor_service Sensor Service
 *
 * Service to register and get data from sensors.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/sensor_service/sensor_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/sensor_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>SERVICES_SENSOR, SERVICES_SENSOR_IMPL, and more in Kconfig</tt>
 * <tr><th><b>Service Id</b>  <td><tt>ARC_SC_SVC_ID</tt>
 * </table>
 *
 * Once connected to the sensor service:
 * - \ref sensor_service_start_scanning to scan for sensor availability.\n
 *   Client will receive _MSG_ID_SS_START_SENSOR_SCANNING_EVT_ messages
 *   with attached \ref sensor_service_scan_event_t for each detected sensor.
 * - \ref sensor_service_subscribe_data to subscribe to a sensor\n
 *   Client will receive _MSG_ID_SS_SENSOR_SUBSCRIBE_DATA_EVT_ messages
 *   with attached \ref sensor_service_subscribe_data_event_t.\n
 *   Data depends on the sensor type (see sensor_data_format.h for details).
 *
 * @ingroup services
 * @{
 */

/**
 * Sensor Service Message definitions
 */
#define MSG_ID_SENSOR_SERVICE_RSP                               ( \
		MSG_ID_SENSOR_SERVICE_BASE + 0x40)
#define MSG_ID_SENSOR_SERVICE_EVT                               ( \
		MSG_ID_SENSOR_SERVICE_BASE + 0x80)

/**
 * Response Message Ids
 */
#define MSG_ID_SENSOR_SERVICE_START_SCANNING_RSP         ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x01)
#define MSG_ID_SENSOR_SERVICE_GET_SENSOR_LIST_RSP        ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x02)
#define MSG_ID_SENSOR_SERVICE_STOP_SCANNING_RSP          ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x03)
#define MSG_ID_SENSOR_SERVICE_PAIR_RSP                   ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x05)
#define MSG_ID_SENSOR_SERVICE_UNPAIR_RSP                 ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x06)
#define MSG_ID_SENSOR_SERVICE_SUBSCRIBE_DATA_RSP         ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x07)
#define MSG_ID_SENSOR_SERVICE_UNSUBSCRIBE_DATA_RSP       ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x08)
#define MSG_ID_SENSOR_SERVICE_OPT_CALIBRATION_RSP        ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x09)

#define MSG_ID_SENSOR_SERVICE_SET_PROPERTY_RSP           ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x0a)
#define MSG_ID_SENSOR_SERVICE_GET_PROPERTY_RSP           ( \
		MSG_ID_SENSOR_SERVICE_RSP | 0x0b)

/**
 * Asynchronous Message Ids
 */
#define MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT         ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x01)
#define MSG_ID_SENSOR_SERVICE_GET_SENSOR_LIST_EVT        ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x02)
#define MSG_ID_SENSOR_SERVICE_STOP_SCANNING_EVT          ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x03)
#define MSG_ID_SENSOR_SERVICE_SUBSCRIBE_DATA_EVT         ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x04)
#define MSG_ID_SENSOR_SERVICE_UNSUBSCRIBE_DATA_EVT       ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x05)
#define MSG_ID_SENSOR_SERVICE_OPT_CALIBRATION_EVT        ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x06)
#define MSG_ID_SENSOR_SERVICE_GET_PROPERTY_EVT           ( \
		MSG_ID_SENSOR_SERVICE_EVT | 0x07)


#define GET_SENSOR_TYPE(sensor_handle)  ((((uint32_t)(sensor_handle)) >> \
					  24) & 0xFF)
#define GET_SENSOR_ID(sensor_handle)    (((uint32_t)(sensor_handle)) & 0xFFFFFF)
#define GET_SENSOR_HANDLE(type,	\
			  id)     (void *)((((type) & \
					     0xFF) << 24) | ((id) & 0xFFFFFF))

/**
 * The type for supported sensor device
 */
typedef void *sensor_service_t;  /*!< Composed of (sensor_type << 24) | (sensor_id & 0xFFFFFF) */

/**
 * On board sensor scan data
 */
typedef struct {
	uint32_t ch_id; /*!< sensor handle = GET_SENSOR_HANDLE(sensor_type, ch_id) */
} sensor_service_on_board_scan_data_t;

/** Common status to indicates the sensor_core status , when we try to request a cmd*/
typedef enum {
	RESP_SUCCESS = 0,
	RESP_NO_DEVICE,
	RESP_DEVICE_EXCEPTION,
	RESP_WRONG_CMD,
	RESP_INVALID_PARAM,
} sensor_service_ret_type;

/**
 * Sensor service response message
 */
typedef struct {
	struct cfw_message header;
	sensor_service_t handle;
	sensor_service_ret_type status;
} sensor_service_message_general_rsp_t;

/**
 * Sensor service scan event\n
 * Sensor handle is given by _GET_SENSOR_HANDLE(evt->sensor_type, evt->on_board_data.ch_id);_
 */
typedef struct {
	struct cfw_message head; /*!< Message header */
	uint8_t sensor_type;  /*!< Sensor type */
	sensor_service_t handle; /*!< Reserved */
	uint16_t len;         /*!< Data length */
	union {
		uint8_t data[0];
		sensor_service_on_board_scan_data_t on_board_data;
	};
} sensor_service_scan_event_t;

/**
 * Sensor service report subscribe data
 */
typedef struct {
	struct cfw_message head;
	sensor_service_t handle;
	sensor_service_sensor_data_header_t sensor_data_header;
} sensor_service_subscribe_data_event_t;

/**
 * Start the sensor scanning.
 *
 * @param  p_service_conn      Service connection
 * @param  p_priv              Pointer to private data that will be passed back in response
 * @param  sensor_type_bitmap  Specify the bitmap of sensor types.
 *                             Bitmap refer to \ref ss_sensor_type_t.
 *                             eg:SENSOR_ACCELEROMETER|SENSOR_GYROSCOPE,...
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_START_SCANNING_RSP_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_start_scanning(cfw_service_conn_t * p_service_conn,
				   void *		p_priv,
				   uint32_t		sensor_type_bitmap);

/**
 * Stop the sensor scanning.
 *
 * @param  p_service_conn      Service connection
 * @param  p_priv              Pointer to private data that will be passed back in response
 * @param  sensor_type_bitmap  Specify the bitmap of sensor types.
 *                             Bitmap refer to \ref ss_sensor_type_t.
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_STOP_SCANNING_EVT_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_stop_scanning(cfw_service_conn_t *	p_service_conn,
				  void *		p_priv,
				  uint32_t		sensor_type_bitmap);

/**
 * Subscribe to sensor data
 *
 * @param  p_service_conn      Service connection
 * @param  p_priv              Pointer to private data that will be passed back in response
 * @param  sensor              Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 * @param  data_type           Specific to sensor type.
 * @param  data_type_nr        Size of data type
 * @param  sampling_interval   Fequency of sensor data sampling,unit[HZ].
 * @param  reporting_interval  Frequency of sensor data reporting,unit[ms].
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_SUBSCRIBE_DATA_RSP_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_subscribe_data(cfw_service_conn_t *p_service_conn,
				   void *p_priv, sensor_service_t sensor,
				   uint8_t *data_type, uint8_t data_type_nr,
				   uint16_t sampling_interval,
				   uint16_t reporting_interval);

/**
 * Unsubscribe from sensor data
 *
 * @param  p_service_conn   Service connection
 * @param  p_priv           Pointer to private data that will be passed back in response
 * @param  sensor           Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 * @param  data_type        Specific to sensor type.
 * @param  data_type_nr     Size of data type
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_UNSUBSCRIBE_DATA_RSP_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_unsubscribe_data(cfw_service_conn_t *p_service_conn,
				     void *p_priv, sensor_service_t sensor,
				     uint8_t *data_type,
				     uint8_t data_type_nr);

/**
 * Set the sensor property.
 *
 * @param  p_service_conn   Service connection
 * @param  p_priv           Pointer to private data that will be passed back in response
 * @param  sensor           Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 * @param  len              Property parameter length.
 * @param  value            Property parameter.
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_SET_PROPERTY_RSP_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_set_property(cfw_service_conn_t *p_service_conn,
				 void *p_priv, sensor_service_t sensor,
				 uint8_t len,
				 uint8_t *value);
/**
 * Get the sensor property.
 *
 * @param  p_service_conn   Service connection
 * @param  p_priv           Pointer to private data that will be passed back in response
 * @param  sensor           Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_GET_PROPERTY_RSP_ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_get_property(cfw_service_conn_t *	p_service_conn,
				 void *			p_priv,
				 sensor_service_t	sensor);

/**
 * Set the calibration .
 *
 * @param  p_service_conn   Service connection
 * @param  p_priv           Pointer to private data that will be passed back in response
 * @param  sensor           Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 * @param  calibration_type Specific to sensor type.
 * @param  len              Calibration parameter length. (Only valid for SET_CALIBRATION_CMD).
 * @param  value            Calibration parameter.(Only valid for SET_CALIBRATION_CMD).
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_OPT_CALIBRATION_RSP_\ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_set_calibration(cfw_service_conn_t *p_service_conn,
				    void *p_priv, sensor_service_t sensor,
				    uint8_t calibration_type, uint8_t len,
				    uint8_t *value);

/**
 * Operation on the calibration parameter.
 *
 * @param  p_service_conn   Service connection
 * @param  p_priv           Pointer to private data that will be passed back in response
 * @param  sensor           Sensor handle as received on \ref MSG_ID_SENSOR_SERVICE_START_SCANNING_EVT
 * @param  clb_cmd          Calibration command (START_CALIBRATION_CMD/STOP_CALIBRATION_CMD/SET_CALIBRATION_CMD)
 * @param  calibration_type Specific to sensor type.
 *
 * @b Response: _MSG_ID_SENSOR_SERVICE_OPT_CALIBRATION_RSP\ with attached \ref sensor_service_message_general_rsp_t
 */
void sensor_service_opt_calibration(cfw_service_conn_t *p_service_conn,
				    void *p_priv, sensor_service_t sensor,
				    uint8_t clb_cmd,
				    uint8_t calibration_type);

/** @} */
#endif   /*__SENSOR_SVC_API_H__*/
