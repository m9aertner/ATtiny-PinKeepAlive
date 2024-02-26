/*
 * Small test executable. Put a capture of an SML datagram into sml.bin file, then feed that file name as argument.
 *
 * g++ -I . -DPROGMEM= -D__TEST__=1 -o test-sml TinySMLDecoder.cpp TinySMLDecoderTest.cpp ModbusCRC.cpp ObisValues.cpp
 * ./test-sml testdata/sml.bin
 */

#include <stdio.h>
#include "TinySMLDecoder.h"

#if __TEST__
int main(int argc, char *argv[])
{
    ObisValues obisValues = ObisValues();
    TinySMLDecoder d = TinySMLDecoder(&obisValues);
    unsigned char buffer[1];
    FILE *fIN = fopen(argv[1], "rb");
    while (fread(buffer, 1, sizeof(buffer), fIN) > 0)
    {
        d.feed(buffer[0]);
    }
    fclose(fIN);
    printf("\n");
    uint8_t m = obisValues.getLiveRegistersCount();
    uint32_t dec = 0;
    for (uint8_t k = 0; k < m;)
    {
        uint16_t r = obisValues.getLiveRegister(k);
        printf("R%d: 0x%04X\n", 256 + k, r);
        dec = (dec << 16) + (r & 0xFFFF);
        k++;
        if((k & 1) == 0) {
            printf("      %d\n", dec);
        }
    }
    return 0;
}
#endif

// END
