# Velocity Sensor — Firmware

Open-source Arduino Zero firmware for the dual-comparator optical droplet velocity sensor. It measures droplet velocity in a microfluidic channel by timing a droplet's transit between two optical comparator sensors (COMP_A and COMP_B) separated by a known distance, and reports the velocity in mm/s over Serial.

Companion firmware to the *HardwareX* manuscript. For day-to-day operation, see [`OPERATION.md`](./OPERATION.md).

## Status

Active development. Target hardware: **Arduino Zero** (Atmel SAMD21G18A, Cortex-M0+).

The time-of-flight algorithm is a clean, hand-written C++ state machine (`src/VelSensor.{h,cpp}`) with no external code generators or proprietary dependencies — it builds from this source alone and runs unchanged under a host-side unit-test environment.

## How it works

Two light-dependent resistors (LDRs), spaced a fixed distance `d` apart along the channel, each feed a comparator whose threshold is set by a digital potentiometer. As a droplet passes, it crosses Sensor 1 (COMP_A) first and Sensor 2 (COMP_B) second. Each comparator rising edge raises an interrupt; the firmware latches a timestamp and runs a small state machine:

1. The first accepted **S1** rising edge **starts** the timer.
2. Each accepted **S2** rising edge is counted; the **N-th** one **stops** the timer.
3. The velocity `d / Δt` (mm/s) is then computed and reset for the next droplet.

Each ISR applies a per-sensor debounce window and a minimum inter-event guard so that comparator chatter and ringing do not produce spurious edges.

## Quick start

Requires [PlatformIO](https://platformio.org/).

```sh
cd firmware/velocity_sensor
pio run                # build for Arduino Zero
pio run -t upload      # build + flash
pio device monitor     # open Serial at 38400 baud
pio test -e native     # run the host-side state-machine unit tests
```

PlatformIO automatically pulls the Arduino framework; no external libraries are required.

## File layout

```
src/
├── Config.h          ← pin map, digipot wiper values, algorithm parameters, debounce guards
├── VelSensor.{h,cpp} ← clean-room time-of-flight state machine (no Arduino deps)
└── main.cpp          ← Arduino glue: pins, I²C digipots, ISRs, main loop, serial banner
test/
└── test_velsensor/   ← native (host-side) equivalence tests for the state machine
```

`VelSensor.{h,cpp}` depend on no Arduino headers, so the algorithm is unit-tested natively (`pio test -e native`) independent of the board.

## Configuration

All tunable values live in `src/Config.h` (single source of truth). Edit and rebuild:

| Constant | Default | Meaning |
|---|---|---|
| `WIPER1` | 76 | DigiPot 1 wiper — sets COMP_B (Sensor 2) threshold |
| `WIPER2` | 83 | DigiPot 2 wiper — sets COMP_A (Sensor 1) threshold |
| `N_S2_EDGES` | 2 | Number of accepted S2 edges to accumulate before computing a velocity |
| `D_MM` | 9.88 | Centre-to-centre separation of the two LDRs, in millimetres |
| `DEBOUNCE_TIME_US` | 1000 | Per-sensor debounce window (µs) |
| `MIN_INTER_S1_US` | 3000 | Minimum inter-event gap for Sensor 1 (µs) |
| `MIN_INTER_S2_US` | 3000 | Minimum inter-event gap for Sensor 2 (µs) |
| `SERIAL_BAUD` | 38400 | Serial monitor baud rate |

The wiper values control the comparator thresholds and must be tuned to the signal amplitude of your specific optical sensor assembly. `D_MM` must match the physical LDR spacing in your build, since reported velocities scale directly with it.

## Hardware requirements

- Arduino Zero (or compatible SAMD21G18A board)
- Two optical comparator outputs connected to **D7** (COMP_A / Sensor 1) and **D6** (COMP_B / Sensor 2)
- Two digital potentiometers (I²C address `0x2F`) for threshold adjustment:
  - DigiPot 1 on the default I²C bus (SDA/SCL)
  - DigiPot 2 on a second I²C bus (SDA = D11, SCL = D13)
- Status LEDs: green (D8), red (D9), yellow (D12), blue (A5), velocity light source (D10), OFS light source (A4)
- Buttons: B1 (D2), B2 (D3), B3 (D5) — active-low with internal pull-ups (reserved; not used by the algorithm)

Full bill of materials, schematic, and assembly are in the *HardwareX* manuscript.

## Serial output

At 38 400 baud the firmware prints a short banner, then one velocity value (mm/s) per completed measurement:

```
>>> Optical Droplet Velocity Sensor <<<
Digital Potentiometer 1: 76
Digital Potentiometer 2: 83
n: 2
=== Velocity (mm/s), one value per detected droplet ===
3293.33
2470.00
2638.41
```

Each floating-point number is a droplet velocity in mm/s. Only completed measurements that differ from the previous value and are `> 0` are printed.

## Dependencies

Declared in `platformio.ini`; no external libraries are required beyond the Arduino framework (pulled automatically by PlatformIO). The host-side test environment uses the bundled Unity framework.

## License

CERN Open Hardware Licence Version 2 — Strongly Reciprocal (CERN-OHL-S v2).
See [`LICENSE-hardware`](../../LICENSE-hardware) at the repository root.

## Citation

If you use this firmware, please cite:

> [manuscript citation TBD — to be added after HardwareX acceptance]

## Maintainer

[Author block TBD — to be added before publication]
