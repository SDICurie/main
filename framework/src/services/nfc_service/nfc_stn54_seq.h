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

#ifndef __NFC_SERVICE_SEQ_H__
#define __NFC_SERVICE_SEQ_H__

/* NDLC/NCI/HCI definitions: */
#define TIMER_MESSAGE_TYPE                              0xFF

#define PCB_SYNC_ACK                                    0x20
#define PCB_SYNC_NACK                                   0x10
#define PCB_SYNC_WAIT                                   0x30
#define PCB_SYNC_NOINFO                                 0x00
#define PCB_SYNC_MASK                                   PCB_SYNC_WAIT

#define PCB_TYPE_SUPERVISOR                             0xc0
#define PCB_SUPERVISOR_RETRANSMIT_NO        0x02
#define PCB_SUPERVISOR_RETRANSMIT_YES       0x00
#define PCB_SUPERVISOR_RETRANSMIT_MASK      PCB_SUPERVISOR_RETRANSMIT_NO

#define PCB_TYPE_DATAFRAME                          0x80
#define PCB_DATAFRAME_RETRANSMIT_NO         0x04
#define PCB_DATAFRAME_RETRANSMIT_YES        0x00
#define PCB_DATAFRAME_RETRANSMIT_MASK       PCB_DATAFRAME_RETRANSMIT_NO

#define PCB_TYPE_MASK                       PCB_TYPE_SUPERVISOR

#define PCB_FRAME_CRC_INFO_PRESENT          0x08
#define PCB_FRAME_CRC_INFO_NOTPRESENT       0x00
#define PCB_FRAME_CRC_INFO_MASK             PCB_FRAME_CRC_INFO_PRESENT

#define ST21NFC_FRAME_HEAD_DATASIZE 1
#define ST21NFC_FRAME_TAIL_DATASIZE 0

/* st21nfc NCI frames definitions */
#define ST21NFC_NCI_FRAME_HEADER         4
#define ST21NFC_NCI_FRAME_MAX_SIZE       255

/* NCI/HCI definitions */
#define NCI_TYPE_OFFSET       0x00
#define NCI_OID_OFFSET        0x01
#define NCI_PAYLOAD_OFFSET    0x02
#define HCI_1RST_BYTE_OFFSET  0x03
#define HCI_2ND_BYTE_OFFSET   0x04

#define NCI_FRAME_TYPE_MASK   0x7F
#define NCI_MAX_FRAME_SIZE    0xfa

/* GID + type */
#define NCI_CORE_RSP      0x40
#define NCI_CORE_NTF      0x60
#define NCI_RF_RSP        0x41
#define NCI_RF_NTF        0x61
#define NCI_NFCEE_RSP     0x42
#define NCI_NFCEE_NTF     0x62
#define NCI_PROP_RSP      0x4F
#define NCI_PROP_NTF      0x6F

#define NCI_CORE_RESET            0b000000
#define NCI_CORE_INIT             0b000001
#define NCI_CORE_SET_CONFIG       0b000010
#define NCI_CORE_GET_CONFIG       0b000011
#define NCI_CORE_CONN_CREATE      0b000100
#define NCI_CORE_CONN_CLOSE       0b000101
#define NCI_CORE_CONN_CREDITS     0b000110
#define NCI_CORE_GENERIC_ERROR    0b000111
#define NCI_CORE_INTERFACE_ERROR  0b001000

#define NCI_NFCEE_DISCOVER        0b000000
#define NCI_NFCEE_MODE_SET        0b000001

#define NCI_RF_DISCOVER_MAP       0b000000
#define NCI_RF_SET_LISTEN_MODE_ROUTING 0b000001
#define NCI_RF_GET_LISTEN_MODE_ROUTING 0b000010
#define NCI_RF_DISCOVER           0b000011
#define NCI_RF_DISCOVER_SELECT    0b000100
#define NCI_RF_INTF_ACTIVATED     0b000101
#define NFC_RF_DEACTIVATE         0b000110
#define NCI_RF_FIELD_INFO         0b000111
#define NCI_RF_T3T_POLLING        0b001000
#define NCI_RF_NFCEE_ACTION       0b001001
#define NCI_RF_NFCEE_DISCOVERY_REQ     0b001010
#define NCI_RF_PARAMETER_UPDATE   0b001011

/* NCI status codes */
#define NCI_STATUS_OK                                     0x00
#define NCI_STATUS_REJECTED                               0x01
#define NCI_STATUS_RF_FRAME_CORRUPTED                     0x02
#define NCI_STATUS_FAILED                                 0x03
#define NCI_STATUS_NOT_INITIALIZED                        0x04
#define NCI_STATUS_SYNTAX_ERROR                           0x05
#define NCI_STATUS_SEMANTIC_ERROR                         0x06
// RFU                                                    0x07 through 0x08
#define NCI_STATUS_INVALID_PARAM                          0x09
#define NCI_STATUS_MESSAGE_SIZE_EXCEEDED                  0x0A
// RFU                                                    0x0B through 0x9F
#define NCI_STATUS_RF_DISCOVERY_ALREADY_STATRED           0xA0
#define NCI_STATUS_RF_DISCOVERY_TARGET_ACTIVATION_FAILED  0xA1
#define NCI_STATUS_RF_DISCOVERY_TEAR_DOWN                 0xA2
// RFU                                                    0xA3 through 0xAF
#define NCI_STATUS_RF_TRANSMISSION_ERROR                  0xB0
#define NCI_STATUS_RF_PROTOCOL_ERROR                      0xB1
#define NCI_STATUS_RF_TIMEOUT_ERROR                       0xB2
// RFU                                                    0xB3 through 0xBF
#define NCI_STATUS_NFCEE_INTERFACE_ACTIVATION_FAILED      0xC0
#define NCI_STATUS_NFCEE_TRANSMISSION_ERROR               0xC1
#define NCI_STATUS_NFCEE_PROTOCOL_ERROR                   0xC2
#define NCI_STATUS_TIMEOUT_ERROR                          0xC3
// RFU                                                    0xC4 through 0xDF
#define NCI_STATUS_RSP_NOT_YET_RECEIVED                   0xDF  // Overloading from RFU Range. --Ian
// PROPRIETARY                                            0xE0 through 0xFF

#define NCI_DATA_HCI          0x01

#define HCI_CMD_TYPE          0x00
#define HCI_EVT_TYPE          0x40
#define HCI_RSP_TYPE          0x80

#define HCI_EVT_TRANSMIT_DATA   0x50
#define HCI_EVT_WTX_REQUEST     0x51
#define HCI_NOTIFY_PIPE_CREATED 0x12
#define HCI_OPEN_PIPE           0x03

#define HCI_ANY_OK            0x00
#define HCI_ADMIN_GATE_DATA   0x81
#define HCI_DM_GATE_DATA      0x82
#define HCI_NO_FRAG_MASK      0x80

#define HCI_FIRST_ESE_PIPE_ID 0x50
#define HCI_LAST_ESE_PIPE_ID  0x56

#define APDU_GATE_ID          0xf0

typedef struct {
	uint8_t pcb; /* ndlc_header */
	uint8_t buffer[ST21NFC_NCI_FRAME_MAX_SIZE - 1]; /* ndlc data   */
} ndlc_msg_t;

/* ==================== */

enum {
	eOPEN_ADMIN,
	eSET_SESSION_ID,
	eSET_WHITELIST,
	eHCI_INIT_DONE,
	eWAIT_PIPE_CREATION_NTF,
	eWAIT_PIPE_OPEN_NTF,
	eWAIT_PIPE_INFO,
	eAPDU_PIPE_OK,
	eAPDU_PIPE_ATR,
	eAPDU_PIPE_WAIT_ATR,
	eAPDU_PIPE_READY
};

/* ==================== */

void nfc_tuning_io(void);
void nfc_tuning_card_a(void);

void nfc_scn_default(bool start, uint8_t *rx_data, uint8_t len);

void nfc_scn_reset_init(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_enable_se(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_start_rf(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_stop_rf(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_enable_rf_field_ntf(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_get_config(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_set_config(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_set_mode(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_fw_update(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_raw_write(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_ams_read_reg(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_ams_write_reg(bool start, uint8_t *rx_data, uint8_t len);
void nfc_scn_ams_status_dump(bool start, uint8_t *rx_data, uint8_t len);

#endif /* __NFC_SERVICE_SEQ_H__ */
