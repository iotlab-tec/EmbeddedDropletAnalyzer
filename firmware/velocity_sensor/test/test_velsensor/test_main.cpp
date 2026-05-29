// Host-side equivalence tests for the time-of-flight state machine.
//
// These mirror the behavioural spec in the firmware brief: a droplet train
// crosses Sensor 1 then Sensor 2, the timer starts on the first accepted S1
// rising edge, the N-th accepted S2 rising edge stops it, and the velocity
// d / Δt is reported once the cycle resets. Build/run with:
//
//   pio test -e native
//
// The state machine sees one latched (fs1, fs2) event per step() call, exactly
// as main.cpp's loop drives it (flags are cleared between events, so each event
// carries a single rising flag). A counted S2 edge therefore requires an
// intervening S1 to re-arm the S2 edge detector — i.e. the physical pattern is
// an alternating S1, S2, S1, S2, … droplet train.

#include <unity.h>
#include "VelSensor.h"

static constexpr float D_MM = 9.88f;
static constexpr uint8_t N  = 2;

void setUp() {}
void tearDown() {}

// Helper: one event with only S1 latched.
static void s1(VelSensor &s, uint32_t t) { s.step(true, false, t); }
// Helper: one event with only S2 latched.
static void s2(VelSensor &s, uint32_t t) { s.step(false, true, t); }

// A full measurement: timer spans the starting S1 edge to the N-th S2 edge, and
// velocity = d / Δt is emitted exactly once, on the reset that follows.
void test_full_measurement_reports_d_over_dt() {
    VelSensor s;
    s.init(N, D_MM);

    s1(s, 500);    // first-ever S1 edge: edge detector returns false, timer NOT started
    s2(s, 600);    // S2 #1 before any timer start: ignored
    TEST_ASSERT_FALSE(s.timerInit);

    s1(s, 1000);   // accepted S1 rising edge → timer starts here
    TEST_ASSERT_TRUE(s.timerInit);
    TEST_ASSERT_EQUAL_UINT32(1000, s.startTime);

    s2(s, 2000);   // counted S2 edge #1 (of N) — no result yet
    TEST_ASSERT_EQUAL_UINT8(1, s.counter_s2);
    TEST_ASSERT_FALSE(s.timerStop);
    TEST_ASSERT_TRUE(s.vel <= 0.0f);   // nothing reported before the N-th S2 edge

    s1(s, 3000);   // timer already running → no change to startTime
    TEST_ASSERT_EQUAL_UINT32(1000, s.startTime);
    TEST_ASSERT_TRUE(s.vel <= 0.0f);

    s2(s, 4000);   // counted S2 edge #2 == N → timer stops, stopTime latched
    TEST_ASSERT_TRUE(s.timerStop);
    TEST_ASSERT_EQUAL_UINT32(4000, s.stopTime);
    TEST_ASSERT_TRUE(s.vel <= 0.0f);   // still no positive result until the reset

    s1(s, 5000);   // reset cycle → velocity computed
    const float dt_s     = (4000.0f - 1000.0f) / 1000000.0f;   // 3 ms
    const float expected = D_MM / dt_s;                        // 3293.33 mm/s
    TEST_ASSERT_TRUE(s.vel > 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, expected, s.vel);
    TEST_ASSERT_EQUAL_UINT8(0, s.counter_s2);                  // counter reset for next droplet
    TEST_ASSERT_FALSE(s.timerStop);
}

// No velocity is ever reported before the N-th accepted S2 edge.
void test_no_output_before_nth_s2_edge() {
    VelSensor s;
    s.init(N, D_MM);

    s1(s, 100);
    s2(s, 200);
    s1(s, 1000);   // timer start
    s2(s, 2000);   // only the 1st of N S2 edges

    // With a single counted S2 edge the timer must not have stopped and no
    // positive velocity may have appeared.
    TEST_ASSERT_FALSE(s.timerStop);
    TEST_ASSERT_EQUAL_UINT8(1, s.counter_s2);
    TEST_ASSERT_TRUE(s.vel <= 0.0f);
}

// A continuous alternating S1/S2 droplet train produces successive independent
// measurements. The S1 edge that closes a measurement (the reset) does NOT start
// the next one — timerInit drops to false on reset and a fresh measurement begins
// on the following accepted S1 edge.
void test_consecutive_measurements() {
    VelSensor s;
    s.init(N, D_MM);

    // Measurement A: timer spans the S1 @1000 to the 2nd S2 @4000 (Δt = 3 ms),
    // reported on the reset S1 @5000.
    s1(s, 100);
    s2(s, 200);
    s1(s, 1000);
    s2(s, 2000);
    s1(s, 3000);
    s2(s, 4000);
    s1(s, 5000);   // reset → reports A
    const float velA = D_MM / ((4000.0f - 1000.0f) / 1000000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, velA, s.vel);
    TEST_ASSERT_FALSE(s.timerInit);   // reset cleared the timer

    // Measurement B continues the same train: fresh start on S1 @7000, 2nd S2
    // @13000 (Δt = 6 ms), reported on the reset S1 @14000.
    s2(s, 6000);
    s1(s, 7000);
    s2(s, 8000);
    s1(s, 9000);
    s2(s, 13000);
    s1(s, 14000);  // reset → reports B
    const float velB = D_MM / ((13000.0f - 7000.0f) / 1000000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, velB, s.vel);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_full_measurement_reports_d_over_dt);
    RUN_TEST(test_no_output_before_nth_s2_edge);
    RUN_TEST(test_consecutive_measurements);
    return UNITY_END();
}
