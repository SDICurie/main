/*
 ****************************************************************************
 * Copyright 2012(c) Analog Devices, Inc.
 *
 * File : adxl362_regs.h
 *
 * Usage: Register File of ADXL362 DRIVER.
 *
 ****************************************************************************
 * License:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   Neither the name of the copyright holder nor the names of the
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 * The information provided is believed to be accurate and reliable.
 * The copyright holder assumes no responsibility
 * for the consequences of use
 * of such information nor for any infringement of patents or
 * other rights of third parties which may result from its use.
 * No license is granted by implication or otherwise under any patent or
 * patent rights of the copyright holder.
 **************************************************************************/
#ifndef __ADXL362_REGS_H__
#define __ADXL362_REGS_H__
/* Instructions */
#define ADXL362_WRITE_REG              0x0A
#define ADXL362_READ_REG               0x0B
#define ADXL362_READ_FIFO              0x0D

/* Const Registers                     address     R/W */
#define ADXL362_REG_DEVID_AD           0x00     /* R */
#define ADXL362_REG_DEVID_MST          0x01     /* R */
#define ADXL362_REG_PARTID             0x02     /* R */
#define ADXL362_REG_REVID              0x03     /* R */

/* RO Registers                        address     R/W */
#define ADXL362_REG_XDATA8             0x08     /* R */
#define ADXL362_REG_YDATA8             0x09     /* R */
#define ADXL362_REG_ZDATA8             0x0A     /* R */
#define ADXL362_REG_STATUS             0x0B     /* R */
#define ADXL362_REG_FIFO_ENTRIES_L     0x0C     /* R */
#define ADXL362_REG_FIFO_ENTRIES_H     0x0D     /* R */
#define ADXL362_REG_XDATA_L            0x0E     /* R */
#define ADXL362_REG_XDATA_H            0x0F     /* R */
#define ADXL362_REG_YDATA_L            0x10     /* R */
#define ADXL362_REG_YDATA_H            0x11     /* R */
#define ADXL362_REG_ZDATA_L            0x12     /* R */
#define ADXL362_REG_ZDATA_H            0x13     /* R */
#define ADXL362_REG_TEMP_L             0x14     /* R */
#define ADXL362_REG_TEMP_H             0x15     /* R */
#define ADXL362_REG_Reserved1          0x16     /* R */
#define ADXL362_REG_Reserved2          0x17     /* R */

/* Control/Config Registers            address     R/W */
#define ADXL362_REG_SOFT_RESET         0x1F     /* W */
#define ADXL362_REG_THRESH_ACT_L       0x20     /* RW */
#define ADXL362_REG_THRESH_ACT_H       0x21     /* RW */
#define ADXL362_REG_TIME_ACT           0x22     /* RW */
#define ADXL362_REG_THRESH_INACT_L     0x23     /* RW */
#define ADXL362_REG_THRESH_INACT_H     0x24     /* RW */
#define ADXL362_REG_TIME_INACT_L       0x25     /* RW */
#define ADXL362_REG_TIME_INACT_H       0x26     /* RW */
#define ADXL362_REG_ACT_INACT_CTL      0x27     /* RW */
#define ADXL362_REG_FIFO_CONTROL       0x28     /* RW */
#define ADXL362_REG_FIFO_SAMPLES       0x29     /* RW */
#define ADXL362_REG_INTMAP1            0x2A     /* RW */
#define ADXL362_REG_INTMAP2            0x2B     /* RW */
#define ADXL362_REG_FILTER_CTL         0x2C     /* RW */
#define ADXL362_REG_POWER_CTL          0x2D     /* RW */
#define ADXL362_REG_SELF_TEST          0x2E     /* RW */

/* Key Value in soft reset */
#define ADXL362_SOFT_RESET_KEY         0x52

/* ACT_INACT_CTL bit setting defines */
#define ADXL362_ACT_ENABLE             0x01
#define ADXL362_ACT_DISABLE            0x00
#define ADXL362_ACT_AC                 0x02
#define ADXL362_ACT_DC                 0x00
#define ADXL362_INACT_ENABLE           0x04
#define ADXL362_INACT_DISABLE          0x00
#define ADXL362_INACT_AC               0x08
#define ADXL362_INACT_DC               0x00
#define ADXL362_ACT_INACT_LINK         0x10
#define ADXL362_ACT_INACT_LOOP         0x20

/* FIFO bit setting defines */
#define ADXL362_FIFO_MODE_OFF          0x00
#define ADXL362_FIFO_MODE_FIFO         0x01
#define ADXL362_FIFO_MODE_STREAM       0x02
#define ADXL362_FIFO_MODE_TRIGGER      0x03
#define ADXL362_FIFO_TEMP              0x04
#define ADXL362_FIFO_SAMPLES_AH        0x08

/* INTMAP1 and INTMAP2 bit setting defines */
#define ADXL362_INT_DATA_READY         0x01
#define ADXL362_INT_FIFO_READY         0x02
#define ADXL362_INT_FIFO_WATERMARK     0x04
#define ADXL362_INT_FIFO_OVERRUN       0x08
#define ADXL362_INT_ACT                0x10
#define ADXL362_INT_INACT              0x20
#define ADXL362_INT_AWAKE              0x40
#define ADXL362_INT_LOW                0x80

/* Filter Control bit setting defines */
#define ADXL362_RATE_400               0x05
#define ADXL362_RATE_200               0x04
#define ADXL362_RATE_100               0x03   /* default */
#define ADXL362_RATE_50                0x02
#define ADXL362_RATE_25                0x01
#define ADXL362_RATE_12_5              0x00
#define ADXL362_EXT_TRIGGER            0x08
#define ADXL362_AXIS_X                 0x00
#define ADXL362_AXIS_Y                 0x10
#define ADXL362_AXIS_Z                 0x20
#define ADXL362_RANGE_2G               0x00
#define ADXL362_RANGE_4G               0x40
#define ADXL362_RANGE_8G               0x80

/* Power Control bit setting defines */
#define ADXL362_STANDBY                0x00
#define ADXL362_MEASURE                0x02
#define ADXL362_AUTO_SLEEP             0x04
#define ADXL362_SLEEP                  0x08
#define ADXL362_LOW_POWER              0x00
#define ADXL362_LOW_NOISE1             0x10
#define ADXL362_LOW_NOISE2             0x20
#define ADXL362_LOW_NOISE3             0x30
#define ADXL362_EXT_CLOCK              0x40

/*  bit setting defines */
#define ADXL362_SELFTEST_ON            0x01
#define ADXL362_SELFTEST_OFF           0x00

/* Expected Register IDs */
#define ADXL362_DEVID_AD               (0xAD)
#define ADXL362_DEVID_MST              (0x1D)
#define ADXL362_PART_ID                (0xF2)

#define ADXL362_ID_TEST                (ADXL362_DEVID_AD | ADXL362_DEVID_MST <<	\
					8 | ADXL362_PART_ID << 16)
#endif
