/*
   High Accuracy Calibrated Clock
   ATmega328P @ nominal 16 MHz

   Features:
   - Timer1 free-running at prescaler 1
   - 64-bit cycle counter
   - Uses calibrated oscillator frequency
   - Accurate microseconds/milliseconds conversion
   - Tick-based scheduling

   Example calibrated frequency:
   15,999,400 Hz
*/

#include <Arduino.h>

const uint32_t CALIBRATED_F_CPU = 15999400UL;

/****************************************************************/
/*                     64-bit Tick Counter                      */
/****************************************************************/

volatile uint32_t timer1OverflowCount = 0;

ISR(TIMER1_OVF_vect)
{
    timer1OverflowCount++;
}

/*
   Returns current cycle count.

   Resolution:
   1 tick = 1 CPU cycle

   At 15,999,400 Hz:

   1 tick = 62.5023438 ns
*/
uint64_t getTicks()
{
    uint32_t high1;
    uint32_t high2;
    uint16_t low;

    do
    {
        high1 = timer1OverflowCount;
        low   = TCNT1;
        high2 = timer1OverflowCount;
    }
    while (high1 != high2);

    return (((uint64_t)high1) << 16) | low;
}

/****************************************************************/
/*                  Time Conversion Functions                   */
/****************************************************************/

uint64_t ticksToMicros(uint64_t ticks)
{
    return (ticks * 1000000ULL) / CALIBRATED_F_CPU;
}

uint64_t ticksToMillis(uint64_t ticks)
{
    return (ticks * 1000ULL) / CALIBRATED_F_CPU;
}

uint64_t microsCal()
{
    return ticksToMicros(getTicks());
}

uint64_t millisCal()
{
    return ticksToMillis(getTicks());
}

/****************************************************************/
/*                Tick-Based Scheduling Helpers                 */
/****************************************************************/

uint64_t secondsToTicks(double seconds)
{
    return (uint64_t)(seconds * CALIBRATED_F_CPU);
}

uint64_t millisToTicks(uint32_t ms)
{
    return ((uint64_t)ms * CALIBRATED_F_CPU) / 1000ULL;
}

uint64_t microsToTicks(uint32_t us)
{
    return ((uint64_t)us * CALIBRATED_F_CPU) / 1000000ULL;
}

/****************************************************************/
/*                   Non-blocking Timer Class                   */
/****************************************************************/

class TickTimer
{
public:

    void start(uint64_t intervalTicks)
    {
        interval = intervalTicks;
        deadline = getTicks() + intervalTicks;
    }

    bool expired()
    {
        uint64_t now = getTicks();

        if ((int64_t)(now - deadline) >= 0)
        {
            deadline += interval;
            return true;
        }

        return false;
    }

private:

    uint64_t interval;
    uint64_t deadline;
};

/****************************************************************/
/*                         Example Usage                        */
/****************************************************************/

TickTimer heartbeat;

void setup()
{
    Serial.begin(115200);

    cli();

    TCCR1A = 0;

    /*
       CS10 = 1

       Prescaler = 1

       Timer frequency =
       calibrated oscillator frequency
    */
    TCCR1B = _BV(CS10);

    TCNT1 = 0;

    TIMSK1 = _BV(TOIE1);

    sei();

    heartbeat.start(millisToTicks(1000));

    Serial.println();
    Serial.println(F("Calibrated clock started"));
}

void loop()
{
    if (heartbeat.expired())
    {
        uint64_t ticks = getTicks();

        Serial.print(F("ticks="));
        Serial.print((uint32_t)(ticks >> 32));
        Serial.print(':');
        Serial.print((uint32_t)ticks);

        Serial.print(F("  ms="));
        Serial.print((unsigned long long)ticksToMillis(ticks));

        Serial.print(F("  us="));
        Serial.println((unsigned long long)ticksToMicros(ticks));
    }

    /*
       Example event timeout:

       static uint64_t start = getTicks();

       if (getTicks() - start > millisToTicks(250))
       {
           ...
       }
    */
}
