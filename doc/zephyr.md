@page zephyr_os Zephyr OS
@{

Main OS is based on Zephyr.
- Quark is running multi-threaded microkernel.
- ARC is running single-threaded nanokernel.

For more information on Zephyr see https://www.zephyrproject.org/.

Curie&trade; BSP provides an @ref os that offers a common API to handle many objects regardless of the low-level OS.

## Execution contexts

### Tasks

Tasks are preemptible execution contexts, they are threads with a given priority.
Whenever a task with higher priority than current is ready, it will preempt the
current task.
- The microkernel allows multiple tasks creation.
- The nanokernel has only one task that calls the main() function.

### Fiber

A fiber is like a thread that is not preemptible. Its execution will continue
until it releases the CPU by sleeping, waiting on a semaphore / mutex.
Only interrupts can interrupt a fiber. All fibers are of higher priority than
the task.

### Interrupt

#### Signaling mechanisms
An interrupt is executed in a specific context, no processing should be done
inside the interrupt function.

The generic behavior of an interrupt is either:
- _to signal a semaphore_ to wakeup a sleeping fiber / task if there is no data to
pass to the non-interrupt context;
- or _to allocate and send a message_ to the non-interrupt context when there is
data to pass.

#### Driver callbacks

Driver implementation generally allows callback functions to be defined.

These callbacks are _always called in the interrupt context_.
A driver user should only use the mechanisms described above to signal the
application (or service) of the event that occurred.

#### Limited resources
As the system resources are small, the number of tasks / fibers should be limited to the minimum.

If an application needs to implement heavy processing of data, then it could
make sense to create a dedicated task for this processing with a low priority
so it runs when no other service / applications needs the CPU.

@}
