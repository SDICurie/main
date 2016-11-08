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

#ifndef __BLOS_COMBUFF_H__
#define __BLOS_COMBUFF_H__

/* The below structure is intended to transport information from/to the bootloader to/from the OS. */
/* It must be aligned on 4 bytes */

#include "platform-desc.h"
#include "part.h"

enum boot_indicators {
	INDICATOR_NONE = 0,
	INDICATOR_CORRUPTED_IMAGE = 1 << 1,
	INDICATOR_SUCESSIVE_WATCHDOGS = 1 << 2,
	INDICATOR_PANIC = 1 << 3,
	INDICATOR_OTA_FAILED = 1 << 4,
	INDICATOR_OTA_COMPLETE = 1 << 5,

	/* OEM-defined indicators start here */
	INDICATOR_OEM = 1 << 31,
};

struct blos_combuff {
	/* Header */
	char magic[4]; /* 'BLOS' */
	uint32_t major : 8; /* 1 */
	uint32_t minor : 8; /* 0 */

	/* Runtime - Bi-directional */
	uint32_t watchdog_counter : 8;
	uint32_t boot_target : 8;

	/* Runtime - Bootloader->OS */
	uint32_t indicators;
	struct platform_description plat_desc;
	struct partition *partitions;
	uint32_t partitions_count : 8;
};

#endif /* __BLOS_COMBUFF_H__ */
