/*
 * Implementation for PinKeepAlive - Flashing the LED
 */

#include <Arduino.h>
#include "PinKeepAlive.h"

#ifndef METER_PIN
#if __has_include("meterpin.h")
#include "meterpin.h" // #define METER_PIN "1234"
#else
#error Please copy meterpin-sample.h to meterpin.h and amend to your PIN
#endif
#endif
static const char PIN[] = METER_PIN;

#define LED_W PIN_PB0 // Physical Pin 2
#ifndef LED_ON
#define LED_ON LOW // LED is active LOW
#define LED_OFF HIGH
#endif
#ifndef BTN_IN
#define BTN_IN PIN_PB1 // Physical Pin 3
#endif

#ifndef KEEP_ALIVE_TICKS
#define KEEP_ALIVE_TICKS 1000 // 1000 x 100ms = 100s until we flash briefly to keep the unit on its feet
#endif
#ifndef FLASH_TICKS
#define FLASH_TICKS 3         // 300ms flash to keep it alive, note I found 200ms flash is not enough
#endif
#ifndef SHORT_PULSE_TICKS
#define SHORT_PULSE_TICKS 4   // 400ms flash min duration for a button press works reliably (here)
#endif
#ifndef DIGIT_GAP0_TICKS
#define DIGIT_GAP0_TICKS 10   // 1000ms flash duration to enter PIN entry mode
#endif
#ifndef DIGIT_GAP1_TICKS
#define DIGIT_GAP1_TICKS 3    // 300ms flash duration between PIN digit entry short pulses
#endif
#ifndef DIGIT_GAP2_TICKS
#define DIGIT_GAP2_TICKS 38   // 3800ms flash duration between PIN digits (not sure if that works with consecutive zeros)
#endif
#ifndef PIN_RESEND_TOCKS
#define PIN_RESEND_TOCKS 12   // After 12 keep-alives (KEEP_ALIVE_TICKS, 12 x 100s = 20min), let's re-send the PIN
#endif
#ifndef PIN_FORCE_TICKS
#define PIN_FORCE_TICKS 1300  // 130s: waiting for more than 120s makes the unit forget the PIN, ready for new entry.
#endif


static int queue[4 + (sizeof(PIN) - 1) * 19 + 1]; // 4 setup, then max 9 pulses with gap1 for each digit, then gap2. Maybe one to force pin mode.
static uint8_t pr = 0;
static uint8_t pw = 0;
static uint8_t tocks = 0; // 1 tock is one keep-alive period (about 100s)
static uint16_t nn = 0;

void push(unsigned int onOrOff, unsigned int ticks)
{
    unsigned int d = (onOrOff == LED_ON ? 50000 : 0) + ticks;
    queue[pw] = d;
    pw = (pw + 1) % (sizeof(queue) / sizeof(queue[0]));
}

unsigned int pop()
{
    unsigned int d = 0;
    if (pr != pw)
    {
        d = queue[pr];
        pr = (pr + 1) % (sizeof(queue) / sizeof(queue[0]));
    }
    return d;
}

void clearQueue()
{
    pw = 0;
    pr = 0;
    nn = 0;
    tocks = 0;
}

// Section: Implementation PinKeepAlive - Flashing the LED

void pushPinEntry()
{
    const char *p = PIN;
    if (!*p)
    {
        return; // Disable PIN entry via empty PIN.
    }

    // Start with two short pulse flashes to get to the PIN entry mode
    push(LED_ON, SHORT_PULSE_TICKS);
    push(LED_OFF, DIGIT_GAP0_TICKS);
    push(LED_ON, SHORT_PULSE_TICKS);
    push(LED_OFF, DIGIT_GAP0_TICKS);

    // Then, for each PIN digit, register respective flashes
    while (*p >= '0' && *p <= '9')
    {
        unsigned int digit = *p++ - '0';
        while (digit--)
        {
            push(LED_ON, SHORT_PULSE_TICKS);
            push(LED_OFF, DIGIT_GAP1_TICKS);
        }
        push(LED_OFF, DIGIT_GAP2_TICKS);
    }
}

void onTickPinKeepAlive()
{
    // Waiting? Wait up to nn ticks.
    int transition = (nn == 0) || (--nn == 0);

    if (digitalRead(BTN_IN) == 0)
    {
        clearQueue();
        push(LED_ON, SHORT_PULSE_TICKS);
        transition = true;
    }

    if (transition)
    {
        unsigned int d = pop();
        if (d != 0)
        {
            if (d >= 50000)
            {
                digitalWrite(LED_W, LED_ON);
            }
            else
            {
                digitalWrite(LED_W, LED_OFF);
            }
            nn = d % 50000;
        }
        else if (tocks < PIN_RESEND_TOCKS)
        {
            tocks++;
            push(LED_OFF, KEEP_ALIVE_TICKS);
            push(LED_ON, FLASH_TICKS);
        }
        else
        {
            tocks = 0;
            push(LED_OFF, PIN_FORCE_TICKS);
            pushPinEntry();
        }
    }
}

void setupPinKeepAlive()
{
    pinMode(LED_W, OUTPUT);
    digitalWrite(LED_W, LED_OFF);
    pinMode(BTN_IN, INPUT_PULLUP);

    noInterrupts();
    TCCR1B = 0x00;        // Stop
    TCCR1A = 0x00;        // Timer/Counter1 CTC (Clear Timer on Compare) mode (with TCCR1B)
    TCCR1B = 0x08;        // Timer/Counter1 CTC (Clear Timer on Compare) mode (with TCCR1B)
    TCNT1 = 0x00;         // Reset timer value to 0
    OCR1A = 12500;        // Output Compare Register A; 125.000 Hz / 12.500 = 10/s <=> 100ms
    TIMSK1 = _BV(OCIE1A); // Timer/Counter1 Output Compare A Match Interrupt Enable
    TCCR1B = 0x0B;        // Timer/Counter1 Prescaler 64 <=> 8 MHz / 64 = 125.000Hz (Start)
    interrupts();
}

// END
