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

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "os/os.h"

#ifdef CONFIG_MICROKERNEL

#endif

/* useful to know the number of elements into an array (the dimension) */
#define DIM(x)  (sizeof(x) / sizeof(x[0]))

/** IRQ line used for tests from IRQ contexts */
#define UNIT_TESTS_IRQ_LINE      (32 + 21)   /* User IRQ 21: mailboxes */


/* Type for functions to be set as task entry point */
typedef void (*T_TASK_FCT)(void);


typedef void (*T_CALLBACK)(void *data);

void ipc_msg_handler_set(void (*hdl)(void));

/* Utility services exports */
extern uint32_t get_time_ms(void);
extern void cunit_start_tasks(void);

extern void trigger_test_interrupt(void);

/* Intel instruction to trigger a SOFTWARE interrupt  (required and 'immediate' value)
 * Interrupt triggered with this ASM instruction are non-maskable (sw interrupt) */
#define TRIGGER_SW_INTERRUPT(irq)      __asm__("INT %0" : : "i" (irq));


typedef enum {
	E_CALLBACK_MUTEX = 0,
	E_CALLBACK_SEMA,
	E_CALLBACK_QUEUE,
	E_CALLBACK_IRQ,
	E_CALLBACK_MALLOC,
	E_CALLBACK_FREE
} T_CALLBACK_FUNCTION;

/** interrupt structure to pass to the interrupt stub handler */
typedef struct {
	T_CALLBACK_FUNCTION type;
	void *data;
	volatile bool isr_called;
} interrupt_param_t;

void trigger_mbx_isr(interrupt_param_t *);
bool wait_mbx_isr(interrupt_param_t *param);
void mbx_status_reset(void);
void test_interrupt_init(void);


#endif    /* __UTILITY_H__  */
