/*
  GPS PPS Calibration for ATmega328P
  ----------------------------------

  Purpose:
  - Measure the actual oscillator frequency of the ATmega328
  - Count Timer1 cycles between GPS PPS pulses
  - Determine CALIBRATED_F_CPU for high-accuracy timing

  Hardware Setup:

  GPS Module (e.g., u-blox NEO-6M, NEO-M8N):
    VCC  -> 5V
    GND  -> GND
    TX   -> (not used here)
    RX   -> (not used here)
    PPS  -> Arduino D2 (INT0)  <-- critical, one pulse per second

  Arduino:
    D2 <- PPS pulse from GPS
    GND <- GPS GND
    5V  <- GPS VCC

  Notes:
    - Timer1 runs free at prescaler = 1
    - Each tick = 1 CPU cycle
    - PPS pulse triggers an interrupt
*/

#include <Arduino.h>

// 64-bit overflow counter for Timer1
volatile uint32_t timer1OverflowCount = 0;

// Timer1 Overflow ISR
ISR(TIMER1_OVF_vect) {
  timer1OverflowCount++;
}

// Get the current 64-bit tick count
uint64_t getTicks() {
  uint32_t high1, high2;
  uint16_t low;

  do {
    high1 = timer1OverflowCount;
    low = TCNT1;
    high2 = timer1OverflowCount;
  } while (high1 != high2);

  return ((uint64_t)high1 << 16) | low;
}

// PPS ISR: called on rising edge of PPS
void ppsISR() {
  static uint64_t previousTicks = 0;

  uint64_t nowTicks = getTicks();

  if (previousTicks != 0) {
    uint64_t deltaTicks = nowTicks - previousTicks;
    Serial.print("Ticks between PPS: ");
    Serial.println(deltaTicks);

    // Optional: calculate approximate frequency
    double frequency = (double)deltaTicks; // ticks per 1 second
    Serial.print("Approx. measured frequency: ");
    Serial.println(frequency, 3);
  }

  previousTicks = nowTicks;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("GPS PPS Calibration Started"));

  // Timer1 setup: free-running 16-bit timer, prescaler 1
  cli();             // disable interrupts
  TCCR1A = 0;
  TCCR1B = _BV(CS10);  // prescaler = 1, full CPU clock
  TCNT1 = 0;          // reset timer
  TIMSK1 = _BV(TOIE1); // enable Timer1 overflow interrupt
  sei();             // enable interrupts

  // Attach PPS interrupt
  pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), ppsISR, RISING);
}

void loop() {
  // Nothing needed in loop, all handled in PPS ISR
}
