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

#include <stdint.h>
#include "machine/soc/intel/quark_se/pm_pupdr.h"
#include "machine/soc/intel/quark_se/soc_config.h"

#include "quark_se_common.h"

#ifdef CONFIG_DEEPSLEEP
#include <zephyr.h>
#include "drivers/intel_qrk_aonpt.h"
#include "infra/time.h"
#include "infra/log.h"
#include "scss_registers.h"
#include "util/misc.h"
#include "infra/tcmd/handler.h"
#include <stdio.h>
#include <stdlib.h>
#include "util/compiler.h"

/* Minimum deeplseep duration in ms */
#define DEEP_SLEEP_MIN_DURATION 10
/* Minimum deeplseep duration in 32k base */
#define DEEP_SLEEP_MIN_DURATION_32K (DEEP_SLEEP_MIN_DURATION * 32768 / 1000)
/* Estimated wakeup delay in ms */
#define ESTIMATED_WAKEUP_DELAY  3
/* Estimated wakeup delay in 32k base */
#define ESTIMATED_WAKEUP_DELAY_32K (ESTIMATED_WAKEUP_DELAY * 32768 / 1000)

static uint32_t cycle_count = 0;
static uint32_t cycle_idle = 0;
static volatile uint32_t start = 0;

static bool enable_deep_sleep = true;
#endif /* CONFIG_DEEPSLEEP */

/* AONPT interruption stub */
#if CONFIG_X86_IAMCU
__asm__("_aonptIntStub:\n\t" \
	"pushl %eax\n\t" \
	"pushl %edx\n\t" \
	"pushl %ecx\n\t" \
	"movl $aonpt_ISR, %eax\n\t" \
	"call _execute_handler\n\t" \
	"popl %ecx\n\t"	\
	"popl %edx\n\t"	\
	"popl %eax\n\t"	\
	"iret\n\t");
#else
__asm__("_aonptIntStub:\n\t" \
	"call _IntEnt\n\t" \
	"pushl $0\n\t" \
	"call aonpt_ISR\n\t" \
	"jmp _IntExitWithEoi\n\t");
#endif

/* Computing the ARC timeout:
 * The overall goal is to check if the ARC can go into deepsleep. To check this
 * the system knows:
 * - what time it is now (start variable)
 * - when is the next timeout of the ARC (ie what is the next timer expiracy)
 *   (shared_data->arc_next_wakeup variable, called 'wake' here)
 *
 * These 2 values are expressed in the AON counter unit value (1/32768 seconds).
 * So 'wake - start' corresponds to the time where the Arc has nothing to do and
 * the platform can go into deepsleep (from ARC point of view). If this result
 * is negative, that means the ARC has something to do and is busy.
 * The problem comes from the fact that these 2 values are limited to
 * UINT32_MAX, and may wrap to 0.
 *
 * Case study:
 * P: Time when ARC publishes its timeout
 * S: Start value (ie 'now')
 * W: Wake value (ie 'when Arc will work')
 *
 * Basic cases:
 * |----------------+---------+-----------+-----------------------------------|
 * 0                P         S           W                            UINT32_MAX
 *                  |_____________________|
 *
 * W - S > 0. Deespleep allowed
 *
 * |----------------+---------------------+----------+------------------------|
 * 0                P                     W          S                 UINT32_MAX
 *                  |_____________________|
 *
 * W - S < 0. Deespleep not allowed
 *
 * Basic wrap cases:
 *
 * |----+--------+------------------------+-----------------------------------|
 * 0    S        W                        P                                  UINT32_MAX
 * ______________|                        |________________________________WRAP to 0
 *
 * W - S > 0. Deepsleep allowed
 *
 *
 * |-------------+------+-----------------+-----------------------------------|
 * 0             W      S                 P                                  UINT32_MAX
 * ______________|                        |________________________________WRAP to 0
 *
 * W - S < 0. Deepsleep not allowed
 *
 * Corner wrap cases:
 *
 * |----+---------------------------------+---------------------------+-------|
 * 0    W                                 P                           S      UINT32_MAX
 * _____|                                 |________________________________WRAP to 0
 *
 * W - S > 0. Deepsleep allowed
 *
 * Corner-Corner wrap case:
 *
 * |----+---------------------------------+---------------------------+-------|
 * 0    S                                 P                           W     UINT32_MAX
 *                                        |___________________________|
 *
 * W - S < 0. Deepsleep *not* allowed. In this case, the arithmetic distance is
 * greater than the maximum allowed time for tickless idle: overflow time of the
 * internal TICK timer (UINT32_MAX/core_frequency -> ~134 s at 32MHz).
 *
 * Conclusion:
 *  * Assumption: W - S is correct even in case of overflow, provided that the
 *  (real) delta was < UINT32_MAX/2, which is always true
 *  * Rule: W - S > 0 -> sleep, W - S <= 0 -> do not sleep
 * */
int32_t _tickless_idle_hook(int32_t ticks)
{
	if (pm_is_shutdown_allowed()) {
		// handle shutdown
		pm_shutdown();
	}
#ifdef CONFIG_DEEPSLEEP
	if (
		!enable_deep_sleep ||
		ticks < DEEP_SLEEP_MIN_DURATION ||
		!pm_is_deepsleep_allowed()) {
		/* We won't go to deep sleep */
		return 0;
	}

	/* Find out if both cores want to go in deepsleep, and for how long */
	start = get_uptime_32k();
	pr_debug(LOG_MODULE_QUARK_SE, "Tickless hook: %d curr %d", ticks,
		 get_time_ms());
	int qrk_timeout = ((int64_t)ticks * 32768) / 1000;
	int arc_timeout = 0;
	int timeout;

	if (shared_data->arc_next_wakeup_valid) {
		/* The cast into int gives valid results as soon as the absolute
		 * distance is < UINT32_MAX/2, which is always the case. */
		uint32_t arc_next_wake = shared_data->arc_next_wakeup;
		arc_timeout = (int)(arc_next_wake - start);

		/* Arc set a timeout but it is in the past: we won't go to deepsleep now.
		 * Wait for a future call of this function, when the ARC has set its next
		 * wakeup time */
		if (arc_timeout < 0) {
			return 0;
		}

		timeout = MIN(qrk_timeout, arc_timeout);
	} else {
		timeout = qrk_timeout;
	}

	/* Check that final deep sleep duration is above threshold */
	if ((timeout - ESTIMATED_WAKEUP_DELAY_32K) <
	    DEEP_SLEEP_MIN_DURATION_32K) {
		return 0;
	}
	qrk_aonpt_configure(timeout - ESTIMATED_WAKEUP_DELAY_32K, NULL, true);
	qrk_aonpt_start();

	/* Publish final wakeup time to let ARC a last chance to cancel deepsleep */
	shared_data->soc_next_wakeup = start + timeout;

	if (pm_core_deepsleep()) {
		start = 0;
		qrk_aonpt_stop();
		return 0; /* Deepsleep failed */
	}

	/* Force interrupt enabling as it will not
	 * be done in os idle func */
	__asm__ __volatile__ ("sti;");

	uint32_t flags = irq_lock();
	/* Check if the wakeup source is on ARC core
	 * or is the aon periodic timer */
	if (start) {
		/* Trigger AONPT interruption by simulating hardware behaviour :
		 * - push flags
		 * - push code segment
		 * - call aonpt interruption stub
		 */
		__asm__ __volatile__ ("pushf;"
				      "push %cs;"
				      "call _aonptIntStub;");
	}
	/* When getting out of deepsleep, ARC processor does
	 * not reschedule. If the ARC has something to do, force a
	 * rescheduling by sending it an interrupt through mailbox. */
	MBX_CTRL(IPC_QRK_SS_ASYNC) = 0x80000000;

	irq_unlock(flags);
	return 1; /* Deepsleep occurred */
#else
	return 0; /* Deepsleep not available */
#endif
}

int32_t _tickless_idle_hook_exit()
{
#ifdef CONFIG_DEEPSLEEP
	if (!start) {
		return 0;
	}

	/* Compute deepsleep ticks */
	int32_t ret =
		(((uint64_t)(get_uptime_32k() - start)) * 1000 + 16383) / 32768;
	start = 0;

	cycle_count++;
	cycle_idle += ret;

	if (cycle_count % 10 == 0) {
		pr_debug(LOG_MODULE_QUARK_SE, "idle %d count %d",
			 cycle_idle, cycle_count);
	}

	return ret;
#else
	return 0; /* Deepsleep not available */
#endif
}

#ifdef CONFIG_DEEPSLEEP
uint32_t get_deepsleep_count()
{
	return cycle_count;
}
#endif

#if defined(CONFIG_DEEPSLEEP)
void pm_stat_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	char buffer[100];
	static unsigned int last_cycle = 0;
	static unsigned int last_timestamp = 0;

	snprintf(buffer, 100, "time: %d count: %d ratio: %d sincelast: %d",
		 cycle_idle, cycle_count,
		 (uint32_t)((uint64_t)cycle_idle * 1000 / get_uptime_ms()),
		 (uint32_t)((((uint64_t)cycle_idle -
			      last_cycle) *
			     1000) / (get_uptime_ms() - last_timestamp)));
	last_cycle = cycle_idle;
	last_timestamp = get_uptime_ms();
	TCMD_RSP_FINAL(ctx, buffer);
}
DECLARE_TEST_COMMAND(system, slpstat, pm_stat_tcmd);

void pm_idle_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	if (argc == 3) {
		TCMD_RSP_FINAL(ctx, NULL);
		enable_deep_sleep = (atoi(argv[2]) == 1 ? true : false);
	} else {
		TCMD_RSP_ERROR(ctx, TCMD_ERROR_MSG_INV_ARG);
	}
}

DECLARE_TEST_COMMAND(system, idle, pm_idle_tcmd);
#endif
