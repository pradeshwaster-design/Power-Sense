# Power Sense — Voltage / Current / Energy Monitor

**Repository:** <https://github.com/pradeshwaster-design/Power-Sense>

Power Sense is a DIY 230 V AC smart power strip that measures voltage, current, real power, power factor and energy, and serves a live web dashboard from an ESP32-C3 Super Mini.

> **Safety first:** mains voltage is lethal. This is a prototype, not a certified product. Read [docs/SAFETY.md](docs/SAFETY.md) before connecting anything to mains.

## At a glance

| Item | Choice |
|---|---|
| Controller (v1) | ESP32-C3 Super Mini (HW-466AB) |
| Controller (cost-down) | ESP32-C3-MINI-1 module (recommended) or ESP-12E + external ADC (not recommended) |
| Voltage sensor | ZMPT101B module, 0–250 V AC |
| Current sensor | ZMCT103C module, 5 A AC |
| Load limit | 5 A / ~1150 W at 230 V (derate to 4 A continuous) |
| Switching | One 2-pole 10 A relay for the whole strip, or one relay per socket |
| Dashboard | Local web app from LittleFS, SSE live data, Chart.js |
| Connectivity | Wi-Fi, mDNS, optional MQTT / Home Assistant |

## Documents

- **[Full build plan](docs/PLAN.md)** — architecture, firmware, calibration, testing, cost-down path.
- **[Pinout & wiring](docs/PINOUT.md)** — ESP32-C3 header, sensor modules, divider, relay, ESP-12E migration.
- **[Bill of materials](docs/BOM.csv)** — parts and approximate India prices.
- **[Safety checklist](docs/SAFETY.md)** — read before mains testing.

## Quick pin map

| Function | ESP32-C3 pin | Notes |
|---|---|---|
| Voltage sense | GPIO0 / ADC1_CH0 | via 2:1 divider + clamp |
| Current sense | GPIO1 / ADC1_CH1 | via 2:1 divider + clamp |
| Relay | GPIO3 | NPN driver + flyback |
| Push button | GPIO4 | to GND, internal pull-up |
| OLED SDA (optional) | GPIO5 | digital only |
| OLED SCL (optional) | GPIO6 | digital only |
| Buzzer (optional) | GPIO7 | via NPN |
| Spare / relay 2 | GPIO10 | |
| UART debug | GPIO20 / GPIO21 | RX / TX |

**Do not use** GPIO2, GPIO8 or GPIO9 for external circuits: they are strapping pins (GPIO9 is the BOOT button, GPIO8 is the onboard LED).

## Build order

1. Read [docs/SAFETY.md](docs/SAFETY.md) completely.
2. Build the 5 V / 3.3 V low-voltage side; test from a bench supply with the HLK disconnected.
3. Build the 2:1 divider and verify the ADC readings on the bench.
4. Characterize the sensors with isolated/low-voltage sources; calibrate.
5. Add the HLK, relay, and mains wiring; test with an RCD and isolation transformer.
6. Add the dashboard, OTA, energy logging, and enclosure.
7. Only then consider the cost-down board.

## Status

Planning stage. No firmware or PCB files yet. See the plan for the recommended order of work.
