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

#include <infra/time.h>
#include "infra/tcmd/handler.h"
#include "util/compiler.h"
#include <string.h>
#include <stdio.h>

void __cyg_profile_func_enter(void *, void *) notrace;
void __cyg_profile_func_exit(void *, void *) notrace;

struct profiling {
	uint8_t direction;
	void *func;
	uint32_t timestamp;
};

#define PROFILING_MAX_SIZE 4096
#define PROFILING_RAM_ADDR (0xa800e000 - PROFILING_MAX_SIZE)

#define BUFFER_SIZE (PROFILING_MAX_SIZE / sizeof(struct profiling) - 1)
uint32_t *index = (uint32_t *)(PROFILING_RAM_ADDR);
struct profiling *buffer = (struct profiling *)(PROFILING_RAM_ADDR + 4);

void __cyg_profile_func_enter(void *func, void *caller)
{
	if (*index >= BUFFER_SIZE) return;
	buffer[*index].direction = 'e';
	buffer[*index].func = func;
	buffer[*index].timestamp = get_uptime_32k();
	*index = *index + 1;
}


void __cyg_profile_func_exit(void *func, void *caller)
{
	if (*index >= BUFFER_SIZE) return;
	buffer[*index].direction = 'x';
	buffer[*index].func = func;
	buffer[*index].timestamp = get_uptime_32k();
	*index = *index + 1;
}

void get_profiling(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	char tmp[64];
	uint32_t i;

	for (i = 0; i < *index; i++) {
		tmp[0] = buffer[i].direction;
		snprintf(tmp + 1, sizeof(tmp) - 1,
			 " %p %d", buffer[i].func, buffer[i].timestamp);
		TCMD_RSP_PROVISIONAL(ctx, tmp);
	}
	TCMD_RSP_FINAL(ctx, NULL);
}
DECLARE_TEST_COMMAND_ENG(debug, profiling, get_profiling);
