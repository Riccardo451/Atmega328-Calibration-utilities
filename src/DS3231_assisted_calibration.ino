/*
  DS3231 Clock Calibration for ATmega328P
  ---------------------------------------

  Purpose:
  - Measure the actual oscillator frequency
    of the ATmega328P.

  Method:
  - Count Timer1 CPU cycles.
  - Measure elapsed time using a DS3231 RTC.
  - Calculate actual oscillator frequency.

  Recommended run time:
    Minimum: 1 hour
    Better : 6 hours
    Best   : 24 hours

  ----------------------------------------------------------------

  Wiring

  DS3231 Module        Arduino Uno / ATmega328P

  VCC ---------------- 5V
  GND ---------------- GND
  SDA ---------------- A4
  SCL ---------------- A5

  Optional:

  SQW ---------------- D2

  (Not used by this sketch)

  ----------------------------------------------------------------

  Required Library:

  RTClib by Adafruit

  Install from Library Manager.

*/

#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

/**********************************************************/
/*                 64-bit Tick Counter                    */
/**********************************************************/

volatile uint32_t timer1OverflowCount = 0;

ISR(TIMER1_OVF_vect)
{
    timer1OverflowCount++;
}

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

    return ((uint64_t)high1 << 16) | low;
}

/**********************************************************/
/*                    Calibration Data                    */
/**********************************************************/

DateTime startRTC;
uint64_t startTicks;

const uint32_t MEASUREMENT_SECONDS =
    3600UL;        // 1 hour

// For higher accuracy:
//
// 21600 = 6 hours
// 86400 = 24 hours

bool completed = false;

/**********************************************************/

void setup()
{
    Serial.begin(115200);

    while (!Serial)
        ;

    Serial.println();
    Serial.println(F("DS3231 Calibration"));
    Serial.println();

    if (!rtc.begin())
    {
        Serial.println(F("DS3231 not found"));
        while (1);
    }

    /******************************************************/
    /* Timer1 setup                                       */
    /******************************************************/

    cli();

    TCCR1A = 0;
    TCCR1B = _BV(CS10);   // prescaler = 1

    TCNT1 = 0;

    TIMSK1 = _BV(TOIE1);

    sei();

    /******************************************************/

    startRTC   = rtc.now();
    startTicks = getTicks();

    Serial.print(F("Start Unix Time: "));
    Serial.println(startRTC.unixtime());

    Serial.println(F("Calibration running..."));
    Serial.println();
}

void loop()
{
    if (completed)
        return;

    DateTime nowRTC = rtc.now();

    uint32_t elapsedSeconds =
        nowRTC.unixtime() -
        startRTC.unixtime();

    static uint32_t lastPrint = 0;

    if (elapsedSeconds != lastPrint)
    {
        lastPrint = elapsedSeconds;

        Serial.print(F("Elapsed: "));
        Serial.print(elapsedSeconds);
        Serial.println(F(" s"));
    }

    if (elapsedSeconds >= MEASUREMENT_SECONDS)
    {
        uint64_t endTicks = getTicks();

        uint64_t deltaTicks =
            endTicks - startTicks;

        double measuredFrequency =
            (double)deltaTicks /
            (double)elapsedSeconds;

        double ppm =
            ((measuredFrequency - 16000000.0)
            / 16000000.0)
            * 1000000.0;

        Serial.println();
        Serial.println(F("--------------------------------"));
        Serial.println(F("CALIBRATION COMPLETE"));
        Serial.println(F("--------------------------------"));

        Serial.print(F("Elapsed Seconds: "));
        Serial.println(elapsedSeconds);

        Serial.print(F("Total Ticks: "));
        Serial.println((unsigned long)(deltaTicks >> 32));

        Serial.print(F("Measured Frequency: "));
        Serial.println(measuredFrequency, 6);

        Serial.print(F("PPM Error: "));
        Serial.println(ppm, 3);

        Serial.println();

        Serial.print(F("Use this value: "));
        Serial.println();

        Serial.print(F("const uint32_t CALIBRATED_F_CPU = "));
        Serial.print((uint32_t)(measuredFrequency + 0.5));
        Serial.println(F("UL;"));

        Serial.println();
        Serial.println(F("Done."));

        completed = true;
    }
}
