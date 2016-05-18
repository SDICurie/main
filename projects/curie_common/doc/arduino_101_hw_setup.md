Hardware Setup {#arduino_101_hw_setup}
========

## Required Hardware

**Arduino 101 board** and its USB type B cable.
![Arduino_101](Arduino_101.jpg)

**UART to USB FTDI cable**

This cable is required to get logs from the platform.
They are available at:
- https://www.adafruit.com/products/954
- http://www.ftdichip.com/Products/Cables/USBTTLSerial.htm

**Flyswatter2 JTAG probe** is necessary to:
- backup the Arduino Firmware and recover it;
- debug the code using gdb;
- flash the bootloader.

It is available at http://www.tincantools.com/JTAG/Flyswatter2.html.

![Flyswatter2](Flyswatter2.jpg)

## Connection

Board has to be connected as shown in the following pictures.

**Connect the Flyswatter2**

Connect the JTAG probe as shown on the picture below:

![Flyswatter2 Connection](Arduino_101_JTAG.jpg)

**Connect the UART to USB converter**

Connect the UART to USB converter as shown on the picture below:

![UART Connection](Arduino_101_UART.jpg)

Plug the TX wire of the cable to the RX pin of the Arduino 101 header and
the RX wire to the TX pin. Do not forget to plug the GND wire to the GND pin
of the Arduino 101 header.

