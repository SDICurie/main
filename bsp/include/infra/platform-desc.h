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

#ifndef __PLATFORM_DESC_H__
#define __PLATFORM_DESC_H__

#include <stdint.h>

/* Defines platform resources */

/* All structures in this file are shared between the bootloader and the main
 * application.
 * They must be aligned on 4 bytes */

/* The list of storage devices to define partitions for */
enum storage_id {
	INTERNAL_FLASH_0,
	SPI_FLASH_0,
};

enum device_type {
	STORAGE_DEVICE,
};

enum storage_technology {
	INTERNAL_FLASH,
	SPI_FLASH
};

/* Storage device corresponding to: SPI_FLASH_0, INTERNAL_FLASH_0,... */
struct storage_device {
	/* Header */
	uint32_t major : 8; /* 1 */
	uint32_t minor : 8; /* 0 */

	/* Global ID of the storage device in the system */
	uint32_t id : 8; /* storage_id */

	/* Below content is subject to change */

	/* link to the driver */
	uint32_t technology : 8;

	/* Sequence number per technology/controller (or controller group if regions
	 * in memory space are contiguous) */
	uint32_t tech_index : 8; /* storage_technology */

	/* Driver can use it or not. */
	uint32_t block_size;
};

struct platform_device {
	/* Header */
	uint32_t major : 8; /* 1 */
	uint32_t minor : 8; /* 0 */

	uint32_t type : 8; /* device_type */
	void *device;
};

/* Memory structure transfered by the bootloader to the OS.
 * The bootloader/OS communication buffer shoud provide a pointer to this structure
 */
struct platform_description {
	/* Header */
	char magic[4]; /* 'TREE' */
	uint32_t major : 8; /* 1 */
	uint32_t minor : 8; /* 0 */

	/* Device-tree like */
	uint32_t devices_count : 8;
	struct platform_device *devices;
};
#endif /* __PLATFORM_DESC_H__ */
