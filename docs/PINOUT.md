# Power Sense — Pinout & Wiring

Complete wiring reference for the simple **Power Sense** monitor: ESP32-C3 Super Mini, ZMPT101B voltage sensor, ZMCT103C 5 A current sensor and a 5 V 2 A USB charger.

**No relay, no display, no push button, no divider.**

---

## Circuit diagram

### Low-voltage side

```text
      5 V 2 A CHARGER
      +5V ───────────────────► ESP32-C3 5V
      GND ───────────────────► ESP32-C3 GND

      ESP32-C3 3V3 ──┬───────► ZMPT101B VCC
                     └───────► ZMCT103C VCC

      ESP32-C3 GND ──┬───────► ZMPT101B GND
                     ├───────► ZMCT103C GND
                     ├───────► 100 nF (GPIO3 filter)
                     ├───────► 100 nF (GPIO4 filter)
                     └───────► 100 µF + 100 nF (3V3 decoupling)
```

### ZMPT101B — voltage

```text
              +------------------+
  3V3 --------| VCC          OUT |--------*--------► GPIO3
              |                  |        |
  GND --------| GND              |      [100 nF]
              |   ZMPT101B       |        |
  L   --------| AC-L             |       GND
  N   --------| AC-N             |
              +------------------+
```

### ZMCT103C — current

```text
              +------------------+
  3V3 --------| VCC          OUT |--------*--------► GPIO4
              |                  |        |
  GND --------| GND              |      [100 nF]
  GND --------| GND              |        |
              |   ZMCT103C       |       GND
              |  CT terminals    |  ◄── CT secondary from the toroid
              +------------------+

  Live wire ──────( CT hole )──────► load Live
```

The 100 nF capacitors go from each OUT pin to GND, physically close to the ESP32.

### AC side

```text
230 V L ──[5 A fuse]──┬──( CT hole )──► load Live
                      └── ZMPT101B AC-L

230 V N ──────────────┬────────────────► load Neutral
                      └── ZMPT101B AC-N

230 V E ──────────────────────────────► load Earth
```

- The ZMPT101B is across Live and Neutral, before the strip switch.
- The CT is around the Live wire only, after the fuse, with the load current flowing through it.
- Nothing on the AC side connects to the ESP32 side.

---

## 1. Power

```text
5 V 2 A USB charger
        │
   5V pin (or USB-C)
        │
ESP32-C3 Super Mini
        │
   3V3 ─┼── ZMPT101B VCC
        └── ZMCT103C VCC

   GND ─┬── ZMPT101B GND
        ├── ZMCT103C GND
        └── 100 µF / 100 nF decoupling
```

| Rail | Source | Loads |
|---|---|---|
| 5 V | USB charger | ESP32-C3 5V pin |
| 3.3 V | C3 onboard regulator | ZMPT101B and ZMCT103C, a few mA each |

- The ESP32-C3 Wi-Fi peak is about 350 mA; the sensors add only a few mA. The onboard regulator easily covers this.
- Add a **100 µF** electrolytic and a **100 nF** ceramic capacitor across 3.3 V/GND near the sensors.
- Do **not** connect the charger to 5V and a PC USB cable at the same time unless the board has a back-feed diode. Unplug the external supply and mains before programming.

---

## 2. ESP32-C3 Super Mini

| Header pin | GPIO | Connect to | Notes |
|---|---|---|---|
| **5V** | — | 5 V charger | Main supply |
| **GND** | — | Common ground | Sensors and decoupling |
| **3V3** | — | Both sensor VCC | Regulator output |
| **GPIO3** | 3 | ZMPT101B OUT | ADC1_CH3 |
| **GPIO4** | 4 | ZMCT103C OUT | ADC1_CH4 |
| GPIO0 | 0 | Not used | ADC1_CH0, free |
| GPIO1 | 1 | Not used | ADC1_CH1, free |
| GPIO2 | 2 | Not used | ADC1_CH2; strapping pin |
| GPIO5 | 5 | Not used | ADC2; unreliable with Wi-Fi |
| GPIO6 | 6 | Not used | — |
| GPIO7 | 7 | Not used | — |
| GPIO8 | 8 | Not used | Strapping; onboard LED |
| GPIO9 | 9 | Not used | BOOT strapping |
| GPIO10 | 10 | Not used | — |
| GPIO20 | 20 | Not used | UART0 RX |
| GPIO21 | 21 | Not used | UART0 TX |

Only **5V, GND, 3V3, GPIO3 and GPIO4** are required.

> Verify the silkscreen on your board. Some batches swap the 5V/3V3 positions, and some use GPIO8 or GPIO2 for the onboard LED.

### Why these pins

- **GPIO3 and GPIO4** are ADC1 channels and work with Wi-Fi enabled.
- ADC2 (GPIO5) shares hardware with the radio and gives poor readings while Wi-Fi is active.
- GPIO8 and GPIO9 are strapping pins; a sensor pulling them at boot can stop the board from starting.
- GPIO20/GPIO21 are the serial UART.
- GPIO18/GPIO19 are USB D-/D+; leave them alone.

---

## 3. ZMPT101B voltage sensor

| Module pin | Connect to | Notes |
|---|---|---|
| VCC | 3V3 | A few mA |
| GND | GND | One of the two GND pins |
| OUT | GPIO3 | Biased at about 1.65 V |
| AC-L | Mains Live | 2-pin screw terminal |
| AC-N | Mains Neutral | 2-pin screw terminal |

- The output is an AC waveform centred on **VCC/2 ≈ 1.65 V**.
- The multi-turn trimpot sets the gain. At 3.3 V the op-amp can only swing about **±0.15 V** around the midpoint before clipping, so turn the gain **down**.
- The transformer provides galvanic isolation between mains and the output.
- The AC terminals are live. Adjust the pot only with an insulated tool and with the circuit enclosed; use an RCD and preferably an isolation transformer.

> **Header order varies.** A common ZMPT101B order is VCC–OUT–GND–GND. Read the silkscreen before applying power.

---

## 4. ZMCT103C current sensor

| Module pin | Connect to | Notes |
|---|---|---|
| GND | GND | There are usually two GND pins |
| GND | GND | Duplicate ground |
| OUT | GPIO4 | Biased at about 1.65 V |
| VCC | 3V3 | A few mA |
| CT hole | Live wire passes through | One wire only |

- The output is an AC waveform centred on **VCC/2 ≈ 1.65 V**, proportional to the current in the wire.
- The trimpot sets the gain. At 3.3 V, the module may clip well below 5 A; turn it down and check the raw ADC maximum.
- The CT provides isolation. The mains wire does not touch the PCB.
- Passing both Live and Neutral through the hole cancels the measurement. Pass **Live only**.
- Rated 5 A; stay below 4 A continuous.
- Never open the CT secondary while current flows. Keep the module and its burden resistor intact.
- **Do not** wire the CT secondary in series with the load. Only the primary (the mains wire through the hole) carries load current.

> **Header order varies.** A common ZMCT103C order is GND–GND–OUT–VCC. Read the silkscreen before applying power.

---

## 5. Analog input

No divider is required at 3.3 V.

```text
Sensor OUT ──●── GPIO3 / GPIO4
             │
          [100 nF]
             │
            GND
```

- Sensor output range: about **0–1.8 V**, centred on **1.65 V**.
- ESP32-C3 ADC: 12-bit, 3.3 V maximum, 11 dB attenuation.
- The 100 nF capacitor filters high-frequency noise and must sit close to the ESP32 pin.
- Do **not** connect the sensor output to 5 V or to a 5 V-powered module without a divider.

### Clipping check

The LM358 output cannot reach 3.3 V. It saturates near **1.8 V**, which is only about 0.15 V above the 1.65 V bias. The positive half-cycle therefore clips first.

- Print raw ADC min/max for each channel.
- Increase the load while watching the maximum.
- If the maximum stops rising, reduce the module gain.
- Expect the bias near mid-scale (about 2000–2200 counts at 11 dB).

---

## 6. Complete connection table

| From | To | Notes |
|---|---|---|
| 5 V 2 A charger + | ESP32 5V | Main supply |
| 5 V 2 A charger − | ESP32 GND | Main supply |
| ESP32 3V3 | ZMPT101B VCC | 3.3 V rail |
| ESP32 3V3 | ZMCT103C VCC | 3.3 V rail |
| ESP32 GND | ZMPT101B GND | Common ground |
| ESP32 GND | ZMCT103C GND | Common ground |
| ZMPT101B OUT | GPIO3 | 0–1.8 V signal |
| ZMCT103C OUT | GPIO4 | 0–1.8 V signal |
| ZMPT101B AC-L | Mains Live | After the fuse |
| ZMPT101B AC-N | Mains Neutral | — |
| CT hole | Mains Live only | In series with the load |

Add **100 nF** from each OUT pin to GND, and **100 µF + 100 nF** across 3.3 V/GND.

---

## 7. Mains wiring

```text
230 V L ──[5 A fuse]──┬── ZMPT101B AC-L
                      ├── CT (Live wire only)
                      └── strip / load Live

230 V N ──────────────┴── ZMPT101B AC-N
                          strip / load Neutral

230 V E ───────────────── strip / load Earth
```

- **Fuse the Live wire** before the ZMPT and the sockets.
- **ZMPT101B** connects across Live and Neutral, in parallel with the load.
- **CT** goes around the Live wire feeding the load.
- **Earth** is continuous and never switched.
- Use 1.5 mm² wire for mains and 0.75 mm² for the 3.3 V/sensor wiring.
- Keep at least **6 mm** between mains and low-voltage conductors.
- Use a plastic enclosure with a barrier between the mains and low-voltage sides.
- The strip is **always live** when plugged in; there is no relay.

---

## 8. Optional 5 V fallback

Only if the 3.3 V signal is too small or the current sensor clips too early.

```text
Sensor OUT ──[10k]──●──[20k]── GND
                    │
GPIO3/GPIO4 ────────●──[100 nF]── GND
                    │
              BAT54S (optional)
            pin 3 = signal
            pin 1 = GND
            pin 2 = 3V3
```

- Power both modules from **5V** instead of 3V3.
- The divider turns the 0–3.5 V sensor swing into about 0–2.3 V at the pin.
- The LM358 does not reach 5 V at its output, so 10k/20k is safe here.
- Recalibrate after the change.

---

## 9. Before power-up checklist

- [ ] 5 V and GND polarity checked; no shorts.
- [ ] Sensors connected to **3.3 V**, not 5 V.
- [ ] Sensor outputs go to GPIO3 and GPIO4, not to 5 V or 3.3 V.
- [ ] 100 nF fitted from each OUT pin to GND.
- [ ] Sensors read about 1.65 V DC on OUT with no signal.
- [ ] Raw ADC is near mid-scale and not pinned at 0 or 4095.
- [ ] CT is around the Live wire only, one wire through the hole.
- [ ] Fuse and earth connected.
- [ ] No exposed mains conductors.
- [ ] External 5 V and USB are not connected at the same time.
- [ ] Load is within 5 A (4 A continuous).
