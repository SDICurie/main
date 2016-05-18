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

#include "util/assert.h"
#include "machine/soc/intel/quark_se/quark/log_backend_flash.h"
#include "machine/soc/intel/quark_se/soc_config.h"
#include "project_mapping.h"

#include <board.h>
#include <infra/device.h>
#include <infra/log.h>
#include <drivers/spi_flash.h>


#define FLASH_SECTOR_SIZE       SERIAL_FLASH_BLOCK_SIZE
#define LOG_FLASH_ADDRESS_START (SPI_LOG_START_BLOCK * SERIAL_FLASH_BLOCK_SIZE)
#define LOG_FLASH_SECTOR_START  (SPI_LOG_START_BLOCK)
#define FLASH_LOG_SECTOR_COUNT  (SPI_LOG_END_BLOCK)
#define READ_BLOCK_LEN          256
#define READ_BLOCK_COUNT        (FLASH_SECTOR_SIZE / READ_BLOCK_LEN)
#define SPARE                   0xFFFFFFFF

static uint32_t flash_index; /* current write pointer to flash */
static int current_index = 0; /* current write pointer in header contains all flash_index */
static int current_sector = 0; /* current write sector */

static struct td_device *spi_dev;

void log_backend_flash_init()
{
	spi_dev = (struct td_device *)&pf_sba_device_flash_spi0;
	uint32_t data[READ_BLOCK_LEN / 4] = { 0 };
	unsigned int retlen;
	int i, j;

	current_sector = 1;
	flash_index = LOG_FLASH_ADDRESS_START + current_sector *
		      FLASH_SECTOR_SIZE;
	current_index = 0;

	/* TODO check if OTA pacakage and erase block */
	for (i = READ_BLOCK_COUNT; i > 0; i--) {
		spi_flash_read(spi_dev, READ_BLOCK_LEN * (i - 1),
			       READ_BLOCK_LEN / 4,
			       &retlen,
			       data);
		assert(retlen == READ_BLOCK_LEN / 4);

		for (j = retlen; j > 0; j--) {
			if (data[j - 1] != SPARE) {
				flash_index = data[j - 1];
				current_index =
					(i - 1) * (READ_BLOCK_LEN / 4) + j;
				break;
			}
		}
		if (flash_index != LOG_FLASH_ADDRESS_START + current_sector *
		    FLASH_SECTOR_SIZE)
			break;
	}
}

static void spi_flash_puts(const char *s, uint16_t len)
{
	unsigned int wlen = 0;
	int next_sector;

	if ((flash_index + len) >
	    (FLASH_LOG_SECTOR_COUNT * FLASH_SECTOR_SIZE)) {
		flash_index = FLASH_SECTOR_SIZE;
	}

	spi_flash_write_byte(spi_dev, LOG_FLASH_ADDRESS_START + flash_index,
			     len, &wlen,
			     (uint8_t *)s);
	assert(wlen == len);
	flash_index += wlen;

	if (current_index >= READ_BLOCK_LEN / 4 * READ_BLOCK_COUNT) {
		spi_flash_sector_erase(spi_dev, LOG_FLASH_SECTOR_START, 1);
		current_index = 0;
	}
	spi_flash_write_byte(spi_dev, LOG_FLASH_ADDRESS_START +
			     (current_index * 4),
			     sizeof(flash_index), &wlen,
			     (uint8_t *)&flash_index);
	current_index++;

	next_sector = flash_index / FLASH_SECTOR_SIZE;
	if (next_sector != current_sector) {
		current_sector = next_sector;
		/* Prepare one erased sector */
		spi_flash_sector_erase(spi_dev, LOG_FLASH_SECTOR_START +
				       ((next_sector +
					 1) % FLASH_LOG_SECTOR_COUNT), 1);
	}
	;
}

static bool is_spi_flash_ready(void)
{
	return true;
}

struct log_backend log_backend_flash = {
	.put_one_msg = spi_flash_puts,
	.is_backend_ready = is_spi_flash_ready
};
