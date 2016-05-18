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

#ifndef __CONSOLE_MANAGER_H
#define __CONSOLE_MANAGER_H

#include <stdbool.h>

/**
 * @defgroup console_manager Console manager
 * Console management to enable log output
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/console_manager.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * <tr><th><b>Config flag</b> <td><tt>CONSOLE_MANAGER</tt>
 * </table>
 *
 * A console backend defines a way to output log messages.
 *
 * Before any use, the console manager must be initialized through
 * @ref console_manager_init. This is automatically done by @ref bsp_init.
 *
 * @ingroup infra
 * @{
 */

/**
 * Define the content of a console backend
 */
typedef struct console_backend {
	/** Name of the console backend */
	const char *name;
	/** Function to initialize the console backend */
	void (*init)(void);
	/** Function to activate the console backend */
	void (*activate_log_backend)(bool activate);
	/** Structure containing the function to output the message */
	struct log_backend *log_backend;
	/** Boolean to know if the console backend is the default one */
	bool default_log_backend;
} console_backend_t;


/**
 * Initialize the console.
 */
void console_manager_init(void);


/**
 * Activate the log backend
 *
 * @param console_name Console name
 * @param activate `true`to activate, `false`to deactivated
 */
int console_manager_activate_log_backend(const char *	console_name,
					 bool		activate);


/** @} */

#endif /* __CONSOLE_MANAGER_H */
