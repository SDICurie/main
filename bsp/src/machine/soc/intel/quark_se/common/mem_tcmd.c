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
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "infra/tcmd/handler.h"

/* defines for all operation type */
#define WRITE_PARAM                     5
#define READ_PARAM                      4
#define ADDR_OFFSET                     3
#define SIZE_OFFSET                     2

/* Write operation specific */
#define WRITE_VALUE_OFFSET              4

/* answer length */
#define LENGTH                          80

static char answer[LENGTH];

/*
 * @addtogroup infra_tcmd
 * @{
 */

/*
 * @defgroup infra_tcmd_mem PEEKnPOKE Test Commands
 * Interfaces to support PEEKnPOKE Test Commands.
 * @{
 */

/*
 * Reads on a given address: mem read <size> <addr>
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name)
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The opaque context to pass to responses
 */
void memory_read(int argc, char **argv, struct tcmd_handler_ctx *ctx)
{
	uint32_t address, size, value;
	char *endptr = NULL;

	errno = 0;

	if (argc != READ_PARAM) {
		TCMD_RSP_ERROR(ctx, "CMD: mem read 1|2|4 addr");
		return;
	}
	address = strtoul(argv[ADDR_OFFSET], &endptr, 16);
	if (((errno == ERANGE) && (address == LONG_MAX || address == LONG_MIN))
	    || (errno != 0 && address == 0)) {
		TCMD_RSP_ERROR(ctx, "strtoul error");
		return;
	}
	if ((endptr == argv[ADDR_OFFSET]) || (*endptr != '\0')) {
		TCMD_RSP_ERROR(ctx, "Address in hexa !");
		return;
	}

	size = strtoul(argv[SIZE_OFFSET], &endptr, 10);
	if (((errno == ERANGE) && (size == LONG_MAX || size == LONG_MIN))
	    || (errno != 0 && size == 0)) {
		TCMD_RSP_ERROR(ctx, "strtoul error");
		return;
	}
	if ((endptr == argv[SIZE_OFFSET]) || (*endptr != '\0')) {
		TCMD_RSP_ERROR(ctx, "Size 1|2|4 !");
		return;
	}

	switch (size) {
	case 1:
		value = *(uint8_t *)address;
		break;
	case 2:
		value = *(uint16_t *)address;
		break;
	case 4:
		value = *(uint32_t *)address;
		break;
	default:
		TCMD_RSP_ERROR(ctx, "size: 1byte | 2bytes | 4bytes");
		return;
	}
	snprintf(answer, LENGTH, "read: 0x%08X = 0x%x",
		 (unsigned int)address, (unsigned int)value);
	TCMD_RSP_FINAL(ctx, answer);
}

DECLARE_TEST_COMMAND(mem, read, memory_read);

/*
 * Writes given value to a given address: mem write <size> <addr> <Val>
 *
 * @param[in]   argc        Number of arguments in the Test Command (including group and name),
 * @param[in]   argv        Table of null-terminated buffers containing the arguments
 * @param[in]   ctx         The opaque context to pass to responses
 */
void memory_write(int argc, char **argv, struct tcmd_handler_ctx *ctx)
{
	uint32_t address, size, value;
	char *endptr = NULL;

	errno = 0;

	if (argc != WRITE_PARAM) {
		TCMD_RSP_ERROR(ctx, "CMD: mem write 1|2|4 addr val");
		return;
	}
	address = strtoul(argv[ADDR_OFFSET], &endptr, 16);
	if (((errno == ERANGE) && (address == LONG_MAX || address == LONG_MIN))
	    || (errno != 0 && address == 0)) {
		TCMD_RSP_ERROR(ctx, "strtoul error");
		return;
	}
	if ((endptr == argv[ADDR_OFFSET]) || (*endptr != '\0')) {
		TCMD_RSP_ERROR(ctx, "Address in hexa !");
		return;
	}

	size = strtoul(argv[SIZE_OFFSET], &endptr, 10);
	if (((errno == ERANGE) && (size == LONG_MAX || size == LONG_MIN))
	    || (errno != 0 && size == 0)) {
		TCMD_RSP_ERROR(ctx, "strtoul error");
		return;
	}
	if ((endptr == argv[SIZE_OFFSET]) || (*endptr != '\0')) {
		TCMD_RSP_ERROR(ctx, "Size 1|2|4 !");
		return;
	}
	value = (uint32_t)strtoul(argv[WRITE_VALUE_OFFSET], &endptr, 16);
	if (((errno == ERANGE) && (value == LONG_MAX || value == LONG_MIN))
	    || (errno != 0 && value == 0)) {
		TCMD_RSP_ERROR(ctx, "strtoul error");
		return;
	}
	if ((endptr == argv[WRITE_VALUE_OFFSET]) || (*endptr != '\0')) {
		TCMD_RSP_ERROR(ctx, "Value in hexa !");
		return;
	}

	snprintf(answer, LENGTH, "write: 0x%08X = 0x%x",
		 (unsigned int)address, (unsigned int)value);

	switch (size) {
	case 1:
		*(uint8_t *)address = (uint8_t)value;
		break;
	case 2:
		*(uint16_t *)address = (uint16_t)value;
		break;
	case 4:
		*(uint32_t *)address = (uint32_t)value;
		break;
	default:
		TCMD_RSP_ERROR(ctx, "size: 1byte | 2bytes | 4bytes");
		return;
	}

	TCMD_RSP_FINAL(ctx, answer);
}

DECLARE_TEST_COMMAND(mem, write, memory_write);

/*
 * @}
 *
 * @}
 */
