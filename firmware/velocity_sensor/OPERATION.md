# Operation Manual

This manual covers day-to-day use of the dual-comparator optical droplet velocity sensor. For build instructions and architectural details, see [`README.md`](./README.md). For the bill of materials and assembly, see the *HardwareX* manuscript.

## What this device does

Two light-dependent resistors (LDRs) spaced a fixed distance `d` apart watch the channel. As a droplet passes it crosses **Sensor 1** (COMP_A) first and **Sensor 2** (COMP_B) second. The firmware times the transit and prints the droplet velocity in mm/s over Serial:

- the first accepted Sensor 1 rising edge **starts** the timer;
- each accepted Sensor 2 rising edge is counted, and the **N-th** one **stops** the timer (`N_S2_EDGES`, default 2);
- the firmware computes `velocity = d / Δt` and resets for the next droplet.

There are no operating modes or buttons to press — once powered and configured, the device reports a velocity for each completed measurement automatically.

## Hardware overview

Pin assignments are fixed in firmware (`src/Config.h`) and must match how your board is wired:

| Function | Pin | Notes |
|---|---|---|
| Comparator A (Sensor 1, first hit) | D7 | INPUT, rising-edge interrupt |
| Comparator B (Sensor 2, second hit) | D6 | INPUT, rising-edge interrupt |
| Green LED | D8 | |
| Red LED | D9 | |
| Yellow LED | D12 | On while the timer is stopped (measurement pending) |
| Blue LED | A5 | On while the timer is running |
| Velocity light source | D10 | LED illuminating the LDRs; HIGH at boot |
| OFS light source | A4 | |
| DigiPot 1 SDA / SCL | board defaults | I²C, address `0x2F` |
| DigiPot 2 SDA / SCL | D11 / D13 | second I²C bus (SERCOM1) |
| Buttons B1 / B2 / B3 | D2 / D3 / D5 | INPUT_PULLUP, reserved (not used by the algorithm) |

## LED reference

| LED state | Meaning |
|---|---|
| Velocity LED (D10) steady on | Light source powered — normal at boot and during operation |
| Blue (A5) on | Timer running — a droplet's S1 edge has started a measurement |
| Yellow (D12) on | Timer stopped — the N-th S2 edge has been seen; velocity is about to be reported |
| Both blue and yellow off | Idle between measurements |

## Configuring thresholds and geometry

All tunable values live in `src/Config.h`. The two that almost always need attention:

- **`WIPER1` / `WIPER2`** — the digital-potentiometer wiper values that set the two comparator trip thresholds. Tune these to the signal amplitude of your optical assembly: too low and noise triggers spurious edges, too high and real droplets are missed.
- **`D_MM`** — the centre-to-centre separation of the two LDRs, in millimetres. Reported velocities scale directly with this value, so it must match your physical build.

Edit `src/Config.h`, then rebuild and re-flash (`pio run -t upload`). See [`README.md`](./README.md) for the full constant table.

## Measuring velocity — step by step

### 1. Power on

Connect the device via USB or its dedicated power input. The velocity light source (D10) comes on immediately. The firmware initialises the two I²C buses, pushes the wiper values to both digital potentiometers, then releases the buses and attaches the comparator interrupts.

### 2. Open the Serial monitor

At **38 400 baud** you should see the boot banner:

```
>>> Optical Droplet Velocity Sensor <<<
Digital Potentiometer 1: 76
Digital Potentiometer 2: 83
n: 2
=== Velocity (mm/s), one value per detected droplet ===
```

Confirm the reported wiper values and `n` match what you set in `Config.h`.

### 3. Run droplets through the channel

As droplets pass the sensors:

- The **blue** LED lights when a measurement starts (first S1 edge).
- The **yellow** LED lights when the timer stops (N-th S2 edge).
- A velocity value (mm/s) is printed for each completed measurement:

```
3293.33
2470.00
2638.41
```

Only completed measurements that differ from the previous value and are `> 0` are printed, so a steady stream of identical droplets will still print each new reading as the timing varies.

### 4. Record the readings

Capture the Serial output with your monitor's logging feature (or pipe `pio device monitor` to a file). Each line is one droplet velocity in mm/s.

## Specifications

| Parameter | Value |
|---|---|
| Velocity unit | mm/s |
| Sensor separation `d` | 9.88 mm (configurable) |
| S2 edges per measurement (`N`) | 2 (configurable) |
| Per-sensor debounce window | 1 ms |
| Minimum inter-event gap (per sensor) | 3 ms |
| Timestamp resolution | 1 µs (`micros()`) |
| Serial baud | 38 400 |
| Comparator threshold control | two I²C digital potentiometers (addr `0x2F`) |

## Troubleshooting

**No velocity values printed.**
Confirm the Serial monitor is at 38 400 baud and the boot banner appears. If the banner shows but no readings follow, droplets may not be crossing both sensors, or the comparator thresholds are mis-set — see below.

**Spurious or noisy readings.**
Comparator chatter is usually a threshold problem. Raise the relevant wiper value (`WIPER1` for Sensor 2 / COMP_B, `WIPER2` for Sensor 1 / COMP_A) in `Config.h` until only real droplet crossings trigger. The 1 ms debounce and 3 ms inter-event guards suppress short ringing but cannot compensate for a threshold set into the noise floor.

**Readings are systematically too high or too low.**
Check `D_MM` against the actual LDR spacing in your build — velocity scales linearly with it. Also confirm Sensor 1 (D7) and Sensor 2 (D6) are wired in flow order: the droplet must reach D7 before D6.

**Velocity LED (D10) is off.**
The light source is not powered; the LDRs will not see a usable signal. Check the D10 LED and its wiring.

**The first droplet of a run is never reported.**
This is expected: the state machine reports a velocity on the reset that follows a complete S1→(N×S2) cycle, so there is a one-cycle warm-up before the first reading appears. Subsequent droplets report normally.

## Recompiling the firmware

See [`README.md`](./README.md) for the build process. The compile-time constants in `src/Config.h` can be tuned without touching any other file:

- Edit **`WIPER1`** / **`WIPER2`** to set the comparator thresholds for your optical assembly.
- Edit **`D_MM`** to match the LDR centre-to-centre spacing of your build.
- Edit **`N_S2_EDGES`** to change how many S2 edges are accumulated per measurement.
- Adjust **`DEBOUNCE_TIME_US`** / **`MIN_INTER_S1_US`** / **`MIN_INTER_S2_US`** if your droplet rate or signal shape needs different debounce behaviour.

After any change, run the host-side tests (`pio test -e native`) to confirm the state machine still behaves as specified, then `pio run -t upload` to flash.
