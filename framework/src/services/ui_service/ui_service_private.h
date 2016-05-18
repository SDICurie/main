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

#ifndef __UI_SERVICE_PRIVATE_H__
#define __UI_SERVICE_PRIVATE_H__

#include "infra/log.h"

/**
 * User Interaction service external message IDs.
 */
#define MSG_ID_UI_GET_FEAT_REQ         (MSG_ID_UI_SERVICE_BASE + 0x01)
#define MSG_ID_UI_GET_EVT_REQ          (MSG_ID_UI_SERVICE_BASE + 0x02)
#define MSG_ID_UI_SET_EVT_REQ          (MSG_ID_UI_SERVICE_BASE + 0x03)
#define MSG_ID_UI_LED_REQ              (MSG_ID_UI_SERVICE_BASE + 0x04)
#define MSG_ID_UI_LPAL_REQ             (MSG_ID_UI_SERVICE_BASE + 0x05)
#define MSG_ID_UI_ASR_REQ              (MSG_ID_UI_SERVICE_BASE + 0x06)
#define MSG_ID_UI_VIBR_REQ             (MSG_ID_UI_SERVICE_BASE + 0x07)
#define MSG_ID_UI_BTN_SET_TIMING_REQ   (MSG_ID_UI_SERVICE_BASE + 0x08)

/**
 * User Interaction service internal message IDs.
 */
#define MSG_ID_UI_LOCAL_TOUCH_EVT       (MSG_ID_UI_SERVICE_EVT + 0x07)
#define MSG_ID_UI_LOCAL_TAP_EVT         (MSG_ID_UI_SERVICE_EVT + 0x08)
#define MSG_ID_UI_LOCAL_LED_EVT         (MSG_ID_UI_SERVICE_EVT + 0x09)

DEFINE_LOG_MODULE(LOG_MODULE_UI_SVC, "UI_S")

/**
 * @brief Tests if some bits are set.
 */
#define TEST_EVT(m, e) (m & e)

/**
 * @brief Sets a bit to 1.
 */
#define SET_EVT_BIT(a, x) (a |= x)

/**
 * @brief Forces a bit into 0 (zero).
 */
#define CLEAR_EVT_BIT(a, x) (a &= ~x)

/**
 * @brief Returns position of a bit.
 */
#define EVT_BIT_POS(a) (a >> 1)

/**
 * UI Service states.
 */
typedef enum {
	UI_SET_EVT_READY,
	UI_SET_EVT_BLOCKING
} ui_service_states_t;

/**
 * Structure for pairs of event identifier and associated service/driver.
 */
typedef struct {
	uint16_t event_id;
	uint16_t interface_id;
} ui_events_interfaces_list_t;

/**
 * Structure for tracking services opened by the UI Service.
 *
 * Filled when received event(s) request needs a service to be opened.
 */
typedef struct {
	uint16_t svc_id[MAX_SERVICES];
	uint8_t svc_open[MAX_SERVICES];
	uint8_t count;
} ui_svc_t;

/**
 * Structure to store received event and response.
 */
typedef struct {
	struct cfw_message *msg_req;
	uint32_t events_req;
	uint32_t events_enable_req;
	uint32_t events_resp;
	uint32_t events_enable_resp;
} ui_events_req_t;

/**
 * Message for ui_service_set_enabled_events_req request.
 */
typedef struct ui_set_events_req {
	struct cfw_message header;
	uint32_t mask;
	uint32_t enable;
} ui_set_events_req_t;

/**
 * Message for ui_service_play_led_pattern_req request.
 */
typedef struct ui_play_led_pattern_req {
	struct cfw_message header;
	enum led_type type;
	uint8_t led_id;
	led_s pattern;
	enum pattern_policy policy;
} ui_play_led_pattern_req_t;

/**
 * Message for ui_play_vibr_pattern_req request.
 */
typedef struct ui_play_vibr_pattern_req {
	struct cfw_message header;
	vibration_type type;
	vibration_u pattern;
} ui_play_vibr_pattern_req_t;

/**
 * Messages for MSG_ID_UI_SERVICE_LPAL_EVT events.
 */
typedef struct ui_lpal_evt {
	struct cfw_message header;
	uint32_t ui_lpal_event_id;
} ui_lpal_evt_t;

/**
 * Message for MSG_ID_UI_SERVICE_ASR_EVT events.
 */
typedef struct ui_asr_evt {
	struct cfw_message header;
	uint32_t ui_asr_event_id;
} ui_asr_evt_t;

/**
 * Gets available events.
 *
 * @return mask with bit fields representing event(s).
 */
uint32_t ui_service_get_events_list(void);

/**
 * Stop the UI Service.
 *
 * @return error code
 */
int8_t ui_service_stop(void);

#endif /* __UI_SERVICE_PRIVATE_H__ */
