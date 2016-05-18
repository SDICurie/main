/**
 * COPYRIGHT(c) 2014 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#ifndef __NFC_SERVICE_SM_H__
#define __NFC_SERVICE_SM_H__

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

enum {
	/* Service command events */
	EV_NFC_INIT,
	EV_NFC_DISABLE,
	EV_NFC_RF_START,
	EV_NFC_RF_STOP,
	EV_NFC_CONFIG_CE,
	EV_NFC_HIBERNATE,

	/* Driver events */
	EV_NFC_RX_IRQ,

	/* Command sequencer events */
	EV_NFC_SEQ_PASS,
	EV_NFC_SEQ_FAIL,
	EV_NFC_RF_TUNING,
#ifdef CONFIG_NFC_STN54_FW_UPDATE
	EV_NFC_FW_UPDATE_START,
	EV_NFC_FW_UPDATE_CONTINUE,
	EV_NFC_FW_UPDATE_DONE,
	EV_NFC_FW_UPDATE_FAIL,
#endif

	/* Scenario events */
	EV_NFC_SCN_START,
	EV_NFC_SCN_DONE,
	EV_NFC_SCN_FAIL,
	EV_NFC_SCN_TIMEOUT,
	EV_NFC_SCN_TIMER,
	EV_NFC_SCN_IGNORE,

	/* NFC error handling */
	EV_NFC_ERR_START,
	EV_NFC_ERR_DONE,

	/* "any event" placeholder for the FSM table */
	EV_NFC_ANY,
	EV_NFC_MAX_EVENT
} nfc_fsm_events;

/**
 * Structure to hold a pending service message and its associated callback.
 */
struct nfc_msg_cb {
	struct cfw_message *msg;
	void (*msg_cb)(struct cfw_message *msg, uint8_t result);
};

/**
 * Scenario function typedef.
 *
 * The scenarios are called first time with the 'start' parameter set to
 * true. This allows to intialize the scenario, and also pass configuration
 * data via the rx_data/len arguments.
 *
 * Subsequent calls to the scenarios are triggered by the rx interrupt, where
 * 'start' parameter is set to false, and rx_data points to received data from
 * the controller.
 *
 * @param start set to true to initialize the scenario, only once
 * @param rx_data pointer to data received from the NFC controller
 * @param len length of data received from the NFC controller
 */
typedef void (*scn_exec_t)(bool start, uint8_t *rx_data, uint8_t len);

/**
 * Initialize the NFC FSM.
 *
 * @param queue the queue on which the FSM event handler should run
 *
 * @return 0 if success
 */
void nfc_fsm_init(void *queue);

/**
 * Post an event to the NFC FSM queue.
 *
 * @param event the event id
 * @param event_data optional data associated with the event when applicable.
 * @param msg_cb optional message callback which can be called on a later event.
 */
int nfc_fsm_event_post(int event, void *event_data, struct nfc_msg_cb *msg_cb);

/**
 * Schedule aNFC scenario.
 *
 * This is used to queue up several scenarios to be run in sequence.
 * After all the scenarios are queue, the user must call nfc_scn_start to
 * start executing the sequence.
 *
 * Normally this is run only form withing the FSM actions, but can be useful to
 * run scenarios from test commands.
 *
 * The args and arg_len parameters will be passed to the scenario only once, at
 * scenario start.
 *
 * @param scn the scenario function to be run
 * @param timeout_ms maximum allowed time for scenario to run. 0 if disabled.
 * @param arg_len length of argument data provided in 'args'
 * @param args pointer to argument data (optional)
 */
int nfc_scn_queue(scn_exec_t scn, int timeout_ms, uint8_t arg_len,
		  uint8_t *args);

/**
 * Start executing the queued scenarios
 */
int nfc_scn_start(void);

/**
 * Emit an error event.
 *
 * @param status the error stats from @ref NFC_STATUS
 * @param error internal error id from the sequencer
 */
void nfc_error_evt(uint8_t status, uint8_t error);

#endif /* __NFC_SERVICE_SM_H__ */
