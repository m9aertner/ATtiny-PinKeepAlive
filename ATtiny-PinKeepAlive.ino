/*
   ATtiny-PinKeepAlive
   - Chip: ATtiny841
   - Clock: 8MHz internal

   Hardware:
   - PB0       White LED via BC557 PNP transistor, with 4K7 resistor to base, active LOW
   - PB1       Push button against GND

   v0.1        February 2024
*/

#ifndef METER_PIN
#include "meterpin.h"          // #define METER_PIN "1234"
#endif
static const char PIN[] = METER_PIN;

#define LED_W          PIN_PB0 // Physical Pin 2
#define LED_ON             LOW // LED is active LOW
#define LED_OFF           HIGH
#define BTN_IN         PIN_PB1 // Physical Pin 3
#define TIMER0_COUNT        42 // Counter value of 42 gives about 100 msec timer interval

#define KEEP_ALIVE_TICKS  1000 // 1000 x 100ms = 100s until we flash briefly to keep the unit on its feet
#define FLASH_TICKS          3 // 300ms flash to keep it alive, note I found 200ms flash is not enough
#define SHORT_PULSE_TICKS    4 // 400ms flash min duration for a button press works reliably (here)
#define DIGIT_GAP0_TICKS    10 // 1000ms flash duration to enter PIN entry mode
#define DIGIT_GAP1_TICKS     3 // 300ms flash duration between PIN digit entry short pulses
#define DIGIT_GAP2_TICKS    38 // 3800ms flash duration between PIN digits (not sure if that works with consecutive zeros)

volatile static bool ticked = false;
volatile static unsigned long count = 0;

static int queue[4 + (sizeof(PIN)-1) * 19]; // 4 setup, then max 9 pulses with gap1 for each digit, then gap2
static int pr = 0;
static int pw = 0;

static unsigned int nn = 0;

void push(unsigned int onOrOff, unsigned int ticks) {
  unsigned int d = (onOrOff == LED_ON ? 50000 : 0) + ticks;
  queue[pw] = d;
  pw = (pw + 1) % sizeof(queue);
}

unsigned int pop() {
  unsigned int d = 0;
  if (pr != pw) {
    d = queue[pr];
    pr = (pr + 1) % sizeof(queue);
  }
  return d;
}

void clearQueue() {
  pw = 0;
  pr = 0;
}

ISR( TIMER0_COMPA_vect ) {
  if ( ++count > TIMER0_COUNT ) {
    count = 0;
    ticked = true;
  }
}

void setup() {
  pinMode(LED_W, OUTPUT);
  digitalWrite(LED_W, LED_OFF);
  pinMode(BTN_IN, INPUT_PULLUP);

  cli();
  TIMSK0 = 0;               // Timer/Counter Interrupt Mask Register, disable all timer interrupts
  OCR0A  = 0;               // Output Compare Register A
  TIMSK0 |= _BV( OCIE0A );  // Timer/Counter0 Output Compare Match A Interrupt Enable
  sei();

  // Start with two short pulse flashes to get to the PIN entry mode
  push(LED_ON, SHORT_PULSE_TICKS);
  push(LED_OFF, DIGIT_GAP0_TICKS);
  push(LED_ON, SHORT_PULSE_TICKS);
  push(LED_OFF, DIGIT_GAP0_TICKS);

  // Then, for each PIN digit, register respective flashes
  const char *p = PIN;
  while(*p >= '0' && *p <= '9') {
    unsigned int digit = *p++ - '0';
    while(digit--) {
      push(LED_ON, SHORT_PULSE_TICKS);
      push(LED_OFF, DIGIT_GAP1_TICKS);
    }
    push(LED_OFF, DIGIT_GAP2_TICKS);
  }
}

void onTick() {
  // Waiting? Wait up to nn ticks.
  int transition = (nn == 0) || (--nn == 0);

  if (digitalRead(BTN_IN) == 0) {
    clearQueue();
    push(LED_ON, SHORT_PULSE_TICKS);
    nn = 0;
    transition = true;
  }

  if (transition) {
    unsigned int d = pop();
    if (d != 0) {
      if (d >= 50000) {
        digitalWrite(LED_W, LED_ON);
      } else {
        digitalWrite(LED_W, LED_OFF);
      }
      nn = d % 50000;
    } else {
      push(LED_OFF, KEEP_ALIVE_TICKS);
      push(LED_ON, FLASH_TICKS);
    }
  }
}

void loop() {
  if (ticked) {
    ticked = false;
    onTick();
  }
}

// END
