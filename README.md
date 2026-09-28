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

---

De sketch voor de SJALAY is gebaseerd op deze twee recentste sketches voor ROOM en HVAC:
- ESP32_C6_MATTER_ROOM_7mei_1330.ino
- ESP32_C6_MATTER_HVAC_18mar_1105.ino

PLAN: Eigenlijk moet deze controller (buiten de algemene platform functionaliteit en web UI) vooral deze vereenvoudigde serie taken uitvoeren om ons vakantiehuis vanop afstand te kunnen monitoren en besturen:

- De standaard "roomsensors" van de roomsense pcb in de UI uitlezen. (Niet de optionele)
- De sensor waarden met JSON string naar google sheets sturen om te loggen op lange termijn
- Een 5V relais bedienen als de kamertemperatuur (DS18B20 en DHT22) onder de gewenste temperatuur is. Volg de bestaande logica ook ivm vocht. (De TSTAT sense functie uit de room sketch mag ook weggelaten worden.)
- De pixel uitgang (IO4) moet een serie Powerpixels kunnen aansturen vanuit de UI met de bestaande logica

Hier is een complete lijst van features die te realizeren zijn. Gebruik dit als leidraad.

Bouwlijst voor de Sjalay-sketch. Geen Matter, geen TSTAT, geen optionele RoomSense-sensoren, geen Flobecq-kringen/ECO.

A. Platform

ESP32-C6, #define Serial Serial0
Partities 16 MB: nvs 20 KB, otadata 8 KB, app0/app1 6 MB, SPIFFS ~4 MB
Wi-Fi STA; SSID/wachtwoord uit NVS
Static IP uit NVS, anders DHCP (gateway = x.x.x.1)
Wi-Fi reconnect in loop() bij verlies (4G)
AP-fallback + captive portal als Wi-Fi faalt (ROOM-<naam> / Sjalay-Setup)
NTP + tijdzone CET/CEST
NVS room-config voor alle instellingen
AsyncTCP + ESPAsyncWebServer poort 80
OTA .bin + reboot
Factory reset via web + serial R binnen 5 s na boot / reset_nvs
Crash-log in NVS (largest heap-block < 25 KB), teller + wissen in settings
Geen mDNS, geen Matter, geen serial-statusdump (alleen boot-R)


B. Web-UI (look van de roomsketch)

Gele header (naam + uptime + datum/tijd), rode sidebar, witte pagina, blauwe labels
Sidebar: Status / OTA / JSON / Settings (geen Matter)
/ status — groepen + tabellen + sliders/switches
Live-refresh via JS fetch('/json') (hybrid, weinig heap)
/settings — formulier, opslaan + reboot
/json compacte keys (Sheets + UI)
/update OTA + reboot-knop
Mobiele CSS max-width: 600px
Sensor-⚠ rood (defect) / oranje (verdacht)
HTML chunked AsyncResponseStream + char[] i.p.v. String (heap-arm)
CORS * op JSON (dashboard/HA later)


C. Pinnen (RoomSense + relais)

PinFunctieIO6DHT22IO3DS18B20 OneWireIO1LDR1IO5PIR MOV1IO4NeoPixel / PowerpixelsIO10Relais → WOLF E1 (uitgang, actief LOW of zoals module)IO15ongebruikt (reserve relais)IO13/11I2C ongebruikt (geen TSL, geen MCP)

Relais uit in setup() vóór Wi-Fi (fail-safe: E1 open)
Watchdog: hang → reset → relais blijft uit tot logica weer loopt
Relais volgt heating_on (geen 10 min-override)


D. Sensoren (alleen standaard RoomSense)

DHT22: temp + vocht, dauwpunt
DS18B20 max 4: scan/rescan, CRC, nicknames, primaire → room_temp
Fallback: DS ongeldig (NaN / <5 / >40 °C) → DHT22; beide defect → room_temp = 0 + melding
LDR1 0–100 (donker = 100)
PIR MOV1: LOW = beweging, triggers/min, licht-aan-timer
Niet: CO₂, stof, TSL2561, MOV2, beam/LDR2, TSTAT-ingang


E. Verwarming (softwarethermostaat)

Schakelaar Verwarming in de UI (persistent NVS)
Uit = relais altijd open (zomer)
Aan = thermostat

Setpoint-slider 10–30 °C (persistent)
Dauwpuntbeveiliging: effective = max(setpoint, dew + dew_margin)
heating_on = (verwarming aan) && (room_temp < effective − 0.5)
Relais IO10 volgt heating_on onmiddellijk
Duty% live + sliding window 4 u (12 × 20 min) → JSON/Sheets
UI toont: room temp (DS + DHT), vocht, dauwpunt, dew-alert, setpoint, verwarming aan/uit, ketelvraag (heating_on), relais
Niet: TSTAT, Thuis/Uit, override 10 min, vent%-slider, vent-PWM, HTTP-poll van andere ESPs


F. Powerpixels (IO4) — bestaande room-logica, zonder MOV2

1–30 pixels, aantal + nicknames in NVS
Fade-engine sin-ease, 1–10 s
RGB-kleurkiezer (web + NVS)
Pixel 0: AUTO = MOV1 + LDR donker, of manueel aan
Pixel 1+: manueel aan/uit, persistent
Licht-aan tijd 0–30 min + 5 s overtime
Bed-modus: MOV-pixel(s) gedwongen uit
/capabilities — pixelnamen JSON
/setcolor, /toggle_pixel_mode, /toggle_pixel, /set_fade_duration, /set_light_on_min, /toggle_bed


G. Google Sheets

gas_url in settings (leeg = uit)
HTTPS POST elke 5 min, payload = /json
Compact schema (één circuit, geen SCH/ECO):

H. Settings-velden

Room-naam, Wi-Fi SSID/wachtwoord, static IP, MAC (read-only)
Heating setpoint default, dew-margin
LDR dark-threshold
Aantal pixels, default RGB, pixelnamen
DS18B20 nicknames, primaire sensor, rescan 1-Wire
Google Script URL
Crashteller + wissen
Opslaan + reboot; factory reset-knop
Niet: CO₂/stof/zon/MOV2/beam/TSTAT-checkboxes, ECO, circuits, vent default, serial-verbose


I. Statuspagina-groepen

HVAC — temps, vocht, dauwpunt, alarm, setpoint-slider, verwarming-switch, ketelvraag-dot, relais-dot
Verlichting — LDR, MOV1-dot, licht-tijd, RGB, bed, dim-snelheid, pixels
Beweging — MOV1 trig/min
Controller — IP, RSSI, heap, largest block


J. Wat er bewust níet in zit

- Matter / HomeKit / /matter
- TSTAT-ingang, Thuis/Uit, override 10 min
- MCP23017, 7 kringen, rooms pollen
- ECO-boiler, SCH/WON-pompen, 6 vaste boiler-DS
- CO₂, stof, TSL2561, MOV2, beam
- Ventilator-PWM IO20
- mDNS, MQTT (later als remote-vanuit-Flobecq nodig is)

---

## 10. SJALAY-sketch — huidige implementatie (v0.9, 27 sep 2026)

Dit hoofdstuk beschrijft wat er **effectief gebouwd en werkend getest** is, als
aanvulling op de bouwlijst (A–J) hierboven. Bij twijfel is de code in de
sketch (`SJALAY_CONTROLLER_v0.9_...ino`) de bron van waarheid; dit is een
leeswijzer erbij.

### 10.1 Bestand en board-instellingen

- Bestandsnaam bevat versie + datum, bv. `SJALAY_CONTROLLER_v0.9_27sep_pixel0fix.ino`.
- Arduino IDE: **Board** = ESP32C6 Dev Module, **Flash Size** = 16MB,
  **Partition Scheme** = Custom (`partitions.csv` in dezelfde map),
  **USB CDC On Boot** = Enabled.
- `#define Serial Serial0` staat bovenaan — verplicht op de C6, anders werkt
  de seriële monitor niet correct.
- Versienummer staat in `#define SJALAY_VERSION` en verschijnt in de UI
  (Controller-groep) en in `/json` (veld `ver`).

### 10.2 Effectief gebruikte pinnen

Van de volledige RoomSense-pinout (sectie 4) gebruikt de Sjalay-sketch enkel:

| Pin | Functie | Opmerking |
|-----|---------|-----------|
| IO6 | DHT22 data | temp + vocht |
| IO3 | DS18B20 OneWire | tot 4 sensoren op 1 bus |
| IO1 | LDR1 analoog | 0-100 geschaald, donker = 100 |
| IO5 | PIR MOV1 | `INPUT_PULLUP`, LOW = beweging |
| IO4 | NeoPixel data | powerpixels, 1-30 stuks |
| IO10 | Relais → WOLF E1 | actief-laag (`RELAY_ACTIVE_LOW`), fail-safe UIT vóór Wi-Fi |
| IO15 | (nog) niet gebruikt | gereserveerd voor eventueel 2e relais |

I²C (IO11/IO13), CO₂ (IO18), stof (IO7/IO12), LDR2 (IO2) en MOV2 (IO19)
worden bewust niet aangesproken — zie punt J.

### 10.3 Webinterface — pagina's en endpoints

**Pagina's** (zichtbaar in de sidebar): `/` (Status), `/update` (OTA),
`/settings` (Settings). `/json` staat ook in de sidebar als rechtstreekse
link naar de ruwe data.

**Actie-endpoints** (allemaal `HTTP GET`, AJAX via `submitAjax()` in de
statuspagina, antwoorden met `text/plain "OK"` tenzij anders vermeld):

| Endpoint | Werking |
|---|---|
| `/save_settings` | verwerkt het volledige Settings-formulier, herstart daarna |
| `/factory_reset` | wist alle NVS-instellingen, herstart |
| `/clear_crash_log` | wist de crash-teller |
| `/rescan_ds` | herscant de DS18B20-bus, redirect naar `/settings` |
| `/toggle_heating_auto` | wisselt Automatisch/Handmatig voor verwarming |
| `/toggle_relay_manual` | wisselt relaisstand in handmatige modus |
| `/set_setpoint?value=` | setpoint-slider (10-30 °C) |
| `/toggle_pixel_mode` | pixel 0: AUTO ↔ MANUEEL |
| `/toggle_pixel?idx=` | pixel `idx` aan/uit (idx 0 = pixel 0 in MANUEEL, idx 1+ = normale pixels) |
| `/setcolor?r=&g=&b=` | zet + bewaart de powerpixel-kleur, past onmiddellijk toe |
| `/set_fade_duration?value=` | dim-snelheid (1-10 s) |
| `/set_light_on_min?value=` | licht-aan tijd na PIR-trigger (0-30 min) |
| `/toggle_bed` | bed-modus aan/uit (dwingt pixel 0 uit, zie 10.6) |
| `/capabilities` | JSON met pixelnamen (voor eventuele externe dashboards) |
| `/reboot` | herstart direct |

### 10.4 Settings-pagina — velden

Eén formulier (`/save_settings`, herstart na opslaan) met:

- **Algemeen**: room-naam, Wi-Fi SSID/wachtwoord, static IP (leeg = DHCP),
  dauwpuntmarge, LDR donker-drempel (0-100), aantal pixels (1-30, herstart
  nodig om echt van kleur/lengte te veranderen), Google Script-URL (leeg =
  logging uit), MAC-adres (alleen-lezen).
- **Pixel-namen**: 1 tekstveld per geconfigureerde pixel (pixel 0 heeft
  "(MOV1)" als hint).
- **Sensoren (DS18B20)**: 1 tekstveld per gevonden sensor (met huidige
  temperatuur als referentie) + een dropdown om de **primaire sensor** te
  kiezen (die bepaalt `room_temp` samen met de DHT22-fallback).
- Los van dat formulier: **Herscan DS18B20-bus** (navigeert direct weg —
  eerst opslaan als er nog wijzigingen in het formulier staan), **crash-log
  wissen**, en **factory reset** (met bevestigingsdialoog).

### 10.5 Verwarmingslogica (samengevat)

- Twee modi, via `/toggle_heating_auto`: **Automatisch** (softwarethermostaat)
  of **Handmatig** (directe schakelaar, handig om te testen zonder werkende
  sensoren).
- Automatisch: `effective_setpoint = max(setpoint, dauwpunt + dew_margin)`;
  relais gaat aan als `room_temp < effective_setpoint - 0.5`.
- Handmatig: relais volgt gewoon de `/toggle_relay_manual`-schakelaar.
- In beide gevallen wordt het relais **onmiddellijk** aangepast bij elke
  wijziging (geen wachttijd tot de volgende sensorcyclus).
- **Duty-cyclus** (nieuw in v0.8): een 4-uur sliding window opgebouwd uit
  12 blokken van 20 minuten. Elk blok registreert welk aandeel van die
  20 minuten het relais aan stond; het duty%-veld is het gemiddelde van de
  laatste (max 12) blokken. Zichtbaar in de UI en meegestuurd in `/json`
  en dus ook naar Google Sheets.

### 10.6 Powerpixels — pixel 0 en bed-modus (belangrijk!)

Pixel 0 is de "MOV-pixel" en heeft twee lagen logica, in deze volgorde van
voorrang:

1. **Bed-modus** (schakelaar in de groep "Verlichting") — als die AAN staat,
   is pixel 0 **altijd** uit, wat de modus (AUTO/MANUEEL) ook is. Dit is een
   bewuste ontwerpkeuze (zie punt F: "bed-modus dwingt MOV-pixel(s) uit"),
   bedoeld om 's nachts geen bewegingslicht te krijgen.
2. **Modus AUTO/MANUEEL** (schakelaar bovenaan de Powerpixels-groep) —
   enkel relevant als bed-modus UIT staat:
   - AUTO: pixel 0 gaat aan bij beweging (PIR MOV1) **en** het is donker
     genoeg (LDR boven de donker-drempel).
   - MANUEEL: een aparte AAN/UIT-schakelaar verschijnt, rechtstreeks
     bediend door de gebruiker.

**Aandachtspunt uit de praktijk:** als bed-modus per ongeluk aan blijft
staan van een eerdere test, lijkt de MANUEEL-schakelaar van pixel 0 niet te
werken (hij springt in de UI terug uit en de LED gaat nooit branden) — dit
is geen bug, bed-modus wint gewoon altijd. Sinds v0.9 toont de UI dit
expliciet met een eigen statusregel ("Bed-modus actief → geforceerd UIT")
zodra dat het geval is, met een knop om ze direct uit te zetten.

Pixels 1 en hoger zijn altijd rechtstreeks manueel aan/uit, persistent in
NVS, zonder bed-override.

### 10.7 Google Sheets-logging

- Ingeschakeld zodra `gas_url` (Apps Script webhook-URL) is ingevuld in
  Settings.
- Elke 5 minuten (niet in AP-modus) wordt de volledige `/json`-payload via
  HTTPS POST naar die URL gestuurd (`WiFiClientSecure` met `setInsecure()`,
  Apps Script's typische 302-redirect wordt gevolgd).
- Laatste resultaatcode (`gcode`) en tijdstip zijn zichtbaar in de
  "Logging"-groep op de statuspagina.

### 10.8 Shelly-stopcontacten per pixel (optioneel, nieuw in v0.10)

Elke pixel (ook pixel 0) kan gekoppeld worden aan **0 tot 3 Shelly
slimme stopcontacten**, die dan simultaan meeschakelen met de pixel.
Bedoeld voor bv. een echte lamp die mee moet gaan met een powerpixel als
indicatie, zonder dat er Matter/HomeKit/cloud bij komt kijken — alles
verloopt lokaal over het eigen netwerk.

**Instellen** (Settings, onder "Pixel-namen"): een extra tekstveld per
pixel met het formaat
```
192.168.0.50:Keukenlamp,192.168.0.51:Tafellamp
```
— `ip:nickname`-paren gescheiden door een komma, max. 3 per pixel. Leeg
laten = geen koppeling voor die pixel.

**Werking:** de sketch onthoudt de vorige aan/uit-status van elke pixel
en vergelijkt die elke lus-cyclus met de actuele status
(`updatePixelLogic()`). Enkel bij een **effectieve wissel** (rand-detectie,
dus niet continu) stuurt hij voor elk gekoppeld IP een korte lokale
HTTP-GET:
```
http://<ip>/relay/0?turn=on
http://<ip>/relay/0?turn=off
```
Dit endpoint werkt zowel op oudere (Gen1) als nieuwere (Gen2/Gen3, "Plus")
Shelly-stopcontacten, zonder Shelly-cloud-account of app nodig. Er is
bewust **geen retry en geen terugkoppeling** ingebouwd (fire-and-forget,
zoals de Sheets-log) — met een korte timeout (1-1,5s connect/response) zodat
een uitgeschakelde of onbereikbare Shelly de hoofdlus niet blokkeert.

**Op de statuspagina** staat naast elke pixelnaam, indien gekoppeld, een
kleine grijze aanduiding met de ingestelde nicknames (bv. "→ Keukenlamp,
Tafellamp"). Dit toont enkel wát er gekoppeld is, niet de actuele
live-status van de Shelly zelf (geen polling, om de pagina snel te houden).

**Praktisch:** geef elke Shelly een **vast IP** (reservering in de router,
of instelbaar in de Shelly-app zelf) — anders verandert het gekoppelde IP
na een herstart van de Shelly en moet je het opnieuw instellen in Settings.

### 10.9 Gekende aandachtspunten

- NVS-instellingen (Wi-Fi, kleuren, pixelnamen, Shelly-koppelingen,
  bed-modus, …) blijven behouden over firmware-updates heen (zelfde
  `Preferences`-namespace `"sjalay-cfg"`). Enkel een **factory reset** (web
  of seriële `R` binnen 5 s na boot) wist alles. Test dus na een update
  altijd even de status van schakelaars zoals bed-modus, automatische
  verwarming, enz. — die staan mogelijk nog zoals bij de vorige test.
- Zonder RoomSense-shield aangesloten werkt de sketch nog steeds: DHT22
  geeft NaN, DS18B20-telling is 0, dit wordt netjes gedetecteerd en getoond
  (geen crash), met een melding in de HVAC-groep.
- De Shelly-koppeling is volledig los te testen van de RoomSense-hardware:
  ook zonder sensoren kan je pixels manueel aan/uit zetten in de UI en zo
  de Shelly-HTTP-calls verifiëren (serial monitor toont `[Shelly] ... -> HTTP ...`).

### 10.10 Versiegeschiedenis (kort)

| Versie | Belangrijkste inhoud |
|---|---|
| v0.1 – v0.1.1 | Platformlaag (Wi-Fi/AP/NTP/OTA/crash-log/factory-reset), AP-SSID-zichtbaarheidsfix |
| v0.2 | Sensoren: DHT22, DS18B20, LDR1, PIR MOV1 |
| v0.3 | Verwarmingslogica + relais IO10 |
| v0.4 | Powerpixels (fade-engine, AUTO/manueel, bed-modus) |
| v0.5 | Volledige AJAX-live-UI + compact `/json` |
| v0.6 | UI-stijl exact zoals ROOM-sketch, heap-KB-bug gefixt |
| v0.7 – v0.7.1 | Pixel-0 AUTO/MANUEEL-herwerking, Google Sheets-logging, compile-fix |
| v0.8 | STAP 7: DS18B20-nicknames/primaire sensor instelbaar, LDR-drempel instelbaar, duty-cyclus 4u |
| v0.9 | Bugfix: bed-modus/pixel-0-interactie nu expliciet zichtbaar in de UI |
| v0.10 | STAP 8: optionele Shelly-stopcontacten per pixel (lokale HTTP, geen cloud) |

---

## 11. JSON-velden (`/json`)

Dit is het compacte schema dat zowel de live-UI (elke 3 s via `fetch`) als
de Google Sheets-log (elke 5 min) gebruikt. Booleans staan als JSON
`true`/`false`, niet als 0/1 (behalve waar expliciet vermeld).

| Veld | Type | Beschrijving |
|---|---|---|
| `rid` | string | Room-naam (uit Settings) |
| `ver` | string | Firmwareversie (bv. `"0.9"`) |
| `ip` | string | IP-adres (of `192.168.4.1` in AP-modus) |
| `rssi` | int | Wi-Fi signaalsterkte in dBm (0 in AP-modus) |
| `heap` | uint | Vrije heap in KB |
| `lb` | uint | Grootste vrije geheugenblok in KB (indicator voor fragmentatie) |
| `crash` | uint | Aantal geregistreerde crashes (lage-heap-events) |
| `upt` | ulong | Uptime in seconden sinds boot |
| `ap` | bool | `true` = toestel zit in AP/setup-modus |
| `t2` | float | DHT22-temperatuur in °C (0 als sensor defect/ontbreekt) |
| `t2ok` | bool | DHT22-meting geldig? |
| `h` | float | DHT22-relatieve vochtigheid in % |
| `dp` | float | Berekend dauwpunt in °C (uit DHT22) |
| `t1` | float | DS18B20 primaire-sensor-temperatuur in °C |
| `dsok` | bool | Minstens 1 DS18B20 gevonden? |
| `dsc` | int | Aantal gevonden DS18B20-sensoren (max 4) |
| `rt` | float | `room_temp` — effectief gebruikte kamertemperatuur (DS18B20 primair, fallback DHT22, anders 0) |
| `tm` | string | Waarschuwingstekst bij sensorfout (leeg = alles ok) |
| `ldr` | int | LDR1-lichtwaarde, geschaald 0-100 (100 = donker) |
| `mov` | int | Aantal PIR MOV1-triggers in de laatste minuut |
| `hauto` | bool | Verwarming in automatische modus (softwarethermostaat)? |
| `hsp` | int | Ingestelde setpoint-temperatuur in °C (10-30) |
| `heff` | float | Effectieve setpoint in °C (incl. dauwpuntmarge indien van toepassing) |
| `rman` | bool | Gewenste relaisstand in handmatige modus |
| `hon` | bool | Huidige ketelvraag / relaisstand (`heating_on`) |
| `duty` | float | Duty-cyclus verwarming over de laatste 4 uur, in % |
| `bed` | bool | Bed-modus actief? (dwingt pixel 0 uit) |
| `p0m` | int | Pixel-0-modus: `0` = AUTO, `1` = MANUEEL |
| `p0on` | bool | Gewenste pixel-0-staat in MANUEEL (los van of bed-modus dat overschrijft) |
| `pn` | int | Aantal geconfigureerde pixels |
| `fd` | int | Dim/fade-snelheid in seconden (1-10) |
| `lom` | int | Licht-aan-tijd in minuten (0-30) na een PIR-trigger |
| `pon` | string | Bitstring (lengte = `pn`) met actuele aan/uit-status per pixel; teken op index *i* = pixel *i* (`'1'`=aan, `'0'`=uit) |
| `nr`, `ng`, `nb` | int | Huidige RGB-kleurwaarde (0-255) van de powerpixels |
| `gas` | bool | Google Sheets-logging ingeschakeld (URL niet leeg)? |
| `gcode` | int | Laatste HTTP-resultaatcode van de Sheets-POST (0 = nog niet geprobeerd) |
