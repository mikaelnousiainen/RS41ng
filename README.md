# RS41ng - Amateur radio firmware for Vaisala RS41 radiosonde

**NOTE:** This firmware is a work in progress and some features might not work as expected yet!

This is a custom, amateur radio-oriented firmware for [Vaisala RS41 radiosondes](https://www.vaisala.com/en/products/instruments-sensors-and-other-measurement-devices/soundings-products/rs41).
Some of the code is based on an earlier RS41 firmware project called [RS41HUP](https://github.com/df8oe/RS41HUP),
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
  * Morse code (CW)
  * JT65/JT9/JT4/FT8/WSPR/FSQ digital modes on HF/VHF amateur radio bands using an external Si5351 clock generator connected to the external I²C bus
  * "Pip" mode, which transmits a short beep generated using CW to indicate presence of the transmitter
* Support for transmitting multiple modes consecutively with custom, rotating comment messages (see `config.c`)
* Support for GPS-based scheduling is available for transmission modes that require specific timing for transmissions
* Support for custom sensors via the external I²C bus
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

### External sensors

It is possible to connect external sensors to the I²C bus of the RS41 radiosonde.

The following sensors are currently supported:

* Bosch BMP280/BME280 barometric pressure / temperature / humidity sensor
  * Note that only BME280 sensors will report humidity. For BMP280 humidity readings will be zero.

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

## Building the firmware

Software requirements:

* [GNU GCC toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads/9-2-2019-12)
  version 8.3.0 or higher for cross-compiling the firmware for the ARM Cortex-M3 architecture (`arm-none-eabi-gcc`)
* [CMake](https://cmake.org/) version 3.6 or higher for building the firmware
* [OpenOCD](http://openocd.org/) version 0.10.0 or higher for flashing the firmware

On a Red Hat/Fedora Linux installation, the following packages can be installed:
```bash
dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-binutils-cs arm-none-eabi-newlib libbsd-devel cmake openocd
```

Note that the code uses `strlcpy()` also in the test code, which requires `libbsd-dev` (Debian/Ubuntu) or
`libbsd-devel` (Red Hat/Fedora) to be installed for compilation to succeed.

### Steps to build the firmware

1. Install the required software dependencies listed above
2. Build the firmware using the following commands
    ```
    mkdir build
    cd build
    cmake ..
    make
    ```
3. The firmware will be stored in file `build/src/RS41ng.elf`

### Building the firmware with Docker

Using Docker to build the firmware is usually the easiest option, because it provides a stable Fedora Linux-based
build environment on any platform. It should work on Windows and Mac operating systems too.

The Docker environment can also help address issues with the build process, including the `strlcpy()` errors observed on certain Linux distributions.

1. Install Docker if not already installed
2. Set the current directory to the RS41ng source directory
3. Build the RS41ng compiler Docker image using the following command. It is necessary to build the Docker image only once.
    ```
    docker build -t rs41ng_compiler .
    ```
4. Build the firmware using the following command. If you need to rebuild the firmware, simply run the command again.
    ```
    docker run --rm -it -v $(pwd):/usr/local/src/RS41ng rs41ng_compiler
    ```
5. The firmware will be stored in file `build/src/RS41ng.elf`

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

1. Remove batteries from the sonde
2. Connect an ST-LINK v2 programmer dongle to the sonde via the following pins:
  * SWDIO -> Pin 9 (SWDIO)
  * SWCLK -> Pin 8 (SWCLK)
  * GND -> Pin 1 (GND)
  * 3.3V -> Pin 5 (MCU switch 3.3V)
3. Unlock the flash protection - needed only before reprogramming the sonde for the first time
  * `openocd -f ./openocd_rs41.cfg -c "init; halt; flash protect 0 0 31 off; exit"`
  * **NOTE:** If the above command fails with an error about erasing sectors, try replacing the number `31` with `63`:
    * `openocd -f ./openocd_rs41.cfg -c "init; halt; flash protect 0 0 63 off; exit"`
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
