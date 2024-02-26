/*
 * My attempt at decoding SML ... small state machine.
 */

#include <Arduino.h>
#include "ObisValues.h"

#ifndef N_KNOWN_OBIS_CODES
#error Number of known OBIS codes should have been defined in ObisValues.h
#endif
#define N_PERSISTENT_OBIS_REGISTERS 4 // Registers at or beyond this index will be reset to 0 when not received
#define UNKNOWN_OBIS_CODE -1
#define OBIS_CODE_BYTES_LENGTH 5

// Version indicator, exposed via Live Registers 0, 1
static const uint32_t VERSION = 2024050601;

static const uint8_t KNOWN_OBIS_CODES[N_KNOWN_OBIS_CODES][OBIS_CODE_BYTES_LENGTH] = {
    {0x01, 0x00, 0x01, 0x08, 0x00}, // 1-0:1.8.0 Positive active energy (A+) total [kWh]
    {0x01, 0x00, 0x02, 0x08, 0x00}, // 1-0:2.8.0 Negative active energy (A+) total [kWh]
    {0x01, 0x00, 0x10, 0x07, 0x00}  // 1-0:16.7.0 Sum active instantaneous power (A+ - A-) [kW]
};

ObisValues::ObisValues()
{
    reset();

    for (uint8_t rr = 0; rr < N_KNOWN_OBIS_REGISTERS; rr++)
    {
        liveRegisters[rr] = 0;
        tempRegisters[rr] = 0;
    }
}

void ObisValues::reset()
{
    obisCodeDetected = UNKNOWN_OBIS_CODE;

    for (uint8_t rr = 0; rr < N_KNOWN_OBIS_REGISTERS; rr++)
    {
        registerIsSet[rr] = false;
    }
}

void ObisValues::feedObisOctetString(uint8_t *buf, uint8_t len)
{
    if (len == 0)
    {
        obisCodeDetected = UNKNOWN_OBIS_CODE;
    }
    else if (len == 1 + OBIS_CODE_BYTES_LENGTH && buf[OBIS_CODE_BYTES_LENGTH] == 0xFF)
    {
        for (uint8_t cc = 0; cc < N_KNOWN_OBIS_CODES; cc++)
        {
            const uint8_t *p = KNOWN_OBIS_CODES[cc];
            const uint8_t *b = buf; // e.g. 01 00 10 07 00 FF, len = 6
            uint8_t m = 0;
            for (uint8_t jj = 0; jj < OBIS_CODE_BYTES_LENGTH; jj++)
            {
                m |= (*p++ != *b++);
            }
            if (m == 0)
            {
                obisCodeDetected = cc;
                break;
            }
        }
    }
}

void ObisValues::feedObisValue(uint32_t value)
{
    if (obisCodeDetected != UNKNOWN_OBIS_CODE)
    {
        uint8_t rr = obisCodeDetected * 2;
        registerIsSet[rr] = true;
        tempRegisters[rr] = (uint16_t)(value >> 16);
        rr++;
        registerIsSet[rr] = true;
        tempRegisters[rr] = (uint16_t)value;
        obisCodeDetected = UNKNOWN_OBIS_CODE;
    }
}

void ObisValues::commit()
{
    for (uint8_t rr = 0; rr < N_KNOWN_OBIS_REGISTERS; rr++)
    {
        if (registerIsSet[rr])
        {
            registerIsSet[rr] = false;
            liveRegisters[rr] = tempRegisters[rr];
        }
        else if (rr >= N_PERSISTENT_OBIS_REGISTERS)
        {
            liveRegisters[rr] = 0;
        }
    }
    obisCodeDetected = UNKNOWN_OBIS_CODE;
}

uint8_t ObisValues::getLiveRegistersCount()
{
    return 2 + N_KNOWN_OBIS_REGISTERS;
}

uint16_t ObisValues::getLiveRegister(uint8_t n)
{
    uint16_t r = 0;
    if (n == 0)
    {
        r = (uint16_t)(VERSION >> 16);
    }
    else if (n == 1)
    {
        r = (uint16_t)(VERSION);
    }
    else if (n < N_KNOWN_OBIS_REGISTERS + 2)
    {
        r = liveRegisters[n - 2];
    }
    return r;
}

// END
