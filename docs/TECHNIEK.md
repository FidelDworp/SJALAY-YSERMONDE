# Techniek — gemeenschappelijk voor alle huizen

De code is de bron van waarheid; dit is een leeswijzer. Huisspecifieke installatie: [SJALAY.md](SJALAY.md), [YSERMONDE.md](YSERMONDE.md).

## 1. Compileren en flashen

- **Arduino IDE:** board **ESP32C6 Dev Module**, **Flash Size 16 MB**, Partition Scheme **Custom** (`partitions.csv` in de sketchmap, overschrijft de menukeuze: borden met 4 MB zijn dus niet bruikbaar), **USB CDC On Boot: Disabled** (anders stopt het compileren met een foutmelding; IO12/IO13 zijn dan gewone pinnen, IO13 = I2C-SDA).
- **Toolchain:** ESP32-core 3.3.12, ESPAsyncWebServer 3.12.1 + Async TCP (Filip lokaal 3.5.0, Claude 4.0.0: beide werken), OneWireNg, Adafruit NeoPixel. Geen DHT-bibliotheek (eigen uitlezing).
- **Eerste flash** van een nieuw bord met de kabel via de USB-C-poort **"UART"** (bootloader + partitietabel). Daarna alles via **OTA**.
- **Seriële monitor:** 115200 baud, poort "UART". Het openen van de poort herstart het bord (reset-reden `POWERON`).
- **OTA:** `/update` → enkel `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER.ino.bin` (niet `.bootloader`, `.partitions` of `.merged`). Voortgangsbalk, daarna "Controller herstart…" tot hij terug is (via `/ver`), dan naar `/advanced`. Kant-en-klare bestanden staan in `bin/`. Zelf maken: Sketch → Export Compiled Binary.
- **Versie:** `VH_VERSION` in de sketch, zichtbaar in de kop van elke pagina en onderaan `/advanced`.

## 2. Pinnen

| Pin | Functie |
|---|---|
| IO3 | DS18B20 OneWire (T-Bus, meerdere sensoren parallel) |
| IO4 | Pixels data |
| IO5 | PIR MOV1 |
| IO6 | DHT22 (3,3 V) |
| IO1 | LDR1 (analoog) |
| **IO10** | **relais 1: verwarming** (actief-laag, `RELAY_ACTIVE_LOW`) |
| **IO2** | **relais 2: SWW-boiler** (actief-laag) |
| **IO19** | **relais 3: power cycle sensorrail** (NC-contact, onbekrachtigd = gevoed) |
| IO13 / IO11 | I2C SDA / SCL (SCD41, gepland) |
| IO12, IO7, IO18, IO15, IO20 | niet gebruikt (IO15 = strapping-pin) |

Relaismodules: **met optocoupler**, actief-laag, op 5 V. Alle relais zijn fail-safe: bij de opstart eerst op rustniveau gezet en pas dan als uitgang.

## 3. Pagina's en adressen

- **Pagina's:** `/` landingspagina (gebruikers), `/advanced` (technisch, ververst elke 3 s), `/settings`, `/update` (OTA), `/json` (live data), `/ver` (`versie|uptime`).
- **Bediening:** `/toggle_heating_auto`, `/toggle_relay_manual`, `/set_setpoint?value=`, `/toggle_sww_auto`, `/toggle_relay2_manual`, `/set_boiler_setpoint?value=`, `/toggle_pixel?idx=`, `/toggle_pixel_mode`, `/toggle_bed`, `/setcolor?r=&g=&b=`, `/set_fade_duration`, `/set_light_on_min`.
- **Beheer:** `/dht_reset`, `/rescan_ds`, `/save_settings`, `/factory_reset`, `/clear_crash_log`, `/reboot`, `/capabilities`.
- De landingspagina zet bij elke opening beide regelkringen op **Automatisch**.

## 4. Instellingen (`/settings`)

Room-naam · Wi-Fi · vast IP (leeg = DHCP) · mDNS-naam · **Verwarming aanwezig** / **SWW-boiler aanwezig** (uit = relais altijd uit, bediening verborgen) · **HomeWizard P1 IP** (leeg = geen P1) · dauwpuntmarge (−10…+10 °C) en dauwpuntbeveiliging aan/uit · minimum kamer-setpoint (2–25 °C, standaard 5) en boiler-setpoint (3–50 °C, standaard 10) · hysterese CV/SWW · LDR-drempel ("donker onder", standaard 60) · aantal pixels, pixelnamen en Shelly-koppeling (`ip:naam`, max. 3 per pixel) · Google Script-URL en interval (1–60 min, standaard 5) · DS18B20-namen en rollen (kamer, boiler, kelder, ketelpomp, retour).

Opgeslagen in NVS (naamruimte `sjalay-cfg`). Seriële monitor: binnen 5 s na de opstart `R` = NVS wissen.

## 5. Regeling

- **Verwarming (relais 1):** automatisch met `effectief setpoint = max(setpoint, dauwpunt + marge)` (dauwpuntbeveiliging uit: `= setpoint`) en hysterese (standaard 1,0 °C). Geforceerd **uit** bij kamer ≥ 30 °C en zolang de kamertemperatuur niet betrouwbaar is (ook bij de opstart, tot de eerste geldige meting; ook in handmatige modus). Het getal tussen haakjes op de landingspagina is het effectieve setpoint.
- **SWW (relais 2):** setpoint 10–60 °C, hysterese standaard 5,0 °C. Geforceerd uit bij boiler ≥ 60 °C of een defecte boilersensor.
- **Kamertemperatuur:** primaire DS18B20 → terugval DHT22 (enkel als de laatste geldige meting jonger is dan 5 min) → beide weg = relais 1 uit. Geldige meetbereiken vanaf −10 °C (vorst blijft meetbaar). Een sensor geldt als ontbrekend na 3 mislukte lezingen.
- **Verlichting:** pixel 0 = AUTO (aan bij beweging + donker), MANUEEL of bed-modus (altijd uit); pixels 1+ manueel. Elke pixel schakelt 0–3 Shelly's mee (`http://<ip>/relay/0?turn=on|off`). LDR: 0 = donker, 100 = fel.

## 6. Robuustheid en diagnose

- **DHT22:** eigen uitlezing `dhtRead22()` (interrupts hooguit ±5 ms uit). Bij uitval blijven vocht en dauwpunt op de laatste geldige waarde; na 5 fouten opnieuw initialiseren; na 6 een **power cycle via relais 3** (5 s uit, 3 s opwarmen, eerst 3 pogingen minstens 5 min uit elkaar, daarna max. 1 per uur). Tijdens de cycle worden ook DS18B20 en LDR niet gelezen; een DS18B20-waarde van 85,0 °C direct erna wordt genegeerd.
- **Reset-reden met fase:** de laatste fase van de loop (`WIFI`, `PIR`, `PIX`, `SHELLY`, `DHT`, `DS`, `HEAT`, `SWW`, `POST`, `P1`, `OTA`, `SET`, `DHTPW`…) wordt bewaard. Na een watchdog of crash staat die in de reset-reden, bv. `PANIC@POST`. Let op: de fase zegt waar de **loop** was, niet noodzakelijk waar de fout zat (een andere taak kan crashen).
- **`/advanced` → Controller:** IP, mDNS, **DNS**, RSSI, MAC, heap, crash-teller, reset-reden, opstartteller, NVS-bezetting, DS18B20- en POST-fouten, firmwareversie.
- **OTA:** een vastgelopen update wordt bij een nieuwe upload afgebroken; de herstart gebeurt 3 s na "Update OK".
- **Setup-AP "Vakantiehuis-Setup"** (open, `192.168.4.1`): als Wi-Fi niet lukt. Enkel AP (geen achtergrondzoektocht die het AP verstoort); zonder bezoeker na 5 min herstart en opnieuw Wi-Fi proberen.

## 7. Google Sheet en Apps Script

- **Sheet:** `*R-VAKANTIEHUIZEN_Log`, tabblad **"Data"** (rij 1 titel, rij 2 koppen, data vanaf rij 3); oude data in "Archief v1.2". **Eén Sheet en één web-app-URL voor alle huizen.**
- **Web-app-URL** (in `/settings` van elk huis): `https://script.google.com/macros/s/AKfycby95iiPEI0Zd7wKGgo_FXL5UD9LWJ4vu3GO_RVnOFa9mlxIM8lr7NjRh6w0aTfXKiT_kg/exec`
- **Script:** `GoogleScript/VAKANTIEHUIZEN_SHEETS_LOGGER.gs` **v1.3a**. 63 kolommen A–BK per soort data: A–C identiteit · D–M verwarming en binnenklimaat · N–P andere temperaturen · Q–T SWW · U–AP elektriciteit en gas · AQ–AS water · AT–BB licht, pixels, beweging · BC–BK controller en diagnose. Tabel `COLUMNS` in het script; nieuwe kolommen enkel achteraan. `MAX_ROWS` 50 000 voor alle huizen samen.
- **Huisfilter:** keuzelijst in **C1** (ALL of één huis; verbergt rijen, wist niets). `rebuildRoomList()` bouwt de lijst uit kolom C.
- **Vorstalarm per mail, per huis:** kamer < 3 °C, boiler < 4 °C (enkel als SWW aanwezig), kelder < 2 °C; herstel boven alarm + 1,5 °C; max. 1 mail per 6 u; "geen data" na 3 u (uurlijkse trigger). Mail naar Filip via zijn eigen Gmail.
- **Updaten:** code plakken en opslaan → **Implementeren → Implementaties beheren → potlood → Nieuwe versie** (anders draait de oude code) → indien nodig `setupHeaders()`. **Nooit "Nieuwe implementatie"** (nieuwe URL, logging stopt). Bij script én sketch: eerst het script.
- **Tests in de editor:** `test()` (Sjalay-rij), `testYsermonde()` (P1-rij), `testMail()`, `testAlarm()`. `doPost` en `sjalayAlarmCheck` niet rechtstreeks uitvoeren.
- **POST:** HTTPS, timeout 5 s, doorverwijzing niet volgen; 200/302/303 = OK. Af en toe een mislukte POST (±3–5 %) is normaal (Google, 4G); de volgende lukt weer.
- **Grafiektabbladen:** moeten nog naar de nieuwe kolommen (v1.3) omgezet worden.

## 8. `/json`-sleutels

Compact schema, één schema voor de live-pagina's en de Sheet. Booleans `0`/`1`; ontbrekende waarde = `null` = lege cel. **Sleutels niet wijzigen zonder akkoord.**

| Key | Betekenis | Sheet-kolom |
|---|---|---|
| `room` | Room-naam | Kamer |
| `a` | Uptime (s) | Uptime (s) |
| `b` | Ketelvraag (relais 1) | HEAT |
| `c` | Verwarmings-setpoint (schuiver) | *(niet gelogd)* |
| `d` | Kamertemperatuur (effectief) | Temp1 (°C) |
| `e` / `f` / `g` | DHT22-temperatuur / vocht (%) / dauwpunt | Temp2 / Vocht / Dauwpt |
| `h` | Dauwpuntcorrectie actief (setpoint < dauwpunt + marge) | Dew Alert |
| `i` / `j` / `k` | SWW-pomp / boiler-setpoint / boilertemperatuur | SWW pomp / SWW Set / Boiler |
| `l` | LDR (0 = donker, 100 = fel) | Licht LDR |
| `m` / `n` | Nacht / bed-modus | Nacht / Bed |
| `o` `p` `q` | RGB-kleur | R / G / B |
| `r` | Pixels aan (bitstring met `P=`-prefix) | Pixels aan |
| `s` | Pixel-0-modus (0 AUTO, 1 MANUEEL) | Pixel mode |
| `t` | PIR-triggers in het telvenster | MOV1 |
| `u` | Wi-Fi (dBm) | RSSI |
| `v` / `w` | Vrije heap / grootste blok (KB) | Heap vrij / Heap blok |
| `x` | Aantal DS18B20's | DS count |
| `y` | Effectief setpoint | Set (°C) |
| `z` | Reset-reden (met fase, bv. `PANIC@POST`) | Reset |
| `A` | Opstartteller | Boots |
| `B` / `C` | DHT22: fouten op rij / totaal sinds boot | DHT reeks / DHT fouten |
| `D` | Mislukte POSTs sinds boot | POST fouten |
| `E` / `F` / `G` | DS18B20 kelder / ketelpomp / retour | Kelder / Ketelpomp / Retour |
| `H` | CO2 (ppm), SCD41 (gepland v1.16) | CO2 |
| `I` | Bron vocht: 0 DHT22, 1 SCD41, 2 laatste geldige (gepland v1.17) | Vocht-bron |
| `J` / `K` | Verwarming / SWW-boiler aanwezig | Verwarming / SWW-boiler |
| `pa` | P1-status: 0 OK, 1 geen antwoord, 2 niet ingesteld, 3 ongeldige data (waarden `null` als ouder dan 60 s) | P1 status |
| `pb` / `pc` `pd` `pe` | Vermogen totaal / L1–L3 (W, negatief = injectie) | Vermogen / L1–L3 |
| `pf` / `pg` | Afname dag (T1) / nacht (T2) (kWh) | Afname dag / nacht |
| `ph` / `pi` | Injectie dag / nacht (kWh) | Injectie dag / nacht |
| `pj` | Tarief (1 = dag, 2 = nacht) | Tarief |
| `pk` / `pl` | Kwartiervermogen / maandpiek (W) | Kwartier / Maandpiek |
| `pm` `pn` `po` | Spanning L1–L3 (V) | U L1–L3 |
| `pp` `pq` `pr` | Stroom L1–L3 (A) | I L1–L3 |
| `ps` / `pt` | Gasmeterstand (m³) / tijdstip gasmeting | Gas / Gas tijd |
| `pu` | Lange stroomonderbrekingen | Stroomuitval |
| `pv` | Wi-Fi dongle (%) | Dongle Wi-Fi |
| `wa` / `wb` / `wc` | Watermeter: status / stand (m³) / debiet (later) | Water status / Water / Debiet |

## 9. Netwerkregels

- **Elk huis een eigen subnet** (Tailscale routeert per subnet): thuis `192.168.0.x`, Sjalay `192.168.50.x`, Ysermonde `192.168.30.x`. De sketch gaat uit van een /24-netwerk met de router op **.1**.
- **Vaste IP's onder .100**, buiten de DHCP-reeks van de router (Telenet deelt hoge adressen uit).
- **DNS bij een vast IP:** router + **8.8.8.8** (sinds v1.14d). Controle: regel DNS op `/advanced`.
- **Port forwarding** enkel met login (v1.15) en een niet-standaard externe poort.
- Wi-Fi-gegevens, wachtwoorden en publieke IP-adressen **nooit** in deze publieke repo.

## 10. Beveiliging bij port forwarding (gepland v1.15)

| | Op de Wi-Fi van het huis | Van buitenaf |
|---|---|---|
| Landingspagina (lampen, temperatuur) | open | login |
| `/advanced`, `/settings`, `/update`, `/json`, beheer | login | login |

Gebruikersnaam en wachtwoord in `/settings` (leeg = geen login, zoals de Sjalay achter Tailscale). Digest-login. De controller herkent lokale verzoeken aan het afzender-IP. Blokkering na 5 foute pogingen (5 min). Optie: landingspagina ook van buitenaf open (standaard nee).

## 11. Geleerde lessen

- **Adafruit-DHT-bibliotheek** zette de interrupts seconden uit bij een haperende DHT22 → `INT_WDT`-herstarts. Opgelost met een eigen uitlezing (v1.10).
- **Meetgrenzen:** een ondergrens van 5 °C liet de kamersensor bij vorst als "defect" gelden → verwarming geblokkeerd net wanneer nodig. Grenzen nu −10 °C (v1.11).
- **Arduino IDE-prototypes** komen vóór de eerste functie: een `struct` die als parameter dient, moet daarboven staan (v1.14a). Altijd ook lokaal compileren.
- **Setup-AP als AP+STA** viel weg tijdens het invullen (de client sprong van kanaal). Nu enkel AP (v1.14b).
- **SNTP** (`configTzTime`) crashte op het thuisnetwerk (`udp_new` zonder TCPIP-lock). De klok werd niet gebruikt: verwijderd (v1.14c). Ook in de ROOM-sketch in het oog houden.
- **Vast IP:** een adres binnen de DHCP-reeks botste met een ander toestel; en met enkel de router als DNS mislukte elke POST (HTTP -1 na 12 s). Nu DNS-reserve 8.8.8.8 (v1.14d).
- **Crash zoeken:** seriële monitor openen tot "Guru Meditation", backtrace + het `.elf` van dezelfde build aan Claude geven (`addr2line`).
