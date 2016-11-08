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

#ifndef _SCSI_COMMAND_HANDLER_
#define _SCSI_COMMAND_HANDLER_

#include "drivers/data_type.h"

/* Sence keys */
#define NO_SENSE				0
#define NOT_READY				2
#define HARDWARE_ERROR			4
#define ILLEGAL_REQUEST			5

/* Sense ASC and ASCQ */
#define INVALID_CDB					(0x2000)
#define ADDRESS_OUT_OF_RANGE		(0x2100)
#define MEDIUM_NOT_PRESENT			(0x3A00)
#define WRITE_PROTECTED				(0x2700)
#define UNRECOVERED_READ_ERROR		(0x1100)

/**
 * Handle SCSI command from host
 *
 * @param   lun 	Logical Unit Number
 * @param   args	Pointer to command arguments
 * @return  0 if success
 */
int handle_scsi_command(uint8_t lun, uint8_t *args);

/**
 * Data to send on Request Sense SCSI command
 *
 * @param   lun 			Logical Unit Number
 * @param   sense_key 		Sense Key defined in SCSI protocol
 * @param   additional_key 	ASC and ASCQ code defined in SCSI protocol
 * @return  none
 */
void scsi_add_sense_data(uint8_t lun, uint8_t sense_key, uint16_t additional_key);

#endif // _SCSI_COMMAND_HANDLER_
