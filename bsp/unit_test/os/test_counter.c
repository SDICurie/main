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

#include "util/cunit_test.h"
#include "os/os.h"

/* test millisecond timer incrementation */
void test_counter_millisecond_incrementation(void)
{
	volatile uint32_t ms_time = get_time_ms();
	uint32_t us_time;
	volatile uint32_t ms_time_new = 0;
	volatile uint32_t us_time_new = 0;

	/* initialization */
	/* wait millisecond transition from (X)ms to (X+1)ms */
	while ((ms_time_new = get_time_ms()) <= ms_time) {
		;
	}

	ms_time = ms_time_new;
	us_time = get_time_us();

	// Wait 1ms
	while ((us_time_new = get_time_us()) < (us_time + 1000)) {
		;
	}

	ms_time_new = get_time_ms();
	CU_ASSERT("1 ms timer incrementation failed", ms_time_new ==
		  (ms_time + CONVERT_TICKS_TO_MS(1)));

	/* test 2 */
	{
		uint32_t t1, t2;
		t1 = get_time_ms();
		t2 = get_time_ms();
		CU_ASSERT("should be the same", t1 == t2);
	}
}

/* test micro second timer incrementation */
void test_counter_microsecond_incrementation(void)
{
	volatile uint32_t ms_time = get_time_ms();
	volatile uint32_t us_time;
	volatile uint32_t us_time_new;
	volatile uint32_t ms_time_new = 0;

	/* initialization */
	/* wait millisecond transition from (X)ms to (X+1)ms */
	while ((ms_time_new = get_time_ms()) <= ms_time) {
		;
	}

	/* save micro second */
	us_time = get_time_us();
	ms_time = get_time_ms();
	ms_time_new = 0;

	// Wait 1ms
	while ((ms_time_new = get_time_ms()) <= ms_time) {
		;
	}

	us_time_new = get_time_us();
	CU_ASSERT("microsecond timer incrementation failed",
		  ((us_time_new >=
		    (us_time + 1000 * CONVERT_TICKS_TO_MS(1))) &&
		   (us_time_new <=
		    (us_time + 1000 * CONVERT_TICKS_TO_MS(1) + 50))));

	/* test 2 */
	{
		uint32_t t1, t2;
		t1 = get_time_us();
		t2 = get_time_us();
		CU_ASSERT("should be the same", t1 == t2);
	}
}
