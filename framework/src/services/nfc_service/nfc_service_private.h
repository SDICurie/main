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

#ifndef __NFC_SERVICE_PRIVATE_H__
#define __NFC_SERVICE_PRIVATE_H__

#include "infra/log.h"
#include "services/nfc_service/nfc_service.h"

#define MSG_ID_NFC_SERVICE_INIT             0x01 /**< Power up and init controller */
#define MSG_ID_NFC_SERVICE_DISABLE          0x02 /**< Cleanup session and power down controller */
#define MSG_ID_NFC_SERVICE_SET_CE_MODE      0x03 /**< Enable Card Emulation configuration */
#define MSG_ID_NFC_SERVICE_START_RF         0x04 /**< Start RF loop */
#define MSG_ID_NFC_SERVICE_STOP_RF          0x05 /**< Stop RF loop */
#define MSG_ID_NFC_SERVICE_HIBERNATE        0x06 /**< Enter Hibernate mode */

DEFINE_LOG_MODULE(LOG_MODULE_NFC, " NFC")

#endif
