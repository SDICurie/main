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

#include <stdarg.h>
#include "util/assert.h"

#include "os/os.h"
#include "infra/log.h"
#include "log_impl.h"

static uint8_t log_level_limit;

static const char *levels_string[LOG_LEVEL_NUM] = {[LOG_LEVEL_ERROR] =
							   "ERROR",
						   [LOG_LEVEL_WARNING] = "WARN",
						   [LOG_LEVEL_INFO] =
							   "INFO",
						   [LOG_LEVEL_DEBUG] = "DEBUG" };

void log_init()
{
	log_level_limit = LOG_LEVEL_DEBUG;
	log_impl_init();
}

void log_vprintk(uint8_t level, const char *module_short_name,
		 const char *format,
		 va_list args)
{
	/* filter by global level limit */
	if (level > log_level_limit)
		return;

	log_write_msg(level, module_short_name, format, args);
}

void log_printk(uint8_t level, const char *module_short_name,
		const char *format,
		...)
{
	va_list args;

	va_start(args, format);
	log_vprintk(level, module_short_name, format, args);
	va_end(args);
}

const char *log_get_level_name(uint8_t level)
{
	if (level >= LOG_LEVEL_NUM)
		return "";
	return levels_string[level];
}

int8_t log_set_global_level(uint8_t new_level)
{
	if (new_level >= LOG_LEVEL_NUM) {
		return -1;
	}
	log_level_limit = new_level;
	return 0;
}

uint8_t log_get_global_level()
{
	return log_level_limit;
}
