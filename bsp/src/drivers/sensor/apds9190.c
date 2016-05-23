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

#include <zephyr.h>
#include "drivers/apds9190.h"
#include "infra/log.h"
#include "drivers/gpio.h"
#include "drivers/soc_comparator.h"
#include "machine.h"

#define SBA_TIMEOUT 100

#define REG_ENABLE         0x00
#define REG_PTIME          0x02
#define REG_WTIME          0x03
#define REG_PILT           0x08
#define REG_PIHT           0x0A
#define REG_PERS           0x0C
#define REG_CONFIG         0x0D
#define REG_PPCOUNT        0x0E
#define REG_CONTROL        0x0F
#define REG_REV            0x11
#define REG_STATUS         0x13
#define REG_PDATA          0x18

#define ENABLE_PIEN        (1 << 5)
#define ENABLE_WEN         (1 << 3)
#define ENABLE_PEN         (1 << 2)
#define ENABLE_PON         (1 << 0)

#define CONTROL_ENABLE_CH1 0x20

#define COMMAND_ENABLE     0x80
#define COMMAND_AI         0x20
#define COMMAND_CLEAR      0xE5

#define CONTROL_100mA      0x00
#define CONTROL_50mA       0x40
#define CONTROL_25mA       0x80
#define CONTROL_12mA       0xC0
#define CONTROL_ENABLE_CH1 0x20

/* Device driver internal functions */
static void apds_sba_completion_callback(struct sba_request *);
static DRIVER_API_RC i2c_sync_write(struct td_device *, int len, uint8_t *data);
static DRIVER_API_RC i2c_write_reg(struct td_device *dev, uint8_t reg,
				   uint8_t data);
static int apds9190_set_enable(struct td_device *dev, uint8_t reg);
static int apds9190_clear_irq(struct td_device *dev);
static void apds9190_set_threshold(struct td_device *dev, uint16_t l,
				   uint16_t h);
static void apds9190_update_config(struct td_device *dev);
static void comparator_cb(void *);

int apds9190_set_callback(struct td_device *device, void (*callback)(bool,
								     void *),
			  void *param)
{
	struct td_device *dev = (struct td_device *)device;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	uint32_t flags = irq_lock();

	ir_dev->cb = callback;
	ir_dev->cb_data = param;
	irq_unlock(flags);
	return 0;
}

static void apds_timer_cb(void *param)
{
	struct td_device *dev = (struct td_device *)param;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	uint16_t value = apds9190_read_prox(dev);

	pr_debug(LOG_MODULE_DRV, "Proximity: %d", value);

	if (!ir_dev->prox && value >= ir_dev->piht) {
		ir_dev->prox = true;
		goto call_user_cb;
	} else if (ir_dev->prox && value <= ir_dev->pilt) {
		ir_dev->prox = false;
		goto call_user_cb;
	}
	return;

call_user_cb:
	/* Call user callback */
	if (ir_dev->cb) {
		ir_dev->cb(ir_dev->prox, ir_dev->cb_data);
	}
}

static void apds_clear_irq_callback(struct sba_request *req)
{
	struct td_device *dev = (struct td_device *)req->priv_data;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	comp_configure(ir_dev->comparator_device,
		       ir_dev->comp_pin, 1, 1, comparator_cb, dev);
}

static void apds_update_threshold_callback(struct sba_request *req)
{
	struct td_device *dev = (struct td_device *)req->priv_data;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	ir_dev->irq_tx_buff[0] = COMMAND_CLEAR;
	ir_dev->irq_req.callback = apds_clear_irq_callback;
	ir_dev->irq_req.tx_len = 1;
	sba_exec_dev_request((struct sba_device *)dev, &ir_dev->irq_req);
}

static void comparator_cb(void *param)
{
	struct td_device *dev = (struct td_device *)param;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	/* Disable comparator */
	comp_disable(ir_dev->comp_pin);

	/* Update sensor state */
	ir_dev->prox = !ir_dev->prox;

	/* Update thresholds */
	uint16_t l, h;
	if (ir_dev->prox) {
		l = ir_dev->pilt;
		h = 0xFFFF;
	} else {
		l = 0;
		h = ir_dev->piht;
	}

	ir_dev->irq_tx_buff[1] = l;
	ir_dev->irq_tx_buff[2] = l >> 8;
	ir_dev->irq_tx_buff[3] = h;
	ir_dev->irq_tx_buff[4] = h >> 8;
	ir_dev->irq_tx_buff[0] = COMMAND_ENABLE | COMMAND_AI | REG_PILT;
	ir_dev->irq_req.callback = apds_update_threshold_callback;
	ir_dev->irq_req.tx_len = 5;
	sba_exec_dev_request((struct sba_device *)dev, &ir_dev->irq_req);

	/* Call user callback */
	if (ir_dev->cb) {
		ir_dev->cb(ir_dev->prox, ir_dev->cb_data);
	}
}

static int apds9190_init(struct td_device *device)
{
	int ret;
	struct sba_device *dev = (struct sba_device *)device;
	struct apds9190_info *ir_dev = (struct apds9190_info *)device->priv;

	pr_debug(LOG_MODULE_DRV, "APDS9190 %d init", device->id);

	/* Create a taken semaphore for sba transfers */
	if ((ir_dev->i2c_sync_sem = semaphore_create(0)) == NULL) {
		return -1;
	}
	/* Init sba_request struct */
	ir_dev->req.addr.slave_addr = dev->addr.slave_addr;
	ir_dev->req.request_type = SBA_TX;
	ir_dev->irq_req.addr.slave_addr = dev->addr.slave_addr;
	ir_dev->irq_req.request_type = SBA_TX;
	ir_dev->irq_req.tx_buff = ir_dev->irq_tx_buff;
	ir_dev->irq_req.rx_len = 0;
	ir_dev->irq_req.priv_data = dev;

	/* Stop device */
	ret = apds9190_set_enable(device, ENABLE_PON);

	if (ret != 0) {
		pr_error(LOG_MODULE_DRV, "apds9190 %d probe %d\n", device->id,
			 ret);
		return ret;
	}

	/* Default state is detached */
	ir_dev->prox = false;

	/* Enable comparator interrupt if available */
	if (ir_dev->comp_pin < 0) {
		/* No comparator configured, use polling mode */
		apds9190_update_config(device);
		goto apds_polling_mode;
	}

	ir_dev->comparator_device =
		(struct td_device *)&pf_device_soc_comparator;

	pr_info(LOG_MODULE_DRV, "apds: use interrupt mode");
	apds9190_update_config(device);
	apds9190_set_threshold(device, 0, ir_dev->piht);
	/* Enable apds9190 device in interrupt mode */
	apds9190_set_enable(device, ENABLE_PON |
			    ENABLE_PIEN |
			    ENABLE_WEN |
			    ENABLE_PEN);
	apds9190_clear_irq(device);
	comp_configure(ir_dev->comparator_device,
		       ir_dev->comp_pin, 1, 1, comparator_cb, device);

	return 0;

apds_polling_mode:
	pr_info(LOG_MODULE_DRV, "apds: use polling mode");
	/* Disable interrupt mode on apds device */
	apds9190_set_enable(device, ENABLE_PON |
			    ENABLE_WEN |
			    ENABLE_PEN);
	/* Create / start timer */
	timer_create(apds_timer_cb, (void *)device, 1000, true, true, NULL);

	return 0;
}

struct driver apds9190_driver = {
	.init = apds9190_init,
	.suspend = NULL,
	.resume = NULL
};

static void apds_sba_completion_callback(struct sba_request *req)
{
	/* Give led sba semaphore to notify that the i2c transfer is complete */
	semaphore_give((T_SEMAPHORE)(req->priv_data), NULL);
}

static int i2c_read_reg16(struct td_device *dev, uint8_t reg)
{
	DRIVER_API_RC ret;
	OS_ERR_TYPE ret_os;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;
	struct sba_request *req = &ir_dev->req;
	uint8_t rsp[2];
	uint8_t cmd = COMMAND_ENABLE | reg;

	req->request_type = SBA_TRANSFER;
	req->tx_buff = &cmd;
	req->tx_len = 1;
	req->rx_buff = rsp;
	req->rx_len = 2;
	req->priv_data = ir_dev->i2c_sync_sem;
	req->callback = apds_sba_completion_callback;
	req->full_duplex = 1;

	if ((ret =
		     sba_exec_dev_request((struct sba_device *)dev,
					  req)) == DRV_RC_OK) {
		/* Wait for transfer to complete (timeout = 100ms) */
		if ((ret_os =
			     semaphore_take(ir_dev->i2c_sync_sem,
					    SBA_TIMEOUT)) != E_OS_OK) {
			if (ret_os == E_OS_ERR_BUSY) {
				ret = DRV_RC_TIMEOUT;
			} else {
				ret = DRV_RC_FAIL;
			}
		} else {
			ret = req->status;
		}
	}

	if (ret < 0)
		return ret;
	else
		return rsp[0] | rsp[1] << 8;
}

uint8_t i2c_read_reg8(struct td_device *dev, uint8_t reg)
{
	DRIVER_API_RC ret;
	OS_ERR_TYPE ret_os;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;
	struct sba_request *req = &ir_dev->req;
	uint8_t rsp;
	uint8_t cmd = COMMAND_ENABLE | reg;

	req->request_type = SBA_TRANSFER;
	req->tx_buff = &cmd;
	req->tx_len = 1;
	req->rx_buff = &rsp;
	req->rx_len = 1;
	req->priv_data = ir_dev->i2c_sync_sem;
	req->callback = apds_sba_completion_callback;
	req->full_duplex = 1;

	if ((ret =
		     sba_exec_dev_request((struct sba_device *)dev,
					  req)) == DRV_RC_OK) {
		/* Wait for transfer to complete (timeout = 100ms) */
		if ((ret_os =
			     semaphore_take(ir_dev->i2c_sync_sem,
					    SBA_TIMEOUT)) != E_OS_OK) {
			if (ret_os == E_OS_ERR_BUSY) {
				ret = DRV_RC_TIMEOUT;
			} else {
				ret = DRV_RC_FAIL;
			}
		} else {
			ret = req->status;
		}
	}

	if (ret < 0)
		return ret;
	else
		return rsp;
}

static DRIVER_API_RC i2c_sync_write(struct td_device *dev, int len,
				    uint8_t *data)
{
	DRIVER_API_RC ret;
	OS_ERR_TYPE ret_os;
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;
	struct sba_request *req = &ir_dev->req;

	req->request_type = SBA_TX;
	req->priv_data = ir_dev->i2c_sync_sem;
	req->callback = apds_sba_completion_callback;
	req->tx_len = len;
	req->tx_buff = data;

	if ((ret =
		     sba_exec_dev_request((struct sba_device *)dev,
					  req)) == DRV_RC_OK) {
		/* Wait for transfer to complete (timeout = 100ms) */
		if ((ret_os =
			     semaphore_take(ir_dev->i2c_sync_sem,
					    SBA_TIMEOUT)) != E_OS_OK) {
			if (ret_os == E_OS_ERR_BUSY) {
				ret = DRV_RC_TIMEOUT;
			} else {
				ret = DRV_RC_FAIL;
			}
		} else {
			ret = req->status;
		}
	}

	return ret;
}

static DRIVER_API_RC i2c_write_reg(struct td_device *dev, uint8_t reg,
				   uint8_t data)
{
	uint8_t buf[2];

	buf[0] = COMMAND_ENABLE | reg;
	buf[1] = data;
	return i2c_sync_write(dev, 2, buf);
}

static int apds9190_clear_irq(struct td_device *dev)
{
	uint8_t cmd = COMMAND_CLEAR;

	return i2c_sync_write(dev, 1, &cmd);
}

static int apds9190_set_enable(struct td_device *dev, uint8_t reg)
{
	return i2c_write_reg(dev, REG_ENABLE, reg);
}

int apds9190_read_prox(struct td_device *dev)
{
	return i2c_read_reg16(dev, REG_PDATA);
}

static void apds9190_set_threshold(struct td_device *dev, uint16_t l,
				   uint16_t h)
{
	uint8_t cmd[5];

	/* Write thresholds */
	cmd[1] = l;
	cmd[2] = l >> 8;
	cmd[3] = h;
	cmd[4] = h >> 8;
	cmd[0] = COMMAND_ENABLE | COMMAND_AI | REG_PILT;
	i2c_sync_write(dev, 5, cmd);
}
void apds9190_update_config(struct td_device *dev)
{
	struct apds9190_info *ir_dev = (struct apds9190_info *)dev->priv;

	i2c_write_reg(dev, REG_PTIME, ir_dev->ptime);
	i2c_write_reg(dev, REG_WTIME, ir_dev->wtime);
	i2c_write_reg(dev, REG_PERS, ir_dev->pers);
	i2c_write_reg(dev, REG_CONFIG, ir_dev->config);
	i2c_write_reg(dev, REG_PPCOUNT, ir_dev->ppcount);
	i2c_write_reg(dev, REG_CONTROL, CONTROL_ENABLE_CH1 | CONTROL_12mA);
}

bool apds9190_get_status(struct td_device *device)
{
	struct apds9190_info *ir_dev = (struct apds9190_info *)device->priv;

	return ir_dev->prox;
}
