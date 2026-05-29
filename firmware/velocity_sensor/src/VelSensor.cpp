#include "VelSensor.h"

void VelSensor::init(uint8_t n, float d_mm) {
    n_         = n;
    d_         = d_mm;
    vel        = 0.0f;
    timerInit  = false;
    timerStop  = false;
    startTime  = 0;
    stopTime   = 0;
    counter_s2 = 0;
    edge_s1_   = EdgeDetector{};
    edge_s2_   = EdgeDetector{};
}

void VelSensor::step(bool fs1, bool fs2, uint32_t timeUs) {
    // A full measurement is complete when the timer has both started and stopped;
    // that is the cue to compute the velocity and reset for the next droplet.
    const bool reset = timerInit && timerStop;

    const bool re1        = edge_s1_.step(fs1);
    const bool block1_clk = re1 && !reset;

    const bool re2        = edge_s2_.step(fs2);
    const bool block2_clk = re2 && timerInit && !reset;

    // Block 1 — start the timer on the first S1 edge of a droplet.
    if (block1_clk) {
        if (!timerInit) {
            startTime = timeUs;
            timerInit = true;
        }
    } else if (reset) {
        timerInit = false;
    }

    // Block 2 — accumulate S2 edges, stop on the N-th, then compute velocity.
    if (block2_clk) {
        timerStop  = (counter_s2 == n_ - 1);
        counter_s2 = counter_s2 + 1;
        stopTime   = timeUs;
        vel        = -1.0f;
    } else if (reset) {
        timerStop  = false;
        vel        = d_ / ((float)(stopTime - startTime) / 1000000.0f);  // mm / s
        counter_s2 = 0;
    } else {
        vel = -1.0f;
    }
}
