# Power Sense

**A simple, open-source energy monitor for a 230 V AC power strip.**

Power Sense measures AC voltage, current, real power, apparent power, power factor and energy, and shows the readings on a local web dashboard. It is built around an ESP32-C3 Super Mini, a ZMPT101B voltage sensor and a ZMCT103C 5 A current sensor.

> **Safety first:** this project connects to mains voltage. Read [docs/SAFETY.md](docs/SAFETY.md) before connecting anything. If you are not confident working with 230 V AC, use a certified energy-monitoring plug instead.

> **Scope:** this version is a **monitor only**. It has **no relay, no display and no push button**. The power strip is live whenever it is plugged in.

## What it does

- Measures AC voltage and current.
- Calculates real power, apparent power, power factor and energy.
- Shows live readings on a web dashboard over Wi-Fi.
- Stores calibration and energy data on the ESP32.
- Uses a standard **5 V 2 A USB charger** for power.

## What it does not do

- It does **not** switch the load. Use the power strip's own mechanical switch.
- It has **no OLED or LCD**.
- It has **no push button**.
- It does **not** need a cloud account or an internet connection.

## Hardware

| Part | Purpose |
|---|---|
| ESP32-C3 Super Mini | Reads the sensors and serves the dashboard |
| ZMPT101B module | Measures AC voltage |
| ZMCT103C 5 A module | Measures AC current |
| 5 V 2 A USB charger | Powers the ESP32 and both sensors |
| 2:1 divider parts | Scales each 0–5 V sensor output to 0–2.5 V |
| 5 A fuse and holder | Protects the load wiring and the 5 A CT |
| Plastic enclosure | Keeps mains and low voltage separated |

See [docs/BOM.csv](docs/BOM.csv) for the full parts list and approximate India prices.

## Pinout

### ESP32-C3 Super Mini

Only four header pins are used:

| ESP32-C3 pin | Connect to | Notes |
|---|---|---|
| **5V** | 5 V from the USB charger; sensor VCC | Powers both sensor modules |
| **GND** | Common ground | Sensors, dividers and ESP32 share this ground |
| **GPIO0** | Voltage divider output | ADC1_CH0 |
| **GPIO1** | Current divider output | ADC1_CH1 |
| 3V3 | Not used | Do not power the 5 V sensor modules from 3.3 V |
| Other GPIO | Not used | Leave free |

Header order, with the USB-C connector at the top:

```text
Left:   5V   GND   3V3   GPIO4   GPIO3   GPIO2   GPIO1   GPIO0
Right:  GPIO5  GPIO6  GPIO7  GPIO8   GPIO9   GPIO10  GPIO20  GPIO21
```

> Verify the silkscreen on your board. Some batches swap the 5V/3V3 positions.

### ZMPT101B voltage sensor

| Module pin | Connect to |
|---|---|
| VCC | 5 V |
| GND | GND |
| OUT | 2:1 divider input → GPIO0 |
| AC-L | Mains Live, after the fuse |
| AC-N | Mains Neutral |

### ZMCT103C current sensor

| Module pin | Connect to |
|---|---|
| GND | GND |
| GND | GND |
| OUT | 2:1 divider input → GPIO1 |
| VCC | 5 V |
| CT hole | **Live wire only**, after the fuse |

> Sensor header order varies between batches. A common ZMCT103C order is GND–GND–OUT–VCC; a common ZMPT101B order is VCC–OUT–GND–GND. Always read the module silkscreen before applying 5 V.

## Divider (required)

Both sensor outputs are 0–5 V AC, biased at about 2.5 V. The ESP32-C3 ADC is **3.3 V maximum** and is only linear to about **2.5 V**. Use this divider on each channel:

```text
Sensor OUT ──[10k 1%]──●──[10k 1%]── GND
                       │
                     [1k 1%]
                       │
ESP32 GPIO0/1 ─────────●──[10nF]── GND
                       │
                    BAT54S
                  pin 3 = signal
                  pin 1 = GND
                  pin 2 = 3.3V
```

- Divider ratio: **2:1** — 0–5 V becomes 0–2.5 V.
- The 2.5 V sensor bias becomes 1.25 V at the ADC.
- The BAT54S clamps the ADC pin to the 3.3 V rail and ground.
- Do **not** connect a sensor output directly to the ESP32.

## Wiring

```text
230 V L ──[5 A fuse]──┬── ZMPT101B AC-L
                      ├── CT (Live wire only)
                      └── strip Live

230 V N ──────────────┴── ZMPT101B AC-N
                          strip Neutral

230 V E ───────────────── strip Earth

5 V 2 A charger ──USB-C──> ESP32-C3

ESP32 5V ──┬── ZMPT101B VCC
           └── ZMCT103C VCC

ESP32 GND ─┬── ZMPT101B GND
           ├── ZMCT103C GND
           └── divider GND

ZMPT101B OUT ──> 2:1 divider ──> GPIO0
ZMCT103C OUT ──> 2:1 divider ──> GPIO1
```

## Power

- Use a **reputable 5 V 2 A USB charger** with proper isolation. Do not use a cheap unbranded charger.
- Plug the charger into a **separate wall outlet** for the cleanest readings. If it is plugged into the monitored strip, its own consumption is included in the measurement.
- The ESP32-C3 Wi-Fi peak is about 350 mA; both sensors draw about 30 mA. A 2 A charger has plenty of margin.
- Add a **100 µF** electrolytic and a **100 nF** ceramic capacitor across 5 V/GND near the sensors.
- Do **not** connect the ESP32 to a PC USB port while the sensors are connected to mains. Unplug mains before programming, or use OTA.

## Quick start

1. Read [docs/SAFETY.md](docs/SAFETY.md).
2. Build the two 2:1 dividers on a small perfboard.
3. Connect the ESP32, sensors and dividers as shown above — **without mains**.
4. Power the ESP32 from the 5 V charger and check:
   - 5 V at the sensor VCC pins.
   - About 2.5 V DC at each sensor OUT pin.
   - About 1.25 V DC at GPIO0 and GPIO1.
5. Only then connect the mains side, using an RCD and preferably an isolation transformer.
6. The firmware and dashboard are the next milestone; see [docs/PLAN.md](docs/PLAN.md).

## Repository

- **[Build plan](docs/PLAN.md)** — architecture, firmware, calibration and test plan.
- **[Pinout & wiring](docs/PINOUT.md)** — complete connection tables.
- **[Bill of materials](docs/BOM.csv)** — parts and approximate prices.
- **[Safety checklist](docs/SAFETY.md)** — read before mains testing.

## Status

Hardware plan and pinout are complete. Firmware, dashboard and enclosure are next. This version intentionally has **no relay, no display and no push button**.
