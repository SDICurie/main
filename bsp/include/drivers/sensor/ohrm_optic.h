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
#ifndef __OHRM_OPTIC_H__
#define __OHRM_OPTIC_H__
/* Definitions */
typedef enum {
	MUX_16M = 0,
	MUX_8M = 1,
	MUX_4M = 2,
	MUX_2M = 3,
	MUX_1M = 4,
	MUX_500k = 5,
	MUX_250k = 6,
	MUX_125k = 7,
	NUM_MUX_POSITIONS
} mux_code_t;

typedef struct {
	/* input */
	uint16_t adc_conv;
	/* output codes */
	mux_code_t mux_code;
	uint16_t dac_code;
	uint8_t dcp_code;
} optic_track_mem_t;

/* Function Declarations */
void alg_optic_init(optic_track_mem_t *p_track_mem);
void alg_optic(optic_track_mem_t *p_track_mem);
uint8_t alg_hr_on_wrist_detection(uint16_t blind_adc, uint8_t dcp_code,
				  uint8_t mux_code);
#endif
