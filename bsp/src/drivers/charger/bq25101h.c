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

struct charger_cb_list {
	list_t next;
	void (*cb)(DRV_CHG_EVT, void *);
	void *priv;
};

/****************************************************************************************
*********************** LOCAL FUNCTON IMPLEMENTATION ***********************************
****************************************************************************************/

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
		CHARGING ? "BQ25101H is charging" : "BQ25101H is discharging");

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
	struct charger_info *priv = (struct charger_info *)param;

	if (pin_state)
		generic_callback(priv, DISCHARGING);
	else
		generic_callback(priv, CHARGING);
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

/****************************************************************************************
*********************** ACCESS FUNCTION IMPLEMENTATION *********************************
****************************************************************************************/
static int charger_init(struct td_device *dev)
{
	struct charger_info *priv = (struct charger_info *)dev->priv;

	list_init(&priv->cb_head);
	if (!priv->evt_dev)
		return -EINVAL;
	/* Attach callback to managed_comparator event */
	managed_comparator_register_callback(priv->evt_dev, charger_callback,
					     priv);
	/* Update state variable */
	charger_get_current_soc(dev);
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

	if (managed_comparator_get_state(priv->evt_dev))
		priv->state = DISCHARGING;
	else
		priv->state = CHARGING;
	return priv->state;
}

struct driver charger_driver =
{
	.init = charger_init,
	.suspend = NULL,
	.resume = NULL
};
