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

/* Intel Quark Watchdog Timer driver
 *
 */

#include <watchdog.h>
#include "infra/log.h"
#include "machine.h"
#include "drivers/clk_system.h"

#include <drivers/ioapic.h>

/* To be removed once suspend and resume dependency is resolved */
#include "infra/device.h"
#include "drivers/intel_qrk_wdt.h"

static struct clk_gate_info_s clk_gate_info = {
	.clk_gate_register = PERIPH_CLK_GATE_CTRL,
	.bits_mask = WDT_CLK_GATE_MASK,
};

static enum wdt_mode config_mode;

static void qrk_cxxxx_wdt_clk_enable(struct device *dev)
{
	MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
			       QRK_SCSS_PERIPH_CFG0_OFFSET) |=
		QRK_SCSS_PERIPH_CFG0_WDT_ENABLE;
}

static void qrk_cxxxx_wdt_gating_enable(struct clk_gate_info_s *clk_gate_info)
{
	set_clock_gate(clk_gate_info, CLK_GATE_ON);
}

static void qrk_cxxxx_wdt_clk_disable(struct device *dev)
{
	/* Disable the clock for the peripheral watchdog */
	MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
			       QRK_SCSS_PERIPH_CFG0_OFFSET) &=
		~QRK_SCSS_PERIPH_CFG0_WDT_ENABLE;
}

static void qrk_cxxxx_wdt_gating_disable(struct clk_gate_info_s *clk_gate_info)
{
	set_clock_gate(clk_gate_info, CLK_GATE_OFF);
}

static void qrk_cxxxx_wdt_tickle(struct device *dev)
{
	/* This register is used to restart the WDT
	 *  counter. as a safetly feature to pervent
	 *  accidenctal restarts the value 0x76 must be
	 *  written. A restart also clears the WDT
	 *  interrupt.*/
	MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR,
			       QRK_WDT_CRR) = QRK_WDT_CRR_VAL;
}

static void qrk_cxxxx_wdt_irq_connect()
{
	/* routed watchdog to NMI*/
	IRQ_CONNECT(SOC_WDT_INTERRUPT, 0, NULL, NULL, IOAPIC_NMI);
	irq_enable(SOC_WDT_INTERRUPT);
}

static int qrk_cxxxx_wdt_set_config(struct device *	dev,
				    struct wdt_config * config)
{
	int ret = DEV_OK;

	qrk_cxxxx_wdt_gating_enable(&clk_gate_info);
	/* Enables the clock for the peripheral watchdog */
	qrk_cxxxx_wdt_clk_enable(dev);

	/*  Set timeout value
	 *  [7:4] TOP_INIT - the initial timeout value is hardcoded in silicon,
	 *  only bits [3:0] TOP are relevant.
	 *  Once tickled TOP is loaded at the next expiration.
	 */
	uint32_t i;
	uint32_t ref =
		(1 << 16) / (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000);        /* 2^16/FREQ_CPU */
	uint32_t timeout = config->timeout * 1000;
	for (i = 0; i < 16; i++) {
		if (timeout <= ref) break;
		ref = ref << 1;
	}
	if (i > 15) {
		ret = DEV_INVALID_CONF;
		i = 15;
	}
	MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR, QRK_WDT_TORR) = i;

	/* Set response mode */
	if (WDT_MODE_RESET == config->mode) {
		MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR,
				       QRK_WDT_CR) &= ~QRK_WDT_CR_INT_ENABLE;
	} else {
		MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR,
				       QRK_WDT_CR) |= QRK_WDT_CR_INT_ENABLE;

		/* routed watchdog to NMI*/
		qrk_cxxxx_wdt_irq_connect();

		/* unmask WDT interrupts to quark  */
		QRK_UNMASK_INTERRUPTS(SCSS_INT_WATCHDOG_MASK_OFFSET);
	}

	/* Save config mode */
	config_mode = config->mode;

	/* Enable WDT, cannot be disabled until soc reset */
	MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR,
			       QRK_WDT_CR) |= QRK_WDT_CR_ENABLE;

	qrk_cxxxx_wdt_tickle(dev);

	return ret;
}


static struct wdt_driver_api wdt_dw_funcs = {
	.set_config = qrk_cxxxx_wdt_set_config,
	.get_config = NULL,
	.enable = qrk_cxxxx_wdt_clk_enable,
	.disable = qrk_cxxxx_wdt_clk_disable,
	.reload = qrk_cxxxx_wdt_tickle,
};


int qrk_cxxxx_wdt_init(struct device *dev)
{
	dev->driver_api = &wdt_dw_funcs;
	return 0;
}

DEVICE_INIT(wdt, "WATCHDOG", &qrk_cxxxx_wdt_init,
	    NULL, NULL,
	    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);


static int qrk_cxxxx_wdt_suspend(struct td_device *dev, PM_POWERSTATE state)
{
	struct wdt_pm_data *registers = (struct wdt_pm_data *)dev->priv;

	/* Save Config */
	registers->control_register =
		MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR, QRK_WDT_CR);
	registers->timeout_range_register =
		MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR, QRK_WDT_TORR);

	/* Disable WDT */
	qrk_cxxxx_wdt_gating_disable(&clk_gate_info);
	return DRV_RC_OK;
}

static int qrk_cxxxx_wdt_resume(struct td_device *dev)
{
	struct wdt_pm_data *registers = (struct wdt_pm_data *)dev->priv;

	/* Re-enable watchdog */
	if (config_mode == WDT_MODE_INTERRUPT_RESET)
		qrk_cxxxx_wdt_irq_connect();
	qrk_cxxxx_wdt_gating_enable(&clk_gate_info);

	/* unmask WDT interrupts to quark  */
	QRK_UNMASK_INTERRUPTS(SCSS_INT_WATCHDOG_MASK_OFFSET);

	/* Restore Config */
	MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR, QRK_WDT_TORR) =
		registers->timeout_range_register;
	MMIO_REG_VAL_FROM_BASE(QRK_WDT_BASE_ADDR, QRK_WDT_CR) =
		registers->control_register;
	qrk_cxxxx_wdt_tickle(NULL);

	pr_debug(LOG_MODULE_DRV, "wdt resume device %d", dev->id);
	return DRV_RC_OK;
}

struct driver watchdog_driver = {
	.init = NULL,
	.suspend = qrk_cxxxx_wdt_suspend,
	.resume = qrk_cxxxx_wdt_resume
};
