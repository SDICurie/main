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

#include "util/cunit_test.h"
#include "infra/device.h"
#include "infra/log.h"
#include "infra/pm.h"

#include "drivers/data_type.h"

/* Test timer in deepsleep */
#define TIMER_DELAY 2000
#define TIMER_DELAY_THRESHOLD 5
#define TIMER_TIMEOUT 5000

T_SEMAPHORE sema_test_timer;

typedef struct {
	uint32_t start_time;
	bool called;
	bool succeed;
} timer_verif_t;

static void timer_cb(void *data)
{
	timer_verif_t *verif = (timer_verif_t *)(data);

	/* Check elapsed time */
	uint32_t elapsed_time = get_time_ms() - verif->start_time;

	cu_print("Elapsed time = %d, Expected elapsed time = %d\n",
		 elapsed_time, TIMER_DELAY);

	/* Compute succeed */
	verif->succeed = elapsed_time >
			 (TIMER_DELAY - TIMER_DELAY_THRESHOLD) &&
			 elapsed_time < (TIMER_DELAY + TIMER_DELAY_THRESHOLD);
	verif->called = true;

	/* Release semaphore to stop the test */
	semaphore_give(sema_test_timer, NULL);
}

void deep_sleep_timer_test(void)
{
	cu_print("##################################################\n");
	cu_print("# Purpose of deep sleep timer test:              #\n");
	cu_print("# - create a timer                               #\n");
	cu_print("# - unplug usb                                   #\n");
	cu_print("# - check elapsed time in timer callback         #\n");
	cu_print("##################################################\n");

	T_TIMER timer = NULL;
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;
	timer_verif_t verif;

	/* Initialize timer data */
	verif.called = false;
	verif.succeed = false;
	verif.start_time = get_time_ms();

	/* Create semaphore to block while timer is called */
	sema_test_timer = semaphore_create(0);

	/* Create and start timer */
	timer = timer_create(timer_cb, &verif, TIMER_DELAY, false, true, &err);
	CU_ASSERT("timer_create error", err == E_OS_OK && timer != NULL);

	/* Request USB disconnection in order to go into deep sleep */
	CU_HOST_CMD_USB_OFF();

	/* Wait for timer callback */
	semaphore_take(sema_test_timer, TIMER_TIMEOUT);
	semaphore_delete(sema_test_timer);

	/* Request USB re-connection and wait a bit */
	CU_HOST_CMD_USB_ON();
	local_task_sleep_ms(1000);

	/* Check that callback has been called and elapsed time is good */
	CU_ASSERT("timer test in deep sleep failed",
		  verif.called && verif.succeed);
}
