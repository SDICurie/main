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

#ifndef __FACTORY_DATA_H__
#define __FACTORY_DATA_H__

/**
 * @defgroup infra_factory_data Factory Data
 * Defines structures for OEM (Intel&reg;) and customer data (512 bytes each).
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/factory_data.h"</tt>
 * </table>
 *
 * The factory data is located in OTP memory. It includes information on
 * the hardware and the product. It is provisionned during manufacturing.
 *
 * See @ref memory_partitioning for location.
 *
 * @ingroup infra
 * @{
 */

#include <stdint.h>
#include "util/compiler.h"


/** Magic string value mandatory in all factory_data struct. */
#define FACTORY_DATA_MAGIC "$FA!"

/** The version supported by this code. Increment each time the definition of
 * the factory_data struct is modified */
#define FACTORY_DATA_VERSION 0x01

enum hardware_type {
	HARDWARE_TYPE_EVT = 0x00,     /*!< Engineering hardware */
	HARDWARE_TYPE_DVT = 0x04,     /*!< Comes after EVT */
	HARDWARE_TYPE_PVT = 0x08,     /*!< Production-ready hardware  */
	HARDWARE_TYPE_PR = 0x09,      /*!< PRototype hardware */
	HARDWARE_TYPE_FF = 0x0c,      /*!< Form Factor hardware, like PR but with final product form factor */

	HARDWARE_TYPE_MAX = 0x0F,
};

/**
 * Contains OEM, i.e. Intel data.
 * Size: 512 bytes
 */
struct oem_data {
	/** Project-specific header. Keep to 0xFFFFFFFF by default */
	uint8_t header[4];

	/** Always equal to $FA! */
	uint8_t magic[4];

	/** OEM data format version */
	uint8_t version;

	/** OEM Production flag
	 * factory mode = 0xFF, production mode = 0x01
	 * shall be set to 0x01 at end of oem manufacturing process */
	uint8_t production_mode_oem;

	/** OEM Production flag
	 * factory mode = 0xFF, production mode = 0x01
	 * shall be set to 0x01 at end of customer manufacturing process */
	uint8_t production_mode_customer;

	/** Reserved for later use */
	uint8_t reserved;

	/** Hardware info: 16 bytes */
	uint8_t hardware_info[16];

	/** UUID stored as binary data, usually displayed as hexadecimal.
	 * It is used to generate debug token and is a fallback if factory_sn
	 * is not unique */
	uint8_t uuid[16];

	/** Factory serial number
	 * Up to 32 ASCII characters. If its length is < 32, it is NULL terminated.*/
	char factory_sn[32];

	/** Hardware identification number, stored as binary data
	 * usually displayed as hexadecimal. Left padded with 0 if smaller than 32 bytes */
	uint8_t hardware_id[32];

	/** Project specific data */
	uint8_t project_data[84];

	/** Public keys and other infos used for secure boot and manufacturing
	 * size: 64*4+128 bytes */
	uint8_t security[320];
} __packed;

/**
 * Describes board hardware info
 */
struct product_hw_info {
	/** Name of board */
	uint8_t name;
	/** Type of board, used for debug and tracking */
	uint8_t type;
	/** Revision number of board, used for debug and tracking */
	uint8_t revision;
	/** Project specific field */
	uint8_t variant;
} __packed;

/**
 * Contains Customer data.
 * Size: 512 bytes
 */
struct customer_data {
	/** Product serial number
	 * Up to 16 ASCII characters. If its length is < 16, it is NULL terminated.*/
	uint8_t product_sn[16];

	/** Product hardware info */
	struct product_hw_info product_hw_info;

	uint8_t reserved[492];
} __packed;

/**
 * Defines the data provisioned at the factory for each device.
 *
 * Consist in 1 page of 1024 bytes to store OEM (Intel&reg;) and customer data:
 *  - 512 bytes for OEM
 *  - 512 bytes for customer
 */
struct factory_data {
	struct oem_data oem_data;
	struct customer_data customer_data;
} __packed;


/**
 * Pointer on the global factory_data instance. This instance is usually not part
 * of this binary but is instead flashed separately.
 */
extern const struct factory_data *global_factory_data;

/** @} */

#endif /* __FACTORY_DATA_H__ */
