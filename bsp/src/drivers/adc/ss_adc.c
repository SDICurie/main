/******************************************************************************
 *
 * Synopsys DesignWare Sensor and Control IP Subsystem IO Software Driver and
 * documentation (hereinafter, "Software") is an Unsupported proprietary work
 * of Synopsys, Inc. unless otherwise expressly agreed to in writing between
 * Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Modifications Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <nanokernel.h>
#include <arch/cpu.h>
#include "util/assert.h"
#include "os/os.h"
#include "machine.h"
#include "adc_priv.h"

#include "drivers/clk_system.h"
#include "drivers/ss_adc.h"

#include "infra/device.h"
#include "infra/log.h"
#include "infra/ipc.h"
#include "infra/time.h"

#define DRV_NAME "adc"
#define ADC_MAX_CNT (1)

#define ADC_CLOCK_GATE          (1 << 31)
#define ADC_STANDBY             (0x02)
#define ADC_NORMAL_WO_CALIB     (0x04)
#define ADC_MODE_MASK           (0x07)

#define ONE_BIT_SET     (0x1)
#define FIVE_BITS_SET   (0x1f)
#define SIX_BITS_SET    (0x3f)
#define ELEVEN_BITS_SET (0x7ff)

#define INPUT_MODE_POS      (5)
#define CAPTURE_MODE_POS    (6)
#define OUTPUT_MODE_POS     (7)
#define SERIAL_DELAY_POS    (8)
#define SEQUENCE_MODE_POS   (13)
#define SEQ_ENTRIES_POS     (16)
#define THRESHOLD_POS       (24)

#define SEQ_DELAY_EVEN_POS  (5)
#define SEQ_MUX_ODD_POS     (16)
#define SEQ_DELAY_ODD_POS   (21)

#define ADC_PM_FSM_STATUS_MSK   (1 << 3)

#define REPEAT_TIME         24
#define DUMP_NO             8
#define FINE_RATIO          8
#define FINE_SAMPLE_DLY     14

const ss_adc_cfg_data_t fine_config = {
	.in_mode = SINGLED_ENDED,
	.out_mode = PARALLEL,
	.serial_dly = 1,
	.seq_mode = SINGLESHOT,
	.capture_mode = FALLING_EDGE,
	.clock_ratio = FINE_RATIO,
	.sample_width = WIDTH_12_BIT,
	.priv = NULL,
};

static void adc_goto_normal_mode_wo_calibration(struct td_device *dev);
static void adc_goto_deep_power_down(struct td_device *dev);
static void ss_adc_enable(struct td_device *dev);
static void ss_adc_disable(struct td_device *dev);

static void ss_adc_set_config(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;
	uint32_t reg_val = 0, val = 0, ctrl = 0;

	ctrl = ADC_INT_DSB | ADC_CLK_ENABLE;
	WRITE_ARC_REG(ctrl, info->reg_base + ADC_CTRL);

	/* set sample width, input mode, output mode and serial delay */
	reg_val = READ_ARC_REG(info->reg_base + ADC_SET);
	reg_val &= ADC_CONFIG_SET_MASK;
	val = (fine_config.sample_width) & FIVE_BITS_SET;
	val |= ((fine_config.in_mode & ONE_BIT_SET) << INPUT_MODE_POS);
	val |= ((fine_config.capture_mode & ONE_BIT_SET) << CAPTURE_MODE_POS);
	val |= ((fine_config.out_mode & ONE_BIT_SET) << OUTPUT_MODE_POS);
	val |= ((fine_config.serial_dly & FIVE_BITS_SET) << SERIAL_DELAY_POS);
	val |= ((fine_config.seq_mode & ONE_BIT_SET) << SEQUENCE_MODE_POS);
	WRITE_ARC_REG(reg_val | val, info->reg_base + ADC_SET);

	info->rx_len = 0;
	info->seq_mode = fine_config.seq_mode;
	info->seq_size = 1;
	info->state = ADC_STATE_IDLE;

	/* set  clock ratio */
	WRITE_ARC_REG(fine_config.clock_ratio & ADC_CLK_RATIO_MASK,
		      info->reg_base + ADC_DIVSEQSTAT);

	/* disable clock once setup done */
	WRITE_ARC_REG(ADC_INT_ENABLE & ~(ADC_CLK_ENABLE),
		      info->reg_base + ADC_CTRL);
}

static void ss_adc_enable(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;

	adc_goto_normal_mode_wo_calibration(dev);
	/* Enable adc clock and reset sequence pointer */
	set_clock_gate(info->clk_gate_info, CLK_GATE_ON);

	WRITE_ARC_REG(ADC_INT_DSB | ADC_CLK_ENABLE | ADC_SEQ_TABLE_RST
		      | ADC_CLR_OVERFLOW | ADC_CLR_UNDRFLOW | ADC_CLR_DATA_A
		      , info->reg_base + ADC_CTRL);
}

static void ss_adc_disable(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;
	uint32_t saved;

	WRITE_ARC_REG((READ_ARC_REG(info->reg_base +
				    ADC_CTRL) & (~ADC_SEQ_START)),
		      info->reg_base + ADC_CTRL);

	WRITE_ARC_REG(ADC_INT_DSB | ADC_CLK_ENABLE | ADC_SEQ_PTR_RST,
		      info->reg_base + ADC_CTRL);

	/* No need to protect ADC_SET using lock and unlock of interruptions,
	 * we call it qith interrupts locked already or in interrupt context.
	 */
	saved = irq_lock();
	info->cfg.cb_err = NULL;
	info->cfg.cb_rx = NULL;
	/* TODO sak should we do this on disable ?? */
	WRITE_ARC_REG(READ_ARC_REG(info->reg_base + ADC_SET) | ADC_FLUSH_RX,
		      info->reg_base + ADC_SET);
	set_clock_gate(info->clk_gate_info, CLK_GATE_OFF);

	adc_goto_deep_power_down(dev);
	irq_unlock(saved);
}

static DRIVER_API_RC ss_adc_set_seq(struct td_device *dev, uint8_t channel_id)
{
	struct adc_info_t *info = dev->priv;
	uint32_t ctrl = 0, reg_val = 0;
	uint32_t i = 0, num_iters = 0;

	ss_adc_enable(dev);
	assert(REPEAT_TIME > 0);

	/* Setup sequence table */
	info->seq_size = REPEAT_TIME;

	reg_val = READ_ARC_REG(info->reg_base + ADC_SET);
	reg_val &= ADC_SEQ_SIZE_SET_MASK;
	reg_val |= (((REPEAT_TIME - 1) & SIX_BITS_SET) << SEQ_ENTRIES_POS);
	reg_val &= ADC_FTL_SET_MASK;
	reg_val |= ((info->seq_size - 1) << THRESHOLD_POS);
	WRITE_ARC_REG(reg_val, info->reg_base + ADC_SET);

	num_iters = REPEAT_TIME / 2;
	if (num_iters > 0) {
		reg_val =
			((FINE_SAMPLE_DLY &
			  ELEVEN_BITS_SET) << SEQ_DELAY_ODD_POS);
		reg_val |= ((channel_id & FIVE_BITS_SET) << SEQ_MUX_ODD_POS);
		reg_val |=
			((FINE_SAMPLE_DLY &
			  ELEVEN_BITS_SET) << SEQ_DELAY_EVEN_POS);
		reg_val |= (channel_id & FIVE_BITS_SET);
		for (i = 0; i < num_iters; i++)
			WRITE_ARC_REG(reg_val, info->reg_base + ADC_SEQ);
	}

	if (0 != (REPEAT_TIME % 2)) {
		reg_val =
			((FINE_SAMPLE_DLY &
			  ELEVEN_BITS_SET) << SEQ_DELAY_EVEN_POS);
		reg_val |= (channel_id & FIVE_BITS_SET);
		WRITE_ARC_REG(reg_val, info->reg_base + ADC_SEQ);
	}

	/* Reset Sequence Pointer */
	ctrl = READ_ARC_REG(info->reg_base + ADC_CTRL);
	ctrl |= ADC_SEQ_PTR_RST;
	WRITE_ARC_REG(ctrl | ADC_SEQ_PTR_RST, info->reg_base + ADC_CTRL);

	WRITE_ARC_REG(ADC_SEQ_START | ADC_ENABLE | ADC_CLK_ENABLE,
		      info->reg_base + ADC_CTRL);
	return DRV_RC_OK;
}

/*
 * Function to put the ADC controller into a working state.
 */
static void adc_goto_normal_mode_wo_calibration(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;
	uint32_t creg;
	uint32_t saved;

	// read creg slave to get current Power Mode
	creg = READ_ARC_REG(info->creg_slv);

	// perform power up to "Normal mode w/o calibration" cycle if not already there
	if ((creg & ADC_MODE_MASK) != ADC_NORMAL_WO_CALIB) {
		/* Protect AR_IO_CREG_MST0_CTRL using lock and unlock of interruptions */
		saved = irq_lock();
		// Read current CREG master
		creg = READ_ARC_REG(info->creg_mst);
		creg &= ~(ADC_MODE_MASK);
		// request ADC to go to Standby mode
		creg |= ADC_STANDBY | ADC_CLOCK_GATE;
		WRITE_ARC_REG(creg, info->creg_mst);
		irq_unlock(saved);

		// Poll CREG Slave 0 for Power Mode status = requested status
		while (((creg = READ_ARC_REG(info->creg_slv)) &
			ADC_PM_FSM_STATUS_MSK) == 0) ;

		/* Protect AR_IO_CREG_MST0_CTRL using lock and unlock of interruptions */
		saved = irq_lock();
		creg = READ_ARC_REG(info->creg_mst);

		creg &= ~(ADC_MODE_MASK);
		// request ADC to go to Normal mode w/o calibration
		creg |= ADC_NORMAL_WO_CALIB | ADC_CLOCK_GATE;
		WRITE_ARC_REG(creg, info->creg_mst);
		irq_unlock(saved);

		// Poll CREG Slave 0 for Power Mode status = requested status
		while (((creg = READ_ARC_REG(info->creg_slv)) &
			ADC_PM_FSM_STATUS_MSK) == 0) ;
	}
}

static void adc_goto_deep_power_down(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;
	uint32_t creg;
	uint32_t saved;

	// read creg slave to get current Power Mode
	creg = READ_ARC_REG(info->creg_slv);
	// perform cycle down to "Deep Power Down mode" if not already there
	if ((creg & ADC_MODE_MASK) != 0) {
		/* Protect AR_IO_CREG_MST0_CTRL using lock and unlock of interruptions */
		saved = irq_lock();

		// Read current CREG master
		creg = READ_ARC_REG(info->creg_mst);
		creg &= ~(ADC_MODE_MASK);

		// request ADC to go to Deep Power Down mode
		creg |= 0 | ADC_CLOCK_GATE;
		WRITE_ARC_REG(creg, info->creg_mst);
		irq_unlock(saved);

		// Poll CREG Slave 0 for Power Mode status = requested status
		while (((creg = READ_ARC_REG(info->creg_slv)) &
			ADC_PM_FSM_STATUS_MSK) == 0) ;
	}
}

static int ss_adc_init(struct td_device *dev)
{
	struct adc_info_t *info = dev->priv;

	ss_adc_disable(dev); /* disable IP by default */
	info->adc_in_use = mutex_create();
	pr_debug(LOG_MODULE_DRV, "%s %d init", DRV_NAME, dev->id);
	return 0;
}

static int ss_adc_suspend(struct td_device *dev, PM_POWERSTATE state)
{
	pr_debug(LOG_MODULE_DRV, "%s %d suspend", DRV_NAME, dev->id);
	return 0;
}

static int ss_adc_resume(struct td_device *dev)
{
	pr_debug(LOG_MODULE_DRV, "%s %d resume", DRV_NAME, dev->id);
	/* disable IP by default */
	ss_adc_disable(dev);
	return 0;
}

DRIVER_API_RC ss_adc_read(uint8_t channel_id, uint16_t *result_value)
{
	struct td_device *adc_dev = &pf_device_ss_adc;
	struct adc_info_t *info = adc_dev->priv;
	uint32_t data[REPEAT_TIME] = { 0 };
	DRIVER_API_RC ret = DRV_RC_OK;
	uint32_t start_point;
	int count = 0;
	uint32_t temp_value = 0;
	uint32_t reg_get_sample = 0;

	mutex_lock(info->adc_in_use, OS_WAIT_FOREVER);

	ss_adc_set_config(adc_dev);
	ss_adc_set_seq(adc_dev, channel_id);

	start_point = get_uptime_ms();
	reg_get_sample =
		READ_ARC_REG(info->reg_base + ADC_SET) | ADC_POP_SAMPLE;
	while (1) {
		uint32_t reg_val = READ_ARC_REG(info->reg_base + ADC_INTSTAT);
		if (reg_val & ADC_INT_DATA_A) {
			for (int i = 0; i < REPEAT_TIME; i++) {
				/* read sample */
				WRITE_ARC_REG(reg_get_sample,
					      info->reg_base + ADC_SET);
				data[i] = READ_ARC_REG(
					info->reg_base + ADC_SAMPLE);
			}
			/* clear data status*/
			reg_val = READ_ARC_REG(info->reg_base + ADC_CTRL);
			WRITE_ARC_REG(reg_val | ADC_CLR_DATA_A,
				      info->reg_base + ADC_CTRL);
			break;
		}

		if (get_uptime_ms() - start_point >= 10) {
			ret = DRV_RC_TIMEOUT;
			ss_adc_disable(adc_dev);
			goto fail;
		}
	}
	ss_adc_disable(adc_dev);

	for (int i = DUMP_NO; i < REPEAT_TIME; i++) {
		if (data[i] != 0) {
			temp_value += data[i];
			count++;
		}
	}

	*result_value = temp_value / count;
fail:
	mutex_unlock(info->adc_in_use);
	return ret;
}

struct driver ss_adc_driver = {
	.init = ss_adc_init,
	.suspend = ss_adc_suspend,
	.resume = ss_adc_resume,
};
