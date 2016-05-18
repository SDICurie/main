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

#ifndef __LOG_H
#define __LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <infra/log_backend.h>
#include <stdarg.h>
#include <string.h>
#include "util/misc.h"

/**
 * @defgroup infra_log Log
 * Output ASCII messages
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "infra/log.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/infra</tt>
 * </table>
 *
 * @ingroup infra
 * @{
 */

/** Log levels. */
enum log_levels {
	LOG_LEVEL_ERROR,    /*!< Error log level */
	LOG_LEVEL_WARNING,  /*!< Warning log level */
	LOG_LEVEL_INFO,     /*!< Info log level */
	LOG_LEVEL_DEBUG,    /*!< Debug log level, requires debug to be activated for
	                     * this module at compile time */
	LOG_LEVEL_NUM
};

/** Message sent to log_printk larger than this size will be truncated on some
 * implementations. */
#define LOG_MAX_MSG_LEN (80)

/**
 * Initializes the log instance.
 * This function is called in @ref bsp_init.
 *
 * This function must be called before writing any log. In most cases, a call to
 * log_set_backend() is also required to set log output. An exception to this
 * rule is the case of a log "slave" when multi-core log is used which doesn't
 * need a backend.
 */
void log_init();

/**
 * Set the log backend.
 *
 * This function can be called before log_init(), and also at any time later
 * for example to change the log backend at a given point in time.
 *
 * @param backend log_backend to be used by this logger instance
 */
void log_set_backend(struct log_backend backend);

/**
 * Initialize and start the logger task.
 * This function is called by @ref bsp_init
 */
void log_start(void);

/**
 * Send a user's log message into the backend.
 *
 * On some buffered implementations the message is not immediately output to the
 * backend. Call the log_flush() function to make sure that all messages are
 * really output.
 * Message longer than LOG_MAX_MSG_LEN will be truncated on some implementations.
 *
 * @param level Log level for this message
 * @param module_short_name Short name of the log module. Must be 4
 * characters long (5 including trailing \0)
 * @param format printf-like string format
 */
void log_printk(uint8_t level, const char *module_short_name,
		const char *format,
		...);

/**
 * Same as log_printk() except that this function is called with a va_list
 * instead of a variable number of arguments.
 *
 * @param level Log level for this message
 * @param module_short_name Short_name of the log module. Must be < 4
 * characters (5 including trailing \0)
 * @param format printf-like string format
 * @param args
 */
void log_vprintk(uint8_t level, const char *module_short_name,
		 const char *format,
		 va_list args);

/**
 * Get the human-friendly name of a log level.
 *
 * @param level Level id
 * @return Log level name or NULL if not found
 */
const char *log_get_level_name(uint8_t level);

/**
 * Set the global log level value. This acts as a maximum overall level limit.
 *
 * @param level  New log level value, from the log level enum
 * @return -1 if error,
 *          0 if new level was set
 */
int8_t log_set_global_level(uint8_t level);

/**
 * Get the global log level applying to all modules.
 *
 * @return Global log level
 */
uint8_t log_get_global_level();


/**
 * On bufferized implementations, make sure that all pending messages are
 * flushed to the log_backend. This function has no effect on unbuffered
 * implementations.
 */
void log_flush();

/**
 * Suspend logger task.
 * This is used to prevent logging from interrupting any lower priority
 * work. Deep sleep process is an example.
 */
void log_suspend();

/**
 * Resume logger task.
 */
void log_resume();

/**
 * Log an error message.
 *
 * @param module_id ID of the module related to this message
 * @param format printf-like string format
 */
#define pr_error(module_id, format, ...) log_printk(LOG_LEVEL_ERROR, module_id,	\
						    format, \
						    ## __VA_ARGS__)

/**
 * Log a warning message.
 *
 * @param module_id ID of the log module related to this message
 * @param format printf-like string format
 */
#define pr_warning(module_id, format, ...) log_printk(LOG_LEVEL_WARNING, \
						      module_id, format, \
						      ## __VA_ARGS__)

/**
 * Log an info message.
 *
 * @param module_id ID of the log module related to this message
 * @param format printf-like string format
 */
#define pr_info(module_id, format, ...) log_printk(LOG_LEVEL_INFO, module_id, \
						   format, \
						   ## __VA_ARGS__)

/**
 * Log a debug message.
 *
 * Note that this call will have an effect only if the log module was declared
 * using the `DEFINE_LOG_MODULE_DEBUG` macro.
 *
 * @param module_id ID of the log module related to this message
 * @param format printf-like string format
 */
#define pr_debug(module_id, format, ...) pr_debug_ ## module_id(format,	\
								## __VA_ARGS__)

/**
 * Declare a log module with a specific ID.
 *
 * The log module can be used only in the scope where it was declared.
 *
 * For a module declared this way, the pr_debug() function will be a
 * no-op. Use `DEFINE_LOG_MODULE_DEBUG` instead to activate debugging logs.
 *
 * @param module_id Id to use in later calls to pr_xxx() functions family
 * @param short_name Short name which will be printed in log consoles.
 * Must be < 4 characters (5 including trailing \0)
 */
#define DEFINE_LOG_MODULE(module_id, \
			  short_name) static const char *const module_id = \
	short_name; \
	STATIC_ASSERT(sizeof(short_name) == 5);	\
	inline static void notrace pr_debug_ ## module_id(const char *format, \
							  ...) {; }

/**
 * Like `DEFINE_LOG_MODULE` but also activates pr_debug() for this log module.
 */
#define DEFINE_LOG_MODULE_DEBUG(module_id, \
				short_name) static const char *const module_id \
		= short_name; \
	STATIC_ASSERT(sizeof(short_name) == 5);	\
	inline static void notrace pr_debug_ ## module_id(const char *format, \
							  ...) { \
		va_list args; \
		va_start(args, format);	\
		log_vprintk(LOG_LEVEL_DEBUG, module_id, format, args); \
		va_end(args); \
	}

/* Define some global log modules */
DEFINE_LOG_MODULE(LOG_MODULE_MAIN, "MAIN")
DEFINE_LOG_MODULE(LOG_MODULE_USB, " USB")
DEFINE_LOG_MODULE(LOG_MODULE_BLE, " BLE")
DEFINE_LOG_MODULE(LOG_MODULE_DRV, " DRV")
DEFINE_LOG_MODULE(LOG_MODULE_UTIL, "UTIL")

/** @} */

#endif /* __LOG_H */
