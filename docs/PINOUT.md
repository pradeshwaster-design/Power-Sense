# Power Sense — Pinout & Wiring

Complete wiring reference for the final build: **ESP32-C3 Super Mini**, **ZMPT101B** voltage sensor and a **passive 5 A CT** module.

**Monitor only — no relay, no display, no push button.**

> Mains is lethal. Read [SAFETY.md](SAFETY.md) before connecting anything.

---

## 1. Power

```text
5 V 2 A USB charger ──USB-C──► ESP32-C3 Super Mini
ESP32-C3 3V3 ──┬──► ZMPT101B VCC
               └──► 10 kΩ bias divider (top)
ESP32-C3 GND ──┬──► ZMPT101B GND
               ├──► bias divider (bottom)
               ├──► 22 µF (–)
               └──► 100 nF filters (optional)
```

| Rail | Source | Loads |
|---|---|---|
| 5 V | USB-C charger | ESP32-C3 |
| 3.3 V | C3 regulator | ZMPT101B, CT bias divider |

The CT module has **no VCC** — it is a passive transformer. Do not connect it to 3.3 V.

---

## 2. ESP32-C3 Super Mini

| Pin | Connect to | Notes |
|---|---|---|
| **GPIO3** | ZMPT101B OUT | ADC1_CH3 |
| **GPIO4** | CT S through 1 kΩ | ADC1_CH4 |
| **3V3** | ZMPT101B VCC, 10 kΩ divider top | Regulator output |
| **GND** | ZMPT101B GND, divider bottom, 22 µF, filters | Common ground |
| **5V / USB-C** | 5 V charger | Main supply |
| GPIO0/1/2 | Not used | ADC1, free |
| GPIO5 | Not used | ADC2 — unreliable with WiFi |
| GPIO8/9 | Not used | Strapping; GPIO8 also drives the LED |
| GPIO20/21 | Not used | UART |
| GPIO18/19 | Not used | USB D-/D+ |

Only **GPIO3, GPIO4, 3V3, GND and USB-C** are required.

---

## 3. ZMPT101B voltage sensor

| Module pin | Connect to | Notes |
|---|---|---|
| VCC | 3V3 | 3-pin module |
| GND | GND | — |
| OUT | GPIO3 | Biased at about 1.65 V |
| AC-L | Mains Live | After the fuse |
| AC-N | Mains Neutral | — |

- The output is an AC waveform centred on **VCC/2 ≈ 1.65 V**.
- The trimpot sets the gain. Adjust it **once**, before calibration, so the raw voltage count is roughly **400–900** at normal mains and the waveform does not clip.
- Some modules use an LM358 with limited output swing at 3.3 V. If the reading saturates or the top of the waveform flattens, reduce the gain.
- The transformer provides isolation between mains and the output.

> Header order varies between batches. A common order is VCC–OUT–GND–GND. Read the silkscreen before applying power.

---

## 4. Passive CT current sensor

The module has only two pins: **G** and **S**. It is a current transformer with a burden resistor, not an amplified module.

| Module pin | Connect to | Notes |
|---|---|---|
| G | **1.65 V bias node** | **Not ground** |
| S | 1 kΩ → GPIO4 | Signal |
| CT hole | Live wire only | In series with the load |

### Bias network

```text
                        3V3
                         │
                       [10k]
                         │
  CT module G ───────────●────────┬────────► 1.65 V bias
                         │        │
                       [10k]    [22 µF]  (+ to bias)
                         │        │
                        GND      GND
```

- The two 10 kΩ resistors make **1.65 V** from the 3.3 V rail.
- The 22 µF capacitor holds the bias steady.
- The CT signal rides on this bias, so the ADC can read both half-cycles.
- **Do not ground the `G` pin.** The negative half-cycle would be clipped and the readings would be wrong.
- The CT output is a small AC current; the module's burden resistor converts it to voltage.

### Series resistor and filters

```text
CT S ──[1k]──┬──► GPIO4
             │
          [100 nF]  (optional)
             │
            GND
```

- The 1 kΩ resistor limits fault current and isolates the ADC input capacitance.
- The optional 100 nF from GPIO4 to GND filters high-frequency noise.
- Fit the same optional 100 nF from GPIO3 to GND.

### Burden resistor

Some CT modules already include a burden resistor; some do not. With the mains disconnected, measure the resistance across **S** and **G**:

- A few hundred ohms or less: a burden is fitted. **Do not add another.**
- Very high resistance or open: fit **150 Ω** across S and G.

A CT secondary must never be open while current flows. Keep the burden connected.

---

## 5. Full signal wiring

```text
                        3V3
                         │
                       [10k]
                         │
  CT G ──────────────────●────────┬────────► bias ≈ 1.65 V
                         │        │
                       [10k]    [22 µF]
                         │        │
                        GND      GND

  CT S ──[1k]──┬──────────────────────────► GPIO4
               │
            [100 nF] (optional)
               │
              GND

  ZMPT VCC ──────────────────────────────► 3V3
       GND ──────────────────────────────► GND
       OUT ──┬───────────────────────────► GPIO3
             │
          [100 nF] (optional)
             │
            GND
```

---

## 6. AC wiring

```text
230 V L ──[5 A fuse]──┬──( CT hole )──► load Live
                      └── ZMPT101B AC-L

230 V N ──────────────┬────────────────► load Neutral
                      └── ZMPT101B AC-N

230 V E ──────────────────────────────► load Earth
```

- **Fuse the Live wire** before the ZMPT and the sockets.
- **ZMPT101B** connects across Live and Neutral, in parallel with the load.
- **CT** goes around the Live wire only, with the load current flowing through it.
- **Earth** is continuous and never switched.
- Use 1.5 mm² wire for mains and 0.75 mm² for the low-voltage wiring.
- Keep at least **6 mm** between mains and low-voltage conductors.
- Use a plastic enclosure with a barrier between the mains and low-voltage sides.
- The strip is **always live** when plugged in; there is no relay.

---

## 7. Connection summary

| From | To | Notes |
|---|---|---|
| USB-C charger | ESP32-C3 USB-C | 5 V 2 A |
| ESP32 3V3 | ZMPT101B VCC | — |
| ESP32 3V3 | 10 kΩ divider top | Bias supply |
| ESP32 GND | ZMPT101B GND | Common |
| ESP32 GND | 10 kΩ divider bottom | Common |
| ESP32 GND | 22 µF (–) | Common |
| Divider midpoint | CT G | **1.65 V bias** |
| CT S | 1 kΩ → GPIO4 | Signal |
| ZMPT OUT | GPIO3 | Signal |
| ZMPT AC-L | Mains Live, after fuse | Mains |
| ZMPT AC-N | Mains Neutral | Mains |
| CT hole | Mains Live only | Mains |

---

## 8. Before power-up checklist

- [ ] 5 V and 3.3 V rails checked; no shorts.
- [ ] ZMPT101B VCC on **3V3**, not 5 V.
- [ ] CT `G` on the **1.65 V bias**, not GND.
- [ ] CT `S` goes through **1 kΩ** to GPIO4.
- [ ] Burden resistor present (module or added 150 Ω).
- [ ] 22 µF polarity correct (+ to bias).
- [ ] Optional 100 nF filters fitted from GPIO3/GPIO4 to GND.
- [ ] Bias node measures about **1.65 V** with no mains.
- [ ] ZMPT OUT measures about **1.65 V** with no mains.
- [ ] CT around **Live only**, one wire through the hole.
- [ ] Fuse and earth connected.
- [ ] No exposed mains conductors.
- [ ] Load within **5 A** (4 A continuous).
- [ ] Mains unplugged before touching the board or USB.
