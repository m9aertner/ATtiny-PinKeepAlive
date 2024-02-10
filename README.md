# ATtiny-PinKeepAlive

Flash in the PIN for my utility electricity meter via white LED, then keep the PIN active by flashing the LED briefly every 100 seconds.

# Why?

The electricity power meter supplied by my local power company came mis-configured so there's no way to enable extended INFO-DSS IR datagrams permanently.
The respective menu item has been configured away. Thank you very much!

After PIN entry, the extended datagrams do appear, but only for 120 seconds. After that, the unit reverts to basic datagrams again, requiring another PIN entry.

# Basic operation

A button is connected to a small microcontroller (ATtiny841) and makes a LED light up when pressed. The button is de-bounced so that minimum LED "on" duration
is around half a second, followed by a tenth of a second minimium off period. The button can be pressed for longer, extending the "on" period, which enables
entry of "long" press light impulses. In short, this function converts the error-prone manual flashing contraption to simple button presses.

# Keep Alive

After the last button press, the unit waits for 100 seconds, then emits a half-second LED flash and repeats. Pressing the button interrupts this waiting period.
This simulates some activity to or the power meter so it does not ever reach the 120 second (2 minute) PIN timeout.

Consequently, the extended SML datagrams continue to be emitted.

# What the heck?

I still have not found the reason why anybody would want to configure away the option to emit extended datagrams permanently. What's the purpose?!
