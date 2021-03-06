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

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "cfw/cfw.h"

/**
 * Press events generated by the button driver.
 */
#define SINGLE_PRESS   (1 << 0) /*!< id returned when user performs a single press */
#define DOUBLE_PRESS   (1 << 1) /*!< id returned when user performs a double press */
#define MAX_PRESS      (1 << 2) /*!< id returned when user performs a press longer than the max press time */
#ifdef CONFIG_BUTTON_MULTIPLE_PRESS
#define MULTIPLE_PRESS (1 << 3) /*!< id returned when at least two buttons are press in the same time */
#endif
#ifdef CONFIG_BUTTON_FEEDBACK
#define FEEDBACK_EVT   (1 << 4) /*!< id returned when a feedback event is generated */
#endif

/**
 * Press duration configuration in millisecond.
 */
struct button_time_setting {
	uint16_t double_press_time; /*!< max time allowed of the first press duration in case of double press */
	uint16_t max_time;          /*!< above this duration a max_time event is sent */
};

struct button;

/**
 * Enable button.
 * @param  button button parameters and configuration
 * @param  button_count number of buttons to use
 * @param  cb function to call when a button event is detected
 * @return none
 */
int button_init(struct button *button, uint8_t button_count, void (*cb)(
			uint8_t button_id, uint8_t event, uint32_t param));

/**
 * Disable the button module
 * @return none
 */
void button_shutdown();

/**
 * Global struct defining button parameters and internal variables.
 */
struct button {
	struct button_time_setting timing;  /*!< time setting to customize the press duration */
	int (*init)(struct button *);       /*!< init hook for button */
	void (*shutdown)(struct button *);   /*!< shutdown hook for button */
	void *priv;                         /*!< wrapper private data */
	uint8_t press_mask;                 /*!< bit mask for events returned by the button module
	                                     *  (SINGLE_PRESS | DOUBLE_PRESS ...) */
	uint8_t action_count;               /*!< number of actions muxing for button */
	uint16_t *action_time;              /*!< timing array for the button actions (absolute time in ms) */
	/* Internal fields */
#ifdef CONFIG_BUTTON_FEEDBACK
	uint8_t action_index;
#endif
	volatile uint8_t state;             /*!< internal varialble to get the new state of the gpio pin */
	void (*button_cb)(uint8_t, uint8_t, uint32_t); /*!< callback function for power button event notification */
	volatile uint32_t time;             /*!< date of the last gpio state changement */
	T_TIMER timer;                      /*!< internal timer used for double/max press and feedback */
};

/**
 * Get button current state.
 * @return bitfield of pressed buttons
 */
uint32_t button_get_states();

/**
 * Update button event mask.
 * @param button_id button id to use
 * @param press_mask new press event mask to use
 * @return none
 */
void button_set_press_mask(uint8_t button_id, uint8_t press_mask);

/**
 * Report gpio events to button module.
 * @param button button device to use
 * @param state button state (true=pressed, false=released)
 * @param timestamp timestamp from the get_uptime_ms function
 * @return none
 */
void button_gpio_event(struct button *button, bool state, uint32_t timestamp);

#endif /* BUTTON_H */
