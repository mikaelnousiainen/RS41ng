#include "f_rtty.h"

uint8_t start_bits;
rttyStates send_rtty(char *buffer) {
  static uint8_t nr_bit = 0;
  nr_bit++;
  if (start_bits){
    start_bits--;
    return rttyOne;
  }

  if (nr_bit == 1) {
    return rttyZero;
  }
  if (nr_bit > 1 && nr_bit < (RTTY_7BIT ? 9 : 10)) {
    if ((*(buffer) >> (nr_bit - 2)) & 0x01) {
      return rttyOne;
    } else {
      return rttyZero;
    }
  }

  #ifdef RTTY_7BIT
  nr_bit++;
  #endif

  if (nr_bit == 10) {
    return rttyOne;
  }
  #ifdef RTTY_USE_2_STOP_BITS
  if (nr_bit == 11) {
    return rttyOne;
  }
  #endif
  nr_bit = 0;
  return rttyEnd;
}
