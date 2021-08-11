/*
 * JTEncode.cpp - JT65/JT9/WSPR/FSQ encoder library for Arduino
 *
 * Copyright (C) 2015-2018 Jason Milldrum <milldrum@gmail.com>
 *
 * Based on the algorithms presented in the WSJT software suite.
 * Thanks to Andy Talbot G4JNT for the whitepaper on the WSPR encoding
 * process that helped me to understand all of this.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "JTEncode.h"
#include "crc14.h"
#include "generator.h"

#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

// Define an upper bound on the number of glyphs.  Defining it this
// way allows adding characters without having to update a hard-coded
// upper bound.
#define NGLYPHS         (sizeof(fsq_code_table)/sizeof(fsq_code_table[0]))

/* Public Class Members */

JTEncode::JTEncode(void)
{
  // Initialize the Reed-Solomon encoder
  rs_inst = (struct rs *) init_rs_int(6, 0x43, 3, 1, 51, 0);
}

JTEncode::~JTEncode()
{
    free_rs_int(rs_inst);
}

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
uint8_t jt65_buffer[JT65_ENCODE_COUNT];

void JTEncode::jt65_encode(const char * msg, uint8_t * symbols)
{
  char message[14];
  memset(message, 0, 14);
  strcpy(message, msg);

  // Ensure that the message text conforms to standards
  // --------------------------------------------------
  jt_message_prep(message);

  // Bit packing
  // -----------
  uint8_t c[12];
  jt65_bit_packing(message, c);

  // Reed-Solomon encoding
  // ---------------------
  rs_encode(c, jt65_buffer);

  // Interleaving
  // ------------
  jt65_interleave(jt65_buffer);

  // Gray Code
  // ---------
  jt_gray_code(jt65_buffer, JT65_ENCODE_COUNT);

  // Merge with sync vector
  // ----------------------
  jt65_merge_sync_vector(jt65_buffer, symbols);
}

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
uint8_t jt9_buffer_s[JT9_BIT_COUNT];
uint8_t jt9_buffer_a[JT9_ENCODE_COUNT];
void JTEncode::jt9_encode(const char * msg, uint8_t * symbols)
{
  char message[14];
  memset(message, 0, 14);
  strcpy(message, msg);

  // Ensure that the message text conforms to standards
  // --------------------------------------------------
  jt_message_prep(message);

  // Bit packing
  // -----------
  uint8_t c[13];
  jt9_bit_packing(message, c);

  // Convolutional Encoding
  // ---------------------
  convolve(c, jt9_buffer_s, 13, JT9_BIT_COUNT);

  // Interleaving
  // ------------
  jt9_interleave(jt9_buffer_s);

  // Pack into 3-bit symbols
  // -----------------------
  jt9_packbits(jt9_buffer_s, jt9_buffer_a);

  // Gray Code
  // ---------
  jt_gray_code(jt9_buffer_a, JT9_ENCODE_COUNT);

  // Merge with sync vector
  // ----------------------
  jt9_merge_sync_vector(jt9_buffer_a, symbols);
}

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
uint8_t jt4_buffer_s[JT4_SYMBOL_COUNT];
void JTEncode::jt4_encode(const char * msg, uint8_t * symbols)
{
  char message[14];
  memset(message, 0, 14);
  strcpy(message, msg);

  // Ensure that the message text conforms to standards
  // --------------------------------------------------
  jt_message_prep(message);

  // Bit packing
  // -----------
  uint8_t c[13];
  jt9_bit_packing(message, c);

  // Convolutional Encoding
  // ---------------------
  convolve(c, jt4_buffer_s, 13, JT4_BIT_COUNT);

  // Interleaving
  // ------------
  jt9_interleave(jt4_buffer_s);
  memmove(jt4_buffer_s + 1, jt4_buffer_s, JT4_BIT_COUNT);
    jt4_buffer_s[0] = 0; // Append a 0 bit to start of sequence

  // Merge with sync vector
  // ----------------------
  jt4_merge_sync_vector(jt4_buffer_s, symbols);
}

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
uint8_t wspr_buffer_s[WSPR_SYMBOL_COUNT];
void JTEncode::wspr_encode(const char * call, const char * loc, const uint8_t dbm, uint8_t * symbols)
{
  char call_[7];
  char loc_[5];
  uint8_t dbm_ = dbm;
  strcpy(call_, call);
  strcpy(loc_, loc);

  // Ensure that the message text conforms to standards
  // --------------------------------------------------
  wspr_message_prep(call_, loc_, dbm_);

  // Bit packing
  // -----------
  uint8_t c[11];
  wspr_bit_packing(c);

  // Convolutional Encoding
  // ---------------------
  convolve(c, wspr_buffer_s, 11, WSPR_BIT_COUNT);

  // Interleaving
  // ------------
  wspr_interleave(wspr_buffer_s);

  // Merge with sync vector
  // ----------------------
  wspr_merge_sync_vector(wspr_buffer_s, symbols);
}

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
char fsq_tx_buffer[155];
void JTEncode::fsq_encode(const char * from_call, const char * message, uint8_t * symbols)
{
  char * tx_message;
  uint16_t symbol_pos = 0;
  uint8_t i, fch, vcode1, vcode2;
  uint8_t cur_tone = 0;

  // Clear out the transmit buffer
  // -----------------------------
  memset(fsq_tx_buffer, 0, 155);

  // Create the message to be transmitted
  // ------------------------------------
  sprintf(fsq_tx_buffer, "  \n%s: %s", from_call, message);

  tx_message = fsq_tx_buffer;

  // Iterate through the message and encode
  // --------------------------------------
  while(*tx_message != '\0')
  {
    for(i = 0; i < NGLYPHS; i++)
    {
      uint8_t ch = (uint8_t)*tx_message;

      // Check each element of the varicode table to see if we've found the
      // character we're trying to send.
      fch = fsq_code_table[i].ch;

      if(fch == ch)
      {
          // Found the character, now fetch the varicode chars
          vcode1 = fsq_code_table[i].var[0];
          vcode2 = fsq_code_table[i].var[1];

          // Transmit the appropriate tone per a varicode char
          if(vcode2 == 0)
          {
            // If the 2nd varicode char is a 0 in the table,
            // we are transmitting a lowercase character, and thus
            // only transmit one tone for this character.

            // Generate tone
            cur_tone = ((cur_tone + vcode1 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;
          }
          else
          {
            // If the 2nd varicode char is anything other than 0 in
            // the table, then we need to transmit both

            // Generate 1st tone
            cur_tone = ((cur_tone + vcode1 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;

            // Generate 2nd tone
            cur_tone = ((cur_tone + vcode2 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;
          }
          break; // We've found and transmitted the char,
             // so exit the for loop
        }
    }

    tx_message++;
  }

  // Message termination
  // ----------------
  symbols[symbol_pos] = 0xff;
}

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
char fsq_dir_tx_buffer[155];
void JTEncode::fsq_dir_encode(const char * from_call, const char * to_call, const char cmd, const char * message, uint8_t * symbols)
{
  char * tx_message;
  uint16_t symbol_pos = 0;
  uint8_t i, fch, vcode1, vcode2, from_call_crc;
  uint8_t cur_tone = 0;

  // Generate a CRC on from_call
  // ---------------------------
  from_call_crc = crc8(from_call);

  // Clear out the transmit buffer
  // -----------------------------
  memset(fsq_dir_tx_buffer, 0, 155);

  // Create the message to be transmitted
  // We are building a directed message here.
  // FSQ very specifically needs "  \b  " in
  // directed mode to indicate EOT. A single backspace won't do it.
  sprintf(fsq_dir_tx_buffer, "  \n%s:%02x%s%c%s%s", from_call, from_call_crc, to_call, cmd, message, "  \b  ");

  tx_message = fsq_dir_tx_buffer;

  // Iterate through the message and encode
  // --------------------------------------
  while(*tx_message != '\0')
  {
    for(i = 0; i < NGLYPHS; i++)
    {
      uint8_t ch = (uint8_t)*tx_message;

      // Check each element of the varicode table to see if we've found the
      // character we're trying to send.
      fch = fsq_code_table[i].ch;

      if(fch == ch)
      {
          // Found the character, now fetch the varicode chars
          vcode1 = fsq_code_table[i].var[0];
          vcode2 = fsq_code_table[i].var[1];

          // Transmit the appropriate tone per a varicode char
          if(vcode2 == 0)
          {
            // If the 2nd varicode char is a 0 in the table,
            // we are transmitting a lowercase character, and thus
            // only transmit one tone for this character.

            // Generate tone
            cur_tone = ((cur_tone + vcode1 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;
          }
          else
          {
            // If the 2nd varicode char is anything other than 0 in
            // the table, then we need to transmit both

            // Generate 1st tone
            cur_tone = ((cur_tone + vcode1 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;

            // Generate 2nd tone
            cur_tone = ((cur_tone + vcode2 + 1) % 33);
            symbols[symbol_pos++] = cur_tone;
          }
          break; // We've found and transmitted the char,
             // so exit the for loop
        }
    }

    tx_message++;
  }

  // Message termination
  // ----------------
  symbols[symbol_pos] = 0xff;
}

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
uint8_t ft8_buffer_s[FT8_BIT_COUNT];
void JTEncode::ft8_encode(const char * msg, uint8_t * symbols)
{
  char message[19];
  memset(message, 0, 19);
  strcpy(message, msg);

  // Bit packing
  // -----------
  uint8_t c[77];
  memset(c, 0, 77);
  ft8_bit_packing(message, c);

  // Message Encoding
  // ----------------
  ft8_encode(c, ft8_buffer_s);

  // Merge with sync vector
  // ----------------------
  ft8_merge_sync_vector(ft8_buffer_s, symbols);
}

/* Private Class Members */

uint8_t JTEncode::jt_code(char c)
{
  // Validate the input then return the proper integer code.
  // Return 255 as an error code if the char is not allowed.

  if(isdigit(c))
  {
    return (uint8_t)(c - 48);
  }
  else if(c >= 'A' && c <= 'Z')
  {
    return (uint8_t)(c - 55);
  }
  else if(c == ' ')
  {
    return 36;
  }
  else if(c == '+')
  {
    return 37;
  }
  else if(c == '-')
  {
    return 38;
  }
  else if(c == '.')
  {
    return 39;
  }
  else if(c == '/')
  {
    return 40;
  }
  else if(c == '?')
  {
    return 41;
  }
  else
  {
    return 255;
  }
}

uint8_t JTEncode::ft_code(char c)
{
	/* Validate the input then return the proper integer code */
	// Return 255 as an error code if the char is not allowed

	if(isdigit(c))
	{
		return (uint8_t)(c) - 47;
	}
	else if(c >= 'A' && c <= 'Z')
	{
		return (uint8_t)(c) - 54;
	}
	else if(c == ' ')
	{
		return 0;
	}
  else if(c == '+')
	{
		return 37;
	}
	else if(c == '-')
	{
		return 38;
	}
	else if(c == '.')
	{
		return 39;
	}
	else if(c == '/')
	{
		return 40;
	}
	else if(c == '?')
	{
		return 41;
	}
	else
	{
		return 255;
	}
}

uint8_t JTEncode::wspr_code(char c)
{
  // Validate the input then return the proper integer code.
  // Return 255 as an error code if the char is not allowed.

  if(isdigit(c))
	{
		return (uint8_t)(c - 48);
	}
	else if(c == ' ')
	{
		return 36;
	}
	else if(c >= 'A' && c <= 'Z')
	{
		return (uint8_t)(c - 55);
	}
	else
	{
		return 255;
	}
}

uint8_t JTEncode::gray_code(uint8_t c)
{
  return (c >> 1) ^ c;
}

int8_t JTEncode::hex2int(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

void JTEncode::jt_message_prep(char * message)
{
  uint8_t i;

  // Pad the message with trailing spaces
  uint8_t len = strlen(message);
  if(len < 13)
  {
    for(i = len; i <= 13; i++)
    {
      message[i] = ' ';
    }
  }

  // Convert all chars to uppercase
  for(i = 0; i < 13; i++)
  {
    if(islower(message[i]))
    {
      message[i] = toupper(message[i]);
    }
  }
}

void JTEncode::ft_message_prep(char * message)
{
  uint8_t i;
  char temp_msg[14];

  snprintf(temp_msg, 14, "%13s", message);

  // Convert all chars to uppercase
  for(i = 0; i < 13; i++)
  {
    if(islower(temp_msg[i]))
    {
      temp_msg[i] = toupper(temp_msg[i]);
    }
  }

  strcpy(message, temp_msg);
}

void JTEncode::wspr_message_prep(char * call, char * loc, uint8_t dbm)
{
  // Callsign validation and padding
  // -------------------------------

	// If only the 2nd character is a digit, then pad with a space.
	// If this happens, then the callsign will be truncated if it is
	// longer than 5 characters.
	if((call[1] >= '0' && call[1] <= '9') && (call[2] < '0' || call[2] > '9'))
	{
		memmove(call + 1, call, 5);
		call[0] = ' ';
	}

	// Now the 3rd charcter in the callsign must be a digit
	if(call[2] < '0' || call[2] > '9')
	{
    // TODO: need a better way to handle this
		call[2] = '0';
	}

	// Ensure that the only allowed characters are digits and
	// uppercase letters
	uint8_t i;
	for(i = 0; i < 6; i++)
	{
		call[i] = toupper(call[i]);
		if(!(isdigit(call[i]) || isupper(call[i])))
		{
			call[i] = ' ';
		}
	}

  memcpy(callsign, call, 6);

	// Grid locator validation
	for(i = 0; i < 4; i++)
	{
		loc[i] = toupper(loc[i]);
		if(!(isdigit(loc[i]) || (loc[i] >= 'A' && loc[i] <= 'R')))
		{
      memcpy(loc, "AA00", 5);
      //loc = "AA00";
		}
	}

  memcpy(locator, loc, 4);

	// Power level validation
	// Only certain increments are allowed
	if(dbm > 60)
	{
		dbm = 60;
	}
  const uint8_t valid_dbm[19] =
    {0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40,
     43, 47, 50, 53, 57, 60};
  for(i = 0; i < 19; i++)
  {
    if(dbm == valid_dbm[i])
    {
      power = dbm;
    }
  }
  // If we got this far, we have an invalid power level, so we'll round down
  for(i = 1; i < 19; i++)
  {
    if(dbm < valid_dbm[i] && dbm >= valid_dbm[i - 1])
    {
      power = valid_dbm[i - 1];
    }
  }
}

void JTEncode::jt65_bit_packing(char * message, uint8_t * c)
{
  uint32_t n1, n2, n3;

  // Find the N values
  n1 = jt_code(message[0]);
  n1 = n1 * 42 + jt_code(message[1]);
  n1 = n1 * 42 + jt_code(message[2]);
  n1 = n1 * 42 + jt_code(message[3]);
  n1 = n1 * 42 + jt_code(message[4]);

  n2 = jt_code(message[5]);
  n2 = n2 * 42 + jt_code(message[6]);
  n2 = n2 * 42 + jt_code(message[7]);
  n2 = n2 * 42 + jt_code(message[8]);
  n2 = n2 * 42 + jt_code(message[9]);

  n3 = jt_code(message[10]);
  n3 = n3 * 42 + jt_code(message[11]);
  n3 = n3 * 42 + jt_code(message[12]);

  // Pack bits 15 and 16 of N3 into N1 and N2,
  // then mask reset of N3 bits
  n1 = (n1 << 1) + ((n3 >> 15) & 1);
  n2 = (n2 << 1) + ((n3 >> 16) & 1);
  n3 = n3 & 0x7fff;

  // Set the freeform message flag
  n3 += 32768;

  c[0] = (n1 >> 22) & 0x003f;
  c[1] = (n1 >> 16) & 0x003f;
  c[2] = (n1 >> 10) & 0x003f;
  c[3] = (n1 >> 4) & 0x003f;
  c[4] = ((n1 & 0x000f) << 2) + ((n2 >> 26) & 0x0003);
  c[5] = (n2 >> 20) & 0x003f;
  c[6] = (n2 >> 14) & 0x003f;
  c[7] = (n2 >> 8) & 0x003f;
  c[8] = (n2 >> 2) & 0x003f;
  c[9] = ((n2 & 0x0003) << 4) + ((n3 >> 12) & 0x000f);
  c[10] = (n3 >> 6) & 0x003f;
  c[11] = n3 & 0x003f;
}

void JTEncode::jt9_bit_packing(char * message, uint8_t * c)
{
  uint32_t n1, n2, n3;

  // Find the N values
  n1 = jt_code(message[0]);
  n1 = n1 * 42 + jt_code(message[1]);
  n1 = n1 * 42 + jt_code(message[2]);
  n1 = n1 * 42 + jt_code(message[3]);
  n1 = n1 * 42 + jt_code(message[4]);

  n2 = jt_code(message[5]);
  n2 = n2 * 42 + jt_code(message[6]);
  n2 = n2 * 42 + jt_code(message[7]);
  n2 = n2 * 42 + jt_code(message[8]);
  n2 = n2 * 42 + jt_code(message[9]);

  n3 = jt_code(message[10]);
  n3 = n3 * 42 + jt_code(message[11]);
  n3 = n3 * 42 + jt_code(message[12]);

  // Pack bits 15 and 16 of N3 into N1 and N2,
  // then mask reset of N3 bits
  n1 = (n1 << 1) + ((n3 >> 15) & 1);
  n2 = (n2 << 1) + ((n3 >> 16) & 1);
  n3 = n3 & 0x7fff;

  // Set the freeform message flag
  n3 += 32768;

  // 71 message bits to pack, plus 1 bit flag for freeform message.
  // 31 zero bits appended to end.
  // N1 and N2 are 28 bits each, N3 is 16 bits
  // A little less work to start with the least-significant bits
  c[3] = (uint8_t)((n1 & 0x0f) << 4);
  n1 = n1 >> 4;
  c[2] = (uint8_t)(n1 & 0xff);
  n1 = n1 >> 8;
  c[1] = (uint8_t)(n1 & 0xff);
  n1 = n1 >> 8;
  c[0] = (uint8_t)(n1 & 0xff);

  c[6] = (uint8_t)(n2 & 0xff);
  n2 = n2 >> 8;
  c[5] = (uint8_t)(n2 & 0xff);
  n2 = n2 >> 8;
  c[4] = (uint8_t)(n2 & 0xff);
  n2 = n2 >> 8;
  c[3] |= (uint8_t)(n2 & 0x0f);

  c[8] = (uint8_t)(n3 & 0xff);
  n3 = n3 >> 8;
  c[7] = (uint8_t)(n3 & 0xff);

  c[9] = 0;
  c[10] = 0;
  c[11] = 0;
  c[12] = 0;
}

void JTEncode::wspr_bit_packing(uint8_t * c)
{
  uint32_t n, m;

	n = wspr_code(callsign[0]);
	n = n * 36 + wspr_code(callsign[1]);
	n = n * 10 + wspr_code(callsign[2]);
	n = n * 27 + (wspr_code(callsign[3]) - 10);
	n = n * 27 + (wspr_code(callsign[4]) - 10);
	n = n * 27 + (wspr_code(callsign[5]) - 10);

	m = ((179 - 10 * (locator[0] - 'A') - (locator[2] - '0')) * 180) +
		(10 * (locator[1] - 'A')) + (locator[3] - '0');
	m = (m * 128) + power + 64;

	// Callsign is 28 bits, locator/power is 22 bits.
	// A little less work to start with the least-significant bits
	c[3] = (uint8_t)((n & 0x0f) << 4);
	n = n >> 4;
	c[2] = (uint8_t)(n & 0xff);
	n = n >> 8;
	c[1] = (uint8_t)(n & 0xff);
	n = n >> 8;
	c[0] = (uint8_t)(n & 0xff);

	c[6] = (uint8_t)((m & 0x03) << 6);
	m = m >> 2;
	c[5] = (uint8_t)(m & 0xff);
	m = m >> 8;
	c[4] = (uint8_t)(m & 0xff);
	m = m >> 8;
	c[3] |= (uint8_t)(m & 0x0f);
	c[7] = 0;
	c[8] = 0;
	c[9] = 0;
	c[10] = 0;
}

void JTEncode::ft8_bit_packing(char* message, uint8_t* codeword)
{
    // Just encoding type 0 free text and type 0.5 telemetry for now

	// The bit packing algorithm is:
	// sum(message(pos) * 42^pos)

	uint8_t i3 = 0;
	uint8_t n3 = 0;
	uint8_t qa[10];
	uint8_t qb[10];
	char c18[19];
	bool telem = false;
	char temp_msg[19];
	memset(qa, 0, 10);
	memset(qb, 0, 10);

	uint8_t i, j, x, i0 = 0;
	uint32_t ireg = 0;

	// See if this is a telemetry message
	// Has to be hex digits, can be no more than 18
	for(i = 0; i < 19; ++i)
	{
		if(message[i] == 0 || message[i] == ' ')
		{
			break;
		}
		else if(hex2int(message[i]) == -1)
		{
			telem = false;
			break;
		}
		else
		{
			c18[i] = message[i];
			telem = true;
		}
	}

	// If telemetry
	if(telem)
	{
		// Get the first 18 hex digits
		for(i = 0; i < strlen(message); ++i)
		{
			i0 = i;
			if(message[i] == ' ')
			{
				--i0;
				break;
			}
		}

		memset(c18, 0, 19);
		memmove(c18, message, i0 + 1);
		snprintf(temp_msg, 19, "%*s", 18, c18);

		// Convert all chars to uppercase
	    for(i = 0; i < strlen(temp_msg); i++)
	    {
	      if(islower(temp_msg[i]))
	      {
	        temp_msg[i] = toupper(temp_msg[i]);
	      }
	    }
		strcpy(message, temp_msg);


		uint8_t temp_int;
		temp_int = message[0] == ' ' ? 0 : hex2int(message[0]);
		for(i = 1; i < 4; ++i)
		{
			codeword[i - 1] = (((temp_int << i) & 0x8) >> 3) & 1;
		}
		temp_int = message[1] == ' ' ? 0 : hex2int(message[1]);
		for(i = 0; i < 4; ++i)
		{
			codeword[i + 3] = (((temp_int << i) & 0x8) >> 3) & 1;
		}
		for(i = 0; i < 8; ++i)
		{
			if(message[2 * i + 2] == ' ')
			{
				temp_int = 0;
			}
			else
			{
				temp_int = hex2int(message[2 * i + 2]);
			}
			for(j = 0; j < 4; ++j)
			{
				codeword[(i + 1) * 8 + j - 1] = (((temp_int << j) & 0x8) >> 3) & 1;
			}
			if(message[2 * i + 3] == ' ')
			{
				temp_int = 0;
			}
			else
			{
				temp_int = hex2int(message[2 * i + 3]);
			}
			for(j = 0; j < 4; ++j)
			{
				codeword[(i + 1) * 8 + j + 3] = (((temp_int << j) & 0x8) >> 3) & 1;
			}
		}

		i3 = 0;
		n3 = 5;
	}
	else
	{
		ft_message_prep(message);

		for(i = 0; i < 13; ++i)
		{
			x = ft_code(message[i]);

			// mult
			ireg = 0;
			for(j = 0; j < 9; ++j)
			{
				ireg = (uint8_t)qa[j] * 42 + (uint8_t)((ireg >> 8) & 0xff);
				qb[j] = (uint8_t)(ireg & 0xff);
			}
			qb[9] = (uint8_t)((ireg >> 8) & 0xff);

			// add
			ireg = x << 8;
			for(j = 0; j < 9; ++j)
			{
				ireg = (uint8_t)qb[j] + (uint8_t)((ireg >> 8) & 0xff);
				qa[j] = (uint8_t)(ireg & 0xff);
			}
			qa[9] = (uint8_t)((ireg >> 8) & 0xff);
		}

		// Format bits to output array
		for(i = 1; i < 8; ++i)
		{
			codeword[i - 1] = (((qa[8] << i) & 0x80) >> 7) & 1;
		}
		for(i = 0; i < 8; ++i)
		{
			for(j = 0; j < 8; ++j)
			{
				codeword[(i + 1) * 8 + j - 1] = (((qa[7 - i] << j) & 0x80) >> 7) & 1;
			}
		}
	}

	// Write the message type bits at the end of the array
	for(i = 0; i < 3; ++i)
	{
		codeword[i + 71] = (n3 >> i) & 1;
	}
	for(i = 0; i < 3; ++i)
	{
		codeword[i + 74] = (i3 >> i) & 1;
	}
}

uint8_t jt65_buffer_d[JT65_ENCODE_COUNT];
void JTEncode::jt65_interleave(uint8_t * s)
{
  uint8_t i, j;

  // Interleave
  for(i = 0; i < 9; i++)
  {
    for(j = 0; j < 7; j++)
    {
        jt65_buffer_d[(j * 9) + i] = s[(i * 7) + j];
    }
  }

  memcpy(s, jt65_buffer_d, JT65_ENCODE_COUNT);
}

uint8_t jt9_buffer_d[JT9_BIT_COUNT];
void JTEncode::jt9_interleave(uint8_t * s)
{
  uint8_t i;

  // Do the interleave
  for(i = 0; i < JT9_BIT_COUNT; i++)
  {
    //#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
    #if defined(__arm__)
      jt9_buffer_d[jt9i[i]] = s[i];
    #else
      uint8_t j;
      j = pgm_read_byte(&jt9i[i]);
      jt9_buffer_d[j] = s[i];
    #endif
  }

  memcpy(s, jt9_buffer_d, JT9_BIT_COUNT);
}

uint8_t wspr_buffer_d[WSPR_BIT_COUNT];
void JTEncode::wspr_interleave(uint8_t * s)
{
	uint8_t rev, index_temp, i, j, k;

	i = 0;

	for(j = 0; j < 255; j++)
	{
		// Bit reverse the index
		index_temp = j;
		rev = 0;

		for(k = 0; k < 8; k++)
		{
			if(index_temp & 0x01)
			{
				rev = rev | (1 << (7 - k));
			}
			index_temp = index_temp >> 1;
		}

		if(rev < WSPR_BIT_COUNT)
		{
            wspr_buffer_d[rev] = s[i];
			i++;
		}

		if(i >= WSPR_BIT_COUNT)
		{
			break;
		}
	}

  memcpy(s, wspr_buffer_d, WSPR_BIT_COUNT);
}

void JTEncode::jt9_packbits(uint8_t * d, uint8_t * a)
{
  uint8_t i, k;
  k = 0;
  memset(a, 0, JT9_ENCODE_COUNT);

  for(i = 0; i < JT9_ENCODE_COUNT; i++)
  {
    a[i] = (d[k] & 1) << 2;
    k++;

    a[i] |= ((d[k] & 1) << 1);
    k++;

    a[i] |= (d[k] & 1);
    k++;
  }
}

void JTEncode::jt_gray_code(uint8_t * g, uint8_t symbol_count)
{
  uint8_t i;

  for(i = 0; i < symbol_count; i++)
  {
    g[i] = gray_code(g[i]);
  }
}

void JTEncode::ft8_encode(uint8_t* codeword, uint8_t* symbols)
{
	const uint8_t FT8_N = 174;
	const uint8_t FT8_K = 91;
	const uint8_t FT8_M = FT8_N - FT8_K;

	uint8_t tempchar[FT8_K];
	uint8_t message91[FT8_K];
	uint8_t pchecks[FT8_M];
	uint8_t i1_msg_bytes[12];
	uint8_t i, j;
	uint16_t ncrc14;

	crc_t crc;
	crc_cfg_t crc_cfg;
	crc_cfg.reflect_in = 0;
	crc_cfg.xor_in = 0;
	crc_cfg.reflect_out = 0;
	crc_cfg.xor_out = 0;
	crc = crc_init(&crc_cfg);

	// Add 14-bit CRC to form 91-bit message
	memset(tempchar, 0, 91);
	memcpy(tempchar, codeword, 77);
	tempchar[77] = 0;
	tempchar[78] = 0;
	tempchar[79] = 0;
	memset(i1_msg_bytes, 0, 12);
	for(i = 0; i < 10; ++i)
	{
		for(j = 0; j < 8; ++j)
		{
			i1_msg_bytes[i] <<= 1;
			i1_msg_bytes[i] |= tempchar[i * 8 + j];
		}
	}

	ncrc14 = crc_update(&crc_cfg, crc, (unsigned char *)i1_msg_bytes, 12);
	crc = crc_finalize(&crc_cfg, crc);

	for(i = 0; i < 14; ++i)
	{
		if((((ncrc14 << (i + 2)) & 0x8000) >> 15) & 1)
		{
			tempchar[i + 77] = 1;
		}
		else
		{
			tempchar[i + 77] = 0;
		}
	}
	memcpy(message91, tempchar, 91);

	for(i = 0; i < FT8_M; ++i)
	{
		uint32_t nsum = 0;
		for(j = 0; j < FT8_K; ++j)
		{
      #if defined(__arm__)
      uint8_t bits = generator_bits[i][j / 8];
      #else
      uint8_t bits = pgm_read_byte(&(generator_bits[i][j / 8]));
      #endif
			bits <<= (j % 8);
			bits &= 0x80;
			bits >>= 7;
			bits &= 1;
			nsum += (message91[j] * bits);
		}
		pchecks[i] = nsum % 2;
	}

	memcpy(symbols, message91, FT8_K);
	memcpy(symbols + FT8_K, pchecks, FT8_M);
}

const uint8_t jt65_sync_vector[JT65_SYMBOL_COUNT] =
        {1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0,
         0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
         0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1,
         0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
         1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1,
         0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1,
         1, 1, 1, 1, 1, 1};

void JTEncode::jt65_merge_sync_vector(uint8_t * g, uint8_t * symbols)
{
  uint8_t i, j = 0;

  for(i = 0; i < JT65_SYMBOL_COUNT; i++)
  {
    if(jt65_sync_vector[i])
    {
      symbols[i] = 0;
    }
    else
    {
      symbols[i] = g[j] + 2;
      j++;
    }
  }
}

const uint8_t jt9_sync_vector[JT9_SYMBOL_COUNT] =
        {1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
         0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1,
         0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 1, 0, 1};
void JTEncode::jt9_merge_sync_vector(uint8_t * g, uint8_t * symbols)
{
  uint8_t i, j = 0;

  for(i = 0; i < JT9_SYMBOL_COUNT; i++)
  {
    if(jt9_sync_vector[i])
    {
      symbols[i] = 0;
    }
    else
    {
      symbols[i] = g[j] + 1;
      j++;
    }
  }
}

void JTEncode::jt4_merge_sync_vector(uint8_t * g, uint8_t * symbols)
{
  uint8_t i;
  const uint8_t sync_vector[JT4_SYMBOL_COUNT] =
	{0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0,
   0, 0, 0, 0, 1, 1, 0, 0, 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,1 ,0 ,1 ,1,
   0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0,
   1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0,
   0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0,
   1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1,
   1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1,
   0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1,
   1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1,
   0, 1, 1, 1, 1, 0, 1, 0, 1};

	for(i = 0; i < JT4_SYMBOL_COUNT; i++)
	{
		symbols[i] = sync_vector[i] + (2 * g[i]);
	}
}

const uint8_t wspr_sync_vector[WSPR_SYMBOL_COUNT] =
        {1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,
         1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
         0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
         0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
         1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
         0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
         1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
         1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0};
void JTEncode::wspr_merge_sync_vector(uint8_t * g, uint8_t * symbols)
{
  uint8_t i;

	for(i = 0; i < WSPR_SYMBOL_COUNT; i++)
	{
		symbols[i] = wspr_sync_vector[i] + (2 * g[i]);
	}
}

void JTEncode::ft8_merge_sync_vector(uint8_t* symbols, uint8_t* output)
{
	const uint8_t costas7x7[7] = {3, 1, 4, 0, 6, 5, 2};
	const uint8_t graymap[8] = {0, 1, 3, 2, 5, 6, 4, 7};
	uint8_t i, j, k, idx;

	// Insert Costas sync arrays
	memcpy(output, costas7x7, 7);
	memcpy(output + 36, costas7x7, 7);
	memcpy(output + FT8_SYMBOL_COUNT - 7, costas7x7, 7);

	k = 6;
	for(j = 0; j < 58; ++j) // 58 data symbols
	{
		i = 3 * j;
		++k;
		if(j == 29)
		{
			k += 7;
		}
		idx = symbols[i] * 4 + symbols[i + 1] * 2 + symbols[i + 2];
		output[k] = graymap[idx];
	}
}

void JTEncode::convolve(uint8_t * c, uint8_t * s, uint8_t message_size, uint8_t bit_size)
{
  uint32_t reg_0 = 0;
  uint32_t reg_1 = 0;
  uint32_t reg_temp = 0;
  uint8_t input_bit, parity_bit;
  uint8_t bit_count = 0;
  uint8_t i, j, k;

  for(i = 0; i < message_size; i++)
  {
    for(j = 0; j < 8; j++)
    {
      // Set input bit according the MSB of current element
      input_bit = (((c[i] << j) & 0x80) == 0x80) ? 1 : 0;

      // Shift both registers and put in the new input bit
      reg_0 = reg_0 << 1;
      reg_1 = reg_1 << 1;
      reg_0 |= (uint32_t)input_bit;
      reg_1 |= (uint32_t)input_bit;

      // AND Register 0 with feedback taps, calculate parity
      reg_temp = reg_0 & 0xf2d05351;
      parity_bit = 0;
      for(k = 0; k < 32; k++)
      {
        parity_bit = parity_bit ^ (reg_temp & 0x01);
        reg_temp = reg_temp >> 1;
      }
      s[bit_count] = parity_bit;
      bit_count++;

      // AND Register 1 with feedback taps, calculate parity
      reg_temp = reg_1 & 0xe4613c47;
      parity_bit = 0;
      for(k = 0; k < 32; k++)
      {
        parity_bit = parity_bit ^ (reg_temp & 0x01);
        reg_temp = reg_temp >> 1;
      }
      s[bit_count] = parity_bit;
      bit_count++;
      if(bit_count >= bit_size)
      {
        break;
      }
    }
  }
}

void JTEncode::rs_encode(uint8_t * data, uint8_t * symbols)
{
  // Adapted from wrapkarn.c in the WSJT-X source code
  uint8_t dat1[12];
  uint8_t b[51];
  uint8_t sym[JT65_ENCODE_COUNT];
  uint8_t i;

  // Reverse data order for the Karn codec.
  for(i = 0; i < 12; i++)
  {
    dat1[i] = data[11 - i];
  }

  // Compute the parity symbols
  encode_rs_int(rs_inst, dat1, b);

  // Move parity symbols and data into symbols array, in reverse order.
  for (i = 0; i < 51; i++)
  {
    sym[50 - i] = b[i];
  }

  for (i = 0; i < 12; i++)
  {
    sym[i + 51] = dat1[11 - i];
  }

  memcpy(symbols, sym, JT65_ENCODE_COUNT);
}

uint8_t JTEncode::crc8(const char * text)
{
  uint8_t crc = '\0';
  uint8_t ch;
  uint8_t len = (uint8_t) strlen(text);

  int i;
  for(i = 0; i < len; i++)
  {
    ch = text[i];
    //#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
    #if defined(__arm__)
    crc = crc8_table[(crc) ^ ch];
    #else
    crc = pgm_read_byte(&(crc8_table[(crc) ^ ch]));
    #endif
    crc &= 0xFF;
  }

  return crc;
}
