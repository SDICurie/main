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

#ifndef __INFRA_TIME_H__
#define __INFRA_TIME_H__
#include <stdint.h>

/**
 * @defgroup time Time management
 *
 * The time management provides functions to manage date and duration.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/time.h"</tt>
 * </table>
 *
 * @ingroup infra
 * @{
 */

/**
 * Return the platform uptime in ms unit.
 *
 * @return Number of ms since the power on of the board.
 */
uint32_t get_uptime_ms(void);

/**
 * Return the platform uptime in ms unit.
 *
 * @return Number of ms since the power on of the board.
 */
uint64_t get_uptime64_ms(void);

/**
 * Return the platform uptime in 32KHz clock unit.
 *
 * @return Number of 1/32K seconds since the power on of the board.
 */
uint32_t get_uptime_32k(void);

/**
 * Set epoch time to define the reference date.
 *
 * @param epoch_time Number of second since 1970.
 */
void set_time(uint32_t epoch_time);

/**
 * Return epoch time
 *
 * @return Number of second since 1970.
 */
uint32_t time(void);

/**
 * Convert uptime (ms) to epoch time
 *
 * @param uptime_ms Number of ms since the power on of the board.
 *
 * @return Corresponding epoch_time calculated from uptime_ms.
 */
uint32_t uptime_to_epoch(uint32_t uptime_ms);

/**@} */

#endif /* __INFRA_TIME_H__ */
