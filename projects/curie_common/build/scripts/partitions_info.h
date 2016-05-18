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

#include "project_mapping.h"

/* BLE has its own flash unit */
/* BLE Core chip has 256KiB Flash, but no blocks. Emulating them here for genericity */
#define BLE_CORE_FLASH_ID                               3
#define BLE_CORE_FLASH_NB_BLOCKS                        256
#define BLE_CORE_FLASH_BLOCK_SIZE                       1024

/* BLE partitions are not defined in the mapping */
#define BLE_APP_FLASH_ID           BLE_CORE_FLASH_ID
#define BLE_APP_START_BLOCK        0
#define BLE_APP_NB_BLOCKS          BLE_CORE_FLASH_NB_BLOCKS
#define BLE_APP_END_BLOCK          (BLE_APP_START_BLOCK + BLE_APP_NB_BLOCKS - 1)

/* Those Partitions were not defined the same way as all others */
#define FSBL_FLASH_FLASH_ID           EMBEDDED_OTP_FLASH_ID
#define FSBL_FLASH_START_BLOCK        0
#define FSBL_FLASH_NB_BLOCKS          7
#define FSBL_FLASH_END_BLOCK          (FSBL_FLASH_START_BLOCK +	\
				       FSBL_FLASH_NB_BLOCKS - 1)

#define SSBL_FLASH_FLASH_ID           EMBEDDED_FLASH_ID
#define SSBL_FLASH_START_BLOCK        0
#define SSBL_FLASH_NB_BLOCKS          30
#define SSBL_FLASH_END_BLOCK          (SSBL_FLASH_START_BLOCK +	\
				       SSBL_FLASH_NB_BLOCKS - 1)

#define QUARK_FLASH_FLASH_ID           EMBEDDED_FLASH_ID
#define QUARK_FLASH_START_BLOCK        QUARK_START_PAGE
#define QUARK_FLASH_NB_BLOCKS          QUARK_NB_PAGE
#define QUARK_FLASH_END_BLOCK          (QUARK_START_PAGE + QUARK_NB_PAGE - 1)

#define ARC_FLASH_FLASH_ID           EMBEDDED_FLASH_ID
#define ARC_FLASH_START_BLOCK        ARC_START_PAGE
#define ARC_FLASH_NB_BLOCKS          ARC_NB_PAGE
#define ARC_FLASH_END_BLOCK          (ARC_START_PAGE + ARC_NB_PAGE - 1)

#define DEFINE_FLASH_CHIP(flash_flag, nice_name)      \
	flash_chip = { 'name': # flash_flag,		 \
		       'nice_name': nice_name,			    \
		       'flash_id': flash_flag ## _ID,		      \
		       'nb_blocks': flash_flag ## _NB_BLOCKS,	      \
		       'block_size': flash_flag ## _BLOCK_SIZE	      \
	}

#define DEFINE_PARTITION(partition_flag)	       \
	partition = { 'name': # partition_flag,		  \
		      'flash_id': partition_flag ## _FLASH_ID,	      \
		      'start_block': partition_flag ## _START_BLOCK,  \
		      'end_block': partition_flag ## _END_BLOCK	      \
	}

/* Below this point, it generates an array easy to extract with python */
DEFINE_FLASH_CHIP(EMBEDDED_OTP_FLASH, "Embedded OTP Flash");
DEFINE_FLASH_CHIP(EMBEDDED_FLASH, "Embedded Flash");
DEFINE_FLASH_CHIP(SERIAL_FLASH, "Serial Flash");
DEFINE_FLASH_CHIP(BLE_CORE_FLASH, "BLE core Flash");

DEFINE_PARTITION(FSBL_FLASH);
DEFINE_PARTITION(SSBL_FLASH);
DEFINE_PARTITION(DEBUGPANIC);
DEFINE_PARTITION(QUARK_FLASH);
DEFINE_PARTITION(ARC_FLASH);
DEFINE_PARTITION(FACTORY_RESET_NON_PERSISTENT);
DEFINE_PARTITION(FACTORY_RESET_PERSISTENT);
DEFINE_PARTITION(APPLICATION_DATA);
DEFINE_PARTITION(FACTORY_SETTINGS);
DEFINE_PARTITION(SPI_FOTA);
DEFINE_PARTITION(SPI_APPLICATION_DATA);
DEFINE_PARTITION(SPI_SYSTEM_EVENT);
DEFINE_PARTITION(BLE_APP);
