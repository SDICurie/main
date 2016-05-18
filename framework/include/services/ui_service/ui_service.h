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

#ifndef _UI_SERVICE_H
#define _UI_SERVICE_H

#include <stdint.h>

#include "cfw/cfw_service.h"

#include "services/services_ids.h"
#include "util/misc.h"
#include "drivers/led/led.h"
#include "drivers/haptic.h"
#include "lib/button/button.h"

/**
 * @defgroup ui_service UI Service
 * Handles User Interface events and notifications.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt>\#include "services/ui_service/ui_service.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>framework/src/services/ui_service</tt>
 * <tr><th><b>Config flag</b> <td><tt>UI_SERVICE, UI_SERVICE_IMPL, and more in Kconfig</tt>
 * <tr><th><b>Service Id</b>  <td><tt>UI_SVC_SERVICE_ID</tt>
 * </table>
 *
 * @ingroup services
 * @{
 */

/* Uncomment when testing the UI Service: */
#define TEST_UI_SVC 1

#ifdef TEST_UI_SVC
#define TEST_SVC_ID (MAX_SERVICES)
#define UI_TEST_EVT_IDX UI_EVT_IDX_BASE << 15
#endif

/**
 * List of status and error codes.
 */
#define UI_SUCCESS        0
#define UI_ERROR         -1
#define UI_BUSY          -2
#define UI_NOTFOUND      -3
#define UI_OPENSVC_FAIL  -4
#define UI_EVTREQ_FAIL   -5
#define UI_INVALID_PARAM -6
#define UI_INVALID_REQ   -7
#define UI_NO_DRV        -8
#define UI_ABORT         -9

/**
 * List of drivers identifiers.
 */
#define UI_DRV_ID_BASE           0x0300
#define UI_DRV_DEVICE_BIT_OFFSET 8
#define UI_DRV_DEVICE_MASK       ((1 << UI_DRV_DEVICE_BIT_OFFSET) - 1)
#define UIDRV_BTN_BASE           (UI_DRV_ID_BASE + \
				  (3 << UI_DRV_DEVICE_BIT_OFFSET))

/**
 * Indexes (bit position) for available events in the events masks.
 */
#define UI_EVT_IDX_BASE           0x0001
#define UI_BTN_SINGLE_EVT_IDX     UI_EVT_IDX_BASE << 3
#define UI_BTN_DOUBLE_EVT_IDX     UI_EVT_IDX_BASE << 4
#define UI_BTN_MAX_EVT_IDX        UI_EVT_IDX_BASE << 5
#define UI_BTN_MULTIPLE_EVT_IDX   UI_EVT_IDX_BASE << 6


/**
 * Indexes for available notifications.
 */
#define UI_LED_NOTIF_IDX  0x0001
#define UI_VIBR_NOTIF_IDX 0x0002

/**
 * User Interaction service external message IDs.
 */
#define MSG_ID_UI_SERVICE_RSP                  (MSG_ID_UI_SERVICE_BASE + 0x40)
#define MSG_ID_UI_SERVICE_EVT                  (MSG_ID_UI_SERVICE_BASE + 0x80)

#define MSG_ID_UI_SERVICE_GET_AVAILABLE_FEATURES_RSP     (MSG_ID_UI_SERVICE_RSP	\
							  + 0x01)
#define MSG_ID_UI_SERVICE_GET_ENABLED_EVENT_RSP          (MSG_ID_UI_SERVICE_RSP	\
							  + 0x02)
#define MSG_ID_UI_SERVICE_SET_ENABLED_EVENT_RSP          (MSG_ID_UI_SERVICE_RSP	\
							  + 0x03)
#define MSG_ID_UI_SERVICE_PLAY_LED_PATTERN_RSP           (MSG_ID_UI_SERVICE_RSP	\
							  + 0x04)
#define MSG_ID_UI_SERVICE_PLAY_VIBR_PATTERN_RSP          (MSG_ID_UI_SERVICE_RSP	\
							  + 0x07)

#define MSG_ID_UI_SERVICE_BTN_SINGLE_EVT       (MSG_ID_UI_SERVICE_EVT + 0x05)
#define MSG_ID_UI_SERVICE_BTN_DOUBLE_EVT       (MSG_ID_UI_SERVICE_EVT + 0x06)
#define MSG_ID_UI_SERVICE_BTN_MAX_EVT          (MSG_ID_UI_SERVICE_EVT + 0x07)
#define MSG_ID_UI_SERVICE_BTN_MULTIPLE_EVT     (MSG_ID_UI_SERVICE_EVT + 0x08)

/**
 * Message for \ref ui_service_get_available_features response.
 */
typedef struct ui_service_get_available_features_rsp {
	struct cfw_message header;
	uint32_t events;
	uint32_t notifications;
	int status;
} ui_service_get_available_features_rsp_t;

/**
 * Message for \ref ui_service_get_enabled_events response.
 */
typedef struct ui_service_get_events_rsp {
	struct cfw_message header;
	uint32_t events;
	int status;
} ui_service_get_events_rsp_t;

/**
 * Message for \ref ui_service_set_enabled_events response.
 */
typedef struct ui_set_events_rsp {
	struct cfw_message header;
	int status;
} ui_service_set_events_rsp_t;

/**
 * Message for \ref ui_service_play_led_pattern response.
 */
typedef struct ui_service_play_led_pattern_rsp {
	struct cfw_message header;
	uint8_t led_id;
	int status;
} ui_service_play_led_pattern_rsp_t;

/**
 * Message for \ref ui_service_play_vibr_pattern response.
 */
typedef struct ui_play_vibr_pattern_rsp {
	struct cfw_message header;
	int status;
} ui_service_play_vibr_pattern_rsp_t;


/**
 * UI service configuration structure
 */
struct ui_service_config {
	uint8_t btn_count; /*!< Number of buttons available */
	struct button *btns; /*!< Button list used by UI service */
	uint8_t led_count; /*!< Number of LEDs available */
	struct led *leds; /*!< Led list used by UI service */
};

/**
 * Message for MSG_ID_UI_BTN_*_EVT events.
 */
typedef struct ui_service_button_evt {
	struct cfw_message header;
	uint8_t btn;
	uint32_t param; /* <! In case of MSG_ID_UI_BTN_MAX_EVT. param is 0 when
	                 * timeout occurs, and 1 when button released. */
} ui_service_button_evt_t;

/**
 * Drivers events union.
 */
union ui_service_drv_evt {
	ui_service_button_evt_t btn_evt;
};

/**
 * Policy to be used for provided pattern if a pattern is in progress.
 */
enum pattern_policy {
	PATTERN_OVERRIDE = 0,   /**< Override pattern in progress */
	PATTERN_CANCEL,         /**< Cancel provided pattern */
	PATTERN_QUEUE,          /**< Wait end of pattern in progress to launch provided pattern */
};

/**
 * Retrieve all supported user events and user notifications.
 * @param  service_conn service connection pointer
 * @param  priv private data pointer that will be passed back in the response.
 *
 * @b Response: _MSG_ID_UI_SERVICE_GET_AVAILABLE_FEATURES_RSP_ with attached \ref ui_service_get_available_features_rsp_t
 */
void ui_service_get_available_features(cfw_service_conn_t *	service_conn,
				       void *			priv);

/**
 * Get current enabled events.
 * @param  service_conn service connection pointer
 * @param  priv private data pointer that will be passed back in the response.
 *
 * @b Response: _MSG_ID_UI_SERVICE_GET_ENABLED_EVENT_RSP_ with attached \ref ui_service_get_events_rsp_t
 */
void ui_service_get_enabled_events(cfw_service_conn_t *service_conn, void *priv);

/**
 * Enable one or several events.
 * @param  service_conn service connection pointer
 * @param  mask bit field to select event(s) to be changed
 * @param  enable bit field to enable=1/disable=0 selected event(s).
 * @param  priv private data pointer that will be passed back in the response.
 * @return UI_INVALID_PARAM if wrong mask,
 *         UI_SUCCESS if event successfully enabled
 *
 * @b Response: _MSG_ID_UI_SERVICE_SET_ENABLED_EVENT_RSP_ with attached \ref ui_service_set_events_rsp_t
 */
int8_t ui_service_set_enabled_events(cfw_service_conn_t *service_conn,
				     uint32_t mask, uint32_t enable,
				     void *priv);

/**
 * Play a led pattern on LED1 or LED2.
 * @param  service_conn service connection pointer
 * @param  led_id LED1 or LED2
 * @param  type LED pattern type
 * @param  pattern pointer to LED pattern data
 * @param  policy policy to use if a pattern is in progress
 * @param  priv private data pointer that will be passed back in the response.
 *
 * @b Response: _MSG_ID_UI_SERVICE_PLAY_LED_PATTERN_RSP_ with attached \ref ui_service_play_led_pattern_rsp_t
 */
void ui_service_play_led_pattern(cfw_service_conn_t *service_conn,
				 uint8_t led_id, enum led_type type,
				 led_s *pattern, enum pattern_policy policy,
				 void *priv);

/**
 * Play a vibration pattern.
 * @param  service_conn service connection pointer
 * @param  type pattern type to be played by the DRV2605 haptic driver
 * @param  pattern pointer to pattern data
 * @param  priv private data pointer that will be passed back in the response.
 *
 * @b Response: _MSG_ID_UI_SERVICE_PLAY_VIBR_PATTERN_RSP_ with attached \ref ui_service_play_vibr_pattern_rsp_t
 */
void ui_service_play_vibr_pattern(cfw_service_conn_t *service_conn,
				  vibration_type type, vibration_u *pattern,
				  void *priv);

/** @} */

#endif /* ifndef _UI_SERVICE_H */
