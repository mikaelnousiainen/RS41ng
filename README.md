# RS41ng - Amateur radio firmware for Vaisala RS41 radiosonde

**NOTE:** This is a work in progress and some features do not work correctly yet!

This is a custom, amateur radio-oriented firmware for Vaisala RS41 radiosondes.
Some of the code is based on an earlier RS41 firmware project called [RS41HUP](https://github.com/df8oe/RS41HUP),
but most of it has been rewritten from scratch. The Horus 4FSK code has been adapted from
the [darksidelemm fork of RS41HUP](https://github.com/darksidelemm/RS41HUP).

The motivation to develop this firmware is to provide cleaner, customizable and
more modular codebase for developing RS41 radiosonde-based experiments.

The main features this firmware aims to implement are:
* Support for transmitting multiple modes consecutively with custom, rotating comment messages
* Support for standard 1200-baud APRS
* Support for [Horus 4FSK mode](https://github.com/projecthorus/horusdemodlib/wiki) that has improved performance compared to APRS or RTTY
* Support for morse code (CW)
* Support for additional digital modes on HF/VHF amateur radio bands using an external Si5351 clock generator connected to the external I²C bus
* Support for custom sensors via the external I²C bus
* Enhanced support for the internal Si4032 radio transmitter via PWM-based tone generation (and ultimately DMA-based symbol timing, if possible)
* Extensibility to allow easy addition of new digital modes

## Features

* APRS on 70cm amateur radio band using the internal Si4032 radio transmitter
  * Bell 202 frequencies are generated via hardware PWM, but the symbol timing is created in a loop with delay
  * There is also code available to use DMA transfers for symbol timing to achieve greater accuracy, but I have not been able to get the timings working correctly
* Horus 4FSK on 70cm amateur radio band using the internal Si4032 radio transmitter
  * The Horus 4FSK mode has significantly [improved performance compared to APRS or RTTY](https://github.com/projecthorus/horusdemodlib/wiki)
  * Use [horus-gui](https://github.com/projecthorus/horus-gui) software to receive the 4FSK mode and to submit packets to [Habhub](http://habhub.org/) high-altitude balloon tracking platform
    * See [horus-gui installation and usage instructions](https://github.com/projecthorus/horusdemodlib/wiki/1.1-Horus-GUI-Reception-Guide-(Windows-Linux-OSX))
    * Based on [horusdemodlib](https://github.com/projecthorus/horusdemodlib) library that is responsible for demodulating the signal
* Morse code (CW) on on 70cm amateur radio band using the internal Si4032 radio transmitter
* Digital mode beacons on HF/VHF frequencies using a Si5351 clock generator connected to the external I²C bus of the RS41 radiosonde
  * The JTEncode library provides JT65/JT9/JT4/FT8/WSPR/FSQ beacon transmissions. I've decoded FT8 and WSPR successfully.
  * GPS-based scheduling is available for modes that require specific timing for transmissions
* External I²C bus sensor drivers
  * Bosch BMP280 barometric pressure / temperature / humidity sensor

### Planned features

* Support for more I²C sensors
* Configurable transmission frequencies and schedules based on location / altitude
* Morse code (CW) on Si5351 (HF + 2m)
* RTTY on both Si4032 (70cm, non-standard shift) and Si5351 (HF + 2m) with configurable shift
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
dnf install arm-none-eabi-gcc-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-binutils-cs arm-none-eabi-newlib cmake openocd
```

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

## Flashing the firmware

Hardware requirements:

* A working Vaisala RS41 radiosonde :)
* A programmer for the STM32 microcontroller present in the RS41 radiosonde. ST-LINK v2 USB-dongles work well.

The pinout of the RS41 connector (by DF8OE) is the following:

```
______________________|           |______________________
|                                                       |
|   1           2           3           4           5   |
|                                                       |
|   6           7           8           9          10   |
|_______________________________________________________|

(View from the bottom of the sonde, pay attention to the gap in the connector)
```

* 1 - SWDIO (PA13)
* 2 - RST
* 3 - MCU switched 3.3V out to external device / Vcc (Boost out) 5.0V
  -- This pin powers the device via 3.3V from an ST-LINK programmer dongle
* 4 - I2C2_SCL (PB10) / UART3 TX -- This is the external I²C port clock pin for Si5351 and sensors
* 5 - GND
* 6 - GND
* 7 - SWCLK (PA14)
* 8 - +U_Battery / VBAT 3.3V
* 9 - +VDD_MCU / PB1 * (10k + cap + 10k)
* 10 - I2C2_SDA (PB11) / UART3 RX -- This is the external I²C port data pin for Si5351 and sensors

### Steps to flash the firmware

1. Remove batteries from the sonde
2. Connect an ST-LINK v2 programmer dongle to the sonde via the following pins:
  * SWDIO -> Pin 1
  * SWCLK -> Pin 7
  * GND -> Pin 5
  * 3.3V -> Pin 3
3. Unlock the flash protection - needed only before reprogramming the sonde for the first time
  * `openocd -f ./openocd_rs41.cfg -c "init; halt; flash protect 0 0 64 off; exit"`
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

* Original codebase: DF8OE and other authors of the [RS41HUP](https://github.com/df8oe/RS41HUP) project
* Horus 4FSK code adapted from [darksidelemm fork of RS41HUP](https://github.com/darksidelemm/RS41HUP) project
* Mikael Nousiainen OH3BHX <oh3bhx@sral.fi>
