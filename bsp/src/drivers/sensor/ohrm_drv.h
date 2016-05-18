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
#ifndef __OHRM_DRV_H__
#define __OHRM_DRV_H__
#include "sensors/phy_sensor_api/phy_sensor_drv_api.h"
#include "machine.h"
#include "microkernel.h"
#include "nanokernel.h"
#include "util/assert.h"

#include "bmi160_support.h"
#include "adxl362_support.h"
#include "drivers/sensor/sensor_bus_common.h"
#include "drivers/sensor/ohrm_optic.h"
#include "drivers/serial_bus_access.h"
#include "drivers/ss_adc.h"

#include "infra/ipc.h"
#include "infra/device.h"

#define OHRM_ADC_CHANNEL 12
#define DAC_NORMAL         0x40
#define DAC_SHUTDOWN       0x46

#define DCP_ACR_ADDRESS    0x10
#define DCP_WR0_ADDRESS    0x00
#define DCP_WR1_ADDRESS    0x01
#define DCP_WRITE          0xc0
#define DCP_READ           0x08
#define DCP_WR_INIT        0xff



#define ADXL362_FRAME_SIZE     6
#define ADXL362_WATERMARK_CNT  1
#define DAC_SLAVE_ADDR      SPI_SE_3
#define DIGPOT_SLAVE_ADDR      SPI_SE_2
extern struct sensor_sba_info *ohrm_sba_info;
#endif
