@addtogroup test_command_syntax
@{

### Test Commands list

**Manufacturing Test Commands list:**

These commands are available on all builds (release & debug).

Group                       | Command               |
----------------------------|:----------------------|
@ref factory                | get, end, lock        |
@ref ble                    | info, version, discover, write, read, read_multiple, connect, disconnect, subscribe, unsubscribe, scan, tx_test, rx_test |
[arc.]@ref spi              | tx, rx, trx           |
[arc.]@ref gpio             | conf, get, set        |
@ref system                 | idle, slpstat, power_off, shutdown, reset, dump_evt |
[arc.]@ref i2c              | write, probe, read, rx |
[arc.]@ref adc              | get                   |
[arc.]@ref mem              | read, write           |
[arc.]@ref log              | set                   |
@ref log                    | setbackend            |
@ref led                    | on, off               |
@ref battery                | level, status, vbatt, temperature |
@ref charger                | status, type          |
[arc.]@ref version          | get                   |
@ref nfc                    | reset, pwr_up, pwr_down, cmd |

**Engineering Test Commands list:**

These commands are available on all debug builds only.

Group                       | Command               |
----------------------------|:----------------------|
@ref rtc                    | set, read, alarm, status |
@ref pwm                    | conf, start, stop     |
[arc.]\ref log_d "log"      | list, print           |
[arc.]@ref debug            | panic                 |
@ref ss                     | startsc, stopsc, sbc, unsbc, clb, setprop, getprop |
@ref ble_d "ble"            | enable, test_set_tx_pwr, tx_carrier, set_name, key, clear, rssi, dbg, add_service, security |
@ref cfw_d                  | inject, poll          |
@ref tcmd                   | slaves, version       |
@ref dbg                    | pool                  |
@ref battery_d "battery"    | cycle, period         |
@ref nfc_d "nfc"            | svc, fsm              |
@ref property               | read                  |
@ref ui                     | led_blink_x1, led_blink_x2, led_blink_x3, led_wave_x1, led_wave_x2, vibr |
@ref adc_d "adc"            | comp                  |

### Manufacturing Test Commands detail

@anchor factory
**Factory:**

~~~~~~~~
factory get
~~~~~~~~
Display factory data

~~~~~~~~
factory end <mode>
~~~~~~~~
Set factory end of production mode:
   - mode: oem|customer

~~~~~~~~
factory lock <uuid>
~~~~~~~~
Disable jtag port and rom write access:
   - uuid: uuid must be equal to the one written on rom

@anchor ble
**BLE:**

~~~~~~~~
ble info [type]
~~~~~~~~
Get BLE related information.
   - type if not specified 0 is assumed
        - 0     - BDA and current name
        - 1     - Number of bonded device

Example: ble info 1
        - 1 return number of bonded devices

~~~~~~~~
ble version
~~~~~~~~
Get BLE controller version.

~~~~~~~~
ble discover <conn_ref> <type> <uuid> [start_handle] [end_handle]
~~~~~~~~
Central mode only.
Discover the attributes on a connected device:
   - conn_ref - connection reference
   - type     - type of the discovery (0 = primary, 1 = secondary, 2 = included, 3 = characteristic, 4 = descriptor)
   - uuid     - 16-bit or 128-bit UUID to discover
   - start_handle - optional start attribute handle for the discovery
   - end_handle - optional end attribute handle for the discovery

Example: ble discover 0xa0004000 0 0x180f
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 0 is to retrieve primary service
   - 0x180f is battery service UUID

Example: ble discover 0xa0004002 0 1000c00f-fed9-4766-b18f-dead0d24beef
   - 0xa0004002 is the connection reference displayed in logs when remote device connected
   - 0 is to retrieve primary service
   - 1000c00f-fed9-4766-b18f-dead0d24beef is vendor 128-bit UUID of primary remote service

~~~~~~~~
ble write <conn_ref> <withResponse> <handle> <offset> <value>
~~~~~~~~
Central mode only.
Write an attribute on a connected device:
   - conn_ref       - connection reference
   - withResponse   - 1 if response needed, otherwise 0
   - handle         - handle of the attribute to write
   - offset         - offset at which to write
   - value          - ascii hex encoded string

Example: ble write 0xa0004000 1 14 0 43757269652054657374207772206e72202330
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 1 for response needed
   - 14 is characteristic handle
   - 0 is offset at which to write
   - 43757269652054657374207772206e72202330 is value to write

~~~~~~~~
ble read <conn_ref> <handle> <offset>
~~~~~~~~
Central mode only.
Read an attribute on a connected device:
   - conn_ref - connection reference
   - handle   - handle of the attribute to read
   - offset   - offset at which to read

Example: ble read 0xa0004000 14 0
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 14 is characteristic handle
   - 0 is the offset

~~~~~~~~
ble read_multiple <conn_ref> <handle_count> <handle_list>
~~~~~~~~
Central mode only.
Read multiple attributes on a connected device:
   - conn_ref     - connection reference
   - handle_count - count of attribute handle to read
   - handle_list  - list of attribute handle to read

Example: ble read_multiple 0xa0004000 3 4 7 16
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 3 is count of attribute handle to read
   - 4 7 16 is the list of attribute handle to read

~~~~~~~~
ble connect <address>/<type>
~~~~~~~~
Central mode only.
Initiate a connection as central to a remote peripheral:
   - address - Address (format = AA:BB:CC:DD:EE:FF)
   - type    - Address type (0 = public, 1 = random)

Example: ble connect AA:BB:CC:DD:EE:FF/0

~~~~~~~~
ble disconnect <conn_ref>
~~~~~~~~
Central mode only.
Disconnect from a connected device:
   - conn_ref - connection reference

~~~~~~~~
ble subscribe <conn_ref> <ccc_handle> <value> <value_handle>
~~~~~~~~
Central mode only.
Subscribe to a characteristic on a connected device:
   - conn_ref     - connection reference
   - ccc_handle   - handle of the Client Characteritic Configuration attribute
   - value        - value to write
   - value_handle - handle of the Characteritic Value attribute

Example: ble subscribe 0xa0004000 3 1 2

~~~~~~~~
ble unsubscribe <conn_ref> <subscribe_ref>
~~~~~~~~
Central mode only.
Unsubscribe from a characteristic on a connected device:
   - conn_ref      - connection reference
   - subscribe_ref - subscription reference returned at subscribe

Example: ble unsubscribe 0xa0004000 0xa0001209

~~~~~~~~
ble notify <conn_ref> <service_uuid> <value>
~~~~~~~~
Central mode only.
Notify the main characteristic value of a service:
   - conn_ref     - connection reference. If 0, will broadcast to all
   - service_uuid - uuid of the service that will notify its main characteristic
   - value        - ascii hex encoded string of notified value

Example: ble notify 0xa0004000 0x180f 6F
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 0x180f is the BAS service UUID value
   - 6F is the value to notify, here a 1 byte string containing ASCII char 0x6F

~~~~~~~~
ble indicate <conn_ref> <service_uuid> <value>
~~~~~~~~
Central mode only.
Indicate the main characteristic value of a service:
   - conn_ref     - connection reference.  If 0, will broadcast to all
   - service_uuid - uuid of the service that will indicate its main characteristic
   - value        - ascii hex encoded string of indicated value

Example: ble indicate 0xa0004000 0x180f A4
   - 0xa0004000 is the connection reference displayed in logs when remote device connected
   - 0x180f is the BAS service UUID value
   - A4 is the value to indicate, here a 1 byte string containing ASCII char 0xA4

~~~~~~~~
ble update <service_uuid> <values ... >
~~~~~~~~
Central mode only.
Call the update function of a service with the parameters:
   - service_uuid - uuid of the service that will be updated
   - values       - ascii hex encoded string of value

Example: ble indicate 0x180D 7C
   - 0x180D is the HRS service UUID value
   - 7C is the updated value to add

~~~~~~~~
ble scan <op> <type> <interval> <window>
~~~~~~~~
Central mode only.
Start central scan:
   - op         - start (parameters required) or stop (no parameters)
   - type       - scan type (passive:0 or active: 1)
   - interval   - scan interval (N * 0.625ms)
   - window     - scan window (N * 0.625ms)

Examples:
   - ble scan start 1 0x0060 0x0030 -- active scan, fast interval (60ms), fast window (30ms)
   - ble scan start 0 0x0800 0x0012 -- passive scan, slow interval (1.28s), slow window (11.25ms)
   - ble scan stop

~~~~~~~~
ble tx_test <operation> <freq> <len> <pattern>
~~~~~~~~
Start BLE dtm tx:
   - operation: start|stop
   - freq: actual frequency = 2024 + `<freq>` x 2Mhz, `<freq>` < 40
   - len : Payload length
   - pattern: Packet type
   - Example 1: ble tx_test start 20 4 0
       pattern = 0 - PRBS9, 1 - 0X0F, 2 - 0X55, 0xFFFFFFFF - Vendor Specific
   - Example 2: ble tx_test stop

~~~~~~~~
ble rx_test <operation> <freq>
~~~~~~~~
Start BLE dtm rx:
   - operation: start|stop
   - freq: 2024 + freq x 2Mhz, freq < 40
   - Example 1: ble rx_test start 20
   - Example 2: ble rx_test stop


@anchor spi
**SPI:**

~~~~~~~~
[arc].spi tx <bus_id> <slave_addr> <byte0> [<byte1>... <byte_n>]
~~~~~~~~
Send data on a SPI port:
   - bus_id: spim0|spim1|spis0|ss_spim0|ss_spim1 (see @ref SBA_BUSID)
   - slave_addr: Chip select (see @ref SPI_SLAVE_ENABLE)
   - byte0 [byte_n]: Bytes to send

~~~~~~~~
[arc].spi rx <bus_id> <slave_addr> <len>
~~~~~~~~
Receive data from a SPI port:
   - bus_id: spim0|spim1|spis0|ss_spim0|ss_spim1 (see @ref SBA_BUSID)
   - slave_addr: Chip select (see @ref SPI_SLAVE_ENABLE)
   - len: Length of the read buffer

~~~~~~~~
[arc].spi trx <bus_id> <slave_addr> [<byte0> <byte1>... <byte_n>] <len_to_read>
~~~~~~~~
Send data on a SPI port:
   - bus_id: spim0|spim1|spis0|ss_spim0|ss_spim1 (see @ref SBA_BUSID)
   - slave_addr: Chip select (see @ref SPI_SLAVE_ENABLE)
   - byte0 [byte_n]: Bytes to send
   - len_to_read: Length of the read buffer

@anchor gpio
**GPIO:**

~~~~~~~~
[arc.]gpio conf <soc|aon|8b0|8b1> <index> <mode>
~~~~~~~~
Configure SOC/AON port GPIO:
   - soc|aon|8b0|8b1: Port to config
   - index: Index of the gpio
   - mode: in|out

~~~~~~~~
[arc.]gpio get <soc|aon|8b0|8b1> <index>
~~~~~~~~
Get SOC/AON port GPIO:
   - soc|aon|8b0|8b1: Port to get
   - index: Index of the gpio

~~~~~~~~
[arc.]gpio set <soc|aon|8b0|8b1> <index> <val>
~~~~~~~~
Set SOC/AON port GPIO:
   - soc|aon|8b0|8b1: Port to set
   - index: Index of the gpio
   - val: 0|1 value to be set

@anchor system
**Power Management:**

~~~~~~~~
system idle <enable>
~~~~~~~~
Enable or disable deepsleep when system is idle
   - enable : 0:disabled|1:enabled

~~~~~~~~
system slpstat
~~~~~~~~
Get statistics on sleep time:
   - time: total time spent in deepsleep (in ms)
   - count: number of times the board was in deepsleep
   - ratio: ratio of total time spent in deepsleep (/1000)
   - sincelast: ratio of time spent in deepsleep since the last "system slpstat" command (/1000)

~~~~~~~~
system power_off
~~~~~~~~
Power off the board

~~~~~~~~
system shutdown <shutdown_mode>
~~~~~~~~
Shutdown the board:
   - shutdown_mode:
     - 0 - SHUTDOWN
     - 1 - CRITICAL_SHUTDOWN
     - 2 - THERMAL_SHUTDOWN
     - 3 - BATTERY_SHUTDOWN
     - 4 - REBOOT

~~~~~~~~
system reset <reboot_mode>
~~~~~~~~
Reboot the board:
   - reboot_mode:
     - 0 - MAIN
     - 1 - CHARGING
     - 2 - WIRELESS_CHARGING
     - 3 - RECOVERY
     - 4 - FLASHING
     - 5 - FACTORY
     - 6 - OTA
     - 7 - DTM
     - 8 - CERTIFICATION
     - 9 - RESERVED_0
     - 10 - APP_1
     - 11 - APP_2
     - 12 - RESERVED_1
     - 13 - RESERVED_2
     - 14 - RESERVED_3
     - 15 - RESERVED_4

~~~~~~~~
system dump_evt
~~~~~~~~
Dump all system events on the console

@anchor i2c
**I2C:**

~~~~~~~~
[arc.]i2c write <bus_id> <slave_addr> <register> [value1... valueN]
~~~~~~~~
Start tx:
   - bus_id: i2c0|i2c1|ss_i2c0|ss_i2c1
   - slave_addr: Address of the slave
   - register: Slave register number
   - value1> [value2... valueN]: Value to write

~~~~~~~~
[arc.]i2c probe <bus_id> <slave_addr>
~~~~~~~~
Probe a device:
   - bus_id: i2c0|i2c1|ss_i2c0|ss_i2c1 (see @ref SBA_BUSID)
   - slave_addr: Address of the slave

~~~~~~~~
[arc.]i2c read <bus_id> <slave_addr> <register> <len>
~~~~~~~~
Read data:
   - bus_id: i2c0|i2c1|ss_i2c0|ss_i2c1 (see @ref SBA_BUSID)
   - slave_addr: I2C address if configured as a Slave
   - register: Slave register number
   - len: Length of the read buffer

~~~~~~~~
[arc.]i2c rx <bus_id> <slave_addr> <len>
~~~~~~~~
RX data:
   - bus_id: i2c0|i2c1|ss_i2c0|ss_i2c1 (see @ref SBA_BUSID)
   - slave_addr: I2C address if configured as a Slave
   - len: Length of the read buffer

@anchor adc
**ADC:**

~~~~~~~~
[arc.]adc get <channel>
~~~~~~~~
Get the channel value:
   - channel: ADC channel

@anchor mem
**Memory:**

~~~~~~~~
[arc.]mem read <size> <addr>
~~~~~~~~
Read on a given address:
   - size: Size to read (in byte : 1|2|4)
   - addr: Address to read (in hexadecimal)
   - Example: mem read 4 0xA8000010 - Read the 4 bytes at 0xA8000010 address

~~~~~~~~
[arc.]mem write <size> <addr> <val>
~~~~~~~~
Write given value to a given address:
   - size: Size to write (in byte : 1|2|4)
   - addr: Address to write (in hexadecimal)
   - val: Value to write
   - Example: mem write 4 0xA8000010 0x51EEb001 - Write 0x51EEb001 at 0xA8000010
     address

@anchor log
**Logger:**

~~~~~~~~
[arc.]log set <level> or log set <module>
~~~~~~~~
Set an action:
   - module: 0, 1, 2, ...
   - Example 1: log set 1 1 - enable module 1
   - Example 2: log set 2 0 - disable module 2
   - Example 3: log set 1 - set log level to WARNING

~~~~~~~~
log setbackend [none|uart*|usb]
~~~~~~~~
Gets current log backend set and sets logging to specified backend
   - Example 1: log setbackend - gets the current backend set
   - Example 2: log setbackend usb - sets log backend to usb

@anchor led
**Led:**

~~~~~~~~
led on [<index>] [<color>] [duration]
~~~~~~~~
Turn rgb led on:
   - index: led index [default: 0]
   - color: 'r' or 'g' or 'b' [default: 'r']
   - duration: turn led off after specified seconds [default: always on]

~~~~~~~~
led off [<index>]
~~~~~~~~
Turn rgb led off:
   - index: led index [default: 0]

@anchor battery
**Battery:**

~~~~~~~~
battery level
~~~~~~~~
Get battery level

~~~~~~~~
battery status
~~~~~~~~
Get battery status

~~~~~~~~
battery vbatt
~~~~~~~~
Get battery voltage

~~~~~~~~
battery temperature
~~~~~~~~
Get battery temperature

@anchor charger
~~~~~~~~
charger status
~~~~~~~~
Get charger status

~~~~~~~~
charger type
~~~~~~~~
Get charger type

@anchor version
**Version:**

~~~~~~~~
[arc.]version get
~~~~~~~~
Get the binary version header that allows to uniquely identify the binary used

@anchor nfc
**NFC:**

~~~~~~~~
nfc reset
~~~~~~~~
Reset the NFC controller

~~~~~~~~
nfc pwr_up
~~~~~~~~
Power up the NFC controller

~~~~~~~~
nfc pwr_down
~~~~~~~~
Power down the NFC controller

~~~~~~~~
nfc cmd <reset|init>
~~~~~~~~
Send basic commands to the NFC controller
   - The supported commands are 'reset' and 'init'

### Engineering Test Commands Detail

@anchor rtc
**RTC:**

~~~~~~~~
rtc set <time>
~~~~~~~~
Set initial RTC time:
   - time: Configuration value for the 32bit RTC value (timestamp)

~~~~~~~~
rtc read
~~~~~~~~
Read current RTC

~~~~~~~~
rtc alarm <rtc_alarm_time>
~~~~~~~~
Set RTC Alarm time, and start RTC:
   - rtc_alarm_time: Configuration value for the 32bit RTC alarm value (in s)

~~~~~~~~
rtc status
~~~~~~~~
Get status of RTC(not config, running, finished)

@anchor pwm
**Pulse Width Modulation:**

~~~~~~~~
pwm conf <channel> <frequency> <unit> <duty_cycle>
~~~~~~~~
Configure a channel of PWM:
   - channel: PWM channel number [0-3]
   - frequency: Frequency
   - unit: mHz|Hz|kHz
   - duty_cycle: PWM duty cycle in nanoseconds (time high)

~~~~~~~~
pwm start <channel>
~~~~~~~~
Start a channel of PWM:
   - channel: PWM channel number

~~~~~~~~
pwm stop <channel>
~~~~~~~~
Stop a channel of PWM:
   - channel: PWM channel number

@anchor log_d
**Logger:**

~~~~~~~~
[arc.]log list
~~~~~~~~
List modules

~~~~~~~~
[arc.]log print <n> <string_message>
~~~~~~~~
Print message :
   - n: number of messages
   - string_message: message to print

@anchor debug
**Panic:**

~~~~~~~~
[arc.]debug panic <panic_id>
~~~~~~~~
Panic generator:
   - panic_id:
     - 0 - Division by 0
     - 1 - Unaligned access
     - 2 - Watchdog panic (timeout 2,097s)
     - 3 - Invalid adress
     - 4 - User panic
     - 5 - Stack Overflow
     - 6 - Invalid OpCode

@anchor ss
**Sensors:**

~~~~~~~~
ss startsc <sensor_type>
~~~~~~~~
Start scanning a sensor:
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE

~~~~~~~~
ss stopsc <sensor_type>
~~~~~~~~
Stop scanning a sensor
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE

~~~~~~~~
ss sbc <sensor_type> <sampling_freq> <reporting_interval>
~~~~~~~~
Set the parameter of sensor subscribing data:
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE
   - sampling_freq: Samling frequency
   - reporting_interval: Reporting interval

~~~~~~~~
ss unsbc <sensor_type>
~~~~~~~~
Unset the parameter of sensor subscribing data:
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE

~~~~~~~~
ss clb <clb_cmd> <sensor_type> <calibration_type> <data>
~~~~~~~~
Operation the calibration of sensors:
   - clb_cmd: start/get/stop/set
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE
   - calibration_type: specific type of different sensors,it is 0 for On_board sensors
   - data: data for clb_cmd=set

~~~~~~~~
ss setprop <sensor_type> <data>
~~~~~~~~
Set a sensor property
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE
   - data : data for the sensor property

~~~~~~~~
ss getprop <sensor_type>
~~~~~~~~
Get a sensor property
   - sensor_type: GESTURE|TAPPING|SIMPLEGES|STEPCOUNTER|ACTIVITY|ACCELEROMETER|GYROSCOPE

@anchor ble_d
**BLE:**

~~~~~~~~
ble enable <enable_flag> [name]
~~~~~~~~
Enable BLE:
   - enable_flag: 0 - disable; 1 - enable;
   - [name]:        Local Name of the Local BLE device
   - Example 1: ble enable 1 App - enables BLE with name = "App" in
     normal mode
   - Example 2: ble enable 1 - enables BLE in normal mode
   - Example 3: ble enable 0 - disables BLE

~~~~~~~~
ble test_set_tx_pwr <dbm>
~~~~~~~~
Set BLE tx power:
   - dbm: -60dBm...+4dBm
   - Example: BLE test_set_tx_pwr -16

~~~~~~~~
ble tx_carrier <operation> <freq>
~~~~~~~~
Start tx carrier test:
   - operation: start|stop
   - freq: actual frequency = 2024 + `<freq>` x 2Mhz, `<freq>` < 40
   - Example 1: ble tx_carrier start 10
   - Example 2: ble tx_carrier stop

~~~~~~~~
ble set_name <name>
~~~~~~~~
Set BLE device name:
   - Example: ble set_name Test
   - the name will be used as BLE name for the device
   - the name set with this command is persistent after reboot

~~~~~~~~
ble key <conn_ref> <action> <pass_key>
~~~~~~~~
Return a passkey if required by the bonding:
   - conn_ref - connection reference
   - action :
     0 No key (may be used to reject).
     1 Security data is a 6-digit passkey.
   - pass_key - six digits key to return

~~~~~~~~
ble security <conn_ref> <sec_level>
~~~~~~~~
Set security level on existing connection and encrypt connection:
   - conn_ref - connection reference
   - sec_level - 0: low (NO security), 1: medium (Just Works) 2: high

Example: ble security 1
This will set medium security level: Just works pairing will used if not paired and the link will be encrypted.

~~~~~~~~
ble clear
~~~~~~~~
Clear the BLE bonding information

~~~~~~~~
ble rssi <start|stop> [<delta_dBm> <min_count>]
~~~~~~~~
Start BLE rssi reporting:
   - Example: ble rssi start 5 3
   - The default option is delta_dBm 5 and min_count 3

~~~~~~~~
ble dbg <u0> <u1>
~~~~~~~~
BLE debug command to test BLE service
   - u0, u1: dummy values
   - Example: ble dbg 5 6 - prints "ble dbg: 6/0x6 7/7"

~~~~~~~~
ble add_service <uuid>
~~~~~~~~
Add BLE service
   - uuid: (DIS) 180a, (BAS) 180f
   - Example: ble add_service 180F

@anchor cfw_d
**CFW Message Injection:**

~~~~~~~~
cfw inject <message>
~~~~~~~~
Send a cfw message:
   - message: Message to send (in hexadecimal)

~~~~~~~~
cwf poll
~~~~~~~~
Read a cfw message

@anchor tcmd
**Slaves:**

~~~~~~~~
tcmd slaves
~~~~~~~~
List registered slaves

**Version:**

~~~~~~~~
tcmd version
~~~~~~~~
Get the tcmd 'engine' version

@anchor dbg
**Memory:**

~~~~~~~~
dbg pool
~~~~~~~~
Get statistics on memory pool usage

@anchor battery_d
**Battery:**

~~~~~~~~
battery cycle
~~~~~~~~
Get battery charge cycle

~~~~~~~~
battery period vbatt|temp <period_second>
~~~~~~~~
Set ADC measure interval:
   - period_second: If is equal to 0, corresponding measure is suspended

@anchor nfc_d
**NFC:**

~~~~~~~~
nfc svc <enable|disable|start|stop>
~~~~~~~~
Configure the NFC service

~~~~~~~~
nfc fsm <enable|disable|start|stop|scn_all_done|fsm_auto_test>
~~~~~~~~
Post an event to the NFC FSM queue

@anchor property
**Properties:**

~~~~~~~~
property read <service_id> <property_id>
~~~~~~~~
Print the properties of a specific service:
   - service_id: id of the service
   - property_id: id of the property

@anchor ui
**UI:**

~~~~~~~~
ui led_blink_x1 <led_number> <intensity> <repetition_count> <color_red> <color_green> <color_blue> <duration[0]_on> <duration[0]_off>
~~~~~~~~
Play led pattern "blink x1":
   - led_number: id of the led
   - intensity: led intensity (0-255)
   - repetition_count: 0 - play led pattern only once, 1 to 255 - led pattern is repeated x times
   - color_red, color_green, color_blue: led color (0-255)
   - duration[0]_on: time in ms with the led on (0-65535)
   - duration[0]_off: time in ms with the led off (0-65535)

~~~~~~~~
ui led_blink_x2 <led_number> <intensity> <repetition_count> <color_red> <color_green> <color_blue> <duration[0]_on> <duration[0]_off> <duration[1]_on> <duration[1]_off>
~~~~~~~~
Play led pattern "blink x2":
   - led_number: id of the led
   - intensity: led intensity (0-255)
   - repetition_count: 0 - play led pattern only once, 1 to 255 - led pattern is repeated x times
   - color_red, color_green, color_blue: led color (0-255)
   - duration[0]_on: time in ms with the led on for the first pattern (0-65535)
   - duration[0]_off: time in ms with the led off for the first pattern (0-65535)
   - duration[1]_on: time in ms with the led on for the second pattern (0-65535)
   - duration[1]_off: time in ms with the led off for the second pattern (0-65535)

~~~~~~~~
ui led_blink_x3 <led_number> <intensity> <repetition_count> <color_red> <color_green> <color_blue> <duration[0]_on> <duration[0]_off> <duration[1]_on> <duration[1]_off> <duration[2]_on> <duration[2]_off>
~~~~~~~~
Play led pattern "blink x3":
   - led_number: id of the led
   - intensity: led intensity (0-255)
   - repetition_count: 0 - play led pattern only once, 1 to 255 - led pattern is repeated x times
   - color_red, color_green, color_blue: led color (0-255)
   - duration[0]_on: time in ms with the led on for the first pattern (0-65535)
   - duration[0]_off: time in ms with the led off for the first pattern (0-65535)
   - duration[1]_on: time in ms with the led on for the second pattern (0-65535)
   - duration[1]_off: time in ms with the led off for the second pattern (0-65535)
   - duration[1]_on: time in ms with the led on for the third pattern (0-65535)
   - duration[1]_off: time in ms with the led off for the third pattern (0-65535)

~~~~~~~~
ui led_wave_x1 <led_number> <intensity> <repetition_count> <color_red> <color_green> <color_blue> <duration[0]_on> <duration[0]_off>
~~~~~~~~
Play led pattern "wave x1":
   - led_number: id of the led
   - intensity: led intensity (0-255)
   - repetition_count: 0 - play led pattern only once, 1 to 255 - led pattern is repeated x times
   - color_red, color_green, color_blue: led color (0-255)
   - duration[0]_on: time in ms with the led on (0-65535)
   - duration[0]_off: time in ms with the led off (0-65535)

~~~~~~~~
ui led_wave_x2 <led_number> <intensity> <repetition_count> <color_red> <color_green> <color_blue> <duration[0]_on> <duration[0]_off> <duration[1]_on> <duration[1]_off>
~~~~~~~~
Play led pattern "wave x2":
   - led_number: id of the led
   - intensity: led intensity (0-255)
   - repetition_count: 0 - play led pattern only once, 1 to 255 - led pattern is repeated x times
   - color_red, color_green, color_blue: led color (0-255)
   - duration[0]_on: time in ms with the led on for the first pattern (0-65535)
   - duration[0]_off: time in ms with the led off for the first pattern (0-65535)
   - duration[1]_on: time in ms with the led on for the second pattern (0-65535)
   - duration[1]_off: time in ms with the led off for the second pattern (0-65535)

~~~~~~~~
ui vibr x2 <intensity> <duration_on_1> <duration_off_1> <duration_on_2> <duration_off_2> <repetition>
~~~~~~~~
Play vibration pattern "x2":
   - intensity: pulse amplitude (0-255)
   - duration_on_1: time in ms with vibration on for the first pattern (0-65535)
   - duration_off_1: time in ms with vibration off for the first pattern (0-65535)
   - duration_on_2: time in ms with vibration on for the second pattern (0-65535)
   - duration_off_2: time in ms with vibration off for the second pattern (0-65535)
   - repetition: 0 - play vibration pattern only once, 1 to 255 - vibration pattern is repeated x times

~~~~~~~~
ui vibr special <effect_1> <pause_1> <effect_2> <pause_2> <effect_3> <pause_3> <effect_4> <pause_4> <effect_5>
~~~~~~~~
Play vibration pattern "special":
   - effect_[1|2|3|4|5]: vibration effect (1-123), use 0 to stop
   - pause_[1|2|3|4] : time in ms with vibration off between each effect (0-65535)

@anchor adc_d
**ADC:**

~~~~~~~~
adc comp <channel> <vref> <polarity>
~~~~~~~~
Configure an adc comparator:
   - channel: [0-%d]
   - vref: [ref_a/ref_b]
   - polarity: [above/under]

@}
