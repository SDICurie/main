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

#include <errno.h>

#include "os/os.h"
#include "infra/device.h"
#include "infra/log.h"
#include "infra/time.h"
#include "drivers/usb_pm.h"
#include "drivers/usb_api.h"
#include "drivers/gpio.h"
#include "drivers/soc_comparator.h"
#include "machine.h"
#include "drivers/usb_acm.h"

struct usb_pm_cb_list;

struct usb_pm_cb_list {
	list_t next;
	void (*cb)(bool, void *);
	void *priv;
};

/* Timer definition */
static T_TIMER usb_debounce_timer;
static bool usb_deb_timer_started = false;
#define USB_DB_DELAY_50ms (50)

static int usb_pm_init(struct td_device *dev);
static int usb_pm_suspend(struct td_device *dev, PM_POWERSTATE state);
static int usb_pm_resume(struct td_device *dev);
static void usb_comp_callback(void *priv_data);

static void call_user_callback(void *item, void *param)
{
	((struct usb_pm_cb_list *)item)->cb(
		(bool)param,
		((struct usb_pm_cb_list *)item)->
		priv);
}

#ifdef CONFIG_USB

#define OSC_INTERNAL 1
#define OSC_EXTERNAL 0
#define INTERNAL_OSC_TRIM 0x240

/* get_uptime_32k returns always-on counter value running off 32KHz RTC clock */
static void delay_us(uint32_t us)
{
	int timeout = get_uptime_32k() + (us + 30) / 30;

	while (get_uptime_32k() < timeout) ;
}

void set_oscillator(int internal)
{
	if (internal) {
		/* Start internal oscillator (with trim) */
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE, SCSS_OSC0_CFG1) |=
			INTERNAL_OSC_TRIM << OSC0_CFG1_INTERNAL_OSC_TRIM_BIT |
			OSC0_CFG1_INTERNAL_OSC_EN_MASK;
		/* Wait internal oscillator ready */
		while (!((MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
						 SCSS_OSC0_STAT1) &
			  OSC0_STAT1_LOCK_INTERNAL)))
			;
		/* Trim internal oscillator */
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE, SCSS_OSC0_CFG1) =
			INTERNAL_OSC_TRIM << OSC0_CFG1_INTERNAL_OSC_TRIM_BIT |
			OSC0_CFG1_INTERNAL_OSC_EN_MASK;
	} else {
		/* Set clk to 32MHz, external oscillator, 5.5pF load */
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE, SCSS_OSC0_CFG1) |=
			OSC0_CFG1_XTAL_OSC_TRIM_5_55_PF |
			OSC0_CFG1_XTAL_OSC_EN_MASK;
		/* Wait internal regulator ready */
		while (!((MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
						 SCSS_OSC0_STAT1) &
			  OSC0_STAT1_LOCK_XTAL)))
			;
		/* Switch to external oscillator */
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE, SCSS_OSC0_CFG1) =
			OSC0_CFG1_XTAL_OSC_TRIM_5_55_PF |
			(OSC0_CFG1_XTAL_OSC_EN_MASK |
			 OSC0_CFG1_XTAL_OSC_OUT_MASK);
	}
}

void enable_vusb_regulator(struct usb_pm_info *usb_pm, bool enable)
{
	gpio_cfg_data_t config;
	struct td_device *gpiodev = usb_pm->vusb_enable_dev;

	if (gpiodev) {
		config.gpio_type = GPIO_OUTPUT;
		gpio_set_config(gpiodev, usb_pm->vusb_enable_pin, &config);
		gpio_write(gpiodev, usb_pm->vusb_enable_pin, enable);

		/* We need to wait some time here for USB voltage regulator to stabilize */
		delay_us(90);
	} else {
		pr_warning(LOG_MODULE_USB,
			   "USB Voltage regulator enable not supported");
	}
}
#endif

/* Soc Specific initialization */
static inline void usb_generic_callback(struct td_device *dev)
{
	struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;

	// Toggle USB status
	priv->is_plugged = !priv->is_plugged;
	if (priv->is_plugged) {
		pr_info(LOG_MODULE_USB, "USB plugged");
		pm_wakelock_acquire(&priv->usb_pm_wakelock);
	} else {
		pr_info(LOG_MODULE_USB, "USB unplugged");
		pm_wakelock_release(&priv->usb_pm_wakelock);
	}

#ifdef CONFIG_USB
#ifdef CONFIG_QUARK_SE_SWITCH_INTERNAL_OSCILLATOR
	set_oscillator(priv->is_plugged ? OSC_EXTERNAL : OSC_INTERNAL);
#endif
	enable_vusb_regulator(priv, priv->is_plugged);
#endif
	/* Call user callbacks */
	list_foreach(&priv->cb_head, call_user_callback,
		     (void *)priv->is_plugged);
}

static void usb_debounce_timer_handler(void *priv_data)
{
	struct td_device *dev = (struct td_device *)priv_data;
	struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;

	usb_deb_timer_started = false;
	usb_generic_callback(dev);

	// Configure comparator as USB wake up source
	int ret = comp_configure(priv->evt_dev, priv->source_pin,
				 priv->is_plugged ? 1 : 0, 1, usb_comp_callback,
				 priv_data);

	if (ret != 0) {
		pr_error(LOG_MODULE_DRV, "usb_pm: cb config failed (%d)", ret);
	}
}

static void usb_comp_callback(void *priv_data)
{
	if (usb_deb_timer_started == true)
		return;
	else {
		usb_deb_timer_started = true;
		timer_start(usb_debounce_timer, USB_DB_DELAY_50ms, NULL);
	}
}

static int usb_pm_init(struct td_device *dev)
{
	struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;

	pm_wakelock_init(&priv->usb_pm_wakelock);
	usb_debounce_timer =
		timer_create(usb_debounce_timer_handler, dev, USB_DB_DELAY_50ms,
			     false, false,
			     NULL);
	usb_deb_timer_started = false;
	priv->is_plugged = false;
	list_init(&priv->cb_head);

	if (!priv->evt_dev) {
		return -EINVAL;
	}
	return usb_pm_resume(dev);
}

bool usb_pm_is_plugged(struct td_device *dev)
{
	struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;

	return priv->is_plugged;
}

int usb_pm_register_callback(struct td_device *dev, void (*cb)(bool, void *),
			     void *priv)
{
	struct usb_pm_info *usb_dev = (struct usb_pm_info *)dev->priv;

	// Alloc memory for cb and priv argument
	struct usb_pm_cb_list *item =
		(struct usb_pm_cb_list *)balloc(sizeof(struct usb_pm_cb_list),
						NULL);

	// Fill new allocated item
	item->priv = priv;
	item->cb = cb;
	// Add item in list
	list_add_head(&usb_dev->cb_head, (list_t *)item);
	return 0;
}

static bool check_item_callback(list_t *item, void *cb)
{
	return ((struct usb_pm_cb_list *)item)->cb == cb;
}

int usb_pm_unregister_callback(struct td_device *dev, void (*cb)(bool, void *))
{
	struct usb_pm_info *usb_dev = (struct usb_pm_info *)dev->priv;

	list_t *element =
		list_find_first(&usb_dev->cb_head, check_item_callback,
				(void *)cb);

	if (element == NULL) {
		// element not found
		return -1;
	}
	// Remove list element
	list_remove(&usb_dev->cb_head, element);
	bfree(element);

	return 0;
}

static int usb_pm_suspend(struct td_device *dev, PM_POWERSTATE state)
{
	struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;

	pr_debug(LOG_MODULE_DRV, "usb_pm: %d suspend", dev->id);
	if (state == PM_SHUTDOWN) {
		pm_wakelock_release(&priv->usb_pm_wakelock);
	}
	return 0;
}

static int usb_pm_resume(struct td_device *dev)
{
	if (((struct usb_pm_info *)(dev->priv))->interrupt_source ==
	    USB_COMPARATOR_IRQ_SOURCE) {
		int ret = 0;
		struct usb_pm_info *priv = (struct usb_pm_info *)dev->priv;
		// Configure comparator as USB wake up source
		ret = comp_configure(priv->evt_dev, priv->source_pin,
				     priv->is_plugged ? 1 : 0, 1,
				     usb_comp_callback,
				     dev);
		pr_debug(LOG_MODULE_DRV, "device %s: use comp %d - %d (%d)",
			 dev->id,
			 priv->interrupt_source, priv->source_pin,
			 ret);
		return ret;
	}

	pr_debug(LOG_MODULE_DRV, "%d: no irq source", dev->id);
	return -1;
}

struct driver usb_pm_driver = {
	.init = usb_pm_init,
	.suspend = usb_pm_suspend,
	.resume = usb_pm_resume
};
