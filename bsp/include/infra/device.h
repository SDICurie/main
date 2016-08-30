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

#ifndef __DEVICE_H__
#define __DEVICE_H__

/**
 * @defgroup infra_device Device model
 * Device driver tree management.
 * @ingroup drivers
 * @{
 */

#include "infra/pm.h"
#include "util/compiler.h"

/**
 * List of all device power states (From shutdown to running).
 */
typedef enum {
	PM_NOT_INIT = 0, /*!< Device not initialized */
	PM_SHUTDOWN, /*!< Device stopped */
	PM_SUSPENDED, /*!< Device suspended */
	PM_RUNNING,  /*!< Device working properly */
	PM_COUNT
} PM_POWERSTATE;

/* Structures pre-declaration */
struct td_device;
struct driver;

/**
 * Common API for all devices.
 */
struct __packed __aligned(4) td_device
{
	void *priv;                     /*!< Private data pointer */
	struct driver *driver;          /*!< Driver used for device */
	PM_POWERSTATE powerstate : 8;   /*!< Powerstate of device */
	uint8_t id;                     /*!< ID of device */
};

/**
 * Common API for all device drivers.
 */
struct driver {
	int (*init)(struct td_device *dev);                                /*!< Callback for device init */
	int (*suspend)(struct td_device *dev, PM_POWERSTATE state);        /*!< Callback for device suspend */
	int (*resume)(struct td_device *dev);                              /*!< Callback for device resume */
};

/**
 * Suspends all devices in the device tree of current CPU.
 *
 * This function calls the suspend callback of all devices in the device tree.
 * If a suspend callback fails for something else than shutdown, all devices are
 * resumed.
 *
 * @param state suspend type (for shutdown/deepsleep/sleep etc...).
 *
 * @return 0 if success else -1.
 */
int suspend_devices(PM_POWERSTATE state);

/**
 * Resumes all devices in the device tree of current CPU.
 *
 * This function calls the resume callback of all devices in the device tree.
 *
 * @attention It will panic if there is an error during initialization.
 */
void resume_devices(void);

/**
 * Adds all devices to power management infrastructure and init them.
 *
 * @param all_devices pointer to the platform devices array
 * @param all_devices_count size of the all_devices array
 *
 * @attention It will panic if there is an error during initialization.
 */
void init_devices(struct td_device **all_devices, uint32_t all_devices_count);

/**
 * Initializes all board devices.
 *
 * This function must be implemented externally for each BSP. It must call the
 * init_devices() function and pass the device tree for the platform.
 *
 * @attention It will panic if there is an error during initialization.
 */
void init_all_devices(void);

/**
 * Adds extended board devices to power management infrastructure and init them.
 *
 * @param board_devices pointer to the extended board devices array
 * @param boord_devices_count size of the board_devices array
 *
 * @attention It will panic if there is an error during initialization.
 */
void init_board_devices(struct td_device **	board_devices,
			uint32_t		board_devices_count);

/**
 * Initializes extended board devices.
 *
 * This function shall be implemented externally for a specific board. It must call the
 * init_board_devices() function and pass the device tree for the board.
 *
 * @attention It will panic if there is an error during initialization. The function is
 * optional and not necessarily implemented by all devices.
 */
void init_extended_devices(void);


/** @} */

#endif /* __DEVICE_H__ */
