#ifndef CALIBRATED_CLOCK_H
#define CALIBRATED_CLOCK_H

#include <Arduino.h>

class CalibratedClock
{
public:

    static volatile uint32_t _overflowCount;
    static uint32_t _frequency;

    static void begin(uint32_t calibratedFrequency)
    {
        _frequency = calibratedFrequency;

        cli();

        TCCR1A = 0;
        TCCR1B = _BV(CS10);

        TCNT1 = 0;

        TIMSK1 |= _BV(TOIE1);

        sei();
    }

    static inline void overflowISR()
    {
        _overflowCount++;
    }

    static uint64_t ticks()
    {
        uint32_t high1;
        uint32_t high2;
        uint16_t low;

        do
        {
            high1 = _overflowCount;
            low = TCNT1;
            high2 = _overflowCount;
        }
        while (high1 != high2);

        return (((uint64_t)high1) << 16) | low;
    }

    static uint64_t micros()
    {
        return ticks() * 1000000ULL / _frequency;
    }

    static uint64_t millis()
    {
        return ticks() * 1000ULL / _frequency;
    }

    static uint64_t seconds()
    {
        return ticks() / _frequency;
    }

    static uint64_t elapsedTicks(uint64_t start)
    {
        return ticks() - start;
    }

    static uint64_t elapsedMicros(uint64_t start)
    {
        return micros() - start;
    }

    static uint64_t elapsedMillis(uint64_t start)
    {
        return millis() - start;
    }

    static bool everyMillis(uint64_t& previous,
                            uint32_t interval)
    {
        uint64_t now = millis();

        if ((now - previous) >= interval)
        {
            previous += interval;
            return true;
        }

        return false;
    }

    static bool everyMicros(uint64_t& previous,
                            uint32_t interval)
    {
        uint64_t now = micros();

        if ((now - previous) >= interval)
        {
            previous += interval;
            return true;
        }

        return false;
    }

    static double ppmError()
    {
        return ((_frequency - 16000000.0)
                / 16000000.0)
                * 1000000.0;
    }

    static uint32_t frequency()
    {
        return _frequency;
    }
};

#endif
