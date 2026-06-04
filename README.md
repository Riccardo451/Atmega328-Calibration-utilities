# High Accuracy Timekeeping for ATmega328P

## Overview

This project replaces Arduino's `millis()` and `micros()` with a high-accuracy timing system based on a free-running 64-bit hardware cycle counter.

The system uses:

* ATmega328P Timer1
* Prescaler = 1
* Full CPU clock rate
* 64-bit cycle accumulation
* User-calibrated oscillator frequency

Unlike Arduino's Timer0-based timing functions, this implementation directly counts CPU cycles and converts them to time using a measured oscillator frequency.

The result is a clock whose accuracy is limited almost entirely by:

* Crystal accuracy
* Crystal temperature drift
* Calibration reference quality

rather than Arduino timing implementation details.

---

# Why Not Use millis()?

Arduino's timing system:

* Uses Timer0
* Uses a prescaler of 64
* Generates interrupts approximately every 1.024 ms
* Applies software compensation

While excellent for general applications, it introduces:

* 1 ms granularity
* Quantization effects
* Additional software approximation

For long-term timing applications these limitations become measurable.

Examples:

* Precision logging
* Data acquisition
* Astronomical timing
* GPS-disciplined clocks
* Long-duration experiments

---

# Architecture

Timer1 runs continuously:

```
CPU Clock
    |
    v
Timer1 (16-bit)
    |
    +--> Overflow ISR
                |
                v
        64-bit Tick Counter
```

Each tick corresponds to exactly one CPU clock cycle.

For a nominal 16 MHz crystal:

```
1 tick = 62.5 ns
```

For a calibrated crystal:

```
1 tick = 1 / measured_frequency
```

Example:

```
Measured frequency = 15,999,400 Hz

Tick period:

1 / 15,999,400

= 62.5023438 ns
```

---

# Timing Accuracy

Example crystal:

```
Actual frequency:

15,999,400 Hz
```

Nominal frequency:

```
16,000,000 Hz
```

Error:

```
-600 Hz
```

PPM:

```
ppm = (actual - nominal)
      / nominal
      × 1,000,000

ppm = -37.5
```

Daily drift if uncorrected:

```
37.5 ppm

= 3.24 seconds/day
```

After calibration:

```
Residual error ≈ measurement error
```

which can be reduced to only a few ppm.

---

# Measuring Actual Clock Frequency

Two practical methods are recommended:

1. GPS PPS (best)
2. DS3231 RTC (very good)

---

# Method 1: GPS PPS Calibration

## Why GPS?

Many GPS modules provide:

```
1 PPS
```

(One Pulse Per Second)

This pulse is synchronized to UTC and is typically accurate within tens of nanoseconds.

Examples:

* u-blox NEO-6M
* u-blox NEO-M8N
* u-blox M9N

---

## Wiring

GPS PPS:

```
PPS ---> Arduino D2 (INT0)
GND ---> GND
```

---

## Principle

Count CPU cycles between PPS pulses.

Perfect clock:

```
16,000,000 cycles
```

Measured:

```
15,999,400 cycles
```

Therefore:

```
actual_frequency
=
15,999,400 Hz
```

---

## GPS Calibration Sketch

```cpp
volatile uint32_t overflowCount = 0;
volatile uint64_t lastTicks = 0;

ISR(TIMER1_OVF_vect)
{
    overflowCount++;
}

uint64_t getTicks()
{
    uint32_t high;
    uint16_t low;

    noInterrupts();

    high = overflowCount;
    low = TCNT1;

    if ((TIFR1 & _BV(TOV1)) && low < 65535)
        high++;

    interrupts();

    return ((uint64_t)high << 16) | low;
}

void ppsISR()
{
    static uint64_t previous = 0;

    uint64_t now = getTicks();

    if (previous != 0)
    {
        uint64_t diff = now - previous;

        Serial.print("Measured Frequency: ");
        Serial.println(diff);
    }

    previous = now;
}

void setup()
{
    Serial.begin(115200);

    TCCR1A = 0;
    TCCR1B = _BV(CS10);

    TIMSK1 = _BV(TOIE1);

    attachInterrupt(
        digitalPinToInterrupt(2),
        ppsISR,
        RISING);
}

void loop()
{
}
```

---

## Averaging

Do not use a single PPS interval.

Average several minutes.

Example:

```
300 measurements
```

Average:

```
15,999,403
15,999,401
15,999,399
...
```

Result:

```
15,999,401.8 Hz
```

Use:

```cpp
const uint32_t CALIBRATED_F_CPU =
    15999402UL;
```

---

# Method 2: DS3231 Calibration

## Why DS3231?

The DS3231 contains:

* Temperature compensated crystal
* Factory calibration

Typical accuracy:

```
±2 ppm
```

which is dramatically better than most Arduino crystals.

---

## Wiring

```
DS3231 SDA -> A4
DS3231 SCL -> A5
GND -> GND
VCC -> 5V
```

---

## Principle

Measure CPU cycles during a long RTC interval.

Longer intervals produce better accuracy.

Recommended:

```
1 hour
```

or

```
24 hours
```

---

## Example

Start:

```
RTC:
12:00:00
```

Ticks:

```
123456789000
```

End:

```
13:00:00
```

Ticks:

```
181054629000
```

Difference:

```
57,597,840,000 ticks
```

Actual frequency:

```
57,597,840,000
/
3600

=
15,999,400 Hz
```

---

## DS3231 Calibration Sketch

```cpp
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

uint64_t startTicks;
DateTime startTime;

void setup()
{
    Serial.begin(115200);

    rtc.begin();

    startTime = rtc.now();

    startTicks = getTicks();

    Serial.println("Calibration started");
}

void loop()
{
    DateTime now = rtc.now();

    uint32_t elapsed =
        now.unixtime()
        - startTime.unixtime();

    if (elapsed >= 3600)
    {
        uint64_t endTicks = getTicks();

        uint64_t delta =
            endTicks - startTicks;

        double frequency =
            (double)delta /
            elapsed;

        Serial.print(
            "Measured frequency: ");

        Serial.println(
            frequency,
            3);

        while (1);
    }
}
```

---

# Calculating PPM

Once actual frequency is known:

```
ppm =
(actual - nominal)
/
nominal
× 1,000,000
```

Example:

```
actual = 15,999,400
nominal = 16,000,000
```

```
ppm = -37.5
```

---

# Recommended Calibration Workflow

## Best Possible

1. GPS PPS
2. Average 5–10 minutes
3. Determine frequency
4. Store calibrated frequency

Expected accuracy:

```
< 1 ppm
```

---

## Very Good

1. DS3231
2. Measure 24 hours
3. Calculate frequency

Expected accuracy:

```
≈2 ppm
```

---

# Temperature Effects

Even after calibration:

```
frequency changes with temperature
```

Typical crystal:

```
10–30 ppm
```

over normal operating range.

Therefore:

GPS PPS > DS3231 > crystal-only calibration

for long-term accuracy.

---

# Final Recommendation

For the highest accuracy:

1. Measure frequency using GPS PPS.
2. Average several hundred PPS intervals.
3. Store the measured frequency in:

```cpp
const uint32_t CALIBRATED_F_CPU =
    15999400UL;
```

4. Use the 64-bit Timer1 clock for all scheduling and timing.
5. Use milliseconds/microseconds only for display and communication.

This approach provides timing accuracy limited almost entirely by the quality of the reference used during calibration rather than by Arduino's software timing implementation.
