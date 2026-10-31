# Power Sense — Safety Checklist

This project connects directly to **230 V AC mains**. Mains electricity can kill you, start a fire, or destroy your equipment. Read this entire document before applying power.

> If you are not confident working with 230 V AC, stop and use a certified energy-monitoring plug instead. The skills needed are not learned from a web page.

---

## 1. Non-negotiable rules

1. **Never work on the circuit while it is plugged into mains.** Unplug, wait 30 seconds for capacitors to discharge, and verify with a multimeter before touching anything.
2. **Use a reputable 5 V 2 A USB charger.** It must be double-insulated and in good condition. Do not use a cheap unbranded charger; the charger is part of the isolation barrier.
3. **Do not connect the ESP32 to a PC USB port while the sensors are connected to mains.** Unplug mains before programming, or use OTA.
4. **Fuse the Live wire** before the ZMPT101B and the sockets. Use a 5 A slow-blow fuse for the load; a 0.1 A fuse for the ZMPT branch is recommended.
5. **Keep Earth continuous.** The earth pin must connect directly from the input plug to every socket earth. Never switch or fuse the earth.
6. **Keep at least 6 mm between mains and low-voltage conductors.** Use PCB slots, barriers and heat-shrink. Do not rely on solder mask for insulation.
7. **Use the isolated sensor modules.** Do not replace the ZMPT101B or ZMCT103C with a non-isolated divider or a shunt on the mains side.
8. **Do not exceed 5 A.** The current sensor is rated 5 A. For continuous use, stay below 4 A.
9. **Use a CAT III multimeter** for mains measurements. A cheap CAT II meter can arc and injure you.
10. **Test with an RCD/GFCI and, ideally, an isolation transformer.** The RCD protects you on the earthed side. An isolation transformer removes the earth reference and allows safer probing, but it does not limit fault current by itself, and an upstream RCD will not protect you from contact with the isolated secondary. Treat the secondary as live.
11. **One-hand rule.** Keep one hand in your pocket or behind your back when probing a live circuit. Do not complete a circuit across your chest.
12. **No jewellery, watches or loose sleeves.** Work on a dry, non-conductive surface.
13. **Do not leave the prototype unattended** during the first mains tests.
14. **Label the enclosure:** "230 V AC — 5 A MAX — DO NOT OPEN — NO USER SERVICEABLE PARTS".
15. **The strip is always live.** This version has no relay. Unplug it to de-energize the sockets.
16. **Never open the CT secondary circuit while current is flowing.** Keep the ZMCT103C module and its burden resistor intact; an open CT secondary can generate a dangerous voltage spike.
17. **Do not sell or permanently install an uncertified prototype.** It has not been tested for isolation, creepage, thermal performance or EMC.

---

## 2. Recommended test setup

```text
Wall outlet → RCD/GFCI → Isolation transformer (optional but recommended)
            → Variac (optional, for gradual voltage)
            → Fused monitor/strip
            → Known load (40 W bulb → 1000 W heater)
```

- Start with a **40 W incandescent bulb**. It limits current and gives a visible indication.
- Move to a **1000 W heater** only after the low-power test passes.
- Use a **clamp meter** and a **CAT III multimeter** as references.
- Keep a **fire extinguisher** (CO₂ or dry powder) nearby. Do not use water.
- Work with another person present who knows how to disconnect power.

---

## 3. Mains wiring checklist

- [ ] Live wire goes through the 5 A fuse first.
- [ ] ZMPT101B is connected across Live and Neutral, after the fuse.
- [ ] ZMPT branch has a 0.1 A fuse (recommended).
- [ ] Current transformer is on Live only, after the fuse.
- [ ] Earth is continuous and unswitched.
- [ ] All mains joints are soldered, crimped and heat-shrunk.
- [ ] No stray strands; no wire free-standing.
- [ ] Cable glands and strain relief on input and output cables.
- [ ] Fuse rating and voltage rating marked.
- [ ] Enclosure closed before power is applied.
- [ ] There is **no relay** in this build; the strip is live whenever plugged in.

---

## 4. Low-voltage checklist

- [ ] 5 V rail measured before connecting the sensors.
- [ ] Sensors are powered from **5 V**, not 3.3 V.
- [ ] Sensor outputs are about 2.5 V DC with no signal.
- [ ] Divider outputs are about 1.25 V DC with no signal.
- [ ] Divider outputs never exceed 2.5 V.
- [ ] BAT54S clamps installed in the correct orientation.
- [ ] 100 µF and 100 nF capacitors installed on the 5 V rail.
- [ ] Common ground between ESP32, sensors and dividers.
- [ ] DC ground is **not** connected to mains neutral or earth.
- [ ] ADC wires are short and away from mains wiring.

---

## 5. First power-up procedure

1. Inspect the board under bright light; look for solder bridges, loose strands and reversed polarity.
2. Check continuity: L–N should be open except through the ZMPT101B; L–E and N–E should be open.
3. Check earth continuity from the input earth pin to each socket earth; target <0.1 Ω.
4. Power the low-voltage side from the 5 V charger **without mains**. Verify the ESP32 boots and the dashboard loads.
5. Measure 5 V at each sensor VCC pin and about 2.5 V at each sensor OUT pin.
6. Measure about 1.25 V at GPIO0 and GPIO1.
7. Connect mains through the RCD/isolation transformer with **no load**. Verify the voltage reading.
8. Switch on a 40 W bulb. Verify the current and power readings.
9. Switch on a 1000 W heater for one minute. Check the current reading against a clamp meter.
10. Check temperatures: fuse holder, terminals, sensors. Nothing should be too hot to touch.
11. Run for 30 minutes at 1000 W; check again.
12. Only then consider a longer test.

If anything smells burnt, sparks, trips the RCD, or resets the ESP32, **stop immediately** and find the fault before reapplying power.

---

## 6. Enclosure and mechanical safety

- Use flame-retardant ABS/PC, not bare cardboard or wood.
- Separate the mains compartment from the low-voltage compartment with a plastic barrier.
- Keep the ESP32 antenna away from metal and mains wiring.
- Use cable glands and strain relief; a pulled cable must not pull on a terminal.
- Mount the fuse holder and terminals so a loose wire cannot touch the low-voltage side.
- Use a proper earth terminal with a star washer; bond the enclosure if it is metallic (preferably use plastic).
- Add ventilation if the charger or sensors get warm; do not block the vents.
- Do not put the prototype in a metal box with the antenna inside.

---

## 7. Electrical safety tests

| Test | Method | Pass criterion |
|---|---|---|
| Earth continuity | Multimeter, input earth to each socket earth | <0.1 Ω |
| Insulation resistance | 500 V insulation tester, mains to low voltage | >1 MΩ (preferably >10 MΩ) |
| Leakage current | Proper leakage tester | Within applicable limits |
| Fuse rating | Visual | 5 A main, 0.1 A ZMPT branch |
| Creepage | Visual | ≥6 mm mains-to-low-voltage |
| Clearance | Visual | ≥3 mm L-to-N |
| Thermal | Thermocouple/thermal camera at 4 A | Terminals <60 °C |
| CT operation | Compare with clamp meter at 1 A, 2 A, 4 A | Within ±5% after calibration |
| Strain relief | 10 kg pull test on cable | No movement at terminals |

An insulation tester is not a multimeter. If you do not have one, have the prototype inspected by someone who does. Do not skip this test if the device will be left plugged in.

---

## 8. What can go wrong

| Failure | Result | Prevention |
|---|---|---|
| Sensor output connected directly to 3.3 V ADC | Clipped readings, damaged pin | 2:1 divider + BAT54S |
| ESP32 USB and mains connected | Lethal path through USB, damaged PC | Unplug mains before USB; use the 5 V charger or OTA |
| Poor 5 V charger | Weak isolation, shock/fire risk | Use a reputable double-insulated charger |
| CT overloaded above 5 A | Saturation, overheating, wrong readings | 5 A fuse, derate to 4 A |
| CT secondary opened under load | Dangerous voltage spike | Keep the module and burden resistor intact |
| Missing fuse | Fire if wiring or sensor fails | 5 A fuse on Live |
| Earth switched or fused | Unsafe exposed metal | Earth continuous, unswitched |
| Creepage too small | Arc from mains to low voltage | 6 mm spacing, slots, barrier |
| Loose mains strand | Short, fire | Ferrules, crimp, inspect, heat-shrink |
| Energy writes every second | Flash wear | Persist every 0.1 kWh/15 min |

---

## 9. If you are unsure

- Use a certified energy-monitoring plug and integrate it over MQTT or Home Assistant.
- Use a low-voltage AC adapter and a current sensor for a safe learning project first.
- Have a licensed electrician inspect the mains wiring and enclosure.
- Do not put the prototype into permanent wall wiring.

**Mains electricity is not a learning sandbox. Build the low-voltage side first, test it thoroughly, and only then add mains under proper supervision.**
