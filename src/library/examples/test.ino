#include "CalibratedClock.h"

void setup()
{
    CalClock.begin(15999402UL);
}

void loop()
{
    static uint64_t last = 0;

    if (CalClock.elapsedMs(last) >= 1000)
    {
        last = CalClock.millis();

        Serial.println(CalClock.millis());
    }
}
