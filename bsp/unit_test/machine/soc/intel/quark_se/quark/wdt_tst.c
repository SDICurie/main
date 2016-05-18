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

#include <stdbool.h>
#include <watchdog.h>
#include "os/os.h"
#include "util/cunit_test.h"
#include "machine.h"

typedef enum {
	WDT_TST_STATE_IDLE = 0,
	WDT_TST_STATE_KEEPALIVE,
	WDT_TST_STATE_REBOOT,
	WDT_TST_STATE_MAX
} wdt_tst_state;

static void qrk_wdt_config(struct device *dev, uint32_t timeout)
{
	struct wdt_config config;
	int res;

	config.timeout = timeout;
	config.mode = WDT_MODE_RESET;
	res = wdt_set_config(dev, &config);
	CU_ASSERT("Wdt configuration failure", res == DEV_OK);
}

static void qrk_wdt_keepalive_test(struct device *dev, uint32_t timeout_ticke)
{
	uint32_t start_time;
	uint32_t cur_time;
	uint32_t count;

	cu_print(
		" Acknowledge multiple times the watchdog system to prevent panic and reset\n");
	MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
			       SCSS_GPS1) = WDT_TST_STATE_KEEPALIVE;
	start_time = get_time_ms();
	for (count = 0; count < 10; count++) {
		while (1) {
			cur_time = get_time_ms();
			if (cur_time - start_time > timeout_ticke) {
				wdt_reload(dev);
				start_time = cur_time;
				break;
			}
		}
	}
}

static void qrk_wdt_reboot_test(struct device *dev, uint32_t timeout)
{
	uint32_t start_time;
	uint32_t cur_time;

	cu_print(" Wait watchdog timeout [reboot test]\n");
	MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
			       SCSS_GPS1) = WDT_TST_STATE_REBOOT;
	start_time = get_time_ms();
	while (1) {
		cu_print(".");
		cur_time = get_time_ms();
		if (cur_time - start_time > timeout + 10) {
			MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
					       SCSS_GPS1) = WDT_TST_STATE_IDLE;
			wdt_disable(dev);
			CU_ASSERT("Wdt reboot test Failed", false);
			break;
		}
	}
}

void qrk_wdt_test(void)
{
	uint32_t timeout_ms;
	uint32_t timeout_tickle_ms;
	struct device *wdt_dev;

	extern struct device DEVICE_NAME_GET(wdt);
	wdt_dev = DEVICE_GET(wdt);
	wdt_tst_state state;

	state = MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE, SCSS_GPS1);
	if (state == WDT_TST_STATE_IDLE) {
		cu_print("##################################################\n");
		cu_print("# Purpose of Watchdog tests :                    #\n");
		cu_print("#            Configure watchdog                  #\n");
		cu_print("#            Keep alive watchdog multiple times  #\n");
		cu_print("#            Test watchdog reboot                #\n");
		cu_print("##################################################\n");
		timeout_ms = 25;
		timeout_tickle_ms = 20;
		qrk_wdt_config(wdt_dev, timeout_ms);
		qrk_wdt_keepalive_test(wdt_dev, timeout_tickle_ms);
		qrk_wdt_reboot_test(wdt_dev, timeout_ms);
	} else if (state == WDT_TST_STATE_REBOOT) {
		cu_print("##################################################\n");
		cu_print("# Watchdog tests done                            #\n");
		cu_print("##################################################\n");
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
				       SCSS_GPS1) = WDT_TST_STATE_IDLE;
	} else {
		CU_ASSERT("Wdt test Failed", false);
		MMIO_REG_VAL_FROM_BASE(SCSS_REGISTER_BASE,
				       SCSS_GPS1) = WDT_TST_STATE_IDLE;
	}
}
