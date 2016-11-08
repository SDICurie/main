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
#include "scsi_command_handler.h"
#include "msc.h"
#include "msc_function.h"

//#define MSC_FUNCTION_DEBUG

#ifdef MSC_FUNCTION_DEBUG
#define pr_debug_msc_function(...) pr_debug(LOG_MODULE_USB, __VA_ARGS__)
#else
#define pr_debug_msc_function(...)\
	do {\
} while (0)
#endif

#define CBW_SIGNATURE 0x43425355
#define CBW_DATA_LENGTH 31
#define CBW_ARG_MIN_LENGTH 1
#define CBW_ARG_MAX_LENGTH 16

#define CSW_SIGNATURE 0x53425355
#define CSW_DATA_LENGTH 13

msc_function_state_t msc_function_state;

msc_cbw_struct_t msc_cbw;
msc_csw_struct_t msc_csw;

uint8_t msc_buffer[MSC_BUFFER_LENGTH];
uint16_t msc_buffer_length;

static int msc_check_cbw(endpoint_handle_t *pdev);
static int msc_process_command(void);
static int msc_wait_for_data(uint8_t *buffer, uint16_t length);
static int msc_write_data(uint8_t* buffer, uint16_t length);
static int msc_write_csw(uint8_t CSW_Status);
static void msc_handle_error(void);


static int msc_wait_for_data(uint8_t *buffer, uint16_t length)
{
	pr_debug_msc_function("MSC_BOT_Prepare_Receive\n");
	return usb_ep_read(MSD_OUT_EP_ADDR, buffer, length, NULL);
}

static int msc_process_command(void)
{
	if (handle_scsi_command(msc_cbw.lun_number, &msc_cbw.arg_data[0]) != 0) {
		return -1;
	}

	if (msc_buffer_length > 0) {
		msc_write_data(msc_buffer, msc_buffer_length);
	}
	else if (msc_buffer_length == 0) {
			msc_write_csw(CSW_STATUS_OK);
		}


	return 0;
}

static int msc_check_cbw(endpoint_handle_t *pdev)
{
	pr_debug_msc_function("msc_check_cbw\n");

	(void)(pdev);

	msc_csw.tag = msc_cbw.tag;
	msc_csw.data_residure = msc_cbw.data_length;

	pr_debug_msc_function("MSC_BOT_cbw.dDataLength is 0x%x\n", msc_cbw.data_length);

	bool is_cbw_valid = true;

	is_cbw_valid = (pdev->actual == CBW_DATA_LENGTH) &&
					(msc_cbw.signature == CBW_SIGNATURE) &&
					((msc_cbw.arg_length >= CBW_ARG_MIN_LENGTH) && (msc_cbw.arg_length <= CBW_ARG_MAX_LENGTH));

	if (!is_cbw_valid) {
		scsi_add_sense_data(msc_cbw.lun_number, ILLEGAL_REQUEST, INVALID_CDB);
		return -1;
	}

	return 0;
}

static int msc_write_data(uint8_t* buffer, uint16_t length)
{

	length = MIN(msc_cbw.data_length, length);
	msc_csw.data_residure -= length;
	msc_csw.status = CSW_STATUS_OK;

#if MSC_FUNCTION_DEBUG
	pr_debug_msc_function("MSC_BOT_csw.dDataResidue is 0x%x\n", msc_csw.data_residure);
	pr_debug_msc_function("MSC_BOT_State -> 0x%x\n", msc_function_state);
	pr_debug_msc_function("MSC_BOT_SendData -> usb_ep_write\n");
#endif

	return usb_ep_write(MSD_IN_EP_ADDR, buffer, length, NULL);
}

static int msc_write_csw(uint8_t CSW_Status)
{
	pr_debug_msc_function("msc_send_csw with Status 0x%x with function state 0x%x\n", msc_csw.status, msc_function_state);

	msc_csw.signature = CSW_SIGNATURE;
	msc_csw.status = CSW_Status;
	msc_function_state = MSC_NO_TASK_STATE;

	return usb_ep_write(MSD_IN_EP_ADDR, (uint8_t *)&msc_csw, CSW_DATA_LENGTH, NULL);
}

static void msc_handle_error(void)
{
	pr_debug_msc_function("msc_handle_error\n");

	uint8_t retVal = -1;

	retVal = usb_ep_stall(MSD_IN_EP_ADDR, 1, NULL);
	pr_debug_msc_function("MSC_BOT_Abort -> Stall MSC_IN_EP with retVal 0x%x\n", retVal);

	if (CBW_OUT_DIRECTION_REQUEST(msc_cbw.flags) && (msc_cbw.data_length != 0)) {
		retVal = usb_ep_stall(MSD_OUT_EP_ADDR, 1, NULL);
		pr_debug_msc_function("MSC_BOT_Abort -> Stall MSC_OUT_EP with retVal 0x%x\n", retVal);
	}
}

void msc_handle_clear_feature(uint8_t epnum)
{
	if (UE_GET_DIR(epnum) == UE_DIR_IN) {
		msc_write_csw(CSW_STATUS_FAILED);
	}
}

void msc_handle_data_to_host(endpoint_handle_t *pdev, uint8_t epnum)
{

	(void)(pdev);
	(void)(epnum);

	pr_debug_msc_function("MSC_BOT_DataIn -> MSC_BOT_State is 0x%x\n", msc_function_state);

	switch (msc_function_state)
	{
		case MSC_NO_TASK_STATE:
			msc_wait_for_data((uint8_t *)&msc_cbw, CBW_DATA_LENGTH);
			break;

		case MSC_WRITE_DATA_STATE:
			if (msc_process_command() != 0) {
				msc_write_csw(CSW_STATUS_FAILED);
			}
			break;

		case MSC_WRITE_CSW_STATE:
			msc_write_csw(CSW_STATUS_OK);
			break;
	}
}

void msc_handle_data_from_host(endpoint_handle_t *pdev, uint8_t epnum)
{
	(void)(pdev);
	(void)(epnum);

	pr_debug_msc_function("MSC_BOT_DataOut -> MSC_BOT_State is 0x%x\n", msc_function_state);

	if (msc_check_cbw(pdev) != 0) {
		msc_handle_error();
		return;
	}

	if (msc_process_command() != 0) {
		msc_handle_error();
		return;
	}
}

void msc_init()
{
	msc_function_state = MSC_NO_TASK_STATE;
	msc_wait_for_data((uint8_t *)&msc_cbw, CBW_DATA_LENGTH);
}
