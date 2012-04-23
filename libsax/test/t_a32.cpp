#include <stdio.h>
#include <sax/adler32.h>

#define LEN 7001
volatile int checksum;

int main(void){
    int i;
    uint8_t data[LEN];
    for(i=0; i<LEN; i++)
        data[i]= ((i*i)>>3) + 123*i;
    for(i=0; i<1000; i++){
        checksum= calc_adler32(1, data, LEN);
    }
    printf("%X == 50E6E508\n", checksum);
    return 0;
}
