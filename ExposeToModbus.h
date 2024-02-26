/*
 * Variables and definitions for RS-485 Connection - ModBus RTU
 */

#ifndef __EXPOSETOMODBUS_H
#define __EXPOSETOMODBUS_H

#include <Arduino.h>
#include "ObisValues.h"

// Which baud rate do we want to run the bus on? Ideally, you align this with the other sensors on your bus.
// Otherwise you need a master that can switch baud rates on the fly to access different sensors. HomeAssistant can't, AFAIK.
// We run the Microcontroller at 8MHz to get reliable 115200 baud. Didn't work reliably for me at 4MHz.
#ifndef RS_485_BAUD
#define RS_485_BAUD 115200
#endif

// RS-485 address. Default is 9. This is chosen randomly here, just make sure it does not collide with any other sensors on your bus.
#ifndef RS_485_ADDRESS
#define RS_485_ADDRESS ((uint8_t)0x09)
#endif
#define RS_485_READ_INPUT_REGISTER ((uint8_t)0x04) // Function Code 04

// Setup
void setupModbus(ObisValues *obisValues);

// Notify on every main loop.
void onTickModbus();

#endif // __EXPOSETOMODBUS_H
