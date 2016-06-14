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

#ifndef CLK_SYSTEM_H_
#define CLK_SYSTEM_H_

#include <stdint.h>

#define CLK_GATE_OFF (0)
#define CLK_GATE_ON  (~CLK_GATE_OFF)


/**
 * @defgroup clk_gate Clock Management Driver
 * Clock Management Driver API.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "drivers/clk_system.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/drivers/clk</tt>
 * <tr><th><b>Config flag</b> <td><tt>CLK_SYSTEM</tt>
 * </table>
 *
 * The Clock Gating Driver allows to dynamically enable/disable one or several clocks for
 * specific peripherals.
 * Disabling the clock when a peripheral is not in use saves power.
 *
 * @ingroup soc_drivers
 * @{
 */

/**
 * Hybrid oscillator enum
 *
 */
enum oscillator {
	CLK_OSC_INTERNAL = 0, /*!< Internal silicon oscillator */
	CLK_OSC_EXTERNAL, /*!< External XTAL oscillator */
	CLK_OSC_LAST
};


/**
 *  Clock gating system driver.
 */
extern struct driver clk_system_driver;

/**
 *  Clock gate data which contains register address and bits implicated.
 */
struct clk_gate_info_s {
	uint32_t clk_gate_register;  /*!< Register changed for clock gate */
	uint32_t bits_mask;          /*!< Mask used for clock gate */
};

/**
 *  Configure clock gate for a specific peripheral
 *
 *  @param  clk_gate_info   Pointer to a clock gate data structure
 *  @param  value           Desired state of clock gate
 */
void set_clock_gate(struct clk_gate_info_s *clk_gate_info, uint32_t value);

/**
 * Configure current oscillator type
 *
 * @param oscillator The new oscillator to use
 */
void clk_set_oscillator(enum oscillator oscillator);

/**
 * Return currently configured oscillator
 *
 * @return The currently configured oscillator
 */
enum oscillator clk_get_oscillator(void);

/** @} */

#endif /* CLK_SYSTEM_H_ */
