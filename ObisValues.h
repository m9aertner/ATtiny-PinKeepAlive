/*
 * Connection / exchange of OBIS values between SML decoder and Modbus.
 */

#ifndef __OBISVALUES_H
#define __OBISVALUES_H

#include <Arduino.h>

#ifndef N_KNOWN_OBIS_CODES
#define N_KNOWN_OBIS_CODES 3
#endif
#define N_KNOWN_OBIS_REGISTERS (N_KNOWN_OBIS_CODES*2)

class ObisValues
{
public:
    ObisValues();

    void reset();

    /*
     * Present binary OBIS code. Will accept subsequent value if recognized.
     * To reset, pass zero length.
     *
     * Example: 01 00 01 08 00 ff = 1-0:1.8.0
     */
    void feedObisOctetString(uint8_t *buf, uint8_t len);

    /*
     * Pass in value.
     * Will not have any effect unless recognized OBIS code has been fed before.
     */
    void feedObisValue(uint32_t value);

    /*
     * Make fed values become "visible" to consumer.
     */
    void commit();

    /*
     * Number of registers (16bit) is twice the number of values (32bit)
     */
    uint8_t getLiveRegistersCount();

    /*
     * Get a register value. Note high word has lower index.
     */
    uint16_t getLiveRegister(uint8_t n);

private:
    int8_t obisCodeDetected;

    uint16_t liveRegisters[N_KNOWN_OBIS_REGISTERS];
    uint16_t tempRegisters[N_KNOWN_OBIS_REGISTERS];
    bool registerIsSet[N_KNOWN_OBIS_REGISTERS];
};

#endif // __OBISVALUES_H

// END
