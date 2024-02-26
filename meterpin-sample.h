#define METER_PIN "1234"

// Alternatively, specify empty string to not ever flash in the PIN.
// Use this when you want the Modbus-RTU readout and if
// a) the coarse short datagram kWh values are good enough for your use case, or
// b) if your unit emits long datagrams anyway (mine does now, after someone
//    from my utility company re-configured my unit on-site upon my request!)
//#define METER_PIN ""

#define __DEBUG__ 0
