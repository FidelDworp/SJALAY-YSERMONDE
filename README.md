# Remote bediening WOLF-ketel + SWW-boiler via ESP32-C6 + 4G

**Project:** Remote in-/uitschakelen van de WOLF Brander F/CNK/U-25 + CB-155 boiler  
**Locatie:** Sjalay (Recht) – geen permanente WiFi/fiber  
**Datum overname:** september 2026  

---

## 1. Doel

De klassieke stookolieketel en de warmwaterboiler (SWW) op afstand kunnen bedienen (aan/uit warmtevraag) vanaf telefoon of Home Assistant, zonder permanente vaste internetverbinding.

**Identificatie toestellen (typeplaatjes):**
- **Ketel:** WOLF F/CNK/U-25  
  - Serienummer: 168120 / 1144  
  - Bouwjaar: 2014  
  - Vermogen: 20–25 kW  
  - Mat.-Nr.: 8906850  
- **SWW-boiler:** CB-155 / FB-155 / TB-155  
  - Inhoud: 155 liter  
  - Serienummer: 1214  

Huidige oplossing:
- TV/muziek via Telenet ONE op smartphones
- 4G-data via Telenet ONE SIM in dedicated router
- ESP32-C6 + RoomSense shield als IoT-controller met MQTT (of vergelijkbaar)

---

## 2. Netwerk / Internet

| Onderdeel              | Keuze                          | Opmerking |
|------------------------|--------------------------------|---------|
| 4G-router              | **TP-Link Archer MR600**       | Desktop-model, Cat6, externe antennes, 4× Gigabit |
| SIM                    | Telenet ONE data-SIM (onbeperkt) | Uit oude iPhone 5 gehaald |
| Snelheid (gemeten)     | ≈ 125 Mb/s down / 25 Mb/s up   | Meer dan voldoende voor IoT |
| Alternatief overwogen  | FRITZ!Box 6825 4G              | Goedkoper, alleen 2,4 GHz Wi-Fi 6, USB-C |

**Status:** Router is gekocht (Tweedekans Coolblue ± €110), geïnstalleerd en werkt perfect.

Oude iPhone 5 (opgeblazen batterij) → later inleveren bij Krëfel Geraardsbergen (Astridlaan 40).

---

## 3. Ketel- en boiler-interface (kern)

### 3.1 Parametreerbare ingang E1
De ketel heeft één **parametreerbare ingang E1** (potentiaalvrij contact).  
Deze wordt geconfigureerd via parameter **HG13**.

| HG13-instelling     | Effect bij **open** contact                  | Effect bij **gesloten** contact      | Gebruik |
|---------------------|----------------------------------------------|--------------------------------------|---------|
| **RT** (waarde 1)   | Alleen verwarming geblokkeerd (zomerstand)   | Verwarming vrijgegeven               | Alleen CV |
| **WW / DHW**        | Alleen warmwaterbereiding geblokkeerd        | Warmwaterbereiding vrijgegeven       | Alleen SWW |
| **RT/WW** of **RT/DHW** | Verwarming **én** warm water geblokkeerd | Beide vrijgegeven                    | Gecombineerd (aanbevolen start) |

De BM 2744329 communiceert via **eBUS**. E1 werkt onafhankelijk/parallel van de BM.

### 3.2 Waar vind ik het E1-contact?
1. Open de regelaar / bedieningspaneel van de ketel (meestal vooraan of zijkant).
2. Zoek de klemmenstrook of stekkerlijst.
3. Zoek de klemmen die gemarkeerd zijn als **E1** (soms “Eingang E1” of “parametreerbare ingang”).
4. Het is een 2-polige aansluiting (potentiaalvrij).  
   Maak **duidelijke foto’s** van de hele klemmenstrook voordat je iets losmaakt.

### 3.3 Hoe parameters (HG13) bekijken en wijzigen?
1. Ga naar de **BM 2744329** bedieningsmodule (muurthermostaat).
2. Ga naar het **installateurs-/vakmanniveau**.  
   Meestal door een code in te voeren (vaak **1111** of vergelijkbaar – zie handleiding BM of probeer standaard WOLF-codes).
3. Zoek parameter **HG13** (of “Eingang E1” / “Functie E1”).
4. Noteer de huidige waarde.
5. Wijzig indien nodig naar de gewenste functie (RT, WW of RT/WW).
6. Sla op en verlaat het installateursniveau.

**Tip:** Noteer altijd de originele waarde voordat je iets wijzigt.

### 3.4 Aanbevolen hardware-aansturing
- ESP32-C6 stuurt een **potentiaalvrij relais** via een GPIO
- Relaiscontact over de E1-klemmen van de ketel
- Originele BM blijft bij voorkeur aangesloten als backup

**Belangrijke veiligheidsregels**
- Nooit veiligheidscontacten (STB, maximumthermostaat, druk…) overbruggen
- Galvanische scheiding via relais (optocoupler of goed relais)
- Fail-safe overwegen (NC-contact of watchdog) zodat de ketel niet permanent blijft branden bij crash van de ESP
- Eerst testen met ketel uitgeschakeld / in service-stand

---

## 4. Hardware-architectuur (RoomSense shield)

De ESP32-C6 wordt gemonteerd op een **RoomSense shield**.  
Dit shield heeft twee RJ45-aansluitingen:

| Kabel              | Bestemming                                      | Lengte     | Doel |
|--------------------|--------------------------------------------------|------------|------|
| **RoomSense UTP**  | RoomSense sensorprintje (temp, licht, PIR, …)   | tot 10 m   | Sensoren in de kamer boven de ketel |
| **OPTIONAL RJ45**  | Relais-module                                   | kort       | Aansturing van het E1-relais |

### Aanbevolen plaatsing
- **ESP32-C6 + shield + relais** → in behuizing **in de kelder bij de ketel** (korte, betrouwbare bedrading naar E1).
- **RoomSense sensorprintje** → via max. 10 m UTP-kabel op de muur in de kamer boven de ketel (betere meetwaarden voor temperatuur/licht).

### Pin-mapping RoomSense shield (ESP32-C6)

| ESP32-C6 Pin | Device/Functie                                  | Opmerking / Gebruik voor dit project |
|--------------|--------------------------------------------------|--------------------------------------|
| IO13         | I2C SDA (Pull-up 4.7k → 5V)                     | Sensoren |
| IO11         | I2C SCL (Pull-up 4.7k → 5V)                     | Sensoren |
| IO3          | DS18B20 OneWire                                 | Temperatuursensor |
| IO4          | Pixels data                                     | Status-LED’s |
| IO5          | MOV1 PIR                                        | Beweging |
| IO6          | DHT22 data                                      | Temp/vocht |
| IO12         | Sharp dust LED out                              | Stofsensor |
| IO7          | Sharp dust analog                               | Stofsensor |
| IO1          | LDR1 analog (10k pull-up → 3V3)                 | Lichtsensor |
| IO18         | CO2 PWM input (MH-Z19 = 5V!)                    | CO₂ |
| IO19         | MOV2 PIR (of Dotstar CLK)                       | Beweging 2 |
| **IO10**     | **TSTAT switch (Gnd = ON)**                     | **Zeer geschikt voor relais-aansturing** |
| IO2          | LDR2 analog                                     | Lichtsensor 2 |
| **IO15**     | **Reserve 2 (Output)**                          | **Goed alternatief voor relais** |
| **IO20/WKP** | **Reserve 3**                                   | Alternatief voor relais |
| IO0          | Reserve 1 (BOOT pin – voorzichtig)              | Liever niet gebruiken |
| GND          | Ground                                          | — |
| VIN          | Power Input (3.6–6V)                            | Voeding |
| 3V3          | 3.3V Output                                     | — |

**Aanbeveling voor het relais:**  
Gebruik **IO10** (TSTAT switch) of **IO15** (Reserve 2) als stuuruitgang naar de relais-module.  
IO10 is ontworpen voor thermostat-schakeling en is daardoor de meest logische keuze.

---

## 5. Geplande hardware (eindopstelling)

- ESP32-C6 op **RoomSense shield**
- Behuizing in de kelder bij de ketel
- Relais-module (via OPTIONAL RJ45 of direct)
- RoomSense sensorprintje (via max. 10 m UTP) in de kamer boven
- Temperatuur-, licht- en eventueel andere sensoren
- Voeding via de 4G-router of aparte adapter

---

## 6. Componentenlijst om mee te nemen naar Sjalay (testen)

### Essentieel
- [ ] ESP32-C6 + RoomSense shield
- [ ] 5V of 3.3V **relais-module** (bij voorkeur met optocoupler / galvanische scheiding)
- [ ] RJ45-kabel(s) voor RoomSense + OPTIONAL
- [ ] Jumperkabels / breadboard-draadjes
- [ ] USB-C kabel + powerbank of adapter
- [ ] Multimeter
- [ ] Schroevendraaierset + zaklamp

### Handig
- [ ] Laptop/telefoon met serial monitor / ESPHome
- [ ] Korte 2-aderige kabel (0,5–0,75 mm²) voor E1
- [ ] Isolatietape / krimpkous
- [ ] Foto’s van klemmenstrook (E1)

### Optioneel
- [ ] Temperatuursensor (DS18B20 of BME280)
- [ ] LED + weerstand als statusindicatie

---

## 7. Stappenplan (hoog niveau)

1. **Op Sjalay – verkenning**
   - Typeplaatjes controleren (al gedaan)
   - Ketel openen → **E1-klemmen lokaliseren** + foto’s
   - Op de BM → installateursniveau → **HG13** uitlezen
   - Beslissen: RT, WW of RT/WW

2. **Testopstelling**
   - Relais aansluiten op E1 (via IO10 of IO15)
   - ESP32 simpele sketch: relais open/dicht
   - Controleren of ketel/boiler correct reageert

3. **Software**
   - ESPHome of Arduino + MQTT
   - Verbinding met Archer MR600 (2,4 GHz)
   - Remote bediening + status

4. **Definitieve montage**
   - ESP + shield + relais in behuizing in de **kelder**
   - RoomSense sensorprintje via max. 10 m UTP in de kamer boven
   - Netjes bedraden + fail-safe

---

## 8. Belangrijke documentatie / referenties

- WOLF parameter **HG13** = functie van ingang E1 (RT / WW / RT/WW)
- BM 2744329 = bedieningsmodule (eBUS)
- Archer MR600 = 4G Cat6 dual-band router
- ESP32-C6 + RoomSense shield = Wi-Fi 6 + sensoren + TSTAT-uitgang

---

## 9. Status (25 sept 2026)

- [x] 4G-router gekozen, gekocht en werkend (125/25 Mb/s)
- [x] Oude iPhone 5 onbruikbaar → later recyclen bij Krëfel
- [x] Typeplaatjes ketel + boiler gedocumenteerd
- [x] RoomSense shield pinout + architectuur vastgelegd
- [ ] E1-klemmen en HG13 ter plaatse controleren
- [ ] Relais-test met ESP32 (IO10 of IO15)
- [ ] Definitieve software + behuizing

---

**Veiligheid eerst.**  
Bij twijfel over de aansluiting: foto’s maken en eerst raadplegen voordat er permanent wordt aangesloten.

Dit document is bedoeld als overnamedossier voor de repository.
