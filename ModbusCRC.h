/*
 * Helper for ModBus RTU CRC computation
 */

#include <Arduino.h>

#ifndef __MODBUSCRC_H
#define __MODBUSCRC_H

class ModbusCRC
{
public:
  ModbusCRC();

  void reset();
  void feed(uint8_t c);

  uint8_t getCRCLowByte();  // Send this first.
  uint8_t getCRCHighByte(); // High byte second.

protected:
  uint16_t crc;
  uint16_t poly;
  uint16_t xorOut;
};

class X25CRC : public ModbusCRC
{
public:
  X25CRC();
  void reset();
};

#endif // __MODBUSCRC_H
