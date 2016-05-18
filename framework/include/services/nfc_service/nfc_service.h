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

#ifndef __NFC_SERVICE_API_H__
#define __NFC_SERVICE_API_H__

#include "cfw/cfw.h"
#include "services/services_ids.h"

/**
 * @defgroup nfc_service_api NFC Service
 *
 * Define the interface for NFC service.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/nfc_service/nfc_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/nfc_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>NFC_SERVICE, NFC_SERVICE_IMPL, and more in Kconfig</tt>
 * <tr><th><b>Service Id</b>  <td><tt>NFC_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

#define MSG_ID_NFC_SERVICE_INIT_RSP         0x81 /**< Response to device init request */
#define MSG_ID_NFC_SERVICE_DISABLE_RSP      0x82 /**< Response to device disable request */
#define MSG_ID_NFC_SERVICE_SET_CE_MODE_RSP  0x83 /**< Response to card emulation request */
#define MSG_ID_NFC_SERVICE_START_RF_RSP     0x84 /**< Response to RF activation request */
#define MSG_ID_NFC_SERVICE_STOP_RF_RSP      0x85 /**< Response to RF deactivation request */
#define MSG_ID_NFC_SERVICE_HIBERNATE_RSP    0x86 /**< Response to Hibernate mode request */

#define MSG_ID_NFC_SERVICE_ERROR_EVT        0x1001 /**< Notification of an error event */
#define MSG_ID_NFC_SERVICE_NFCEE_EVT        0x1002 /**< Notification of a NFCEE detection */
#define MSG_ID_NFC_SERVICE_RF_EVT           0x1003 /**< Notification of RF state change */

/** NFC response/event status codes. */
enum NFC_STATUS {
	NFC_STATUS_SUCCESS = 0, /**< General NFC Success code */
	NFC_STATUS_PENDING,     /**< Request received and execution started, response pending */
	NFC_STATUS_TIMEOUT,     /**< Request timed out */
	NFC_STATUS_NOT_SUPPORTED, /**< Request/feature/parameter not supported */
	NFC_STATUS_NOT_ALLOWED, /**< Request not allowed, other request pending */
	NFC_STATUS_NOT_ENABLED, /**< NFC not enabled */
	NFC_STATUS_ERROR,       /**< Generic Error */
	NFC_STATUS_WRONG_STATE, /**< Wrong state for request */
	NFC_STATUS_ERROR_PARAMETER, /**< Parameter in request is wrong */
};

typedef struct nfc_service_rsp_msg {
	struct cfw_message header; /*!< Header message */
	uint8_t status;         /**< Status code of the command sent previously @ref NFC_STATUS */
} nfc_service_rsp_msg_t;

typedef struct nfc_service_rf_evt {
	struct cfw_message header; /**< Component framework message header (@ref cfw), MUST be first element of structure */
	uint8_t rf_type;       /**< RF event type - ex: interface activate, deactivated */
} nfc_service_rf_evt_t;

typedef struct nfc_service_nfcee_evt {
	struct cfw_message header; /**< Component framework message header (@ref cfw), MUST be first element of structure */
	uint8_t nfcee_event;   /**< NFCEE event type, ex 'detected', 'activated', 'action' */
	uint8_t nfcee_id;      /**< Id of the NFCEE */
} nfc_service_nfcee_evt_t;

typedef struct nfc_service_err_evt {
	struct cfw_message header; /**< Component framework message header (@ref cfw), MUST be first element of structure */
	uint8_t status;        /**< Status code relevant for the error @ref NFC_STATUS */
	uint8_t err_id;        /**< Error specific id, ex scenario fail code. */
} nfc_service_err_evt_t;

/**
 * Enable NFC controller and stack. To be called before any NFC service related call.
 *
 * This command will power up the controller and run the Card Emulation
 * configuration sequence. It will not start the RF frontend.
 * This has to be called only once, after which the RF interface can be turned
 * on or off as many times as it is necessary.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_INIT_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_init(cfw_service_conn_t *svc_handle, void *priv);

/**
 * Disable NFC controller and stack.
 * Cleans up the controller configuration and powers it down. Can be called
 * at any time, and it has higher priority than other commands.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_DISABLE_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_disable(cfw_service_conn_t *svc_handle, void *priv);

/**
 * Configure controller for Card Emulation mode
 * This command only configures the controller to allow for Card Emulation
 * mode via the embedded Secure Element.
 *
 * This will not start the RF loop.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_SET_CE_MODE_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_set_ce_mode(cfw_service_conn_t *svc_handle, void *priv);

/**
 * Start the RF loop.
 * Configures the RF interface and starts the RF polling loop. Only after this
 * command the RF interface is active.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_START_RF_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_start_rf(cfw_service_conn_t *svc_handle, void *priv);

/**
 * Stop the RF loop.
 * Stops the RF polling loop and returns the controller in idle mode.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_STOP_RF_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_stop_rf(cfw_service_conn_t *svc_handle, void *priv);

/**
 * Enter Hibernate mode.
 *
 * @param svc_handle Client svc handle (cfw handle)
 * @param priv Pointer to private structure passed back in the response
 *
 * @return @ref OS_ERR_TYPE
 *
 * @b Response: _MSG_ID_NFC_SERVICE_HIBERNATE_RSP_ with attached \ref nfc_service_rsp_msg_t
 */
int nfc_hibernate(cfw_service_conn_t *svc_handle, void *priv);

/** @} */

#endif
