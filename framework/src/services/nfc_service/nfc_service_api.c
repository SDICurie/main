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

#include "cfw/cfw_service.h"
#include "services/nfc_service/nfc_service.h"
#include "nfc_service_private.h"

int nfc_init(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_INIT, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}

int nfc_disable(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_DISABLE, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}

int nfc_set_ce_mode(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_SET_CE_MODE, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}

int nfc_start_rf(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_START_RF, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}

int nfc_stop_rf(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_STOP_RF, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}

int nfc_hibernate(cfw_service_conn_t *h, void *priv)
{
	struct cfw_message *msg = cfw_alloc_message_for_service(
		h, MSG_ID_NFC_SERVICE_HIBERNATE, sizeof(*msg), priv);

	cfw_send_message(msg);
	return 0;
}
