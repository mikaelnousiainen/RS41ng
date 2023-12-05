# RS41ng - Amateur radio firmware for Vaisala RS41 radiosonde
## Attention: This clone do not support DFM17 (cloned during implementation)

## This clone has special attention to 
- recover GPS after disortion
- TX Watchdog to reboot if 10 min silent.
- 4FSK Mode with two frequencies used alternating every intervall (reach more receivers)

**NOTE:** While this firmware has been tested with great success on a number of high-altitude balloon
flights, it is still a work in progress and some features might not work as expected yet!
In particular, the time sync (scheduling) features and use of an external Si5351 as a transmitter need more testing.

This is a custom, amateur radio-oriented firmware for [Vaisala RS41 radiosondes](https://www.vaisala.com/en/products/instruments-sensors-and-other-measurement-devices/soundings-products/rs41).
Some code is based on an earlier RS41 firmware project called [RS41HUP](https://github.com/df8oe/RS41HUP),
but most of it has been rewritten from scratch. The Horus 4FSK code has been adapted from
the [darksidelemm fork of RS41HUP](https://github.com/darksidelemm/RS41HUP).

## Asking questions, filing feature requests and reporting issues

* Please use [GitHub discussions](../../discussions) for asking questions and for sharing info about your RS41-based projects
  * For example, questions about firmware configuration and connecting of external chips to the sonde belong here
* Please use [GitHub issues](../../issues) to file new feature requests or issues that you have already identified with the firmware
  * However, please remember to post questions about usage to [GitHub discussions](../../discussions)

## What are the Vaisala RS41 radiosondes and how can I get one?

The RS41 radiosondes are used extensively for atmospheric sounding by the meteorological institutes in various countries and thus easily
available to be collected once they land, an activity called radiosonde hunting: see YouTube presentation about
[Tracking and Chasing Weather Balloons by Andreas Spiess](https://www.youtube.com/watch?v=vQfztG60umI) or
[Chasing Radiosonde Weather Balloons used in Meteorology for Fun by Mark VK5QI](https://www.youtube.com/watch?v=fb9gNomWrAY)
for more details!

You can track radiosondes without any additional equipment either via [SondeHub](https://tracker.sondehub.org/) or [radiosondy.info](https://radiosondy.info/)
that both use an existing network of receiver stations. Alternatively, you can set up your own radiosonde receiver station.

For your own receiver station, you will need:

1. A cheap software-defined radio USB dongle, such as an [RTL-SDR](https://www.rtl-sdr.com/about-rtl-sdr/)
2. An antenna suitable for receiving the 400 MHz radiosonde band transmissions. Antennas for the 70 cm amateur radio band usually work fine!
3. Radiosonde tracker software: common choices are [RS41 Tracker](http://escursioni.altervista.org/Radiosonde/) for Windows
   and [radiosonde_auto_rx](https://github.com/projecthorus/radiosonde_auto_rx) for Linux / Raspberry Pi.

### What can I do with an RS41 radiosonde?

The [Vaisala RS41 radiosondes](https://www.vaisala.com/en/products/instruments-sensors-and-other-measurement-devices/soundings-products/rs41)
uses an off-the-shelf [STM32F100C8](https://www.st.com/en/microcontrollers-microprocessors/stm32f100c8.html)
32-bit microcontroller, which can be reprogrammed using an [ST-LINK v2 programmer](https://www.st.com/en/development-tools/st-link-v2.html)
or a smaller [ST-LINK v2 USB dongle](https://www.adafruit.com/product/2548).

The RS41 hardware can be programmed to transmit different kinds of RF modulations (morse code, APRS and different FSK modulations)
on the 70 cm (~433 MHz) amateur radio band. The radiosonde contains a [UBX-G6010](https://www.u-blox.com/en/product/ubx-g6010-st-chip)
GPS chip, so it can be used as a tracker device, e.g. for high-altitude balloons.

The RS41ng firmware is just one example of what can be achieved with the RS41 hardware!

## Why does the RS41ng firmware exist?

The motivation to develop this firmware is to provide a clean, customizable and
modular codebase for developing RS41 radiosonde-based experiments.

See the feature list below.

## Features

The main features the RS41ng firmware are:

* Support for multiple transmission modes:
  * Standard 1200-baud APRS
    * Option to transmit APRS weather reports using readings from an external BMP280/BME280 sensor
  * [Horus 4FSK v1 and v2 modes](https://github.com/projecthorus/horusdemodlib/wiki) that has improved performance compared to APRS or RTTY
    * There is an option to use continuous transmit mode (for either V1 or V2 mode), which helps with receiver frequency synchronization and improves reception.
    * In order to use Horus 4FSK mode on a flight, you will need to request a new Horus 4FSK payload ID in GitHub according to the instructions at: https://github.com/projecthorus/horusdemodlib/wiki#how-do-i-transmit-it
  * Morse code (CW)
  * JT65/JT9/JT4/FT8/WSPR/FSQ digital modes on HF/VHF amateur radio bands using an external Si5351 clock generator connected to the external I²C bus
  * "Pip" mode, which transmits a short beep generated using CW to indicate presence of the transmitter
* Support for transmitting multiple modes consecutively with custom, rotating comment messages (see `config.c`)
* Support for GPS-based scheduling is available for transmission modes that require specific timing for transmissions
* Support for custom sensors via the external I²C bus
* Support for counting pulses on expansion header pin 2 (I2C2_SDA (PB11) / UART3 RX) for use with sensors like Geiger counters
* GPS NMEA data output via the external serial port pin 3 (see below). This disables use of I²C devices as the serial port pins are shared with the I²C bus pins.
  * This allows using the RS41 sonde GPS data in external tracker hardware, such as Raspberry Pi or other microcontrollers.
* Enhanced support for the internal Si4032 radio transmitter via PWM-based tone generation (and ultimately DMA-based symbol timing, if possible)
* Extensibility to allow easy addition of new transmission modes and new sensors

### Transmission modes

On the internal Si4032 transmitter:

* APRS (1200 baud)
* Horus 4FSK v1 and v2 (100 baud)
* Morse code (CW)
* "Pip" - a short beep to indicate presence of the transmitter

On an external Si5351 clock generator connected to the external I²C bus of the RS41 radiosonde:

* Horus 4FSK v1 and v2 (50 baud, because the Si5351 frequency changes are slow)
* JT65/JT9/JT4/FT8/WSPR/FSQ mode beacon transmissions using the JTEncode library. I've decoded FT8, WSPR and FSQ modes successfully.
  * GPS-based scheduling is available for modes that require specific timing for transmissions
* Morse code (CW)
* "Pip"

#### Notes about APRS

* Bell 202 frequencies are generated via hardware PWM, but the symbol timing is created in a loop with delay
* There is also code available to use DMA transfers for symbol timing to achieve greater accuracy, but I have not been able to get the timings working correctly

#### Notes about Horus 4FSK

* The Horus 4FSK v1 and v2 modes have significantly [improved performance compared to APRS or RTTY](https://github.com/projecthorus/horusdemodlib/wiki).
* Use [horus-gui](https://github.com/projecthorus/horus-gui) software to receive the 4FSK mode and to submit packets to [Habhub](http://habhub.org/) high-altitude balloon tracking platform.
* See [horus-gui installation and usage instructions](https://github.com/projecthorus/horusdemodlib/wiki/1.1-Horus-GUI-Reception-Guide-(Windows-Linux-OSX)) and [horusdemodlib](https://github.com/projecthorus/horusdemodlib) library that is responsible for demodulating the signal.
* In order to use Horus 4FSK mode on a flight, you will need to request a new Horus 4FSK payload ID in GitHub according to the instructions at: https://github.com/projecthorus/horusdemodlib/wiki#how-do-i-transmit-it

### External sensors

It is possible to connect external sensors to the I²C bus of the RS41 radiosonde.

The following sensors are currently supported:

* Bosch BMP280/BME280 barometric pressure / temperature / humidity sensor
  * Note that only BME280 sensors will report humidity. For BMP280 humidity readings will be zero.
* RadSens universal dosimeter-radiometer module for measuring radiation
  * https://www.tindie.com/stores/climateguard/
  * https://github.com/climateguard/RadSens

Sensor driver code contributions are welcome!

### Planned features

* Configurable transmission frequencies and schedules based on location / altitude
* Support for more I²C sensors
* RTTY on both Si4032 (70 cm, non-standard shift) and Si5351 (HF + 2m) with configurable shift
* Investigate possibility to implement 1200 bps Bell 202 modulation (and
  possibly also 300 bps Bell 103 modulation) for APRS using Si5351,
  this requires special handling to make Si5351 change frequency quickly
    * See: https://github.com/etherkit/Si5351Arduino/issues/22

## Configuring the firmware

1. Configure your amateur radio call sign, transmission schedule (time sync),
   transmit frequencies and transmission mode parameters in `config.h`
2. Set up transmitted message templates in `config.c`, depending on the modes you use

### Power consumption and power-saving features

Power consumption notes (at 3V supply voltage) by Mark VK5QI:

- GPS in full power acquisition mode: 110-120 mA (TX off), 160-170 mA (TX on)
- GPS locked (5 sats), full power: 96 - 126 mA (TX off), 170 mA (TX on)
- GPS locked, powersave mode, state 1: ~96 - 110 mA (TX off), ?
- GPS locked, powersave mode, state 2: 60 - 90mA (TX off), 130 mA (TX on) - average 72 mA with TX every minute 

The variations seem to be the GPS powering up every second to do its fixes.

### Time sync settings

The time sync feature is a simple way to activate the transmissions every N seconds, delayed by the `TIME_SYNC_OFFSET_SECONDS` setting.

Please note that the time sync requires a stable GPS signal (good visibility to the sky) to work correctly!

#### Time-slotted modes

For time-slotted modes like FT8 and WSPR, there default configuration file (`config.h`) already contains useful defaults.

##### FT8 example

The default FT8 configuration is:

```
#define FT8_TIME_SYNC_SECONDS 15
#define FT8_TIME_SYNC_OFFSET_SECONDS 1
```

The above means that transmissions should start every 15 seconds (counting from the beginning of each hour),
but delayed by 1 second. As an example, transmissions after 12:00:00 (hh:mm:ss) would happen at
12:00:01, 12:00:16, 12:00:31; 12:00:46, 12:01:01 and so on.

The offset of 1 second is just to make sure the transmission starts strictly after the 15-second periods and never before.

If the offset was 5 seconds, transmissions would happen at 12:00:05, 12:00:20, 12:00:35, 12:00:50 and 12:01:05.

In order to transmit only in even or odd time slots of FT8, you could use:

```
// Transmit every 30 seconds, even time slot
#define FT8_TIME_SYNC_SECONDS 30
#define FT8_TIME_SYNC_OFFSET_SECONDS 1
```

```
// Transmit every 30 seconds, odd time slot
#define FT8_TIME_SYNC_SECONDS 30
#define FT8_TIME_SYNC_OFFSET_SECONDS 16
```

The latter (odd time slot) example would transmit at 12:00:16, 12:00:46, 12:01:16, 12:01:46 and so on.

##### WSPR example

For WSPR, the transmissions happen every 120 seconds, so the configuration has to be:

```
#define WSPR_TIME_SYNC_SECONDS 120
#define WSPR_TIME_SYNC_OFFSET_SECONDS 1
```

The above means that transmissions should start every 120 seconds (counting from the beginning of each hour),
but delayed by 1 second. As an example, transmissions after 12:00:00 (hh:mm:ss) would happen at
12:00:01, 12:02:01, 12:04:01 and so on.

The offset of 1 second is just to make sure the transmission starts strictly after the 120-second periods and never before.

#### Manually configuring time slots for multiple payloads transmitting at different times

Sometimes it is necessary to set transmission time slots when using multiple payloads/transmitters on the same frequency.
The time slots guarantee that the transmissions from different payloads will not happen simultaneously.

The time sync settings can be used to configure this type of time slots.

In this example, we configure 3 payloads to transmit every 120 seconds, so that each payload is scheduled to transmit
evenly across the 120-second period, every 40 seconds (120 / 3 = 40).

First, the `TIME_SYNC_SECONDS` setting of the particular mode has to be set to the *full interval where all payloads are expected to have their transmissions finished*.
This setting has to be the same for every payload. Do also remember to take into account transmission length too (depending on the mode).

Next, the `TIME_SYNC_OFFSET_SECONDS` needs to be set independently for each payload. The first payload would have
an offset of 0, the second an offset of 40 and the third and offset of 80.

As an example, transmissions after 12:00:00 (hh:mm:ss) would happen at:

- Payload 1: 12:00:00
- Payload 2: 12:00:40
- Payload 3: 12:01:20
- Payload 1: 12:02:00
- Payload 2: 12:02:40
- Payload 3: 12:03:20
- ...

The configuration with 3 payloads using Horus 4FSK V2 mode would be the following.

Payload 1:

```
#define HORUS_V2_TIME_SYNC_SECONDS 120 // everything happens at 120 seconds interval
#define HORUS_V2_TIME_SYNC_OFFSET_SECONDS 0 // the first payload will transmit at 0 seconds within the 120 second interval
```

Payload 2:

```
#define HORUS_V2_TIME_SYNC_SECONDS 120 // everything happens at 120 seconds interval
#define HORUS_V2_TIME_SYNC_OFFSET_SECONDS 40 // the second payload will transmit at 40 seconds within the 120 second interval
```

Payload 3:

```
#define HORUS_V2_TIME_SYNC_SECONDS 120 // everything happens at 120 seconds interval
#define HORUS_V2_TIME_SYNC_OFFSET_SECONDS 80 // the third payload will transmit at 80 seconds within the 120 second interval
```

### Using two alternating frequencies in HORUS-V1/V2 Mode

In Europe, there are two areas where the receivers listen on their respective regional frequencies. So the problem can arise that you have to decide
which channel to choose for the best reception reports on the route. So the idea was to define an additional channel for sending.
You can find these two new entries in config.h 

```
// V1 and V2 Version of horus 4FSK have their own switch
#define RADIO_SI4032_TX_FREQUENCY2_HORUS_V2_ACTIV  true
#define RADIO_SI4032_TX_FREQUENCY2_HORUS_V2        437600000

#define RADIO_SI4032_TX_FREQUENCY2_HORUS_V1_ACTIV  false
#define RADIO_SI4032_TX_FREQUENCY2_HORUS_V1        437600000
```

If activated, the frequency will change after each TX Intervall in HORUS-V1 also V2-Mode (not working in continous mode)

### TX-Watchdog: Rebooting after no transmissions over long time

While doing much testing sessions, sometimes the sonde was found in an state where GPS signals good conditions (long interval blink)
but there where no transmissions over a longer time. I did not found the reason so i decided to implement an watchdog. You can define the timeout
in the config (see sample)

The `config.h` shows new entries to activate this improvement.

```
// Enable this setting to watch TX and make a cold start on the CPU if no transmission occured after TX_LOST_FOR_SECONDS 
// TX hangs up longer then 10 min, then REBOOT
#define RADIO_TX_LOST_CHECK_ENABLE true
#define RADIO_TX_LOST_FOR_SECONDS 600
```

### Flying solar powered 

#### GPS-Watchdog: Rebooting after not reaching a 3D fix

Sometimes you launch the sonde in an area where GPS cannot be received and then take it to a place where GPS is available again. 
Also if the sonde lost GPS in a jamming-zone. Anyway if this signal lost longer then aprox. 20 seconds the Ublox fall into a sleep mode.
I did not found a way to get it out of that sleep mode other then making a reboot. Thats what acually is implemented.
You can decide what the sonde should do after lost GPS signal and how long it waits before kicks an reboot.

There are ideas to prepare a RS41 powered by solar panels only. Goal is to fly more then one day don't have this limit with batteries. 
Some addional enhancements are needable to reach this goal. One of them is to make sure after sunrise that there is a GPS fix.
To solve this its a good idea to make a System Reset in the air if a missing GPS-FIX reaches out some time limit (e.g. 3 minutes or longer).

The `config.h` shows new entries to activate this improvement.

```
// Enable this setting to make a cold start on the CPU if GPS-Fix missing longer then GPS_REBOOT_MISSING_GPS_FIX_SECONDS 
#define GPS_REBOOT_MISSING_GPS_FIX_ENABLE true

// If enabled above, define threschold how long a missing GPS-Fix before cold start (dont hold it to short)
#define GPS_REBOOT_MISSING_GPS_FIX_SECONDS 200
```

#### Probe does not start at sunrise because current too low

If solar panels are to be used, it is important to consider a few things:

- Autostart nessesery. Currently, it is still unclear when the RS41ng software will cause a probe to start on its own when batteries are connected.
If the batteries are inserted into the existing holder (2 x AA type) the voltage is conducted via a voltage regulator on the board. In this case, 
the button on the board probably works and must be pressed to start the software. This is not helpfull if you wait for pushing a button in the air.
But there is another method to connect the battery voltage. In this case, the regulator is bypassed. Here the probe starts immediately as soon as at 
least 1.5 volts are present (up to 4 volts are tested). The battery is connected directly to one end of an SMD component. 
I'm sure your community has more info on this.
- Solar panels should deliver 3-4 volts and a current of 200 mA.
- You need a capacity of around 2.000 µF to buffer the solar power a little bit. Somehow the RS41 will not start without them.

Lets have look on the voltage if the sun rises:
- Its dark, the capacitors are empty.
- First sun on the panels brings the voltage up to 300 mV. This will load slowly the capacitors.
- After a while the voltage comes up to 500 mV. The LEDs on the board start to glow and flicker a bit.
- More and more sun falling on the panels. The voltage climbs to 800-1.000 mV. Over 800 mV the red LED shows that the software starts but...
- While the voltage is still to low on 800 mV the main procedure on the board tries to init some hardware elements. This is successful 
up to the moment when the GPS module gets initialized. There is a loop programmed and waiting the gps_init will work one day. Not this time.
- The voltage climbs still in background but the loop will never end. The only way to init the GPS unit successful is to initiate a System Reset.
- A System Restart is now implemented after waiting 10 times the loop initiated the GPS unit unsuccessfull.
- If the System is booting meanwhile the voltage is up to get the GPS unit initiated this time. 
- Everything fine now .... But there is no guarantee this way of recovering is the best one - in practice it works.



## Building the firmware

The easiest and the recommended method to build the firmware is using Docker.

If you have a Linux environment -- Windows Subsystem for Linux (WSL) or macOS might work too -- and
you feel adventurous, you can try to build using the Linux-based instructions.

### Building the firmware with Docker

Using Docker to build the firmware is usually the easiest option, because it provides a stable Fedora Linux-based
build environment on any platform. It should work on Windows and Mac operating systems too.

The Docker environment can also help address issues with the build process.

1. Install Docker if not already installed
2. Set the current directory to the RS41ng source directory
3. Build the RS41ng compiler Docker image using the following command. It is necessary to build the Docker image only once.
    ```
    docker build -t rs41ng_compiler .
    ```
4. Build the firmware using the following command. If you need to rebuild the firmware, simply run the command again.
   On Linux/macOS, run:
    ```
    docker run --rm -it -v $(pwd):/usr/local/src/RS41ng rs41ng_compiler
    ```
    On Windows, run:
    ```
    docker run --rm -it -v %cd%:/usr/local/src/RS41ng rs41ng_compiler
    ```
5. The firmware will be stored in file `build/src/RS41ng.elf`

Now you can flash the firmware using instructions below (skip the build instructions for Linux).

### Building the firmware in a Linux environment

Software requirements:

* [GNU GCC toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads/9-2-2019-12)
  version 8.3.0 or higher for cross-compiling the firmware for the ARM Cortex-M3 architecture (`arm-none-eabi-gcc`)
* [CMake](https://cmake.org/) version 3.6 or higher for building the firmware
* [OpenOCD](http://openocd.org/) version 0.10.0 or higher for flashing the firmware

On a Red Hat/Fedora Linux installation, the following packages can be installed:
```bash
dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-binutils-cs arm-none-eabi-newlib cmake openocd
```

#### Steps to build the firmware on Linux

1. Install the required software dependencies listed above
2. Build the firmware using the following commands
    ```
    mkdir build
    cd build
    cmake ..
    make
    ```
3. The firmware will be stored in file `build/src/RS41ng.elf`

## Flashing the firmware

Hardware requirements:

* A working [Vaisala RS41 radiosonde](https://www.vaisala.com/en/products/instruments-sensors-and-other-measurement-devices/soundings-products/rs41) :)
* An [ST-LINK v2 programmer for the STM32 microcontroller](https://www.st.com/en/development-tools/st-link-v2.html) in the RS41 radiosonde. 
  * These smaller [ST-LINK v2 USB dongles](https://www.adafruit.com/product/2548) also work well.
  * [Partco sells the ST-LINK v2 USB dongles in Finland](https://www.partco.fi/en/measurement/debugging/20140-stlinkv2.html)

The pinout of the RS41 connector (by DF8OE and VK5QI) is the following:

```
______________________|           |______________________
|                                                       |
|   9           7           5           3           1   |
|                                                       |
|   10          8           6           4           2   |
|_______________________________________________________|

(View from the bottom of the sonde, pay attention to the gap in the connector)
```

* 1 - GND
* 2 - I2C2_SDA (PB11) / UART3 RX
    * This is the external I²C port data pin for Si5351 and sensors
    * This pin can be used as input for the pulse counter.
* 3 - I2C2_SCL (PB10) / UART3 TX
    * This is the external I²C port clock pin for Si5351 and sensors
    * This pin can alternatively be used to output GPS NMEA data to external tracker hardware (e.g. Raspberry Pi or other microcontrollers)
* 4 - +VDD_MCU / PB1 * (10k + cap + 10k)
* 5 - MCU switched 3.3V out to external device / Vcc (Boost out) 5.0V
    * This pin powers the device via 3.3V voltage from an ST-LINK programmer dongle
    * This pin can be used to supply power to external devices, e.g. Si5351, BMP280 or other sensors
* 6 - +U_Battery / VBAT 3.3V
* 7 - RST
* 8 - SWCLK (PA14)
* 9 - SWDIO (PA13)
* 10 - GND

### Steps to flash the firmware

1. If your ST-LINK v2 programmer is capable of providing power (as some third-party clones are),
   remove the batteries from the sonde. Otherwise, leave the batteries in and power on the sonde.
2. Connect an ST-LINK v2 programmer dongle to the sonde via the following pins:
  * SWDIO -> Pin 9 (SWDIO)
  * SWCLK -> Pin 8 (SWCLK)
  * GND -> Pin 1 (GND)
  * 3.3V -> Pin 5 (MCU switch 3.3V) (only required when using the programmer to power the sonde)
3. Unlock the flash protection - needed only before reprogramming the sonde for the first time
  * `openocd -f ./openocd_rs41.cfg -c "init; halt; flash protect 0 0 63 off; exit"`
  * **NOTE:** If the above command fails with an error about erasing sectors, try replacing the number `63` with `31`:
    * `openocd -f ./openocd_rs41.cfg -c "init; halt; flash protect 0 0 31 off; exit"`
4. Flash the firmware
  * `openocd -f ./openocd_rs41.cfg -c "program build/src/RS41ng.elf verify reset exit"`
5. Power cycle the sonde to start running the new firmware

## Developing / debugging the firmware

It is possible to receive log messages from the firmware program and to perform debugging of the firmware using GNU GDB.

Also, please note that Red Hat/Fedora do not provide GDB for ARM architectures, so you will need to manually download
and install GDB from [ARM GNU GCC toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads/9-2-2019-12).

Semihosting allows the firmware to send log messages via special system calls to OpenOCD, so that you
can get real-time feedback and debug output from the application.

**Semihosting has to be disabled when the RS41 radiosonde is not connected to the STM32 programmer dongle,
otherwise the firmware will not run.**

### Steps to start firmware debugging and semihosting

1. Connect the RS41 radiosonde to a computer via the STM32 ST-LINK programmer dongle
2. Enable semihosting and logging in `config.h` by uncommenting the following lines
    ```
    #define SEMIHOSTING_ENABLE
    #define LOGGING_ENABLE
    ```
3. Start OpenOCD and leave it running in the background
    ```
    openocd -f ./openocd_rs41.cfg
    ```
4. Start ARM GDB
    ```
    arm-none-eabi-gdb
    ```
5. Connect GDB to OpenOCD for flashing and debugging (assumes you are in the `build` directory with Makefiles from CMake ready for build)
    ```
    target remote localhost:3333
    monitor arm semihosting enable
    make
    load src/RS41ng.elf
    monitor reset halt
    continue # this command runs the firmware
    ```
6. OpenOCD will output log messages from the firmware and GDB can be used to interrupt
   and inspect the firmware program.

To load debugging symbols for settings breakpoints and to perform more detailed inspection,
use command `file src/RS41ng.elf`.

## Si4032 Bell FSK modulation hack for APRS

The idea behind the APRS / Bell 202 modulation implementation is based on RS41HUP project and its "ancestors"
and I'm describing it here, since it has not been documented elsewhere.

The Si4032 chip seems to support only a very specific type of FSK, where one can define two frequencies
(on a 625 Hz granularity) that can be toggled by a register change. Because of the granularity, this mechanism cannot be directly
used to generate Bell 202 tones.

The way Bell 202 AFSK is implemented for Si4032 is kind of a hack, where the code toggles these two frequencies at
a rate of 1200 and 2200 Hz, which produces the two Bell 202 tones even though the actual frequencies are something else.

Additionally, the timing of 1200/2200 Hz was done in RS41HUP by using experimentally determined delays
and by disabling all interrupts, so they won't interfere with the timings.

I have attempted to implement Bell 202 frequency generation using hardware DMA and PWM, but have failed to generate
correct symbol rate that other APRS equipment are able to decode. I have tried to decode the DMA-based modulation with
some tools intended for debugging APRS and while some bytes are decoded correctly every once in a while,
the timings are mostly off for some unknown reason.

Currently, the Bell 202 modulation implementation uses hardware PWM to generate the individual tone frequencies,
but the symbol timing is created in a loop with delay that was chosen carefully via experiments.

## Debugging APRS

Here are some tools and command-line examples to receive and debug APRS messages using an
SDR receiver. There are examples for using both [rx_tools](https://github.com/rxseger/rx_tools)
and [rtl-sdr](https://github.com/osmocom/rtl-sdr) tools to interface with the SDR receiver.
The example commands assume you are using an RTL-SDR dongle, but `rx_fm` (from `rx_tools`)
supports other types of devices too, as it's based on SoapySDR.

### Dire Wolf

[Dire Wolf](https://github.com/wb2osz/direwolf) can decode APRS (and lots of other digital modes)
from audio streams.

rx_tools:

```bash
rx_fm -f 432500000 -M fm -s 250000 -r 48000 -g 22 -d driver=rtlsdr - | direwolf -n 1 -D 1 -r 48000 -b 16 -
```

rtl-sdr:

```bash
rtl_fm -f 432500000 -M fm -s 250k -r 48000 -g 22 - | direwolf -n 1 -D 1 -r 48000 -b 16 -
```

### SigPlay

[SigPlay](https://bk.gnarf.org/creativity/sigplay/) is a set of tools for DSP and signal processing.
SigPlay also includes a command-line tool to decode and print out raw data from Bell 202 encoding,
which is really useful, as it allows you to see the bytes that actually get transmitted --
even if the packet is not a valid APRS packet!

rx_tools:

```bash
rx_fm -f 432500000 -M fm -s 250000 -r 48000 -g 22 -d driver=rtlsdr - | ./aprs -
```

rtl-sdr:

```bash
rtl_fm -f 432500000 -M fm -s 250k -r 48000 -g 22 - | ./aprs -
```

# Authors

* Mikael Nousiainen OH3BHX <oh3bhx@sral.fi>
* Original codebase: DF8OE and other authors of the [RS41HUP](https://github.com/df8oe/RS41HUP) project
* Horus 4FSK code adapted from [darksidelemm fork of RS41HUP](https://github.com/darksidelemm/RS41HUP) project

# Additional documentation

## Vaisala RS41 hardware documentation

* https://github.com/bazjo/RS41_Hardware - Reverse-engineered documentation on the RS41 hardware
* https://github.com/bazjo/RS41_Decoding - Information about decoding the RS41 data transmission
* http://happysat.nl/RS-41/RS41.html - Vaisala RS-41 SGP Modification and info about the original firmware settings
* https://destevez.net/2018/06/flashing-a-vaisala-rs41-radiosonde/
* https://destevez.net/2017/11/tracking-an-rs41-sgp-radiosonde-and-reporting-to-aprs/

## Alternative firmware projects

* https://github.com/df8oe/RS41HUP - The original amateur radio firmware for RS41
* https://github.com/darksidelemm/RS41HUP - A fork of the original firmware that includes support for Horus 4FSK (but omits APRS)
* https://github.com/darksidelemm/RS41FOX - RS41-FOX - RS41 Amateur Radio Direction Finding (Foxhunting) Beacon
* http://www.om3bc.com/docs/rs41/rs41_en.html
