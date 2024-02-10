/*
   ATtiny-PinKeepAlive
   - Chip: ATtiny841
   - Clock: 8MHz internal

   Hardware:
   - PB0       White LED via BC557 PNP transistor, with 4K7 resistor to base, active LOW
   - PB1       Push button against GND
   - Pxx, Pxx  Serial TXD, RXD, 57600 8N1

   v0.1        February 2024
*/

#define LED_W          PIN_PB0 // Physical Pin 2
#define BTN_IN         PIN_PB1 // Physical Pin 3
#define TIMER0_COUNT        42 // Counter value of 42 gives about 100 msec timer interval
#define KEEP_ALIVE_TICKS    50 // 1000 x 100ms = 100s until we flash briefly to keep the unit on its feet
#define FLASH_TICKS          3 // 200ms flash is not enough to alert the unit, we need 300ms or more
#define SHORT_PULSE_TICKS    4 // 400ms flash min duration for a button press

volatile static bool ticked = false;
volatile static unsigned long count = 0;
static unsigned int zz = 0;
static unsigned int nn = KEEP_ALIVE_TICKS;

ISR( TIMER0_COMPA_vect ) {
  if ( ++count > TIMER0_COUNT ) {
    count = 0;
    ticked = true;
  }
}

void setup() {
  pinMode(LED_W, OUTPUT);
  digitalWrite(LED_W, HIGH);
  pinMode(BTN_IN, INPUT_PULLUP);

  cli();
  TIMSK0 = 0;               // Timer/Counter Interrupt Mask Register, disable all timer interrupts
  OCR0A  = 0;               // Output Compare Register A
  TIMSK0 |= _BV( OCIE0A );  // Timer/Counter0 Output Compare Match A Interrupt Enable
  sei();
}

// zz = 0  Waiting for button press, LED off period
// zz = 1  Transition after 100s, keep-alive flash start
// zz = 2  Manual short flash triggered

void onTick() {
  // Waiting? Wait up to nn ticks.
  boolean transition = (nn == 0) || (--nn == 0);

  if (transition) {
    if (zz == 0) {
      digitalWrite(LED_W, LOW);
      nn = FLASH_TICKS;
      zz = 1;
    } else if (zz == 1) {
      digitalWrite(LED_W, HIGH);
      nn = KEEP_ALIVE_TICKS;
      zz = 0;
    } else if (zz == 2) {
      // End of short manual pulse
      // Button still pressed? Do nothing, keep the LED lit.
      // Button released? Switch LED off and revert to Z0.
      if (digitalRead(BTN_IN) != 0) {
        digitalWrite(LED_W, HIGH);
        nn = KEEP_ALIVE_TICKS;
        zz = 0;
      }
    } else {
      zz = 0;
    }
  } else if (digitalRead(BTN_IN) == 0) {
    if (zz == 0 || zz == 1) {
      digitalWrite(LED_W, LOW);
      nn = SHORT_PULSE_TICKS;
      zz = 2;
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
