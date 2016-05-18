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

#include <string.h>
#include <stdio.h>
#include <infra/factory_data.h>
#include "machine.h"
#include <infra/tcmd/handler.h>
#include <os/os.h>

const struct factory_data *global_factory_data =
	(struct factory_data *)FACTORY_DATA_ADDR;

__weak void factory_get_project_data(struct tcmd_handler_ctx *ctx)
{
	/* Project specific TCMD_RSP_PROVISIONAL  */
	return;
}

int valid_factory_header(struct tcmd_handler_ctx *ctx)
{
	if (strncmp((char *)global_factory_data->oem_data.magic,
		    FACTORY_DATA_MAGIC, 4)) {
		TCMD_RSP_FINAL(ctx, "No factory data");
		return 0;
	}
	if (global_factory_data->oem_data.version > FACTORY_DATA_VERSION) {
		TCMD_RSP_FINAL(ctx, "Unsupported factory data version");
		return 0;
	}
	return 1;
}

/**
 * @Brief Test command to display factory data: factory get
 * @param[in]	argc	Number of arguments in the Test command
 * @param[in]	argv	Table of multi-terminated buffers containing the
 * arguments
 * @param[in]	ctx	The context to pass back to responses
 */
void factory_get(int argc, char *argv[], struct tcmd_handler_ctx *ctx)
{
	if (!valid_factory_header(ctx))
		return;

	static const uint32_t maxlen = 64;
	char *buf = balloc(maxlen, NULL);

	/* Ouput OEM UUID */
	snprintf(buf, maxlen, "OEM UUID: ");
	char *p = buf + 10;
	int i;
	for (i = 0; i < 16; ++i) {
		snprintf(p, maxlen, "%02x",
			 global_factory_data->oem_data.uuid[i]);
		p += 2;
	}
	*p = '\0';
	TCMD_RSP_PROVISIONAL(ctx, buf);

	/* Output OEM factory serial number */
	snprintf(buf, maxlen, "OEM S/N: ");
	memcpy(buf + 9, global_factory_data->oem_data.factory_sn, 32);
	*(buf + 9 + 32) = '\0';
	TCMD_RSP_PROVISIONAL(ctx, buf);

	/* Ouput OEM hardware ID */
	snprintf(buf, maxlen, "OEM HWID: ");
	p = buf + 10;
	for (i = 0; i < 32; ++i) {
		snprintf(p, maxlen, "%02x",
			 global_factory_data->oem_data.hardware_id[i]);
		p += 2;
	}
	*p = '\0';
	TCMD_RSP_PROVISIONAL(ctx, buf);

	/* Output Customer factory serial number */
	snprintf(buf, maxlen, "CUSTOMER S/N: ");
	memcpy(buf + 14, global_factory_data->customer_data.product_sn, 16);
	*(buf + 14 + 16) = '\0';
	TCMD_RSP_PROVISIONAL(ctx, buf);

	/* Output Product hardware ID */
	snprintf(buf, maxlen, "CUSTOMER HWID: ");
	p = buf + 15;
	snprintf(p, maxlen, "%02x",
		 global_factory_data->customer_data.product_hw_info.name);
	p += 2;
	snprintf(p, maxlen, "%02x",
		 global_factory_data->customer_data.product_hw_info.type);
	p += 2;
	snprintf(p, maxlen, "%02x",
		 global_factory_data->customer_data.product_hw_info.revision);
	p += 2;
	snprintf(p, maxlen, "%02x",
		 global_factory_data->customer_data.product_hw_info.variant);
	p += 2;
	*p = '\0';
	TCMD_RSP_PROVISIONAL(ctx, buf);

	/* Output project specific data if available */
	factory_get_project_data(ctx);

	snprintf(buf, maxlen, "");
	TCMD_RSP_FINAL(ctx, buf);
	bfree(buf);
}

DECLARE_TEST_COMMAND(factory, get, factory_get);
