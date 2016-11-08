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

#ifndef __PARTITION_H__
#define __PARTITION_H__

/**
 * @defgroup infra_partition Partition Table
 * Gives access to partitions definitions.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/partition.h"</tt>
 * </table>
 *
 * The partition table is shared in RAM by the bootloader and made available
 * to the cores.
 *
 * See @ref memory_partitioning for an explaination of how it is defined.
 *
 * @ingroup infra
 * @{
 */

#include "infra/part.h"


/**
 *
 * Returns the partition definition for the specified id.
 *
 * @param id The partition id
 *
 * @return The partition structure or NULL
 */
struct partition *partition_get(uint8_t id);

/**
 *
 * Returns the partition definition for the specified name.
 *
 * @param name The partition friendly name
 *
 * @return The partition structure or NULL
 */
struct partition *partition_get_by_name(char *name);

void partition_printk(void);
/** @} */

#endif /* __PARTITION_H__ */
