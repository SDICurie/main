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

/*
 * ZEPHYR OS abstraction / internal utilities
 */

#include <zephyr.h>
#include "os/os.h"
#include "infra/panic.h"
#include "common.h"

/*
 * Initializes private variables.
 *
 * IMPORTANT : this function must be called during the initialization
 *             of the OS abstraction layer.
 *             This function shall only be called once after reset, otherwise
 *             it may cause the take/lock and give/unlock services to fail.
 */
void os_init(void)
{
	/* initialize all modules of the OS abstraction */
	os_init_sync();
	os_init_timer();
}


/**
 * Copies error code to caller's variable, or panics if caller did not specify
 * an error variable.
 *
 * @param [out] err : pointer to caller's error variable - may be _NULL
 * @param [in] localErr : error detected by the function
 *
 */
void error_management(OS_ERR_TYPE *err, OS_ERR_TYPE localErr)
{
	if (NULL != err) {
		*err = localErr;
	} else {
		if (E_OS_OK != localErr) {
			/* TODO: clean-up */
			panic(localErr);
		}
	}
}


void local_task_sleep_ms(int time)
{
	int ticks = CONVERT_MS_TO_TICKS(time);

	local_task_sleep_ticks(ticks);
}

void local_task_sleep_ticks(int ticks)
{
#if defined CONFIG_NANOKERNEL
	struct nano_timer tmr;
	uint32_t tmrData;
	nano_timer_init(&tmr, &tmrData);
	if (E_EXEC_LVL_TASK == _getExecLevel()) {
		nano_task_timer_start(&tmr, ticks);
		nano_task_timer_test(&tmr, TICKS_UNLIMITED); /* may cause a context switch */
		nano_task_timer_stop(&tmr);
	} else {
		nano_fiber_timer_start(&tmr, ticks);
		nano_fiber_timer_test(&tmr, TICKS_UNLIMITED); /* may cause a context switch */
		nano_fiber_timer_stop(&tmr);
	}
#elif defined CONFIG_MICROKERNEL
	task_sleep(ticks);
#endif
}
