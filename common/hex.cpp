#include "hex.h"

uint8_t hexify_nibble(uint8_t in){
        in &= 0xf;
        in += '0';
        if(in > '9')
          in += ('a' - '9');
        return in;
}

void hexify_uint16(uint16_t in, uint8_t *out){
        out[3] = hexify_nibble((in & 0x000f) >>  0);
        out[2] = hexify_nibble((in & 0x00f0) >>  4);
        out[1] = hexify_nibble((in & 0x0f00) >>  8);
        out[0] = hexify_nibble((in & 0xf000) >> 12);
}

