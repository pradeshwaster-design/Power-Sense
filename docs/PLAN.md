# Power Sense — Design & Test Plan

**Repository:** <https://github.com/pradeshwaster-design/Power-Sense>

**Goal:** a simple, self-contained 230 V AC energy monitor. It measures RMS voltage, RMS current and real power, serves a local WiFi dashboard and stores its calibration in flash.

> **Mains voltage is lethal.** Read [SAFETY.md](SAFETY.md) before connecting anything.

---

## 1. Scope

### Included

- AC voltage, current and real power.
- Self-hosted WiFi dashboard at `192.168.4.1`.
- Hidden calibration page, saved to NVS.
- Idle-noise removal, smoothing and warm-up.
- Serial output for debugging.
- One 5 V USB supply.

### Not included

- **No relay.** The strip is live whenever plugged in; use its mechanical switch.
- **No display, no push button.**
- No power factor, energy (kWh) or history logging.
- No router, cloud, app or MQTT.
- No per-socket control.

---

## 2. Limits

| Item | Limit |
|---|---|
| Mains | 230 V AC, 50 Hz, single phase |
| Maximum current | **5 A** (CT rating) |
| Continuous current | **4 A** recommended |
| Maximum load | ≈ 1150 W at 230 V; derate to ≈ 920 W continuous |
| ADC | ESP32-C3, 12-bit, 11 dB attenuation |
| Sampling window | 200 ms (10 cycles at 50 Hz) |
| Accuracy | roughly ±5% after calibration; worse at very small loads |

The current transformer is the limiting part. Above 5 A it saturates and the readings become meaningless.

---

## 3. Architecture

```text
230 V AC ──[5 A fuse]──┬── ZMPT101B ── GPIO3
                       ├── CT (Live only) ── bias + 1k ── GPIO4
                       └── power strip sockets (always live)

USB-C 5 V charger ──► ESP32-C3 Super Mini
ESP32-C3 3V3 ───────► ZMPT101B and CT bias divider
ESP32-C3 ──WiFi AP──► phone / laptop dashboard
```

- The ZMPT101B is across Live and Neutral, before the strip switch, so voltage is available even when the strip is off.
- The CT surrounds the Live wire feeding the sockets, so it measures only the load current.
- The CT is passive: its output must be biased to **1.65 V** before the ADC can read both half-cycles.

---

## 4. Hardware

| Part | Qty | Notes |
|---|---|---|
| ESP32-C3 Super Mini | 1 | USB-C, 4 MB flash |
| ZMPT101B module | 1 | 3.3 V, 3-pin |
| 5 A passive CT module | 1 | 2-pin `G`/`S`, no VCC |
| 10 kΩ 1% | 2 | 1.65 V bias divider |
| 1 kΩ 1% | 1 | CT series protection |
| 22 µF 16 V | 1 | Bias decoupling |
| 100 nF | 2 | Optional ADC filters |
| 150 Ω | 1 | Only if the CT has no burden |
| 5 A fuse + holder | 1 | Mains protection |
| Enclosure and wiring | — | Insulated, mains-rated |

Full prices: [BOM.csv](BOM.csv).

---

## 5. Analog front-end

### 5.1 Voltage channel

```text
ZMPT101B VCC ── 3V3
         GND ── GND
         OUT ── GPIO3 (ADC1_CH3)
```

- Output is biased at VCC/2, about **1.65 V**.
- The trimpot sets the gain. Set it once so the signal is clean and the raw count is roughly **400–900** at normal mains.
- Do not exceed 3.3 V at GPIO3.

### 5.2 Current channel

```text
3V3 ──[10k]──┬──────────────► CT G (bias ≈ 1.65 V)
             │
           [10k]
             │
            GND

CT S ──[1k]──┬──────────────► GPIO4 (ADC1_CH4)
             │
          [100 nF] (optional)
             │
            GND
```

- The CT is a passive transformer; it has no VCC and its output is AC.
- The 10 kΩ/10 kΩ divider biases `G` at **1.65 V**, so the ADC can read the full waveform.
- **`G` is not ground.** Grounding it clips the negative half-cycle.
- The 22 µF capacitor stabilises the bias node.
- The 1 kΩ series resistor protects the ADC; the optional 100 nF filters noise.

### 5.3 Burden resistor

With the mains disconnected, measure across `S` and `G`:

- A few hundred ohms or less: the module already has a burden. Do not add one.
- Open or very high: fit **150 Ω** across `S` and `G`.

Never leave the CT secondary open while current flows.

---

## 6. Firmware

### 6.1 Stack

- Arduino framework, ESP32 core.
- Built-in `WiFi`, `WebServer` and `Preferences`.
- Board: `esp32-c3-devkitm-1` (works for the Super Mini).
- Source: [../firmware/power_sense/power_sense.ino](../firmware/power_sense/power_sense.ino).

### 6.2 Behaviour

| Item | Value |
|---|---|
| Window | 200 ms (10 cycles at 50 Hz) |
| Buffer | Up to 3000 sample pairs |
| Update rate | Every ~800 ms |
| Display smoothing | Low-pass, α = 0.3 |
| Warm-up | 5 s before the filter starts |
| Dead-bands | V < 20 V, I < 0.03 A → zero |

### 6.3 Measurement

```text
vOff = mean(v);  iOff = mean(i)
Vrms = sqrt(mean((v - vOff)^2)) * V_CAL
Irms = sqrt(mean((i - iOff)^2)) * I_CAL
P    = |mean((v - vOff) * (i - iOff))| * V_CAL * I_CAL
```

Noise is removed in quadrature:

```text
signal = sqrt(max(0, raw² - noise²))
```

### 6.4 Network

| Route | Purpose |
|---|---|
| `/` | Dashboard |
| `/data` | JSON readings |
| `/cal` | Calibration page, unlinked |
| `/setcal` | Save new factors |
| `/zero` | Record idle noise |
| `/reset` | Restore defaults |

AP: `PowerSense` / `12345678`, address `192.168.4.1`.

### 6.5 Persistence

`V_CAL`, `I_CAL`, `V_NOISE` and `I_NOISE` are stored in the `ps` NVS namespace and reloaded at boot. A re-upload does not clear them; use **Reset to defaults** to restore.

---

## 7. Calibration

### 7.1 Noise

1. Power the board with the CT empty and the mains **off**.
2. Open `/cal` and tap **Zero idle noise**.
3. The current raw RMS values are stored as `V_NOISE` and `I_NOISE`.

### 7.2 Voltage

1. Measure the real mains voltage with a multimeter.
2. Enter it in *Real voltage* and save.
3. The firmware sets `V_CAL = real / vCs`.
4. Verify against a second measurement.

### 7.3 Current

1. Pass a known resistive load (60 W or 100 W bulb) through the CT.
2. Real current = `watts ÷ volts`.
3. Enter it in *Real current* and save.
4. The firmware sets `I_CAL = real / iCs`.
5. Verify with a second load.

### 7.4 Checks

| Test | Expected |
|---|---|
| Mains off | 0 V, 0 A, 0 W |
| 60 W bulb | ≈ 0.26 A, ≈ 60 W |
| 100 W bulb | ≈ 0.44 A, ≈ 100 W |
| 1000 W heater | ≈ 4.3 A, ≈ 1000 W |
| Small load (< 20 W) | Reads poorly; this is expected |

---

## 8. Test plan

### Stage 0 — Bench, no mains

- [ ] Check for shorts between 3.3 V and GND.
- [ ] Bias node and ZMPT OUT measure about 1.65 V.
- [ ] CT `S` sits at about 1.65 V through the 1 kΩ resistor.
- [ ] Burden resistor present.
- [ ] No exposed mains conductors.

### Stage 1 — Boot and network

- [ ] Board boots; Serial prints at 115200.
- [ ] Hotspot `PowerSense` appears.
- [ ] Dashboard loads at `192.168.4.1`.
- [ ] `/data` returns JSON.
- [ ] `/cal` loads.

### Stage 2 — Signals

- [ ] Raw voltage count is in the expected range at 230 V.
- [ ] Raw current count rises with load.
- [ ] No clipping at the top of the waveform.
- [ ] Readings fall to zero with the load off.

### Stage 3 — Low voltage / isolation transformer

- [ ] Apply a known AC voltage; adjust the ZMPT trimpot.
- [ ] Pass a known current through the CT; check linearity.
- [ ] Calibrate V and I.
- [ ] Recheck at a second point.

### Stage 4 — Mains and load

- [ ] Use an RCD and, preferably, an isolation transformer.
- [ ] Start with a 40 W bulb.
- [ ] Then a 1000 W heater; compare with a clamp meter.
- [ ] Check terminal and sensor temperature after 30 minutes.

### Stage 5 — Accuracy and soak

- [ ] Compare V, I and P with a commercial meter.
- [ ] Run 1 hour at 4 A; terminals below 60 °C.
- [ ] Run 24 hours at 500 W; check WiFi reconnects.
- [ ] Earth continuity < 0.1 Ω.
- [ ] Insulation resistance > 1 MΩ (preferably > 10 MΩ).

**Pass criteria:** V and I within ±5% after calibration, stable readings, no overheating, all safety checks pass.

---

## 9. Safety

- Mains is lethal. Unplug before touching anything.
- Fuse Live; keep Earth continuous.
- Keep at least 6 mm between mains and low voltage.
- Use an insulated enclosure and strain relief.
- Use a reputable 5 V charger; disconnect mains before connecting a laptop.
- The CT goes around **Live only**; never open its secondary under load.
- Do not exceed 5 A (4 A continuous).
- The strip is always live — there is no relay.
- Do not sell or permanently install an uncertified prototype.

Full checklist: [SAFETY.md](SAFETY.md).

---

## 10. Limitations and next steps

### Limitations

- ESP32-C3 ADC is noisy and non-linear; small loads read poorly.
- Sequential sampling adds a phase error; power factor is not displayed.
- No energy logging, history or cloud.
- `/cal` is unprotected.
- Hobby-grade, not revenue-grade.

### Next steps

1. Station mode for home WiFi.
2. kWh accumulation and history charts.
3. MQTT / Home Assistant.
4. External ADC (ADS1115) or an energy IC (ATM90E26) for accuracy.
5. Password protection and a 3D-printed enclosure.

---

## 11. References

- ZMPT101B module: <https://robu.in/product/ac-voltage-sensor-module-zmpt101b-single-phase/>
- 5 A AC current sensor module: <https://robu.in/product/5a-range-single-phase-ac-current-sensor-module/>
- ESP32-C3 Super Mini: <https://quartzcomponents.com/products/esp32-c3-super-mini-development-board-with-soldered-headers-hw-466ab>
- ESP32-C3 datasheet: <https://documentation.espressif.com/esp32-c3_datasheet_en.pdf>
- ZMCT103C datasheet: <https://innovatorsguru.com/wp-content/uploads/2019/08/zmct103C-datasheet.pdf>
- OpenEnergyMonitor EmonLib: <https://github.com/openenergymonitor/EmonLib>
- Arduino WiFi library: <https://docs.arduino.cc/libraries/wifi/>
- Arduino WebServer library: <https://docs.arduino.cc/libraries/webserver/>
