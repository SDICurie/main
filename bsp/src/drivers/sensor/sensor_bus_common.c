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

#include "util/compiler.h"
#include "machine.h"
#include "os/os.h"
#include "infra/log.h"
#include "infra/time.h"
#include "drivers/data_type.h"
#include "string.h"
#include "drivers/serial_bus_access.h"
#include "drivers/sensor/sensor_bus_common.h"

/************************* Use Serial Bus Access API *******************/

#define AON_CNT_TICK_PER_MS 33
#define SENSOR_BUS_TIMEOUT 30000 //30s

static void sensor_bus_callback_sleep(struct sba_request *req)
{
	struct sensor_sba_req *sensor_req = (struct sensor_sba_req *)req;

	semaphore_give(sensor_req->sem, NULL);
}

static void sensor_bus_callback_spin(struct sba_request *req)
{
	struct sensor_sba_req *sensor_req = (struct sensor_sba_req *)req;

	sensor_req->complete_flag++;
}

static uint8_t get_sba_req(uint8_t *req_bitmap, uint8_t req_count)
{
	uint8_t i;
	uint32_t saved = irq_lock();

	for (i = 0; i < req_count; i++) {
		if (!(*req_bitmap & (1 << i))) {
			*req_bitmap |= (1 << i);
			break;
		}
	}
	irq_unlock(saved);
	return i;
}

static void release_sba_req(uint8_t *req_bitmap, uint8_t i)
{
	uint32_t saved = irq_lock();

	*req_bitmap &= ~(1 << i);
	irq_unlock(saved);
}

static void config_req_read(uint8_t *tx_buffer, uint32_t tx_len,
			    uint8_t *rx_buffer, uint32_t rx_len,
			    sba_request_t *req)
{
	req->request_type = SBA_TRANSFER;
	req->tx_buff = tx_buffer;
	req->tx_len = tx_len;
	req->rx_buff = rx_buffer;
	req->rx_len = rx_len;
	req->status = 1;
}

static void config_req_write(uint8_t *tx_buffer, uint32_t tx_len,
			     sba_request_t *req)
{
	req->request_type = SBA_TX;
	req->tx_buff = tx_buffer;
	req->tx_len = tx_len;
	req->rx_len = 0;
	req->status = 1;
}

DRIVER_API_RC sensor_bus_access(struct sensor_sba_info *info,
				uint8_t *tx_buffer, uint32_t tx_len,
				uint8_t *rx_buffer,
				uint32_t rx_len, bool req_read,
				int slave_addr)
{
	int i;
	sba_request_t *req;
	DRIVER_API_RC ret = DRV_RC_OK;
	OS_ERR_TYPE err;

	if ((i = get_sba_req(&info->bitmap, info->req_cnt)) >= info->req_cnt) {
		pr_debug(LOG_MODULE_DRV, "%s:DEV[%d] No req left", __func__,
			 info->dev_id);
		return DRV_RC_FAIL;
	}

	req = &info->reqs[i].req;

	if (0 <= slave_addr)
		req->addr.cs = slave_addr;

	if (req_read)
		config_req_read(tx_buffer, tx_len, rx_buffer, rx_len, req);
	else
		config_req_write(tx_buffer, tx_len, req);

	if (sba_exec_request(req)) {
		pr_debug(LOG_MODULE_DRV, "%s:DEV[%d] request exec error",
			 __func__,
			 info->dev_id);
		release_sba_req(&info->bitmap, i);
		return DRV_RC_FAIL;
	}

	if (info->block_type == SLEEP) {
		if ((err =
			     semaphore_take(info->reqs[i].sem,
					    SENSOR_BUS_TIMEOUT))) {
			pr_debug(LOG_MODULE_DRV,
				 "%s:DEV[%d] take semaphore err#%d", __func__,
				 info->dev_id,
				 err);
			release_sba_req(&info->bitmap, i);
			return DRV_RC_FAIL;
		}
	} else {
		struct sensor_sba_req *sensor_req =
			(struct sensor_sba_req *)req;
		while (1) {
			if (sensor_req->complete_flag != 0) {
				sensor_req->complete_flag = 0;
				break;
			}
		}
	}

	if (req->status) {
		pr_error(LOG_MODULE_DRV, "%s:DEV[%d] state error", __func__,
			 info->dev_id);
		ret = DRV_RC_FAIL;
	}
	release_sba_req(&info->bitmap, i);

	return ret;
}

struct sensor_sba_info *sensor_config_bus(int slave_addr, uint8_t dev_id,
					  SBA_BUSID bus_id,
					  SENSOR_BUS_TYPE bus_type, int req_num,
					  BLOCK_TYPE block_type)
{
	struct sensor_sba_req *reqs = NULL;
	struct sensor_sba_info *info = NULL;
	int i;

	reqs = (struct sensor_sba_req *)balloc(
		sizeof(struct sensor_sba_req) * req_num, NULL);
	if (!reqs)
		return NULL;

	info = (struct sensor_sba_info *)balloc(sizeof(struct sensor_sba_info),
						NULL);
	if (!info)
		goto FAIL;

	info->reqs = reqs;
	info->bitmap = 0;
	info->req_cnt = req_num;
	info->dev_id = dev_id;
	info->bus_type = bus_type;
	info->block_type = block_type;

	for (i = 0; i < req_num; i++) {
		reqs[i].req.bus_id = bus_id;
		reqs[i].req.status = 1;

		if (block_type == SPIN) {
			reqs[i].req.callback = sensor_bus_callback_spin;
			reqs[i].complete_flag = 0;
		} else {
			reqs[i].req.callback = sensor_bus_callback_sleep;
			reqs[i].sem = semaphore_create(0);
			if (!reqs[i].sem)
				goto FAIL;
		}

		reqs[i].req.full_duplex = 0;

		reqs[i].req.addr.slave_addr = slave_addr;
	}

	pr_debug(LOG_MODULE_DRV, "%s: Serial BUS[%d] initialized", __func__,
		 bus_id);

	return info;

FAIL:
	for (i = 0; i < req_num; i++) {
		if (reqs[i].sem) {
			semaphore_delete(reqs[i].sem);
			reqs[i].sem = NULL;
		}
	}
	if (reqs)
		bfree(reqs);

	if (info)
		bfree(info);

	pr_debug(LOG_MODULE_DRV, "%s: Serial BUS[%d] init failed", __func__,
		 bus_id);
	return NULL;
}

static void wait_ticks(uint32_t ticks)
{
	uint32_t start = get_uptime_32k();

	while ((get_uptime_32k() - start) < ticks) ;
}

void sensor_delay_ms(uint32_t msecs)
{
	if (msecs >= CONVERT_TICKS_TO_MS(1)) {
		T_SEMAPHORE sem = semaphore_create(0);
/* Ensure that we will wait at least 'msecs' millisecond as if semaphore_take is called
 * right before the timer tick interrupt it will be released immediately */
		semaphore_take(sem, (int)msecs + 1);
		semaphore_delete(sem);
	} else {
		wait_ticks(msecs * AON_CNT_TICK_PER_MS);
	}
}
