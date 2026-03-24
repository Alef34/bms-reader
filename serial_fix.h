#ifndef SERIAL_FIX_H
#define SERIAL_FIX_H

// Workaround for TFT_eSPI Serial issue on ESP32-C3
// Forward declare Serial to make it available in all compilation units

#ifdef __cplusplus
extern "C++"
{
#endif

    class HardwareSerial;
    extern HardwareSerial Serial;

#ifdef __cplusplus
}
#endif

#endif // SERIAL_FIX_H
