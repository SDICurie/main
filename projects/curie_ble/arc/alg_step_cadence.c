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
#include "infra/time.h"
#include "opencore_algo_support.h"

#define MAG_THRESHOLD_LVL    500       /* Dummy threshold just to compute the step count */
#define FIVE_SEC_DURATION    5 * 1000  /* Cadence computation period (ms) */
#define FIVE_SEC_IN_ONE_MIN  12        /* Cadence unit is nb/mn */
#define MAX_CADENCE          512

/* Structure used to report event */
static struct stepcadence_result stepcadence_rpt_buf;

static uint16_t step_count = 0;
static uint32_t local_time = 0;

/* Algorithm to compute cadence */
static int stepcadence_algorithm_process(int16_t accel_data[3])
{
	uint32_t mag = 0;
	int cadence = -1;

	mag = (uint32_t)(accel_data[0] +
			 accel_data[1] +
			 accel_data[2]);
	if (mag > MAG_THRESHOLD_LVL) {
		step_count++;
	}
	/* Compute cadence every FIVE_SEC_DURATION period */
	if (get_uptime_ms() >=
	    (local_time + FIVE_SEC_DURATION)) {
		/* cadence unit is nb/mn  and limited to MAX_CADENCE */
		cadence = (step_count * FIVE_SEC_IN_ONE_MIN) % MAX_CADENCE;
		step_count = 0;
		local_time = get_uptime_ms();
	}
	return cadence;
}

/*
 * The callback function below is the executed function on reception of
 * accelerometer data.
 */
static int stepcadence_algorithm_exec(void **sensor_data, feed_general_t *feed)
{
	int16_t accel_data[3] = { 0 };
	int index_a = GetDemandIdx(feed, SENSOR_ACCELEROMETER);
	int result = 0;
	int ret = 0;

	/* Retrieve accelerometer data and run algorithm */
	if (index_a >= 0 && sensor_data[index_a] != NULL) {
		int size = GetSensorDataFrameSize(
			feed->demand[index_a].type,
			feed->demand[index_a].id);
		if (size > 0)
			memcpy(&accel_data[0], sensor_data[index_a], size);

		/* Run algorithm and report cadence if any */
		result = stepcadence_algorithm_process(accel_data);
		if (result != -1) {
			exposed_sensor_t *sensor = GetExposedStruct(
				SENSOR_ABS_STEPCADENCE, DEFAULT_ID);
			if (sensor != NULL) {
				struct stepcadence_result *value =
					(struct stepcadence_result *)
					sensor->rpt_data_buf;
				value->cadence = result;
				sensor->ready_flag = 1;
				ret = 1;
			}
		}
	}

	/* Accept to go to idle */
	START_IDLE(feed);

	return ret;
}

/*
 * The callback function below is called to initialize the algorithm.
 */
static int stepcadence_algorithm_init(feed_general_t *feed_ptr)
{
	return 0;
}


/*
 * The callback function below is called to stop the algorithm.
 */
static int stepcadence_algorithm_deinit(feed_general_t *feed_ptr)
{
	return 0;
}



/*
 * The callback function below is called when there is no motion in
 * physical sensor so all the algorithms have to go to sleep.
 */
static int stepcadence_algorithm_goto_idle(feed_general_t *feed_prt)
{
	return 0;
}


/*
 * The callback function below is called when there is any motion in
 * physical sensor, and all the basic algorithms will be awaken by sensor core.
 */
static int stepcadence_algorithm_out_idle(feed_general_t *feed_prt)
{
	return 0;
}



/*
 * In the struct sensor_data_data_demand_t:
 * - freq : frequence value(Hz) of physical sensor sampling
 *          that is need by basic algorithm.
 * - rt   : reporting time (ms) of sensor_core feeding interval, which is
 *          required for basic algorithm.
 */
sensor_data_demand_t stepcadence_algo_demand[] = {
	{
		.type = SENSOR_ACCELEROMETER,
		.id = DEFAULT_ID,
		.range_max = DEFAULT_VALUE,
		.range_min = DEFAULT_VALUE,
		.freq = 100,
		.rt = 1000,
	}
};

/*
 * feed_general_t struct figures out user's basic
 * algorithm attribute and reference callbacks.
 * - type : basic algo type that user can define to one of
 *          basic_algo_type_t enum in opencore_algo_common.h.
 * - demand: describes what physical sensor data is needed in the basic algo.
 * - demand_length: count of demand array.
 * - ctl_api: consists of the api callbacks used to control the basic algo,
 *            such as init, exex, goto_idle and out_idle
 * <define_feedinit> is used to make out the feed list in opencore.
 */
static feed_general_t stepcadence_algo = {
	.type = BASIC_ALGO_STEPCOUNTER,
	.demand = stepcadence_algo_demand,
	.demand_length = sizeof(stepcadence_algo_demand)
			 / sizeof(sensor_data_demand_t),
	.ctl_api = {
		.init = stepcadence_algorithm_init,
		.deinit = stepcadence_algorithm_deinit,
		.exec = stepcadence_algorithm_exec,
		.goto_idle = stepcadence_algorithm_goto_idle,
		.out_idle = stepcadence_algorithm_out_idle,
	},
};
define_feedinit(stepcadence_algo);

/*
 * exposed_sensor_t struct is the event reporter of the algo.
 * - depend_flag: shift value of the type of the basic algo that user defined.
 * - type: type of the exposed sensor that user defines to one of
 *         ss_sensor_type_t enum in sensor_data_format.h and add this type mask
 *         to BOARD_SENSOR_MASK define.
 * - rpt_data_buf: pointer of report data buf.
 * - rpt_data_buf_len: length of rpt_data_buf.
 * <define_exposedinit> is used to expose the sensor.
 */
static exposed_sensor_t stepcadence_exposed_sensor = {
	.depend_flag = 1 << BASIC_ALGO_STEPCOUNTER,
	.type = SENSOR_ABS_STEPCADENCE,
	.id = DEFAULT_ID,
	.rpt_data_buf = (void *)&stepcadence_rpt_buf,
	.rpt_data_buf_len = sizeof(struct stepcadence_result),
};
define_exposedinit(stepcadence_exposed_sensor);
