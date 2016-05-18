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

#include "drivers/serial_bus_access.h"
#include "adxl362_support.h"
#include "drivers/sensor/sensor_bus_common.h"
#define ADXL362_BUS_WRITE_BUFFER_SIZE 4

/************************* Use Serial Bus Access API *******************/

extern struct sensor_sba_info *ohrm_sba_info;

DRIVER_API_RC adxl362_bus_read(uint8_t reg_addr, uint8_t *reg_data, uint8_t cnt)
{
	uint8_t reg_buf[2];

	reg_buf[0] = ADXL362_READ_REG;
	reg_buf[1] = reg_addr;
	return sensor_bus_access(ohrm_sba_info, reg_buf, 2, reg_data, cnt, true,
				 ADXL362_SLAVE_ADDR);
}

DRIVER_API_RC adxl362_bus_fifo_read(uint8_t *fifo_data, uint8_t cnt)
{
	uint8_t reg_buf;

	reg_buf = ADXL362_READ_FIFO;
	return sensor_bus_access(ohrm_sba_info, &reg_buf, 1, fifo_data, cnt,
				 true,
				 ADXL362_SLAVE_ADDR);
}

DRIVER_API_RC adxl362_bus_write(uint8_t reg_addr, uint8_t *reg_data,
				uint8_t cnt)
{
	uint8_t buffer[ADXL362_BUS_WRITE_BUFFER_SIZE];

	buffer[0] = ADXL362_WRITE_REG;
	buffer[1] = reg_addr;
	memcpy(&buffer[2], reg_data, cnt);
	return sensor_bus_access(ohrm_sba_info, buffer, 2 + cnt, NULL, 0, false,
				 ADXL362_SLAVE_ADDR);
}
