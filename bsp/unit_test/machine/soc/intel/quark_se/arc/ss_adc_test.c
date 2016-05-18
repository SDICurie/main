/*******************************************************************************
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "drivers/ss_adc.h"
#include "machine.h"
#include "util/cunit_test.h"

#define TEST_CHANNEL        10
#define NUM_ADC_CHANNELS    19
#define TEST_ADC_DEV        SS_ADC_ID

static void adc_read(uint32_t channel_id)
{
	uint16_t data;
	uint32_t result;
	DRIVER_API_RC ret = ss_adc_read(channel_id, &data);

	CU_ASSERT("ADC read ERROR\n", (ret == DRV_RC_OK));
	result = ss_adc_data_to_mv(data, ADC_RESOLUTION);
	cu_print("Channel[%d] = %d - %d.%d Volts\n",
		 channel_id, data,
		 result / 1000, result % 1000);
}

static void adc_read_test(void)
{
	int i;

	cu_print("Starting ADC read Test\n");
	cu_print(" Gathers samples from all ADC channels.\n");
	for (i = 0; i < NUM_ADC_CHANNELS; i++)
		adc_read(i);
}

void adc_test(void)
{
	cu_print("#################################\n");
	cu_print("Starting ADC Test\n");
	cu_print("#################################\n");

	adc_read_test();

	cu_print("#################################\n");
	cu_print("End of ADC test\n");
	cu_print("#################################\n");
}
