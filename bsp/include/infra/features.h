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

#ifndef __FEATURES_H__
#define __FEATURES_H__

#include <stdint.h>

/* This will contain all board features */
extern volatile uint32_t *board_features;

/**
 * @defgroup features Board features
 * Board features tools.
 *
 * For more information on board features, see @ref board_features.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/features.h"</tt>
 * </table>
 *
 * @ingroup infra
 * @{
 */

/**
 * Check board feature status.
 * see @ref board_features
 *
 * @param _x Feature to check
 */
#define board_feature_has(_x) \
	((*board_features >> _x) & 0x1)

/**
 * Disable a board feature.
 * see @ref board_features
 *
 * @param _x Feature to disable
 */
#define board_feature_disable(_x) \
	*board_features &= ~(1 << _x)

/**
 * Enable a board feature.
 * see @ref board_features
 *
 * @param _x Feature to enable
 */
#define board_feature_enable(_x) \
	*board_features |= (1 << _x)

/** @} */

#endif
