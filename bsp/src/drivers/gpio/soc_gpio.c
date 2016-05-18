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

#include <nanokernel.h>

#include "drivers/gpio.h"

#include "machine.h"
#include "infra/device.h"
#include "infra/log.h"
#include "util/assert.h"

#define GPIO_CLKENA_POS         (31)
#define GPIO_LS_SYNC_POS        (0)

/* Access registers with a shorter macro */
#define GPIO_REG(_dev, _offset) MMIO_REG_VAL_FROM_BASE(_dev->reg_base, _offset)

static void soc_gpio_ISR_proc(struct td_device *dev);

/*! \brief  Interrupt handler for GPIO port 0 (32 bits)
 */
#ifdef CONFIG_SOC_GPIO_32
static void gpio_isr(void *unused)
{
	soc_gpio_ISR_proc(&pf_device_soc_gpio_32);
}
#endif

/*! \brief  Interrupt handler for GPIO port 1 (aon, 6 bits)
 */
#ifdef CONFIG_SOC_GPIO_AON
static void gpio_aon_isr(void *unused)
{
	soc_gpio_ISR_proc(&pf_device_soc_gpio_aon);
}
#endif


static int soc_gpio_init(struct td_device *dev)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	/* enable peripheral clock */
	SET_MMIO_BIT(
		(volatile uint32_t *)(gpio_dev->reg_base +
				      SOC_GPIO_LS_SYNC),
		(uint32_t)GPIO_CLKENA_POS);
	SET_MMIO_BIT(
		(volatile uint32_t *)(gpio_dev->reg_base +
				      SOC_GPIO_LS_SYNC),
		(uint32_t)GPIO_LS_SYNC_POS);
	/* Clear any existing interrupts */
	GPIO_REG(gpio_dev, SOC_GPIO_PORTA_EOI) = ~(0);
	/* Disable interrupts for all GPIO lines and mask all */
	GPIO_REG(gpio_dev, SOC_GPIO_INTMASK) = 0;
	GPIO_REG(gpio_dev, SOC_GPIO_INTEN) = 0;
	/* enable interrupt for this GPIO block */
	switch (dev->id) {
#ifdef CONFIG_SOC_GPIO_32
	case SOC_GPIO_32_ID:
		irq_connect_dynamic(gpio_dev->vector, ISR_DEFAULT_PRIO,
				    gpio_isr, NULL,
				    0);
		break;
#endif
#ifdef CONFIG_SOC_GPIO_AON
	case SOC_GPIO_AON_ID:
		irq_connect_dynamic(gpio_dev->vector, ISR_DEFAULT_PRIO,
				    gpio_aon_isr, NULL,
				    0);
		break;
#endif
	default:
		pr_debug(LOG_MODULE_DRV, "init unknown ss gpio %d", dev->id);
		return DRV_RC_FAIL;
	}
	irq_enable(gpio_dev->vector);
	/* Enable GPIO  Interrupt into SSS  */
	SOC_UNMASK_INTERRUPTS(gpio_dev->gpio_int_mask);

	return 0;
}

static int soc_gpio_suspend(struct td_device *dev, PM_POWERSTATE state)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	/* Save Config */
	gpio_dev->pm_context.gpio_type =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_SWPORTA_DDR);
	gpio_dev->pm_context.int_type =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_INTTYPE_LEVEL);
	gpio_dev->pm_context.int_polarity =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_INTPOLARITY);
	gpio_dev->pm_context.int_debounce =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_DEBOUNCE);
	gpio_dev->pm_context.int_ls_sync =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_LS_SYNC);
	gpio_dev->pm_context.int_bothedge =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_INT_BOTHEDGE);
	gpio_dev->pm_context.output_values =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_SWPORTA_DR);
	gpio_dev->pm_context.mask_interrupt =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_INTMASK);
	gpio_dev->pm_context.mask_inten =
		GPIO_REG(gpio_dev,
			 SOC_GPIO_INTEN);

	pr_debug(LOG_MODULE_DRV, "gpio suspend device %d (%d)", dev->id,
		 state);

	return DRV_RC_OK;
}

static int soc_gpio_resume(struct td_device *dev)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	GPIO_REG(gpio_dev, SOC_GPIO_INTTYPE_LEVEL) =
		gpio_dev->pm_context.int_type;
	GPIO_REG(gpio_dev, SOC_GPIO_INTPOLARITY) =
		gpio_dev->pm_context.int_polarity;
	GPIO_REG(gpio_dev, SOC_GPIO_DEBOUNCE) =
		gpio_dev->pm_context.int_debounce;
	GPIO_REG(gpio_dev, SOC_GPIO_LS_SYNC) =
		gpio_dev->pm_context.int_ls_sync;
	GPIO_REG(gpio_dev, SOC_GPIO_INT_BOTHEDGE) =
		gpio_dev->pm_context.int_bothedge;
	GPIO_REG(gpio_dev, SOC_GPIO_INTMASK) =
		gpio_dev->pm_context.mask_interrupt;
	GPIO_REG(gpio_dev, SOC_GPIO_INTEN) =
		gpio_dev->pm_context.mask_inten;
	GPIO_REG(gpio_dev, SOC_GPIO_SWPORTA_DR) =
		gpio_dev->pm_context.output_values;
	GPIO_REG(gpio_dev, SOC_GPIO_SWPORTA_DDR) =
		gpio_dev->pm_context.gpio_type;

	/* enable interrupt for this GPIO block */
	switch (dev->id) {
#ifdef CONFIG_SOC_GPIO_32
	case SOC_GPIO_32_ID:
		irq_connect_dynamic(gpio_dev->vector, ISR_DEFAULT_PRIO,
				    gpio_isr, NULL,
				    0);
		break;
#endif
#ifdef CONFIG_SOC_GPIO_AON
	case SOC_GPIO_AON_ID:
		irq_connect_dynamic(gpio_dev->vector, ISR_DEFAULT_PRIO,
				    gpio_aon_isr, NULL,
				    0);
		break;
#endif
	default:
		pr_debug(LOG_MODULE_DRV, "init unknown ss gpio %d", dev->id);
		return DRV_RC_FAIL;
	}
	irq_enable(gpio_dev->vector);
	/* Enable GPIO  Interrupt into SSS  */
	SOC_UNMASK_INTERRUPTS(gpio_dev->gpio_int_mask);

	pr_debug(LOG_MODULE_DRV, "gpio resume device %d", dev->id);
	return DRV_RC_OK;
}

const struct driver soc_gpio_driver = {
	.init = soc_gpio_init,
	.suspend = soc_gpio_suspend,
	.resume = soc_gpio_resume
};


static void *soc_gpio_get_callback_arg(struct td_device *dev, uint8_t pin)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	if (pin >= gpio_dev->no_bits) {
		return NULL;
	}

	return gpio_dev->gpio_cb_arg[pin];
}

static DRIVER_API_RC soc_gpio_set_config(struct td_device *dev, uint8_t bit,
					 gpio_cfg_data_t *config)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;
	uint32_t saved;

	// Check pin index
	if (bit >= gpio_dev->no_bits) {
		return DRV_RC_CONTROLLER_NOT_ACCESSIBLE;
	}

	// Validate config data
	if ((config->gpio_type != GPIO_INPUT) &&
	    (config->gpio_type != GPIO_OUTPUT) &&
	    (config->gpio_type != GPIO_INTERRUPT)) {
		return DRV_RC_INVALID_CONFIG;
	}

	// Check listen configuration
	if (config->gpio_type == GPIO_INTERRUPT) {
		if (gpio_dev->gpio_cb[bit] != NULL) {
			// pin is already in use
			return DRV_RC_CONTROLLER_IN_USE;
		}
		if ((config->int_type != LEVEL) &&
		    (config->int_type != EDGE) &&
		    (config->int_type != DOUBLE_EDGE)) {
			// invalid config for listen
			return DRV_RC_INVALID_CONFIG;
		}
	}

	/* Disable Interrupts from this bit */
	GPIO_REG(gpio_dev, SOC_GPIO_INTEN) &= ~(1 << bit);

	// Set interrupt handler to NULL
	gpio_dev->gpio_cb[bit] = NULL;
	gpio_dev->gpio_cb_arg[bit] = NULL;

	switch (config->gpio_type) {
	case GPIO_INPUT:
		/* configure as input */
		CLEAR_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
						     SOC_GPIO_SWPORTA_DDR),
			       (uint32_t)bit);
		break;
	case GPIO_OUTPUT:
		/* configure as output */
		SET_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
						   SOC_GPIO_SWPORTA_DDR),
			     (uint32_t)bit);
		break;
	case GPIO_INTERRUPT:
		saved = irq_lock();
		// Configure interrupt handler
		gpio_dev->gpio_cb[bit] = config->gpio_cb;
		gpio_dev->gpio_cb_arg[bit] = config->gpio_cb_arg;
		GPIO_REG(gpio_dev, SOC_GPIO_SWPORTA_DDR) &= ~(1 << bit);

		/* Set Level, Edge or Double Edge */
		if (config->int_type == LEVEL) {
			GPIO_REG(gpio_dev,
				 SOC_GPIO_INTTYPE_LEVEL) &= ~(1 << bit);
		} else if (config->int_type == EDGE) {
			GPIO_REG(gpio_dev, SOC_GPIO_INTTYPE_LEVEL) |= (1 << bit);
			GPIO_REG(gpio_dev, SOC_GPIO_INT_BOTHEDGE) &= ~(1 << bit);
		} else { // DOUBLE_EDGE
			GPIO_REG(gpio_dev, SOC_GPIO_INTTYPE_LEVEL) |= (1 << bit);
			GPIO_REG(gpio_dev, SOC_GPIO_INT_BOTHEDGE) |= (1 << bit);
		}

		/* Set Polarity - Active Low / High */
		if (ACTIVE_LOW == config->int_polarity) {
			GPIO_REG(gpio_dev, SOC_GPIO_INTPOLARITY) &= ~(1 << bit);
		} else {
			GPIO_REG(gpio_dev, SOC_GPIO_INTPOLARITY) |= (1 << bit);
		}

		/* Set Debounce - On / Off */
		if (config->int_debounce == DEBOUNCE_OFF) {
			GPIO_REG(gpio_dev, SOC_GPIO_DEBOUNCE) &= ~(1 << bit);
		} else {
			GPIO_REG(gpio_dev, SOC_GPIO_DEBOUNCE) |= (1 << bit);
		}

		/* Enable as Interrupt */
		GPIO_REG(gpio_dev, SOC_GPIO_INTEN) |= (1 << bit);
		/* Unmask Interrupt */
		GPIO_REG(gpio_dev, SOC_GPIO_INTMASK) &= ~(1 << bit);

		irq_unlock(saved);
		break;
	}

	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_deconfig(struct td_device *dev, uint8_t bit)
{
	gpio_cfg_data_t config;

	// Default configuration (input pin without interrupt)
	config.gpio_type = GPIO_INPUT;
	config.int_type = EDGE;
	config.int_polarity = ACTIVE_LOW;
	config.int_debounce = DEBOUNCE_OFF;
	config.int_ls_sync = LS_SYNC_OFF;
	config.gpio_cb = NULL;
	config.gpio_cb_arg = NULL;

	return soc_gpio_set_config(dev, bit, &config);
}

static DRIVER_API_RC soc_gpio_write(struct td_device *dev, uint8_t bit,
				    bool value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	// Check pin index
	if (bit >= gpio_dev->no_bits) {
		return DRV_RC_CONTROLLER_NOT_ACCESSIBLE;
	}
	/* read/modify/write bit */
	if (value) {
		SET_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
						   SOC_GPIO_SWPORTA_DR),
			     (uint32_t)bit);
	} else {
		CLEAR_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
						     SOC_GPIO_SWPORTA_DR),
			       (uint32_t)bit);
	}

	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_write_port(struct td_device *dev, uint32_t value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	GPIO_REG(gpio_dev, SOC_GPIO_SWPORTA_DR) = value;
	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_read(struct td_device *dev, uint8_t bit,
				   bool *value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	// Check pin index
	if (bit >= gpio_dev->no_bits) {
		return DRV_RC_CONTROLLER_NOT_ACCESSIBLE;
	}
	if (GPIO_REG(gpio_dev, SOC_GPIO_SWPORTA_DDR) & (1 << bit)) {
		return DRV_RC_INVALID_OPERATION;  /* not configured as input */
	}

	*value = !!(GPIO_REG(gpio_dev, SOC_GPIO_EXT_PORTA) & (1 << bit));
	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_read_port(struct td_device *dev, uint32_t *value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	*value = GPIO_REG(gpio_dev, SOC_GPIO_EXT_PORTA);
	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_mask_interrupt(struct td_device *dev, uint8_t bit)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	SET_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
					   SOC_GPIO_INTMASK), (uint32_t)bit);
	return DRV_RC_OK;
}

static DRIVER_API_RC soc_gpio_unmask_interrupt(struct td_device *	dev,
					       uint8_t			bit)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	CLEAR_MMIO_BIT((volatile uint32_t *)(gpio_dev->reg_base +
					     SOC_GPIO_INTMASK), (uint32_t)bit);
	return DRV_RC_OK;
}

const struct gpio_driver gpio_device_driver_soc = {
	.set_config = soc_gpio_set_config,
	.deconfig = soc_gpio_deconfig,
	.get_callback_arg = soc_gpio_get_callback_arg,
	.write = soc_gpio_write,
	.write_port = soc_gpio_write_port,
	.read = soc_gpio_read,
	.read_port = soc_gpio_read_port,
	.mask_interrupt = soc_gpio_mask_interrupt,
	.unmask_interrupt = soc_gpio_unmask_interrupt
};

static void soc_gpio_ISR_proc(struct td_device *dev)
{
	unsigned int i;
	struct gpio_port *port = (struct gpio_port *)dev->priv;
	struct gpio_info_struct *gpio_dev =
		(struct gpio_info_struct *)port->config;

	// Save interrupt status
	uint32_t status = GPIO_REG(gpio_dev, SOC_GPIO_INTSTATUS);

	// Clear interrupt flag (write 1 to clear)
	GPIO_REG(gpio_dev, SOC_GPIO_PORTA_EOI) = status;

	for (i = 0; i < gpio_dev->no_bits; i++) {
		if ((status & (1 << i))) {
			if (gpio_dev->gpio_cb[i]) {
				(gpio_dev->gpio_cb[i])(
					!!(GPIO_REG(gpio_dev,
						    SOC_GPIO_EXT_PORTA) &
					   (1 << i)),
					gpio_dev->gpio_cb_arg[i]);
			} else {
				assert(dev->powerstate == PM_NOT_INIT);
				pr_info(LOG_MODULE_DRV,
					"spurious isr: %d on %d", i,
					dev->id);
				soc_gpio_mask_interrupt(dev, i);
			}
		}
	}
}
