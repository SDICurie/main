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

#ifndef FACTORY_H
#define FACTORY_H

#include <stdint.h>
#include <stdbool.h>
#include "factory_data.h"

#define FLASH_OTP_BASE_ADDRESS    0xFFFFE000
#define FLASH_OTP_DATA_BLOCKS     7
#define FLASH_OTP_BLOCKS_SIZE     1024

#define FLASH_OTP_END_ADDRESS     ((FLASH_OTP_BASE_ADDRESS) + \
				   (FLASH_OTP_DATA_BLOCKS)* \
				   (FLASH_OTP_BLOCKS_SIZE) + \
				   (FLASH_OTP_DATA_BLOCKS))

#define FACTORY ((struct oem_data *)FLASH_OTP_BASE_ADDRESS)

/*
 * Checks whether the SW is in factory mode or not
 *
 * @return
 *      - true if SW is still in factory mode
 *      - false otherwise
 */
bool is_factory(void);

#endif // FACTORY_H
