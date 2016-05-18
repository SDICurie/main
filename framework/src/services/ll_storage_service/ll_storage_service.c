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

#include "infra/log.h"

#include "cfw/cfw.h"
#include "cfw/cfw_service.h"

#include "machine.h"
#include "storage.h"
#include "project_mapping.h"
#include "drivers/soc_flash.h"
#include "drivers/spi_flash.h"
#include "services/services_ids.h"
#include "services/ll_storage_service/ll_storage_service.h"
#include "ll_storage_service_private.h"

/****************************************************************************************
************************** SERVICE INITIALIZATION **************************************
****************************************************************************************/

static void ll_storage_client_connected(conn_handle_t *instance);
static void ll_storage_client_disconnected(conn_handle_t *instance);
static void handle_message(struct cfw_message *msg, void *param);

static service_t ll_storage_service = {
	.service_id = LL_STOR_SERVICE_ID,
	.client_connected = ll_storage_client_connected,
	.client_disconnected = ll_storage_client_disconnected,
};

extern const flash_device_t flash_devices[];



static struct {
	flash_partition_t *partitions;
	uint8_t no_part;
} ll_storage_config;

DEFINE_LOG_MODULE(LOG_MODULE_LL_STORAGE_SERVICE, "LLST")

/* Init and Configure partitions seen by the Storage Service. */
static void ll_storage_service_init(int service_id, void *queue)
{
	cfw_register_service(queue, &ll_storage_service, handle_message, NULL);

	/* Flash memory partitioning array */
	extern flash_partition_t storage_configuration[];

	ll_storage_config.partitions = storage_configuration;
	ll_storage_config.no_part = NUMBER_OF_PARTITIONS;
}

CFW_DECLARE_SERVICE(ll_storage, LL_STOR_SERVICE_ID, ll_storage_service_init);


/*******************************************************************************
 *********************** SERVICE IMPLEMENTATION ********************************
 ******************************************************************************/
static void ll_storage_client_connected(conn_handle_t *instance)
{
	pr_debug(LOG_MODULE_LL_STORAGE_SERVICE, "%s: ", __func__);
}

static void ll_storage_client_disconnected(conn_handle_t *instance)
{
	pr_debug(LOG_MODULE_LL_STORAGE_SERVICE, "%s: ", __func__);
}

void handle_erase_block(struct cfw_message *msg)
{
	ll_storage_erase_block_req_msg_t *req =
		(ll_storage_erase_block_req_msg_t *)msg;
	ll_storage_service_erase_block_rsp_msg_t *resp =
		(ll_storage_service_erase_block_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_LL_STORAGE_SERVICE_ERASE_BLOCK_RSP,
			sizeof(*resp));

	flash_device_t flash;
	uint16_t flash_id = 0;
	int16_t partition_index = -1;
	uint32_t i = 0;
	DRIVER_API_RC ret = DRV_RC_FAIL;

	if (req->no_blks == 0) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Write Block: Invalid write size");
		ret = DRV_RC_INVALID_OPERATION;
		goto send;
	}

	for (i = 0; i < ll_storage_config.no_part; i++)
		if (ll_storage_config.partitions[i].partition_id ==
		    req->partition_id) {
			flash_id = ll_storage_config.partitions[i].flash_id;
			partition_index = i;
			break;
		}

	if (partition_index == -1) {
		pr_debug(
			LOG_MODULE_LL_STORAGE_SERVICE,
			"LL Storage Service - Write Block: Invalid partition ID");
		ret = DRV_RC_FAIL;
		goto send;
	}

	uint16_t last_block =
		ll_storage_config.partitions[partition_index].start_block +
		req->st_blk + req->no_blks - 1;

	if (last_block >
	    ll_storage_config.partitions[partition_index].end_block) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Write Block: Partition overflow");
		ret = DRV_RC_OUT_OF_MEM;
		goto send;
	}

	flash = flash_devices[flash_id];

	if (flash.flash_location == EMBEDDED_FLASH) {
		ret =
			soc_flash_block_erase(
				ll_storage_config.partitions[partition_index].
				start_block +
				req->st_blk, req->no_blks);
	}
#ifdef CONFIG_SPI_FLASH
	else {
		// SERIAL_FLASH
		ret = spi_flash_sector_erase(
			(struct td_device *)&pf_sba_device_flash_spi0,
			ll_storage_config.partitions[
				partition_index].start_block + req->st_blk,
			req->no_blks);
	}
#endif

send:
	resp->status = ret;
	cfw_send_message(resp);
}

void handle_write_partition(struct cfw_message *msg)
{
	ll_storage_write_partition_req_msg_t *req =
		(ll_storage_write_partition_req_msg_t *)msg;
	ll_storage_service_write_rsp_msg_t *resp =
		(ll_storage_service_write_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_LL_STORAGE_SERVICE_WRITE_RSP,
			sizeof(*resp));

	flash_device_t flash;
	uint16_t flash_id = 0;
	int16_t partition_index = -1;
	uint32_t i = 0;
	uint32_t size = 0;
	unsigned int retlen = 0;
	DRIVER_API_RC ret = DRV_RC_FAIL;

	if ((req->write_type == WRITE_REQ) && (req->buffer == NULL)) {
		pr_debug(
			LOG_MODULE_LL_STORAGE_SERVICE,
			"LL Storage Service - Write Data: Invalid write buffer");
		ret = DRV_RC_INVALID_CONFIG;
		goto send;
	}

	if ((req->write_type == WRITE_REQ) && (req->size == 0)) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Write Data: Invalid write size");
		ret = DRV_RC_INVALID_OPERATION;
		goto send;
	}

	for (i = 0; i < ll_storage_config.no_part; i++)
		if (ll_storage_config.partitions[i].partition_id ==
		    req->partition_id) {
			flash_id = ll_storage_config.partitions[i].flash_id;
			partition_index = i;
			break;
		}

	if (partition_index == -1) {
		pr_debug(
			LOG_MODULE_LL_STORAGE_SERVICE,
			"LL Storage Service - Write Data: Invalid partition ID");
		ret = DRV_RC_FAIL;
		goto send;
	}

	flash = flash_devices[flash_id];

	if ((req->write_type == WRITE_REQ) &&
	    (((ll_storage_config.partitions[partition_index].start_block *
	       flash.block_size) + (req->st_offset + req->size))
	     > ((ll_storage_config.partitions[partition_index].end_block +
		 1) * flash.block_size))) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Write Data: Partition overflow");
		ret = DRV_RC_OUT_OF_MEM;
		goto send;
	}

	size = req->size / sizeof(uint32_t) + !!(req->size % sizeof(uint32_t));

	if (req->write_type == ERASE_REQ) {
		if (flash.flash_location == EMBEDDED_FLASH) {
			ret = soc_flash_block_erase(
				ll_storage_config.partitions[partition_index].
				start_block,
				(ll_storage_config.
				 partitions[partition_index
				 ].end_block -
				 ll_storage_config.
				 partitions[partition_index].
				 start_block) + 1);
#ifdef CONFIG_SPI_FLASH
		} else { // SERIAL_FLASH
			ret = spi_flash_sector_erase(
				(struct td_device *)&pf_sba_device_flash_spi0,
				ll_storage_config.
				partitions[partition_index].start_block,
				(ll_storage_config.
				 partitions[
					 partition_index].end_block -
				 ll_storage_config.
				 partitions[partition_index].
				 start_block) + 1);
#endif
		}
	} else {
		uint32_t address =
			((ll_storage_config.partitions[partition_index].
			  start_block *
			  flash.block_size) + req->st_offset);

		if (flash.flash_location == EMBEDDED_FLASH) {
			ret = soc_flash_write(address, size, &retlen,
					      req->buffer);
#ifdef CONFIG_SPI_FLASH
		} else { // SERIAL_FLASH
			ret = spi_flash_write(
				(struct td_device *)&pf_sba_device_flash_spi0,
				address, size, &retlen,
				req->buffer);
#endif
		}

		resp->actual_size =
			(retlen *
			 sizeof(uint32_t)) - (((req->size % sizeof(uint32_t)))
					      ? (4 -
						 (req->size %
						  sizeof(uint32_t))) :
					      0);
	}

send:
	resp->status = ret;
	resp->write_type = req->write_type;
	cfw_send_message(resp);
}

void handle_read_partition(struct cfw_message *msg)
{
	ll_storage_read_partition_req_msg_t *req =
		(ll_storage_read_partition_req_msg_t *)msg;
	ll_storage_service_read_rsp_msg_t *resp =
		(ll_storage_service_read_rsp_msg_t *)cfw_alloc_rsp_msg(
			msg,
			MSG_ID_LL_STORAGE_SERVICE_READ_RSP,
			sizeof(*resp));

	flash_device_t flash;
	uint16_t flash_id = 0;
	int16_t partition_index = -1;
	uint32_t i = 0;
	uint32_t size = 0;
	unsigned int retlen = 0;
	DRIVER_API_RC ret = DRV_RC_FAIL;

	if (req->size == 0) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Read Data: Invalid size");
		ret = DRV_RC_INVALID_OPERATION;
		goto send;
	}

	for (i = 0; i < ll_storage_config.no_part; i++)
		if (ll_storage_config.partitions[i].partition_id ==
		    req->partition_id) {
			flash_id = ll_storage_config.partitions[i].flash_id;
			partition_index = i;
			break;
		}

	if (partition_index == -1) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Read Data: Invalid partition ID");
		ret = DRV_RC_FAIL;
		goto send;
	}

	flash = flash_devices[flash_id];

	if (((ll_storage_config.partitions[partition_index].start_block *
	      flash.block_size) + (req->st_offset + req->size))
	    > ((ll_storage_config.partitions[partition_index].end_block +
		1) * flash.block_size)) {
		pr_debug(LOG_MODULE_LL_STORAGE_SERVICE,
			 "LL Storage Service - Read Data: Partition overflow");
		ret = DRV_RC_OUT_OF_MEM;
		goto send;
	}

	size = (req->size / sizeof(uint32_t)) + !!(req->size % sizeof(uint32_t));

	uint32_t address =
		((ll_storage_config.partitions[partition_index].start_block *
		  flash.block_size) + req->st_offset);
	resp->buffer = balloc(size * sizeof(uint32_t), NULL);

	if (flash.flash_location == EMBEDDED_FLASH) {
		ret = soc_flash_read(address, size, &retlen, resp->buffer);
#ifdef CONFIG_SPI_FLASH
	} else { // SERIAL_FLASH
		ret = spi_flash_read(
			(struct td_device *)&pf_sba_device_flash_spi0,
			address, size, &retlen, resp->buffer);
#endif
	}

	resp->actual_read_size =
		(retlen * sizeof(uint32_t)) - (((req->size % sizeof(uint32_t)))
					       ? (4 -
						  (req->size %
						   sizeof(uint32_t))) : 0);

send:
	resp->status = ret;
	cfw_send_message(resp);
}

static void handle_message(struct cfw_message *msg, void *param)
{
	switch (CFW_MESSAGE_ID(msg)) {
	case MSG_ID_LL_ERASE_BLOCK_REQ:
		handle_erase_block(msg);
		break;
	case MSG_ID_LL_READ_PARTITION_REQ:
		handle_read_partition(msg);
		break;
	case MSG_ID_LL_WRITE_PARTITION_REQ:
		handle_write_partition(msg);
		break;
	default:
		cfw_print_default_handle_error_msg(
			LOG_MODULE_LL_STORAGE_SERVICE, CFW_MESSAGE_ID(msg));
		break;
	}

	cfw_msg_free(msg);
}
