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


#ifndef __PART_H__
#define __PART_H__

#include <stdint.h>

#include "platform-desc.h"

/* Offsets and sizes are specified in bytes */
struct partition {
	uint32_t partition_id;
	uint64_t start;
	uint64_t size;
	uint64_t attributes;
	char *name;
	struct storage_device *storage;
};

/* Attributes of a partition */
enum partition_attributes {
	ATTR_NONE = 0,
	ATTR_RESERVED = 1 << 0,
	ATTR_OTP = 1 << 1,
	ATTR_CLEAR_FACTORY_RESET = 1 << 2,

	/* OEM-defined attributes start here */
	ATTR_OEM = 1 << 30,
	ATTR_NB = 1 << 31,
};

/* IDs the bootloader and applications/OS's use to address partitions */
enum partition_ids {
	PART_RESERVED, /* Reserved area */
	PART_FACTORY_INTEL, /* Store Intel persistent factory data (OTP) */
	PART_FACTORY_OEM, /* Store OEM factory data (OTP) */
	PART_RESET_VECTOR, /* x86 Reset vector (OTP) */
	PART_BOOTLOADER, /* x86 bootloader */
	PART_PANIC, /* Panic dump area for debug (incl. x86 and Sensor subsystem) */
	PART_MAIN, /* x86 core main code */
	PART_SENSOR, /* Sensor subsystem code */
	PART_FACTORY_NPERSIST, /* Prop. service - Sensor, BLE, battery/ADC */
	PART_FACTORY_PERSIST, /* Prop. service - Sensor calibration, BLE IDs */
	PART_APP_DATA, /* x86 application data, can be erased */
	PART_FACTORY, /* Used? */
	PART_OTA_CACHE, /* Holds the OTA package for update */
	PART_APP_LOGS, /* Store the application logs */
	PART_ACTIVITY_DATA, /* Another partition to hold application data */
	PART_SYSTEM_EVENTS, /* Debug information (logs) */
	PART_RAWDATA_COLLECT, /* raw data collect partition*/

	/* OEM-defined attributes start here */
	PART_OEM = 0x80,
};

#endif /* __PART_H__ */
