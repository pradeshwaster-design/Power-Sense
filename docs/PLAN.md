# Power Sense — Build Plan

**Repository:** <https://github.com/pradeshwaster-design/Power-Sense>

**Goal:** Power Sense is a single-phase 230 V AC, 50 Hz smart power strip that measures voltage, current, real power, apparent power, power factor and energy, switches the load with a relay, and serves a local web dashboard. The first prototype uses an ESP32-C3 Super Mini, a ZMPT101B voltage sensor and a ZMCT103C 5 A current sensor. A later cost-reduced version may use an ESP8266-family module.

> **This is a mains-voltage project.** Read [SAFETY.md](SAFETY.md) completely before connecting anything to mains. If you are not confident working with 230 V AC, buy a certified smart plug with energy monitoring (Sonoff POW, Shelly EM, etc.) instead.

---

## 1. Scope and assumptions

The plan assumes:

| Item | Assumption |
|---|---|
| Mains | 230 V AC, 50 Hz, single phase (India) |
| Maximum load | 5 A ≈ 1150 W; derate to 4 A continuous |
| Sockets | 4–6 sockets, all switched together by one relay |
| Measurement | Whole-strip voltage and current |
| Sensors | ZMPT101B voltage module, ZMCT103C 5 A current module |
| Controller | ESP32-C3 Super Mini (HW-466AB) |
| Dashboard | Local web page, no cloud required |
| Optional | OLED, buzzer, MQTT/Home Assistant, per-socket relays |

If you want **per-socket** switching or a **16 A** strip, the design changes: use one relay per socket and a 20 A CT (e.g., SCT-013-020) or a dedicated energy-meter IC. The 5 A module is the limiting part of this plan.

---

## 2. Limits you must respect

1. **5 A maximum.** The ZMCT103C is a 5 A CT. Above 5 A it saturates and the readings become meaningless; the module may overheat. Use a 5 A slow-blow fuse and a software trip at 5.0 A. For continuous operation, stay below 4 A.
2. **The ZMPT101B is a 0–250 V module.** In India, mains can rise above 250 V. Calibrate at 230 V, add an over-voltage alert at 260 V, and do not rely on it for safety.
3. **The ESP32-C3 ADC is 3.3 V max, with a usable linear range of about 0–2.5 V.** The sensor outputs are 0–5 V. A divider is mandatory.
4. **ADC2 (GPIO5) does not work while Wi-Fi is active.** Use ADC1 pins GPIO0–GPIO4 only.
5. **The ESP32-C3 Super Mini's 5 V pin and USB-C must not be powered at the same time.** Disconnect mains before connecting USB, or use OTA programming.
6. **This is not a revenue-grade meter.** Expect ±2–5% accuracy after calibration. Do not use it for billing or safety-critical decisions.
7. **Do not sell or permanently install an uncertified prototype.** Mains products need isolation, creepage, fusing, enclosure, and regulatory testing.

---

## 3. System architecture

```
                 ┌─────────────────────────────────────────────┐
  230 V AC ──────┤  Fuse, MOV, X2, switch, terminal blocks     │
  input          └───────────────┬─────────────────────────────┘
                                 │
              ┌──────────────────┼───────────────────┐
              │                  │                   │
       ┌──────▼──────┐    ┌──────▼──────┐     ┌──────▼──────┐
       │ HLK-5M05    │    │ ZMPT101B    │     │ Relay       │
       │ 5 V 1 A     │    │ L-N         │     │ L (and N)   │
       └──────┬──────┘    └──────┬──────┘     └──────┬──────┘
              │ 5 V              │ 0-5 V             │
              │                  │                   │
              │           ┌──────▼──────┐            │
              │           │ 2:1 divider │            │
              │           │ + clamp     │            │
              │           └──────┬──────┘            │
              │                  │ 0-2.5 V           │
              │           ┌──────▼──────┐            │
              │           │ ESP32-C3    │            │
              │           │ GPIO0 (V)   │            │
              │           │ GPIO1 (I)   │            │
              │           │ GPIO3 relay ├────────────┘
              │           │ Wi-Fi + web │
              │           └──────┬──────┘
              │                  │
       ┌──────▼──────┐    ┌──────▼──────┐
       │ ZMCT103C    │    │ Dashboard   │
       │ CT on Live  │    │ phone / PC  │
       └─────────────┘    └─────────────┘
```

Power flow: mains → fuse/MOV → HLK (control power) and ZMPT (voltage sense); mains → relay → CT → sockets. The CT measures only the load current after the relay.

---

## 4. Key design decisions

### 4.1 Why a divider on both sensor outputs

Both modules output an AC waveform centered at about 2.5 V and swinging up to 5 V. The ESP32-C3's ADC pin is not 5 V tolerant, and its usable range is about 0–2.5 V. A 2:1 divider maps 0–5 V to 0–2.5 V and the 2.5 V bias to 1.25 V, which is mid-scale for that range. A BAT54S clamp and a 10 nF capacitor protect the pin and filter noise. See [PINOUT.md](PINOUT.md) for the schematic.

### 4.2 Why GPIO0/GPIO1

- ADC1 channels 0–4 are on GPIO0–GPIO4.
- GPIO5 is ADC2; its analog function is unavailable with Wi-Fi (digital use is fine).
- GPIO2 is a strapping pin.
- GPIO0/GPIO1 are free, safe at boot, and adjacent on the header.

### 4.3 Why not the ESP8266 internal ADC

The ESP8266 has one 10-bit ADC with a 0–1 V range. Two channels are impossible without an external ADC or a multiplexer, and the resolution is four times coarser than the ESP32-C3's 12-bit ADC. The ESP-12E's cost saving is small compared with the extra MCP3202, regulator, and board complexity. See §13.

### 4.4 Why a 5 V HLK module, not a capacitive dropper

A capacitive dropper is not isolated and is dangerous for a DIY build. The HLK-5M05 is an isolated AC-DC module with a 5 V, 1 A output. It powers the ESP32, sensors and relay. Add a 0.5 A fuse on its input.

### 4.5 Why a relay plus a transistor

The ESP32 GPIO cannot source the ~70 mA a relay coil needs. Use an NPN transistor (S8050/2N2222), a base resistor, a pull-down, and a flyback diode. A relay module with an optocoupler is also acceptable, but a bare relay is easier to inspect and repair.

---

## 5. Bill of materials

See [BOM.csv](BOM.csv) for the full list. The essential parts:

| Part | Approx. price (INR) | Notes |
|---|---|---|
| ESP32-C3 Super Mini | 235 | QuartzComponents, HW-466AB |
| ZMPT101B module | 150–250 | Robu |
| ZMCT103C 5 A module | 150–250 | Robu |
| HLK-5M05 (5 V 1 A) | 250–350 | Isolated AC-DC |
| Relay 10 A / 250 VAC + S8050 + 1N4007 | 80–150 | Or a quality relay module |
| 5 A fuse + holder, MOV, X2 cap | 100–150 | Safety parts |
| Terminals, wires, heat-shrink | 200–300 | |
| Enclosure, sockets, switch | 300–600 | Plastic, 4–6 socket strip |
| Divider parts (resistors, caps, BAT54S) | 50–100 | Two channels |
| PCB (optional) | 200–500 | Prototype can use perfboard |

**Prototype total:** roughly ₹2,500–3,500, depending on enclosure and sourcing. Prices vary, and shipping is extra; check current listings.

---

## 6. Pinout summary

Full details, header diagram, sensor pinouts, divider and relay driver: **[PINOUT.md](PINOUT.md)**.

| Function | ESP32-C3 pin | Direction | Notes |
|---|---|---|---|
| Voltage sense | GPIO0 | Analog in | ADC1_CH0, divider output |
| Current sense | GPIO1 | Analog in | ADC1_CH1, divider output |
| Relay | GPIO3 | Digital out | NPN driver, active-high |
| Button | GPIO4 | Digital in | To GND, internal pull-up |
| OLED SDA | GPIO5 | Digital I/O | Optional; ADC2 not used |
| OLED SCL | GPIO6 | Digital I/O | Optional |
| Buzzer | GPIO7 | Digital out | Optional, via NPN |
| Spare / relay 2 | GPIO10 | Digital out | Optional |
| UART RX/TX | GPIO20/21 | UART | Debug header |

**Avoid:** GPIO2 (strapping), GPIO8 (onboard LED + strapping), GPIO9 (BOOT + strapping).

---

## 7. Mains and low-voltage wiring

### 7.1 Mains side

```
L ──[5 A fuse]──[16 A switch]──●
    ├── HLK L
    ├── ZMPT L
    ├── relay COM → relay NO → CT → socket L
    ├── MOV → N
    └── X2 → N

N ──●── HLK N
    ├── ZMPT N
    └── socket N (direct, or via second relay pole)

E ──●── socket E (direct, never switched)
```

Rules:

- Fuse on Live, before everything.
- MOV (275 VAC) and X2 capacitor across L–N after the fuse.
- ZMPT101B before the relay, so voltage is available when the relay is off.
- CT after the relay on Live, so it measures only the load.
- Earth continuous, never switched.
- Prefer a 2-pole relay to break both L and N. A 1-pole relay is acceptable only if the plug polarity is known and the neutral is not switched.
- Add a 0.5 A fuse for the HLK and a 0.1 A fuse for the ZMPT branch.
- Use 1.5 mm² wire for mains; 0.75 mm² for the HLK/ZMPT branches.
- Use a plastic enclosure with a barrier between mains and low voltage.
- Keep 6 mm creepage between mains and low-voltage conductors; use PCB slots.
- Cover all mains joints with heat-shrink; no exposed strands.

### 7.2 Low-voltage side

- HLK +5 V → ESP32 5V, sensor VCC, relay coil.
- HLK 0 V → ESP32 GND, sensor GND, relay driver GND.
- Do **not** connect the DC ground to mains neutral or earth. The HLK and sensors provide isolation; the DC side floats.
- Add a 1000 µF / 10 V electrolytic and a 100 nF ceramic on the 5 V rail near the ESP32.
- Add a 100 µF capacitor near the relay driver.
- Keep the ADC traces short and away from the relay, the HLK, and mains wiring.

### 7.3 Relay driver

```
GPIO3 ──[1k]── base S8050
                 │
               [10k]── GND
 emitter ── GND
 collector ── relay coil (−)
 relay coil (+) ── 5 V
 1N4007 across coil, cathode to +5 V
 100 nF across coil
```

- 10 kΩ pull-down on GPIO3 keeps the relay off during boot.
- Relay contact snubber: 100 Ω / 1 W + 100 nF X2 across the contacts.
- Use a relay rated at least 10 A / 250 VAC, even though the project is limited to 5 A.

---

## 8. Analog front-end

Each channel:

```
Sensor OUT ──[10k 1%]──●──[10k 1%]── GND
                       │
                     [1k]
                       │
ESP32 ADC ─────────────●──[10nF]── GND
                       │
                    BAT54S
                  pin 3 = signal
                  pin 1 = GND
                  pin 2 = 3.3V
```

- Divider ratio 2:1; 0–5 V → 0–2.5 V.
- 10 nF gives a pole around 3 kHz with the 5 kΩ source; good for 50 Hz and low-order harmonics.
- BAT54S clamps transients to the rails.
- Use 1% metal-film resistors; mismatch is removed by calibration.
- Do not add a separate bias network: the sensor output already carries the 2.5 V bias.

**Setting the sensor gain pots:**

1. Power the sensors from 5 V, no mains.
2. Measure each OUT pin with a multimeter; it should be about 2.5 V DC.
3. Apply the highest expected voltage/current.
4. Adjust the pot so the output peak stays below ~4.5 V (i.e., below 2.25 V at the ADC after the divider). This leaves headroom for mains surges and avoids clipping.
5. Re-check the divider output with an oscilloscope if available; it should be a clean sine centered at ~1.25 V.

> **Live-adjustment warning:** setting the voltage pot requires mains at the ZMPT input. Use an insulated trimmer tool, keep the mains terminals covered, and work through an RCD/isolation transformer. Never touch the module's input terminals while powered. If you cannot do this safely, set the pot to minimum gain and calibrate in software instead.

---

## 9. Power supply and relay

### 9.1 Power budget

| Load | Current |
|---|---|
| ESP32-C3 Wi-Fi peak | ~350 mA |
| ZMPT101B + ZMCT103C | ~30 mA |
| Relay coil | ~70 mA |
| OLED + buzzer | ~30 mA |
| **Total peak** | **~480 mA** |

The HLK-5M05 (1 A) has enough margin. The HLK-PM01 (0.6 A) is marginal; use the 1 A version.

### 9.2 Relay choice

- 10 A / 250 VAC sealed relay (e.g., Songle SRD-05VDC-SL-C for a single pole, or a 2-pole equivalent).
- For a 2-pole relay, switch L and N; keep E continuous.
- Add a snubber across the contacts to reduce arcing and EMI.
- The relay is the most likely part to fail/weld. Use a reputable brand and never exceed its rating.

### 9.3 Fusing

- Main: 5 A slow-blow (protects the CT rating and wiring).
- HLK branch: 0.5 A slow-blow.
- ZMPT branch: 0.1 A slow-blow (optional but recommended).
- The fuse is a safety device, not a current sensor. Size it for the wiring and the 5 A CT.

---

## 10. Firmware architecture

### 10.1 Stack

- **Framework:** Arduino on PlatformIO (easier than ESP-IDF for this project).
- **Board:** `esp32-c3-devkitm-1` or the Super Mini board definition.
- **Filesystem:** LittleFS for the dashboard, calibration backup, and history.
- **Libraries:**
  - `ESPAsyncWebServer` + `AsyncTCP`
  - `ArduinoJson`
  - `ArduinoOTA` (built into the ESP32 Arduino core)
  - `WiFiManager` (optional, for initial Wi-Fi setup)
  - `Preferences` (NVS, built in)
  - `LittleFS` (built in)
  - `PubSubClient` (optional MQTT)
- **Dashboard:** vanilla HTML/CSS/JS + Chart.js bundled locally. No build step.

### 10.2 Task model

The ESP32-C3 is single-core, so use FreeRTOS tasks carefully:

| Task | Priority | Purpose |
|---|---|---|
| `meterTask` | 10 | Sample ADC, compute RMS/P/PF/energy |
| `webTask` / `loop` | 1 | Async web server, SSE, OTA, MQTT |
| `buttonTask` | 2 | Debounce, short/long press |

- `meterTask` runs every 1 ms (v1) or uses the ADC continuous driver at 4–8 kHz (v2).
- Protect the shared `Measurement` struct with a mutex, or double-buffer it with an atomic index.
- Feed the task watchdog in the metering task. If it stalls for more than 5 s, reset.
- Relay defaults to OFF at boot. Do not restore the previous relay state automatically; a power cut should not re-energize a load unexpectedly.
- Enable the brown-out detector. Add a 10 kΩ pull-down on the relay driver.

### 10.3 Sampling strategy

**v1 (simple):**

- Every 1 ms, read voltage and current back-to-back.
- Each channel gets 1 kHz, i.e., 20 samples per 50 Hz cycle.
- Accumulate over a 200 ms window (10 cycles).
- Use raw `analogRead` for speed and apply a calibration curve; `analogReadMilliVolts` is convenient but slower.
- Remove the DC bias with a slow IIR filter (time constant ~1 s), or subtract the window mean.
- The V/I pair is separated by ~50–100 µs; calibrate the phase offset.

**v2 (better):**

- Use the ESP32-C3 `adc_continuous` driver with a pattern of the two ADC1 channels.
- Sample 8 kHz total (4 kHz per channel), 80 samples per cycle.
- Process blocks in the metering task; the hardware handles timing.
- This reduces jitter and phase error, and improves low-power-factor accuracy.

### 10.4 Metering math

For each 200 ms window:

```
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
- Clipping: the divider maps the sensor's 5 V maximum to about 2.5 V, or ~3100 ADC counts. If the raw ADC stays above ~3000 or below ~100, flag `sensor_clipped` and ask the user to reduce the pot gain. (The ~4000 count rail is the ADC's own limit, not the sensor's.)
- Window synchronization: a fixed 200 ms window is fine. For better results, start the window on a voltage zero crossing and use an integer number of cycles.

### 10.5 Protection logic

| Condition | Action |
|---|---|
| Irms > 5.0 A for 1 s | Trip relay, latch fault, show "overcurrent" |
| Vrms > 260 V for 1 s | Trip relay, latch fault, show "overvoltage" |
| Vrms < 180 V for 5 s | Warn only (brownout) |
| Sensor clipped | Warn, do not trip |
| Current > 0.2 A while relay commanded OFF | Warn: welded contact or miswired CT; do not re-energize |
| Temperature > 70 °C (optional NTC) | Trip relay |

A latched fault requires a manual reset from the dashboard or the button. Do not auto-retry an overcurrent fault.

### 10.6 Energy persistence

- Keep energy in RAM in Wh.
- Write to NVS when energy increases by 0.1 kWh, or every 15 minutes, whichever comes first.
- On boot, restore the last value. You may lose up to 0.1 kWh after a power cut.
- Avoid writing every second; NVS flash has limited erase cycles.
- Store daily energy separately and reset at local midnight using NTP time.

### 10.7 Calibration storage

Store in NVS:

```
v_cal, i_cal, phase_cal, v_offset, i_offset,
overcurrent_limit, overvoltage_limit, undervoltage_limit,
energy_kwh, day_energy_kwh, last_day,
wifi_ssid, wifi_pass, dashboard_user, dashboard_pass,
price_per_kwh
```

Provide a "factory reset" that clears calibration and Wi-Fi but preserves the energy counter if desired.

---

## 11. Web dashboard and API

### 11.1 Pages

- `/` — live dashboard.
- `/settings` — calibration, limits, price, Wi-Fi reset, OTA.
- `/history` — 24 h / 7 day charts.

### 11.2 Live data

Use **Server-Sent Events (SSE)** at `/events`, one JSON object every 500 ms:

```json
{
  "v": 230.4,
  "i": 1.23,
  "p": 281.5,
  "s": 283.4,
  "pf": 0.993,
  "hz": 50.01,
  "kwh": 12.345,
  "relay": true,
  "fault": null,
  "rssi": -62,
  "uptime": 12345
}
```

SSE is simpler than WebSocket and reconnects automatically. If no clients are connected, skip the send.

### 11.3 REST API

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/status` | Current measurement JSON |
| GET | `/api/history?hours=24` | Downsampled history |
| POST | `/api/relay` | `{"on": true}` |
| GET | `/api/config` | Calibration and limits |
| POST | `/api/config` | Update calibration and limits |
| POST | `/api/calibrate` | Offset / voltage / current / phase calibration |
| GET | `/api/health` | Heap, RSSI, uptime, reset reason |
| POST | `/api/ota` | Firmware upload |
| POST | `/api/reset` | Reboot |

Require HTTP Basic authentication for all `/api/*` routes and the dashboard. Change the default password on first use. Do not expose the device to the internet without a VPN; use Home Assistant or a local MQTT broker instead.

### 11.4 Dashboard layout

```
┌─────────────────────────────────────────────────────┐
│  Power Sense        ● Online        [ Relay ON ]    │
├─────────────┬─────────────┬─────────────┬───────────┤
│ 230.4 V     │ 1.23 A      │ 281.5 W     │ 0.993 PF  │
│ Voltage     │ Current     │ Power       │ Power fac.│
├─────────────┴─────────────┴─────────────┴───────────┤
│ 12.345 kWh        ₹ 98.76        50.01 Hz           │
├─────────────────────────────────────────────────────┤
│  Live chart: V / I / P (last 60 s)                  │
│  ───────────────────────────────────────────────    │
├─────────────────────────────────────────────────────┤
│  History: 24 h / 7 days        Settings   OTA       │
└─────────────────────────────────────────────────────┘
```

- Mobile-first CSS grid; large numbers; color-coded PF (green > 0.9, amber 0.7–0.9, red < 0.7).
- Chart.js with `animation: false`, 60 points at 1 Hz, and decimation for history.
- Show a red banner for faults: overcurrent, overvoltage, sensor clipping, Wi-Fi lost.
- Relay button requires a confirmation for "ON" if the load is unknown; "OFF" is immediate.
- Add a "last updated" timestamp and an offline badge.

### 11.5 Optional OLED

- 0.96" SSD1306 I2C, 128×64.
- SDA → GPIO5, SCL → GPIO6, VCC → 3.3 V.
- Show V, I, P, relay state; cycle pages every 3 s.
- Do not use GPIO8/9 for I2C; they are strapping pins.
- The OLED is optional; the web dashboard is the primary UI.

---

## 12. Calibration

### 12.1 Offsets

1. Relay off, no load, sensors powered.
2. Read raw ADC for 2 s on each channel.
3. Compute the mean; that is `vOffset` and `iOffset`.
4. Store in NVS.

### 12.2 Voltage calibration

1. Apply a known AC voltage (mains through an isolation transformer/variac, or a known low-voltage AC source).
2. Measure the true voltage with a CAT III multimeter.
3. Read `Vrms_raw` from the firmware's uncalibrated output.
4. `V_CAL = V_true / Vrms_raw`.
5. Store and verify at a second voltage.

### 12.3 Current calibration

1. Use a resistive load, e.g., a 1000 W heater (≈4.35 A at 230 V).
2. Measure the true current with a clamp meter.
3. Read `Irms_raw`.
4. `I_CAL = I_true / Irms_raw`.
5. Verify at a second load (e.g., 500 W).

### 12.4 Phase calibration

1. With the resistive load (PF ≈ 1.00), adjust `phaseSamples` until the dashboard reads PF = 1.00.
2. Check with an inductive load (fan, motor). The PF should be lower and plausible.
3. If PF reads negative, reverse the CT wire direction or invert the current sign in software.
4. Re-check power against a commercial meter.

### 12.5 Verification

| Test | Expected |
|---|---|
| No load, relay off | I ≈ 0, P ≈ 0, V ≈ mains |
| 60 W bulb | I ≈ 0.26 A, P ≈ 60 W, PF ≈ 1 |
| 1000 W heater | I ≈ 4.35 A, P ≈ 1000 W, PF ≈ 1 |
| Fan / motor | PF < 1, P < S |
| 1 kWh run | Energy within ±5% of a commercial meter |

Document the final constants and the calibration date in the repository. Recalibrate if you change the divider, sensor module, or potentiometer setting.

---

## 13. ESP8266 / ESP-12E migration

### 13.1 The hard limitation

The ESP8266 family has:

- One 10-bit ADC on A0.
- 0–1.0 V input range.
- No second channel.
- No simultaneous sampling.

You cannot connect both the voltage and current sensors to A0. The options are:

| Option | Cost | Accuracy | Complexity | Recommendation |
|---|---|---|---|---|
| ESP-12E + MCP3202 external ADC | Low | Good | Medium | Use if you must use ESP-12E |
| ESP-12E + PZEM-004T | Medium | Very good | Low | Simplest ESP8266 path |
| ESP-12E + CD4051 mux into A0 | Lowest | Poor | Medium | Not recommended |
| ESP32-C3-MINI-1 module | Low | Good | Low | **Best cost-down path** |

### 13.2 ESP-12E pin map

| Function | ESP-12E pin | Notes |
|---|---|---|
| Relay | GPIO5 | Active-high via NPN |
| Button | GPIO16 | To GND, pull-up |
| MCP3202 CS | GPIO4 | 10 kΩ pull-up |
| MCP3202 SCK | GPIO14 | |
| MCP3202 MOSI | GPIO13 | |
| MCP3202 MISO | GPIO12 | |
| UART debug | GPIO1 / GPIO3 | TX / RX |
| A0 | not used | 0–1 V only |
| Avoid | GPIO0, GPIO2, GPIO15 | Strapping pins |
| Avoid | GPIO6–GPIO11 | Internal flash |

MCP3202 wiring: VDD (which is also the ADC reference) to 3.3 V, VSS to GND, CH0 to the voltage divider, CH1 to the current divider, SPI to the pins above. The MCP3202 is a 12-bit, 2-channel SPI ADC (100 ksps at 5 V, 50 ksps at 3.3 V); sampling V and I within ~20 µs gives much better phase accuracy than the ESP8266 internal ADC.

### 13.3 Power for the ESP-12E

- ESP-12E is a 3.3 V module; the HLK-5M05 is 5 V.
- Add an AMS1117-3.3 (or a 3.3 V HLK module) with 10 µF input/output capacitors.
- Keep the 5 V rail for the sensors and relay.
- The ESP-12E is 2 mm pitch; use an adapter or a custom PCB.

### 13.4 Flash size

- ESP8285 has 1 MB flash; OTA + LittleFS dashboard is tight.
- ESP-12E/ESP-12F has 4 MB; much more comfortable.
- If you use 1 MB, serve a minimal dashboard and skip OTA, or use a compressed single-page UI.

### 13.5 Recommendation

**Use an ESP32-C3-MINI-1 module for the cost-reduced version.** It keeps the 12-bit ADC, the 5 ADC1 channels, the RISC-V core, and the Wi-Fi stack. It removes the dev board's USB connector and headers, which is where most of the cost is. The ESP-12E path saves a little on the module but adds an external ADC, a 3.3 V regulator, 2 mm pitch assembly, and a much weaker analog front-end.

If the goal is the absolute lowest cost and you can accept lower accuracy, the ESP-12E + MCP3202 is workable. If the goal is the least work, ESP-12E + PZEM-004T is better. Do not use the ESP8266's A0 for both channels.

---

## 14. PCB and enclosure

### 14.1 Prototype

- Perfboard or a custom carrier board for the ESP32-C3 Super Mini.
- Keep the mains section on a separate board or a clearly barriered area.
- Use screw terminals for L/N/E in and out; use 5.08 mm pitch.
- Use a fuse holder, MOV, X2 cap, HLK module, relay, and terminal blocks.
- Mount the CT so the Live wire passes through it cleanly.
- Use a plastic enclosure with a removable lid and cable glands.

### 14.2 Custom PCB

- 2-layer, 1.6 mm, 2 oz copper for the mains traces.
- Mains trace width ≥ 2 mm; clearance ≥ 3 mm between L and N; ≥ 6 mm between mains and low voltage.
- Add isolation slots under the relay and HLK.
- Keep the ADC traces short, guarded by ground, and away from the relay/HLK.
- Add a ground plane under the low-voltage section only.
- Add mounting holes, a fuse holder, and a silkscreen warning.
- Consider a 3.3 V LDO and a 5 V rail for the ESP32-C3 module.
- Add test points for 5 V, 3.3 V, GND, V_ADC, I_ADC, relay drive.

### 14.3 Enclosure

- Plastic, flame-retardant (ABS/PC), with a barrier between mains and low voltage.
- Cable glands and strain relief on the input cable.
- Earth bond to the socket earth bus; use a star washer and a dedicated earth terminal.
- Label: "230 V AC — 5 A MAX — DO NOT OPEN — NO USER SERVICEABLE PARTS".
- Keep the ESP32 antenna away from metal and mains wiring; do not put the board inside a metal box.
- Add ventilation if the relay or HLK gets warm.

---

## 15. Test plan

### Stage 0 — Bench inspection (no mains)

- [ ] Check for shorts between 5 V, 3.3 V and GND.
- [ ] Check the divider outputs with a multimeter.
- [ ] Check the relay driver with a dummy coil or LED.
- [ ] Check the CT and ZMPT wiring; no mains connected.
- [ ] Verify the ESP32 boots, Wi-Fi connects, dashboard loads.

### Stage 1 — 5 V power

- [ ] Apply 5 V from a bench supply with a current limit.
- [ ] Measure 5 V and 3.3 V rails.
- [ ] Check current draw: ~100–200 mA idle, <500 mA peak.
- [ ] Check the relay switches with a 5 V coil and a dummy load.

### Stage 2 — ADC and divider

- [ ] Inject 0.5 V, 1.0 V, 2.0 V, 3.0 V, 4.0 V, 4.8 V DC into each divider input; record ADC counts (expect about half the input).
- [ ] Check linearity and saturation; the divider output must stay below 2.5 V at 5 V input.
- [ ] Inject a 50 Hz sine (1 Vpp, 1.25 V offset) from a function generator; verify RMS and frequency.
- [ ] Check the BAT54S clamps at ~3.6 V by injecting 5 V through a 10 kΩ resistor directly at the ADC node (current-limited).

### Stage 3 — Sensors (low voltage / isolation transformer)

- [ ] Power the sensors from 5 V; measure ~2.5 V bias on each OUT.
- [ ] Apply a known AC voltage (variac/isolation transformer); record the output.
- [ ] Pass a known current through the CT (e.g., a 100 W bulb); record the output.
- [ ] Adjust the pots for a good signal without clipping.
- [ ] Check linearity at 50 V, 100 V, 150 V, 200 V, 230 V.
- [ ] Check current linearity at 0.5 A, 1 A, 2 A, 3 A, 4 A.

### Stage 4 — Relay and load (mains, with RCD)

- [ ] Use an RCD/GFCI and an isolation transformer for the first mains test.
- [ ] Start with a 40 W bulb.
- [ ] Switch the relay 50 times; check the ESP32 does not reset.
- [ ] Check the snubber and flyback diode temperature.
- [ ] Then use a 1000 W heater; check the current reading against a clamp meter.
- [ ] Check the terminal and relay temperature after 30 minutes.

### Stage 5 — Accuracy

- [ ] Compare V, I, P, PF with a commercial energy meter.
- [ ] Run a 1 kWh load and compare energy.
- [ ] Test at low PF (fan/motor) and check the phase calibration.
- [ ] Test at no load and check the zero offset.
- [ ] Test with a dimmer or SMPS load; note the waveform distortion and accuracy.

### Stage 6 — Soak and safety

- [ ] 24 h run at 500 W; check memory, Wi-Fi reconnect, energy accumulation.
- [ ] 1 h run at 4.5 A; check temperatures (<60 °C at terminals, <70 °C at relay).
- [ ] Fuse test: do not intentionally blow a fuse; verify the rating and holder.
- [ ] Earth continuity: <0.1 Ω from the earth pin to the enclosure/socket earth.
- [ ] Insulation resistance: use a proper insulation tester (500 V DC) between mains and low voltage; target >1 MΩ (preferably >10 MΩ).
- [ ] Strain relief: pull test on the input cable.
- [ ] Final visual inspection: no exposed mains, no loose strands, no damaged insulation.

**Pass criteria:** V ±2%, I ±5%, P ±5% at PF > 0.8; energy ±5% over 1 kWh; no resets during relay switching; no overheating; all safety checks pass.

---

## 16. Firmware file structure

```
power-sense/
├── platformio.ini
├── data/
│   ├── index.html
│   ├── app.js
│   ├── style.css
│   └── chart.umd.min.js
├── src/
│   ├── main.cpp
│   ├── config.h
│   ├── metering.h / metering.cpp
│   ├── sensors.h / sensors.cpp
│   ├── relay.h / relay.cpp
│   ├── web.h / web.cpp
│   └── storage.h / storage.cpp
└── docs/
    ├── PLAN.md
    ├── PINOUT.md
    ├── BOM.csv
    └── SAFETY.md
```

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

> `ArduinoOTA` is built into the ESP32 Arduino core; do not add it as a library. Package names change, so check the current PlatformIO registry. For ESP8266, use `ESPAsyncTCP` and the ESP8266 AsyncWebServer fork.

---

## 17. Code skeleton

### 17.1 Configuration

```cpp
// config.h
#pragma once
#include <Arduino.h>

constexpr int PIN_V       = 0;   // ADC1_CH0
constexpr int PIN_I       = 1;   // ADC1_CH1
constexpr int PIN_RELAY   = 3;
constexpr int PIN_BUTTON  = 4;
constexpr int PIN_SDA     = 5;
constexpr int PIN_SCL     = 6;
constexpr int PIN_BUZZER  = 7;

constexpr uint32_t WINDOW_MS = 200;

// Defaults only. Calibrate every unit.
// Both are physical units per 12-bit ADC count (3.3 V reference).
// The example assumes the pots give 2.5 V peak at 230 V / 5 A,
// i.e. about 1100 RMS counts after the 2:1 divider.
constexpr float V_CAL_DEFAULT = 0.21f;    // V RMS per ADC count
constexpr float I_CAL_DEFAULT = 0.0046f;  // A RMS per ADC count
constexpr float PHASE_CAL_DEFAULT = 0.0f; // samples

constexpr float OVERCURRENT_A = 5.0f;
constexpr float OVERVOLTAGE_V = 260.0f;
constexpr float UNDERVOLTAGE_V = 180.0f;
```

Call `analogReadResolution(12)` and `analogSetAttenuation(ADC_11db)` in `setup()` before starting the metering task, so both channels use the 0–2.5 V range described above.

### 17.2 Metering task (simplified)

```cpp
// metering.cpp — core idea, not production-ready
#include <Arduino.h>
#include "config.h"

struct Measurement {
  float v, i, p, s, pf, hz, kwh;
  bool relay;
  const char* fault;
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
    // Read back-to-back to minimize phase error.
    int rawV = analogRead(PIN_V);
    int rawI = analogRead(PIN_I);

    // Simple DC tracker; replace with a calibrated curve if needed.
    vOff += (rawV - vOff) * 0.0005f;
    iOff += (rawI - iOff) * 0.0005f;

    float v = rawV - vOff;
    float i = rawI - iOff;
    sumV2 += v * v;
    sumI2 += i * i;
    sumVI += v * i;
    n++;

    if (millis() - t0 >= WINDOW_MS) {
      float vRmsRaw = sqrtf(sumV2 / n);
      float iRmsRaw = sqrtf(sumI2 / n);
      float pRaw    = sumVI / n;

      float vRms = vRmsRaw * V_CAL_DEFAULT;
      float iRms = iRmsRaw * I_CAL_DEFAULT;
      float p    = pRaw * V_CAL_DEFAULT * I_CAL_DEFAULT;
      float s    = vRms * iRms;

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

    vTaskDelay(pdMS_TO_TICKS(1)); // 1 kHz per channel; use ADC continuous for 4 kHz
  }
}
```

This skeleton shows the structure, not the final implementation. The production version needs:

- Calibrated ADC conversion (raw counts → mV).
- Phase correction with a circular buffer.
- Frequency measurement.
- Clipping detection.
- Protection trip logic.
- Energy persistence.
- Mutex-protected snapshots for the web task.
- Watchdog feeding.

### 17.3 Relay control

```cpp
void setRelay(bool on) {
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  gRelayOn = on;
}
```

- Initialize GPIO3 as output and write LOW **before** setting it as output, to avoid a glitch.
- Add a 10 kΩ pull-down.
- On boot, relay is off.

### 17.4 SSE endpoint (ESPAsyncWebServer)

```cpp
AsyncEventSource events("/events");

void onEventsConnect(AsyncEventSourceClient* client) {
  client->send("retry: 2000\n\n", nullptr, 0, 0);
}

// In loop or a timer:
if (events.count()) {
  char buf[256];
  Measurement m = snapshot();
  snprintf(buf, sizeof(buf),
    "data: {\"v\":%.1f,\"i\":%.2f,\"p\":%.1f,\"pf\":%.3f,"
    "\"kwh\":%.3f,\"relay\":%s}\n\n",
    m.v, m.i, m.p, m.pf, m.kwh, m.relay ? "true" : "false");
  events.send(buf, "message", millis());
}
```

### 17.5 API example

```cpp
server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
  Measurement m = snapshot();
  AsyncResponseStream* res = req->beginResponseStream("application/json");
  res->printf("{\"v\":%.2f,\"i\":%.3f,\"p\":%.1f,\"s\":%.1f,"
              "\"pf\":%.3f,\"kwh\":%.4f,\"relay\":%s}",
              m.v, m.i, m.p, m.s, m.pf, m.kwh,
              m.relay ? "true" : "false");
  req->send(res);
});
```

---

## 18. Milestones

| Week | Deliverable |
|---|---|
| 1 | Safety setup, BOM, sensor characterization, breadboard 5 V rails |
| 2 | Divider + ADC test, metering math on the bench, calibration with a function generator |
| 3 | Mains wiring on an isolation transformer, relay test, first V/I/P readings |
| 4 | Web dashboard, SSE, relay control, OTA, settings page |
| 5 | Calibration with real loads, energy logging, 24 h soak, fixes |
| 6 | Enclosure, final safety inspection, documentation; optional PCB |
| 7+ | Cost-down prototype (C3 module or ESP-12E + MCP3202/PZEM) |

---

## 19. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Electric shock | Enclosure, fuse, MOV, isolation transformer, RCD, one-hand rule, no live work |
| 5 A overload | 5 A fuse, software trip, label, derate to 4 A |
| CT saturation | Do not exceed 5 A; use a 20 A CT for larger loads |
| ADC clipping | 2:1 divider, pot adjustment, BAT54S clamp, clipping flag |
| Relay EMI resets ESP32 | Flyback diode, snubber, 1000 µF bulk cap, keep ADC wires away |
| Relay welding | Quality relay, snubber, fuse, replace after many cycles |
| Wi-Fi instability | Test the Super Mini early; keep antenna clear; use a plastic enclosure |
| Flash wear | Persist energy every 0.1 kWh/15 min, not every second |
| Calibration drift | Use 1% resistors, lock the pots, recalibrate periodically |
| ESP8266 ADC limitation | Use MCP3202 or PZEM; prefer C3 module for cost-down |
| USB + mains | Disconnect mains before USB; use OTA; label the enclosure |
| Regulatory | Do not sell; use a certified smart plug for production |

---

## 20. Open questions

1. **One relay for the whole strip or one per socket?** Per-socket needs multiple relays, multiple GPIOs, and preferably multiple CTs or a multiplexed measurement.
2. **Is 5 A / ~1 kW enough?** If you need 16 A, change the CT (20 A), relay (16 A), fuse (16 A), and wiring.
3. **Do you want a local OLED?** It adds GPIO5/6 and a 3.3 V load, but is useful when Wi-Fi is down.
4. **Do you want MQTT/Home Assistant?** It adds a broker dependency but gives history and automation.
5. **Enclosure type?** A 4-socket plastic strip with a separate mains compartment is easiest; a 3D-printed box needs flame-retardant filament and careful spacing.
6. **Single-phase only?** Yes for this design; three-phase needs different sensors and math.

---

## 21. References

- Robu — 5 A single-phase AC current sensor module (ZMCT103C): <https://robu.in/product/5a-range-single-phase-ac-current-sensor-module/>
- Robu — ZMPT101B AC voltage sensor module: <https://robu.in/product/ac-voltage-sensor-module-zmpt101b-single-phase/>
- QuartzComponents — ESP32-C3 Super Mini HW-466AB: <https://quartzcomponents.com/products/esp32-c3-super-mini-development-board-with-soldered-headers-hw-466ab>
- Espressif — ESP32-C3 datasheet: <https://documentation.espressif.com/esp32-c3_datasheet_en.pdf>
- Random Nerd Tutorials — Getting Started with the ESP32-C3 Super Mini: <https://randomnerdtutorials.com/getting-started-esp32-c3-super-mini/>
- Last Minute Engineers — ESP32-C3 Super Mini pinout: <https://lastminuteengineers.com/esp32-c3-super-mini-pinout-reference/>
- OpenEnergyMonitor — EmonLib (RMS/real-power concepts): <https://github.com/openenergymonitor/EmonLib>
- ZMCT103C datasheet: <https://innovatorsguru.com/wp-content/uploads/2019/08/zmct103C-datasheet.pdf>
- MCP3202 datasheet: <https://ww1.microchip.com/downloads/en/DeviceDoc/21034D.pdf>
- HLK-5M05 datasheet: search “Hi-Link HLK-5M05 datasheet”
- PZEM-004T v3 datasheet: search “PZEM-004T v3.0 Modbus”
