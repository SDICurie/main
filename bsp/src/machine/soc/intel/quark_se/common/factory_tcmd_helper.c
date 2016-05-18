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
#include <infra/tcmd/handler.h>
#include "infra/factory_data.h"
#include "curie_factory_data.h"
#include <os/os.h>

/**
 *  Project specific function to print project data defined in project_data field
 */
void factory_get_project_data(struct tcmd_handler_ctx *ctx)
{
	static const uint32_t maxlen = 64;
	char *buf = balloc(maxlen, NULL);
	struct curie_oem_data *oem_data;
	int i;

	/* BT information */
	oem_data =
		(struct curie_oem_data *)(global_factory_data->oem_data.
					  project_data);
	snprintf(buf, maxlen, "BT: ");
	char *p = buf + 4;
	for (i = 0; i < MAC_ADDRESS_SIZE; ++i) {
		snprintf(p, maxlen, "%02x:", oem_data->bt_address[i]);
		p += 3;
	}
	*(p - 1) = ' ';
	snprintf(p, maxlen, "(type: %02x)\0", oem_data->bt_mac_address_type);

	TCMD_RSP_PROVISIONAL(ctx, buf);
	bfree(buf);
	return;
}
