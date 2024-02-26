/*
 * Definitions for reading IR stream from Info-DSS
 */

#include <Arduino.h>
#include "ReadFromInfoDSS.h"
#include "TinySMLDecoder.h"

static TinySMLDecoder *tinySMLDecoder;

void onTickInfoDSS()
{
    if (Serial1.available() > 0)
    {
        uint8_t cc = Serial1.read();
        tinySMLDecoder->feed(cc);
    }
}

void setupInfoDSS(TinySMLDecoder *tinySMLDecoder_)
{
    tinySMLDecoder = tinySMLDecoder_;
    Serial1.begin(INFO_DSS_BAUD, SERIAL_8N1);
    while (!Serial1)
        ;
}

// END
