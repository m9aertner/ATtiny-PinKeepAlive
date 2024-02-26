/*
 * My attempty at decoding SML ... small state machine.
 */

#ifndef __TINYSMLDECODER_H
#define __TINYSMLDECODER_H

#if __has_include("debug.h")
#include "debug.h"
#endif

#include "ModbusCRC.h"
#include "ObisValues.h"

class TinySMLDecoder
{
public:
    TinySMLDecoder(ObisValues *obisValues_)
        : obisValues(obisValues_)
    {
        reset();
    }

    void reset();
    void feed(uint8_t cc);

private:
    ObisValues *obisValues;

    // State management
    uint8_t z;  // SML encoding-level status
    uint8_t n;  // number of bytes to load (1st msg part)
    uint8_t m;  // number of bytes to load (EOM msg part)
    uint8_t dt; // data type (first byte, mask with 0x70)
    uint8_t zr; // status to return to after reading n bytes

protected:
    // Acces to received data in on... methods
    uint8_t p;       // index into buf (received length)
    uint8_t buf[16]; // variable-length data stored here

    int8_t scaler; // which scaler was received in the current OBIS code list element

    // Nesting
    uint8_t level;
    uint8_t open[9]; // Note we do not need more than 5 levels (for my meter, anyway)
    uint8_t read[9];

    // Debugging, indented tree output
#if __DEBUG__
    char indent[21]; // Debug / Logging only.
    unsigned char maxlevel;
#endif

    // CRC
    uint8_t msgLowCRC;
    uint8_t msgHighCRC;
    X25CRC crc = X25CRC();

    // Callback methods, protected if ever someone wants to subclass this
    void onBOM();
    void onList(uint8_t nListElements);
    void onElement();
    void onOctetString();
    void onBoolean();
    void onInteger();
    void onUnsigned();
    void onUnrecognizedElement();
    void onEOM();
    void onGoodCRC();
    void onBadCRC();
    uint32_t toScale(int64_t rawValue);

private:
    // Low level SML structure decoding. It should not be needed to subclass these.
    void expect(uint8_t cc);
    void maybeLeaveLevel();
    void leaveLevel();
};

#endif // __TINYSMLDECODER_H

// END
