/*
   ATtiny-PinKeepAlive
   - Chip: ATtiny841
   - Clock: 8MHz internal

   Hardware:
   - PB0       White LED via BC557 PNP transistor, with 4K7 resistor to base, active LOW
               To use an NPN transistor (active HIGH), define LED_ON and LED_OFF accordingly in meterpin.h
   - PB1       Push button against GND
   - PA1       TXD0 USART0 RS-485, physical pin 12
   - PA2       RXD0 USART0 RS-485, physical pin 11
   - PA5       TXD1 USART1 DEBUG, physical pin 8, used with __DEBUG__ = 1 (may cause timing ModBus RTU problems)
   - PA4       RXD1 USART1 INFO-DSS Infrared (idle high), physical pin 9

   v0.5        May 2024
*/

#include <Arduino.h>
#include "PinKeepAlive.h"
#include "ExposeToModbus.h"
#include "ReadFromInfoDSS.h"

#ifdef PINMAPPING_CCW
#error "Sketch was written for clockwise pin mapping!"
#endif

volatile static bool ticked = false;

static ObisValues obisValues = ObisValues();
static TinySMLDecoder tinySMLDecoder = TinySMLDecoder(&obisValues);

// Called every 100ms
ISR(TIMER1_COMPA_vect)
{
  ticked = true;
}

// Reset ticked to false, return true if it was ticked.
bool resetTicked()
{
  bool r = ticked;
  ticked = false;
  return r;
}

void setup()
{
  setupPinKeepAlive();
  pushPinEntry();
  setupModbus(&obisValues);
  setupInfoDSS(&tinySMLDecoder);
}

void loop()
{
  if (resetTicked())
  {
    onTickPinKeepAlive();
  }

  onTickModbus();
  onTickInfoDSS();
}

// END
