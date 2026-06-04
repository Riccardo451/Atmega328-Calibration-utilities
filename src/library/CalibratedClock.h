#ifndef CALIBRATED_CLOCK_H
#define CALIBRATED_CLOCK_H

/*
    CalibratedClock
    ----------------

    High-accuracy Timer1-based clock for ATmega328P.

    Configuration:

        #define CAL_CLOCK_FREQ 15999402ULL
        #include "CalibratedClock.h"

    before including this file.

    If omitted:

        16000000 Hz

    will be assumed.

    Resolution:
        1 CPU cycle

    Timing source:
        Timer1
        Prescaler = 1

    Overflow period:
        65536 / F_CPU

        ~4.096 ms @ 16 MHz
*/

#include <Arduino.h>
#include <util/atomic.h>

#ifndef CAL_CLOCK_FREQ
#define CAL_CLOCK_FREQ 16000000ULL
#endif

class CalibratedClock
{
public:

    /*
        Q32.32 scale factors

        time =
            ticks * scale >> 32
    */

    static constexpr uint64_t MICRO_SCALE =
        ((1000000ULL << 32) / CAL_CLOCK_FREQ);

    static constexpr uint64_t MILLI_SCALE =
        ((1000ULL << 32) / CAL_CLOCK_FREQ);

    static constexpr uint64_t SECOND_SCALE =
        ((1ULL << 32) / CAL_CLOCK_FREQ);

    static volatile uint64_t overflowCount;

    static void begin()
    {
        cli();

        TCCR1A = 0;
        TCCR1B = _BV(CS10);

        TCNT1 = 0;

        TIMSK1 |= _BV(TOIE1);

        sei();
    }

    static inline void overflowISR()
    {
        overflowCount++;
    }

    /*
        64-bit cycle counter

        Tick 0..65535:
            Timer1

        Tick 65536..:
            overflowCount
    */

    static uint64_t ticks()
    {
        uint64_t high;
        uint16_t low;

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            high = overflowCount;
            low = TCNT1;

            /*
                Handle pending overflow
            */

            if ((TIFR1 & _BV(TOV1)) &&
                low < 65535)
            {
                high++;
            }
        }

        return (high << 16) | low;
    }

    /*
        Fast fixed-point conversion
    */

    static uint64_t micros()
    {
        return (ticks() * MICRO_SCALE) >> 32;
    }

    static uint64_t millis()
    {
        return (ticks() * MILLI_SCALE) >> 32;
    }

    static uint64_t seconds()
    {
        return (ticks() * SECOND_SCALE) >> 32;
    }

    /*
        Raw conversions
    */

    static uint64_t ticksToMicros(uint64_t t)
    {
        return (t * MICRO_SCALE) >> 32;
    }

    static uint64_t ticksToMillis(uint64_t t)
    {
        return (t * MILLI_SCALE) >> 32;
    }

    static uint64_t ticksToSeconds(uint64_t t)
    {
        return (t * SECOND_SCALE) >> 32;
    }

    /*
        Interval conversions
    */

    static constexpr uint64_t microsToTicks(uint64_t us)
    {
        return (us * CAL_CLOCK_FREQ) / 1000000ULL;
    }

    static constexpr uint64_t millisToTicks(uint64_t ms)
    {
        return (ms * CAL_CLOCK_FREQ) / 1000ULL;
    }

    static constexpr uint64_t secondsToTicks(uint64_t sec)
    {
        return sec * CAL_CLOCK_FREQ;
    }

    /*
        Elapsed helpers
    */

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

    /*
        Timeout helper

        Example:

            uint64_t t = millis();

            if(timeoutMillis(t,500))
                ...
    */

    static bool timeoutTicks(
        uint64_t start,
        uint64_t interval)
    {
        return (ticks() - start) >= interval;
    }

    static bool timeoutMillis(
        uint64_t start,
        uint64_t interval)
    {
        return (millis() - start) >= interval;
    }

    static bool timeoutMicros(
        uint64_t start,
        uint64_t interval)
    {
        return (micros() - start) >= interval;
    }

    /*
        Periodic task helper

        Example:

            static uint64_t heartbeat =
                millis();

            if(everyMillis(
                    heartbeat,
                    1000))
            {
                ...
            }
    */

    static bool everyTicks(
        uint64_t& previous,
        uint64_t interval)
    {
        uint64_t now = ticks();

        if ((now - previous) >= interval)
        {
            previous += interval;
            return true;
        }

        return false;
    }

    static bool everyMillis(
        uint64_t& previous,
        uint64_t interval)
    {
        uint64_t now = millis();

        if ((now - previous) >= interval)
        {
            previous += interval;
            return true;
        }

        return false;
    }

    static bool everyMicros(
        uint64_t& previous,
        uint64_t interval)
    {
        uint64_t now = micros();

        if ((now - previous) >= interval)
        {
            previous += interval;
            return true;
        }

        return false;
    }

    /*
        Information
    */

    static constexpr uint64_t frequency()
    {
        return CAL_CLOCK_FREQ;
    }

    static constexpr double ppmError()
    {
        return
            ((double)CAL_CLOCK_FREQ
             - 16000000.0)
            / 16000000.0
            * 1000000.0;
    }
};

/*
    Static storage
*/

volatile uint64_t CalibratedClock::overflowCount = 0;

/*
    Timer1 ISR
*/

ISR(TIMER1_OVF_vect)
{
    CalibratedClock::overflowISR();
}

#endif
