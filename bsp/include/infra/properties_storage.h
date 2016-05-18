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

#ifndef __PROPERTIES_STORAGE_H
#define __PROPERTIES_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup infra_properties_storage Properties storage
 * Properties storage API
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/properties_storage.h"</tt>
 * </table>
 *
 * The property storage API allows to store key/value pairs on non-volatile
 * memory. Each value can be flagged as persistent to factory reset or not.
 *
 * The API of the property storage is synchronous, and not thread safe.
 *
 * To manage properties at a higher level see also @ref properties_service.
 *
 * For properties storage location see @ref memory_partitioning.
 *
 *
 * @ingroup infra
 * @{
 */

/** Error codes */
typedef enum {
	PROPERTIES_STORAGE_SUCCESS = 0,         /*!< Operation succeed */
	PROPERTIES_STORAGE_BOUNDS_ERROR,        /*!< Overflow */
	PROPERTIES_STORAGE_IO_ERROR,            /*!< I/O error */
	PROPERTIES_STORAGE_KEY_NOT_FOUND_ERROR, /*!< Key not found */
	PROPERTIES_STORAGE_INVALID_ARG,         /*!< Invalid argument */
} properties_storage_status_t;

/** Maximum size for the value of a property in byte */
#define PROPERTIES_STORAGE_MAX_VALUE_LEN 512

/** Maximum number of properties that can be stored in the store  */
#define PROPERTIES_STORAGE_MAX_NB_PROPERTIES 32

/**
 * Initialize the property storage.
 */
void properties_storage_init(void);

/**
 * Format all property storage partitions.
 *
 * It also re-initializes the property storage.
 */
void properties_storage_format_all(void);

/**
 * Set the value for a property.
 *
 * If the property didn't previously exist, it is created.
 * This function blocks until completion.
 *
 * @param key Key of the property
 * @param buf New value to store
 * @param len Length of buf in bytes
 * @param factory_reset_persistent Set to true to create a reset persistent
 * property. This argument is only used if the property is created. To change a
 * property flag it needs to be deleted first.
 *
 * @return
 *  - PROPERTIES_STORAGE_INVALID_ARG:  Invalid len (upper than
 *                                     PROPERTIES_STORAGE_MAX_VALUE_LEN)
 *  - PROPERTIES_STORAGE_BOUNDS_ERROR: Buffer length is larger than limit, or store
 *                                     is full
 *  - PROPERTIES_STORAGE_IO_ERROR:     Write failure, element is not stored
 *  - PROPERTIES_STORAGE_SUCCESS:      Store succeed
 */
properties_storage_status_t properties_storage_set(
	uint32_t key, const uint8_t *buf, uint16_t len,
	bool factory_reset_persistent);

/**
 * Get the value for a property.
 *
 * This function blocks until completion.
 *
 * @param key Key of the property
 * @param buf Buffer where to return the read value. It is the responsability
 * of the caller to provide a buffer large enough to contain the property value.
 * @param len Length of output buffer in bytes
 * @param readlen Address where to return the length of the read value.
 *
 * @return
 *  - PROPERTIES_STORAGE_IO_ERROR:  Read failure, element is not read
 *  - PROPERTIES_STORAGE_KEY_NOT_FOUND_ERROR: Unknown key
 *  - PROPERTIES_STORAGE_BOUNDS_ERROR: buf length is too small to contain the value
 *  - PROPERTIES_STORAGE_SUCCESS:      Read succeed
 */
properties_storage_status_t properties_storage_get(uint32_t key, uint8_t *buf,
						   uint16_t len,
						   uint16_t *readlen);

/**
 * Get info about a property.
 *
 * This function blocks until completion.
 *
 * @param key Key of the property
 * @param len Address where to return the size of the property value in bytes
 * @param factory_reset_persistent Address where to return whether the property
 * is persistent upon a factory reset.
 *
 * @return
 *  - PROPERTIES_STORAGE_KEY_NOT_FOUND_ERROR: Unknown key
 *  - PROPERTIES_STORAGE_SUCCESS:      Key exists
 */
properties_storage_status_t properties_storage_get_info(
	uint32_t key, uint16_t *len, bool *factory_reset_persistent);

/**
 * Delete a property.
 *
 * This function blocks until completion.
 *
 * @param key Key of the property
 *
 * @return
 *  - PROPERTIES_STORAGE_IO_ERROR: Read failure, element is not deleted
 *  - PROPERTIES_STORAGE_KEY_NOT_FOUND_ERROR: Unknown key
 *  - PROPERTIES_STORAGE_SUCCESS:      Delete succeed
 */
properties_storage_status_t properties_storage_delete(uint32_t key);

/** @} */

#endif /* __PROPERTIES_STORAGE_H */
