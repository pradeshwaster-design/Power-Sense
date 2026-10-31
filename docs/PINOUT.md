# Pinout & Wiring

This is the quick-reference pinout for the ESP32-C3 Super Mini version. The full design plan is in [PLAN.md](PLAN.md).

---

## 1. ESP32-C3 Super Mini header

**Orientation:** component side up, USB-C connector at the top, antenna at the bottom. This matches the common HW-466AB board.

```
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

> Always verify the silkscreen on your board. Some batches swap the 5V/3V3 positions or label the analog pins A0–A5.

### Pin functions and use in this project

| Header label | GPIO | ADC / special function | Safe for external use? | Project use |
|---|---|---|---|---|
| 5V | — | 5 V input/output | Yes | From HLK-5M05 +5 V; sensors VCC |
| GND | — | Ground | Yes | Common ground |
| 3V3 | — | 3.3 V output/input | Yes | Optional OLED only; do not power the 5 V sensor modules from here |
| GPIO4 | 4 | ADC1_CH4, JTAG | Yes | Push button to GND |
| GPIO3 | 3 | — | Yes | Relay driver |
| GPIO2 | 2 | ADC1_CH2, strapping | **No** | Leave floating |
| GPIO1 | 1 | ADC1_CH1 | Yes | Current sensor (via divider) |
| GPIO0 | 0 | ADC1_CH0 | Yes | Voltage sensor (via divider) |
| GPIO5 | 5 | ADC2_CH0 (not usable with Wi-Fi) | Digital yes, analog no | Optional OLED SDA |
| GPIO6 | 6 | JTAG | Digital yes | Optional OLED SCL |
| GPIO7 | 7 | JTAG | Digital yes | Optional buzzer |
| GPIO8 | 8 | Onboard LED, strapping | **No** | Onboard LED only |
| GPIO9 | 9 | BOOT button, strapping | **No** | BOOT button only |
| GPIO10 | 10 | — | Yes | Spare / relay 2 |
| GPIO20 | 20 | UART0 RX | Yes | Debug / programming header |
| GPIO21 | 21 | UART0 TX | Yes | Debug / programming header |

### Why GPIO0 and GPIO1 for analog

- ESP32-C3 ADC1 channels 0–4 are on GPIO0–GPIO4.
- GPIO5 is ADC2. **ADC2 is unavailable while Wi-Fi is active**, so it must not be used for the sensors.
- GPIO0 and GPIO1 are not strapping pins, so they are safe at boot.
- GPIO2 is an ADC1 pin but is a strapping pin; keep it free.

---

## 2. Sensor module pinouts

> **Verify the header order on your own module.** The tables below list pins by function, not necessarily by physical position. Different batches use different orders. A common ZMPT101B order is VCC–OUT–GND–GND; a common ZMCT103C order is GND–GND–OUT–VCC. Read the silkscreen before applying 5 V.

### ZMPT101B AC voltage sensor module

| Module pin | Connects to | Notes |
|---|---|---|
| VCC | 5 V | Module is designed for 5 V |
| OUT | Divider input → GPIO0 | **Never connect directly to the ESP32** |
| GND | GND | There are usually two GND pins; connect one |
| GND | GND | Duplicate ground |
| AC input L | Mains Live after fuse | 2-pin screw terminal |
| AC input N | Mains Neutral | 2-pin screw terminal |

- Output: AC waveform, 0–5 V, biased at about 2.5 V.
- The multi-turn potentiometer sets the gain. Adjust so the output stays below ~4.5 V peak at your highest expected mains voltage.
- Adjust the pot only with an insulated trimmer tool; the AC input terminals are live. Prefer an RCD/isolation transformer.
- The transformer provides galvanic isolation between mains and the output.

### ZMCT103C 5 A AC current sensor module

| Module pin | Connects to | Notes |
|---|---|---|
| GND | GND | There are usually two GND pins; connect one |
| GND | GND | Duplicate ground |
| OUT | Divider input → GPIO1 | **Never connect directly to the ESP32** |
| VCC | 5 V | Module is designed for 5 V |
| CT hole | Live wire passes through | Only **one** wire through the hole |

- Output: AC waveform, 0–5 V, biased at about 2.5 V.
- The potentiometer sets the gain. Adjust for a good signal at your normal load current.
- The CT provides isolation (typically 4500 V). The mains wire does not touch the PCB.
- Passing both Live and Neutral through the hole cancels the measurement. Pass only Live.
- Rated 5 A; do not exceed 5 A. Derate to 4 A continuous for long life.
- Never open the CT secondary circuit while current is flowing; the module's burden resistor must stay connected. An open secondary can generate a dangerous voltage spike.

---

## 3. Analog front-end (mandatory)

The sensor outputs can reach 5 V. The ESP32-C3 ADC pin is **3.3 V max** and its usable linear range with 11 dB attenuation is about **0–2.5 V**. Use this divider on each channel:

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

- Divider ratio: 2:1.
- 0–5 V sensor output → 0–2.5 V at the ADC.
- 2.5 V bias → 1.25 V at the ADC, which is mid-scale for the 0–2.5 V measurement range.
- The 1 kΩ + 10 nF forms a low-pass filter and limits fault current.
- The BAT54S clamps the pin to the 3.3 V rail and ground.
- Use the same circuit for both channels.
- Keep the divider close to the ESP32; keep the sensor wires short and away from the relay and mains wiring.

**Do not omit the divider.** A direct connection will clip the positive half-cycle and can permanently damage the ESP32.

---

## 4. Complete low-voltage connection table

| From | To | Notes |
|---|---|---|
| HLK-5M05 +V | ESP32 5V | 5 V rail |
| HLK-5M05 −V | ESP32 GND | Common ground |
| ZMPT101B VCC | 5 V | |
| ZMPT101B GND | GND | |
| ZMPT101B OUT | Divider CH-V input | 0–5 V signal |
| Divider CH-V output | GPIO0 | 0–2.5 V |
| ZMCT103C VCC | 5 V | |
| ZMCT103C GND | GND | |
| ZMCT103C OUT | Divider CH-I input | 0–5 V signal |
| Divider CH-I output | GPIO1 | 0–2.5 V |
| Relay module IN | GPIO3 | If using a bare relay, see driver below |
| Relay module VCC | 5 V | |
| Relay module GND | GND | |
| Push button | GPIO4 → GND | Internal pull-up; 100 nF debounce optional |
| OLED VCC | 3V3 | Optional |
| OLED GND | GND | Optional |
| OLED SDA | GPIO5 | Optional |
| OLED SCL | GPIO6 | Optional |
| Buzzer − | NPN collector; base ← GPIO7 via 1 kΩ; + to 5 V | Optional active buzzer |
| UART RX | GPIO20 | Debug header |
| UART TX | GPIO21 | Debug header |

---

## 5. Relay driver

### Bare relay (recommended for learning)

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

- Use a relay rated at least 10 A / 250 VAC. The project is limited to 5 A.
- Add a 100 Ω / 1 W + 100 nF X2 snubber across the relay contacts.
- Add a 10 kΩ pull-down on GPIO3 so the relay is off during boot.

### Opto-isolated relay module

- Connect module `IN` to GPIO3, `VCC` to 5 V, `GND` to GND.
- Many relay modules are **active-low**: `IN` low = relay on. Check yours and invert the firmware logic if needed; do not assume active-high.
- If the module has a JD-VCC jumper, follow the module instructions. For a simple build, a bare relay + transistor is easier to understand and repair.
- Do not drive a relay coil directly from a GPIO pin.

---

## 6. Mains wiring

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

- **Fuse** on Live, before everything else.
- **MOV** (275 VAC) and **X2 capacitor** across L–N after the fuse.
- **ZMPT101B** connected before the relay, so voltage is measured even when the relay is off.
- **CT** on the Live wire after the relay, so current is load-only.
- **Earth** is continuous and never switched.
- Prefer a **2-pole relay** to switch both Live and Neutral; keep Earth continuous. If using a 1-pole relay, switch Live only.
- Add a **0.5 A fuse** in the HLK branch and a **0.1 A fuse** in the ZMPT branch for extra protection.
- Use 1.5 mm² wire for mains, 0.75 mm² for the HLK/sensor branches.
- Use a plastic enclosure with a physical barrier between the mains and low-voltage areas.

> The ZMPT101B and ZMCT103C provide isolation, but their PCB creepage is not a substitute for proper enclosure spacing. Keep at least 6 mm between mains and low-voltage conductors, use slots in the PCB, and cover all mains joints with heat-shrink.

---

## 7. ESP-12E / ESP8285 migration pinout

### The problem

The ESP8266 family (ESP-12E, ESP-12F, ESP8285) has **one 10-bit ADC** on A0:

- Input range: 0–1.0 V (not 3.3 V).
- Resolution: 10-bit.
- Only one channel.
- It cannot sample voltage and current simultaneously.

You cannot connect both sensors to A0 and get good results. Choose one of these options:

### Option A — external MCP3202 ADC (recommended if you must use ESP-12E)

| Function | ESP-12E pin | Notes |
|---|---|---|
| Relay | GPIO5 | Active-high via NPN |
| Push button | GPIO16 | To GND, pull-up |
| MCP3202 CS | GPIO4 | 10 kΩ pull-up |
| MCP3202 SCK | GPIO14 | |
| MCP3202 MOSI | GPIO13 | |
| MCP3202 MISO | GPIO12 | |
| UART debug | GPIO1 / GPIO3 | TX / RX |
| A0 | not used | 0–1 V only |
| Avoid | GPIO0, GPIO2, GPIO15 | Strapping pins |
| Avoid | GPIO6–GPIO11 | Internal flash |

MCP3202 connections:

| MCP3202 pin | Connects to |
|---|---|
| VDD (also the ADC reference) | 3.3 V |
| VSS / DGND | GND |
| CH0 | Voltage divider output (0–2.5 V) |
| CH1 | Current divider output (0–2.5 V) |
| CS (active low) | GPIO4 |
| CLK / SCK | GPIO14 |
| DIN / MOSI | GPIO13 |
| DOUT / MISO | GPIO12 |

- The MCP3202 is a 12-bit, 2-channel, SPI ADC. It is much better than the ESP8266 internal ADC.
- Power the ESP-12E from 3.3 V. Add an AMS1117-3.3 regulator from the 5 V rail, or use a 3.3 V HLK module. The relay and sensors remain on 5 V.
- The ESP-12E is 2 mm pitch and needs an adapter for breadboarding.
- 1 MB ESP8285 flash is tight for OTA + LittleFS dashboard. Prefer a 4 MB ESP-12E/ESP-12F.

### Option B — PZEM-004T v3 module (simplest)

- PZEM-004T measures V, I, P, energy, frequency and PF, and sends the data over isolated UART.
- Connect its 5 V/GND to the 5 V rail and its TX/RX to the ESP8266 UART (GPIO3/GPIO1) through the module's optocouplers.
- **The PZEM UART side is 5 V logic.** Add a divider or level shifter on the PZEM TX line before the ESP8266 RX pin, which is not 5 V tolerant.
- No analog front-end, no ADC calibration.
- More expensive than the two sensors, but far less work and more accurate.
- Follow the PZEM datasheet for the mains side; it is a mains-powered module.

### Option C — analog multiplexer into A0 (not recommended)

- A CD4051/74HC4051 can switch V and I into A0, but the channels are sampled at different times, the 0–1 V range gives poor resolution, and the sample-and-hold settling is difficult.
- Only consider this for a toy; it will not give good power/PF readings.

### Recommendation

**Do not downgrade to ESP-12E just for cost.** The total BOM saving is small, and you lose the 12-bit ADC, the 5 V-tolerant USB-C board, and the easy programming. For cost-down, use an **ESP32-C3-MINI-1 module** on a custom PCB. It keeps the ADC and Wi-Fi and removes the dev-board overhead. If you must use an ESP8266, use the MCP3202 or PZEM-004T; do not use A0 for both signals.

---

## 8. Quick checklist before power-up

- [ ] 5 V and 3.3 V rails measured, no shorts.
- [ ] Sensor outputs measured with a multimeter: about 2.5 V DC with no signal, and a small AC swing.
- [ ] Divider outputs never exceed 2.5 V.
- [ ] Relay is off at boot (GPIO3 pulled down).
- [ ] Flyback diode installed correctly.
- [ ] Fuse, MOV and earth connected.
- [ ] No exposed mains conductors.
- [ ] ESP32 USB disconnected before mains is applied.
- [ ] Current sensor around Live only.
- [ ] Load is within 5 A.
