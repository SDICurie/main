/****************************************************************************************
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************************/
#ifndef __OPENCORE_MAIN_H__
#define __OPENCORE_MAIN_H__

#include <nanokernel.h>
#include "opencore_common.h"
#include "opencore_method.h"
#include "ipc_ia.h"
#include "infra/port.h"
#include "infra/message.h"

#define STACKSIZE 768
#define COMMIT_DATA_MAX_LENGTH 60
#define VALID_FRAME_LENGTH      6
#define RAW_FRAME_LENGTH        7
#define FIFO_NOT_CLEAR 1

typedef enum {
	ALGO_OPEN = 0,
	ALGO_CLOSE,
	ALGO_SUBSCRIBE,
	ALGO_UNSUBSCRIBE,
	ALGO_GET_PROPERTY,
	ALGO_SET_PROPERTY,
	ALGO_CALIBRATION_START,
	ALGO_CALIBRATION_PROCESS,
	ALGO_CALIBRATION_STOP,
	ALGO_CALIBRATION_SET,
	ALGO_CTL_TOP,
}core_sensor_ctl_t;

typedef struct {
	struct ia_cmd *cmd;
	list_t cmd_link;
}cmd_node_t;

extern struct pm_wakelock opencore_main_wl;
extern struct pm_wakelock opencore_cali_wl;

extern list_head_t feed_list;
extern list_head_t exposed_sensor_list;
extern list_head_t phy_sensor_list_int;
extern list_head_t phy_sensor_list_poll;
extern list_head_t phy_sensor_poll_active_list;
extern list_head_t phy_sensor_poll_active_array[PHY_TYPE_KEY_LENGTH *
						PHY_ID_KEY_LENGTH];

extern uint8_t raw_data_calibration_flag;
extern uint8_t calibration_process_error;
extern uint8_t global_suspend_flag;

void TriggerAlgoEngine(uint16_t msg_id, void *priv_data);
void AlgoEngineInit(T_QUEUE service_mgr_queue);
int motion_detect_callback(struct sensor_data *sensor_data, void *priv_data);
void RefleshSensorCore(void);
void SensorCoreInit(void);
void OpenIntSensor(sensor_handle_t *phy_sensor);
void CloseIntSensor(sensor_handle_t *phy_sensor);
int CheckMinDelayBuffer(feed_general_t *feed);
#endif
