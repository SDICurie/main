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

#ifndef _MSC_H
#define _MSC_H

#include "drivers/data_type.h"

#define MSC_BUFFER_LENGTH 512 // maximum size of data packet to be tranfered

#define MSD_IN_EP_ADDR (UE_DIR_IN | 0x01) // Address of MSC in-endpoint
#define MSD_OUT_EP_ADDR (UE_DIR_OUT | 0x01) // Address of MSC out-endpoint

/* Stucture to handle information of exposing partition with MSC device */
struct msc_partition_configuration {
	uint16_t partition_id; /* IDs the bootloader and applications/OS's use to address partitions */
	uint8_t attr; /* Access permision attribute, 0 - read only */
	char *filename; /*Filename of partition in filesystem */
	struct partition *flash_partition; /*Pointer to partition structure defined in infra/part.h */
};

/* Project MSC partition configuration */
extern struct msc_partition_configuration msc_storage_config[];
/* Project MSC partitions count */
extern int msc_partition_config_count;

/**
 * MSC driver initialization function.
 * This function should be called by the platform initialization.
 *
 * @param priv pointer to user defined data
 * @return  none
 */
void msc_class_init(void *priv);

#endif /* _MSC_H */
