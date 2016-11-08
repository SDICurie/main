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


#include "drivers/msc/msc.h"
#include "infra/part.h"

#define READ_ONLY 0

#define define_configuration(part_id, attr_value, filename_ptr) \
{ \
.partition_id = part_id, \
.attr = attr_value, \
.filename = filename_ptr, \
},

//// soc
//PART_RESERVED, /* Reserved area */
//PART_FACTORY_INTEL, /* Store Intel persistent factory data (OTP) */
//PART_FACTORY_OEM, /* Store OEM factory data (OTP) */
//PART_RESET_VECTOR, /* x86 Reset vector (OTP) */
//PART_BOOTLOADER, /* x86 bootloader */
//PART_PANIC, /* Panic dump area for debug (incl. x86 and Sensor subsystem) */
//PART_MAIN, /* x86 core main code */
//PART_SENSOR, /* Sensor subsystem code */
//PART_FACTORY_NPERSIST, /* Prop. service - Sensor, BLE, battery/ADC */
//PART_FACTORY_PERSIST, /* Prop. service - Sensor calibration, BLE IDs */
//PART_APP_DATA, /* x86 application data, can be erased */
//PART_FACTORY, /* Used? */

// spi
//PART_OTA_CACHE, /* Holds the OTA package for update */
//PART_APP_LOGS, /* Store the application logs */
//PART_ACTIVITY_DATA, /* Another partition to hold application data */
//PART_SYSTEM_EVENTS, /* Debug information (logs) */
//PART_RAWDATA_COLLECT, /* raw data collect partition*/

struct msc_partition_configuration msc_storage_config[] = {

define_configuration(PART_RESERVED, READ_ONLY, "reserved.soc")
define_configuration(PART_FACTORY_INTEL, READ_ONLY, "fct_intl.soc")
define_configuration(PART_FACTORY_OEM, READ_ONLY, "fct_oem.soc")
define_configuration(PART_RESET_VECTOR, READ_ONLY, "rst_vect.soc")
define_configuration(PART_BOOTLOADER, READ_ONLY, "bootldr.soc")
define_configuration(PART_PANIC, READ_ONLY, "panic.soc")
define_configuration(PART_MAIN, READ_ONLY, "x86.soc")
define_configuration(PART_SENSOR, READ_ONLY, "sensor.soc")
define_configuration(PART_FACTORY_NPERSIST, READ_ONLY, "fct_nprs.soc")
define_configuration(PART_FACTORY_PERSIST, READ_ONLY, "fct_prs.soc")
define_configuration(PART_APP_DATA, READ_ONLY, "app_data.soc")
define_configuration(PART_FACTORY, READ_ONLY, "factory.soc")

define_configuration(PART_OTA_CACHE, READ_ONLY, "ota.spi")
define_configuration(PART_APP_LOGS, READ_ONLY, "app_logs.spi")
define_configuration(PART_ACTIVITY_DATA, READ_ONLY, "act_data.spi")
define_configuration(PART_RAWDATA_COLLECT, READ_ONLY, "raw_data.spi")

};

int msc_partition_config_count = sizeof(msc_storage_config)/sizeof(struct msc_partition_configuration);
