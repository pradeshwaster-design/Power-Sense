# Power Sense — Build Plan

**Repository:** <https://github.com/pradeshwaster-design/Power-Sense>

**Goal:** Power Sense is a simple, single-phase 230 V AC, 50 Hz energy monitor. It measures voltage, current, real power, apparent power, power factor and energy, and serves a live web dashboard. It is built around an ESP32-C3 Super Mini, a ZMPT101B voltage sensor, a ZMCT103C 5 A current sensor and a 5 V 2 A USB charger.

> **This is a mains-voltage project.** Read [SAFETY.md](SAFETY.md) completely before connecting anything to mains. If you are not confident working with 230 V AC, use a certified energy-monitoring plug instead.

---

## 1. Scope

### Included

- Whole-strip AC voltage and current measurement.
- Real power, apparent power, power factor and energy.
- Local Wi-Fi dashboard.
- Calibration stored on the ESP32.
- Energy counter that survives a power cut.
- Simple power: one 5 V 2 A USB charger.

### Not included

- **No relay.** Power Sense does not switch the load. The strip is live whenever it is plugged in; use the strip's own mechanical switch.
- **No display.** All readings are on the web dashboard.
- **No push button.** Configuration is through the web page.
- **No buzzer, per-socket control, three-phase measurement or revenue-grade billing.**

---

## 2. Limits

| Item | Limit |
|---|---|
| Mains | 230 V AC, 50 Hz, single phase |
| Maximum current | 5 A (the ZMCT103C rating) |
| Continuous current | 4 A recommended |
| Maximum load | ≈ 1150 W at 230 V; derate to ≈ 920 W continuous |
| Accuracy | ±2–5% after calibration |
| Sensor output | 0–5 V AC, biased at about 2.5 V |
| ESP32 ADC | 3.3 V maximum; usable linear range about 0–2.5 V |

The 5 A current transformer is the limiting part. Above 5 A it saturates and the readings become meaningless.

---

## 3. System architecture

```text
230 V AC ──[5 A fuse]──┬── ZMPT101B ── 2:1 divider ── GPIO0
                       ├── CT ── ZMCT103C ── 2:1 divider ── GPIO1
                       └── power strip sockets (always live)

5 V 2 A charger ──USB-C──> ESP32-C3 Super Mini
ESP32 5V / GND ─────────> ZMPT101B and ZMCT103C

ESP32-C3 ──Wi-Fi──> phone / PC dashboard
```

The voltage sensor is connected across Live and Neutral, before the strip's switch, so it can measure mains voltage even when the strip is off. The current transformer is around the Live wire feeding the sockets, so it measures only the load current.

---

## 4. Hardware

See [BOM.csv](BOM.csv) for the full list. The essential parts:

| Part | Approx. price (INR) | Notes |
|---|---|---|
| ESP32-C3 Super Mini | 235 | HW-466AB, USB-C |
| ZMPT101B module | 180 | 0–250 V AC |
| ZMCT103C 5 A module | 180 | 5 A / 5 mA, 1000:1 |
| 5 V 2 A USB charger | 300 | Reputable brand, double-insulated |
| USB-C cable | 100 | Data cable for programming |
| Divider parts | 40 | 10 kΩ, 1 kΩ, 10 nF, BAT54S |
| 5 A fuse + holder | 80 | Main load protection |
| Enclosure and wiring | 500 | Plastic box, terminals, heat-shrink |
| **Approx. total** | **≈ 2,000–2,800** | Prices vary; shipping extra |

A **100 µF** electrolytic and a **100 nF** ceramic capacitor across the 5 V rail near the sensors are recommended.

### Board choice

Use the **ESP32-C3 Super Mini**. It has a 12-bit ADC, the two ADC1 channels needed on GPIO0 and GPIO1, Wi-Fi, USB-C and easy programming.

Do not substitute an ESP8266/ESP-12E in this build. It has one 10-bit ADC with a 0–1 V range and cannot read both sensors without extra hardware.

---

## 5. Pinout

Full details and sensor pinouts: [PINOUT.md](PINOUT.md).

| Function | ESP32-C3 pin | Direction | Notes |
|---|---|---|---|
| Voltage sense | **GPIO0** | Analog in | ADC1_CH0, divider output |
| Current sense | **GPIO1** | Analog in | ADC1_CH1, divider output |
| 5 V supply | **5V** | Power | From the USB charger; sensor VCC |
| Ground | **GND** | Power | Common ground |
| 3.3 V | 3V3 | — | Not used by the sensors |

Only **5V, GND, GPIO0 and GPIO1** are needed. Do not use GPIO2, GPIO8 or GPIO9 for external circuits.

---

## 6. Power

### 6.1 Supply

- Use a **5 V 2 A USB charger** and a USB-C cable to power the ESP32-C3.
- Take 5 V and GND from the ESP32 header to the two sensor modules.
- The ESP32's onboard regulator makes 3.3 V for the ESP32 itself. The sensors stay on 5 V.
- Current draw is about 350 mA peak (ESP32 Wi-Fi) plus about 30 mA for both sensors. The 2 A charger has ample margin.

### 6.2 Power rules

- Use a reputable, double-insulated charger. A poor charger weakens the isolation barrier.
- Plug the charger into a **separate outlet** from the monitored strip for the cleanest readings.
- Do **not** connect the ESP32 to a PC USB port while the sensors are connected to mains.
- Add a 100 µF capacitor and a 100 nF ceramic capacitor across 5 V/GND near the sensors.
- Do not power the 5 V sensor modules from the 3V3 pin.
- If your board's 5V pin measures less than about 4.7 V, power the sensors from a USB-C breakout connected to the same charger, and keep the grounds common.

---

## 7. Analog front-end

Both sensor outputs can reach 5 V. The ESP32-C3 ADC pin is **3.3 V maximum**, and its usable linear range with 11 dB attenuation is about **0–2.5 V**. A 2:1 divider is mandatory.

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

- Divider ratio: **2:1**; 0–5 V becomes 0–2.5 V.
- The 2.5 V sensor bias becomes **1.25 V** at the ADC.
- The 1 kΩ + 10 nF forms a low-pass filter and limits fault current.
- The BAT54S clamps the ADC pin to the 3.3 V rail and ground.
- Use 1% metal-film resistors. Mismatch is removed by calibration.
- Do not add a separate bias network; the sensor output already carries the 2.5 V bias.

### Setting the sensor gain pots

1. Power the sensors from 5 V with **no mains**.
2. Measure each OUT pin; it should be about **2.5 V DC**.
3. Apply the highest expected voltage or current.
4. Adjust the pot so the output peak stays below **4.5 V** (below **2.25 V** at the ADC after the divider).
5. Re-check the divider output with an oscilloscope if available; it should be a clean sine centered at about 1.25 V.

> **Live-adjustment warning:** setting the voltage pot requires mains at the ZMPT input. Use an insulated trimmer tool, keep the mains terminals covered, and work through an RCD/isolation transformer. Never touch the module's input terminals while powered. If you cannot do this safely, set the pot to minimum gain and calibrate in software instead.

---

## 8. Firmware

### 8.1 Stack

- **Framework:** Arduino on PlatformIO.
- **Board:** `esp32-c3-devkitm-1` or the Super Mini board definition.
- **Filesystem:** LittleFS for the dashboard and calibration backup.
- **Libraries:**
  - `ESPAsyncWebServer` + `AsyncTCP`
  - `ArduinoJson`
  - `ArduinoOTA` (built into the ESP32 Arduino core)
  - `Preferences` (NVS, built in)
  - `LittleFS` (built in)
  - `PubSubClient` (optional MQTT)
- **Dashboard:** vanilla HTML/CSS/JS + Chart.js bundled locally. No build step.

`platformio.ini` starting point:

```ini
[env:esp32-c3-supermini]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCORE_DEBUG_LEVEL=0
lib_deps =
    esphome/AsyncTCP-esphome
    esphome/ESPAsyncWebServer-esphome
    bblanchon/ArduinoJson
```

> `ArduinoOTA` is built into the ESP32 Arduino core; do not add it as a library. Package names change, so check the current PlatformIO registry.

### 8.2 Task model

The ESP32-C3 is single-core. Use two tasks:

| Task | Priority | Purpose |
|---|---|---|
| `meterTask` | 10 | Sample ADC, compute RMS/P/PF/energy |
| `webTask` / `loop` | 1 | Async web server, SSE, OTA, MQTT |

- `meterTask` runs every 1 ms (v1) or uses the ADC continuous driver at 4–8 kHz (v2).
- Protect the shared `Measurement` struct with a mutex or double-buffer it.
- Feed the task watchdog in the metering task.
- There is no relay, button or display task.

### 8.3 Sampling

**v1, simple:**

- Every 1 ms, read voltage and current back-to-back.
- Each channel gets 1 kHz, i.e. 20 samples per 50 Hz cycle.
- Accumulate over a 200 ms window (10 cycles).
- Use raw `analogRead` and apply calibration constants.
- Remove the DC bias with a slow IIR filter, or subtract the window mean.
- The V/I pair is separated by about 50–100 µs; calibrate the phase offset.

**v2, better:**

- Use the ESP32-C3 `adc_continuous` driver with a pattern of the two ADC1 channels.
- Sample 8 kHz total (4 kHz per channel), 80 samples per cycle.
- Process blocks in the metering task; the hardware handles timing.
- This reduces jitter and improves low-power-factor accuracy.

Call `analogReadResolution(12)` and `analogSetAttenuation(ADC_11db)` in `setup()` so both channels use the 0–2.5 V range.

### 8.4 Metering math

For each 200 ms window:

```text
Vrms = sqrt(mean((v - vOffset)^2)) * V_CAL
Irms = sqrt(mean((i - iOffset)^2)) * I_CAL
P    = mean((v - vOffset) * (i - iOffset)) * V_CAL * I_CAL
S    = Vrms * Irms
PF   = P / S
E   += P * dt / 3600          // Wh
```

- `V_CAL` and `I_CAL` include the divider ratio and sensor gain.
- `vOffset` and `iOffset` are the DC biases (about 1.25 V at the ADC).
- Phase correction: keep a short circular buffer of current samples and use `i[n + phaseSamples]` in the product. Calibrate `phaseSamples` with a resistive load until PF = 1.00.
- Frequency: zero-crossing detector on the voltage channel with hysteresis; measure over 10 cycles.
- Clipping: the divider maps the sensor's 5 V maximum to about 2.5 V, or ~3100 ADC counts. If the raw ADC stays above ~3000 or below ~100, flag `sensor_clipped` and reduce the pot gain.
- Window synchronization: a fixed 200 ms window is fine. For better results, start on a voltage zero crossing and use an integer number of cycles.

### 8.5 Protection logic

There is no relay to trip. The firmware can only warn:

| Condition | Action |
|---|---|
| Irms > 5.0 A for 1 s | Show "overcurrent" warning; do not switch |
| Vrms > 260 V for 1 s | Show "overvoltage" warning |
| Vrms < 180 V for 5 s | Show "brownout" warning |
| Sensor clipped | Show "sensor clipped" warning |

The user must unplug the load or use the strip's mechanical switch.

### 8.6 Energy persistence

- Keep energy in RAM in Wh.
- Write to NVS when energy increases by 0.1 kWh, or every 15 minutes, whichever comes first.
- On boot, restore the last value. You may lose up to 0.1 kWh after a power cut.
- Avoid writing every second; NVS flash has limited erase cycles.
- Store daily energy separately and reset at local midnight using NTP time.

### 8.7 Calibration storage

Store in NVS:

```text
v_cal, i_cal, phase_cal, v_offset, i_offset,
overcurrent_limit, overvoltage_limit, undervoltage_limit,
energy_kwh, day_energy_kwh, last_day,
wifi_ssid, wifi_pass, dashboard_user, dashboard_pass,
price_per_kwh
```

Provide a "factory reset" that clears calibration and Wi-Fi but preserves the energy counter if desired.

### 8.8 Code skeleton

```cpp
// config.h
#pragma once
#include <Arduino.h>

constexpr int PIN_V = 0;   // ADC1_CH0
constexpr int PIN_I = 1;   // ADC1_CH1
constexpr uint32_t WINDOW_MS = 200;

// Defaults only. Calibrate every unit.
// Both are physical units per 12-bit ADC count (3.3 V reference).
constexpr float V_CAL_DEFAULT = 0.21f;    // V RMS per ADC count
constexpr float I_CAL_DEFAULT = 0.0046f;  // A RMS per ADC count
constexpr float PHASE_CAL_DEFAULT = 0.0f; // samples

constexpr float OVERCURRENT_A = 5.0f;
constexpr float OVERVOLTAGE_V = 260.0f;
constexpr float UNDERVOLTAGE_V = 180.0f;
```

```cpp
// metering.cpp — simplified core
#include <Arduino.h>
#include "config.h"

struct Measurement {
  float v, i, p, s, pf, hz, kwh;
  const char* warning;
};

static Measurement gMeas;
static portMUX_TYPE gMux = portMUX_INITIALIZER_UNLOCKED;

void meterTask(void*) {
  const float dt = WINDOW_MS / 1000.0f;
  float vOff = 0, iOff = 0;
  float sumV2 = 0, sumI2 = 0, sumVI = 0;
  int n = 0;
  uint32_t t0 = millis();

  for (;;) {
    int rawV = analogRead(PIN_V);
    int rawI = analogRead(PIN_I);

    vOff += (rawV - vOff) * 0.0005f;
    iOff += (rawI - iOff) * 0.0005f;

    float v = rawV - vOff;
    float i = rawI - iOff;
    sumV2 += v * v;
    sumI2 += i * i;
    sumVI += v * i;
    n++;

    if (millis() - t0 >= WINDOW_MS) {
      float vRms = sqrtf(sumV2 / n) * V_CAL_DEFAULT;
      float iRms = sqrtf(sumI2 / n) * I_CAL_DEFAULT;
      float p = (sumVI / n) * V_CAL_DEFAULT * I_CAL_DEFAULT;
      float s = vRms * iRms;

      portENTER_CRITICAL(&gMux);
      gMeas.v = vRms;
      gMeas.i = iRms;
      gMeas.p = p;
      gMeas.s = s;
      gMeas.pf = (s > 1.0f) ? (p / s) : 0.0f;
      gMeas.kwh += p * dt / 3600.0f / 1000.0f;
      portEXIT_CRITICAL(&gMux);

      sumV2 = sumI2 = sumVI = 0;
      n = 0;
      t0 = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
```

The production version still needs calibrated ADC conversion, phase correction, frequency measurement, clipping detection, energy persistence, mutex-protected snapshots and watchdog feeding.

### 8.9 Web dashboard

- `/` — live dashboard.
- `/settings` — calibration, limits, price, Wi-Fi reset, OTA.
- `/api/status` — current measurement JSON.
- `/api/config` — calibration and limits.
- `/api/calibrate` — offset / voltage / current / phase calibration.
- `/api/health` — heap, RSSI, uptime, reset reason.
- `/api/ota` — firmware upload.

Use Server-Sent Events at `/events` for live data, or poll `/api/status` every second. SSE reconnects automatically.

Example live JSON:

```json
{
  "v": 230.4,
  "i": 1.23,
  "p": 281.5,
  "s": 283.4,
  "pf": 0.993,
  "hz": 50.01,
  "kwh": 12.345,
  "warning": null,
  "rssi": -62,
  "uptime": 12345
}
```

Require HTTP Basic authentication for `/api/*` and the dashboard. Change the default password on first use. Do not expose the device to the internet without a VPN.

---

## 9. Calibration

### 9.1 Offsets

1. Unplug mains, but keep the 5 V charger connected.
2. Read raw ADC for 2 s on each channel.
3. Compute the mean; that is `vOffset` and `iOffset`.
4. Store in NVS.

With no mains, both sensor outputs should sit at about 2.5 V, which is about 1.25 V at the ADC.

### 9.2 Voltage calibration

1. Apply a known AC voltage through an isolation transformer/variac.
2. Measure the true voltage with a CAT III multimeter.
3. Read `Vrms_raw` from the firmware's uncalibrated output.
4. `V_CAL = V_true / Vrms_raw`.
5. Store and verify at a second voltage.

### 9.3 Current calibration

1. Use a resistive load, e.g. a 1000 W heater (≈4.35 A at 230 V).
2. Measure the true current with a clamp meter.
3. Read `Irms_raw`.
4. `I_CAL = I_true / Irms_raw`.
5. Verify at a second load (e.g. 500 W).

### 9.4 Phase calibration

1. With the resistive load (PF ≈ 1.00), adjust `phaseSamples` until the dashboard reads PF = 1.00.
2. Check with an inductive load (fan, motor). The PF should be lower and plausible.
3. If PF reads negative, reverse the CT wire direction or invert the current sign in software.
4. Re-check power against a commercial meter.

### 9.5 Verification

| Test | Expected |
|---|---|
| No load, strip off | I ≈ 0, P ≈ 0, V ≈ mains |
| 60 W bulb | I ≈ 0.26 A, P ≈ 60 W, PF ≈ 1 |
| 1000 W heater | I ≈ 4.35 A, P ≈ 1000 W, PF ≈ 1 |
| Fan / motor | PF < 1, P < S |
| 1 kWh run | Energy within ±5% of a commercial meter |

Document the final constants and calibration date in the repository. Recalibrate if you change the divider, sensor module or potentiometer setting.

---

## 10. Test plan

### Stage 0 — Bench inspection, no mains

- [ ] Check for shorts between 5 V and GND.
- [ ] Check the divider outputs with a multimeter.
- [ ] Check the CT and ZMPT wiring; no mains connected.
- [ ] Verify the ESP32 boots and Wi-Fi connects.
- [ ] Verify the dashboard loads.

### Stage 1 — 5 V power

- [ ] Connect the 5 V 2 A charger to the ESP32 USB-C.
- [ ] Measure 5 V at the sensor VCC pins.
- [ ] Measure about 2.5 V DC at each sensor OUT pin.
- [ ] Measure about 1.25 V DC at GPIO0 and GPIO1.
- [ ] Check current draw: about 100–200 mA idle, under 500 mA peak.

### Stage 2 — ADC and divider

- [ ] Inject 0.5 V, 1.0 V, 2.0 V, 3.0 V, 4.0 V, 4.8 V DC into each divider input; record ADC counts (expect about half the input).
- [ ] Check linearity; the divider output must stay below 2.5 V at 5 V input.
- [ ] Inject a 50 Hz sine (1 Vpp, 1.25 V offset) from a function generator; verify RMS and frequency.
- [ ] Check the BAT54S clamps at about 3.6 V by injecting 5 V through a 10 kΩ resistor directly at the ADC node (current-limited).

### Stage 3 — Sensors, low voltage / isolation transformer

- [ ] Apply a known AC voltage with a variac/isolation transformer; record the output.
- [ ] Pass a known current through the CT (e.g. a 100 W bulb); record the output.
- [ ] Adjust the pots for a good signal without clipping.
- [ ] Check voltage linearity at 50 V, 100 V, 150 V, 200 V, 230 V.
- [ ] Check current linearity at 0.5 A, 1 A, 2 A, 3 A, 4 A.

### Stage 4 — Mains and load

- [ ] Use an RCD/GFCI and an isolation transformer for the first mains test.
- [ ] Start with a 40 W bulb.
- [ ] Check the voltage and current readings.
- [ ] Then use a 1000 W heater; compare current with a clamp meter.
- [ ] Check terminal and sensor temperature after 30 minutes.

### Stage 5 — Accuracy

- [ ] Compare V, I, P and PF with a commercial energy meter.
- [ ] Run a 1 kWh load and compare energy.
- [ ] Test at low PF (fan/motor) and check phase calibration.
- [ ] Test at no load and check the zero offset.
- [ ] Test with a dimmer or SMPS load; note waveform distortion and accuracy.

### Stage 6 — Soak and safety

- [ ] 24 h run at 500 W; check memory, Wi-Fi reconnect and energy accumulation.
- [ ] 1 h run at 4.0 A; check temperatures (<60 °C at terminals).
- [ ] Earth continuity: <0.1 Ω from the earth pin to the enclosure/socket earth.
- [ ] Insulation resistance: 500 V DC between mains and low voltage; target >1 MΩ (preferably >10 MΩ).
- [ ] Strain relief: pull test on the input cable.
- [ ] Final visual inspection: no exposed mains, no loose strands, no damaged insulation.

**Pass criteria:** V ±2%, I ±5%, P ±5% at PF > 0.8; energy ±5% over 1 kWh; no overheating; all safety checks pass.

---

## 11. Safety

Read [SAFETY.md](SAFETY.md) completely. The short version:

- Mains is lethal. Unplug before touching anything.
- Use a reputable 5 V 2 A charger; do not use a cheap non-isolated charger.
- Do not connect the ESP32 to a PC USB port while mains is connected.
- Fuse the Live wire; keep Earth continuous.
- Keep at least 6 mm between mains and low-voltage conductors.
- The CT goes around **Live only**; never open its secondary while current flows.
- Use an RCD and, preferably, an isolation transformer for the first test.
- The strip is **always live** in this version. There is no relay to turn it off.
- Do not sell or permanently install an uncertified prototype.

---

## 12. Limitations and next steps

### Limitations

- 5 A CT limit; 4 A continuous.
- No switching, display or button.
- Not revenue-grade; expect ±2–5%.
- Wi-Fi only; no cloud service.
- The 5 V charger and sensors add a small measurement offset if powered from the monitored strip.

### Next steps

1. Build and test the two dividers.
2. Build the firmware meter task and `/api/status`.
3. Add the dashboard and SSE.
4. Calibrate with a known voltage and a resistive load.
5. Add energy persistence and OTA.
6. Build the enclosure and run the safety tests.
7. Optional: MQTT/Home Assistant, history graphs, 3D-printed case.

---

## 13. References

- Robu — 5 A single-phase AC current sensor module (ZMCT103C): <https://robu.in/product/5a-range-single-phase-ac-current-sensor-module/>
- Robu — ZMPT101B AC voltage sensor module: <https://robu.in/product/ac-voltage-sensor-module-zmpt101b-single-phase/>
- QuartzComponents — ESP32-C3 Super Mini HW-466AB: <https://quartzcomponents.com/products/esp32-c3-super-mini-development-board-with-soldered-headers-hw-466ab>
- Espressif — ESP32-C3 datasheet: <https://documentation.espressif.com/esp32-c3_datasheet_en.pdf>
- OpenEnergyMonitor — EmonLib (RMS/real-power concepts): <https://github.com/openenergymonitor/EmonLib>
- ZMCT103C datasheet: <https://innovatorsguru.com/wp-content/uploads/2019/08/zmct103C-datasheet.pdf>
