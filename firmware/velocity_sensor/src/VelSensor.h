#pragma once
#include <stdint.h>

// Time-of-flight velocity state machine for the dual-comparator droplet sensor.
//
// A droplet passes Sensor 1 (first) then Sensor 2 (second), each separated by a
// fixed distance d. The interrupt service routines in main.cpp latch a rising
// edge on either sensor into (fs1, fs2) flags and a shared timestamp, then call
// step() once per event from the main loop. Internally:
//
//   * the FIRST accepted S1 rising edge starts the timer (startTime = Time);
//   * each accepted S2 rising edge is counted; the N-th one stops the timer
//     (stopTime = Time);
//   * once a measurement is complete, velocity = d / (stopTime - startTime),
//     and the machine resets for the next droplet.
//
// vel is the only quantity the caller prints. Its value encodes the phase:
//   *  0   before the first measurement (initial state);
//   * -1   while accumulating (timer running, no result yet);
//   * > 0  a completed measurement, in mm/s — the value to report.
//
// The implementation depends on no Arduino headers, so it builds and runs
// unchanged under the `native` PlatformIO test environment.
class VelSensor {
public:
    // Outputs (read-only from the caller's perspective).
    float    vel        = 0.0f;   // mm/s; -1 while accumulating, 0 before first measurement
    bool     timerInit  = false;  // timer started (first S1 edge seen)
    bool     timerStop  = false;  // N-th S2 edge seen; measurement pending
    uint32_t startTime  = 0;      // µs timestamp of the starting S1 edge
    uint32_t stopTime   = 0;      // µs timestamp of the N-th S2 edge
    uint8_t  counter_s2 = 0;      // accepted S2 edges in the current cycle

    // Initialise the machine. n is the number of S2 edges to accumulate before a
    // velocity is computed; d is the sensor centre-to-centre separation in mm.
    void init(uint8_t n, float d_mm);

    // Advance the machine by one event. Call when (fs1 || fs2) is set; pass the
    // latched ISR inputs and the captured timestamp (µs).
    void step(bool fs1, bool fs2, uint32_t timeUs);

private:
    // Per-input rising-edge detector: out is true only on a 0→1 transition, and
    // the first call after init always returns false.
    struct EdgeDetector {
        bool init = true;
        bool prev = false;
        bool step(bool input) {
            bool out = (!init) && (!prev) && input;
            init = false;
            prev = input;
            return out;
        }
    };

    uint8_t      n_ = 2;
    float        d_ = 9.88f;
    EdgeDetector edge_s1_;
    EdgeDetector edge_s2_;
};
