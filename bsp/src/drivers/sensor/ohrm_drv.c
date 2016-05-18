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
#include "ohrm_drv.h"
#include "infra/features.h"
#include "features_soc.h"
#include "drivers/gpio.h"

DEFINE_LOG_MODULE(LOG_MODULE_OHRM, "OHRM")

const static uint8_t dcp_mux_lut[NUM_MUX_POSITIONS] = {
	0,
	64,
	96,
	112,
	120,
	124,
	126,
	127,
};

#define REV_1 0
#define REV_2 1
/* board_feature value: VER_1=0 VER_2=1 */
static uint8_t board_feature;

#define WRIST_OFF  1
#define WRIST_ON   0
static uint8_t wrist_state;

static struct td_device *ss_gpio_dev_0;
static struct td_device *ss_gpio_dev_1;
static optic_track_mem_t loop_ctl_ohrm_track_mem;

static void delay(uint32_t delay_us)
{
	uint32_t interval = delay_us * 32 / 1000;
	uint32_t start_time = get_uptime_32k();

	while ((uint32_t)(get_uptime_32k() - start_time) < interval) ;
}

static int ohrm_activate(struct phy_sensor_t *sensor, bool enable)
{
	if (enable == true) {
		uint8_t tx_buffer[4] = { 0 };
		/* enable ss_0/5V_EN */
		switch (board_feature) {
		case REV_1:
			ipc_request_sync_int(IPC_WRITE_MASK, (1 << 20),
					     (1 << 20),
					     (void *)(SOC_GPIO_BASE_ADDR +
						      SOC_GPIO_SWPORTA_DR));
			break;
		case REV_2:
			gpio_write(ss_gpio_dev_0, 0, 1);
			break;
		}
		/* enable ss_10/3V_EN */
		gpio_write(ss_gpio_dev_1, 2, 1);

		delay(10 * 1000);

		/* init adxl */
		adxl362_init();
		adxl362_sampling_register_setup(ADXL362_ASR_32_Hz,
						ADXL362_WATERMARK_CNT);
		adxl362_sample_trigger(false);

		/* init digpot */
		/* take digpot out of shutdown */
		tx_buffer[0] = DCP_WRITE | DCP_ACR_ADDRESS;
		tx_buffer[1] = 0x40;
		sensor_bus_access(ohrm_sba_info, tx_buffer, 2, NULL, 0, 0,
				  DIGPOT_SLAVE_ADDR);
		/* write the inital wiper setting to the DCP*/
		tx_buffer[0] = DCP_WRITE | DCP_WR0_ADDRESS;
		tx_buffer[1] = DCP_WR_INIT;
		sensor_bus_access(ohrm_sba_info, tx_buffer, 2, NULL, 0, 0,
				  DIGPOT_SLAVE_ADDR);

		/* reset DAC */
		tx_buffer[0] = 0x48;
		tx_buffer[1] = 0x00;
		tx_buffer[2] = 0x00;
		sensor_bus_access(ohrm_sba_info, tx_buffer, 3, NULL, 0, 0,
				  DAC_SLAVE_ADDR);

		/* init loop contol algo */
		alg_optic_init(&loop_ctl_ohrm_track_mem);
	} else {
		adxl362_standby();
		/* disable ss_0/5V_EN */
		switch (board_feature) {
		case REV_1:
			ipc_request_sync_int(IPC_WRITE_MASK, 0, (1 << 20),
					     (void *)(SOC_GPIO_BASE_ADDR +
						      SOC_GPIO_SWPORTA_DR));
			break;
		case REV_2:
			gpio_write(ss_gpio_dev_0, 0, 0);
			break;
		}
		/* disable ss_10/3V_EN */
		gpio_write(ss_gpio_dev_1, 2, 0);
	}
	return 0;
}


static int ohrm_drv_open(struct phy_sensor_t *sensor)
{
	int ret = 0;

	board_feature = board_feature_has(HW_OHRM_GPIO_SS);
	ss_gpio_dev_0 = &pf_device_ss_gpio_8b0;
	ss_gpio_dev_1 = &pf_device_ss_gpio_8b1;
	gpio_cfg_data_t pin_cfg = {
		.gpio_type = GPIO_OUTPUT,
		.int_type = LEVEL,
		.int_polarity = ACTIVE_LOW,
		.int_debounce = DEBOUNCE_OFF,
		.int_ls_sync = LS_SYNC_OFF,
		.gpio_cb = NULL
	};
	/* init ss_10/3V_EN */
	gpio_set_config(ss_gpio_dev_1, 2, &pin_cfg);
	/* init ss_13/LED1 */
	gpio_set_config(ss_gpio_dev_1, 3, &pin_cfg);
	/* init ss_11/LED2 */
	gpio_set_config(ss_gpio_dev_1, 5, &pin_cfg);

	switch (board_feature) {
	case REV_1:
		/* init soc_gpios 1/BURST_EN 14/OP_AMP_SD 20/5V_EN */
		ipc_request_sync_int(IPC_WRITE_MASK,
				     (1 << 20 | 1 << 14 | 1 << 1),
				     (1 << 20 | 1 << 14 | 1 << 1),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DDR));
		ipc_request_sync_int(IPC_WRITE_MASK, (1 << 20), (1 << 20),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DR));
		break;
	case REV_2:
		/* init ss_12/OP_AMP */
		gpio_set_config(ss_gpio_dev_1, 4, &pin_cfg);
		/* init ss_1/BURST_EN */
		gpio_set_config(ss_gpio_dev_0, 1, &pin_cfg);
		/* init ss_0/5V_EN */
		gpio_set_config(ss_gpio_dev_0, 0, &pin_cfg);
		/* enable ss_0/5V_EN */
		gpio_write(ss_gpio_dev_0, 0, 1);
		break;
	}

	/* enable ss_10/3V_EN */
	gpio_write(ss_gpio_dev_1, 2, 1);

	delay(10 * 1000);

	/* init adxl */
	if (0 > adxl362_init()) {
		pr_error(LOG_MODULE_OHRM, "fail to init adxl");
		ret = -1;
	}

	/* disable ss_0/5V_EN */
	switch (board_feature) {
	case REV_1:
		ipc_request_sync_int(IPC_WRITE_MASK, 0, (1 << 20),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DR));
		break;
	case REV_2:
		gpio_write(ss_gpio_dev_0, 0, 0);
		break;
	}

	/* disable ss_10/3V_EN */
	gpio_write(ss_gpio_dev_1, 2, 0);
	return ret;
}

static void ohrm_drv_close(struct phy_sensor_t *sensor)
{
	return;
}

static int ohrm_read_data(struct phy_sensor_t *sensor, uint8_t *buf,
			  uint16_t buff_len)
{
	uint8_t temp_tx_buffer[3] = { 0 };
	struct ohrm_phy_data *data_ptr = (struct ohrm_phy_data *)buf;
	uint8_t temp_code;
	uint16_t dac_code;
	bool leval;
	uint16_t adc_blind_value;
	DRIVER_API_RC ret;


	switch (board_feature) {
	case REV_1:
		/* op_amp enable*/
		ipc_request_sync_int(IPC_WRITE_MASK, (1 << 14), (1 << 14),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DR));
		break;
	case REV_2:
		/* enable ss_12/OP_AMP */
		gpio_write(ss_gpio_dev_1, 4, 1);
		break;
	}

	/* read adc blindly to check if wrist on or off */
	ret = ss_adc_read(OHRM_ADC_CHANNEL, &adc_blind_value);
	if (ret != DRV_RC_OK)
		pr_error(LOG_MODULE_OHRM, "OHRM adc_read error(%d)", ret);

	wrist_state = alg_hr_on_wrist_detection(
		adc_blind_value, loop_ctl_ohrm_track_mem.dcp_code,
		loop_ctl_ohrm_track_mem.
		mux_code);
	data_ptr->adc_blind_value = adc_blind_value;

	/* make DAC into normal mode */
	temp_tx_buffer[0] = DAC_NORMAL;
	sensor_bus_access(ohrm_sba_info, temp_tx_buffer, 3, NULL, 0, 0,
			  DAC_SLAVE_ADDR);

	/* delay 600us to make DAC stable */
	delay(600);

	/* set DAC output voltage */
	dac_code = loop_ctl_ohrm_track_mem.dac_code;
	temp_tx_buffer[0] = (uint8_t)(dac_code >> 8) | (3 << 4);
	temp_tx_buffer[1] = (uint8_t)dac_code;
	sensor_bus_access(ohrm_sba_info, temp_tx_buffer, 3, NULL, 0, 0,
			  DAC_SLAVE_ADDR);

	switch (board_feature) {
	case REV_1:
		/* burst enable */
		ipc_request_sync_int(IPC_WRITE_MASK, (1 << 1), (1 << 1),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DR));
		break;
	case REV_2:
		/* enable ss_1/BURST_EN */
		gpio_write(ss_gpio_dev_0, 1, 1);
		break;
	}

	/* turn on led 1 and 2 */
	gpio_write(ss_gpio_dev_1, 3, 1);
	gpio_write(ss_gpio_dev_1, 5, 1);

	/* trigger accelerometer sampling */
	adxl362_sample_trigger(true);

	/* wait 50~1700us according to mux_code */
	delay(50);

	/* get adc value */
	ret = ss_adc_read(OHRM_ADC_CHANNEL, &data_ptr->adc_value);
	if (ret != DRV_RC_OK)
		pr_error(LOG_MODULE_OHRM, "OHRM adc_read error(%d)", ret);

	/* turn off led 1 and 2 */
	gpio_write(ss_gpio_dev_1, 3, 0);
	gpio_write(ss_gpio_dev_1, 5, 0);

	switch (board_feature) {
	case REV_1:
		/* disable op_amp and burst */
		ipc_request_sync_int(IPC_WRITE_MASK, 0, (1 << 14 | 1 << 1),
				     (void *)(SOC_GPIO_BASE_ADDR +
					      SOC_GPIO_SWPORTA_DR));
		break;
	case REV_2:
		/* disable ss_1/BURST_EN */
		gpio_write(ss_gpio_dev_0, 1, 0);
		/* disable ss_12/OP_AMP */
		gpio_write(ss_gpio_dev_1, 4, 0);
		break;
	}

	/* manage accelerometer sampling */
	leval = adxl362_read_pin1_level();
	if (leval == true) {
		uint8_t adxl_rx_buffer[ADXL362_FRAME_SIZE *
				       ADXL362_WATERMARK_CNT] = { 0 };
		adxl362_fifo_read(adxl_rx_buffer, ADXL362_WATERMARK_CNT);
		int16_t *ptr_from = (int16_t *)adxl_rx_buffer;
		data_ptr->x = ptr_from[0];
		data_ptr->y = ptr_from[1];
		data_ptr->z = ptr_from[2];
	}
	adxl362_sample_trigger(false);

	/* let DAC into shut down mode */
	temp_tx_buffer[0] = DAC_SHUTDOWN;
	sensor_bus_access(ohrm_sba_info, temp_tx_buffer, 3, NULL, 0, 0,
			  DAC_SLAVE_ADDR);

	/* copy mux_code, dac_code and dcp_code from loop_ctl_ohrm_track_mem to ohrm_phy_data */
	data_ptr->mux_code = loop_ctl_ohrm_track_mem.mux_code;
	data_ptr->dac_code = loop_ctl_ohrm_track_mem.dac_code;
	data_ptr->dcp_code = loop_ctl_ohrm_track_mem.dcp_code;

	/* update values in loopctl_ohrm_track_mem by calling alg_optic */
	loop_ctl_ohrm_track_mem.adc_conv = data_ptr->adc_value;
	alg_optic(&loop_ctl_ohrm_track_mem);

	/* set newest values in loop_ctl_ohrm_track_mem to dev
	 *      updata dcp value */
	temp_code = loop_ctl_ohrm_track_mem.dcp_code;
	temp_tx_buffer[0] = DCP_WRITE | DCP_WR0_ADDRESS;
	temp_tx_buffer[1] = temp_code;
	sensor_bus_access(ohrm_sba_info, temp_tx_buffer, 2, NULL, 0, 0,
			  DIGPOT_SLAVE_ADDR);

	/* updata mux value */
	temp_code = dcp_mux_lut[loop_ctl_ohrm_track_mem.mux_code];
	temp_tx_buffer[0] = DCP_WRITE | DCP_WR1_ADDRESS;
	temp_tx_buffer[1] = temp_code;
	sensor_bus_access(ohrm_sba_info, temp_tx_buffer, 2, NULL, 0, 0,
			  DIGPOT_SLAVE_ADDR);

	return sizeof(struct ohrm_phy_data);
}

static struct phy_sensor_t ohrm_sensor = {
	.type = SENSOR_OHRM,
	.raw_data_len = sizeof(struct ohrm_phy_data),
	.report_mode_mask = PHY_SENSOR_REPORT_MODE_POLL_REG_MASK,
	.api = {
		.open = ohrm_drv_open,
		.activate = ohrm_activate,
		.close = ohrm_drv_close,
		.read = ohrm_read_data,
	},
};

int ohrm_sensor_register(void)
{
	int ret = sensor_register(&ohrm_sensor);

	return ret;
}
