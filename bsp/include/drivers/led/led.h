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

#ifndef LED_H_
#define LED_H_

#include <stdint.h>

/**
 * @defgroup led_drivers Led Driver
 * Led Driver API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/led/led.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/led</tt>
 * <tr><th><b>Config flag</b> <td><tt>GPIO_LED | SOC_LED | LP5562_LED</tt>
 * </table>
 *
 * The LED driver provides a generic API to manage LED patterns.
 *
 * @note
 *    The preferred way to play patterns is to use the UI service (see \ref ui_service).
 *
 * @{
 * @ingroup ext_drivers
 *
 */

#define LED_FIXED_INFINITE_DURATION 0
#define LED_REPETITION_CONTINUOUS 255

/**
 * Led type defines the pattern to perform.
 * TODO: Fix the API so the meaning of the type corresponds to what is actually
 * done.
 */
typedef enum led_type {
	LED_NONE = 0x00,        /**< Turn off the LED */
	LED_BLINK_X1 = 0x01,    /**< Light the LED for the specified time */
	LED_BLINK_X2 = 0x02,    /**< Blink the LED twice */
	LED_BLINK_X3 = 0x03,    /**< Blink the LED three times */
	LED_WAVE_X1 = 0x04,     /**< Not implemented */
	LED_WAVE_X2 = 0X05,     /**< Not implemented */
	LED_WAVE_X3 = 0X06,     /**< Not implemented */
} LED_TYPE;

/**
 * RGB color type defines the led color.
 * 0 corresponds to "no color", 255 to "full color".
 */
#ifdef CONFIG_LED_MULTICOLOR
typedef struct {
	uint8_t r;      /**< Red color value */
	uint8_t g;      /**< Green color value */
	uint8_t b;      /**< Blue color value */
} rgb_t;
#endif

/**
 * Defines the duration the LED is ON and the duration it is OFF.
 * `duration_on` corresponds to the time the LED is lightened.
 * `duration_off` corresponds to the time between two blinks or two repetitions.
 */
typedef struct {
	uint16_t duration_on;   /**< Time the LED is ON in milliseconds */
	uint16_t duration_off;  /**< Time the LED is OFF in milliseconds */
} duration_t;

/**
 * Generic pattern parameters structure.
 */
typedef struct led_s {
	uint8_t id;                             /**< LED number */
	uint8_t repetition_count;               /**< Number of times the pattern is repeted.
	                                         * Put 0 to play pattern only once.
	                                         * Put LED_REPETITION_CONTINUOUS for continuous mode */
	uint8_t intensity;                      /**< Light intensity. Put 0 for minimum intensity,
	                                         * 255 for maximum intensity */
	duration_t duration[3];                 /**< Duration in ms for each blink */
#ifdef CONFIG_LED_MULTICOLOR
	rgb_t rgb[3];                           /**< LED color for each blink */
#endif
} led_s;

/**
 * LED configuration structure. For each LED, a struct like this one must be
 * filled and provided at initialization.
 */
struct led {
	uint8_t id;     /* Led identification number */
	void *priv;     /* Private field specific to each driver */
};

/** Set the callback to return result of played LED.
 *  Pointer to the callback have to be provided to the driver at
 *  init of the service.
 *
 *  @param   evt_callback_func Pointer to the callback
 *                             - 1rst parameter is the LED identifier
 *                             - 2nd parameter is the play result
 *  @param   led_config Pointer to the configuration led structure
 *  @param   led_count Number of elements in led_config
 *  @return  none
 */
void led_set_pattern_callback(void (*evt_callback_func)(uint8_t,
							uint8_t),
			      uint8_t led_count, struct led led_config[]);

/**
 * Request the led driver to play a specific pattern.
 *
 * @param  type Pattern selected
 * @param  pattern Pattern parameters
 * @param  ledNb LED identifier
 * @return error status
 *
 * @note
 *    To permanently turn on the LED, use LED_FIXED type and set the first
 *    duration to 0.
 *    This pattern is considered as a "Background" pattern. When another pattern
 *    is requested, this will:
 *    - turn off the LED
 *    - play the asked pattern
 *    - restart the first "LED_FIXED" pattern.
 *
 */
int8_t led_pattern_handler_config(enum led_type type, led_s *pattern,
				  uint8_t ledNb);

/**
 * @}
 */
#endif /* LED_H_ */
