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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "os/os.h"
#include "infra/tcmd/handler.h"

#include "machine.h"
#include "drivers/ss_adc.h"

#define MIN_CH          0
#define MAX_CH          18
#define CH_PARAM        2
#define TEST_DLY        50
#define TEST_RESOLUTION 12
#define TEST_CLK_RATIO  1024
#define LENGTH          80
#define TEST_ADC_DEV    SS_ADC_ID /* TODO: add dev as parameter */

/*
 * Test command to get the channel value : adc get <channel>
 *
 * @param[in]	argc	Number of arguments in the test command
 * @param[in]	argv	Table of null-terminated buffers containing the arguments
 * @param[in]	ctx	The Test command response context
 *
 */
void adc_get(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	/* initialization */
	uint8_t channel;

	if (argc == 3) {
		channel = (uint32_t)(atoi(argv[CH_PARAM]));
	} else {
		TCMD_RSP_ERROR(ctx, "Invalid cmd: adc get <ch>");
		return;
	}

	DRIVER_API_RC status = DRV_RC_FAIL;

	/* Check the parameter */
	if (!(isdigit((unsigned char)argv[CH_PARAM][0]))) {
		TCMD_RSP_ERROR(ctx, "Invalid cmd: adc get <ch>");
	} else {
		if (channel >= MIN_CH && channel <= MAX_CH) {
			uint16_t result_value = 0;
			status = ss_adc_read(channel, &result_value);
			if (status != DRV_RC_OK) {
				TCMD_RSP_ERROR(ctx, NULL);
			} else {
				char answer[LENGTH];
				snprintf(answer, LENGTH, "%lu %lu", channel,
					 result_value);
				TCMD_RSP_FINAL(ctx, answer);
			}
		} else {
			TCMD_RSP_ERROR(ctx, "Invalid num ch: 0_ch_18");
		}
	}
	return;
}

DECLARE_TEST_COMMAND(adc, get, adc_get);

/*
 * @}
 *
 * @}
 */
