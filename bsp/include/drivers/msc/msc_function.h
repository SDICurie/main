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

#ifndef _MSC_FUNCTION_H_
#define _MSC_FUNCTION_H_

#include "drivers/data_type.h"


#define CBW_FLAGS_DIRECTION_BIT_MASK 0x80 // 7th bit is direction bit
#define CBW_OUT_DIRECTION_REQUEST(flag) ((flag & CBW_FLAGS_DIRECTION_BIT_MASK) == 0) // 0 is for out direction
#define CBW_IN_DIRECTION_REQUEST(flag) !(CBW_OUT_DIRECTION_REQUEST(flag)) // 1 is for in direction

/* MSC function state */
typedef enum
{
	MSC_NO_TASK_STATE = 0,		/* Waiting for CBW to be proccessed */
	MSC_WRITE_DATA_STATE,		/* Proccessing with SCSI protocol and write data requested from host */
	MSC_WRITE_CSW_STATE			/* Write CSW message */
} msc_function_state_t;

/* Command Status sended in CSW */
typedef enum
{
	CSW_STATUS_OK = 0,		/* CBW is valid and Command Passed */
	CSW_STATUS_FAILED,		/* CBW is not valid or Command Failed */
} csw_status_t;

/* Endpoint transfer completion structure holding data from usb_ep_complete() callback */
typedef struct
{
	uint8_t ep_address; /* the enpoint address on which the transfer is triggered */
	uint8_t status; /* the status of the transfer */
	void *priv; /* the private data as passed by usb_ep_read() or usb_ep_write() */
	uint16_t actual; /* the actual length transfered */
} endpoint_handle_t;

/* MSC CBW structure according to Bulk-Only Transport Protocol */
typedef struct
{
	uint32_t signature;
	uint32_t tag;
	uint32_t data_length;
	uint8_t flags;
	uint8_t lun_number;
	uint8_t arg_length;
	uint8_t arg_data[16];
} msc_cbw_struct_t;

/* MSC CSW structure according to Bulk-Only Transport Protocol */
typedef struct
{
	uint32_t signature;
	uint32_t tag;
	uint32_t data_residure;
	uint8_t  status;
} msc_csw_struct_t;

/* Data buffer to read MSC CBW data packet from out endpoint */
extern msc_cbw_struct_t  msc_cbw;
/* Data buffer to write MSC CBW data packet to in endpoint */
extern msc_csw_struct_t  msc_csw;

/* Data buffer to transfer data to host according to SCSI protocol*/
extern uint8_t msc_buffer[];
/* Length of data in the buffer to be tranfered */
extern uint16_t msc_buffer_length;

/* State of MSC Device */
extern msc_function_state_t msc_function_state;

/**
 * Mass Storage Class function handler initialisation
 *
 * @return none
 */
void msc_init(void);

/**
 * Handle state of in endpoint
 *
 * @param  pdev 	params from endpoint handler
 * @param  epnum 	number of endpoint
 * @return  none
 */
void msc_handle_data_to_host(endpoint_handle_t  *pdev, uint8_t epnum);

/**
 * Handle state out endpoint
 *
 * @param  pdev 	params from endpoint handler
 * @param  epnum 	number of endpoint
 * @return none
 */
void msc_handle_data_from_host(endpoint_handle_t  *pdev, uint8_t epnum);

/**
 * Handle state of out and in endpoints after clear feature response
 *
 * @param  epnum 	number of endpoint for clear feature request
 * @return  none
 */
void msc_handle_clear_feature(uint8_t epnum);

#endif // _MSC_FUNCTION_H_
