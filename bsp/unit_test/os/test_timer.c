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

#include <stdbool.h>

#include "os/os.h"
#include "utility.h"
#include "util/cunit_test.h"
#include "test_stub.h"

#if 0 /* debug log disable */
#define _log_debug_timer_test(_fmt_, args ...)  cu_print(_fmt_, ## args)
#else
#define _log_debug_timer_test(_fmt_, args ...)
#endif

/* This values can be changed/tweaked if the tests are a bit too aggressive and fail */
#define CALLBACK_DELAY (10)

typedef enum {
	E_CALLBACK_RESET_COUNTER = 0,
	E_CALLBACK_INCREMENT_COUNTER,
	E_CALLBACK_STOP_COUNTER,
	E_CALLBACK_TIME_VERIFICATION
} T_TIMER_CALLBACK;

/** verification structure */
typedef struct {
	T_TIMER_CALLBACK clbk_type;
	uint32_t start_time;
	uint32_t delay;
	uint32_t tolerance;
	volatile uint32_t callback_counter;
	volatile bool b_callback_called;
} time_verif_t;

/** Timer stat*/
typedef struct {
	uint32_t delay;
	uint32_t compute;
	uint32_t previous;
	uint32_t min;
	uint32_t max;
	uint32_t count;
	bool initialized;
} T_TIMER_STAT;


/* -------------- global variable -------------- */
/* prevent from stack overflow : 2 timers are always used */
static T_TIMER timer_pool[TIMER_POOL_SIZE - 2] = { 0 };
static T_TIMER timer = NULL;


/* -------------- local function -------------- */
static void timer_callback_stat(void *data);
static void timer_callback_empty(void *data);
static void timer_callback(void *data);
static bool check_time(uint32_t previous, uint32_t current, uint32_t delay,
		       uint32_t tolerance);
static void init_verif_strut(time_verif_t *verif, T_TIMER_CALLBACK clbck_type,
			     uint32_t delay,
			     uint32_t tolerance);


/* -------------- public function impl -------------- */
void test_timer_functions(void)
{
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;

	clear_panic();

	/* Alloc / Dealloc timer */
	timer = timer_create(timer_callback_empty, NULL, 10, true, false, &err);
	CU_ASSERT("timer is not created", err == E_OS_OK && timer != NULL);
	timer_delete(timer);
	CU_ASSERT("delete failed with invalid parameter", did_panic() == false);

	/* Alloc start / stop Dealloc timer */
	timer = timer_create(timer_callback_empty, NULL, 10, true, true, &err);
	CU_ASSERT("timer is not created", err == E_OS_OK && timer != NULL);
	timer_delete(timer);
	CU_ASSERT("delete failed with invalid parameter", did_panic() == false);

	/* Alloc (startup = false), stop , dealloc */
	timer = timer_create(timer_callback_empty, NULL, 10, true, false, &err);
	CU_ASSERT("timer is not created", err == E_OS_OK && timer != NULL);
	timer_stop(timer);
	CU_ASSERT("timer should be not stopped",
		  did_panic() == false && timer != NULL);
	timer_delete(timer);
	CU_ASSERT("delete timer failed", did_panic() == false);

	/* Alloc, start, stop , dealloc */
	timer = timer_create(timer_callback_empty, NULL, 0, true, false, &err);
	CU_ASSERT("timer is not created", err == E_OS_OK && timer != NULL);

	timer_start(timer, 10, &err);
	CU_ASSERT("timer is not started", err == E_OS_OK && timer != NULL);
	timer_start(timer, 10, &err);
	CU_ASSERT("error , timer should be not launch twice",
		  err == E_OS_ERR_BUSY && timer != NULL);

	timer_stop(timer);
	CU_ASSERT("timer is not stopped", did_panic() == false && timer != NULL);
	timer_stop(timer);
	CU_ASSERT("error, timer is not started",
		  did_panic() == false && timer != NULL);

	timer_delete(timer);
	CU_ASSERT("delete timer failed", did_panic() == false);
	timer = NULL;
	timer_delete(timer);
	CU_ASSERT("timer should be not delete", did_panic() == true);
	clear_panic();

	/* Test startup flag */
	timer = timer_create(timer_callback_empty, NULL, 10, true, true, &err);
	CU_ASSERT("timer is not created", err == E_OS_OK && timer != NULL);
	timer_start(timer, 10, &err);
	CU_ASSERT("error , timer should be not launch twice",
		  err == E_OS_ERR_BUSY && timer != NULL);

	timer_stop(timer);
	CU_ASSERT("timer is not stopped", did_panic() == false && timer != NULL);
	timer_stop(timer);
	CU_ASSERT("error, timer is not started",
		  did_panic() == false && timer != NULL);

	timer_delete(timer);
	CU_ASSERT("delete timer failed", did_panic() == false);
	timer = NULL;
	timer_delete(timer);
	CU_ASSERT("delete timer failed", did_panic() == true);
	clear_panic();

	timer = timer_create(timer_callback_empty, NULL, 100, false, true, &err);
	CU_ASSERT("timer is not created with repeat OFF",
		  err == E_OS_OK && timer != NULL);
	timer_stop(timer);
	CU_ASSERT("timer is not stopped", did_panic() == false);
	timer_stop(timer);
	CU_ASSERT("timer should be not stopped", did_panic() == false);
	/* Invalid scenario */
	timer_delete(timer);

	timer = timer_create(NULL, NULL, 1, true, false, &err);
	CU_ASSERT("invalid parameter \"callback\"",
		  err == E_OS_ERR && timer == NULL);
	timer = timer_create(NULL, NULL, 1, true, false, NULL);
	CU_ASSERT("Test: timer_create, with callback NULL - panic",
		  did_panic() == true);
	clear_panic();
	timer = timer_create(timer_callback, NULL, 0, true, true, &err);
	CU_ASSERT("invalid parameter \"delay==0 and startup == true\"",
		  err == E_OS_ERR && timer == NULL);
	timer = timer_create(timer_callback, NULL, 0, true, true, NULL);
	CU_ASSERT("Test: timer_create, with delay=0 and startup=true- panic",
		  did_panic() == true);
	clear_panic();
	timer = timer_create(timer_callback, NULL, OS_NO_WAIT, true, true, &err);
	CU_ASSERT("invalid delay parameter", err == E_OS_ERR && timer == NULL);
	timer = timer_create(timer_callback, NULL, OS_NO_WAIT, true, true, NULL);
	CU_ASSERT("Test: timer_create, with invalid delay parameter - panic",
		  did_panic() == true);
	clear_panic();
	timer_stop(NULL);
	CU_ASSERT("timer should be not stopped", did_panic() == true);
	clear_panic();
	timer_delete(NULL);
	CU_ASSERT("Test: timer_delete, with timer handler = NULL - panic",
		  did_panic() == true);
	clear_panic();
}

void test_timer_allocation(void)
{
	int i;
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;

	clear_panic();

	_log_debug_timer_test("create %d timers.\n", DIM(timer_pool));

	/* create X Timers with callback */
	for (i = 0; i < DIM(timer_pool); i++) {
		timer_pool[i] =
			timer_create(timer_callback_empty, (void *)i, 100, true,
				     true,
				     &err);
		CU_ASSERT("create timer alloc failed",
			  timer_pool[i] != NULL && err == E_OS_OK);
	}

	local_task_sleep_ms(10); /* wait 10 ms */
	CU_ASSERT("No panic expected", did_panic() == false);
	clear_panic();

	_log_debug_timer_test("stop the %d timers.\n", DIM(timer_pool));

	/* stop X timers */
	for (i = 0; i < DIM(timer_pool); i++) {
		timer_stop(timer_pool[i]);
		CU_ASSERT("timer stop failed", did_panic() == false);
	}
	/* delete X timers */
	for (i = 0; i < DIM(timer_pool); i++) {
		timer_delete(timer_pool[i]);
		CU_ASSERT("timer delete failed", did_panic() == false);
	}
	CU_ASSERT("No panic expected", did_panic() == false);
	clear_panic();

	/* Test to ADD timer into the linked list from Head/Middle/Tail.(delay=4/5/6ms)   -> (i%3)+4 */
	_log_debug_timer_test("create %d timer.\n", DIM(timer_pool));

	/* create X Timers with callback */
	for (i = 0; i < DIM(timer_pool); i++) {
		timer_pool[i] =
			timer_create(timer_callback_empty, (void *)i,
				     ((i % 3) + 4) * 10,
				     true, true,
				     &err);
		CU_ASSERT("create timer callback failed",
			  timer_pool[i] != NULL && err == E_OS_OK);
	}
	_log_debug_timer_test("created %d timer.\n", i);

	local_task_sleep_ms(100); /* wait 100 ms */

	CU_ASSERT("No panic expected", did_panic() == false);
	clear_panic();

	_log_debug_timer_test("stop the %d timers.\n", DIM(timer_pool));

	/* stop X timers */
	for (i = 0; i < DIM(timer_pool); i++) {
		timer_stop(timer_pool[i]);
		CU_ASSERT("timer stop failed", did_panic() == false);

		timer_delete(timer_pool[i]);
		CU_ASSERT("timer delete failed", did_panic() == false);
	}
	CU_ASSERT("No panic expected", did_panic() == false);
	clear_panic();
}

/* test timer callback  */
void test_timer_callback(void)
{
	uint32_t previous_time = get_time_ms();
	uint32_t current_time;
	volatile uint32_t i;
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;
	uint32_t delay = CALLBACK_DELAY;
	uint32_t delay_start = CALLBACK_DELAY * 2;
	time_verif_t verif;

	init_verif_strut(&verif, E_CALLBACK_INCREMENT_COUNTER, delay, 1);
	verif.callback_counter = 0xFF;

	timer = timer_create(timer_callback, &verif, delay, false, true, &err);
	CU_ASSERT("create timer error failed", err == E_OS_OK);
	local_task_sleep_ms(100);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);

	init_verif_strut(&verif, E_CALLBACK_RESET_COUNTER, delay, 1);
	verif.callback_counter = 0xFF;
	timer = timer_create(timer_callback, &verif, delay, true, true, &err);

	CU_ASSERT("create timer error failed", err == E_OS_OK);
	timer_start(timer, delay, &err);
	CU_ASSERT("timer is not starting", (err == E_OS_ERR_BUSY) &&
		  (timer != NULL));

	local_task_sleep_ms(100);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	CU_ASSERT("counter reseted", verif.callback_counter == 0);

	verif.clbk_type = E_CALLBACK_INCREMENT_COUNTER;

	/* check timer incrementation with 1ms callback function */
	for (i = 0; i < 100; i++) {
		verif.b_callback_called = false;
		current_time = get_time_ms();

		if (check_time(previous_time, current_time, delay,
			       10) == false) {
			CU_ASSERT("timer incrementation failed", 0);
			cu_print("index i = %d & callback %d\n", i,
				 verif.callback_counter);
		}
		previous_time = current_time;

		local_task_sleep_ms(10);
		CU_ASSERT("timer callback is not called",
			  verif.b_callback_called == true);
	}
	timer_stop(timer);
	timer_delete(timer);
	CU_ASSERT("timer stopped", did_panic() == false);
	CU_ASSERT("check counter value", verif.callback_counter == 100);
	if (verif.callback_counter != 100) {
		cu_print("callback_counter = %d expected value is: 100\n",
			 verif.callback_counter);
	}
	verif.b_callback_called = false;

	/* make sure the callback is not called after timer_stop() */
	local_task_sleep_ms(delay + 5);
	CU_ASSERT(
		"check callback not called", verif.callback_counter == 100 &&
		verif.b_callback_called == false);


	/* second test with verification inside of the callback */
	init_verif_strut(&verif, E_CALLBACK_TIME_VERIFICATION, delay, 1);
	timer = timer_create(timer_callback, &verif, delay, true, true, &err);
	CU_ASSERT("create timer error failed", err == E_OS_OK && (timer != NULL));
	/* wait for 3 callbacks and verify */
	local_task_sleep_ms(15);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	verif.b_callback_called = false;
	local_task_sleep_ms(10);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	verif.b_callback_called = false;
	local_task_sleep_ms(10);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	verif.b_callback_called = false;

	timer_stop(timer);
	CU_ASSERT("timer stopped", did_panic() == false);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);
	CU_ASSERT("callback called x time", verif.callback_counter == 3);


	/* third test with time verification inside of the callback */
	init_verif_strut(&verif, E_CALLBACK_TIME_VERIFICATION, delay, 1);
	timer = timer_create(timer_callback, &verif, delay, true, true, &err);
	CU_ASSERT("create timer error failed", err == E_OS_OK && (timer != NULL));
	/* wait for callbacks and verify */
	local_task_sleep_ms(15);
	CU_ASSERT("Timer didn't started and timer callback was not called",
		  verif.b_callback_called == true);
	CU_ASSERT("callback counter value incorrect",
		  verif.callback_counter == 1);

	verif.b_callback_called = false;
	timer_stop(timer);
	CU_ASSERT("timer stopped", did_panic() == false);
	/* wait and verify  the timer was stopped */
	local_task_sleep_ms(10);
	CU_ASSERT("Timer didn't stop and timer callback is called again",
		  verif.b_callback_called == false);
	CU_ASSERT("callback counter value incorrect",
		  verif.callback_counter == 1);

	/* Start the timer with new value: 20 ms */
	verif.b_callback_called = false;
	timer_start(timer, delay_start, &err);
	CU_ASSERT("start timer error", err == E_OS_OK);
	local_task_sleep_ms(25);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	CU_ASSERT("callback counter value incorrect",
		  verif.callback_counter == 2);

	verif.b_callback_called = false;
	local_task_sleep_ms(20);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	CU_ASSERT("callback counter value incorrect",
		  verif.callback_counter == 3);

	timer_stop(timer);
	CU_ASSERT("timer stopped", did_panic() == false);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);
}

/* try to stop the timer from the timer_callback */
void test_timer_callback_with_timer_stop(void)
{
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;
	time_verif_t verif;
	uint32_t delay = CALLBACK_DELAY;

	/* test 1 with repeat callback == TRUE*/
	init_verif_strut(&verif, E_CALLBACK_STOP_COUNTER, delay, 1);
	timer = timer_create(timer_callback, &verif, delay, false, true, &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer != NULL));

	local_task_sleep_ms(100);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);
	CU_ASSERT(
		"callback not called", err == E_OS_OK &&
		verif.callback_counter == 1 &&
		verif.b_callback_called);

	/* timer has been already switched off, timer_stop has no effect */
	timer_stop(timer);
	CU_ASSERT("timer stop must fail", did_panic() == false);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);

	/* test with repeat callback == FALSE */
	init_verif_strut(&verif, E_CALLBACK_STOP_COUNTER, delay, 1);
	timer = timer_create(timer_callback, &verif, delay, false, true, &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer != NULL));
	local_task_sleep_ms(100);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);

	CU_ASSERT(
		"callback not called", err == E_OS_OK &&
		verif.callback_counter == 1 &&
		verif.b_callback_called);

	/* timer has been already switched off, timer_stop has no effect */
	timer_stop(timer);
	CU_ASSERT("timer stop must fail", did_panic() == false);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);
}

/* test timer restart  */
void test_timer_restart(void)
{
	uint32_t count;
	volatile uint32_t i;
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;
	uint32_t delay = CALLBACK_DELAY;
	static time_verif_t verif;

	init_verif_strut(&verif, E_CALLBACK_RESET_COUNTER, delay, 1);
	timer = timer_create(timer_callback, &verif, delay, true, false, &err);
	CU_ASSERT("create timer error failed", err == E_OS_OK && (timer != NULL));

	timer_start(timer, delay, &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer != NULL));

	/* wait until the callback is called */
	local_task_sleep_ms(20);
	CU_ASSERT("timer callback is not called",
		  verif.b_callback_called == true);

	CU_ASSERT("counter reseted", verif.callback_counter == 0);
	verif.callback_counter = 0xFF;
	timer_stop(timer);
	CU_ASSERT("timer stopped", did_panic() == false);
	/* Wait 100ms to be sure callback is never called again */
	local_task_sleep_ms(100);
	CU_ASSERT("timer is always running", 0xFF == verif.callback_counter);

	verif.clbk_type = E_CALLBACK_INCREMENT_COUNTER;
	verif.callback_counter = 0;

	/* check timer incrementation with 1ms callback function */
	for (i = 0; i < 10; i++) {
		verif.b_callback_called = false;
		timer_start(timer, delay, &err);
		CU_ASSERT("timer started", err == E_OS_OK);

		/* Wait 20ms to be sure callback is called twice*/
		local_task_sleep_ms(20);
		CU_ASSERT("timer callback is not called",
			  verif.b_callback_called == true);

		count = verif.callback_counter;
		timer_stop(timer);
		CU_ASSERT("timer stopped", did_panic() == false);
		/* Wait 100ms to be sure callback is never called again */
		local_task_sleep_ms(100);
		CU_ASSERT("timer is always running",
			  count == verif.callback_counter);
	}

	timer_stop(timer);
	verif.b_callback_called = false;
	CU_ASSERT("timer stopped", did_panic() == false);
	/* Wait 100ms to be sure callback is never called again */
	local_task_sleep_ms(100);
	CU_ASSERT("one timer is still running",
		  verif.b_callback_called == false);
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == false);
	timer = NULL;
	timer_delete(timer);
	CU_ASSERT("delete callback failed", did_panic() == true);
	clear_panic();
	/* The callback shall be called at least 20 times */
	if (verif.callback_counter < 20) {
		CU_ASSERT("check callback counter value",
			  verif.callback_counter >= 20);
		cu_print(
			"Counter expected: at least 20 --- current counter: %d\n",
			verif.callback_counter);
	}
}

/* verify the callback sequence*/
void test_timer_stat(void)
{
	OS_ERR_TYPE err = E_OS_ERR_UNKNOWN;
	T_TIMER timer_10ms = NULL;
	T_TIMER timer_30ms = NULL;
	T_TIMER timer_50ms = NULL;
	T_TIMER_STAT stat_10ms = { 10000, 0, 0, 0, 0, 0, false }; // 10000 us
	T_TIMER_STAT stat_30ms = { 30000, 0, 0, 0, 0, 0, false };
	T_TIMER_STAT stat_50ms = { 50000, 0, 0, 0, 0, 0, false };
	uint32_t margin = CONVERT_TICKS_TO_MS(1) * 1000; // timer resolution (us)

	timer_10ms =
		timer_create(timer_callback_stat, &stat_10ms, 10, true, true,
			     &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer_10ms != NULL));
	timer_30ms =
		timer_create(timer_callback_stat, &stat_30ms, 30, true, true,
			     &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer_30ms != NULL));
	timer_50ms =
		timer_create(timer_callback_stat, &stat_50ms, 50, true, true,
			     &err);
	CU_ASSERT("timer set callback", (err == E_OS_OK) && (timer_50ms != NULL));

	/* wait 2 seconds */
	local_task_sleep_ms(2000);

	timer_stop(timer_50ms);
	CU_ASSERT("timer counter reseted", did_panic() == false);
	timer_stop(timer_10ms);
	CU_ASSERT("timer counter reseted", did_panic() == false);
	timer_stop(timer_30ms);
	CU_ASSERT("timer counter reseted", did_panic() == false);

	/* verify the delay MIN and MAX are in the 2ms range in Zephyr (Micro) context or 20ms range in Zephyr (Nano) context */
	CU_ASSERT("max jitter timer 10ms not in range",
		  (stat_10ms.max - stat_10ms.delay) <= margin ||
		  (stat_10ms.max - stat_10ms.delay) >= -margin);
	CU_ASSERT("min jitter timer 10ms not in range",
		  (stat_10ms.min - stat_10ms.delay) <= margin ||
		  (stat_10ms.min - stat_10ms.delay) >= -margin);
	CU_ASSERT("max jitter timer 30ms not in range",
		  (stat_30ms.max - stat_30ms.delay) <= margin ||
		  (stat_30ms.max - stat_30ms.delay) >= -margin);
	CU_ASSERT("min jitter timer 30ms not in range",
		  (stat_30ms.min - stat_30ms.delay) <= margin ||
		  (stat_30ms.min - stat_30ms.delay) >= -margin);
	CU_ASSERT("max jitter timer 50ms not in range",
		  (stat_50ms.max - stat_50ms.delay) <= margin ||
		  (stat_50ms.max - stat_50ms.delay) >= -margin);
	CU_ASSERT("min jitter timer 50ms not in range",
		  (stat_50ms.min - stat_50ms.delay) <= margin ||
		  (stat_50ms.min - stat_50ms.delay) >= -margin);
}


/******************* LOCAL FUNCTION *******************************/
static void timer_callback_stat(void *data)
{
	uint32_t delta;
	T_TIMER_STAT *stat = (T_TIMER_STAT *)data;

	if (stat) {
		/* struct not initialized */
		if (stat->initialized == false) {
			stat->compute = 0;
			stat->max = 0;
			stat->min = ~0;
			stat->initialized = true;
			stat->previous = get_time_us();
		} else {
			delta = get_time_us() - stat->previous;
			stat->compute += delta;
			if (delta > stat->max)
				stat->max = delta;
			if (delta < stat->min)
				stat->min = delta;
			stat->count++;
		}
		stat->previous = get_time_us();
	}
}

static void timer_callback_empty(void *data)
{
//    int timerid = (int)data;
//    _log_debug_timer_test("timer_callback_empty from timer %d.\n", timerid);
}

static void timer_callback(void *data)
{
	time_verif_t *p = (time_verif_t *)data;

	if (data != NULL) {
		p->b_callback_called = true;
		p->callback_counter++;

		if (p->clbk_type == E_CALLBACK_RESET_COUNTER) {
			p->callback_counter = 0;
		} else if (p->clbk_type == E_CALLBACK_INCREMENT_COUNTER) {
			;
		} else if (p->clbk_type == E_CALLBACK_STOP_COUNTER) {
			timer_stop(timer);
			CU_ASSERT("callback stop its own timer",
				  did_panic() == false);
		} else if (p->clbk_type == E_CALLBACK_TIME_VERIFICATION) {
			uint32_t current_time = get_time_ms();
			bool callback_called_on_time =
				check_time(p->start_time, current_time,
					   p->delay,
					   p->tolerance);
			CU_ASSERT("callback not called on time",
				  callback_called_on_time);
			p->start_time = current_time;
		} else {
			CU_ASSERT("should never reach this point", false);
		}
	} else {
		CU_ASSERT("should never reach this point", false);
	}
}

static bool check_time(uint32_t previous, uint32_t current, uint32_t delay,
		       uint32_t tolerance)
{
	uint32_t expected_current = previous + delay;

	if (expected_current >= previous) {
		if (current <= (expected_current + tolerance) || current >=
		    (expected_current - tolerance)) {
			return true;
		} else {
			cu_print(
				"ERROR: Delta between callback and expected is bigger than the allowed tolerance.\n");
			cu_print(
				"callback_delay:%d current_delta %d tolerance %d\n",
				delay, current - previous, tolerance);
		}
	}
	return false;
}

static void init_verif_strut(time_verif_t *verif, T_TIMER_CALLBACK clbck_type,
			     uint32_t delay,
			     uint32_t tolerance)
{
	verif->clbk_type = clbck_type;
	verif->start_time = get_time_ms();
	verif->delay = delay;
	verif->tolerance = tolerance;
	verif->callback_counter = 0;
	verif->b_callback_called = false;
	return;
}
