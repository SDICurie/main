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

#ifndef GPIO_IFACE_H_
#define GPIO_IFACE_H_

#include <stdbool.h>
#include "drivers/data_type.h"
#include "infra/device.h"

/**
 * @defgroup common_driver_gpio GPIO Driver
 * GPIO Driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/gpio.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/gpio</tt>
 * <tr><th><b>Config flag</b> <td><tt>SOC_GPIO | SS_GPIO</tt>
 * </table>
 *
 * The GPIO Driver can manage any GPIO in a generic manner.
 * There is one GPIO Driver for Quark (flag SOC_GPIO) and
 * one GPIO Driver for ARC (flag SS_GPIO) but they offer the same API.
 *
 * - \b gpio_set_config configures a GPIO<br>
 *   `DRIVER_API_RC gpio_set_config (struct td_device *dev, uint8_t bit, gpio_cfg_data_t *config)`
 * - \b gpio_write sets a GPIO level<br>
 *   `DRIVER_API_RC gpio_write (struct td_device *dev, uint8_t bit, bool value)`
 * - \b gpio_read gets a GPIO state<br>
 *   `DRIVER_API_RC gpio_read (struct td_device *dev, uint8_t bit, bool *value)`
 * - \b gpio_write_port sets the level for all the GPIOs of the port<br>
 *   `DRIVER_API_RC gpio_write_port (struct td_device *dev, uint32_t value)`
 * - \b gpio_read_port gets the level for all the GPIOs of the port<br>
 *   `DRIVER_API_RC gpio_read_port (struct td_device *dev, uint32_t *value)`
 * - \b gpio_mask_interrupt/gpio_unmask_interrupt masks/unmasks the interrupt for a GPIO<br>
 *   `DRIVER_API_RC gpio_mask_interrupt (struct td_device *dev, uint8_t bit)`<br>
 *   `DRIVER_API_RC gpio_unmask_interrupt (struct td_device *dev, uint8_t bit)`
 * - \b gpio_deconfig unconfigures a GPIO<br>
 *   `DRIVER_API_RC gpio_deconfig (struct td_device *dev, uint8_t bit)`
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 *  GPIO callback function.
 * @param state Pin state
 * @param priv Private data provided on callback registration
 */
typedef void (*gpio_callback_fn)(bool state, void *priv);

/**
 *  GPIO context structure for deep sleep recovery.
 *  This structure is used internally.
 */
struct gpio_pm_ctxt {
	uint32_t gpio_type;     /*!< Set port type (0=INPUT, 1=OUTPUT) */
	uint32_t int_type;      /*!< Sets interrupt type (0=LEVEL, 1=EDGE) */
	uint32_t int_bothedge;  /*!< Enable interrupt on both rising and falling edges (0=OFF, 1=ON) */
	uint32_t int_polarity;  /*!< GPIO polarity configuration (0=LOW, 1=HIGH) */
	uint32_t int_debounce;  /*!< GPIO debounce configuration (0=OFF, 1=ON) */
	uint32_t int_ls_sync;   /*!< GPIO ls sync configuration (0=OFF, 1=ON) */
	uint32_t output_values; /*!< Write values */
	uint32_t mask_interrupt; /*!< Mask interrupt register value */
	uint32_t mask_inten;    /*!< Mask inten register value */
};

/**
 * GPIO management structure.
 * This structure is used to properly instanciate a GPIO device.
 */
typedef struct gpio_info_struct {
	/* Static settings */
	uint32_t reg_base;                 /*!< Base address of device register set */
	uint8_t no_bits;                   /*!< Nb of gpio bits in this entity */
	uint8_t vector;                    /*!< GPIO ISR vector */
	uint32_t gpio_int_mask;            /*!< SSS Interrupt Routing Mask Registers */
	gpio_callback_fn *gpio_cb;         /*!< Array of user callback functions for user */
	void **gpio_cb_arg;                /*!< Array of user priv data for callbacks */
	uint8_t is_init;                   /*!< Init state of GPIO port */
	struct gpio_pm_ctxt pm_context;    /*!< Context for deep sleep recovery */
} gpio_info_t;

/**
 * GPIO types.
 */
typedef enum {
	GPIO_INPUT,  /*!< Configure GPIO pin as input */
	GPIO_OUTPUT, /*!< Configure GPIO pin as output */
	GPIO_INTERRUPT /*!< Configure GPIO pin as interrupt */
} GPIO_TYPE;

/**
 * Interrupt types.
 */
typedef enum {
	LEVEL,       /*!< Configure an interrupt triggered on level */
	EDGE,        /*!< Configure an interrupt triggered on single edge */
	DOUBLE_EDGE  /*!< Configure an interrupt triggered on both rising and falling edges */
} INT_TYPE;

/**
 * Polarity configuration for interrupts.
 */
typedef enum {
	ACTIVE_LOW,  /*!< Configure an interrupt on low level or falling edge */
	ACTIVE_HIGH  /*!< Configure an interrupt on high level or rising edge */
} INT_POLARITY;

/**
 * Debounce configuration for interrupts.
 */
typedef enum {
	DEBOUNCE_OFF, /*!< Disable debounce for interrupt */
	DEBOUNCE_ON  /*!< Enable debounce for interrupt */
} INT_DEBOUNCE;

/**
 * ls_sync configuration for interrupts
 */
typedef enum {
	LS_SYNC_OFF, /*!< Disable ls sync for interrupt */
	LS_SYNC_ON   /*!< Enable ls sync for interrupt */
} INT_LS_SYNC;

/**
 * GPIO configuration structure
 */
typedef struct gpio_cfg_data {
	GPIO_TYPE gpio_type;             /*!< GPIO type */
	INT_TYPE int_type;               /*!< GPIO interrupt type */
	INT_POLARITY int_polarity;       /*!< GPIO polarity configuration */
	INT_DEBOUNCE int_debounce;       /*!< GPIO debounce configuration */
	INT_LS_SYNC int_ls_sync;         /*!< GPIO ls sync configuration */
	gpio_callback_fn gpio_cb;        /*!< Callback function called when an interrupt is triggered on this pin */
	void *gpio_cb_arg;               /*!< Data passed as an argument for the callback function */
} gpio_cfg_data_t;

/**
 * GPIO standard interface. It is used for accessing GPIO devices in a generic
 * way. No matter what port on what side (either Quark or Arc), the call is
 * automatically redirected to the right driver handler.
 */
struct gpio_driver {
	/**
	 * Configure specified GPIO bit in specified GPIO port.
	 *
	 * Configuration parameters must be valid or an error is returned - see return values below.
	 *
	 * @param dev              GPIO device to use
	 * @param bit              Bit in port to configure
	 * @param config           Pointer to configuration structure
	 *
	 * @return
	 *          - DRV_RC_OK                             on success
	 *          - DRV_RC_DEVICE_TYPE_NOT_SUPPORTED      if port id is not supported by this controller
	 *          - DRV_RC_INVALID_CONFIG                 if any configuration parameter is not valid
	 *          - DRV_RC_CONTROLLER_IN_USE              if port/bit is in use
	 *          - DRV_RC_CONTROLLER_NOT_ACCESSIBLE      if port/bit is not accessible from this core
	 *          - DRV_RC_FAIL                           otherwise
	 */
	DRIVER_API_RC (*set_config)(struct td_device *dev, uint8_t bit,
				    gpio_cfg_data_t *config);

	/**
	 * Deconfigure specified GPIO bit in specified GPIO port
	 *
	 * Configuration parameters must be valid or an error is returned - see return values below.
	 *
	 * @param dev             : GPIO device to use
	 * @param bit             : Bit in port to deconfigure
	 *
	 * @return
	 *          - DRV_RC_OK                            on success
	 *          - DRV_RC_DEVICE_TYPE_NOT_SUPPORTED     if port id is not supported by this controller
	 *          - DRV_RC_INVALID_CONFIG                if any configuration parameter is not valid
	 *          - DRV_RC_CONTROLLER_IN_USE,            if port/bit is in use
	 *          - DRV_RC_CONTROLLER_NOT_ACCESSIBLE     if port/bit is not accessible from this core
	 *          - DRV_RC_FAIL                          otherwise
	 */
	DRIVER_API_RC (*deconfig)(struct td_device *dev, uint8_t bit);

	/**
	 * Get callback argument pointer for a specific pin
	 *
	 * @param dev              GPIO device to use
	 * @param pin              Pin in port to configure
	 *
	 * @return  Pointer if success else NULL
	 */
	void * (*get_callback_arg)(struct td_device *dev, uint8_t pin);

	/**
	 * Set output value on the gpio bit
	 *
	 * @param dev              GPIO device to use
	 * @param bit              Bit in port to configure
	 * @param value            Value to write to bit
	 *
	 * @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*write)(struct td_device *dev, uint8_t bit, bool value);

	/**
	 * Write a value to a given port
	 *
	 * @param dev              GPIO device to use
	 * @param value            Value to write to port
	 *
	 * @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*write_port)(struct td_device *dev, uint32_t value);

	/**
	 * Read a GPIO bit
	 *
	 * @param dev              GPIO device to use
	 * @param bit              Bit in port to configure
	 * @param value            Address where to return the read value
	 *
	 * @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*read)(struct td_device *dev, uint8_t bit, bool *value);

	/**
	 * Read a given port
	 *
	 * @param dev              GPIO device to use
	 * @param value            Address where to return the read value
	 *
	 * @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*read_port)(struct td_device *dev, uint32_t *value);

	/**
	 *  Mask the interrupt of a GPIO bit
	 *
	 *  @param dev              GPIO device to use
	 *  @param bit              Bit in port to configure
	 *
	 *  @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*mask_interrupt)(struct td_device *dev, uint8_t bit);

	/**
	 *  Unmask the interrupt of a GPIO bit
	 *
	 *  @param dev              GPIO device to use
	 *  @param bit              Bit in port to configure
	 *
	 *  @return
	 *          - DRV_RC_OK on success
	 *          - DRV_RC_FAIL otherwise
	 */
	DRIVER_API_RC (*unmask_interrupt)(struct td_device *dev, uint8_t bit);
};

/**
 * This structure defines the content of the "private" field within a GPIO
 * td_device. It describes the device configuration and the IO interface
 * used for accessing the device.
 */
struct gpio_port {
	struct gpio_info_struct *config;
	struct gpio_driver *api;
};

static inline DRIVER_API_RC gpio_set_config(struct td_device *dev, uint8_t bit,
					    gpio_cfg_data_t *config)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->set_config(dev, bit, config);
}
static inline DRIVER_API_RC gpio_deconfig(struct td_device *dev, uint8_t bit)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->deconfig(dev, bit);
}
static inline void *gpio_get_callback_arg(struct td_device *dev, uint8_t pin)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->get_callback_arg(dev, pin);
}
static inline DRIVER_API_RC gpio_write(struct td_device *dev, uint8_t bit,
				       bool value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->write(dev, bit, value);
}
static inline DRIVER_API_RC gpio_write_port(struct td_device *	dev,
					    uint32_t		value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->write_port(dev, value);
}
static inline DRIVER_API_RC gpio_read(struct td_device *dev, uint8_t bit,
				      bool *value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->read(dev, bit, value);
}
static inline DRIVER_API_RC gpio_read_port(struct td_device *	dev,
					   uint32_t *		value)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->read_port(dev, value);
}
static inline DRIVER_API_RC gpio_mask_interrupt(struct td_device *	dev,
						uint8_t			bit)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->mask_interrupt(dev, bit);
}
static inline DRIVER_API_RC gpio_unmask_interrupt(struct td_device *	dev,
						  uint8_t		bit)
{
	struct gpio_port *port = (struct gpio_port *)dev->priv;

	return port->api->unmask_interrupt(dev, bit);
}

/** @} */

#endif  /* GPIO_IFACE_H_ */
