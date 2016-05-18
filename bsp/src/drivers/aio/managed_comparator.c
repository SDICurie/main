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
#include "drivers/managed_comparator.h"
#include "drivers/soc_comparator.h"

struct managed_comparator_cb_list {
	list_t next;
	void (*cb)(bool, void *);
	void *priv;
};

static int managed_comparator_init(struct td_device *dev);
static void managed_comparator_callback(void *priv_data);

static void call_user_callback(void *item, void *param)
{
	((struct managed_comparator_cb_list *)item)->cb((bool)param,
							((struct
							  managed_comparator_cb_list
							  *)item)->priv);
}

static void pin_debounce_timer_handler(void *priv_data)
{
	struct managed_comparator_info *priv =
		(struct managed_comparator_info *)priv_data;

	priv->pin_deb_counter = 0;

	/* Call user callbacks */
	list_foreach(&priv->cb_head, call_user_callback,
		     (void *)priv->pin_status);
}

static void managed_comparator_callback(void *priv_data)
{
	struct managed_comparator_info *priv =
		(struct managed_comparator_info *)priv_data;

	if (priv->pin_deb_counter > 0)
		timer_stop(priv->pin_debounce_timer);

	priv->pin_status = !priv->pin_status;
	comp_configure(priv->evt_dev, priv->source_pin,
		       priv->pin_status ? 1 : 0, 1, managed_comparator_callback,
		       priv_data);
	timer_start(priv->pin_debounce_timer, priv->debounce_delay, NULL);

	priv->pin_deb_counter = priv->pin_deb_counter + 1;
	if (priv->pin_deb_counter == 10) {
		timer_stop(priv->pin_debounce_timer);
		pr_error(LOG_MODULE_DRV, "pin %d unexpected toggling",
			 priv->source_pin);
	}
}

static int managed_comparator_init(struct td_device *dev)
{
	struct managed_comparator_info *priv =
		(struct managed_comparator_info *)dev->priv;
	int ret = 0;

	priv->pin_debounce_timer =
		timer_create(pin_debounce_timer_handler, priv,
			     priv->debounce_delay, false,
			     false,
			     NULL);
	priv->pin_deb_counter = 0;
	priv->pin_status = false;
	list_init(&priv->cb_head);

	if (!priv->evt_dev)
		return -EINVAL;
	ret = comp_configure(priv->evt_dev, priv->source_pin, 0, 1,
			     managed_comparator_callback, priv);
	return ret;
}

bool managed_comparator_get_state(struct td_device *dev)
{
	struct managed_comparator_info *priv =
		(struct managed_comparator_info *)dev->priv;

	return priv->pin_status;
}

void managed_comparator_register_callback(struct td_device *dev,
					  void (*cb)(bool, void *), void *priv)
{
	struct managed_comparator_info *managed_comparator_dev =
		(struct managed_comparator_info *)dev->priv;

	/* Alloc memory for cb and priv argument */
	struct managed_comparator_cb_list *item =
		(struct managed_comparator_cb_list *)balloc(sizeof(struct
								   managed_comparator_cb_list),
							    NULL);

	/* Fill new allocated item */
	item->priv = priv;
	item->cb = cb;
	/* Add item in list */
	list_add_head(&managed_comparator_dev->cb_head, (list_t *)item);
}

static bool check_item_callback(list_t *item, void *cb)
{
	return ((struct managed_comparator_cb_list *)item)->cb == cb;
}

int managed_comparator_unregister_callback(struct td_device *dev, void (*cb)(
						   bool, void *))
{
	struct managed_comparator_info *info_dev =
		(struct managed_comparator_info *)dev->priv;

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

struct driver managed_comparator_driver =
{
	.init = managed_comparator_init,
	.suspend = NULL,
	.resume = NULL
};
