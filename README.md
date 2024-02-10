# ATtiny-PinKeepAlive

Flash in the PIN for my utility electricity meter via white LED, then keep the PIN active by flashing the LED briefly every 100 seconds.

# Why?

The electricity power meter supplied by my local power company came mis-configured so there's no way to enable extended INFO-DSS IR datagrams permanently.
The respective menu item has been configured away. Thank you very much!

After PIN entry, the extended datagrams do appear, but only for 120 seconds. After that, the unit falls back to basic datagrams again, requiring another PIN entry. Alternatively, we just simulate some activity so that the 120 second window does not close. By repeated light signals, we're keeping the unit in "extended" mode, so to say.

# Manual operation

A button is connected to a small microcontroller (ATtiny841) and makes a LED light up when pressed. The button is de-bounced so that a minimum LED "on" duration of around half a second is maintained. The button can be pressed for longer, extending the "on" period, which enables entry of "long" press light impulses.

In short, this function converts the error-prone manual flashing contraption to simple button presses.

# Automatic PIN entry

Upon reset, the microcontroller enters austomatic mode. It remains in automatic mode unless the button is pressed.
In automatic mode, the unit first
- emits a short flash to enter the display test page (all display segments activated)
- emits another short flash to proceed to the PIN entry page. With my meter, the display's 2nd line now shows '-0...-' and waits for the first digit.

Then, for each of the (four) digits of the PIN,
- a sequence of short flashes, each followed by a brief OFF pause is emitted, according to the digit value.
- Between digits, we need a pause of about 3-4 seconds. The meter detects this inactivity and switches to the next PIN position automatically.

As we do not have any feedback from the meter (neither IR nor visual) we can do nothing but try and wait for a similar amount of time. Note this cannot be super reliable. If the period is set too short, we're bumping up the current digit too much and if we wait for too long we can skip a digit, causing one or more spurious 0 digits.

The PIN is hard-coded in the source code / microcontroller. To define the PIN, copy file `meterpin-sample.h` to `meterpin.h` and adjust that copy to your PIN. Then build and upload.

# Keep Alive

After the automatic PIN entry or the last button press, the unit waits for 100 seconds, then emits a half-second LED flash and repeats. Pressing the button interrupts this waiting period and causes an immediate flash.

This short flash once in a while simulates some activity to the power meter so it does not ever reach the 120 second (2 minute) PIN timeout.

Consequently, the extended SML datagrams continue to be emitted. Mission accomplished.

# What the heck?

I still have not found the reason why anybody would want to configure away the option to emit extended datagrams permanently. What's the purpose?!

# Caveats

- The LED needs to cause a light stream of at least 400lux at the receiver. I found that a random LED flashlight that I "deconstructed" for the purpose worked nicely: Originally driven by 3x AAA, the LED seems to be OK with being driven by nearly 5V (at 100mA). Hence, I just used a small BC557 transistor with some base resistor, but no further resistor in series with the load. When installed just opposite the meter's receiver photo transistor things seem to be working reliably for me.
- It might work with a series resistor to reach a lower current through the LED, but I have not yet tried that. By using a number of ATtiny ports in parallel it might be possible to get rid of the transistor, too.
- The timing of this whole PIN entry scheme is not overly critical, but not certainly not fool-proof either. After all, it's intended for manual operation. Especially, the inter-digit delays may give you a problem. In this case, please adjust `DIGIT_GAP2_TICKS` in code and re-try.
- If your meter happens to __not__ have a sticker preventing access to the MSB-DSS (IR transmitter/receiver, lucky you!), you want to check there first, as the MSB-DSS may well emit extended datagrams irrspectively of PIN entry status - that is, even without a PIN!
- The PIN is communicated once only after the unit's start. Make sure to have a reset button available and/or cycle the power supply when you
need to input the PIN repeatedly. Or change the code to detect some button press pattern.
- Why ATtiny841? Well, that was the nearest unit I happened to find on my workbench. And it's got two hardware serial ports, which I may use eventually to read the SML and to forward that via wired RS485, eventually. Of course, any Arduino-like microcontroller with some form of periodic interrupt should work.
