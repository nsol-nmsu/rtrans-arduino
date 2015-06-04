#ifndef _hex_h_
#define _hex_h_
#include <stdint.h>

uint8_t hexify_nibble(uint8_t in);
void hexify_uint16(uint16_t in, uint8_t *out);

#endif
