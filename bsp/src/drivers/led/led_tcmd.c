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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "infra/tcmd/handler.h"
#include "drivers/led/led.h"

#ifdef CONFIG_LED_MULTICOLOR
static void set_led_color(led_s *pattern, char color)
{
	switch (color) {
	case 'r':
		pattern->rgb[0].r = 255;
		break;
	case 'g':
		pattern->rgb[0].g = 255;
		break;
	case 'b':
		pattern->rgb[0].b = 255;
		break;
	default:
		break;
	}
}
#endif

void ui_led_tcmd(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	uint8_t index = 0;
	uint8_t ret;

#ifdef CONFIG_LED_MULTICOLOR
	int i;
#endif
	led_s pattern = { 0 };
	if (!strcmp(argv[1], "on")) {
		pattern.intensity = 100;
#ifdef CONFIG_LED_MULTICOLOR
		pattern.rgb[0].r = 255; /* default color is red */
#endif
	}
	pattern.repetition_count = 0;
	switch (argc) {
	case 5:
		pattern.duration[0].duration_on = atoi(argv[4]) * 1000;
	/* no break */

	case 4:
#ifdef CONFIG_LED_MULTICOLOR
		pattern.rgb[0].r = 0;
		i = 0;
		while (argv[3][i]) {
			set_led_color(&pattern, argv[3][i]);
			i++;
		}
		/* no break */
#endif
	case 3:
		index = atoi(argv[2]);
	/* no break */
	case 2:
		ret = led_pattern_handler_config(LED_BLINK_X1, &pattern,
						 index);
		if (ret)
			goto err;
		TCMD_RSP_FINAL(ctx, NULL);
		break;

	default:
		goto err;
		break;
	}
	return;
err:
	TCMD_RSP_ERROR(ctx, NULL);
}

DECLARE_TEST_COMMAND(led, on, ui_led_tcmd);
DECLARE_TEST_COMMAND(led, off, ui_led_tcmd);
