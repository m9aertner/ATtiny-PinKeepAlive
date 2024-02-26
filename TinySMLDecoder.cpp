/*
 * My attempty at decoding SML ... small state machine.
 */

#if __has_include("debug.h")
#include "debug.h"
#endif
#if __DEBUG__
#include <stdio.h>
#endif
#include "TinySMLDecoder.h"

#define OBIS_CODES_ON_LEVEL 5

void TinySMLDecoder::reset()
{
    z = 0;
    n = 0;
    m = 0;
    dt = 0;
    zr = 0;
    p = 0;
    level = 0;

#if __DEBUG__
    maxlevel = 0;
    indent[0] = 0;
#endif

    crc.reset();
    obisValues->reset();
}

void TinySMLDecoder::expect(const uint8_t len)
{
    n = len - 1;
    zr = z;
    if (n > sizeof(buf))
    {
        reset();
    }
}

void TinySMLDecoder::onBOM()
{
#if __DEBUG__
    printf("%s-- BOM\n", &indent);
#endif
}

void TinySMLDecoder::onList(const uint8_t nListElements)
{
#if __DEBUG__
    printf("%s-- %d/%d\n", &indent, read[level] + 1, open[level]);
    printf("%s{\n", &indent);
#endif
    if (nListElements > 0)
    {
        level++;
        open[level] = nListElements;
        read[level] = 0;
#if __DEBUG__
        indent[level - 1] = ' ';
        indent[level] = 0;
        if (level > maxlevel)
        {
            maxlevel = level;
        }
#endif
    }
}

void TinySMLDecoder::onElement()
{
#if __DEBUG__
    printf("%s-- %d/%d\n", indent, read[level] + 1, open[level], dt);
    printf("%s(%02X)", &indent, dt);
    for (uint8_t k = 0; k < p; k++)
    {
        printf("%02X ", buf[k]);
    }
    printf("\n");
#endif
    switch (dt)
    {
    case 0x00:
        onOctetString();
        break;
    case 0x40:
        onBoolean();
        break;
    case 0x50:
        onInteger();
        break;
    case 0x60:
        onUnsigned();
        break;
    default:
        onUnrecognizedElement();
    }
}

void TinySMLDecoder::onOctetString()
{
    if (level == OBIS_CODES_ON_LEVEL && read[level] == 0)
    {
        // List Element 5.0
        obisValues->feedObisOctetString(buf, p);
    }
}

void TinySMLDecoder::onBoolean()
{
    // ignore
}

void TinySMLDecoder::onInteger()
{
#if __DEBUG__
    int64_t raw = 0;
#endif
    if (level == OBIS_CODES_ON_LEVEL)
    {
        const uint8_t nListElement = read[level];
        if (nListElement == 4 && p == 1)
        {
            // Scaler
            scaler = buf[0];
        }
        else if (nListElement == 5)
        {
            // Value
            int64_t value = 0;
            if (buf[0] >= 0x80)
            {
                value = ~0;
            }
            for (uint8_t k = 0; k < p;)
            {
                value = (value << 8) + buf[k++];
            }
#if __DEBUG__
            raw = value;
#endif
            uint32_t scaled = toScale(value);
            obisValues->feedObisValue(scaled);
#if __DEBUG__
            printf("%s-- value raw    = %lld\n", &indent, raw);
            printf("%s-- value scaled = %lu\n", &indent, scaled);
#endif
        }
    }
}

uint32_t TinySMLDecoder::toScale(int64_t value)
{
    while (scaler > 0)
    {
        value *= 10;
        scaler--;
    }
    int8_t remainder = 0;
    while (scaler < 0)
    {
        remainder = value % 10;
        value /= 10;
        scaler++;
    }
    if (value >= 0 && remainder >= 5)
    {
        value++;
    }
    else if (value <= 0 && remainder <= -5)
    {
        value--;
    }
    return (uint32_t)value;
}

void TinySMLDecoder::onUnsigned()
{
    // Ignore. All the measurements we're interested in are signed.
}

void TinySMLDecoder::onUnrecognizedElement()
{
    // Ignore. All the measurements we're interested in are handled.
}

void TinySMLDecoder::maybeLeaveLevel()
{
    while (level)
    {
        read[level]++;
        if (read[level] == open[level])
        {
            leaveLevel();
        }
        else
        {
            break;
        }
    }
}

void TinySMLDecoder::leaveLevel()
{
    if (level)
    {
        if (level-- == OBIS_CODES_ON_LEVEL)
        {
            obisValues->feedObisOctetString(buf, 0);
        }
#if __DEBUG__
        indent[level] = 0;
        printf("%s}\n", &indent);
#endif
    }
}

void TinySMLDecoder::onEOM()
{
#if __DEBUG__
    printf("%s-- EOM, max level = %d\n", &indent, maxlevel);
#endif
    if (msgLowCRC == buf[0] && msgHighCRC == buf[1])
    {
        onGoodCRC();
    }
    else
    {
        onBadCRC();
    }
    reset();
}

void TinySMLDecoder::onGoodCRC()
{
#if __DEBUG__
    printf("%s-- CRC OK\n", &indent);
#endif
    obisValues->commit();
}

void TinySMLDecoder::onBadCRC()
{
#if __DEBUG__
    printf("%s-- BAD CRC", &indent);
#endif
    obisValues->reset();
}

void TinySMLDecoder::feed(const uint8_t cc)
{
    uint8_t z0 = z;

    if (!m)
    {
        crc.feed(cc);
    }

    if (n)
    {
        buf[p++] = cc;
        if (--n == 0)
        {
            onElement();
            maybeLeaveLevel();
            z0 = z = zr;
        }
    }
    else if (m)
    {
        buf[p++] = cc;
        if (--m == 0)
        {
            onEOM();
        }
    }
    else if (z < 4)
    {
        if (cc == 0x1B)
        {
            z++;
        }
    }
    else if (z == 4)
    {
        if (cc == 0x01)
        {
            z++;
        }
        else if (cc == 0x1A)
        {
            z = 8;
        }
    }
    else if (z < 8)
    {
        if (cc == 0x01)
        {
            if (++z == 8)
            {
                // 1B 1B 1B 1B 01 01 01 01
                onBOM();
                z = 9;
            }
        }
    }
    else if (z == 8)
    {
        // 1B 1B 1B 1B 1A cc xx yy
        // Section 8.1, 155 Pos. 6
        if (cc <= 0x03)
        {
            z++;
            m = 2;
            p = 0;
            msgLowCRC = crc.getCRCLowByte();
            msgHighCRC = crc.getCRCHighByte();
        }
    }
    else if (z == 9)
    {
        // 1B 1B 1B 1B 01 01 01 01 ...
        // After BOM in main message content
        dt = (cc & 0xF0);
        p = 0;
        // Note: For now, only length 0x0-0xF is implemented.
        if (dt == 0x70) // List, Section 6.1, 130
        {
            uint8_t nListElements = (cc & 0x0F);
            if (nListElements == 0x00)
            {
                onList(0);
                maybeLeaveLevel();
            }
            else if (level < sizeof(open) - 1)
            {
                onList(nListElements);
            }
            else
            {
                reset();
            }
        }
        else if (cc == 0x1B) // Start of EOM
        {
            z = 1;
        }
        else if (cc == 0x00) // End of message indicator (sent as last list element)
        {
            maybeLeaveLevel();
        }
        else if (dt == 0x00     // Octet String
                 || dt == 0x40  // Boolean
                 || dt == 0x50  // Integer
                 || dt == 0x60) // Unsigned
        {
            expect(cc & 0x0F);
            if (n == 0)
            {
                onElement();
                maybeLeaveLevel();
            }
        }
        else
        {
            reset();
        }
    }

    // Unchanged state? We could not read a valid next input? Reset and re-sync.
    if (z == z0 && z < 9)
    {
        reset();
    }
}

// END
