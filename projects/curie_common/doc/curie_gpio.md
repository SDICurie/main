@addtogroup common_driver_gpio
@{
@warning On Quark_se platform, using the gpio as interrupt mode may not work
as expected with deepsleep activated: when in deepsleep, the processor is
shut-down, so IRQ on GPIOs are not caught.
Prefer use the @ref soc_comparator.
@}
