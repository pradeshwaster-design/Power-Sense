# Power Sense

**A self-contained WiFi energy monitor for 230 V AC — built on an ESP32-C3.**

![MCU](https://img.shields.io/badge/MCU-ESP32--C3-E7352C?logo=espressif&logoColor=white)
![Framework](https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino&logoColor=white)
![Dashboard](https://img.shields.io/badge/Dashboard-WiFi%20hotspot-2ea44f)
![License](https://img.shields.io/badge/License-MIT-blue)

Power Sense measures **RMS voltage, RMS current and real power** with a ZMPT101B voltage sensor and a passive 5 A current transformer. The ESP32-C3 creates its own WiFi hotspot and serves a live, mobile-friendly dashboard. No router, cloud account, app or display is needed.

> **Mains voltage is dangerous.** Read [Safety](#safety) and [docs/SAFETY.md](docs/SAFETY.md) before connecting anything. If you are not confident working with 230 V AC, use a certified energy-monitoring plug instead.

> **Scope:** monitor only — **no relay, no display, no push button**. The power strip is live whenever it is plugged in.

---

## Features

| | |
|---|---|
| **Measurements** | RMS voltage, RMS current, real power |
| **Dashboard** | Self-hosted page at `http://192.168.4.1`, updates every second |
| **WiFi** | The board is its own access point: `PowerSense` / `12345678` |
| **Calibration** | Hidden `/cal` page; enter real values and the board calculates the factors |
| **Storage** | Calibration survives reboots and re-uploads (`Preferences` / NVS) |
| **Signal handling** | DC-offset removal, idle-noise subtraction, smoothing and a 5 s warm-up |
| **Serial output** | Live readings at 115200 baud |
| **Dependencies** | Only built-in `WiFi`, `WebServer` and `Preferences` — nothing to install |
| **Board** | ESP32-C3 Super Mini, USB-C |

---

## Hardware

| Part | Qty | Notes |
|---|---|---|
| ESP32-C3 Super Mini | 1 | USB-C, 4 MB flash |
| ZMPT101B AC voltage module | 1 | 3-pin module, powered from 3.3 V |
| 5 A AC current sensor module | 1 | 2-pin **passive** CT module (`G`, `S`), no VCC |
| 10 kΩ 1% resistor | 2 | 1.65 V bias divider |
| 1 kΩ 1% resistor | 1 | Series protection from CT `S` to GPIO4 |
| 22 µF 16 V electrolytic | 1 | Bias-node decoupling |
| 100 nF ceramic capacitor | 2 | Optional ADC filters on GPIO3 and GPIO4 |
| 150 Ω resistor | 1 | Only if the CT module has no burden resistor |
| 5 A fuse and holder | 1 | Mains protection |
| Enclosure, mains wire, terminals | — | Insulated, mains-rated |

Full list with prices: **[docs/BOM.csv](docs/BOM.csv)**.

---

## Wiring

### Pinout

| Signal | ESP32-C3 pin | Notes |
|---|---|---|
| ZMPT101B `OUT` | **GPIO3** | ADC1_CH3, biased at about 1.65 V |
| CT `S` | **GPIO4** | Through a **1 kΩ** series resistor, ADC1_CH4 |
| CT `G` | **Bias node** | About **1.65 V** — **not ground** |
| Sensor power | **3V3** | ZMPT101B VCC and the 10 kΩ divider top |
| Ground | **GND** | ZMPT101B GND, divider bottom, 22 µF, filters |
| Board power | **USB-C** | 5 V 2 A charger |

Everything else is unused. Avoid GPIO5 (ADC2, unreliable with WiFi), GPIO8/GPIO9 (strapping; GPIO8 also drives the LED) and GPIO20/GPIO21 (UART).

> The CT module is a **passive transformer** with no VCC. Its `G` pin must sit at the 1.65 V bias point. Connecting `G` to ground will clip the negative half-cycle and give wrong readings.

### Signal circuit

```text
                        3V3
                         │
                       [10k]
                         │
  CT module G ───────────●────────┬────────► bias node ≈ 1.65 V
                         │        │
                       [10k]    [22 µF]  (+ to bias)
                         │        │
                        GND      GND

  CT module S ──[1k]──────────────────────► GPIO4

  ZMPT101B VCC ───────────────────────────► 3V3
           GND ───────────────────────────► GND
           OUT ───────────────────────────► GPIO3
```

Optional: fit **100 nF** from GPIO3 to GND and from GPIO4 to GND, close to the ESP32. This reduces ADC noise, especially while WiFi is transmitting.

### AC side

```text
230 V L ──[5 A fuse]──┬──( CT hole )──► load Live
                      └── ZMPT101B AC-L

230 V N ──────────────┬────────────────► load Neutral
                      └── ZMPT101B AC-N

230 V E ──────────────────────────────► load Earth
```

- **ZMPT101B** input terminals go **across Live and Neutral**, in parallel with the load.
- **CT**: pass **only the Live wire** through the hole, in series with the load.
- Nothing on the AC side connects to the ESP32 side. The sensors provide isolation.
- Add a **150 Ω burden resistor** between `S` and `G` only if the module does not already have one. With the mains disconnected, measure across `S` and `G`: a few hundred ohms means a burden is fitted.

---

## How it works

1. **Sample** both ADC channels continuously for **200 ms** — ten 50 Hz cycles.
2. **Remove the DC bias** by subtracting each channel's mean.
3. **Compute RMS** from the raw counts: `Vrms = sqrt(mean(v²))`, `Irms = sqrt(mean(i²))`.
4. **Compute real power** from `P = mean(v × i)`, which preserves the phase between voltage and current.
5. **Subtract idle noise** in quadrature, using values measured with the mains off.
6. **Apply calibration** (`V_CAL`, `I_CAL`) and small dead-bands so an idle sensor reads zero.
7. **Smooth** the result with a low-pass filter and start from the first real reading. A 5 s warm-up lets the bias capacitor and ADC settle after power-up.

The firmware samples voltage and current one after the other, so there is a small phase error. This is fine for monitoring, but it is not a revenue-grade meter.

---

## Quick start

### 1. Arduino IDE

Install **esp32 by Espressif Systems** from Boards Manager, then use:

| Setting | Value |
|---|---|
| Board | **ESP32C3 Dev Module** |
| USB CDC On Boot | **Enabled** (needed for Serial) |
| CPU Frequency | 160 MHz |
| Flash Size | 4 MB |
| Flash Mode | QIO (use DIO if it will not boot) |
| Partition Scheme | Default 4 MB with spiffs |
| Upload Speed | 921600 (try 460800 or 115200 if upload fails) |
| Core Debug Level | None |
| Port | The new COM port |

Open [`firmware/power_sense/power_sense.ino`](firmware/power_sense/power_sense.ino) and upload.

### 2. PlatformIO

```bash
cd firmware
pio run -t upload
pio device monitor
```

Verified with Arduino-ESP32 3.0.4: about 15% RAM and 77% flash used.

### 3. Connect

1. Join the WiFi network **`PowerSense`** (password **`12345678`**).
2. Open **http://192.168.4.1**.

Your phone may warn that the network has no internet. Stay connected — the dashboard is local.

### 4. First readings

Before calibration the numbers are raw ADC counts multiplied by the default factors, so they will look wrong. That is expected. Calibrate before judging accuracy.

---

## Calibration

Open **http://192.168.4.1/cal**. The page is intentionally not linked from the dashboard.

1. **Zero the noise** — with the CT empty and the mains **off**, tap **Zero idle noise**. This records the sensor/ADC noise so it can be subtracted.
2. **Voltage** — measure the real mains voltage with a multimeter, type it into *Real voltage*, and tap **Save calibration**.
3. **Current** — pass a known resistive load (for example a 60 W or 100 W bulb) through the CT. Real current is `watts ÷ volts`. Type it into *Real current* and tap **Save calibration**. Wait for the live reading to settle before saving.
4. **Verify** with a second load. For a resistive load, power should be close to the rated wattage.

Tune the ZMPT101B trimpot **once**, before calibrating, so the voltage signal is clean and the raw count sits roughly between **400 and 900** without clipping. Do not touch it afterwards.

Calibration is stored in flash. **Reset to defaults** restores `V_CAL = 0.305`, `I_CAL = 0.005`.

> The `/cal` page has no password. Anyone on the hotspot who knows the URL can change the calibration. Change the hotspot password in the sketch if that matters to you.

---

## Dashboard and API

| Route | Purpose |
|---|---|
| `/` | Live dashboard (V, A, W) |
| `/data` | JSON: `v`, `i`, `p`, `rv`, `ri`, `vc`, `ic` |
| `/cal` | Calibration page (unlinked) |
| `/setcal?v=…&i=…` | Save a new calibration |
| `/zero` | Record idle noise |
| `/reset` | Restore calibration defaults |

Example `/data` response:

```json
{"v":230.1,"i":0.442,"p":101.7,"rv":754.2,"ri":88.4,"vc":0.305000,"ic":0.005000}
```

`rv` and `ri` are the noise-removed signal levels in ADC counts; they are useful while calibrating.

---

## Troubleshooting

| Problem | Check |
|---|---|
| Upload fails | Hold **BOOT**, plug in USB-C, release, then upload. Try 460800 or 115200 baud. Use a data cable. |
| No Serial output | Set **USB CDC On Boot: Enabled**, open the port at **115200**. |
| Hotspot not visible | The ESP32 AP is 2.4 GHz only. Wait a few seconds after reset; check the board is powered. |
| Voltage shows 0 | ZMPT101B VCC on **3V3**, OUT on **GPIO3**, GND common, trimpot not at minimum. |
| Current always 0 | CT around **Live only**; `G` on the **1.65 V bias**, not ground; `S` through **1k** to GPIO4; load above ~0.03 A; calibrate. |
| Readings jump around | Fit the 100 nF filters, keep `WiFi.setTxPower(WIFI_POWER_8_5dBm)`, use a good USB supply, let the 5 s warm-up finish. |
| Reading is scaled wrong | Recalibrate. The trimpot or burden resistor may have changed. |
| Board resets or WiFi drops | Use a reputable 5 V 2 A charger and a short USB cable. |

---

## Limitations

- Accuracy depends on calibration and on the ESP32-C3 ADC, which is noisy and not perfectly linear.
- Very small loads (a few watts) read poorly.
- Voltage and current are sampled sequentially, so power factor accuracy is limited.
- No energy (kWh) accumulation, power factor display or history logging.
- Calibration page is unprotected.
- This is a hobby project, **not** a certified energy meter.

### Possible next steps

Station mode (join a home router), kWh logging, MQTT/Home Assistant, an external ADC such as the ADS1115, a protected calibration page, or a 3D-printed enclosure.

---

## Safety

This project connects to **230 V AC**, which can cause serious injury or death.

- Use an insulated enclosure and mains-rated wire.
- Keep mains wiring physically separate from the low-voltage side.
- Fuse the Live wire and keep Earth continuous.
- Never touch terminals while powered. Unplug before changing wiring.
- Use an RCD/GFCI and, preferably, an isolation transformer for the first test.
- Do not exceed **5 A**; stay below **4 A** continuous.
- Never open the CT secondary while current flows.
- Power the board from a reputable **5 V USB charger**. Disconnect mains before connecting it to a laptop.
- If you are not experienced with mains wiring, get help from someone who is.

Read the full checklist: **[docs/SAFETY.md](docs/SAFETY.md)**.

---

## Repository

```text
Power-Sense/
├── README.md
├── LICENSE
├── firmware/
│   ├── power_sense/
│   │   └── power_sense.ino      # firmware
│   └── platformio.ini
└── docs/
    ├── BOM.csv                  # parts and prices
    ├── PINOUT.md                # wiring reference
    ├── PLAN.md                  # design and test plan
    └── SAFETY.md                # mains safety checklist
```

## License

MIT — see [LICENSE](LICENSE).

## Author

Built by **Pradesh**, an engineering student working on robotics and embedded systems.

Repository: <https://github.com/pradeshwaster-design/Power-Sense>
