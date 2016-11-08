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

#include <stdio.h>
#include <string.h>

#include "infra/partition.h"

/* Generic platform definitions */
#include "infra/part.h"
#include "infra/blos_combuff.h"

/* For __used */
#include "util/compiler.h"

/* Actual address of the platform structure */
#include "blos_address.h"

/* Test commands */
#include "infra/tcmd/handler.h"

static struct blos_combuff *platform = BLOS_COMBUFF_ADDRESS;

struct partition *partition_get(uint8_t id)
{
	int i;

	for (i = 0; i < platform->partitions_count; i++) {
		if (platform->partitions[i].partition_id == id) {
			return &platform->partitions[i];
		}
	}
	return 0;
}

struct partition *partition_get_by_name(char *name)
{
	int i;

	for (i = 0; i < platform->partitions_count; i++) {
		if (strcmp(platform->partitions[i].name, name) == 0) {
			return &platform->partitions[i];
		}
	}
	return 0;
}

//SOFTEQ

#include "infra/log.h"
void partition_printk(void)
{
	pr_debug(LOG_MODULE_USB, "platform is 0x%x", platform);
	pr_debug(LOG_MODULE_USB, "platform is 0x%x", platform->partitions_count);
	pr_debug(LOG_MODULE_USB, "platform is 0x%x", platform->partitions[0]);
	pr_debug(LOG_MODULE_USB, "platform is 0x%x", platform);

#if 1

	int i;
	char buffer[128];

	for (i = 0; i < platform->partitions_count; i++) {
		struct partition *part = &platform->partitions[i];
		snprintf(buffer,
				 sizeof(buffer) - 1,
				 "/dev%d/%-2d %5d %sbytes %s, start: 0x%x, attr: 0x%x",
				 part->storage->id,
				 part->partition_id,
				 (int)((part->size >
						1024) ? part->size / 1024 : part->size),
				 (part->size > 1024) ? "K" : " ",
				 part->name,
				part->start,
			   part->attributes);


		pr_debug(LOG_MODULE_USB, buffer);

	}
#endif
}

static __used void partition_list(int				argc,
				  char **			argv,
				  struct tcmd_handler_ctx *	ctx)
{
	int i;
	char buffer[128];

	for (i = 0; i < platform->partitions_count; i++) {
		struct partition *part = &platform->partitions[i];
		snprintf(buffer,
			 sizeof(buffer) - 1,
			 "/dev%d/%-2d %5d %sbytes %s",
			 part->storage->id,
			 part->partition_id,
			 (int)((part->size >
				1024) ? part->size / 1024 : part->size),
			 (part->size > 1024) ? "K" : " ",
			 part->name);
		TCMD_RSP_PROVISIONAL(ctx, buffer);
	}
	TCMD_RSP_FINAL(ctx, NULL);
}
DECLARE_TEST_COMMAND_ENG(partition, list, partition_list);
