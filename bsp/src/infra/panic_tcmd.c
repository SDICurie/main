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

#include <ctype.h>
#include <stdlib.h>
#include "infra/tcmd/handler.h"
#include "infra/panic.h"
#ifdef CONFIG_INTEL_QRK_WDT
#include <device.h>
#include <watchdog.h>
#include "os/os.h"
#endif
#define PANIC_ID_IDX 2
#define ARGC         3

/*
 * @addtogroup infra_tcmd
 * @{
 */

/*
 * @defgroup infra_tcmd_panic PANIC Test Commands
 * Interfaces to support PANIC Test Commands.
 * @{
 */

#ifdef CONFIG_DEBUG_PANIC_TCMD
/*
 * Test command for panic generator: debug panic <panic_id>
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The context to pass back to responses
 */
void debug_panic(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	volatile uint32_t panic_id;
	volatile uint32_t aligned_var[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
	volatile uint32_t unaligned_ptr;
	volatile int opcode = 0;

#ifdef CONFIG_INTEL_QRK_WDT
	struct device *wdt_dev;
	struct wdt_config config;
	int res;
#endif

	if (argc != ARGC)
		goto print_help;

	panic_id = strtoul(argv[PANIC_ID_IDX], NULL, 10);

	switch (panic_id) {
	case 0:
		panic_id = 123 / panic_id;
		TCMD_RSP_ERROR(
			ctx,
			"Division by 0 did not panic (sw implementation ?).");
		break;
	case 1:
		unaligned_ptr = (uint32_t)&aligned_var;
		if (*((uint32_t *)(unaligned_ptr + 1)))
			TCMD_RSP_ERROR(
				ctx,
				"Unaligned access is allowed on this platform.");
		break;
	case 2:
#ifdef CONFIG_INTEL_QRK_WDT
		config.timeout = 2097; // Timeout: 2.097s (for 32MHz)
		config.mode = WDT_MODE_INTERRUPT_RESET;
		extern struct device DEVICE_NAME_GET(wdt);
		wdt_dev = DEVICE_GET(wdt);
		res = wdt_set_config(wdt_dev, &config);
		if (res == DEV_OK) {
			TCMD_RSP_FINAL(ctx, "Watchdog");
			irq_lock();
			while (1) ;
		} else
			TCMD_RSP_ERROR(ctx, "Watchdog configuration failure");

#else
		TCMD_RSP_ERROR(ctx, "Watchdog not supported");
#endif
		break;
	case 3:
		TCMD_RSP_FINAL(ctx, "Invalid address");
		*((volatile uint32_t *)0xFFFFFFFF) = 0xABCD;
		break;
	case 4:
		TCMD_RSP_FINAL(ctx, "App Error.");
		panic(0x123456);
		break;
	case 5:
		/* Need MMU to support stack overflow */
		TCMD_RSP_ERROR(ctx, "No Stack Overflow");
		break;
	case 6:
		((void (*)(void))(&opcode))();
		TCMD_RSP_FINAL(ctx, "Wrong OpCode.");
		break;
	default:
		TCMD_RSP_ERROR(ctx, "KO 1");
		break;
	}
	return;

print_help:
	TCMD_RSP_ERROR(ctx, "Usage: debug panic <panic_id>.");
}

DECLARE_TEST_COMMAND_ENG(debug, panic, debug_panic);
#endif
/*
 * @}
 *
 * @}
 */
