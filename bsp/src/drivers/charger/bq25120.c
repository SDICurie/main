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

#include <errno.h>

#include "infra/log.h"
#include "drivers/charger/charger_api.h"
#include "drivers/managed_comparator.h"
#include "machine.h"
#include "infra/time.h"
#include "drivers/serial_bus_access.h"
#include "drivers/gpio.h"

#define GPIO_SOC_CD             (uint8_t)(1) /* TODO: put in defconfig */

#define CHARGER_I2C_ADD         0x6A    /* Charger I2C address */

#define BQ_GET_VALUE(reg, field) (reg &	\
				  (BIT_MASK_ ## field)) >> (BIT_POS_ ## field)   /* reg = value return by I2C read request, field = field to read in the corresponding register */
#define BQ_SET_VALUE(reg, value, field) (reg & \
					 ~(BIT_MASK_ ## field)) | \
	(value << (BIT_POS_ ## field))

#define REG_STATUS_ADD                  0x00    /* Status and Ship Mode control register */
#define REG_FAULT_ADD                   0x01    /* Fault and Faults masks register */
#define REG_TEMP_ADD                    0x02    /* TS Control and Faults masks register */
#define REG_FAST_CHG_ADD                0x03    /* Fast charge control register */
#define REG_PRE_TERM_CHG_ADD            0x04    /* Termination/Pre-charge register */
#define REG_VBREG_ADD                   0x05    /* Battery voltage control register */
#define REG_SYS_VOUT_ADD                0x06    /* SYS VOUT control register */
#define REG_LS_LDO_ADD                  0x07    /* Load switch and LDO control register */
#define REG_BUTTON_ADD                  0x08    /* Push button control register */
#define REG_ILIM_UVLO_ADD               0x09    /* ILIM and battery UVLO control register */
#define REG_VOLT_MONITOR_ADD            0x0A    /* Voltage based battery monitor register */
#define REG_VIN_TIMER_ADD               0x0B    /* VIN_DPM and timers register */

/* status register */
#define BIT_MASK_STATUS                 0xC0
#define BIT_POS_STATUS                  6
#define BIT_MASK_SHIPMODE               0x20
#define BIT_POS_SHIPMODE                5
#define BIT_MASK_RESET_FAULT            0x10
#define BIT_POS_RESET_FAULT             4
#define BIT_MASK_TIMER                  0x08
#define BIT_POS_TIMER                   3
#define BIT_MASK_VINDPM_STAT            0x04
#define BIT_POS_VINDPM_STAT             2
#define BIT_MASK_CD_STAT                0x02
#define BIT_POS_CD_STAT                 1
#define BIT_MASK_SYS_EN_STAT            0x01
#define BIT_POS_SYS_EN_STAT             0
/* Faults register */
#define BIT_MASK_VIN_OV                 0x80
#define BIT_POS_VIN_OV                  7
#define BIT_MASK_VIN_UV                 0x40
#define BIT_POS_VIN_UV                  6
#define BIT_MASK_BAT_UVLO               0x20
#define BIT_POS_BAT_UVLO                5
#define BIT_MASK_BAT_OCP                0x10
#define BIT_POS_BAT_OCP                 4
#define BIT_MASK_VIN_OV_M               0x08
#define BIT_POS_VIN_OV_M                3
#define BIT_MASK_VIN_UV_M               0x04
#define BIT_POS_VIN_UV_M                2
#define BIT_MASK_BAT_UVLO_M             0x02
#define BIT_POS_BAT_UVLO_M              1
#define BIT_MASK_BAT_OCP_M              0x01
#define BIT_POS_BAT_OCP_M               0
/* TS control register */
#define BIT_MASK_TS_EN                  0x80
#define BIT_POS_TS_EN                   7
#define BIT_MASK_TS_FAULT               0x60
#define BIT_POS_TS_FAULT                5
#define BIT_MASK_TS_FAULT_OPEN          0x10
#define BIT_POS_TS_FAULT_OPEN           4
#define BIT_MASK_EN_INT                 0x08
#define BIT_POS_EN_INT                  3
#define BIT_MASK_WAKE_M                 0x04
#define BIT_POS_WAKE_M                  2
#define BIT_MASK_RESET_M                0x02
#define BIT_POS_RESET_M                 1
#define BIT_MASK_TIMER_M                0x01
#define BIT_POS_TIMER_M                 0
/* fast charge control register*/
#define BIT_MASK_ICHRG_RANGE            0x80
#define BIT_POS_ICHRG_RANGE             7
#define BIT_MASK_ICHRG                  0x7C
#define BIT_POS_ICHG                    2
#define BIT_MASK_CE                     0x02
#define BIT_POS_CE                      1
#define BIT_MASK_HZ_MODE                0x01
#define BIT_POS_HZ_MODE                 0
/* Termination/Pre-charge register */
#define BIT_MASK_IPRETERM_RANGE         0x80
#define BIT_POS_IPRETERM_RANGE          7
#define BIT_MASK_IPRETERM               0x7C
#define BIT_POS_IPRETERM                2
#define BIT_MASK_TE                     0x01
#define BIT_POS_TE                      1
/* Battery voltage control register */
#define BIT_MASK_VBREG                  0xFE
#define BIT_POS_VBREG                   1
/* SYS VOUT control register */
#define BIT_MASK_EN_SYS_VOUT            0x80
#define BIT_POS_EN_SYS_VOUT             7
#define BIT_MASK_SYS_SEL                0x60
#define BIT_POS_SYS_SEL                 5
#define BIT_MASK_SYS_VOUT               0x1E
#define BIT_POS_SYS_VOUT                1
/* Load switch and LDO control register */
#define BIT_MASK_EN_LS_LDO              0x80
#define BIT_POS_EN_LS_LDO               7
#define BIT_MASK_LS_LDO                 0x7C
#define BIT_POS_LS_LDO                  2
#define BIT_MASK_MRRESET_VIN            0x01
#define BIT_POS_MRRESET_VIN             0
/* Push button control register */
#define BIT_MASK_MRWAKE1                0x80
#define BIT_POS_MRWAKE1                 7
#define BIT_MASK_MRWAKE2                0x40
#define BIT_POS_MRWAKE2                 6
#define BIT_MASK_MRREC                  0x20
#define BIT_POS_MRREC                   5
#define BIT_MASK_MRRESET                0x18
#define BIT_POS_MRRESET                 3
#define BIT_MASK_PGB_MR                 0x04
#define BIT_POS_PGB_MR                  2
#define BIT_MASK_WAKE1                  0x02
#define BIT_POS_WAKE1                   1
#define BIT_MASK_WAKE2                  0x01
#define BIT_POS_WAKE2                   0
/* ILIM and battery UVLO control register */
#define BIT_MASK_RESET                  0x80
#define BIT_POS_RESET                   7
#define BIT_MASK_INLIM                  0x38
#define BIT_POS_INLIM                   3
#define BIT_MASK_UVLO                   0x07
#define BIT_POS_UVLO                    0
/* Voltage based battery monitor register */
#define BIT_MASK_VBMON_READ             0x80
#define BIT_POS_VBMON_READ              7
#define BIT_MASK_VBMON_RANGE            0x60
#define BIT_POS_VBMON_RANGE             5
#define BIT_MASK_VBMON_TH               0x1C
#define BIT_POS_VBMON_TH                2
/* VIN_DPM and timers register */
#define BIT_MASK_VINDPM_ON              0x80
#define BIT_POS_VINDPM_ON               7
#define BIT_MASK_VINDPM                 0x70
#define BIT_POS_VINDPM                  4
#define BIT_MASK_2XTMR_EN               0x08
#define BIT_POS_2XTMR_EN                3
#define BIT_MASK_TMR                    0x06
#define BIT_POS_TMR                     1


#define REG_TEMP_VALUE          0x08;   /* Disable TS function */
#define REG_ICHRG_VALUE         0x44;   /* Set Fast charge control register to 22mA */
#define REG_IPRETERM_VALUE      0x26;   /* Around 20% of ISET = 4.5mA */
#define REG_VB_VALUE            0x96;   /* Set Battery voltage control register to 4.35V */
#define REG_SHIPMODE_EN         0x20;   /* Enable ship mode */

/* Define timers */
#define CH_TM_DELAY_200ms (200)
#define CH_TM_DELAY_20s (20000) /* < charger_watchdog_timer(50s) / 2 */
#define CH_TM_DELAY_10s (10000) /* < charger_fault_timer(10s) */

#define CH_HIZ_ACTIVEBAT (1) /* 1ms to pass from hi Z to active battery */
/* Define call back function for charging_sm_event */
typedef void (*ch_event_fct)(void *charging_sm_event);

/* Timer definition */
static T_TIMER i2c_wd_timer;    /* Timer to send an I2C request to the charger for reset
                                 *              this internal watchdog (50sec) */
static T_TIMER ch_fault_timer;  /* Timer to manage charger fault */

static bool charger_ready;
static bool sba_busy;   /* Use to block a new SBA request if the preview isn't done */
static struct sba_request req;
static uint8_t tx_buff[5];
static uint8_t rx_buff[5];

struct charger_cb_list {
	list_t next;
	void (*cb)(DRV_CHG_EVT, void *);
	void *priv;
};

/****************************************************************************************
*********************** LOCAL FUNCTON IMPLEMENTATION ***********************************
****************************************************************************************/

static void generic_callback(struct charger_info *priv, DRV_CHG_EVT evt);
static void ch_i2c_config(void);
static void ch_i2c_get_fault(void);
static void ch_i2c_get_status(void);
static void charger_reset(void);

/*** I2C ***/

/**@generic function to call sba request and manage error
 * @param[in] SBA request message pointer
 */
static void ch_i2c_sba_exec_request(sba_request_t *request)
{
	DRIVER_API_RC result;

	result = sba_exec_request(request);
	if (result)
		pr_error(LOG_MODULE_DRV, "charger sba request failed: RC = %d",
			 result);
}

/**@brief Function to release sba blocker and restart i2c_wd_timer.
 */
static void ch_release_i2c(void)
{
	sba_busy = false;
	timer_stop(i2c_wd_timer);
	timer_start(i2c_wd_timer, CH_TM_DELAY_20s, NULL);
}

/**@brief Function to send an I2C request to the charger
 * for reset this internal watchdog.
 */
static void ch_i2c_reset_watchdog()
{
	static struct sba_request wd_req;
	static uint8_t wd_tx_buff[1];
	static uint8_t wd_rx_buff[1];

	wd_tx_buff[0] = REG_STATUS_ADD;
	wd_rx_buff[0] = 0;
	wd_req.bus_id = SBA_I2C_MASTER_1;
	wd_req.addr.slave_addr = CHARGER_I2C_ADD;
	wd_req.tx_buff = wd_tx_buff;
	wd_req.rx_buff = wd_rx_buff;
	wd_req.request_type = SBA_TRANSFER;
	wd_req.tx_len = 1;
	wd_req.rx_len = 1;
	wd_req.callback = NULL;

	sba_exec_request(&wd_req);
}

/**@brief Callback function for ch_i2c_get_fault() function.
 * @param[in] SBA request message pointer
 */
static void ch_i2c_get_fault_callback(struct sba_request *req)
{
	bool status;

	ch_release_i2c();
	pr_error(LOG_MODULE_DRV,
		 "BQ25120 Read status: %d\nVoltage Fault: %x TS Fault: %x",
		 req->status, rx_buff[0],
		 rx_buff[1]);

	status = BQ_GET_VALUE(rx_buff[0], BAT_UVLO);
	if (status)
		timer_start(ch_fault_timer, CH_TM_DELAY_10s, NULL);
}

/**@brief Function to read charger voltage/temperature faults by I2C.
 */
static void ch_i2c_get_fault(void)
{
	if (!sba_busy) {
		sba_busy = true;
		tx_buff[0] = REG_FAULT_ADD;
		rx_buff[0] = 0;

		req.request_type = SBA_TRANSFER;
		req.tx_len = 1;
		req.rx_len = 2;
		req.callback = ch_i2c_get_fault_callback;

		ch_i2c_sba_exec_request(&req);
	} else
		pr_error(LOG_MODULE_DRV, "BQ25120 sba busy");
}

/**@brief Callback function for ch_i2c_get_status() and
 * send charger event by callback to state machine
 * @param[in] SBA request message pointer
 */
static void ch_i2c_get_status_callback(struct sba_request *req)
{
	struct charger_info *priv = req->priv_data;
	static uint8_t status;

	ch_release_i2c();

	if (req->status == DRV_RC_OK) {
		status = BQ_GET_VALUE(rx_buff[0], STATUS);
		if (status == CHARGER_FAULT) {
			status = CHARGER_FAULT + priv->state;
			ch_i2c_get_fault();
		}
		generic_callback(priv, status);
	}
}

/**@brief Function read charger status by I2C.
 */
static void ch_i2c_get_status(void)
{
	if (!sba_busy) {
		sba_busy = true;
		tx_buff[0] = REG_STATUS_ADD;
		rx_buff[0] = 0;         /* clean buffer */

		req.request_type = SBA_TRANSFER;
		req.tx_len = 1;
		req.rx_len = 1;
		req.callback = ch_i2c_get_status_callback;

		ch_i2c_sba_exec_request(&req);
	} else
		pr_error(LOG_MODULE_DRV, "BQ25120 sba busy");
}

/**@brief Callback function for ch_i2c_default_config()
 * @param[in] SBA request message pointer
 */
static void charger_default_config_callback(struct sba_request *req)
{
	ch_release_i2c();
	if (req->status != DRV_RC_OK)
		pr_error(LOG_MODULE_DRV, "BQ25120 I2C default config failed");
}

/**@brief Function set the default config of bq25120.
 */
static void charger_default_config(void)
{
	if (!sba_busy) {
		sba_busy = true;
		tx_buff[0] = REG_TEMP_ADD;
		tx_buff[1] = REG_TEMP_VALUE;

		req.request_type = SBA_TX;
		req.tx_len = 2;
		req.callback = charger_default_config_callback;

		ch_i2c_sba_exec_request(&req);
	} else
		pr_error(LOG_MODULE_DRV, "BQ25120 sba busy");
}
/**@brief Callback function for ch_i2c_config()
 * @param[in] SBA request message pointer
 */
static void ch_i2c_config_callback(struct sba_request *req)
{
	ch_release_i2c();
	if (req->status == DRV_RC_OK) {
		charger_enable();
		charger_ready = true;
	} else
		ch_i2c_config();
}

/**@brief Function to make the charger configuration by I2C.
 */
static void ch_i2c_config(void)
{
	if (!sba_busy) {
		charger_disable();
		sba_busy = true;
		tx_buff[0] = REG_FAST_CHG_ADD;
		tx_buff[1] = REG_ICHRG_VALUE;
		tx_buff[2] = REG_IPRETERM_VALUE;
		tx_buff[3] = REG_VB_VALUE;
		req.request_type = SBA_TX;
		req.tx_len = 4;
		req.callback = ch_i2c_config_callback;

		ch_i2c_sba_exec_request(&req);
	} else
		pr_error(LOG_MODULE_DRV, "BQ25120 sba busy");
}

/**@brief Callback function for charger_reset()
 * @param[in] SBA request message pointer
 */
static void charger_reset_callback(struct sba_request *req)
{
	ch_release_i2c();
	if (req->status != DRV_RC_OK)
		pr_error(LOG_MODULE_DRV, "BQ25120 I2C reset failed");
	else
		charger_default_config();
}

/**@brief Function to reset charger through I2C.
 */
static void charger_reset(void)
{
	if (!sba_busy) {
		sba_busy = true;
		tx_buff[0] = REG_ILIM_UVLO_ADD;
		tx_buff[1] = 0x80;

		req.request_type = SBA_TX;
		req.tx_len = 2;
		req.callback = charger_reset_callback;

		ch_i2c_sba_exec_request(&req);
	} else
		pr_error(LOG_MODULE_DRV, "BQ25120 sba busy");
}

/*** OTHER ***/

static void ch_fault_timer_handler()
{
	timer_stop(ch_fault_timer);
	ch_i2c_get_status();
}

/**@brief Function to call selected registered user callback.
 * @param[in]  Index of callback list
 * @param[in]  Parameter passed by callback (enum DRV_CHG_EVT)
 * @return   none.
 */
static void call_user_callback(void *item, void *param)
{
	((struct charger_cb_list *)item)->cb(
		(DRV_CHG_EVT)param,
		((struct charger_cb_list *)item)->
		priv);
}

/**@brief Function to start call of all registered user callback.
 * @param[in]  pointer of driver device parameters
 * @param[in]  Parameter passed by callback (enum DRV_CHG_EVT)
 * @return   none.
 */
static void generic_callback(struct charger_info *priv, DRV_CHG_EVT evt)
{
	pr_debug(
		LOG_MODULE_DRV, evt ==
		CHARGING ? "BQ25120 is charging" : "BQ25120 is discharging");

	/* Call user callbacks */
	list_foreach(&priv->cb_head, call_user_callback, (void *)evt);
}

/**@brief Callback function for managed_comparator event.
 * @param[in]  state of pin
 * @param[in]  pointer of driver device parameters
 * @return   none.
 */
static void charger_callback(bool pin_state, void *param)
{
	/* TODO: Charger behavior without I2C */
	ch_i2c_get_status();
}

/**@brief Function to check if the callback in parameter and list are equal.
 * @param[in]  Index of callback list
 * @param[in]  Pointer of callback function
 * @return   true if equal, false otherwise.
 */
static bool check_item_callback(list_t *item, void *cb)
{
	return ((struct charger_cb_list *)item)->cb == cb;
}

/**@brief Function to configure the GPIO which controls the charger.
 */
static void ch_gpio_cd_conf(void)
{
	gpio_cfg_data_t pin_cfg = {
		.gpio_type = GPIO_OUTPUT,
		.gpio_cb = NULL
	};

	gpio_set_config(&pf_device_soc_gpio_32, GPIO_SOC_CD, &pin_cfg);
}

/****************************************************************************************
*********************** ACCESS FUNCTION IMPLEMENTATION *********************************
****************************************************************************************/
static int charger_init(struct td_device *dev)
{
	OS_ERR_TYPE ch_tm_error;
	struct charger_info *priv = (struct charger_info *)dev->priv;

	list_init(&priv->cb_head);
	if (!priv->evt_dev)
		return -EINVAL;

	charger_ready = false;

	/* configure sba i2c request */
	sba_busy = false;
	req.bus_id = SBA_I2C_MASTER_1;
	req.addr.slave_addr = CHARGER_I2C_ADD;
	req.tx_buff = tx_buff;
	req.rx_buff = rx_buff;
	req.priv_data = priv;

	/* create timer for watchdog reset */
	i2c_wd_timer =
		timer_create(ch_i2c_reset_watchdog, NULL, CH_TM_DELAY_20s,
			     true, true,
			     &ch_tm_error);

	/* create a timer for fault management */
	ch_fault_timer =
		timer_create(ch_fault_timer_handler, NULL, CH_TM_DELAY_10s,
			     false, false,
			     &ch_tm_error);

	/* configure io to drive CD charger signal */
	ch_gpio_cd_conf();

	/* disable charger to be able to communicate through i2c when usb is not plugged */
	charger_disable();

	/* get initial state of charge status pin*/
	charger_get_current_soc(dev);

	charger_reset();

	/* Attach callback to managed_comparator event */
	managed_comparator_register_callback(priv->evt_dev, charger_callback,
					     priv);

	return 0;
}

void charger_register_callback(struct td_device *dev,
			       void (*cb)(DRV_CHG_EVT, void *), void *priv)
{
	struct charger_info *charger_dev =
		(struct charger_info *)dev->priv;

	/* Alloc memory for cb and priv argument */
	struct charger_cb_list *item =
		(struct charger_cb_list *)balloc(sizeof(struct charger_cb_list),
						 NULL);

	/* Fill new allocated item */
	item->priv = priv;
	item->cb = cb;
	/* Add item in list */
	list_add_head(&charger_dev->cb_head, (list_t *)item);
}

int charger_unregister_callback(struct td_device *dev, void (*cb)(DRV_CHG_EVT,
								  void *))
{
	struct charger_info *info_dev = (struct charger_info *)dev->priv;

	list_t *element =
		list_find_first(&info_dev->cb_head, check_item_callback,
				(void *)cb);

	/* Element not found */
	if (element == NULL)
		return -1;

	/* Remove list element */
	list_remove(&info_dev->cb_head, element);
	bfree(element);

	return 0;
}

DRV_CHG_EVT charger_get_current_soc(struct td_device *dev)
{
	struct charger_info *priv = (struct charger_info *)dev->priv;

	if (!charger_ready) {
		if (managed_comparator_get_state(priv->evt_dev))
			priv->state = DISCHARGING;
		else
			priv->state = CHARGING;
	}
	return priv->state;
}

void charger_enable(void)
{
	gpio_write(&pf_device_soc_gpio_32, GPIO_SOC_CD, 0);
}

void charger_disable(void)
{
	uint32_t start_time;

	gpio_write(&pf_device_soc_gpio_32, GPIO_SOC_CD, 1);
	start_time = get_uptime_ms();
	/* spin 1 ms to wait until the charger is connected */
	while ((get_uptime_ms() - start_time) < CH_HIZ_ACTIVEBAT) ;
}

void charger_config(void)       /* TODO param et defconfig variable par default */
{
	ch_i2c_config();
}

struct driver charger_driver =
{
	.init = charger_init,
	.suspend = NULL,
	.resume = NULL
};
