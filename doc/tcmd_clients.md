@addtogroup infra_tcmd
@{

## Test commands console backends

The SDK includes several test command console backends based on the generic console client.

### UART test commands backend

The UART test command backend relies on the Zephyr OS UART driver API.

It can be activated using the `CONFIG_TCMD_CONSOLE_UART` configuration flag.

### USB ACM test commands backend

The USB ACM test command backend relies on the generic SDK USB ACM driver api.

It can be activated using the `CONFIG_TCMD_CONSOLE_USB_ACM` configuration flag.

@}
