# Power Sense — Build Plan

**Repository:** <https://github.com/pradeshwaster-design/Power-Sense>

**Goal:** Power Sense is a simple, single-phase 230 V AC, 50 Hz energy monitor. It measures voltage, current, real power, apparent power and power factor. It runs on an ESP32-C3 Super Mini with a ZMPT101B voltage sensor, a ZMCT103C 5 A current sensor and a 5 V 2 A USB charger.

> **This is a mains-voltage project.** Read [SAFETY.md](SAFETY.md) completely before connecting anything to mains. If you are not confident working with 230 V AC, use a certified energy-monitoring plug instead.

---

## 1. Scope

### Included

- AC voltage and current measurement.
- Real power, apparent power and power factor.
- Serial output today; a local web dashboard next.
- Simple power: one 5 V 2 A charger, sensors fed from the C3 3.3 V rail.
- No analog divider on the sensor outputs.

### Not included

- **No relay.** Power Sense does not switch the load. The strip is live whenever it is plugged in; use the strip's own mechanical switch.
- **No display.** All readings go to serial and, later, the web dashboard.
- **No push button.** Configuration is by editing constants or the web page.
- **No cloud service or MQTT** in the first version.

---

## 2. Limits

| Item | Limit |
|---|---|
| Mains | 230 V AC, 50 Hz, single phase |
| Maximum current | 5 A (ZMCT103C rating) |
| Continuous current | 4 A recommended |
| Maximum load | ≈ 1150 W at 230 V; derate to ≈ 920 W continuous |
| Sensor supply | 3.3 V from the C3 regulator |
| Sensor output | 0–1.8 V, biased at about 1.65 V |
| ADC | 12-bit, 3.3 V maximum, 11 dB attenuation |
| Accuracy | ±5% after calibration; the 3.3 V headroom limits precision |

The 3.3 V supply is the main limitation. The LM358-type op-amp inside both modules is not rail-to-rail, so the output saturates near **1.8 V**. With a 1.65 V bias there is only about **0.15 V** of usable positive swing. Readings are usable for monitoring but not for billing.

---

## 3. System architecture

```text
230 V AC ──[5 A fuse]──┬── ZMPT101B ── GPIO3
                       ├── CT ── ZMCT103C ── GPIO4
                       └── power strip sockets (always live)

5 V 2 A charger ──> ESP32-C3 5V pin
ESP32-C3 3V3 ─────> ZMPT101B VCC and ZMCT103C VCC
ESP32-C3 GND ─────> sensor GND

ESP32-C3 ──Wi-Fi──> phone / PC dashboard (next milestone)
```

The voltage sensor is connected across Live and Neutral, before the strip's switch, so it measures mains voltage even when the strip is off. The current transformer is around the Live wire feeding the sockets, so it measures only the load current.

---

## 4. Hardware

See [BOM.csv](BOM.csv) for the full list. The essential parts:

| Part | Approx. price (INR) | Notes |
|---|---|---|
| ESP32-C3 Super Mini | 235 | HW-466AB, USB-C |
| ZMPT101B module | 180 | 0–250 V AC |
| ZMCT103C 5 A module | 180 | 5 A / 5 mA, 1000:1 |
| 5 V 2 A USB charger | 300 | Reputable brand, double-insulated |
| USB-C cable | 100 | Programming only |
| 100 nF capacitors (×4) | 20 | ADC filters and decoupling |
| 100 µF electrolytic | 10 | 3.3 V rail |
| 5 A fuse + holder | 80 | Load protection |
| Enclosure and wiring | 590 | Box, terminals, wire, glands |
| **Approx. total** | **≈ 1,875** | Prices vary; shipping extra |

### Board choice

Use the **ESP32-C3 Super Mini**. It has a 12-bit ADC, the ADC1 channels needed on GPIO3 and GPIO4, Wi-Fi, USB-C and easy programming.

Do not substitute an ESP8266/ESP-12E in this build. It has one 10-bit ADC with a 0–1 V range and cannot read both sensors without extra hardware.

---

## 5. Pinout

Full details and sensor pinouts: [PINOUT.md](PINOUT.md).

| Function | ESP32-C3 pin | Direction | Notes |
|---|---|---|---|
| Voltage sense | **GPIO3** | Analog in | ADC1_CH3 |
| Current sense | **GPIO4** | Analog in | ADC1_CH4 |
| 5 V supply | **5V** | Power in | From the charger |
| 3.3 V supply | **3V3** | Power out | Both sensor VCC pins |
| Ground | **GND** | Power | Common ground |

Only **5V, GND, 3V3, GPIO3 and GPIO4** are needed.

Pin rules:

- Use ADC1 only: GPIO0–GPIO4. This build uses GPIO3 and GPIO4.
- Avoid GPIO5 (ADC2): readings are unreliable while Wi-Fi is active.
- Avoid GPIO8 and GPIO9: strapping pins; GPIO8 also drives the onboard LED.
- Avoid GPIO20/GPIO21: UART.
- Avoid GPIO2 unless necessary: it is also a strapping pin.
- GPIO18/GPIO19 are USB D-/D+.

---

## 6. Power

### 6.1 Supply

- Use a **5 V 2 A USB charger**.
- Feed the ESP32-C3 through its **5V pin** (or USB-C).
- Take **3.3 V** and **GND** from the C3 header and feed both sensor modules.
- The sensors draw only a few mA. The ESP32 Wi-Fi peak is about 350 mA; the onboard regulator covers both.

### 6.2 Power rules

- Use a reputable, double-insulated charger.
- Plug the charger into a **separate outlet** from the monitored strip for the cleanest readings.
- Do **not** connect the charger and a PC USB cable at the same time unless the board has a back-feed diode. Unplug the external 5 V and mains before programming.
- Add a **100 µF** electrolytic and a **100 nF** ceramic capacitor across 3.3 V/GND near the sensors.
- Do not power the sensor modules from 5 V without the divider described in section 7.3.
- If the board browns out under Wi-Fi load, use a better 5 V supply or a board with a stronger regulator.

### 6.3 Measured budget

| Load | Current |
|---|---|
| ESP32-C3 idle | ≈ 40 mA |
| ESP32-C3 Wi-Fi peak | ≈ 350 mA |
| ZMPT101B | ≈ 5–10 mA |
| ZMCT103C | ≈ 5–10 mA |
| **Total peak** | **≈ 380 mA** |

That is comfortably inside a 2 A charger and inside a 500 mA 3.3 V regulator.

---

## 7. Analog front-end

### 7.1 No divider at 3.3 V

Both modules bias their output at VCC/2. At 3.3 V:

- Bias: **1.65 V**
- Maximum output: about **1.8 V** (LM358 headroom)
- Usable swing: about **±0.15 V**
- ESP32-C3 ADC maximum: 3.3 V

The output is therefore inside the ADC range and **no divider is needed**.

```text
Sensor OUT ──●── GPIO3 / GPIO4
             │
          [100 nF]
             │
            GND
```

The 100 nF capacitor should sit close to the ESP32 pin. It filters switching noise and charger ripple.

### 7.2 Setting the gain

1. Power the sensors from 3.3 V with **no mains**.
2. Measure each OUT pin: about **1.65 V DC**.
3. Fit the firmware and open the serial monitor.
4. Turn both trimpots fully down, then increase slowly.
5. Watch the raw ADC maximum while increasing the load.
6. Stop before the maximum flattens out; that is op-amp clipping.

Because the positive swing is the limiting side, a clean negative peak does not prove the channel is clean.

### 7.3 5 V fallback

Use this only if the 3.3 V signal is too small or the current channel clips too early.

```text
Sensor OUT ──[10k 1%]──●──[20k 1%]── GND
                       │
GPIO3/GPIO4 ───────────●──[100 nF]── GND
                       │
                    BAT54S (optional)
                  pin 3 = signal
                  pin 1 = GND
                  pin 2 = 3V3
```

- Power both sensors from **5V**.
- The LM358 output at 5 V tops out near 3.5 V, so the divider gives about **2.3 V maximum** at the pin.
- That is inside the ADC's usable range.
- Recalibrate `V_CAL` and `I_CAL` after the change.

---

## 8. Firmware

### 8.1 Stack

- **Arduino** on PlatformIO (or the Arduino IDE).
- **Board:** `esp32-c3-devkitm-1` (works for the Super Mini).
- **Sketch:** [`firmware/power_sense/power_sense.ino`](../firmware/power_sense/power_sense.ino).
- **Project file:** [`firmware/platformio.ini`](../firmware/platformio.ini).

### 8.2 Sampling

- `analogReadResolution(12)`.
- `analogSetPinAttenuation(pin, ADC_11db)` for both channels.
- **400 sample pairs** at a fixed **100 µs** spacing.
- Window: **40 ms**, about two 50 Hz cycles.
- The two channels are read sequentially, so current lags voltage by roughly 40–50 µs. That is under 1° at 50 Hz and is acceptable for monitoring.
- The sketch paces samples with `micros()` so the window stays close to 40 ms. If `analogRead` is slower than the slot, increase `SAMPLE_US`.

### 8.3 Metering math

For each window:

```text
vOff = mean(v);  iOff = mean(i)
Vrms = sqrt(mean((v - vOff)^2)) * V_CAL
Irms = sqrt(mean((i - iOff)^2)) * I_CAL
P    = mean((v - vOff) * (i - iOff)) * V_CAL * I_CAL
S    = Vrms * Irms
PF   = P / S
E   += P * dt / 3600000        // kWh
```

- `V_CAL` is volts RMS per ADC count.
- `I_CAL` is amps RMS per ADC count.
- `dt` is the window length in seconds (0.04 s here).
- Energy is kept in RAM and resets on reboot in this version.

### 8.4 Clipping and fault detection

| Condition | Meaning | Action |
|---|---|---|
| `vMax` or `iMax` stops rising with load | Op-amp clipping | Turn the trimpot down |
| `vMax - vMin < 8` | No signal | Check wiring and sensor power |
| Bias far from mid-scale | Sensor or wiring fault | Check VCC, GND and OUT |
| Irms > 5 A | CT overload | Remove load; use the strip switch |
| ADC pinned at 0 or 4095 | Fault or wrong attenuation | Check the analog input |

There is no relay, so firmware can only warn.

### 8.5 Serial output

```text
V=230.1 V  I=1.234 A  P=281.2 W  PF=0.99  raw V[1980..2320] I[2010..2290]
```

The raw min/max is the practical clipping indicator.

### 8.6 Dashboard (next milestone)

- `/` live dashboard.
- `/api/status` JSON.
- Server-Sent Events or one-second polling.
- Calibration and Wi-Fi settings page.
- Basic authentication; do not expose the device to the internet.

---

## 9. Calibration

### 9.1 Bias check

1. Power the sensors from 3.3 V with **no mains**.
2. Read raw ADC for a few seconds.
3. The mean should be near mid-scale, about **2000–2200** counts at 11 dB.
4. The sketch removes this offset automatically each window.

### 9.2 Voltage calibration

1. Apply a known AC voltage.
2. Measure the true value with a CAT III multimeter.
3. Note the uncalibrated `V` reading.
4. `V_CAL = V_true / V_raw`.
5. Recheck at a second voltage.

### 9.3 Current calibration

1. Use a resistive load, for example a 100 W bulb (≈0.43 A at 230 V) or a 1000 W heater (≈4.35 A).
2. Measure the true current with a clamp meter.
3. Note the uncalibrated `I` reading.
4. `I_CAL = I_true / I_raw`.
5. Recheck at a second load.

### 9.4 Power factor

- With a resistive load, PF should read close to 1.00.
- If PF is negative, the CT is facing the wrong way: flip the Live wire through the hole or swap the sensor leads.
- With a fan or motor, PF should be below 1.
- Sequential sampling adds a small phase error; this build does not correct it.

### 9.5 Verification

| Test | Expected |
|---|---|
| No load | I ≈ 0, P ≈ 0, V ≈ mains |
| 60 W bulb | I ≈ 0.26 A, P ≈ 60 W |
| 1000 W heater | I ≈ 4.35 A, P ≈ 1000 W |
| Fan / motor | PF < 1, P < S |

Record the final constants and the calibration date in the repository. Recalibrate after changing the supply voltage, the divider or a trimpot.

---

## 10. Test plan

### Stage 0 — Bench inspection, no mains

- [ ] Check for shorts between 5 V, 3.3 V and GND.
- [ ] Check the sensor wiring; no mains connected.
- [ ] Verify the ESP32 boots and the sketch prints.
- [ ] Verify the raw ADC is near mid-scale on both channels.

### Stage 1 — Power

- [ ] Connect the 5 V 2 A charger to the 5V pin.
- [ ] Measure 3.3 V at both sensor VCC pins.
- [ ] Measure about 1.65 V at each OUT pin.
- [ ] Check current draw: about 100–200 mA idle, under 500 mA peak.
- [ ] Confirm the board does not reset when Wi-Fi starts.

### Stage 2 — ADC

- [ ] Print raw min/max for both channels.
- [ ] Confirm the values are near mid-scale and stable.
- [ ] Fit 100 nF at each pin; confirm the noise drops.
- [ ] Feed a small 50 Hz signal and verify the RMS calculation.
- [ ] Confirm clipping is detected on the positive peak.

### Stage 3 — Sensors, low voltage / isolation transformer

- [ ] Apply a known AC voltage with a variac/isolation transformer.
- [ ] Adjust the ZMPT trimpot until the signal is clean.
- [ ] Pass a known current through the CT (for example a 100 W bulb).
- [ ] Adjust the current trimpot until the signal is clean.
- [ ] Check linearity at 50 V, 100 V, 150 V, 200 V and 230 V.
- [ ] Check linearity at 0.5 A, 1 A, 2 A, 3 A and 4 A.

### Stage 4 — Mains and load

- [ ] Use an RCD/GFCI and an isolation transformer for the first mains test.
- [ ] Start with a 40 W bulb.
- [ ] Check the voltage and current readings.
- [ ] Then use a 1000 W heater; compare current with a clamp meter.
- [ ] Check terminal and sensor temperature after 30 minutes.

### Stage 5 — Accuracy

- [ ] Compare V, I, P and PF with a commercial energy meter.
- [ ] Test at low PF (fan/motor).
- [ ] Test at no load and check the zero offset.
- [ ] Note accuracy with dimmer and SMPS loads.

### Stage 6 — Soak and safety

- [ ] 24 h run at 500 W; check stability and Wi-Fi reconnect.
- [ ] 1 h run at 4.0 A; check temperatures (< 60 °C at terminals).
- [ ] Earth continuity: < 0.1 Ω from the earth pin to the socket earth.
- [ ] Insulation resistance: > 1 MΩ (preferably > 10 MΩ) between mains and low voltage.
- [ ] Strain relief: pull test on the input cable.
- [ ] Final visual inspection: no exposed mains, no loose strands.

**Pass criteria:** V within ±5%, I and P within ±5% at PF > 0.8; no overheating; all safety checks pass.

---

## 11. Safety

Read [SAFETY.md](SAFETY.md) completely. The short version:

- Mains is lethal. Unplug before touching anything.
- Use a reputable 5 V 2 A charger; do not use a cheap non-isolated charger.
- Do not connect the ESP32 to a PC USB port while mains is connected.
- Do not connect the charger and USB at the same time unless the board has a back-feed diode.
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
- 3.3 V op-amp headroom limits signal range and accuracy.
- No switching, display or button.
- Sequential sampling adds a small phase error.
- Energy resets on reboot in the current sketch.
- Wi-Fi only; no cloud service.

### Next steps

1. Build and test the low-voltage side.
2. Calibrate with a known voltage and a resistive load.
3. Add the web dashboard and JSON API.
4. Add energy persistence (NVS) and OTA.
5. Build the enclosure and run the safety tests.
6. Optional: switch to 5 V plus dividers for more range, or add MQTT/Home Assistant.

---

## 13. References

- ZMPT101B AC voltage sensor module: <https://robu.in/product/ac-voltage-sensor-module-zmpt101b-single-phase/>
- ZMCT103C 5 A current sensor module: <https://robu.in/product/5a-range-single-phase-ac-current-sensor-module/>
- ESP32-C3 Super Mini HW-466AB: <https://quartzcomponents.com/products/esp32-c3-super-mini-development-board-with-soldered-headers-hw-466ab>
- Espressif — ESP32-C3 datasheet: <https://documentation.espressif.com/esp32-c3_datasheet_en.pdf>
- OpenEnergyMonitor — EmonLib concepts: <https://github.com/openenergymonitor/EmonLib>
- ZMCT103C datasheet: <https://innovatorsguru.com/wp-content/uploads/2019/08/zmct103C-datasheet.pdf>
