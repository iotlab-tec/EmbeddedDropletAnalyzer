// Velocity Sensor — Arduino Zero firmware.
//
// Times a droplet's transit between two optical comparator sensors (COMP_A and
// COMP_B) separated by a known distance and reports velocity in mm/s over Serial.
// The time-of-flight state machine lives in VelSensor.{h,cpp}; this file is the
// hardware glue: pin setup, the two digital-potentiometer comparator thresholds,
// the rising-edge ISRs, and the main loop. All tunable values are in Config.h.

#include <Arduino.h>
#include <Wire.h>
#include "wiring_private.h"   // pinPeripheral() for the second SERCOM I²C bus
#include "Config.h"
#include "VelSensor.h"

using namespace Config;

// Second I²C bus for DigiPot 2 (DigiPot 1 shares address 0x2F on the default bus).
TwoWire myWire(&sercom1, PIN_SDA2, PIN_SCL2);

VelSensor sensor;

// Shared state written by the ISRs and consumed by the main loop.
volatile unsigned long lastTriggerTime_s1 = 0;  // last raw S1 edge time (debounce)
volatile unsigned long lastTriggerTime_s2 = 0;  // last raw S2 edge time (debounce)
volatile unsigned long lastValidS1Time    = 0;  // last accepted S1 edge time
volatile unsigned long lastValidS2Time    = 0;  // last accepted S2 edge time

volatile uint32_t Time = 0;       // timestamp (µs) of the most recent accepted edge
volatile bool     fs1  = false;   // Sensor 1 edge flag
volatile bool     fs2  = false;   // Sensor 2 edge flag

float prevVel = 0.0f;

// Interrupt for COMP_A (Sensor 1 — first sensor contacted by the droplet).
void isr_1() {
    unsigned long now = micros();
    if ((now - lastTriggerTime_s1) > DEBOUNCE_TIME_US &&
        (now - lastValidS1Time) > MIN_INTER_S1_US &&
        digitalRead(PIN_COMP_S1) == HIGH) {
        lastTriggerTime_s1 = now;
        lastValidS1Time    = now;
        fs1  = true;
        Time = now;
    }
}

// Interrupt for COMP_B (Sensor 2 — second sensor contacted by the droplet).
void isr_2() {
    unsigned long now = micros();
    if ((now - lastTriggerTime_s2) > DEBOUNCE_TIME_US &&
        (now - lastValidS2Time) > MIN_INTER_S2_US &&
        digitalRead(PIN_COMP_S2) == HIGH) {
        lastTriggerTime_s2 = now;
        lastValidS2Time    = now;
        fs2  = true;
        Time = now;
    }
}

// Push a wiper value to a digital potentiometer at DIGIPOT_ADDR on the given bus.
void setWiper(TwoWire &bus, uint8_t value) {
    bus.beginTransmission(DIGIPOT_ADDR);
    bus.write(value);
    bus.endTransmission();
}

void setupPins() {
    pinMode(PIN_COMP_S1, INPUT);
    pinMode(PIN_COMP_S2, INPUT);
    pinMode(PIN_LED_VEL, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    pinMode(PIN_LED_Y, OUTPUT);
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_OFS, OUTPUT);
    pinMode(PIN_B1, INPUT_PULLUP);
    pinMode(PIN_B2, INPUT_PULLUP);
    pinMode(PIN_B3, INPUT_PULLUP);

    digitalWrite(PIN_LED_VEL, HIGH);  // velocity-sensor light source on
    digitalWrite(PIN_LED_B, LOW);
    digitalWrite(PIN_LED_Y, LOW);
    digitalWrite(PIN_LED_R, LOW);
    digitalWrite(PIN_LED_G, LOW);
    digitalWrite(PIN_LED_OFS, LOW);
}

void setup() {
    sensor.init(N_S2_EDGES, D_MM);

    // Bring up both I²C buses just long enough to push the wiper thresholds,
    // then release them — the buses are not used again after boot.
    Wire.begin();
    myWire.begin();
    pinPeripheral(PIN_SDA2, PIO_SERCOM);
    pinPeripheral(PIN_SCL2, PIO_SERCOM);

    Serial.begin(SERIAL_BAUD);
    Serial.println(">>> Optical Droplet Velocity Sensor <<<");
    delay(10);

    setWiper(Wire, WIPER1);
    setWiper(myWire, WIPER2);
    myWire.end();
    Wire.end();

    setupPins();

    attachInterrupt(digitalPinToInterrupt(PIN_COMP_S1), isr_1, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_COMP_S2), isr_2, RISING);

    Serial.print("Digital Potentiometer 1: ");
    Serial.println(WIPER1);
    Serial.print("Digital Potentiometer 2: ");
    Serial.println(WIPER2);
    Serial.print("n: ");
    Serial.println(N_S2_EDGES);
    Serial.println("=== Velocity (mm/s), one value per detected droplet ===");
    delay(1000);
}

void loop() {
    // LED indicators track the timer state.
    digitalWrite(PIN_LED_B, sensor.timerInit);
    digitalWrite(PIN_LED_Y, sensor.timerStop);

    // Advance the state machine when an ISR has latched an edge. Snapshot the
    // volatile inputs once so they cannot change mid-cycle.
    if (fs1 || fs2) {
        sensor.step(fs1, fs2, Time);
    }

    // Print only completed measurements, and only when the value changes.
    if (sensor.vel != prevVel && sensor.vel > 0.0f) {
        Serial.println(sensor.vel);
        prevVel = sensor.vel;
    }

    // Clear the edge flags here (not in the ISR) to avoid a race.
    fs1 = false;
    fs2 = false;
}
