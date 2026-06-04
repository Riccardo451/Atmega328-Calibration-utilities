#define CAL_CLOCK_FREQ 15999402ULL

#include "CalibratedClock.h"

uint64_t heartbeat;

void setup()
{
    Serial.begin(115200);

    CalibratedClock::begin();

    heartbeat = CalibratedClock::millis();

    Serial.println();
    Serial.println(F("Calibrated Clock"));

    Serial.print(F("Frequency: "));
    Serial.println(
        (unsigned long)
        CalibratedClock::frequency());

    Serial.print(F("PPM: "));
    Serial.println(
        CalibratedClock::ppmError(),
        3);
}

void loop()
{
    if (CalibratedClock::everyMillis(
            heartbeat,
            1000))
    {
        Serial.print(F("ms = "));
        Serial.println(
            (unsigned long long)
            CalibratedClock::millis());
    }
}
