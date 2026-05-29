#pragma once

#include <Arduino.h>

// Single source of truth for the velocity-sensor firmware: pin map, comparator
// threshold (digital-potentiometer wiper) values, and the time-of-flight
// algorithm parameters. Edit these and rebuild to match your hardware build.

namespace Config {

// ── I²C digital potentiometers (comparator thresholds) ─────────────────────────
// Two MCP4xxx-class single-wiper digipots share I²C address 0x2F, so each sits
// on its own bus (see main.cpp: DigiPot 1 on Wire, DigiPot 2 on a second SERCOM
// TwoWire instance). The wipers set the comparator trip thresholds and must be
// tuned to the signal amplitude of your optical sensor assembly.
constexpr uint8_t DIGIPOT_ADDR = 0x2F;
constexpr uint8_t WIPER1       = 76;   // DigiPot 1 → COMP_B → Sensor 2 threshold
constexpr uint8_t WIPER2       = 83;   // DigiPot 2 → COMP_A → Sensor 1 threshold

// ── Time-of-flight algorithm parameters ────────────────────────────────────────
// A droplet crosses Sensor 1 (first) then Sensor 2 (second). The timer starts on
// the first S1 edge and stops on the N-th accepted S2 edge; velocity = d / Δt.
constexpr uint8_t N_S2_EDGES = 2;       // S2 detections to accumulate before a velocity is computed
constexpr float   D_MM       = 9.88f;   // LDR centre-to-centre separation [mm]

// ── ISR debounce / inter-event guards (µs) ─────────────────────────────────────
constexpr unsigned long DEBOUNCE_TIME_US = 1000;   // per-sensor debounce window
constexpr unsigned long MIN_INTER_S1_US  = 3000;   // ignore S1 edges < 3 ms after the previous valid S1
constexpr unsigned long MIN_INTER_S2_US  = 3000;   // ignore S2 edges < 3 ms after the previous valid S2

// ── Serial ─────────────────────────────────────────────────────────────────────
constexpr unsigned long SERIAL_BAUD = 38400;

// ── Pins (Arduino Zero / SAMD21) ────────────────────────────────────────────────
constexpr uint8_t PIN_COMP_S1 = 7;    // COMP_A — Sensor 1 (first hit), comparator output, ISR on RISING
constexpr uint8_t PIN_COMP_S2 = 6;    // COMP_B — Sensor 2 (second hit), comparator output, ISR on RISING
constexpr uint8_t PIN_LED_R   = 9;
constexpr uint8_t PIN_LED_G   = 8;
constexpr uint8_t PIN_LED_Y   = 12;   // driven by timerStop
constexpr uint8_t PIN_LED_B   = A5;   // driven by timerInit
constexpr uint8_t PIN_LED_VEL = 10;   // velocity-sensor light source (HIGH at boot)
constexpr uint8_t PIN_LED_OFS = A4;   // OFS light source
constexpr uint8_t PIN_B1      = 2;    // reserved (INPUT_PULLUP); not used by the algorithm
constexpr uint8_t PIN_B2      = 3;    // reserved (INPUT_PULLUP)
constexpr uint8_t PIN_B3      = 5;    // reserved (INPUT_PULLUP)

// ── Second I²C bus (DigiPot 2) ──────────────────────────────────────────────────
constexpr uint8_t PIN_SDA2 = 11;
constexpr uint8_t PIN_SCL2 = 13;

}  // namespace Config
