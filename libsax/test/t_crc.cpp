#include <stdio.h>
#include <sax/crc.h>

#include <sax/strutil.h>

void test()
{
	std::string str("0123456789");
	uint32_t start = 0;
	uint32_t a, b;
	a = sax::crc32(str, start);
	b = sax::crc32_rev(a, str);
	printf("0x%08x, 0x%08x, 0x%08x\n", start, a, b);
	
	start = 0xaabbccdd;
	a = sax::crc32(str, start);
	b = sax::crc32_rev(a, str);
	printf("0x%08x, 0x%08x, 0x%08x\n", start, a, b);
}


int main(void)
{
    uint8_t buf[1999];
    uint32_t i;

    for(i=0; i<sizeof(buf); i++)
        buf[i]= i+i*i;
/*
    int p[4][3]={{AV_CRC_32_IEEE_LE, 0xEDB88320, 0x3D5CDD04},
                 {AV_CRC_32_IEEE   , 0x04C11DB7, 0xC0F5BAE0},
                 {AV_CRC_16_ANSI   , 0x8005,     0x1FBB    },
                 {AV_CRC_8_ATM     , 0x07,       0xE3      },};
*/
    printf("crc %08X =%X\n", 0xEDB88320, crc32_ieee_le(0, buf, sizeof(buf)));
    printf("crc %08X =%X\n", 0x04C11DB7, crc32_ieee(0, buf, sizeof(buf)));
    printf("crc %08X =%X\n", 0x8005, crc16_ansi(0, buf, sizeof(buf)));
    printf("crc %08X =%X\n", 0x07, crc8_atm(0, buf, sizeof(buf)));
    
    test();
    return 0;
}
