//  Morse Code Playback Functions
//  Mark Jessop 2018-04
//  OK1TE 2018-11
//
//  Based on code from https://github.com/Paradoxis/Arduino-morse-code-translator/blob/master/main.ino
//
#include "morse.h"
#include "config.h"

// All morse delays
#define MORSE_DELAY (1200 / MORSE_WPM)
#define MORSE_DELAY_DOT (MORSE_DELAY * 1)
#define MORSE_DELAY_DASH (MORSE_DELAY * 3)
#define MORSE_DELAY_SPACE (MORSE_DELAY * 7)

// All morse characters
const char MORSE_DOT = '.';
const char MORSE_DASH = '-';

// Letters
const char* const MORSE_LETTERS[] = {
  ".-",     // A
  "-...",   // B
  "-.-.",   // C
  "-..",    // D
  ".",      // E
  "..-.",   // F
  "--.",    // G
  "....",   // H
  "..",     // I
  ".---",   // J
  "-.-",    // K
  ".-..",   // L
  "--",     // M
  "-.",     // N
  "---",    // O
  ".--.",   // P
  "--.-",   // Q
  ".-.",    // R
  "...",    // S
  "-",      // T
  "..-",    // U
  "...-",   // V
  ".--",    // W
  "-..-",   // X
  "-.--",   // Y
  "--.."    // Z
};

// Numerals.
const char* const MORSE_NUMBERS[] = {
  "-----",   // 0
  ".----",   // 1
  "..---",   // 2
  "...--",   // 3
  "....-",   // 4
  ".....",   // 5
  "-....",   // 6
  "--...",   // 7
  "---..",   // 8
  "----."    // 9
};

// Symbols
const char Morse_Slash[] = "-..-.";
const char Morse_Equal[] = "-...-";
const char Morse_FullStop[] = ".-.-.-";
const char Morse_Comma[] = "--..--";
const char Morse_QuestionMark[] = "..--..";
const char Morse_Plus[] = ".-.-.";
const char Morse_AtSign[] = ".--.-.";

// Send a single character
void sendDotOrDash (char unit) {
  //radio_enable_tx();

  // Unit is a dot (500 ms)
  if (unit == MORSE_DOT) {
    //_delay_ms(MORSE_DELAY_DOT);
  }

  // Unit is a dash (1500 ms)
  else if (unit == MORSE_DASH) {
    //_delay_ms(MORSE_DELAY_DASH);
  }

  // Inter-element gap
  //radio_inhibit_tx();
  //_delay_ms(MORSE_DELAY);
}

void sendMorseSequence (const char* sequence) {
  // Counter
  int i = 0;

  // Loop through every character until an 'end-of-line' (null) character is found
  while (sequence[i] != 0) {
    sendDotOrDash(sequence[i]);
    i++;
  }

  // Delay between every letter
  //_delay_ms(MORSE_DELAY * 3);
}

void sendMorse(const char* message){
  int i = 0;
  while (message[i] != 0){
    const char current = message[i];

    // Lower case letters
      if (current >= 'a' && current <= 'z') {
        sendMorseSequence(MORSE_LETTERS[current - 'a']);
      }

      // Capital case letters
      else if (current >= 'A' && current <= 'Z') {
        sendMorseSequence(MORSE_LETTERS[current - 'A']);
      }

      // Numbers
      else if (current >= '0' && current <= '9') {
        sendMorseSequence(MORSE_NUMBERS[current - '0']);
      }

      else switch (current) {
        case '/': sendMorseSequence(Morse_Slash);
          break;
        case '=': sendMorseSequence(Morse_Equal);
          break;
        case '.': sendMorseSequence(Morse_FullStop);
          break;
        case ',': sendMorseSequence(Morse_Comma);
          break;
        case '?': sendMorseSequence(Morse_QuestionMark);
          break;
        case '+': sendMorseSequence(Morse_Plus);
          break;
        case '@': sendMorseSequence(Morse_AtSign);
          break;
          default:
              break;
        // Treat all other characters as a space.
          //_delay_ms(MORSE_DELAY_SPACE);
      }

    i++;
  }

  //radio_disable_tx();
}
