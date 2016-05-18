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

#ifndef HAPTIC_H_
#define HAPTIC_H_

#include <stdint.h>
#include <device.h>

/**
 * @defgroup common_driver_haptic Haptic Driver
 * Haptic Driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/haptic.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/haptic</tt>
 * <tr><th><b>Config flag</b> <td><tt>DRV2605</tt>
 * </table>
 *
 * The Haptic driver allows to play vibration patterns.
 * The Haptic driver is an OS driver and device is defined through OS framework.
 *
 * @note
 *    The preferred way to play patterns is to use the UI service (see \ref ui_service).
 *
 * @ingroup ext_drivers
 * @{
 */

/**
 * Vibration type.
 * This type defines the pattern to perform.
 */
typedef enum vibration_type {
	VIBRATION_NONE,
	VIBRATION_SQUARE_X2,
	VIBRATION_SPECIAL_EFFECTS
}vibration_type;

/**
 * Vibration square_x2 pattern defines the way to switch on / switch off a vibrator
 * according to amplitude, times on, times off and repetition.
 * NB: time durations are in milli second.
 */
typedef struct vibration_square_x2_t {
	uint8_t amplitude;              /*!< Pulse amplitude. Depending on haptic
	                                 * device and board form factor, user may
	                                 * not feel the vibration if amplitude is
	                                 * set too low. */
	uint16_t duration_on_1;         /*!< First pulse duration on state */
	uint16_t duration_off_1;        /*!< First pulse duration off state */
	uint16_t duration_on_2;         /*!< Second pulse duration on state */
	uint16_t duration_off_2;        /*!< Second pulse duration off state */
	uint8_t repetition_count;       /*!< Number of repetitions */
} vibration_square_x2_t;

/**
 * Special effect pattern defines the way to switch on / switch off a vibrator
 * according to effects and durations between effects.
 * NB: time durations are in milli second.
 */
typedef struct vibration_special_effect_t {
	uint8_t effect_1;               /*!< First effect type */
	uint16_t duration_off_1;        /*!< Duration off after first effect */
	uint8_t effect_2;               /*!< Second effect type */
	uint16_t duration_off_2;        /*!< Duration off after second effect */
	uint8_t effect_3;               /*!< Third effect type */
	uint16_t duration_off_3;        /*!< Duration off after third effect */
	uint8_t effect_4;               /*!< Fourth effect type */
	uint16_t duration_off_4;        /*!< Duration off after fourth effect */
	uint8_t effect_5;               /*!< Fifth effect type */
} vibration_special_effect_t;

/**
 * Unified vibration pattern.
 */
typedef union vibration_u {
	vibration_square_x2_t square_x2;
	vibration_special_effect_t special_effect;
} vibration_u;


/**
 * Haptic driver interface
 */
struct haptic_config {
	void (*evt_callback_fn)(int8_t);
};

typedef void (*haptic_api_set_config)(struct device *		dev,
				      struct haptic_config *	cfg);
typedef int8_t (*haptic_api_play)(struct device *	dev,
				  vibration_type	type,
				  vibration_u *		pattern);

struct haptic_driver_api {
	haptic_api_set_config set_config;
	haptic_api_play play;
};

/** Set the callback to return result of played haptic pattern.
 *  Pointer to the callback have to be provided to the driver at
 *  init of the service.
 *
 *  @param   dev Device structure of the driver instance
 *  @param   cfg Haptic configuration
 *  @return  none
 */
inline void haptic_set_config(struct device *		dev,
			      struct haptic_config *	cfg)
{
	struct haptic_driver_api *api;

	api = (struct haptic_driver_api *)dev->driver_api;
	api->set_config(dev, cfg);
}

/**
 * Request the haptic driver to play a specific pattern.
 *
 * @param   dev Device structure of the driver instance
 * @param   type Pattern selected type
 * @param   pattern Pattern parameters
 * @return  none
 */
static inline void haptic_play(struct device *	dev,
			       vibration_type	type,
			       vibration_u *	pattern)
{
	struct haptic_driver_api *api;

	api = (struct haptic_driver_api *)dev->driver_api;
	api->play(dev, type, pattern);
}

/** @} */
#endif /* HAPTIC_H_ */
