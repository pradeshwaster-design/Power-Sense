# Power Sense — Safety Checklist

This project connects directly to **230 V AC mains**. Mains electricity can kill you, start a fire, or destroy your equipment. Read this entire document before applying power.

> If you are not confident working with 230 V AC, stop and use a certified energy-monitoring plug instead. The skills needed are not learned from a web page.

---

## 1. Non-negotiable rules

1. **Never work on the circuit while it is plugged into mains.** Unplug, wait 30 seconds for capacitors to discharge, and verify with a multimeter before touching anything.
2. **Use a reputable 5 V USB charger.** It must be double-insulated and in good condition.
3. **Disconnect mains before connecting the board to a laptop.** Program and test the low-voltage side first, or use a charger and OTA.
4. **Fuse the Live wire** before the ZMPT101B and the sockets. Use a 5 A slow-blow fuse; a 0.1 A fuse for the ZMPT branch is recommended.
5. **Keep Earth continuous.** The earth pin must connect directly from the input plug to every socket earth. Never switch or fuse the earth.
6. **Keep at least 6 mm between mains and low-voltage conductors.** Use barriers and heat-shrink. Do not rely on solder mask for insulation.
7. **Use the isolated sensor modules.** Do not replace the ZMPT101B or the CT with a resistive divider on the mains side.
8. **Do not exceed 5 A.** The CT is rated 5 A. Stay below 4 A continuous.
9. **Use a CAT III multimeter** for mains measurements.
10. **Test with an RCD/GFCI and, ideally, an isolation transformer.** An isolation transformer removes the earth reference but does not limit fault current; an upstream RCD will not protect you from contact with the isolated secondary. Treat the secondary as live.
11. **One-hand rule.** Keep one hand in your pocket or behind your back when probing a live circuit.
12. **No jewellery, watches or loose sleeves.** Work on a dry, non-conductive surface.
13. **Do not leave the prototype unattended** during the first mains tests.
14. **Label the enclosure:** "230 V AC — 5 A MAX — DO NOT OPEN — NO USER SERVICEABLE PARTS".
15. **The strip is always live.** There is no relay. Unplug it to de-energize the sockets.
16. **Never open the CT secondary while current flows.** Keep the burden resistor connected. An open secondary can generate a dangerous voltage spike.
17. **The CT `G` pin is at 1.65 V, not ground.** Never connect it to mains, earth or GND.
18. **Do not sell or permanently install an uncertified prototype.**

---

## 2. Recommended test setup

```text
Wall outlet → RCD/GFCI → Isolation transformer (optional)
            → Variac (optional)
            → Fused monitor/strip
            → Known load (40 W bulb → 1000 W heater)
```

- Start with a **40 W incandescent bulb**.
- Move to a **1000 W heater** only after the low-power test passes.
- Use a **clamp meter** and a **CAT III multimeter** as references.
- Keep a **CO₂ or dry-powder fire extinguisher** nearby. Do not use water.
- Work with another person present who knows how to disconnect power.

---

## 3. Mains wiring checklist

- [ ] Live goes through the 5 A fuse first.
- [ ] ZMPT101B is across Live and Neutral, after the fuse.
- [ ] ZMPT branch has a 0.1 A fuse (recommended).
- [ ] CT is around **Live only**, after the fuse.
- [ ] No other wire passes through the CT hole.
- [ ] Earth is continuous and unswitched.
- [ ] All mains joints are soldered, crimped and heat-shrunk.
- [ ] No stray strands.
- [ ] Cable glands and strain relief fitted.
- [ ] Fuse rating marked.
- [ ] Enclosure closed before power is applied.
- [ ] There is **no relay**; the strip is live whenever plugged in.

---

## 4. Low-voltage checklist

- [ ] 3.3 V measured before connecting the sensors.
- [ ] ZMPT101B VCC on **3V3**, not 5 V.
- [ ] ZMPT OUT on GPIO3, about **1.65 V** with no mains.
- [ ] Bias node measures about **1.65 V**.
- [ ] CT **G** connected to the bias node, **not GND**.
- [ ] CT **S** goes through **1 kΩ** to GPIO4.
- [ ] Burden resistor present (module or 150 Ω).
- [ ] 22 µF fitted with **+ on the bias node**.
- [ ] Optional 100 nF filters from GPIO3/GPIO4 to GND.
- [ ] Common ground between the C3, ZMPT and bias divider.
- [ ] Low-voltage ground is **not** connected to mains neutral or earth.
- [ ] ADC wires are short and away from mains wiring.
- [ ] USB and mains are not connected at the same time while probing.

The CT module is **passive** and has no VCC. Do not power it.

---

## 5. First power-up procedure

1. Inspect the board under bright light; look for solder bridges, loose strands and reversed polarity.
2. Check continuity: L–N should be open except through the ZMPT101B; L–E and N–E should be open.
3. Check earth continuity from the input earth pin to each socket earth; target < 0.1 Ω.
4. Power the low-voltage side from a **5 V charger** with **no mains**.
5. Verify the board boots, the hotspot appears and the dashboard loads.
6. Measure the bias node and ZMPT OUT: about **1.65 V** each.
7. Open `/cal` and check the raw counts; both channels should be near mid-scale with little noise.
8. Connect mains through the RCD/isolation transformer with **no load**. Check the voltage reading.
9. Switch on a 40 W bulb. Check current and power.
10. Switch on a 1000 W heater for one minute. Compare current with a clamp meter.
11. Check temperatures: fuse holder, terminals, CT, ZMPT. Nothing should be too hot to touch.
12. Run for 30 minutes at 1000 W; check again.

If anything smells burnt, sparks, trips the RCD, or resets the ESP32, **stop immediately** and find the fault before reapplying power.

---

## 6. Enclosure and mechanical safety

- Use flame-retardant ABS/PC, not cardboard or wood.
- Separate the mains compartment from the low-voltage compartment with a plastic barrier.
- Keep the ESP32 antenna away from metal and mains wiring.
- Use cable glands and strain relief; a pulled cable must not pull on a terminal.
- Mount the fuse holder and terminals so a loose wire cannot touch the low-voltage side.
- Use a proper earth terminal with a star washer; bond the enclosure if it is metallic (preferably use plastic).
- Add ventilation if the charger or sensors get warm.
- Do not put the board in a metal box with the antenna inside.

---

## 7. Electrical safety tests

| Test | Method | Pass criterion |
|---|---|---|
| Earth continuity | Multimeter, input earth to each socket earth | < 0.1 Ω |
| Insulation resistance | 500 V tester, mains to low voltage | > 1 MΩ (preferably > 10 MΩ) |
| Leakage current | Proper leakage tester | Within applicable limits |
| Fuse rating | Visual | 5 A main, 0.1 A ZMPT branch |
| Creepage | Visual | ≥ 6 mm mains-to-low-voltage |
| Clearance | Visual | ≥ 3 mm L-to-N |
| Thermal | Thermocouple at 4 A | Terminals < 60 °C |
| CT operation | Compare with clamp meter at 1 A, 2 A, 4 A | Within ±5% after calibration |
| Strain relief | 10 kg pull test on cable | No movement at terminals |

An insulation tester is not a multimeter. If you do not have one, have the prototype inspected by someone who does. Do not skip this test if the device will be left plugged in.

---

## 8. What can go wrong

| Failure | Result | Prevention |
|---|---|---|
| CT `G` connected to GND | Clipped negative half-cycle, wrong readings | Connect `G` to the 1.65 V bias |
| No burden resistor | No signal, high CT voltage | Fit 150 Ω if the module lacks one |
| CT secondary opened under load | Dangerous voltage spike | Keep the burden connected |
| CT on both Live and Neutral | Reads zero | Pass Live only |
| CT overloaded above 5 A | Saturation, overheating | 5 A fuse, derate to 4 A |
| ZMPT on 5 V | Output may exceed 3.3 V at GPIO3 | Power it from 3V3 |
| GPIO3/GPIO4 driven above 3.3 V | Damaged ADC pin | Keep signals within 0–3.3 V |
| Missing fuse | Fire if wiring or sensor fails | 5 A fuse on Live |
| Earth switched or fused | Unsafe exposed metal | Earth continuous, unswitched |
| Creepage too small | Arc from mains to low voltage | 6 mm spacing, barrier |
| Loose mains strand | Short, fire | Ferrules, crimp, inspect, heat-shrink |
| Laptop connected while mains is on | Fault path through USB | Unplug mains before USB |

---

## 9. If you are unsure

- Use a certified energy-monitoring plug and integrate it over MQTT or Home Assistant.
- Use a low-voltage AC adapter and a current sensor for a safe learning project first.
- Have a licensed electrician inspect the mains wiring and enclosure.
- Do not put the prototype into permanent wall wiring.

**Mains electricity is not a learning sandbox. Build the low-voltage side first, test it thoroughly, and only then add mains under proper supervision.**
