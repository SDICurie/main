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

#ifndef __OS_H__
#define __OS_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * sys_clock_us_per_tick global variable forward declaration that represents a
 * number of microseconds in one OS timer tick
 */
extern int sys_clock_us_per_tick;
#define CONVERT_MS_TO_TICKS(ms)        (((ms) * sys_clock_ticks_per_sec) / 1000)
#define CONVERT_TICKS_TO_MS(ticks)     ((ticks) * (sys_clock_us_per_tick / 1000))
#define CONVERT_TICKS_TO_32K(ticks)    ((ticks) * \
					(uint64_t)sys_clock_us_per_tick * \
					32768 / 1000000)
#define CONVERT_TICKS_TO_US(ticks)     ((ticks) * sys_clock_us_per_tick)


#ifndef CONFIG_ARC_OS_UNIT_TESTS

/** Total number of element shared by the queues */
#define QUEUE_ELEMENT_POOL_SIZE CONFIG_QUEUE_ELEMENT_POOL_SIZE
/** Total number of timers provided by the abstraction layer */
#define TIMER_POOL_SIZE CONFIG_TIMER_POOL_SIZE

#else

/** Total number of element shared by the queues */
#define QUEUE_ELEMENT_POOL_SIZE 30
/** Total number of timers provided by the abstraction layer */
#define TIMER_POOL_SIZE 8

#endif


/**
 * @defgroup os OS Abstraction Layer
 * OS Abstraction Layer API.
 *
 * This layer defines functions to handle many objects regardless of the
 * low-level OS. This allows inter-operability.
 * - Memory allocation
 * - Sempahore (count)
 * - Mutex (binary)
 * - Read/Write lock
 * - Scheduling
 * - Queue
 * - Time / timer
 * - Task delay
 *
 * Before any API use, the OS Abstraction Layer must be initialized with \ref os_init;
 * As part of the Software Infrastructure, the OS Abstraction Layer is automatically
 * initialized with the infrastructure on \ref bsp_init.
 *
 * Not all the functions can be called from any context.
 * In each category, the allowed type of context is specified for each function.
 *
 * @{
 *
 */

/**********************************************************
************** General definitions  **********************
**********************************************************/
#define UNUSED(p) (void)(p)     /**< Macro for unused arguments, to prevent warnings */

/** Generic OS type for execution status. */
typedef enum {
	E_OS_OK = 0,                    /**< Generic OK status. */
	/* use negative values for errors */
	E_OS_ERR = -1,                  /**< Generic error status. */
	E_OS_ERR_TIMEOUT = -2,          /**< Timeout expired. */
	E_OS_ERR_BUSY = -3,             /**< Resource is not available. */
	E_OS_ERR_OVERFLOW = -4,         /**< Service would cause an overflow. */
	E_OS_ERR_EMPTY = -5,            /**< No data available (e.g. queue is empty). */
	E_OS_ERR_NOT_ALLOWED = -6,      /**< Service is not allowed in current execution context. */
	E_OS_ERR_NO_MEMORY = -7,        /**< All allocated resources are already in use */
	E_OS_ERR_NOT_SUPPORTED = -8,    /**< Service is not supported on current context or OS. */
	E_OS_WDT_EXPIRE = -9,           /**< Quark watchdog expire. */
	E_OS_ASSERT_FAIL = -10,         /**< Assertion error. */
	/* more error codes to be defined */
	E_OS_ERR_UNKNOWN = -100,        /**< Invalid error code (bug?). */
} OS_ERR_TYPE;



/* Types for kernel objects. */
typedef void *T_SEMAPHORE;              /**< Handle of Semaphore. */
typedef void *T_MUTEX;                  /**< Handle of Mutex. */
typedef void *T_QUEUE;                  /**< Handle of Queue. */
typedef void *T_QUEUE_MESSAGE;          /**< Handle of QueueMessage. */
typedef void *T_TIMER;                  /**< Handle of Timer. */
typedef void (*T_ENTRY_POINT)(void *);  /**< Handle of Entry Point. */

/* Special values for \c timeout parameter. */
#define OS_NO_WAIT      0       /**< The blocking function returns immediately */
#define OS_WAIT_FOREVER -1      /**< The blocking function will wait indefinitely */

/**
 * @defgroup os_semaphore Semaphore
 * Control functions for semaphores.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * Semaphores are used to control concurrent accesses to data from multiple
 * processes.
 *
 * For example, an interrupt can fill a buffer, while a task reads the buffer
 * periodically.
 *
 * The maximum number of semaphores is defined by \c SEMAPHORE_POOL_SIZE (32).
 *
 * Not all the functions can be called from interrupt context. Here is a
 * summary of the compatible contexts for each function.
 *
 * Function name            | Task ctxt | Fiber ctxt| Interrupt |
 * -------------------------|:---------:|:---------:|:---------:|
 * @ref semaphore_create    |     X     |     X     |           |
 * @ref semaphore_delete    |     X     |     X     |           |
 * @ref semaphore_give      |     X     |     X     |     X     |
 * @ref semaphore_take      |     X     |     X     |           |
 * @ref semaphore_get_count |     X     |     X     |     X     |
 *
 * @{
 */

/**
 * Create a semaphore.
 *
 * Create or reserve a semaphore object. The service may fail if all allocated
 * semaphores are already being used.
 *
 * @warning This service may panic if:
 * - no semaphore is available, or
 * - when called from an ISR.
 *
 * Semaphores are not dynamically allocated and destroyed. They are picked from
 * a (limited) static pool that is defined at configuration time.
 *
 * Concurrent accesses to this pool are serialized by a framework-specific
 * semaphore.
 *
 * The total number of semaphores that may be in use (created) at the same time
 * is \c SEMAPHORE_POOL_SIZE.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param initialCount Initial count of the semaphore.
 *
 * @return
 *    - Handler of the created semaphore,
 *    - NULL if all allocated semaphores are already being used.
 */
extern T_SEMAPHORE semaphore_create(uint32_t initialCount);

/**
 * Delete a semaphore.
 *
 * Disable a semaphore that was reserved by semaphore_create. Deleting a
 * semaphore while a task is waiting (or will wait) for it to be signaled
 * may create a deadlock.
 *
 * @warning This service may panic if:
 * - semaphore parameter is invalid, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * See also @ref semaphore_create.
 *
 * @param semaphore Handle of the semaphore to delete
 *                  (as returned by @ref semaphore_create).
 *
 */
extern void semaphore_delete(T_SEMAPHORE semaphore);

/**
 * Give/signal a semaphore.
 *
 * @warning This service may panic if err parameter is null and
 * semaphore parameter is invalid.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param semaphore Handle of the semaphore to signal
 *                  (as returned by @ref semaphore_create).
 *
 * @param[out] err  Execution status:
 *     - E_OS_OK Semaphore was freed/signaled,
 *     - E_OS_ERR Semaphore parameter is invalid, or was not created.
 */
extern void semaphore_give(T_SEMAPHORE semaphore, OS_ERR_TYPE *err);

/**
 * Take/block a semaphore.
 *
 * This service may block while waiting on the semaphore,
 * depending on the timeout parameter. This service shall not be called from
 * an ISR context.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param semaphore Handle of the semaphore to take
 *                  (as returned by @ref semaphore_create).
 *
 * @param timeout Maximum number of milliseconds to wait for the semaphore.
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 *
 * @return Execution status:
 *      - E_OS_OK Semaphore was successfully taken,
 *      - E_OS_ERR Semaphore parameter is invalid
 *                 (semaphore was deleted or never created),
 *      - E_OS_ERR_TIMEOUT Could not take semaphore before timeout expiration,
 *      - E_OS_ERR_BUSY Could not take semaphore (did not wait),
 *      - E_OS_ERR_NOT_ALLOWED Service cannot be executed from ISR context.
 */
extern OS_ERR_TYPE  semaphore_take(T_SEMAPHORE semaphore, int timeout);

/**
 * Get the semaphore count.
 *
 * Return the count of a semaphore.
 *
 * @warning This service may panic if err parameter is null and
 * semaphore parameter is invalid.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param semaphore Handle of the semaphore
 *                  (as returned by @ref semaphore_create).
 *
 * @param[out] err  Execution status:
 *         - E_OS_OK  Returned count value is correct,
 *         - E_OS_ERR Semaphore parameter is invalid, or was not created.
 *
 * @return
 *        - negative value: number of clients waiting on the semaphore,
 *        - positive value: number of times the semaphore has been signaled.
 */
extern int32_t semaphore_get_count(T_SEMAPHORE semaphore, OS_ERR_TYPE *err);

/**
 * @}
 */

/**
 *
 * @defgroup os_mutex Mutex
 * Control functions for mutexes.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * A mutex is a synchronization object, used to protect shared data from
 * concurrent access by multiple tasks.
 *
 * The maximum number of mutexes is defined by \c MUTEX_POOL_SIZE (32).
 *
 * @warning Mutexes can't be used inside an ISR (interrupt context).
 *
 * Function name            | Task ctxt | Fiber ctxt| Interrupt |
 * -------------------------|:---------:|:---------:|:---------:|
 * @ref mutex_create        |     X     |     X     |           |
 * @ref mutex_delete        |     X     |     X     |           |
 * @ref mutex_unlock        |     X     |     X     |           |
 * @ref mutex_lock          |     X     |     X     |           |
 *
 * @{
 */

/**
 * Create a mutex.
 *
 * Create or reserve a mutex object. The service may fail if all allocated
 * mutexes are already being used.
 *
 * @warning This service may panic if:
 * - no mutex is available, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * Mutexes are not dynamically allocated and destroyed. They are picked from a
 * (limited) static pool that is defined at configuration time.
 *
 * Concurrent accesses to this pool are serialized by a framework-specific
 * semaphore.
 *
 * The total number of mutexes that may be in use (created) at the same time
 * is \c MUTEX_POOL_SIZE.
 *
 * Mutexes shall not be recursive: consecutive calls to mutex_lock shall fail.
 *
 * @note ZEPHYR SPECIFIC:
 * - microKernel: mutexes shall be implemented as Zephyr resources.
 * - nanoKernel: mutexes shall be implemented as Zephyr semaphores.
 *
 * @return
 *     - Handle of the created mutex,
 *     - NULL if all allocated mutexes are already being used.
 */
extern T_MUTEX mutex_create(void);

/**
 * Delete a mutex.
 *
 * Disable a mutex that was reserved using @ref mutex_create. Deleting a mutex while
 * a task is waiting (or will wait) for it to be freed may create a deadlock.
 *
 * @warning This service may panic if err parameter is null and:
 * - mutex parameter is invalid, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b> task, fiber.
 *
 * @param mutex Handle of the mutex to delete (as returned by @ref mutex_create).
 *
 */
extern void mutex_delete(T_MUTEX mutex);

/**
 * Unlock/give a mutex.
 *
 * @warning This service may panic if:
 * - mutex parameter is invalid, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @note ZEPHYR SPECIFIC:<br/>
 * Unlocking a free mutex will not panic (because ZEPHYR primitives
 * do not return an execution status).
 *
 * @param mutex Handle of the mutex to unlock (as returned by @ref mutex_create).
 *
 */
extern void mutex_unlock(T_MUTEX mutex);

/**
 * Lock/take a mutex.
 *
 * This service may block while waiting on the mutex availability,
 * depending on the timeout parameter.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param mutex Handle of the mutex to lock (as returned by @ref mutex_create).
 *
 * @param timeout Maximum number of milliseconds to wait for the mutex.
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 *
 * @return Execution status:
 *      - E_OS_OK Mutex was successfully taken,
 *      - E_OS_ERR Mutex parameter is invalid (mutex was deleted or never created),
 *      - E_OS_ERR_TIMEOUT Could not take semaphore before timeout expiration,
 *      - E_OS_ERR_BUSY Could not take semaphore (did not wait),
 *      - E_OS_ERR_NOT_ALLOWED Service cannot be executed from ISR context.
 */
extern OS_ERR_TYPE mutex_lock(T_MUTEX mutex, int timeout);

/**
 * @}
 */

/**
 *
 * @defgroup os_rwlock Read/Write Lock
 * Control functions for read/write lock.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * A read/write lock is a synchronization object, used to protect memory access from
 * concurrent writes by multiple tasks and allow concurrent read by multiple tasks.
 *
 * A read/write lock uses 2 mutexes. Therefore the number of available read/write
 * locks is limited by the number of available mutexes.
 *
 * @warning Read/Write locks can't be used inside an ISR (interrupt context).
 *
 * Function name            | Task ctxt | Fiber ctxt| Interrupt |
 * -------------------------|:---------:|:---------:|:---------:|
 * @ref rwlock_init         |     X     |     X     |           |
 * @ref rwlock_delete       |     X     |     X     |           |
 * @ref rwlock_rdlock       |     X     |     X     |           |
 * @ref rwlock_rdunlock     |     X     |     X     |           |
 * @ref rwlock_wrlock       |     X     |     X     |           |
 * @ref rwlock_wrunlock     |     X     |     X     |           |
 *
 * @{
 */

/* Read/Write lock structure*/
struct rwlock_t {
	T_MUTEX rwlock_wrmtx;   /*!< Mutex to restrict simultaneous writes */
	T_MUTEX rwlock_rdmtx;   /*!< Mutex to restrict write/erase operation on flash while read happens, but allows simultaneous writes */
	uint8_t read_count;     /*!< Count for number of concurrent read locks */
};

/**
 * Initialize a Read/Write lock.
 *
 * Initialize a rwlock object.
 *
 * @warning This service may panic if:
 * - no mutex is available, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure to initialize
 *
 */
void rwlock_init(struct rwlock_t *rwlock);

/**
 * Delete a Read/Write lock.
 *
 * Disable the Read/Write lock mutexes that was reserved using @ref rwlock_init.
 * Deleting a rwlock while a task is waiting (or will wait) for it to be freed
 * may create a deadlock.
 *
 * @warning This service may panic if:
 * - rwlock parameter is invalid, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b> task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure
 *
 */
void rwlock_delete(struct rwlock_t *rwlock);

/**
 * Lock/take a read lock.
 *
 * This service may block while waiting on the mutex
 * availability, depending on the timeout parameter.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure
 *
 * @param timeout Maximum number of milliseconds to wait for the lock.
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 */
void rwlock_rdlock(struct rwlock_t *rwlock, int32_t timeout);

/**
 * Unlock/give a read lock.
 *
 * This service may block while waiting on the mutex availability.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure
 *
 */
void rwlock_rdunlock(struct rwlock_t *rwlock);

/**
 * Lock/take a write lock.
 *
 * This service may block while waiting on the mutex
 * availability, depending on the timeout parameter.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure
 *
 * @param timeout Maximum number of milliseconds to wait for the lock.
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 */
void rwlock_wrlock(struct rwlock_t *rwlock, int32_t timeout);

/**
 * Unlock/give a write lock.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param rwlock Pointer to the Read/Write lock structure
 *
 */
void rwlock_wrunlock(struct rwlock_t *rwlock);

/**
 * @}
 */

/**
 * @defgroup os_scheduling Scheduling
 * Control functions to enable or disable scheduling.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * The processor manages the scheduling between multiple tasks and interrupts,
 * based on their priorities.
 *
 * Temporarily disabling scheduling allows to perform critical sections of code
 * knowing that it won't be interrupted.
 *
 * Function name            | Task ctxt | Fiber ctxt| Interrupt |
 * -------------------------|:---------:|:---------:|:---------:|
 * @ref disable_scheduling  |     X     |     X     | No effect |
 * @ref enable_scheduling   |     X     |     X     | No effect |
 *
 * @{
 */

/**
 * Disable scheduling.
 *
 * Disable task preemption.
 *
 * @warning This service has no effect when called from an ISR context.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @note ZEPHYR SPECIFIC:<br/>
 * This service disables all interrupts.
 */
void disable_scheduling(void);

/**
 * Enable scheduling.
 *
 * Restore task preemption.
 *
 * @warning This service has no effect when called from an ISR context.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @note ZEPHYR SPECIFIC:<br/>
 * This service unmasks all interrupts that were masked by a previous
 * call to @ref disable_scheduling.
 */
void enable_scheduling(void);

/**
 * @}
 */

/**
 *
 * @defgroup os_queue Queue
 * Control functions for message queues.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * Message Queues allow the services to send/receive messages from other
 * services.
 *
 * The maximum number of mutexes is defined by \c QUEUE_POOL_SIZE.
 *
 * Function name                | Task ctxt | Fiber ctxt| Interrupt |
 * -----------------------------|:---------:|:---------:|:---------:|
 * @ref queue_delete            |     X     |     X     |           |
 * @ref queue_create            |     X     |     X     |           |
 * @ref queue_get_message       |     X     |     X     |           |
 * @ref queue_send_message      |     X     |     X     |     X     |
 * @ref queue_send_message_head |     X     |     X     |     X     |
 *
 * @{
 */

/** Maximum number of queues */
#define QUEUE_POOL_SIZE             (10)

/**
 * Create a message queue.
 *
 * @warning This service may panic if:
 * - no queue is available, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * The total number of queues that may be in use (created) at the same time
 * is \c QUEUE_POOL_SIZE.
 *
 * @param maxSize Maximum number of messages in the queue
 *                (Rationale: queues only contain pointer to messages).
 *
 * @return
 *    - Handle of the created queue,
 *    - NULL if all allocated queues are already being used.
 */
T_QUEUE queue_create(uint32_t maxSize);

/**
 * Delete a queue.
 *
 * Free a whole queue.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param queue Handle of the queue to free (as returned by @ref queue_create).
 *
 */
void queue_delete(T_QUEUE queue);

/**
 * Read a message from a queue.
 *
 * Read and dequeue a message.
 *
 * @warning This service may panic if err parameter is NULL and:
 * - queue parameter is invalid, or
 * - message parameter is NULL, or
 * - when called from an ISR.
 *
 * <b>Authorized execution levels:</b>  task, fiber.
 *
 * @param queue Handle of the queue (as returned by @ref queue_create).
 *
 * @param[out] message Pointer where to return the read message.
 *
 * @param timeout Maximum number of milliseconds to wait for the message.
 *                Special values OS_NO_WAIT and OS_WAIT_FOREVER may be used.
 *
 * @param[out] err  Execution status:
 *          - E_OS_OK  A message was read,
 *          - E_OS_ERR_TIMEOUT No message was received,
 *          - E_OS_ERR_EMPTY The queue is empty,
 *          - E_OS_ERR Invalid parameter,
 *          - E_OS_ERR_NOT_ALLOWED Service cannot be executed from ISR context.
 */
void queue_get_message(T_QUEUE queue, T_QUEUE_MESSAGE *message, int timeout,
		       OS_ERR_TYPE *err);

/**
 * Send a message to a queue.
 *
 * Send/queue a message at the end of a queue.
 *
 * @warning This service may panic if err parameter is NULL and:
 * - queue parameter is invalid, or
 * - the queue is already full.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param queue Handle of the queue (as returned by @ref queue_create).
 *
 * @param[in] message  Pointer to the message to send.
 *
 * @param[out] err   Execution status:
 *          - E_OS_OK  The message was sent,
 *          - E_OS_ERR_OVERFLOW The queue is full (message was not posted),
 *          - E_OS_ERR Invalid parameter.
 */
void queue_send_message(T_QUEUE queue, T_QUEUE_MESSAGE message,
			OS_ERR_TYPE *err);

/**
 * Send a message on a queue head.
 *
 * Send/queue a message at the head of the queue so that it will be
 * the next to be dequeued.
 *
 * @warning This service may panic if err parameter is NULL and:
 * - queue parameter is invalid, or
 * - the queue is already full.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param queue Handle of the queue (as returned by @ref queue_create).
 *
 * @param[in] message  Pointer to the message to send.
 *
 * @param[out] err   Execution status:
 *          - E_OS_OK  The message was sent,
 *          - E_OS_ERR_OVERFLOW The queue is full (message was not posted),
 *          - E_OS_ERR Invalid parameter.
 */
void queue_send_message_head(T_QUEUE queue, T_QUEUE_MESSAGE message,
			     OS_ERR_TYPE *err);

/**
 * @}
 */

/**
 *
 * @defgroup os_time_timer Time and Timers
 * Control functions to get system time, and manager timers.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * The time functions return the elapsed time in milli-seconds or micro-seconds.
 *
 * The timers are used to perform an action after a specific amount of time.
 *
 * The maximum number of timers is defined by \c TIMER_POOL_SIZE.
 *
 * Function name       | Task ctxt | Fiber ctxt| Interrupt |
 * --------------------|:---------:|:---------:|:---------:|
 * @ref get_time_ms    |     X     |     X     |     X     |
 * @ref get_time_us    |     X     |     X     |     X     |
 * @ref timer_create   |     X     |     X     |     X     |
 * @ref timer_start    |     X     |     X     |     X     |
 * @ref timer_stop     |     X     |     X     |     X     |
 * @ref timer_delete   |     X     |     X     |     X     |
 *
 * @{
 */

/**
 * Get the current time in ms.
 *
 * Return the current tick converted in milliseconds.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @return Current tick converted in milliseconds.
 */
uint32_t get_time_ms(void);

/**
 * Get the current time in us.
 *
 * Return the current tick converted in microseconds.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @return Current tick converted in microseconds.
 */
uint64_t get_time_us(void);

/**
 * Create a timer object.
 *
 * @warning This service may panic if err parameter is null and:
 * - callback parameter is null, or
 * - no timer is available.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * The total number of timers that may be in use (created) at the same time
 * is \c TIMER_POOL_SIZE.
 *
 * @param callback Pointer to the function to be executed on timer expiration.
 * @param privData Pointer to data that shall be passed to the callback.
 * @param delay    Number of milliseconds between function executions.
 * @param repeat   Specifies if the timer shall be re-started after each
 *                 execution of the callback.
 * @param startup  Specifies if the timer shall be start immediately.
 * @param[out] err Execution status:
 *        - E_OS_OK  Timer is programmed,
 *        - E_OS_ERR No timer is available, or callback parameter is NULL.
 *
 * @return
 *       - Handle of the timer,
 *       - NULL if the service fails (e.g. no available timer
 *         or callback is a NULL pointer).
 */
T_TIMER timer_create(T_ENTRY_POINT callback, void *privData, uint32_t delay,
		     bool repeat, bool startup,
		     OS_ERR_TYPE *err);

/**
 * Start a timer.
 *
 * @warning This service may panic if err parameter is null and:
 * - no timer is available.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param tmr  Handle of the timer (as returned by @ref timer_create).
 * @param delay Number of milliseconds between function executions.
 * @param[out] err  Execution status:
 *        - E_OS_OK  Timer is started,
 *        - E_OS_ERR tmr parameter is null, invalid, or timer is running.
 */
void timer_start(T_TIMER tmr, uint32_t delay, OS_ERR_TYPE *err);

/**
 * Stop a timer.
 *
 * This service may panic if:
 * - tmr parameter is null, invalid, or timer is not running.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param tmr Handle of the timer (as returned by @ref timer_create).
 *
 */
void timer_stop(T_TIMER tmr);

/**
 * Delete a timer object.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * @param tmr  Handle of the timer (as returned by @ref timer_create).
 *
 */
void timer_delete(T_TIMER tmr);


/**
 * @}
 */

/**
 *
 * @defgroup os_mem_alloc Memory Allocation
 * Control functions for dynamic memory allocation.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/util</tt>
 * <tr><th><b>Config flag</b> <td><tt>MEMORY_POOLS_BALLOC</tt>
 * </table>
 *
 * Dynamic allocation requires a function to allocate and free memory.
 * If allocated memory is not freed, it causes a memory leak.
 *
 * Buffer are allocated from static pools of memory. If an allocation fails,
 * the system may panic.
 *
 * See @ref memory_partitioning for volatile memory management and
 * memory pools allocation.
 *
 * Function name       | Task ctxt | Fiber ctxt| Interrupt |
 * --------------------|:---------:|:---------:|:---------:|
 * @ref balloc         |     X     |     X     |     X     |
 * @ref bfree          |     X     |     X     |     X     |
 *
 * @{
 */


/**
 * Reserve a block of memory.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * This function returns a pointer on the start of a reserved memory block
 * which size is equal or larger than the requested size.
 *
 * If there is not enough available memory, this function returns a null
 * pointer and sets \c err parameter to E_OS_ERR_NO_MEMORY, or panic
 * if \c err pointer is null.
 *
 * @warning This function may panic if err is null and
 * - size is null, or
 * - there is not enough available memory.
 *
 * @param size Number of bytes to reserve.
 *
 * @param[out] err execution status:
 *    - E_OS_OK Block was successfully reserved,
 *    - E_OS_ERR Size is null,
 *    - E_OS_ERR_NO_MEMORY There is not enough available memory.
 *
 * @return Pointer to the reserved memory block
 *    or null if no block is available.
 */
void *balloc(uint32_t size, OS_ERR_TYPE *err);

/**
 * Free a block of memory.
 *
 * <b>Authorized execution levels:</b>  task, fiber, ISR.
 *
 * This function frees a memory block that was reserved by @ref balloc.
 *
 * The \c buffer parameter must point to the start of the reserved block
 * (i.e. it shall be a pointer returned by @ref balloc).
 *
 * @param buffer Pointer returned by @ref balloc.
 *
 * @return Execution status:
 *    - E_OS_OK Block was successfully freed,
 *    - E_OS_ERR buffer parameter did not match any reserved block.
 */
OS_ERR_TYPE bfree(void *buffer);


/**
 * @}
 */

/**
 *
 * @defgroup os_task Task delay
 * Control functions to manage delays in tasks.
 *
 * <table>
 * <tr><th><b>Include file</b><td><tt> \#include "os/os.h"</tt>
 * <tr><th><b>Source path</b> <td><tt>bsp/src/os/zephyr</tt>
 * </table>
 *
 * Not all the functions can be called from interrupt context. Here is a
 * summary of the compatible contexts for each function.
 *
 * Function name              | Task ctxt | Fiber ctxt| Interrupt |
 * ---------------------------|:---------:|:---------:|:---------:|
 * @ref local_task_sleep_ticks|     X     |     X     |           |
 * @ref local_task_sleep_ms   |     X     |     X     |           |
 *
 * @{
 */


/**
 * Release the execution of the current task for a limited time.
 *
 * @param time Number of milliseconds to sleep.
 */
void local_task_sleep_ms(int time);

/**
 * Release the execution of the current task for a limited time.
 *
 * @param ticks Number of ticks to sleep.
 */
void local_task_sleep_ticks(int ticks);

/**
 * @}
 */

/**
 * Initializes the OS abstraction layer.
 *
 * This function must be called before using any facility provided
 * by the OS abstraction layer.
 */
void os_init(void);

/**
 * @}
 */

#endif
