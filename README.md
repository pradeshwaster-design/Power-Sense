# Power Sense

**A simple, open-source energy monitor for a 230 V AC power strip.**

Power Sense measures AC voltage, current, real power, apparent power and power factor. It runs on an ESP32-C3 Super Mini with a ZMPT101B voltage sensor and a ZMCT103C 5 A current sensor, and it is powered by a single 5 V 2 A USB charger.

> **Safety first:** this project connects to mains voltage. Read [docs/SAFETY.md](docs/SAFETY.md) before connecting anything. If you are not confident working with 230 V AC, use a certified energy-monitoring plug instead.

> **Scope:** this version is a **monitor only**. It has **no relay, no display and no push button**. The power strip is live whenever it is plugged in.

## What it does

- Measures AC voltage and current.
- Calculates real power, apparent power and power factor.
- Sends the readings over serial now; the web dashboard is the next milestone.
- Stores calibration values in the sketch.
- Uses one **5 V 2 A USB charger** for everything.

## What it does not do

- It does **not** switch the load. Use the power strip's own mechanical switch.
- It has **no OLED or LCD**.
- It has **no push button**.
- It does **not** need a cloud account or an internet connection.

## Hardware

| Part | Purpose |
|---|---|
| ESP32-C3 Super Mini | Reads the sensors and runs the firmware |
| ZMPT101B module | Measures AC voltage |
| ZMCT103C 5 A module | Measures AC current |
| 5 V 2 A USB charger | Powers the ESP32; the ESP32 powers the sensors |
| 100 nF capacitors (×4) | ADC filtering and 3.3 V decoupling |
| 100 µF electrolytic | 3.3 V rail near the sensors |
| 5 A fuse and holder | Protects the load wiring and the CT |
| Plastic enclosure | Keeps mains and low voltage separated |

See [docs/BOM.csv](docs/BOM.csv) for the full parts list and approximate India prices.

## Power setup

```text
5 V 2 A charger ──> ESP32-C3 5V pin
ESP32-C3 3V3 pin ──> ZMPT101B VCC
                 └─> ZMCT103C VCC
ESP32-C3 GND ─────> both sensor GND
```

- Power the C3 through its **5V pin** (or USB-C, if you prefer).
- Power **both sensors from the C3's 3V3 pin**. Each module draws only a few mA, and the onboard regulator has plenty of headroom for them.
- Do **not** connect the charger to the 5V pin and a USB cable to the PC at the same time unless your board has a back-feed diode. Program with mains and the external supply disconnected.
- Add a **100 µF** electrolytic and a **100 nF** ceramic capacitor across 3.3 V/GND near the sensors.
- Wi-Fi peaks at about 350 mA; the sensors add only a few mA. A 2 A charger is more than enough.

## Pinout

### ESP32-C3 Super Mini

| Pin | Connect to | Notes |
|---|---|---|
| **5V** | 5 V from the charger | Main supply |
| **GND** | Common ground | ESP32, sensors |
| **3V3** | Both sensor VCC pins | Regulator output |
| **GPIO3** | ZMPT101B OUT | ADC1_CH3 |
| **GPIO4** | ZMCT103C OUT | ADC1_CH4 |

Everything else is unused. Do not use GPIO5 (ADC2, poor readings with Wi-Fi), GPIO8/GPIO9 (strapping; GPIO8 also drives the onboard LED) or GPIO20/GPIO21 (UART).

### ZMPT101B voltage sensor

| Module pin | Connect to |
|---|---|
| VCC | 3V3 |
| GND | GND |
| OUT | GPIO3 |
| AC-L | Mains Live |
| AC-N | Mains Neutral |

### ZMCT103C current sensor

| Module pin | Connect to |
|---|---|
| GND | GND |
| GND | GND |
| OUT | GPIO4 |
| VCC | 3V3 |
| CT hole | **Live wire only**, in series with the load |

> Header order varies between batches. A common ZMCT103C order is GND–GND–OUT–VCC; a common ZMPT101B order is VCC–OUT–GND–GND. Read the module silkscreen before applying power.

## Analog input — no divider needed

Both modules bias their output at **VCC/2**, so at 3.3 V the signal sits at about **1.65 V** and never exceeds the ESP32-C3 ADC range. **Do not add a divider.**

However, the modules use an **LM358-type op-amp that is not rail-to-rail**. At 3.3 V the output tops out around **1.8 V**, leaving only about **0.15 V of headroom above the 1.65 V midpoint**.

- Turn the blue trimpot on the ZMPT101B **down** until the waveform is no longer clipped.
- Turn the current-sensor trimpot down as well; this module may clip well below 5 A at 3.3 V.
- Watch the raw ADC min/max while increasing the load. If the maximum stops rising, the op-amp is clipping; reduce the gain.
- Fit a **100 nF** capacitor from each OUT pin to GND, close to the ESP32.
- The ESP32-C3 ADC is noisy, especially with Wi-Fi on. Average several hundred samples.

Because the usable swing is small, this build is fine for monitoring trends but is **not** a precision meter. If you need the full sensor range, see [5 V fallback](#5-v-fallback).

## AC wiring

```text
230 V L ──[5 A fuse]──┬── ZMPT101B AC-L
                      ├── CT (Live wire through the hole)
                      └── strip / load Live

230 V N ──────────────┴── ZMPT101B AC-N
                          strip / load Neutral

230 V E ───────────────── strip / load Earth
```

- The ZMPT101B input is connected **across Live and Neutral** (in parallel with the load).
- The current transformer goes around the **Live wire only**, with the load current flowing through it.
- **Nothing on the AC side connects to the ESP32 side.** The sensors provide isolation.
- Use a 5 A fuse on Live, keep earth continuous and use an enclosure with a barrier between mains and low voltage.

## Firmware

The sketch is in [`firmware/power_sense/power_sense.ino`](firmware/power_sense/power_sense.ino), with a PlatformIO project file at [`firmware/platformio.ini`](firmware/platformio.ini).

```bash
cd firmware
pio run -t upload      # compile and flash
pio device monitor     # 115200 baud
```

- 12-bit ADC, 11 dB attenuation.
- 400 sample pairs at a fixed 100 µs spacing (40 ms window, about two 50 Hz cycles).
- DC offset removed per window, then RMS, real power and power factor.
- Raw min/max printed so you can spot clipping.

Calibrate the two constants at the top of the sketch:

```cpp
float V_CAL = 1.60f;     // volts RMS per ADC count  (calibrate)
float I_CAL = 0.036f;    // amps RMS per ADC count   (calibrate)
```

Steps:

1. Power up with a known AC voltage and measure it with a multimeter.
2. Read the uncalibrated `V` from the serial monitor.
3. `V_CAL = V_true / V_raw`.
4. Use a known resistive load (for example a 100 W bulb) and a clamp meter.
5. `I_CAL = I_true / I_raw`.

## Quick start

1. Read [docs/SAFETY.md](docs/SAFETY.md).
2. Wire the low-voltage side only: charger → 5V pin, sensors → 3V3/GND, outputs → GPIO3/GPIO4, 100 nF at each output.
3. Power up **without mains** and check:
   - 3.3 V at both sensor VCC pins.
   - About 1.65 V DC at each OUT pin.
   - Raw ADC centred near mid-scale, with no signal.
4. Print raw min/max and turn the trimpots down until the waveform is not clipped.
5. Calibrate `V_CAL` and `I_CAL`.
6. Only then connect the AC side, using an RCD and preferably an isolation transformer.

## 5 V fallback

If the 3.3 V swing is too small or the current sensor clips too early, power both modules from **5V** and divide each output back down:

```text
Sensor OUT ──[10k]──●──[20k]── GND
                    │
ESP32 GPIO3/4 ──────●──[100nF]── GND
                    │
              BAT54S (optional clamp)
```

- 5 V → about 2.2–2.3 V at the pin (the LM358 tops out near 3.5 V, not 5 V).
- That stays inside the ADC's usable range and protects the pin.
- Recalibrate after changing the divider.

## Repository

- **[Build plan](docs/PLAN.md)** — architecture, calculations, calibration and test plan.
- **[Pinout & wiring](docs/PINOUT.md)** — complete connection tables.
- **[Bill of materials](docs/BOM.csv)** — parts and approximate prices.
- **[Safety checklist](docs/SAFETY.md)** — read before mains testing.

## Status

Hardware plan, pinout and a first firmware sketch are done. The web dashboard and enclosure are next. This version intentionally has **no relay, no display and no push button**.
