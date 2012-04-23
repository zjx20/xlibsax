#include "adler32.h"

#define BASE 65521U /* largest prime smaller than 65536 */

#define DO1(buf)  {s1 += *buf++; s2 += s1;}
#define DO4(buf)  DO1(buf); DO1(buf); DO1(buf); DO1(buf);
#define DO16(buf) DO4(buf); DO4(buf); DO4(buf); DO4(buf);

uint32_t calc_adler32(uint32_t adler, const uint8_t *buf, uint32_t len)
{
    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = adler >> 16;

    while (len>0) {
        while(len>16 && s2 < (1U<<31)) {
            DO16(buf); len-=16;
        }
        DO1(buf); len--;
        s1 %= BASE;
        s2 %= BASE;
    }
    return (s2 << 16) | s1;
}

