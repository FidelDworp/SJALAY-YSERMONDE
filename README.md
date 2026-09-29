# Sjalay Controller — vakantiewoning op afstand monitoren en besturen (Recht)

**Locatie:** Sjalay (Recht) — geen permanente WiFi/fiber, gevoed via 4G  
**Basis:** ESP32-C6 + RoomSense shield, eigen webinterface, Google Sheets-logging, Tailscale voor toegang van buitenaf  
**Laatst bijgewerkt:** 29 sep 2026

Dit is geen los "ketel-projectje" meer: de Sjalay-controller doet ondertussen vier dingen tegelijk in het vakantiehuis:

1. **Verwarming op afstand aan/uit** — relais op de E1-ingang van de WOLF-ketel
2. **Sensoren monitoren** — temperatuur, vocht, licht, beweging, met logging naar Google Sheets
3. **Verlichting aansturen** — powerpixels, optioneel gekoppeld aan Shelly-stopcontacten
4. **Overal bereikbaar** — via Tailscale, zonder vaste internet-IP of poort-forwarding

---

## 1. Verwarming & warmwater op afstand

### 1.1 Toestellen

- **Ketel:** WOLF F/CNK/U-25 — SN 168120/1144, bouwjaar 2014, 20–25 kW, Mat.-Nr. 8906850
- **SWW-boiler:** CB-155/FB-155/TB-155, 155 liter, SN 1214
- **Bediening:** BM 2744329 (wandmodule in de living, communiceert via eBUS)

### 1.2 Aansluitpunt: E1-ingang op de ketel (gevonden en bevestigd ✅)

De ketel heeft een parametreerbare, potentiaalvrije ingang **E1** op de klemmenstrook van de ketel zelf (niet op de BM-wandsokkel). De functie wordt bepaald door parameter **HG13** (fachmannebene, instelbereik 1–11).

**HG13 = 1 (Raumthermostat) — bevestigd op de BM (29/09) ✅** Vakmanniveau bereikt via: rechtse ronde knop indrukken → roteren naar "VAKMAN" → bevestigen → code invoeren (**code = 1**, staat in de Montageanleitung) → instellingen "Ketel" (Heizgerät) → HG13. Waarde stond effectief op **1**, dus fabrieksinstelling bevestigd, geen wijziging nodig geweest.

| E1-toestand | Effect |
|---|---|
| **Open** | Verwarming (Heizbetrieb) geblokkeerd — "Sommerbetrieb" |
| **Gesloten** | Verwarming draait normaal, volgens het schema/setpoint van de BM |

Deze functie raakt **uitsluitend de CV-verwarming**. Warmwaterbereiding (SWW) wordt er niet door beïnvloed en blijft dus volledig door de BM zelf beheerd — precies zoals gewenst: verwarming op afstand sturen, SWW aan de BM overlaten.

**Fysiek gevonden en elektrisch bevestigd (29/09):** op de klemmenstrook boven de regelmodule zit een rij van 4 klemblokjes (groen/geel/blauw/wit). Het **gele blokje** droeg enkel een kortsluitbrug en geen enkele sensor — de andere drie (groen/blauw/wit) hebben elk een echte 2-draads sensor erop aangesloten. Meting bevestigde dit definitief:
- **Met brug: 0V** (kortgesloten = "gesloten" = verwarming toegelaten)
- **Zonder brug: 5V** (zwevend/pull-up = "open" = verwarming geblokkeerd)

Dit klopt exact met een potentiaalvrije logica-ingang met interne pull-up naar 5V — de ketel levert zelf de sensorspanning, het relais moet enkel kortsluiten/openen (nooit zelf spanning injecteren). **Het gele blokje is dus bevestigd als E1.**

**Overwogen, niet gekozen:**
- *HG13 = 2 (Maximalthermostat)* — zou bij open contact ook warmwater én vorstbeveiliging blokkeren. Te ingrijpend.
- *Fernschaltkontakt op de BM-wandsokkel* (klemmen 3-4, potentiaalvrij) — functioneel gelijkaardig, maar stuurt altijd CV **en** SWW samen, niet apart regelbaar. Bewaard als alternatief/backup-aansluitpunt indien nodig, makkelijker bereikbaar (living i.p.v. ketelruimte).
- *HG13 = 5–11* — specifieke technische functies (rookgasklep, circulatie, brandersperring, externe brandervraag, retourvoeler), niet geschikt als eenvoudige aan/uit-schakelaar.

**⚡ Bestaande brug over E1 — eerst verwijderen!**

De brug simuleert momenteel een permanent "gesloten contact" (verwarming toegelaten), nodig zolang er geen extern sturingstoestel is aangesloten.

**Belangrijk:** die brug moet **verwijderd** worden vóór je het relais aansluit. Het relais **parallel** naast de bestaande brug zetten werkt niet — een gesloten brug + eender welk relais ernaast blijft altijd "gesloten", en je Sjalay-sturing zou dan genegeerd worden. Knip/verwijder de brug en sluit in de plaats daarvan de twee relaisdraden aan op diezelfde twee E1-klemmen (het gele blokje): het relais neemt dan volledig de rol van de brug over (gesloten = zoals de brug was = verwarming toegelaten; open = geblokkeerd).

**Veiligheid — nooit vergeten:**
- Nooit veiligheidscontacten (STB, maximumthermostaat, druk…) overbruggen
- Galvanische scheiding via relais (optocoupler of degelijk relais) — de ketel levert zelf de 5V, het relais mag geen eigen spanning op de lijn zetten
- Relais **standaard open** (niet-bekrachtigd) gebruiken: bij stroomuitval, ESP-crash of vóór de Wi-Fi-verbinding staat, valt het systeem altijd terug naar de veilige "geblokkeerd"-toestand
- Foto's maken van de volledige klemmenstrook vóór je iets loskoppelt

### 1.3 Bedrijfsstrategie voor een vakantiewoning (belangrijk!)

Uitgangspunt: een dure stookolieketel mag **niet blijven draaien terwijl er niemand is**, behalve voor vorstbeveiliging — maar bij aankomst/vertrek moet er wél volledige controle op afstand zijn, zonder dat iemand fysiek aan de BM moet komen.

**Hoe dat met E1/HG13=1 werkt:**

E1 is de **hoofdschakelaar** die alles overstijgt: zolang E1 **open** staat, blijft de verwarming geblokkeerd, ongeacht wat de BM's eigen klok/schema zegt ("unabhängig von einem digitalen Wolf-Regelungszubehör").

⚠️ **Nog te verifiëren ter plaatse:** de HG13-tabel vermeldt bij optie 1 enkel dat de "Heizbetrieb" geblokkeerd wordt — in tegenstelling tot optie 2, die expliciet óók de vorstbeveiliging blokkeert. Dat suggereert sterk dat de **vorstbeveiliging actief blijft** ook met E1 open — maar bevestig dat expliciet (installateur vragen, of de Montageanleitung van de ketel zelf onder "Frostschutzfunktion" nakijken) vóór je hierop vertrouwt tijdens een koude, onbewoonde periode.

**Praktische regel — zo werkt het in gebruik:**

1. **De BM-module blijft fysiek gewoon staan** op een normale, comfortabele instelling (Automatikbetrieb, gewoon dag/nacht-schema, comfortabele setpoint bv. 20°C). Er moet **nooit iemand aan de BM zelf** komen bij aankomst of vertrek.
2. **Standaard (niemand aanwezig): E1 open** (relais uit) → verwarming geblokkeerd, vorstbeveiliging blijft actief (zie verificatiepunt hierboven).
3. **Vóór aankomst:** relais **sluiten** via de Sjalay-webinterface — verwarming start en volgt vanaf dan gewoon het normale BM-schema/setpoint. Kan al enkele uren op voorhand, zodat het huis warm is bij aankomst.
4. **Tijdens verblijf:** relais gesloten laten — dagelijkse temperatuurregeling gebeurt volledig door de BM zelf, zoals in elk gewoon huis.
5. **Bij vertrek:** relais weer **openen** via de webinterface → terug naar geblokkeerd/vorstbeveiliging-only, zonder dat er iets aan de BM zelf moest gebeuren.

Zo wordt de BM een "domme" thermostaat die altijd hetzelfde comfortschema aanhoudt, en is E1 de enige externe aan/uit-knop — volledige afstandsbediening zonder de BM ooit te moeten herprogrammeren.

---

## 2. Netwerk

| Onderdeel | Instelling |
|---|---|
| 4G-router | TP-Link Archer MR600 (Telenet ONE data-SIM, uit oude iPhone 5) |
| Snelheid | ≈125/25 Mb/s |
| Subnet Sjalay | **192.168.50.0/24** — gateway `.50.1`, DHCP-pool `.100–.199`, vaste IP's `.2–.99` |

*Hernummerd van `192.168.1.0/24` op 28/09 — zie hoofdstuk 3 voor de reden (subnet-conflict met het thuisnetwerk in Zarlardinge, opgelost door Recht een eigen bereik te geven).*

---

## 3. Toegang van buitenaf — Raspberry Pi + Tailscale

Een **Raspberry Pi 3B+** in Recht fungeert als **Tailscale subnet router**: het hele lokale Sjalay-netwerk (niet enkel de Pi) is zo van overal bereikbaar (mobiele data, elders wifi), zonder poort-forwarding of vast internet-IP.

**Setup (28 sep 2026):**
- Raspberry Pi OS 64-bit (Debian 13), geflashed via Raspberry Pi Imager, SSH + wachtwoord-auth ingeschakeld
- Hostname `fidel` → bereikbaar via `fidel.local` (mDNS), onafhankelijk van IP/subnet
- Tailscale: `curl -fsSL https://tailscale.com/install.sh | sh`, dan `sudo tailscale up --advertise-routes=192.168.50.0/24 --accept-dns=false`
- Route goedgekeurd in Tailscale-adminconsole (machine **rpi-fidel-sjalay**)
- IP-forwarding ingeschakeld (`net.ipv4.ip_forward=1`, `net.ipv6.conf.all.forwarding=1`)

**Subnet-conflict opgelost:** het thuisnetwerk in Zarlardinge gebruikte hetzelfde `192.168.1.0/24` als het oorspronkelijke Sjalay-netwerk. Tailscale routeert per subnet/CIDR-blok, dus twee subnet-routers met exact hetzelfde bereik geven een conflict. Opgelost door Recht te hernummeren naar `192.168.50.0/24` (zie hoofdstuk 2) — het thuisnetwerk bleef ongewijzigd.

**Resultaat:** bevestigd werkend — de Sjalay-webinterface is bereikbaar vanaf een iPhone met wifi uitgeschakeld (enkel mobiele data), via de Tailscale-app.

**Thuis (Zarlardinge):** er draait daar al een aparte Tailscale-subnet-router (`rpi-raspberrypi-zarlar`, met Funnel voor een publiek-bereikbare dienst). Omschakeling naar dezelfde subnet-router-aanpak staat gepland — stappen daarvoor in een apart to-do-document ("Tailscale subnet router — Zarlar RPi").

**Nog te doen:**
- [ ] Pi-wachtwoord wijzigen naar iets uniek (`passwd` op de Pi)

---

## 4. Hardware — RoomSense shield pinout (ESP32-C6)

| Pin | Device/Functie | Gebruikt door Sjalay-sketch? |
|-----|-----------------|-------------------------------|
| IO13 | I2C SDA (pull-up 4.7k → 5V) | nee |
| IO11 | I2C SCL (pull-up 4.7k → 5V) | nee |
| IO3 | DS18B20 OneWire | **ja** — temperatuur |
| IO4 | Pixels data | **ja** — powerpixels |
| IO5 | PIR MOV1 | **ja** — beweging |
| IO6 | DHT22 data | **ja** — temp/vocht |
| IO12 | Sharp dust LED out | nee |
| IO7 | Sharp dust analog | nee |
| IO1 | LDR1 analog (10k pull-up → 3V3) | **ja** — licht |
| IO18 | CO2 PWM input (MH-Z19, 5V!) | nee |
| IO19 | MOV2 PIR (of Dotstar CLK) | nee |
| **IO10** | **TSTAT switch (Gnd = ON)** | **ja — relais → WOLF E1** |
| IO2 | LDR2 analog | nee |
| **IO15** | Reserve 2 (Output) | gereserveerd (evt. 2e relais) |
| IO20/WKP | Reserve 3 | nee |
| IO0 | Reserve 1 (BOOT pin) | nee — liever niet gebruiken |
| GND / VIN / 3V3 | Voeding | — |

IO10 is fail-safe-laag (`RELAY_ACTIVE_LOW`) en staat vóór Wi-Fi-verbinding al UIT in `setup()`.

---

## 5. De Sjalay-sketch — huidige implementatie (v0.10, 28 sep 2026)

Bij twijfel is de code in de sketch (`SJALAY_CONTROLLER_v0.10_...ino`) de bron van waarheid; dit is een leeswijzer erbij.

### 5.1 Bestand en board-instellingen

- Arduino IDE: **Board** = ESP32C6 Dev Module, **Flash Size** = 16MB, **Partition Scheme** = Custom (`partitions.csv`), **USB CDC On Boot** = Enabled
- `#define Serial Serial0` bovenaan — verplicht op de C6
- Versienummer in `#define SJALAY_VERSION`, zichtbaar in UI (Controller-groep) en `/json` (`ver`)

### 5.2 Webinterface — pagina's en endpoints

**Pagina's:** `/` (Status), `/update` (OTA), `/settings` (Settings), `/json` (ruwe data).

| Endpoint | Werking |
|---|---|
| `/save_settings` | verwerkt Settings-formulier, herstart |
| `/factory_reset` | wist alle NVS-instellingen, herstart |
| `/clear_crash_log` | wist crash-teller |
| `/rescan_ds` | herscant DS18B20-bus |
| `/toggle_heating_auto` | wisselt Automatisch/Handmatig voor verwarming |
| `/toggle_relay_manual` | wisselt relaisstand (handmatige modus) |
| `/set_setpoint?value=` | setpoint-slider (10-30 °C) |
| `/toggle_pixel_mode` | pixel 0: AUTO ↔ MANUEEL |
| `/toggle_pixel?idx=` | pixel `idx` aan/uit |
| `/setcolor?r=&g=&b=` | powerpixel-kleur |
| `/set_fade_duration?value=` | dim-snelheid (1-10 s) |
| `/set_light_on_min?value=` | licht-aan tijd na PIR (0-30 min) |
| `/toggle_bed` | bed-modus (dwingt pixel 0 uit) |
| `/capabilities` | JSON met pixelnamen |
| `/reboot` | herstart direct |

### 5.3 Settings-velden

- **Algemeen:** room-naam, Wi-Fi SSID/wachtwoord, static IP (leeg = DHCP), dauwpuntmarge, LDR donker-drempel, aantal pixels, Google Script-URL, MAC (read-only)
- **Pixel-namen:** naam + optioneel Shelly-koppeling (`ip:nickname,ip:nickname`, max 3) per pixel
- **Sensoren (DS18B20):** nickname per sensor + keuze primaire sensor
- Los: herscan DS18B20-bus, crash-log wissen, factory reset

### 5.4 Verwarmingslogica

- **Automatisch:** `effective_setpoint = max(setpoint, dauwpunt + dew_margin)`; relais aan als `room_temp < effective_setpoint - 0.5`
- **Handmatig:** relais volgt rechtstreeks de `/toggle_relay_manual`-schakelaar
- Relais wordt **onmiddellijk** aangepast bij elke wijziging
- **Duty-cyclus:** 4-uur sliding window (12 × 20 min), zichtbaar in UI en `/json`/Sheets

### 5.5 Powerpixels — pixel 0 en bed-modus

Volgorde van voorrang:
1. **Bed-modus** aan → pixel 0 altijd uit, ongeacht modus
2. **AUTO/MANUEEL** (enkel relevant als bed-modus uit): AUTO = aan bij beweging + donker genoeg; MANUEEL = losse schakelaar

Pixels 1+ zijn altijd rechtstreeks manueel, persistent, zonder bed-override.

### 5.6 Google Sheets-logging

Actief zodra `gas_url` ingevuld is. Elke 5 minuten HTTPS POST van de volledige `/json`-payload. Laatste resultaatcode (`gcode`) zichtbaar in UI.

### 5.7 Shelly-stopcontacten per pixel

Elke pixel kan gekoppeld worden aan 0-3 Shelly-stopcontacten die simultaan meeschakelen — lokaal, geen cloud. Formaat in Settings: `192.168.50.11:Keukenlamp,192.168.50.12:Tafellamp`. Bij een effectieve aan/uit-wissel van de pixel stuurt de sketch `http://<ip>/relay/0?turn=on|off` (Gen1/2/3-compatibel), fire-and-forget met korte timeout. Geef elke Shelly een vast IP.

**Uitgerold en bevestigd werkend (29/09):** 4 Shelly-stopcontacten gekoppeld en getest, vaste IP's toegekend in `192.168.50.x` (`.11`–`.14`) en gekoppeld via `/settings` (pixel 0 → 3 stopcontacten, pixel 1 → 1 stopcontact). Alles liep in één keer soepel.

**Toekomstidee (niet gepland, nu niet nodig):** de Shelly's zijn Gen2/3-toestellen met Matter-ondersteuning en hebben dus ingebouwde vermogensmeting, opvraagbaar via `http://<ip>/rpc/Switch.GetStatus?id=0` (JSON met `apower` in W, `voltage`, `current`, `aenergy.total` in Wh — lokaal, geen cloud). Zou ooit gebruikt kunnen worden om stroom-/spanningsmeting per pixel te tonen (per Shelly of opgeteld), optioneel gelogd naar Sheets. Blijft voorlopig een idee, geen actieve to-do.

### 5.8 Gekende aandachtspunten

- NVS-instellingen overleven firmware-updates (namespace `"sjalay-cfg"`); enkel factory reset wist alles
- Zonder RoomSense-shield: sketch crasht niet, sensoren tonen gewoon NaN/0 met melding
- Shelly-koppeling los te testen zonder sensoren (serial toont `[Shelly] ... -> HTTP ...`)

### 5.9 Versiegeschiedenis (kort)

| Versie | Inhoud |
|---|---|
| v0.1–v0.1.1 | Platformlaag (Wi-Fi/AP/NTP/OTA/crash-log/factory-reset) |
| v0.2 | Sensoren: DHT22, DS18B20, LDR1, PIR |
| v0.3 | Verwarmingslogica + relais IO10 |
| v0.4 | Powerpixels (fade, AUTO/manueel, bed-modus) |
| v0.5 | Volledige AJAX-live-UI + compact `/json` |
| v0.6 | UI-stijl, heap-KB-bug gefixt |
| v0.7–v0.7.1 | Pixel-0 herwerking, Google Sheets-logging |
| v0.8 | DS18B20-nicknames/primaire sensor, LDR-drempel instelbaar, duty-cyclus 4u |
| v0.9 | Bugfix bed-modus/pixel-0-interactie zichtbaar in UI |
| v0.10 | Optionele Shelly-stopcontacten per pixel |

---

## 6. JSON-velden (`/json`)

Compact schema, gebruikt door live-UI (elke 3s) en Google Sheets-log (elke 5min). Booleans als `true`/`false`.

| Veld | Type | Beschrijving |
|---|---|---|
| `rid` | string | Room-naam |
| `ver` | string | Firmwareversie |
| `ip` | string | IP-adres |
| `rssi` | int | Wi-Fi signaal (dBm) |
| `heap` | uint | Vrije heap (KB) |
| `lb` | uint | Grootste vrije geheugenblok (KB) |
| `crash` | uint | Aantal geregistreerde crashes |
| `upt` | ulong | Uptime (s) |
| `ap` | bool | AP/setup-modus? |
| `t2`/`t2ok` | float/bool | DHT22-temp + geldigheid |
| `h` | float | DHT22-vochtigheid (%) |
| `dp` | float | Dauwpunt (°C) |
| `t1` | float | DS18B20 primaire temp |
| `dsok`/`dsc` | bool/int | DS18B20 gevonden? / aantal |
| `rt` | float | Effectieve room_temp |
| `tm` | string | Sensor-waarschuwing |
| `ldr` | int | Lichtwaarde 0-100 (100=donker) |
| `mov` | int | PIR-triggers laatste minuut |
| `hauto`/`hsp`/`heff` | bool/int/float | Verwarming auto? / setpoint / effectieve setpoint |
| `rman`/`hon` | bool/bool | Handmatige relaisstand / ketelvraag actief |
| `duty` | float | Duty-cyclus verwarming 4u (%) |
| `bed`/`p0m`/`p0on` | bool/int/bool | Bed-modus / pixel-0-modus / pixel-0-staat |
| `pn`/`fd`/`lom` | int | Aantal pixels / fade-snelheid / licht-aan-tijd |
| `pon` | string | Bitstring aan/uit-status per pixel |
| `nr`/`ng`/`nb` | int | Huidige RGB-kleur |
| `gas`/`gcode` | bool/int | Sheets-logging aan? / laatste HTTP-code |

---

## 7. Openstaande punten

- [x] E1 fysiek gelokaliseerd op de ketel (geel klemblokje) + elektrisch bevestigd (0V=gesloten/5V=open)
- [x] HG13 op de BM nagekeken en bevestigd op waarde 1 (vakmancode = 1, uit Montageanleitung; 29/09)
- [ ] **Bestaande brug over E1 verwijderen** vóór het relais aan te sluiten (zie 1.2)
- [ ] **Vorstbeveiliging bij E1-open verifiëren** (installateur of Montageanleitung ketel) — kritiek voor een onbewoonde winterperiode
- [ ] Relais-test met ESP32 (IO10) op de echte E1-klemmen
- [ ] Definitieve montage (behuizing in de kelder)
- [x] Shelly-stopcontacten vast IP toegekend en gekoppeld via `/settings` (4 stuks, 29/09, meteen werkend)
- [ ] Pi-wachtwoord wijzigen (Sjalay-Pi); Zarlar-Pi omschakelen naar subnet-router (apart to-do-document)
- [ ] *(toekomst, niet urgent)* Stroom-/spanningsmeting per pixel via Shelly's tonen in UI — zie 5.7, nog niet nodig
