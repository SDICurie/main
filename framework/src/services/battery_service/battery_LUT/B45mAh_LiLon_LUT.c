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

#include "os/os.h"

#include "battery_LUT.h"

/*
 * @brief Completion of 4 Lookup table :
 * dflt_lookup_tables[0] corresponding to a period of charge
 * dflt_lookup_tables[1] corresponding to Temperature below +6C
 * dflt_lookup_tables[2] corresponding to Temperature between +7C and +12C
 * dflt_lookup_tables[3] corresponding to Temperature above 20C
 *
 * @remark for each lookup table :
 *  - voltage measured @ index ∈ [0;9] are respectively corresponding to fuel gauge ∈ [0;9] %
 *  - voltage measured @ index ∈ [15;24] are respectively corresponding to fuel gauge ∈ [91;100] %
 *  - if voltage measured is between :
 *      - voltage corresponding to the one for 10% and 30%
 *      - voltage corresponding to the one for 31% and 50%
 *      - voltage corresponding to the one for 51% and 70%
 *      - voltage corresponding to the one for 71% and 90%
 *      A linearization is realized
 */

/*  Characteristics of battery 45mAh Li-Lon */
const uint16_t dflt_lookup_tables[BATTPROP_LOOKUP_TABLE_COUNT][
	BATTPROP_LOOKUP_TABLE_SIZE] = {
	{
		3400, 3522, 3570, 3610, 3655,
		3690, 3700, 3720, 3733, 3748,
		3830, 3940, 3990, 4110, 4298,
		4300, 4305, 4309, 4312, 4317,
		4320, 4324, 4329, 4335, 4340
	},
	{
		3320, 3335, 3348, 3363, 3375,
		3383, 3392, 3404, 3411, 3420,
		3449, 3605, 3703, 3828, 3992,
		4026, 4052, 4085, 4108, 4125,
		4143, 4158, 4170, 4188, 4239
	},
	{
		3320, 3347, 3360, 3405, 3417,
		3431, 3462, 3488, 3506, 3527,
		3553, 3664, 3754, 3877, 4051,
		4084, 4110, 4142, 4166, 4182,
		4196, 4208, 4220, 4234, 4254
	},
	{
		3320, 3390, 3405, 3440, 3465,
		3481, 3511, 3534, 3545, 3560,
		3581, 3685, 3773, 3905, 4070,
		4105, 4131, 4163, 4186, 4202,
		4215, 4227, 4239, 4253, 4265
	},
};

const uint16_t dflt_lookup_tables2[BATTPROP_LOOKUP_TABLE_COUNT][
	BATTPROP_LOOKUP_TABLE_SIZE];
