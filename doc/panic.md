@addtogroup infra_panic
@{

### Preamble: a Panic is not an Assert

<tt>assert(int expression)</tt> is to capture programming errors during development phase.
It is disabled as soon as NDEBUG is defined and is consequently only enabled for
debug builds (see [BUILDVARIANT](@ref curie_build_project)).
It is widespread all over the code.

panic(int error) is to reset the device in case of non-recoverable errors,
dumping minimal information for reporting/debugging.
It is enabled for both debug and release builds and is sparingly used.


### Calling a Panic

    void panic (int error);

Error code definition is developer's responsibility. Is it local to a function.
Recommendation is to use an enum.
Panic post processing uses "PC+error code" combo as an unique identifier (panic
ID) to ease correlation from build to build:
- PC post processing is providing the function name, the file name and the line
  number,
- Line number varying from build to build, error code is used.

Example:

Release 1 code only deals with ERROR_1.

\code
typedef enum my_errors {
    ERROR_1,
    ERROR_MAX
} my_errors_t;

int my_function(param_t *param)
{
    int ret = 0;

    switch (param->id) {
        case PARAM_ID_1:
            assert(param->payload == NULL);
            break;
        case PARAM_ID_2:
        case PARAM_ID_3:
        case PARAM_ID_4:
            panic(ERROR_1);
            break;
        }
    }
    return ret;
}
\endcode

Release 2 code is adding ERROR_2 but "function name+ERROR_1" remains an
unchanged identifier.

\code
typedef enum my_errors {
    ERROR_1,
    ERROR_2,
    ERROR_MAX
} my_errors_t;

int my_function(param_t *param)
{
    int ret = 0;

    switch (param->id) {
        case PARAM_ID_1:
            assert(param->payload == NULL);
            break;
        case PARAM_ID_2:
            panic(ERROR_2);
            break;
        case PARAM_ID_3:
        case PARAM_ID_4:
            panic(ERROR_1);
            break;
        }
    }
    return ret;
}
\endcode

### Panic mechanism

Processing is kept minimal during panic handling.

Platform reset is done as soon as possible.

Once reset is complete, additional processing is done.

@if PANIC_INTERNAL
@msc
 Master, Slave;
 Master=>Master    [label="panic(error)", URL="\ref panic"];
 Master box Master [label="Dump panic informations into RAM"];
 Master=>Slave     [label="panic_notify(timeout)", URL="\ref panic_notify"];
 ---               [label="Case 1: Slave is alive"];
 Slave=>Slave      [label="handle_panic_notification(core_id)", URL="\ref handle_panic_notification"];
 Slave=>Master     [label="panic_done", URL="\ref panic_done"];
 ---               [label="Case 2: timeout expires before Slave handshake"];
 Master=>Master    [label="reboot()"];
 ...               [label=""];
 Master box Master [label="Boot"];
 Master box Master [label="Copy panic informations from RAM into NVM"];
@endmsc
@endif

### Understanding a panic dump
@anchor panic_dump Understanding a panic dump

A panic dump basically consists in information about both QRK and ARC cores.

Below is the sample dump from QRK and ARC with the basic explanation.

`PANIC for 0 (dump located on 0xa800e000)`
- Panic dump for core 0 that is QRK and the address of the dump in the RAM

`QRK BUILD CKSUM: 1b795130`
- Checksum of the build, same as the one given by ‘version get’ testcommand.
  This value is unique for each binary.

`QRK BUILD VERSION: 00000001`

`QRK PANIC (256, 0x00000001) - EIP=0x40020b32 - ESP=0xa80080ec`
- The first value corresponds to the interrupt vector.

- We use mainly 3 values:
  - 2 is watchdog.
  - 23 is invalid access on memory (ie segfault).
  - 256 is a SW panic (the code reached a call to ‘panic(…)’).
  - Other value if any would be a predefined architecture exception.

- The second value is the error code, for a panic 256, it corresponds to
  the argument of ‘panic(ERROR_CODE)’. Panic with value 0x01 is most of
  the time a propagated panic from ARC.

`- EIP=0x4001ed30 - ESP=0xa800acc0`
  - Instruction pointer and stack pointer. Basically where your code
    reached the panic.

<tt>
Registers:

    cr2:   00000000
    …
    ss:    00000010

Stack:

    0xa800acc0: 4001ed30 ffffffff a800acd8 4001ee80
    0xa800acd0: 00000000 a8006f28 a800acf4 40010eb4
    …
    0xa800ad50: 0000001d 00000000 00000000 a800ad70
</tt>

- Content of first elements of the stack. All value beginning with `400[1|2]XXXXX`
  is likely a valid address of a function on QRK side followed by the parameters
  to the function. By decoding these addresses you can get the callstack.

`PANIC for 1 (dump located on 0xa8014000)`
- Panic dump for core 1 that is ARC and the address of the dump in the RAM

`ARC BUILD CKSUM: 2ae2cd81`

`ARC BUILD VERSION: 00000001`

`ARC PANIC - eret=0x4004813a - sp=0xa8013c3c`
- eret is the return address of the function.

<tt>
Registers:

    r0:00000001  r1:00000015  r2:00000000  r3:00000000
    …
    sp:a8011a8c  il:4003bd28 r30:00000000  bl:40038af6

Status registers:

         ecr:000d0000
         efa:00000002
        eret:4004813a
    erstatus:0001101e
</tt>

- These registers provide a big hint to know if the Quark or the ARC did panic first.
- If ecr or errstatus is not null, the panic likely comes from the ARC side.

<tt>Stack:

    0xa8011a8c: a8011a94 40038d22 a8011aa4 a800e6ec
    0xa8011a9c: ffffffff 40038150 a8011e3c 4003c07e
    …
    0xa8011b1c: dfc75d4b a9502604 04452120 4004e1ef
</tt>

- Content of first elements of the stack. All value beginning with `400[3|4]XXXXX`
  is likely a valid address of a function on ARC side followed by the parameters
  to the function. By decoding these addresses you can get the callstack.

@}
