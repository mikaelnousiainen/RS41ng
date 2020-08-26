# RS41ng

**NOTE:** This is a work in progress and most features do not work correctly yet!

This is a custom firmware for Vaisala RS41 radiosondes. Some of the code is based
on an earlier RS41 firmware project called [RS41HUP](https://github.com/df8oe/RS41HUP),
but most of it has been rewritten from scratch.

The motivation to develop this firmware is to provide cleaner, customizable and
more modular codebase for developing RS41 radiosonde-based experiments.

The main features this firmware aims to implement are:
* DMA/Timer-based modulation of APRS and some other digital modes via JTEncode library
* Support for HF/VHF transmissions via an external Si5351 chip connected to the external I²C bus
* Support for custom sensors via the external I²C bus

## Features (work in progress)

* APRS on 70cm amateur radio band via Si4032 radio chip
** Bell 202 timing is done via DMA transfers to achieve greater accuracy, but I have not been able to get the timings working correctly
* Digital mode beacons on HF frequencies via a Si5351 radio chip connected to the external I²C bus of the RS41 radiosonde
** The JTEncode library provides JT65/JT9/JT4/FT8/WSPR/FSQ beacon transmissions. I've decoded FT8 and WSPR successfully.
   The implementation is missing correct scheduling of transmissions via GPS clock.
* Support for additional sensors via the external I²C bus
** There is a driver for Bosch BMP280 barometric pressure / temperature / humidity sensor included 

### Bell FSK modulation hack for APRS

The idea behind the APRS / Bell 202 modulation implementation is based on RS41HUP project and its "ancestors"
and I'm describing it here, since it has not been documented elsewhere.

The Si4032 chip seems to support only a very specific type of FSK, where one can define two frequencies
(on a 625 Hz granularity) that can be toggled by a register change. Because of the granularity, this mechanism cannot be directly
used to generate Bell 202 tones.

The way Bell 202 AFSK is implemented for Si4032 is kind of a hack, where the code toggles these two frequencies at
a rate of 1200 and 2200 Hz, which produces the two Bell 202 tones even though the actual frequencies are something else.

Additionally, the timing of 1200/2200 Hz was done in RS41HUP by using experimentally determined delays
and by disabling all interrupts, so they won't interfere with the timings.

I have attempted to implement Bell 202 frequency generation using DMA / Timers, but have failed to generate correct
frequencies that other APRS equipment are able to decode. I have tried to decode the DMA-based modulation with
some tools intended for debugging APRS and while some bytes are decoded correctly every once in a while,
the timings are mostly off for some unknown reason.

# Authors

Mikael Nousiainen OH3BHX <oh3bhx@sral.fi>
