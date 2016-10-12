/*******************************************************************************
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
 ******************************************************************************/

/*******************************************************************************
 *
 * Modifications Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 ******************************************************************************/

/*
 * Intel SOC SPI driver
 *
 */

#include "drivers/ss_spi_iface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/assert.h"

#include "machine.h"
#include "spi_priv.h"
#include "drivers/ss_dw_spi.h"

/**
 *  Clock speed into SPI peripheral
 */
#define FREQ_SPI_CLOCK_IN   (CONFIG_CLOCK_SPEED * 1000 * 1000) /* CONFIG_CLOCK_SPEED in MHz */

#if 0
#define DEBUG_SPI_DRIVER
extern void printk(const char *, ...);
#define DBG(args ...) printk(args);
#else
#define DBG(args ...)
#endif

static DRIVER_API_RC is_valid_controller(SPI_CONTROLLER id);
static void copy_config_data(spi_cfg_data_t *cfg, SPI_CONTROLLER controller_id);

static void spi_fill_fifo(spi_info_pt dev);
void wait_spi_not_busy(spi_info_pt dev);

struct ss_spi_cfg_driver_data {
	spi_cfg_data_t cfg;
	uint16_t tx_threshold,
		 rx_threshold;
};

#define SPI_MAX_CNT     (2)

static struct ss_spi_cfg_driver_data drv_config[SPI_MAX_CNT];

/* SPI master devices private data structures */
static spi_info_pt spi_handles[SPI_MAX_CNT] = { 0 };

DECLARE_INTERRUPT_HANDLER static void spi_mst0_rx_ISR()
{
	spi_mst_rx_ISR_proc(spi_handles[0]);
}
DECLARE_INTERRUPT_HANDLER static void spi_mst0_tx_ISR()
{
	spi_mst_tx_ISR_proc(spi_handles[0]);
}
DECLARE_INTERRUPT_HANDLER static void spi_mst0_err_ISR()
{
	spi_mst_err_ISR_proc(spi_handles[0]);
}
DECLARE_INTERRUPT_HANDLER static void spi_mst1_rx_ISR()
{
	spi_mst_rx_ISR_proc(spi_handles[1]);
}
DECLARE_INTERRUPT_HANDLER static void spi_mst1_tx_ISR()
{
	spi_mst_tx_ISR_proc(spi_handles[1]);
}
DECLARE_INTERRUPT_HANDLER static void spi_mst1_err_ISR()
{
	spi_mst_err_ISR_proc(spi_handles[1]);
}



static spi_info_t spi_master_devs[SPI_MAX_CNT] = {
	{ .instID = 0,
	  .reg_base = AR_IO_SPI_MST0_CTRL,
	  .rx_vector = IO_SPI_MST0_INT_RX_AVAIL,
	  .tx_vector = IO_SPI_MST0_INT_TX_REQ,
	  .err_vector = IO_SPI_MST0_INT_ERR,
	  .rx_isr = spi_mst0_rx_ISR,
	  .tx_isr = spi_mst0_tx_ISR,
	  .err_isr = spi_mst0_err_ISR,
	  .fifo_depth = IO_SPI_MST0_FS,
	  .spi_rx_avail_mask = SCSS_REGISTER_BASE + INT_SS_SPI_0_RX_AVAIL_MASK,
	  .spi_tx_req_mask = SCSS_REGISTER_BASE + INT_SS_SPI_0_TX_REQ_MASK,
	  .spi_err_mask = SCSS_REGISTER_BASE + INT_SS_SPI_0_ERR_INT_MASK,
	  .creg_spi_clk_ctrl = CREG_CLK_CTRL_SPI0 },
	{ .instID = 1,
	  .reg_base = AR_IO_SPI_MST1_CTRL,
	  .rx_vector = IO_SPI_MST1_INT_RX_AVAIL,
	  .tx_vector = IO_SPI_MST1_INT_TX_REQ,
	  .err_vector = IO_SPI_MST1_INT_ERR,
	  .rx_isr = spi_mst1_rx_ISR,
	  .tx_isr = spi_mst1_tx_ISR,
	  .err_isr = spi_mst1_err_ISR,
	  .fifo_depth = IO_SPI_MST1_FS,
	  .spi_rx_avail_mask = SCSS_REGISTER_BASE + INT_SS_SPI_1_RX_AVAIL_MASK,
	  .spi_tx_req_mask = SCSS_REGISTER_BASE + INT_SS_SPI_1_TX_REQ_MASK,
	  .spi_err_mask = SCSS_REGISTER_BASE + INT_SS_SPI_1_ERR_INT_MASK,
	  .creg_spi_clk_ctrl = CREG_CLK_CTRL_SPI1 },
};

DRIVER_API_RC ss_spi_set_config(SPI_CONTROLLER	controller_id,
				spi_cfg_data_t *config)
{
	DRIVER_API_RC rc = DRV_RC_OK;
	spi_info_pt dev = 0;
	uint32_t timing = 0;
	struct ss_spi_cfg_driver_data *drv_cfg = 0;
	uint32_t reg = 0;
	uint32_t saved;

	copy_config_data(config, controller_id);
	drv_cfg = &drv_config[controller_id];

	/* check controller_id is valid */
	if ((rc = is_valid_controller(controller_id)) != DRV_RC_OK) {
		return rc;
	}

	if (ss_spi_status(controller_id) == SPI_BUSY) {
		return DRV_RC_CONTROLLER_IN_USE;
	}

	dev = &spi_master_devs[controller_id];
	spi_handles[controller_id] = dev;

	/* disable controller */
	WRITE_ARC_REG(SPI_DISABLE, dev->reg_base + SPIEN);
	/* enable clock to controller to allow reg writes */
	WRITE_ARC_REG(SPI_CLK_ENABLED, dev->reg_base + CTRL);

	/* Set frame size, bus mode and transfer mode */
	reg = SPI_CLK_ENABLED | drv_cfg->cfg.data_frame_size |
	      (drv_cfg->cfg.bus_mode << 6) | (drv_cfg->cfg.txfr_mode << 8);
	WRITE_ARC_REG(reg, dev->reg_base + CTRL);
	WRITE_ARC_REG((drv_cfg->tx_threshold << 16) | drv_cfg->rx_threshold,
		      dev->reg_base + FTLR);
	WRITE_ARC_REG(SPI_DISABLE_INT, dev->reg_base + INTR_MASK);

	if (READ_ARC_REG(dev->reg_base + SR) & SPI_STATUS_BUSY) {
		return DRV_RC_CONTROLLER_IN_USE;
	}

	/* Protect TIMING using lock and unlock of interruptions */
	saved = irq_lock();
	timing = READ_ARC_REG(dev->reg_base + TIMING);
	timing &= SPI_SCKDEV_SET_MASK;
	timing |= FREQ_SPI_CLOCK_IN / (drv_cfg->cfg.speed * BAUD_DIVISOR);
	WRITE_ARC_REG(timing, dev->reg_base + TIMING);
	irq_unlock(saved);

	dev->dfs = drv_cfg->cfg.data_frame_size;
	dev->rx_len = 0;
	dev->tx_len = 0;
	dev->mode = drv_cfg->cfg.txfr_mode;
	dev->state = SPI_STATE_READY;

	/* user callbacks */
	dev->err_cb = drv_cfg->cfg.cb_err;
	dev->cb_err_data = drv_cfg->cfg.cb_err_data;
	dev->xfer_cb = drv_cfg->cfg.cb_xfer;
	dev->cb_xfer_data = drv_cfg->cfg.cb_xfer_data;

	irq_connect_dynamic(dev->rx_vector, ISR_DEFAULT_PRIO, dev->rx_isr, NULL,
			    0);
	irq_connect_dynamic(dev->tx_vector, ISR_DEFAULT_PRIO, dev->tx_isr, NULL,
			    0);
	irq_connect_dynamic(dev->err_vector, ISR_DEFAULT_PRIO, dev->err_isr,
			    NULL,
			    0);
	irq_enable(dev->rx_vector);
	irq_enable(dev->tx_vector);
	irq_enable(dev->err_vector);

	/*
	 * SoC SPI config
	 */
	/* Setup SPI Interrupt Routing Mask Registers to allow interrupts through */
	MMIO_REG_VAL(dev->spi_rx_avail_mask) &= ENABLE_SSS_INTERRUPTS;
	MMIO_REG_VAL(dev->spi_tx_req_mask) &= ENABLE_SSS_INTERRUPTS;
	MMIO_REG_VAL(dev->spi_err_mask) &= ENABLE_SSS_INTERRUPTS;

	return DRV_RC_OK;
}

spi_cfg_data_t *ss_spi_get_config(SPI_CONTROLLER controller_id)
{
	return &drv_config[controller_id].cfg;
}

DRIVER_API_RC ss_spi_deconfig(struct sba_master_cfg_data *sba_dev)
{
	spi_info_pt dev = 0;
	uint32_t sba_bus_id = get_bus_id_from_sba(sba_dev->bus_id);

	assert(sba_bus_id <= SOC_SPI_SLAVE_0);
	dev = &spi_master_devs[sba_bus_id];

	/* disable controller */
	WRITE_ARC_REG(SPI_DISABLE, dev->reg_base + SPIEN);

	/* enable clock to controller to allow reg writes */
	WRITE_ARC_REG(SPI_CLK_ENABLED, dev->reg_base + CTRL);

	/* Set SPI registers to hardware reset state */
	WRITE_ARC_REG(0, dev->reg_base + TIMING);
	WRITE_ARC_REG(0, dev->reg_base + FTLR);
	WRITE_ARC_REG(0, dev->reg_base + INTR_MASK);
	WRITE_ARC_REG(0x1f, dev->reg_base + CLR_INTR);

	/* disable clock to controller */
	WRITE_ARC_REG(0, dev->reg_base + CTRL);

	/* disable clock to peripheral */
	CLEAR_ARC_BIT(AR_IO_CREG_MST0_CTRL, dev->creg_spi_clk_ctrl);

	return DRV_RC_OK;
}

DRIVER_API_RC ss_spi_clock_enable(struct sba_master_cfg_data *sba_dev)
{
	uint32_t bus_id = get_bus_id_from_sba(sba_dev->bus_id);

	assert(bus_id <= SOC_SPI_SLAVE_0);
	spi_info_pt dev = &spi_master_devs[bus_id];
	uint32_t spi_con = 0;

	/* enable clock to peripheral */
	set_clock_gate(sba_dev->clk_gate_info, CLK_GATE_ON);
	SET_ARC_BIT(AR_IO_CREG_MST0_CTRL, dev->creg_spi_clk_ctrl);

	/* enable device */
	spi_con = READ_ARC_REG(dev->reg_base + SPIEN);
	spi_con |= SPI_ENABLE;           // 1 enables controller
	WRITE_ARC_REG(spi_con, dev->reg_base + SPIEN);

	return DRV_RC_OK;
}

DRIVER_API_RC ss_spi_clock_disable(struct sba_master_cfg_data *sba_dev)
{
	uint32_t bus_id = get_bus_id_from_sba(sba_dev->bus_id);

	assert(bus_id <= SOC_SPI_SLAVE_0);
	spi_info_pt dev = &spi_master_devs[bus_id];
	uint32_t spi_con = 0;

	/* disable clock to peripheral */
	set_clock_gate(sba_dev->clk_gate_info, CLK_GATE_OFF);
	CLEAR_ARC_BIT(AR_IO_CREG_MST0_CTRL, dev->creg_spi_clk_ctrl);

	/* disable device */
	spi_con = READ_ARC_REG(dev->reg_base + SPIEN);
	spi_con &= ~(SPI_ENABLE);           // 0 disables controller
	WRITE_ARC_REG(spi_con, dev->reg_base + SPIEN);

	return DRV_RC_OK;
}

DRIVER_API_RC ss_spi_transfer(SPI_CONTROLLER controller_id, uint8_t *tx_data,
			      uint32_t tx_data_len, uint8_t *rx_data,
			      uint32_t rx_data_len,
			      SPI_SLAVE_ENABLE slave)
{
	spi_info_pt dev = &spi_master_devs[controller_id];
	uint32_t spien = 0;
	uint32_t ctrl = 0;
	uint32_t intr_mask = 0;
	uint32_t saved;

	/* Protect dev->state using lock and unlock of interruptions */
	saved = irq_lock();
	if ((READ_ARC_REG(dev->reg_base + SR) & SPI_STATUS_BUSY) ||
	    (dev->state != SPI_STATE_READY)) {
		return DRV_RC_FAIL;
	}

	dev->state = SPI_STATE_TRANSMIT;
	irq_unlock(saved);

	spien = READ_ARC_REG(dev->reg_base + SPIEN);
	spien &= SPI_ENB_SET_MASK;

	spien &= SPI_SER_SET_MASK;
	spien |= (slave << 4);



	dev->tx_len = tx_data_len;
	dev->tx_buf = tx_data;
	dev->rx_count = dev->tx_count = 0;
	dev->rx_len = rx_data_len;
	dev->rx_buf = rx_data;

	WRITE_ARC_REG(spien, dev->reg_base + SPIEN);

	/* Protect CTRL using lock and unlock of interruptions */
	saved = irq_lock();
	ctrl =
		READ_ARC_REG(dev->reg_base +
			     CTRL) & SPI_NDF_SET_MASK & SPI_TMOD_SET_MASK;
	ctrl |= (rx_data_len - 1) << 16;
	if (tx_data_len == 0) {
		ctrl |= (SPI_RX_ONLY << 8);
		intr_mask = SPI_ENABLE_RX_INT;
	} else if (rx_data_len == 0) {
		ctrl |= (SPI_TX_ONLY << 8);
		intr_mask = SPI_ENABLE_TX_INT;
	} else {
		ctrl |= (SPI_EPROM_RD << 8);
		intr_mask = SPI_ENABLE_RX_INT | SPI_ENABLE_TX_INT;
	}

	WRITE_ARC_REG(SPI_DISABLE_INT, dev->reg_base + INTR_MASK);
	WRITE_ARC_REG(ctrl, dev->reg_base + CTRL);
	irq_unlock(saved);

	WRITE_ARC_REG(spien | SPI_ENABLE, dev->reg_base + SPIEN);

	/* Disable Interrupt */
	if (tx_data_len == 0) {
		WRITE_ARC_REG(SPI_PUSH_DATA | 0x56, dev->reg_base + DR); // start rx-only transfert
	} else {
		spi_fill_fifo(dev);
	}
	/* Enable Interrupt */
	WRITE_ARC_REG(intr_mask, dev->reg_base + INTR_MASK);
	return DRV_RC_OK;
}

DRIVER_SPI_STATUS_CODE ss_spi_status(SPI_CONTROLLER controller_id)
{
	DRIVER_SPI_STATUS_CODE rc = SPI_OK;
	spi_info_pt dev = &spi_master_devs[controller_id];
	uint32_t status = 0;

	status = READ_ARC_REG(dev->reg_base + SR);
	if ((status & SPI_STATUS_BUSY)) {
		rc = SPI_BUSY;
	}
	return rc;
}

/*
 * Check to see if this controller is part of the design ( determined at build time )
 */
static DRIVER_API_RC is_valid_controller(SPI_CONTROLLER id)
{
	// do checks here
	return DRV_RC_OK;
}

static void copy_config_data(spi_cfg_data_t *cfg, SPI_CONTROLLER controller_id)
{
	/* copy passed in config data locally */
	memcpy(&drv_config[controller_id].cfg, cfg,
	       sizeof(drv_config[controller_id].cfg));
	/* TODO default private data - may need to look at this.*/
	drv_config[controller_id].tx_threshold = SPI_TX_FIFO_THRESHOLD;
	drv_config[controller_id].rx_threshold = SPI_RX_FIFO_THRESHOLD;
}

void wait_spi_not_busy(spi_info_pt dev)
{
	uint32_t status;

	do {
		status = READ_ARC_REG(dev->reg_base + SR);
	} while ((status & SPI_STATUS_BUSY));
}


static void transfer_complete(spi_info_pt dev)
{
	uint32_t state = dev->state;

	WRITE_ARC_REG(SPI_DISABLE_INT, dev->reg_base + INTR_MASK);
	dev->state = SPI_STATE_READY;
	wait_spi_not_busy(dev);
	if (SPI_STATE_TRANSMIT == state) {
		if (_Usually(NULL != dev->xfer_cb)) {
			dev->xfer_cb(dev->cb_xfer_data);
		}
	} else if (SPI_STATE_ERROR == state) {
		if (_Usually(NULL != dev->err_cb)) {
			dev->err_cb(dev->cb_err_data);
		}
	}
}

void spi_fill_fifo(spi_info_pt dev)
{
	uint32_t status = READ_ARC_REG(dev->reg_base + SR);
	uint32_t threshold;

	DBG("%s: status: %x tc: %d rc: %d tlen: %d rlen: %d\n", __func__,
	    status,
	    dev->tx_count, dev->rx_count, dev->tx_len,
	    dev->rx_len);
	while ((status & SPI_STATUS_TFNF) &&
	       ((READ_ARC_REG(dev->reg_base +
			      TXFLR) + READ_ARC_REG(dev->reg_base + RXFLR)) <
		dev->fifo_depth)) {
		if (dev->tx_count < dev->tx_len) {
			DBG("push %x\n", dev->tx_buf[dev->tx_count]);
			WRITE_ARC_REG(
				SPI_PUSH_DATA |
				dev->tx_buf[dev->tx_count++],
				dev->reg_base + DR);
		} else {
			threshold = READ_ARC_REG(dev->reg_base + FTLR);
			threshold &= 0x0000ffff; // tx treshold = 0
			// next tx_ISR interrupt will be triggered when tx fifo is empty
			WRITE_ARC_REG(threshold, dev->reg_base + FTLR);
			break;
		}
		status = READ_ARC_REG(dev->reg_base + SR);
	}
}

void spi_mst_rx_ISR_proc(spi_info_pt dev)
{
	WRITE_ARC_REG(SPI_RX_CLR_INTR, dev->reg_base + CLR_INTR);
	while (READ_ARC_REG(dev->reg_base + RXFLR) > 0) {
		if (dev->rx_count < dev->rx_len) {
			WRITE_ARC_REG(SPI_POP_DATA, dev->reg_base + DR);
			dev->rx_buf[dev->rx_count++] = READ_ARC_REG(
				dev->reg_base + DR);
			DBG("pop %x\n", dev->rx_buf[dev->rx_count - 1]);
		}

		if ((dev->rx_len > 0) && (dev->rx_count == dev->rx_len)) {
			transfer_complete(dev);
			return;
		}
	}
}

void spi_mst_tx_ISR_proc(spi_info_pt dev)
{
	WRITE_ARC_REG(SPI_TX_CLR_INTR, dev->reg_base + CLR_INTR);
	if (dev->tx_count < dev->tx_len) {
		spi_fill_fifo(dev);
	} else {
		/* xfer complete, wait for last tx_empty and wait for end of reception. */
		if (dev->rx_len == 0) {
			if ((READ_ARC_REG(dev->reg_base + TXFLR)) == 0) { // everything written
				transfer_complete(dev);
				return;
			}
		} else {
			WRITE_ARC_REG(SPI_ENABLE_RX_INT,
				      dev->reg_base + INTR_MASK);        // only rx remaining
		}
	}
}

void spi_mst_err_ISR_proc(spi_info_pt dev)
{
	uint32_t spien = READ_ARC_REG(dev->reg_base + SPIEN);

	spien &= SPI_ENB_SET_MASK;
	WRITE_ARC_REG(spien | SPI_DISABLE, dev->reg_base + SPIEN);
	WRITE_ARC_REG(SPI_DISABLE_INT, dev->reg_base + INTR_MASK);
	dev->state = SPI_STATE_ERROR;
	transfer_complete(dev);
}
