# Power Sense — Pinout & Wiring

This is the complete wiring reference for the simple **Power Sense** monitor: ESP32-C3 Super Mini, ZMPT101B voltage sensor, ZMCT103C current sensor and a 5 V 2 A USB charger.

**No relay, no display, no push button.**

---

## 1. ESP32-C3 Super Mini

**Orientation:** component side up, USB-C connector at the top, antenna at the bottom. This matches the common HW-466AB board.

```text
                  USB-C
             ┌─────────────┐
        5V ──┤ 1        1  ├── GPIO5
       GND ──┤ 2        2  ├── GPIO6
       3V3 ──┤ 3        3  ├── GPIO7
     GPIO4 ──┤ 4        4  ├── GPIO8
     GPIO3 ──┤ 5        5  ├── GPIO9
     GPIO2 ──┤ 6        6  ├── GPIO10
     GPIO1 ──┤ 7        7  ├── GPIO20
     GPIO0 ──┤ 8        8  ├── GPIO21
             └─────────────┘
```

> Verify the silkscreen on your board. Some batches swap the 5V/3V3 positions.

### Pins used by this project

| Header pin | GPIO | Connect to | Notes |
|---|---|---|---|
| **5V** | — | 5 V from USB charger; sensor VCC | Powers both sensors |
| **GND** | — | Common ground | Sensors, dividers and ESP32 |
| **GPIO0** | 0 | Voltage divider output | ADC1_CH0 |
| **GPIO1** | 1 | Current divider output | ADC1_CH1 |
| 3V3 | — | Not used | Do not power 5 V sensors here |
| GPIO2 | 2 | Not used | Strapping pin |
| GPIO3 | 3 | Not used | — |
| GPIO4 | 4 | Not used | — |
| GPIO5 | 5 | Not used | ADC2, not needed |
| GPIO6 | 6 | Not used | — |
| GPIO7 | 7 | Not used | — |
| GPIO8 | 8 | Not used | Onboard LED / strapping |
| GPIO9 | 9 | Not used | BOOT / strapping |
| GPIO10 | 10 | Not used | — |
| GPIO20 | 20 | Not used | UART0 RX |
| GPIO21 | 21 | Not used | UART0 TX |

Only **5V, GND, GPIO0 and GPIO1** are required.

---

## 2. ZMPT101B voltage sensor

| Module pin | Connect to | Notes |
|---|---|---|
| VCC | 5 V | Module is designed for 5 V |
| GND | GND | There are usually two GND pins; connect one |
| OUT | 2:1 divider input → GPIO0 | **Never connect directly to the ESP32** |
| AC-L | Mains Live, after the fuse | 2-pin screw terminal |
| AC-N | Mains Neutral | 2-pin screw terminal |

- Output: AC waveform, 0–5 V, biased at about 2.5 V.
- The multi-turn potentiometer sets the gain. Adjust so the output stays below about 4.5 V peak at your highest expected mains voltage.
- Adjust the pot only with an insulated trimmer tool; the AC input terminals are live. Prefer an RCD/isolation transformer.
- The transformer provides galvanic isolation between mains and the output.

> **Header order varies.** A common ZMPT101B order is VCC–OUT–GND–GND. Read the module silkscreen before applying 5 V.

---

## 3. ZMCT103C current sensor

| Module pin | Connect to | Notes |
|---|---|---|
| GND | GND | There are usually two GND pins; connect one |
| GND | GND | Duplicate ground |
| OUT | 2:1 divider input → GPIO1 | **Never connect directly to the ESP32** |
| VCC | 5 V | Module is designed for 5 V |
| CT hole | Live wire passes through | Only **one** wire through the hole |

- Output: AC waveform, 0–5 V, biased at about 2.5 V.
- The potentiometer sets the gain. Adjust for a good signal at your normal load current.
- The CT provides isolation. The mains wire does not touch the PCB.
- Passing both Live and Neutral through the hole cancels the measurement. Pass only Live.
- Rated 5 A; do not exceed 5 A. Derate to 4 A continuous for long life.
- Never open the CT secondary circuit while current is flowing; the module's burden resistor must stay connected. An open secondary can generate a dangerous voltage spike.

> **Header order varies.** A common ZMCT103C order is GND–GND–OUT–VCC. Read the module silkscreen before applying 5 V.

---

## 4. Analog front-end

Both sensor outputs can reach 5 V. The ESP32-C3 ADC pin is **3.3 V maximum**, and its usable linear range is about **0–2.5 V**. Use this divider on each channel:

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

- Divider ratio: **2:1**.
- 0–5 V sensor output → 0–2.5 V at the ADC.
- 2.5 V bias → 1.25 V at the ADC.
- The 1 kΩ + 10 nF forms a low-pass filter and limits fault current.
- The BAT54S clamps the pin to the 3.3 V rail and ground.
- Use the same circuit for both channels.
- Keep the divider close to the ESP32; keep the sensor wires short and away from mains wiring.

**Do not omit the divider.** A direct connection will clip the positive half-cycle and can permanently damage the ESP32.

---

## 5. Complete connection table

| From | To | Notes |
|---|---|---|
| 5 V 2 A charger | ESP32-C3 USB-C | Main power |
| ESP32 5V | ZMPT101B VCC | 5 V rail |
| ESP32 5V | ZMCT103C VCC | 5 V rail |
| ESP32 GND | ZMPT101B GND | Common ground |
| ESP32 GND | ZMCT103C GND | Common ground |
| ESP32 GND | Divider GND | Both channels |
| ZMPT101B OUT | Divider CH-V input | 0–5 V signal |
| Divider CH-V output | GPIO0 | 0–2.5 V |
| ZMCT103C OUT | Divider CH-I input | 0–5 V signal |
| Divider CH-I output | GPIO1 | 0–2.5 V |
| ZMPT101B AC-L | Mains Live, after fuse | Mains side |
| ZMPT101B AC-N | Mains Neutral | Mains side |
| CT hole | Mains Live only | After fuse |

Add a **100 µF** electrolytic and a **100 nF** ceramic capacitor between 5 V and GND near the sensors.

---

## 6. Power wiring

```text
5 V 2 A USB charger
        │
      USB-C
        │
   ESP32-C3 Super Mini
        │
   5V ───┼── ZMPT101B VCC
        └── ZMCT103C VCC

   GND ───┬── ZMPT101B GND
          ├── ZMCT103C GND
          └── divider GND
```

Rules:

- Use a reputable, double-insulated **5 V 2 A** charger.
- Plug the charger into a **separate outlet** from the monitored strip for the cleanest readings.
- Do **not** connect the ESP32 to a PC USB port while the sensors are connected to mains.
- Do not power the 5 V sensors from the 3V3 pin.
- Keep the 5 V wires short and twisted where practical.

---

## 7. Mains wiring

```text
230 V L ──[5 A fuse]──┬── ZMPT101B AC-L
                      ├── CT (Live wire only)
                      └── strip Live

230 V N ──────────────┴── ZMPT101B AC-N
                          strip Neutral

230 V E ───────────────── strip Earth
```

- **Fuse** the Live wire before the ZMPT and the sockets.
- **ZMPT101B** connects across Live and Neutral.
- **CT** goes around the Live wire feeding the sockets, after the fuse.
- **Earth** is continuous and never switched.
- Use 1.5 mm² wire for mains; 0.75 mm² for the 5 V/sensor wiring.
- Use a plastic enclosure with a barrier between mains and low voltage.
- Keep at least **6 mm** between mains and low-voltage conductors.
- Cover all mains joints with heat-shrink; no exposed strands.
- The strip is **always live** when plugged in; there is no relay.

> The ZMPT101B and ZMCT103C provide isolation, but their PCB creepage is not a substitute for proper enclosure spacing. Keep the mains side covered and use an RCD for testing.

---

## 8. Before power-up checklist

- [ ] 5 V and GND connections checked; no shorts.
- [ ] Sensor VCC is 5 V, not 3.3 V.
- [ ] Sensor outputs measure about 2.5 V DC with no mains.
- [ ] Divider outputs measure about 1.25 V DC with no mains.
- [ ] Divider outputs never exceed 2.5 V.
- [ ] BAT54S orientation checked.
- [ ] CT is around Live only.
- [ ] Fuse and earth connected.
- [ ] No exposed mains conductors.
- [ ] ESP32 USB disconnected from the PC before mains is applied.
- [ ] Load is within 5 A.
