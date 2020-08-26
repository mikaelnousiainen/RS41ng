JT65/JT9/JT4/FT8/WSPR/FSQ Encoder Library for Arduino
=====================================================

This library very simply generates a set of channel symbols for JT65, JT9, JT4, FT8, or WSPR based on the user providing a properly formatted Type 6 message for JT65, JT9, or JT4 (which is 13 valid characters), Type 0.0 or 0.5 message for FT8 (v2.0.0 protocol) or a callsign, Maidenhead grid locator, and power output for WSPR. It will also generate an arbitrary FSQ message of up to 200 characters in both directed and non-directed format. When paired with a synthesizer that can output frequencies in fine, phase-continuous tuning steps (such as the Si5351), then a beacon or telemetry transmitter can be created which can change the transmitted characters as needed from the Arduino.

Please feel free to use the issues feature of GitHub if you run into problems or have suggestions for important features to implement.

Thanks For Your Support!
------------------------
If you would like to support my library development efforts, I would ask that you please consider sending a [one-time PayPal tip](https://paypal.me/NT7S) or [subscribe to me on SubscribeStar](https://www.subscribestar.com/nt7s) for an ongoing contribution.. Thank you!

Hardware Requirements and Setup
-------------------------------
This library has been written for the Arduino platform and has been successfully tested on the Arduino Uno, an Uno clone, an Arduino Zero clone, and a NodeMCU. Since the library itself does not access the hardware, there is no reason it should not run on any Arduino model of recent vintage as long as it has at least 2 kB of RAM.

How To Install
--------------
The best way to install the library is via the Arduino Library Manager, which is available if you are using Arduino IDE version 1.6.2 or greater. To install it this way, simply go to the menu Sketch > Include Library > Manage Libraries..., and then in the search box at the upper-right, type "Etherkit JTEncode". Click on the entry in the list below, then click on the provided "Install" button. By installing the library this way, you will always have notifications of future library updates, and can easily switch between library versions.

If you need to or would like to install the library in the old way, then you can download a copy of the library in a ZIP file. Download a ZIP file of the library from the GitHub repository by going to [this page](https://github.com/etherkit/JTEncode/releases) and clicking the "Source code (zip)" link under the latest release. Finally, open the Arduino IDE, select menu Sketch > Import Library... > Add Library..., and select the ZIP that you just downloaded.

RAM Usage
---------
Most of the encoding functions need to manipulate multiple arrays of symbols in RAM at the same time, and therefore are quite RAM intensive. Care has been taken to put as much data into program memory as is possible, but the encoding functions still can cause problems with the low RAM microcontrollers such as the ATmegaxx8 series. If you are using these, then please be sure to call them only once when a transmit buffer needs to be created or changed, and call them separately of other subroutine calls. When using other microcontrollers that have more RAM, such as most of the ARM ICs, this won't be as much of a problem. If you see unusual freezes, that almost certainly indicates a RAM shortage.

Example
-------
There is a simple example that is placed in your examples menu under JTEncode. Open this to see how to incorporate this library with your code. The example provided with with the library is meant to be used in conjunction with the [Etherkit Si5351A Breakout Board](https://www.etherkit.com/rf-modules/si5351a-breakout-board.html), although it could be modified to use with other synthesizers which meet the technical requirements of the JT65/JT9/JT4/WSPR/FSQ modes.

To run this example, be sure to download the [Si5351Arduino](https://github.com/etherkit/Si5351Arduino) library and follow the instructions there to connect the Si5351A Breakout Board to your Arduino. In order to trigger transmissions, you will also need to connect a momentary pushbutton from pin 12 of the Arduino to ground.

The example sketch itself is fairly straightforward. JT65, JT9, JT4, FT8, WSPR, and FSQ modes are modulated in same way: phase-continuous multiple-frequency shift keying (MFSK). The message to be transmitted is passed to the JTEncode method corresponding to the desired mode, along with a pointer to an array which holds the returned channel symbols. When the pushbutton is pushed, the sketch then transmits each channel symbol sequentially as an offset from the base frequency given in the sketch define section.

An instance of the JTEncode object is created:

    JTEncode jtencode;

On sketch startup, the mode parameters are set based on which mode is currently selected (by the DEFAULT_MODE define):

    // Set the proper frequency, tone spacing, symbol count, and
    // tone delay depending on mode
    switch(cur_mode)
    {
    case MODE_JT9:
      freq = JT9_DEFAULT_FREQ;
      symbol_count = JT9_SYMBOL_COUNT; // From the library defines
      tone_spacing = JT9_TONE_SPACING;
      tone_delay = JT9_DELAY;
      break;
    case MODE_JT65:
      freq = JT65_DEFAULT_FREQ;
      symbol_count = JT65_SYMBOL_COUNT; // From the library defines
      tone_spacing = JT65_TONE_SPACING;
      tone_delay = JT65_DELAY;
      break;
    case MODE_JT4:
      freq = JT4_DEFAULT_FREQ;
      symbol_count = JT4_SYMBOL_COUNT; // From the library defines
      tone_spacing = JT4_TONE_SPACING;
      tone_delay = JT4_DELAY;
      break;
    case MODE_WSPR:
      freq = WSPR_DEFAULT_FREQ;
      symbol_count = WSPR_SYMBOL_COUNT; // From the library defines
      tone_spacing = WSPR_TONE_SPACING;
      tone_delay = WSPR_DELAY;
      break;
    case MODE_FT8:
      freq = FT8_DEFAULT_FREQ;
      symbol_count = FT8_SYMBOL_COUNT; // From the library defines
      tone_spacing = FT8_TONE_SPACING;
      tone_delay = FT8_DELAY;
      break;
    case MODE_FSQ_2:
      freq = FSQ_DEFAULT_FREQ;
      tone_spacing = FSQ_TONE_SPACING;
      tone_delay = FSQ_2_DELAY;
      break;
    case MODE_FSQ_3:
      freq = FSQ_DEFAULT_FREQ;
      tone_spacing = FSQ_TONE_SPACING;
      tone_delay = FSQ_3_DELAY;
      break;
    case MODE_FSQ_4_5:
      freq = FSQ_DEFAULT_FREQ;
      tone_spacing = FSQ_TONE_SPACING;
      tone_delay = FSQ_4_5_DELAY;
      break;
    case MODE_FSQ_6:
      freq = FSQ_DEFAULT_FREQ;
      tone_spacing = FSQ_TONE_SPACING;
      tone_delay = FSQ_6_DELAY;
      break;
    }

Note that the number of channel symbols for each mode is defined in the library, so you can use those defines to initialize your own symbol array sizes.

Before transmit, the proper class method is chosen based on the desired mode, then the transmit symbol buffer and the other mode information is set:

    // Set the proper frequency and timer CTC depending on mode
    switch(cur_mode)
    {
    case MODE_JT9:
      jtencode.jt9_encode(message, tx_buffer);
      break;
    case MODE_JT65:
      jtencode.jt65_encode(message, tx_buffer);
      break;
    case MODE_JT4:
      jtencode.jt4_encode(message, tx_buffer);
      break;
    case MODE_WSPR:
      jtencode.wspr_encode(call, loc, dbm, tx_buffer);
      break;
    case MODE_FT8:
      jtencode.ft_encode(message, tx_buffer);
      break;
    case MODE_FSQ_2:
    case MODE_FSQ_3:
    case MODE_FSQ_4_5:
    case MODE_FSQ_6:
      jtencode.fsq_dir_encode(call, "n0call", " ", "hello world", tx_buffer);
      break;
    }

As mentioned above, it is best if the message encoding functions are called only when needed, in its own subroutine.

Once the channel symbols have been generated, it is a simple matter of transmitting them in sequence, each the correct amount of time:

    // Now transmit the channel symbols
    for(i = 0; i < symbol_count; i++)
    {
        si5351.set_freq((freq * 100) + (tx_buffer[i] * tone_spacing), SI5351_CLK0);
        delay(tone_delay);
    }

Public Methods
------------------
### jt65_encode()
```
/*
 * jt65_encode(const char * message, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters and returns
 * a channel symbol table.
 *
 * message - Plaintext Type 6 message.
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least size JT65_SYMBOL_COUNT to the method.
 *
 */
```
### jt9_encode()
```
/*
 * jt9_encode(const char * message, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters and returns
 * a channel symbol table.
 *
 * message - Plaintext Type 6 message.
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least size JT9_SYMBOL_COUNT to the method.
 *
 */
```

### jt4_encode()
```
/*
 * jt4_encode(const char * message, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters and returns
 * a channel symbol table.
 *
 * message - Plaintext Type 6 message.
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least size JT9_SYMBOL_COUNT to the method.
 *
 */
 ```

### wspr_encode()
```
/*
 * wspr_encode(const char * call, const char * loc, const uint8_t dbm, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters and returns
 *
 * call - Callsign (6 characters maximum).
 * loc - Maidenhead grid locator (4 characters maximum).
 * dbm - Output power in dBm.
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least size WSPR_SYMBOL_COUNT to the method.
 *
 */
```

### ft8_encode()
```
/*
 * ft8_encode(const char * message, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters or a telemetry message
 * of up to 18 hexadecimal digit (in string format) and returns a channel symbol table.
 * Encoded for the FT8 protocol used in WSJT-X v2.0 and beyond (79 channel symbols).
 *
 * message - Type 0.0 free text message or Type 0.5 telemetry message.
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least size FT8_SYMBOL_COUNT to the method.
 *
 */
 ```

### fsq_encode()
```
/*
 * fsq_encode(const char * from_call, const char * message, uint8_t * symbols)
 *
 * Takes an arbitrary message and returns a FSQ channel symbol table.
 *
 * from_call - Callsign of issuing station (maximum size: 20)
 * message - Null-terminated message string, no greater than 130 chars in length
 * symbols - Array of channel symbols to transmit returned by the method.
 *  Ensure that you pass a uint8_t array of at least the size of the message
 *  plus 5 characters to the method. Terminated in 0xFF.
 *
 */
```

### fsq_dir_encode()
 ```
/*
* fsq_dir_encode(const char * from_call, const char * to_call, const char cmd, const char * message, uint8_t * symbols)
*
* Takes an arbitrary message and returns a FSQ channel symbol table.
*
* from_call - Callsign from which message is directed (maximum size: 20)
* to_call - Callsign to which message is directed (maximum size: 20)
* cmd - Directed command
* message - Null-terminated message string, no greater than 100 chars in length
* symbols - Array of channel symbols to transmit returned by the method.
*  Ensure that you pass a uint8_t array of at least the size of the message
*  plus 5 characters to the method. Terminated in 0xFF.
*
*/
```

Tokens
------
Here are the defines, structs, and enumerations you will find handy to use with the library.

Defines:

    JT65_SYMBOL_COUNT, JT9_SYMBOL_COUNT, JT4_SYMBOL_COUNT, WSPR_SYMBOL_COUNT, FT8_SYMBOL_COUNT

Acknowledgements
----------------
Many thanks to Joe Taylor K1JT for his innovative work in amateur radio. We are lucky to have him. The algorithms in this program were derived from the source code in the [WSJT-X](https://sourceforge.net/p/wsjt/) suite of applications. Also, many thanks for Andy Talbot G4JNT for [his paper](http://www.g4jnt.com/JTModesBcns.htm) on the WSPR coding protocol, which helped me to understand the WSPR encoding process, which in turn helped me to understand the related JT protocols.

Also, a big thank you to Murray Greenman, ZL1BPU for working allowing me to pick his brain regarding his neat new mode FSQ.

Changelog
---------
* v1.2.0

    * Add support for FT8 protocol (79 symbol version introduced December 2018)

* v1.1.3

    * Add support for ESP8266
    * Fix WSPR regression in last release

* v1.1.2

    * Fix buffer bug in _jt_message_prep()_ that caused messages of 11 chars to lock up the processor
    * Made a handful of changes to make the library more friendly to ATmegaxx8 processors
    * Rewrote example sketch to be generically compatible with most Arduino platforms

* v1.1.1

    * Update example sketch for Si5351Arduino v2.0.0

* v1.1.0

    * Added FSQ

* v1.0.1

    * Fixed a bug in _jt65_interleave()_ that was causing a buffer overrun.

* v1.0.0

    * Initial Release


License
-------
JTEncode is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

JTEncode is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with JTEncode.  If not, see <http://www.gnu.org/licenses/>.
