//
//	Morse Code Playback Functions
//	Mark Jessop 2018-04
// OK1TE 2018-10
//
#ifndef __MORSE_H
#define __MORSE_H

void sendDotOrDash (char unit);
void sendMorseSequence (const char* sequence);
void sendMorse(const char* message);

#endif //__MORSE_H