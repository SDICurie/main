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

#include "infra/log.h"
#include "usb.h"
#include "usb_driver_interface.h"
#include "msc_file_system.h"
#include "msc_function.h"
#include "msc.h"
#include "scsi_command_handler.h"

//#define SCSI_DEBUG

#ifdef SCSI_DEBUG
#define pr_debug_msc_scsi(...) pr_debug(LOG_MODULE_USB, __VA_ARGS__)
#else
#define pr_debug_msc_scsi(...)
#endif

/*					SCSI commands id 					*/
#define SCSI_TEST_UNIT_READY						(0x00)
#define SCSI_INQUIRY								(0x12)
#define SCSI_MODE_SENSE6							(0x1A)
#define SCSI_READ10									(0x28)
#define SCSI_READ_CAPACITY10						(0x25)
#define SCSI_REQUEST_SENSE							(0x03)
#define SCSI_WRITE10								(0x2A)


/*					SCSI commands info					*/
#define READ_CAPACITY10_DATA_LEN 					(0x08)
#define MODE_SENSE6_LEN								(0x04)

#define INQUIRY_EVPD_INDEX 							(1)
#define INQUIRY_EVPD_MASK 							(0x01)
#define INQUIRY_PAGE_CODE_INDEX 					(2)
#define INQUIRY_ALLOCATION_LENGTH_INDEX 			(4)

#define REQUEST_SENSE_DATA_LEN 						(0x13)
#define REQUEST_SENSE_ALLOCATION_LENGTH_INDEX 		(4)

#define SENSE_RESPONSE_CODE 						(0x70) // current fixed error
#define SENSE_RESPONSE_CODE_INDEX 					(0)
#define SENSE_KEY_INDEX 							(2)
#define SENSE_ASC_INDEX 							(12)
#define SENSE_ASCQ_INDEX 							(13)
#define SENSE_ADDITIONAL_DATA_LENGTH_INDEX 			(7)

#define READ_10_DATA_ADDR_INDEX 					(2)
#define READ_10_DATA_LENGTH_INDEX 					(7)


/*					Inquiry pages info					*/
#define SUPPORTED_VPD_PAGES_PAGE_CODE				(0x00)
#define SUPPORTED_VPD_PAGES_PAGE_LENGTH				(6)

#define DEVICE_INDENTIFICATION_PAGE_CODE			(0x83)
#define DEVICE_INDENTIFICATION_PAGE_LENGTH			(14)


#define STANDART_INQUIRY_PAGE_LENGTH				(36)

const uint8_t scsi_supported_vpd_pages_page_data[] = {
	0x00,
	SUPPORTED_VPD_PAGES_PAGE_CODE,
	0x00,
	(SUPPORTED_VPD_PAGES_PAGE_LENGTH - 4),
	0x00,
	0x83,
};

const uint8_t scsi_device_indentification_page_data[] = {
	0x00,
	DEVICE_INDENTIFICATION_PAGE_CODE,
	0x00,
	(DEVICE_INDENTIFICATION_PAGE_LENGTH - 4),
	0x01,
	0x00,
	0x12, 0x34, 0x56, 0x78, 0xAB, 0xCD, 0xEF, 0xF0, /* Serial ID:	8 bytes */
};

const uint8_t standart_inquiry_page_data[] = {
	0x00,
	0x80,
	0x02,
	0x02,
	(STANDART_INQUIRY_PAGE_LENGTH - 5),
	0x00,
	0x00,
	0x00,
	'I', 'N', 'T', 'E', 'L', ' ', ' ', ' ', /* Manufacturer, 8 bytes */
	'U', 'S', 'B', ' ', 'M', 'S', 'C', ' ', 'd', 'e', 'v', 'i', 'c', 'e', ' ', ' ', /* Product, 16 Bytes */
	'0', '0', '0' ,'1', /* Version, 4 Bytes */
};

/* 			Mode Sense 6 data			*/
const uint8_t mode_sense_6_data[] = {
	MODE_SENSE6_LEN-1,
	0x00,
#ifdef CONFIG_USB_MSC_WRITE_PROTECTED
	0x80,
#else
	0x00,
#endif
	0x00,
};

typedef struct {
	char sense_key;
	char ASC;
	char ASCQ;
} sense_info_struct_t;

static sense_info_struct_t sense_data;

static uint32_t msc_block_size;
static uint32_t msc_block_count;

static uint32_t scsi_data_addr;
static uint32_t scsi_data_length;

static int scsi_check_address_range(uint8_t lun, uint32_t offset , uint32_t count);
static int scsi_test_unit_ready(uint8_t lun, uint8_t *arg);
static int scsi_inquiry(uint8_t lun, uint8_t *arg);
static int scsi_mode_sense_6(uint8_t lun, uint8_t *arg);
static int scsi_read_capacity_10(uint8_t lun, uint8_t *arg);
static int scsi_read_10(uint8_t lun , uint8_t *arg);
static int scsi_write_10(uint8_t lun , uint8_t *arg);
static int scsi_request_sense(uint8_t lun, uint8_t *arg);
static void scsi_clear_sense_data(void);

static int scsi_check_address_range(uint8_t lun, uint32_t offset , uint32_t count)
{
	if ((offset + count) > msc_block_count) {
		scsi_add_sense_data(lun, ILLEGAL_REQUEST, ADDRESS_OUT_OF_RANGE);
		return -1;
	}

	return 0;
}

static int scsi_test_unit_ready(uint8_t lun, uint8_t *arg)
{
	(void)(arg);

	if (msc_cbw.data_length != 0) {
		scsi_add_sense_data(msc_cbw.lun_number, ILLEGAL_REQUEST, INVALID_CDB);
		return -1;
	}

	msc_buffer_length = 0;

	return 0;
}

static int scsi_inquiry(uint8_t lun, uint8_t *arg)
{
	uint8_t* pPage = NULL;
	uint16_t len = 0;

	if (arg[INQUIRY_EVPD_INDEX] & INQUIRY_EVPD_MASK) {

		switch(arg[INQUIRY_PAGE_CODE_INDEX]) {

		case DEVICE_INDENTIFICATION_PAGE_CODE:
			pPage = (uint8_t *)scsi_device_indentification_page_data;
			len = DEVICE_INDENTIFICATION_PAGE_LENGTH;
			break;

		default:
			pPage = (uint8_t *)scsi_supported_vpd_pages_page_data;
			len = SUPPORTED_VPD_PAGES_PAGE_LENGTH;
			break;
		}

	}
	else {
		uint8_t allocation_length = arg[INQUIRY_ALLOCATION_LENGTH_INDEX];

		pPage = (uint8_t *)standart_inquiry_page_data;
		len = STANDART_INQUIRY_PAGE_LENGTH;

		len = MIN(allocation_length, STANDART_INQUIRY_PAGE_LENGTH);
	}

	msc_buffer_length = len;

	if (pPage != NULL) {
		memcpy(msc_buffer, pPage, msc_buffer_length);
	}

	msc_function_state = MSC_WRITE_CSW_STATE;
	return 0;
}

static int scsi_read_capacity_10(uint8_t lun, uint8_t *arg)
{
	(void)(arg);

	if (msc_get_lun_memory_info(lun, &msc_block_count, &msc_block_size) != 0) {
		scsi_add_sense_data(lun, NOT_READY, MEDIUM_NOT_PRESENT);
		return -1;
	}
	else {

		IUSETDW(msc_buffer, msc_block_count - 1);
		IUSETDW(msc_buffer + 4, msc_block_size);

		msc_buffer_length = READ_CAPACITY10_DATA_LEN;
		msc_function_state = MSC_WRITE_CSW_STATE;

		return 0;
	}
}

static int scsi_mode_sense_6(uint8_t lun, uint8_t *arg)
{
	(void)(arg);
	(void)(lun);

	msc_buffer_length = MODE_SENSE6_LEN;
	memcpy(msc_buffer, mode_sense_6_data, msc_buffer_length);
	msc_function_state = MSC_WRITE_CSW_STATE;

	return 0;
}

static int scsi_request_sense(uint8_t lun, uint8_t *arg)
{
	(void)(arg);
	(void)(lun);

	uint8_t allocation_length = arg[REQUEST_SENSE_ALLOCATION_LENGTH_INDEX];

	pr_debug_msc_scsi("scsi_request_sense\n");

	memset(msc_buffer, 0, REQUEST_SENSE_DATA_LEN);

	msc_buffer[SENSE_RESPONSE_CODE_INDEX] = SENSE_RESPONSE_CODE;
	msc_buffer[SENSE_ADDITIONAL_DATA_LENGTH_INDEX] = REQUEST_SENSE_DATA_LEN - 7;
	msc_buffer[SENSE_KEY_INDEX] = sense_data.sense_key;
	msc_buffer[SENSE_ASC_INDEX] = sense_data.ASC;
	msc_buffer[SENSE_ASCQ_INDEX] = sense_data.ASCQ;

	msc_buffer_length = MIN(allocation_length, REQUEST_SENSE_DATA_LEN);

	msc_function_state = MSC_WRITE_CSW_STATE;

	scsi_clear_sense_data();

	return 0;
}

static int scsi_read_10_init(uint8_t lun , uint8_t *arg)
{
	if (!CBW_IN_DIRECTION_REQUEST(msc_cbw.flags)) {
		scsi_add_sense_data(msc_cbw.lun_number, ILLEGAL_REQUEST, INVALID_CDB);
		return -1;
	}

	if (msc_get_lun_memory_info(lun, &msc_block_count, &msc_block_size) != 0) {
		scsi_add_sense_data(lun, HARDWARE_ERROR, UNRECOVERED_READ_ERROR);
		return -1;
	}

	scsi_data_addr = IUGETDW(arg + READ_10_DATA_ADDR_INDEX);
	scsi_data_length = IUGETW(arg + READ_10_DATA_LENGTH_INDEX);

	if (scsi_check_address_range(lun, scsi_data_addr, scsi_data_length) != 0) {
		scsi_add_sense_data(msc_cbw.lun_number, ILLEGAL_REQUEST, INVALID_CDB);
		return -1;
	}

	msc_function_state = MSC_WRITE_DATA_STATE;

	scsi_data_addr *= msc_block_size;
	scsi_data_length *= msc_block_size;

	if (msc_cbw.data_length != scsi_data_length) {
		scsi_add_sense_data(msc_cbw.lun_number, ILLEGAL_REQUEST, INVALID_CDB);
		return -1;
	}

	return 0;
}

static int scsi_read_10(uint8_t lun , uint8_t *arg)
{
	if (msc_function_state == MSC_NO_TASK_STATE) {
		if (scsi_read_10_init(lun, arg) != 0) {
			return -1;
		}
	}

	if (scsi_data_length == 0) {
		pr_debug_msc_scsi("SCSI_ProcessRead -> SCSI_blk_len is zero\n");
		scsi_add_sense_data(lun, HARDWARE_ERROR, UNRECOVERED_READ_ERROR);
		return -1;
	}

	msc_buffer_length = MIN(scsi_data_length , MSC_BUFFER_LENGTH);

	if (msc_read_lun_data(lun,
						msc_buffer,
						scsi_data_addr/msc_block_size,
						msc_buffer_length/msc_block_size) != 0) {
		scsi_add_sense_data(lun, HARDWARE_ERROR, UNRECOVERED_READ_ERROR);
		return -1;
	}

	scsi_data_addr += msc_buffer_length;
	scsi_data_length -= msc_buffer_length;

	msc_function_state = (scsi_data_length != 0) ? MSC_WRITE_DATA_STATE : MSC_WRITE_CSW_STATE;

	return 0;
}

static int scsi_write_10(uint8_t lun, uint8_t *arg)
{
	(void)(arg);
	scsi_add_sense_data(lun, NOT_READY, WRITE_PROTECTED);
	return -1;
}

static void scsi_clear_sense_data(void)
{
	memset(&sense_data, 0, sizeof(sense_info_struct_t));
}

int handle_scsi_command(uint8_t lun, uint8_t *arg)
{
	int retVal = -1;

	pr_debug_msc_scsi("handle_scsi_command with cmd 0x%x\n", arg[0]);

	switch (arg[0]) {

		case SCSI_INQUIRY:
			retVal = scsi_inquiry(lun, arg);
			break;

		case SCSI_TEST_UNIT_READY:
			retVal = scsi_test_unit_ready(lun, arg);
			break;

		case SCSI_READ_CAPACITY10:
			retVal = scsi_read_capacity_10(lun, arg);
			break;

		case SCSI_MODE_SENSE6:
			retVal = scsi_mode_sense_6 (lun, arg);
			break;

		case SCSI_READ10:
			retVal = scsi_read_10(lun, arg);
			break;

		case SCSI_WRITE10:
			retVal = scsi_write_10(lun, arg);
			break;

		case SCSI_REQUEST_SENSE:
			retVal = scsi_request_sense(lun, arg);
			break;

		default:
			retVal = -1;
			scsi_add_sense_data(lun, ILLEGAL_REQUEST, INVALID_CDB);
	}

	return retVal;
}

void scsi_add_sense_data(uint8_t lun, uint8_t sense_key, uint16_t additional_key)
{
	(void)(lun);
	sense_data.sense_key = sense_key;
	sense_data.ASC = ((additional_key >> 8) & 0xFF);
	sense_data.ASCQ = (additional_key & 0xFF);
}
