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

/* usb msc driver */
#include "drivers/usb_api.h"
#include "drivers/msc/msc.h"
#include "machine.h"
/* infra */
#include "infra/log.h"
#include "infra/bsp.h"
#include "infra/xloop.h"
#include "cfw/cfw.h"
/* Watchdog helper */
#include "infra/wdt_helper.h"
#include "main_event_handler.h"

#define WRITE_DATA_ON_FLASH

#ifdef WRITE_DATA_ON_FLASH
#include "infra/part.h"
#include "infra/partition.h"
#include "util/assert.h"
#include "storage.h"
#include "drivers/soc_flash.h"
#include "drivers/spi_flash.h"
#endif

#ifdef WRITE_DATA_ON_FLASH
#define pr_debug_main(...) //pr_info(LOG_MODULE_MAIN, __VA_ARGS__)
#endif

#define NOTIF_DELAY_MS 5000

/* System main queue it will be used on the component framework to add messages
 * on it. */
static T_QUEUE queue;
static xloop_t loop;

int wdt_func(void *param)
{
	/* Acknowledge the system watchdog to prevent panic and reset */
	wdt_keepalive();

	return 0; /* Continue */
}

#ifdef WRITE_DATA_ON_FLASH

#define TST_LEN 200
uint32_t write_data[TST_LEN];

static __maybe_unused int32_t soc_flash_0_write(uint32_t		address,
		uint32_t		data_size,
		uint8_t *		data)
{
	unsigned int data_write_len;

	/* data_size shall be 4 bytes aligned */
	if (data_size % 4 ||
			soc_flash_write(address,
							data_size / 4,
							&data_write_len,
							(uint32_t *)data)) {
		pr_debug_main("SOC Write Failed");
		return -1;
	}
	return 0;
}

static __maybe_unused int32_t soc_flash_0_erase(uint32_t		first_block_to_erase,
		uint32_t		nb_blocks_to_erase)
{
	if (soc_flash_block_erase(first_block_to_erase,
							  nb_blocks_to_erase) != DRV_RC_OK) {
		return -1;
	}

	return 0;
}

void write_to_soc_flash(struct partition *flash_partition)
{
	uint32_t first_block = 0;
	uint32_t block_count = 0;

	assert(flash_partition != NULL);

	first_block = flash_partition->start /
				  flash_partition->storage->block_size;

	block_count = flash_partition->size /
				  flash_partition->storage->block_size;

	if(soc_flash_0_erase(first_block, 1))
	{
		pr_debug_main("SOC Erase Failed");
	}

	if(soc_flash_0_write(flash_partition->start, TST_LEN*4, (uint8_t *)write_data))
	{
		pr_debug_main("SOC Write Failed");
	}
}

void write_to_spi_flash(struct partition *flash_partition)
{
	struct td_device *spi_flash_handler = (struct td_device *)&pf_sba_device_flash_spi0;
	unsigned int retlen;

	uint32_t first_block = 0;
	uint32_t block_count = 0;

	assert(flash_partition != NULL);

	/* flash_partition->storage->block_size is set to 4096 (that is sector size for W25Q16DV)
	and is defined in bsp/bootable/bootloader/chip/intel/quark_se/partition.c */

	first_block = flash_partition->start /
				  flash_partition->storage->block_size;

	block_count = flash_partition->size /
				  flash_partition->storage->block_size;

	pr_debug_main("spi_flash_sector_erase");

	if(spi_flash_sector_erase(spi_flash_handler, first_block, 1))
	{
		pr_debug_main("SPI Erase Failed");
	}

	pr_debug_main( "spi_flash_sector_erase done");

	if(spi_flash_write(spi_flash_handler, flash_partition->start, TST_LEN, &retlen, write_data))
	{
		pr_debug_main("SPI Write Failed");
	}

	pr_debug_main("spi_flash_write done");
}


void write_to_flash(uint8_t id)
{
	uint8_t i;
	struct partition *flash_partition;

	for (i = 0; i < TST_LEN; i++) {
		write_data[i] = i;
	}

	write_data[0] = 0xdeadface;
	write_data[1] = 0xface1010;
	write_data[2] = 0xc0cac01a;


	flash_partition = partition_get(id);

	assert(flash_partition != NULL);

	pr_debug_main("write_to_flash partition_id is 0x%x", flash_partition->partition_id);
	pr_debug_main("write_to_flash storage_id is 0x%x", flash_partition->storage->id);

	switch(flash_partition->storage->id) {
		case INTERNAL_FLASH_0:
			write_to_soc_flash(flash_partition);
			break;
		case SPI_FLASH_0:
			write_to_spi_flash(flash_partition);
			break;
	}
}
#endif



void main_task(void *param)
{
	/* Init BSP (also init BSP on ARC core) */
	queue = bsp_init();
	pr_info(LOG_MODULE_MAIN, "BSP init done");

	/* start Quark watchdog */
	wdt_start(WDT_MAX_TIMEOUT_MS);

	/* Init the CFW */
	cfw_init(queue);
	pr_info(LOG_MODULE_MAIN, "CFW init done");

	xloop_init_from_queue(&loop, queue);

	/* Main Event Handler */
	main_event_handler_init(&loop);

#ifdef WRITE_DATA_ON_FLASH
	write_to_flash(PART_ACTIVITY_DATA);
	write_to_flash(PART_PANIC);
#endif

	usb_register_function_driver(msc_class_init, NULL);
	//usb_register_function_driver(usb_acm_class_init, NULL);
	usb_driver_init(SOC_USB_BASE_ADDR);

	xloop_post_func_periodic(&loop, wdt_func, NULL, WDT_MAX_TIMEOUT_MS / 2);
	pr_info(LOG_MODULE_MAIN, "Initialization done, go to main loop");

	xloop_run(&loop);
}
