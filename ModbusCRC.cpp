/*
 * Helper for CRC16 computation
 * Depending on parameters, this can do Modbus RTU as well as SML CRCs.
 */

#include "ModbusCRC.h"

ModbusCRC::ModbusCRC()
{
  reset();
}

void ModbusCRC::reset()
{
  crc = 0xFFFF;
  poly = 0xA001;
  xorOut = 0x0000;
}

X25CRC::X25CRC()
{
  reset();
}

void X25CRC::reset()
{
  crc = 0xFFFF;
  poly = 0x8408;
  xorOut = 0xFFFF;
}

void ModbusCRC::feed(uint8_t c)
{
  crc ^= c;
  for (uint8_t i = 8; i != 0; i--)
  {
    if ((crc & 0x0001) != 0)
    {
      crc >>= 1;
      crc ^= poly;
    }
    else
    {
      crc >>= 1;
    }
  }
}

uint8_t ModbusCRC::getCRCLowByte()
{
  return crc ^ xorOut;
}

uint8_t ModbusCRC::getCRCHighByte()
{
  return (crc ^ xorOut) >> 8;
}

// END
