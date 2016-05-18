/****************************************************************************************
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
 ***************************************************************************************/
#ifndef __OPENCORE_SUPPORT_H__
#define __OPENCORE_SUPPORT_H__

#include "opencore_common.h"
#include "opencore_method.h"

#define PARSE_RAW_DATA_HEAD       1
extern char _s_feedinit[], _e_feedinit[];
extern char _s_exposedinit[], _e_exposedinit[];
extern feed_general_t atlsp_algoC;

void OpencoreCommitSensData(uint8_t type, uint8_t id, void *data_ptr,
			    int data_length);
int motion_detect_callback(struct sensor_data *sensor_data, void *priv_data);
void TriggerAlgoEngine(uint16_t msg_id, void *priv_data);

#ifdef SUPPORT_INTERRUPT_MODE
static int raw_data_reg_int_cb(struct sensor_data *sensor_data, void *priv_data);
#endif

static inline void ShareListAdd(list_t *list, list_t *node)
{
	node->next = list->next;
	list->next = node;
}

static inline void ShareListInit(list_t *node)
{
	node->next = node;
}
#endif
