/*
 * Implementation for RS-485 Connection - ModBus RTU
 */

#include <Arduino.h>
#include "ExposeToModbus.h"
#include "ModbusCRC.h"
#include "meterpin.h" // local settings

// Modbus RTU "Silent Interval"
// See page 13 of the "Modbus Serial Line Protocol and Implementation Guide V1.02"
// http://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf
#ifndef __USE_RS485_T15__
// Use 3.5 character times as silent interval. For > 19200bd, we're supposed to use 1.75ms, independent of baud rate.
// Timer/Counter2 increments every 8µs. 219 x 8µs = 1752µs
static const uint16_t RS_485_SILENT_INTERVAL_uS = 1752;
static const uint16_t RS_485_SILENT_INTERVAL_TICKS = (RS_485_SILENT_INTERVAL_uS / 8);
#else
// For my use case it's not necessary, but we may want to be more aggressive and honour an inter-character spacing
// of more than 1.5 character times (> 19200 baud: 750us) as being an frame abort condition.
// Timer/Counter2 increments every 8µs. 94 x 8µs = 752µs
static const uint16_t RS_485_SILENT_INTERVAL_uS = 752;
static const uint16_t RS_485_SILENT_INTERVAL_TICKS = (RS_485_SILENT_INTERVAL_uS / 8);
#endif

// We expect only one APDU here, with limited length: Read Input Registers (Function code 04) with four parameter bytes and 2 CRC bytes.
static uint8_t apdu[16];
// RS 485 RTU reception state
static uint8_t zz = 0;
static uint8_t ct = 0;
static uint8_t expected = 0;

static ModbusCRC crc = ModbusCRC();
static ObisValues *obisValues;

#if __DEBUG__
void printHex(uint8_t c)
{
    if (c < 16)
    {
        Serial1.print('0');
    }
    Serial1.print(c, HEX);
    Serial1.print(' ');
}
#endif // __DEBUG__

void setupModbus(ObisValues *obisValues_)
{
    obisValues = obisValues_;
    TCCR2B = 0x00; // Timer/Counter2 stop
    TCNT2 = 0x00;  // Reset timer value to 0. No interrupt for Timer 2.
    TCCR2A = 0x00; // Timer/Counter2 Normal mode (with TCCR2B)
    TCCR2B = 0x03; // Timer/Counter2 Prescaler 64 <=> 8 MHz / 64 = 125.000 Hz <=> 8µs

    // Note that Modbus RTU *requires* 11 bits per character. When using parity NONE, it is *mandatory* to have two stop bits.
    // I found that a number of Modbus RTU test programs fail unless we're at TWO stop bits. This includes PyModbus, which is
    // most relevant for my use case, as the HomeAsissant Modbus integration builds on top of that library. Similarly,
    // Modbus Master (2.1.0.0, Windows) does not work for me unless two stop bits are selected on "Connect".
    Serial.begin(RS_485_BAUD, SERIAL_8N2);
    while (!Serial)
        ;

#if __DEBUG__
        // Do not disable TX
#else
    // https://github.com/SpenceKonde/ATTinyCore/blob/v2.0.0-devThis-is-the-head-submit-PRs-against-this/avr/extras/ATtiny_x41.md#uart-serial-support
    UCSR1B &= ~(1 << TXEN1); // disable TX, we only ever read from INFO DSS
#endif // __DEBUG__
}

bool checkCRC()
{
    crc.reset();
    crc.feed(RS_485_ADDRESS);
    for (ct = 0; ct < expected - 2;)
    {
        crc.feed(apdu[ct++]);
    }
    return apdu[ct++] == crc.getCRCLowByte() && apdu[ct] == crc.getCRCHighByte();
}

void send(uint8_t b)
{
    Serial.write(b);
    crc.feed(b);
#if __DEBUG__
    printHex(b);
#endif // __DEBUG__
}

void sendCRC()
{
    uint8_t crcL = crc.getCRCLowByte();
    uint8_t crcH = crc.getCRCHighByte();
    send(crcL);
    send(crcH);
#if __DEBUG__
    Serial1.println();
#endif // __DEBUG__
}

/**
 * Return 0x00 or Modbus Exception Code
 */
uint8_t executeReadInputApdu()
{
#if __DEBUG__
    printHex(apdu[0]); // RS_485_READ_INPUT_REGISTER
    printHex(apdu[1]); // Address High
    printHex(apdu[2]); // Address Low
    printHex(apdu[3]); // Register Count High
    printHex(apdu[4]); // Register Count Low
    printHex(apdu[5]); // CRC
    printHex(apdu[6]); // CRC
    Serial1.println();
#endif

    uint8_t address = apdu[2];       // Register address 0, 1, 2, ... (+256)
    uint8_t registerCount = apdu[4]; // Number of registers to read 1, 2, 3, ...
    const uint8_t obisRegisters = obisValues->getLiveRegistersCount();

    if (apdu[1] != 0x01                             // Address high byte must be 0x01
        || apdu[3] != 0x00                          // Register count high byte must be zero
        || address >= obisRegisters                 // Address must not exceed number of registers
        || registerCount == 0x00                    // Register count must be positive
        || registerCount > obisRegisters            // Register count must not exceed number of registers
        || address + registerCount > obisRegisters) // Register access must not exceed number of registers
    {
        return 0x02; // Illegal data address
    }

    delayMicroseconds(RS_485_SILENT_INTERVAL_uS);
    crc.reset();
    send(RS_485_ADDRESS);
    send(RS_485_READ_INPUT_REGISTER);

    send(registerCount << 1); // Number of bytes to send, each register has two bytes
    while (registerCount--)
    {
        uint16_t value = obisValues->getLiveRegister(address++);
        send((uint8_t)(value >> 8));
        send((uint8_t)value);
    }

    sendCRC();
    return 0x00;
}

void sendErrorReply(uint8_t exceptionCode)
{
    delayMicroseconds(RS_485_SILENT_INTERVAL_uS);
    crc.reset();
    send(RS_485_ADDRESS);
    send(apdu[0] + 0x80);
    send(exceptionCode);
    sendCRC();
}

/*
 * Process incoming APDU.
 * For now we only support one single function code (0x04)
 */
void executeApdu()
{
    uint8_t exceptionCode = 0x1; // Illegal Function
    if (apdu[0] == RS_485_READ_INPUT_REGISTER)
    {
        exceptionCode = executeReadInputApdu();
    }
    if (exceptionCode)
    {
        sendErrorReply(exceptionCode);
    }
}

void onRS485Receive(uint8_t c, bool quiet)
{
    if (quiet)
    {
        if (c == RS_485_ADDRESS)
        {
            zz = 1;
        }
        else
        {
            zz = 0;
        }
    }
    else if (zz == 1)
    {
        if (c == RS_485_READ_INPUT_REGISTER)
        {
            zz = 2;
            ct = 0;
            apdu[ct++] = c;
            expected = 7;
        }
        else
        {
            zz = 0;
        }
    }
    else if (zz == 2)
    {
        apdu[ct++] = c;
        if (expected == ct)
        {
            zz = 0;
            if (checkCRC())
            {
                executeApdu();
            }
        }
    }
}

void onTickModbus()
{
    bool quiet = false;
    if (TCNT2 >= RS_485_SILENT_INTERVAL_TICKS)
    {
        quiet = true;
        TCNT2 = RS_485_SILENT_INTERVAL_TICKS;
    }
    if (Serial.available() > 0)
    {
        // Read current input, reset Timer/Counter2 on each character received
        int c = Serial.read();
        TCNT2 = 0;
        onRS485Receive((uint8_t)c, quiet);
    }
}

// END
