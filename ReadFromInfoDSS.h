/*
 * Definitions for reading IR stream from Info-DSS
 */

#ifndef __READFROMINFODSS_H
#define __READFROMINFODSS_H

#include <Arduino.h>
#include "TinySMLDecoder.h"

#ifndef INFO_DSS_BAUD
#define INFO_DSS_BAUD 9600
#endif

// Setup
void setupInfoDSS(TinySMLDecoder *tinySMLDecoder_);

// Notify on every main loop.
void onTickInfoDSS();

#endif // __READFROMINFODSS_H
