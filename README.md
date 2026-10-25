# Sjalay Controller — vakantiewoning op afstand monitoren en besturen (Recht)

**Locatie:** Sjalay (Recht) — geen vaste WiFi/fiber, internet via 4G
**Basis:** ESP32-C6 + RoomSense/Zarlar-shield, eigen webinterface, Google Sheets-logging, Tailscale voor toegang van buitenaf
**Sketch:** v1.3 (`SJALAY_CONTROLLER_v1.0.ino`, bestandsnaam is historisch)
**Laatst bijgewerkt:** 24 okt 2026 (avond)

De controller doet vier dingen:

1. **Verwarming (CV) en warm water (SWW) op afstand aan/uit** — relais 1 op de E1-ingang van de WOLF-ketel, relais 2 op de SF-klem
2. **Sensoren monitoren** — temperatuur (kamer, boiler, kelder), vocht, licht, beweging, met logging naar Google Sheets
3. **Verlichting** — powerpixels en 4 Shelly-stopcontacten
4. **Overal bereikbaar** — via Tailscale, lokaal ook via `http://sjalay.local/`

---

## 0. Status (24/10/2026)

**Klaar en werkend in Recht**
- Controller gemonteerd in de kelder naast ketel en boiler; RoomSense-box in de living
- Alle drie temperaturen correct gemeten: kamer (RoomSense), boiler (DS18B20 in het voelerbuisje, samen met de originele WOLF-sensor), kelder (DS18B20 parallel op de T-bus)
- 4 Shelly-lichtgroepen perfect bedienbaar
- Google Sheets-logging actief (zie 6.1)

**24/10: beide relais aangesloten en getest**
- Relais 1 (E1) en relais 2 (SF) zijn aangesloten. UIT-kant getest met de sliders op minimum: beide relais vallen af
- **SWW AAN-kant werkt:** de boiler zakte na een vol bad van 60 naar 23 °C; met setpoint 30 en daarna 40 °C warmde hij op. Na het uitgaan van de brander loopt de temperatuur nog even door (restwarmte). Dat is nuttig voor de hysterese-keuze
- Boiler koelt traag af (52 → 47 °C in 9 u): isolatie is goed, enkel onderaan de boiler ontbreekt wat isolatie
- CV AAN-kant: nog te testen (kamer koelde 24/10 avond af, 19,2 °C)
- Backup van de Pi gemaakt en gecontroleerd (zie 3). Pi-voeding: voorlopig geen aankoop (zie 3)
- **Analyse van de log 23–24/10:** de controller herstart vaak en de DHT22 valt af en toe weg. Beide worden aangepakt in stap A (zie 7)
- Tailscale vanaf mobiele data werkte 23–24/10 niet goed (DERP-relay Amsterdam onbereikbaar op de iPhone, zie 3)

---

## 1. Verwarming & warmwater op afstand

### 1.1 Toestellen

- **Ketel:** WOLF F/CNK/U-25 — SN 168120/1144, bouwjaar 2014, 20–25 kW
- **SWW-boiler:** CB-155/FB-155/TB-155, 155 l, SN 1214
- **Bediening:** BM 2744329 (wandmodule in de living, eBUS)

Vakmanniveau op de BM: rechtse ronde knop indrukken → "VAKMAN" → bevestigen → **code 1** → "Ketel" (Heizgerät).

### 1.2 CV via de E1-ingang (relais 1, IO10)

**E1** is een potentiaalvrije ingang op klemmenstrook **X20** van de ketel (gele klem, naast SF). Parameter **HG13 = 1** (Raumthermostat, fabrieksinstelling, bevestigd 29/09):

| E1 | Effect |
|---|---|
| **Open** | Verwarming geblokkeerd ("Sommerbetrieb"); pomp- en vorstbeveiliging blijven actief |
| **Gesloten** | Verwarming draait volgens schema/setpoint van de BM |

Gemeten: met brug 0 V, zonder brug 5 V (pull-up van de ketel zelf). Het relais mag dus enkel kortsluiten/openen, nooit spanning injecteren. De oorspronkelijke brug is vervangen door relais 1 (aangesloten 24/10).

**Waarom niet HG13 = 2 (Maximalthermostat):** bij open contact blokkeert de brander ook de vorstbeveiliging, voor CV én SWW. Dat draait onze fail-safe om. Alternatief als backup: Fernschaltkontakt op de BM-wandsokkel (klemmen 3-4), maar dat schakelt CV en SWW samen.

### 1.3 SWW via de SF-klem (relais 2, IO2)

**HG24 = 3** (Warmwasser-Fühler-Betriebsart): SF werkt als extern potentiaalvrij contact. Gesloten = SWW-vraag, open = geen vraag; de brander start dan nooit nutteloos. (Fabrieksinstelling was 1 = gewone NTC-sensor op SF.)

SF zit op X20 als blauw klemmenpaar naast E1: `AF (grijs) — SF (blauw) — E1 (geel) — eBUS (groen)`. Op SF hangt enkel **relais 2** (twee potentiaalvrije draden); de originele WOLF-NTC is van de klem losgekoppeld. Hij mag fysiek in het voelerbuisje blijven zitten, naast onze DS18B20. Een NTC parallel aan het relais zou onvoorspelbaar gedrag geven.

Gevolg: de BM toont geen boilertemperatuur meer. De eigen DS18B20 in de controller blijft die wel meten, loggen en tonen. Na een HG24-wijziging moet de ketel uit- en weer ingeschakeld worden.

**Bevestigd 24/10:** HG24 = 3 en de WOLF-sensor is losgekoppeld; SWW werkt in de praktijk (boiler warmde op naar het setpoint).

### 1.4 Gebruik in de praktijk

De BM blijft permanent op een normaal comfortschema staan; niemand moet nog aan de BM komen.

1. Niemand aanwezig: **E1 open** (relais uit) → verwarming geblokkeerd, vorstbeveiliging blijft actief
2. Vóór aankomst: relais sluiten via de webinterface
3. Tijdens verblijf: gesloten laten
4. Bij vertrek: weer openen

### 1.5 Veiligheid

- Nooit veiligheidscontacten (STB, maximumthermostaat, druk) overbruggen
- Relais zijn fail-safe: onbekrachtigd = open = geblokkeerd. Ze staan al UIT vóór de Wi-Fi-verbinding in `setup()` en bij stroomuitval of crash
- Foto's van de klemmenstrook nemen vóór je iets loskoppelt
- Praktische wintertest van de vorstbeveiliging blijft aangeraden

### 1.6 Terugdraaien naar de oorspronkelijke toestand (instructie voor niet-technici)

Handgeschreven instructie van Filip (25/10/2026), in de ketelkast gestoken bij de gele en blauwe klemmen. Overgenomen zoals geschreven.

**INSTRUCTIES: VERWARMING NAAR OUDE TOESTAND**

**1. In de ketelkast**
- Zorg dat de **230 V-voeding van de verwarming UIT** staat: grijs bakje op de muur, **rode knop indrukken**.
- Je ziet in de kast, boven de gekleurde blokjes, een **witte tape** gekleefd met **2 draadjes en een ijzeren brugje**.
- Neem een kleine **platte (−) schroevendraaier**. Schroef de draadjes in het **gele** en het **blauwe** blokje los. Vervang ze:
  - in het **gele blokje**: door het **brugje** (of een kortsluitdraadje);
  - in het **blauwe blokje**: door het **rode en het witte draadje** (de oorspronkelijke voeler). Geen polariteit van belang.
- Schroef alle **4 schroefjes** weer vast.
- Doe het deksel weer dicht en schroef het vast met **2 kruisschroeven (+)**.
- **Zet de stroom van de verwarming weer aan:** groene knop (grijze bakje).

**2. In de living, aan de oude "WOLF"-thermostaat**
- Druk de **rechtse ronde knop** in. Er komt "WEERGAVE".
- Draai de knop **3 kliks naar rechts**: "VAKMAN".
- Druk de knop in: "CODE NR."
- Draai: "0"; **1 klik rechts**: "1".
- Druk in: "INSTALLATIE".
- Draai **1 klik rechts**: "KETEL".
- Klik in: "HG00".
- Draai naar rechts tot "**HG24**" verschijnt ("3").
- Klik in: "3" knippert.
- Draai naar links tot "**1**".
- Klik in: "1" wordt gekozen.

→ **Nu moet alles weer als origineel werken.**

**Aanvulling (voor de volledigheid):** de draden van de controller die in het gele en blauwe blokje zitten, zijn twee UTP-paren: **oranje / wit-oranje (OW)** op het gele E1-blokje (verwarmingsrelais) en **blauw / wit-blauw (BuW)** op het blauwe SF-blokje (boilerrelais). Na het terugplaatsen van het brugje en de voeler mogen die losgekoppelde draden met isolatietape afgeplakt worden; de controller hoeft dan niet meer aan te blijven.

---

## 2. Netwerk

| Onderdeel | Instelling |
|---|---|
| 4G-router | TP-Link Archer MR600 (Telenet ONE data-SIM), `192.168.50.1`, ≈125/25 Mb/s |
| Subnet | **192.168.50.0/24** — DHCP-pool `.100–.199`, vaste IP's `.2–.99` |
| Vaste IP's | Raspberry Pi `.2`; Sjalay-controller `.10`; Shelly-stopcontacten `.11`–`.14` |
| mDNS | `sjalay.local` (instelbaar in Settings) |

Recht werd op 28/09 hernummerd van `192.168.1.0/24` omdat Zarlardinge hetzelfde subnet gebruikte.

---

## 3. Toegang van buitenaf — Raspberry Pi + Tailscale

Een **Raspberry Pi 3B+** in Recht is **Tailscale subnet router** voor `192.168.50.0/24`: het hele Sjalay-netwerk (ook de ESP32-pagina) is van overal bereikbaar zonder poort-forwarding. Werkte vanaf iPhone op mobiele data; op 23–24/10 tijdelijk niet (zie hieronder). Zarlardinge heeft dezelfde opzet voor `192.168.0.0/24` (details in het Zarlar-document).

**Setup:** Raspberry Pi OS 64-bit (Debian 13), hostname `fidel`, daarna
`sudo tailscale up --advertise-routes=192.168.50.0/24 --accept-dns=false`, IP-forwarding aan, route goedgekeurd in de Tailscale-adminconsole (machine `rpi-fidel-sjalay`).

**SSH:** user `pi`, wachtwoord `fidel2026` (gewijzigd 30/09)
- Lokaal IP `192.168.50.2` (vast via NetworkManager: `nmcli con mod "netplan-wlan0-sjalayke" ipv4.addresses 192.168.50.2/24 ipv4.gateway 192.168.50.1 ipv4.dns 192.168.50.1 ipv4.method manual`)
- Tailscale-IP `100.77.122.107`, MagicDNS `rpi-fidel-sjalay.tail3c7f42.ts.net`
- VNC niet actief (headless, enkel SSH)

### Pi: voeding en kaarten (24/10/2026)

- **Voeding:** met de USB-kaartlezer erbij gaf `vcgencmd get_throttled` `0xd0005` (onderspanning). Daarna, zonder lezer, toonde `dmesg` ~15 onderspanningsmeldingen in 22 min (`0x50000`), later `0x80000` (enkel zachte temperatuurlimiet, onschuldig) en geen meldingen meer. **Beslissing 24/10: geen nieuwe voeding voor de Pi 3B+** (2,1 A-adapter volstaat voor enkel Tailscale). Later eventueel een Pi 4/5 met nieuwe voeding. Controle: `vcgencmd get_throttled` (gewenst `0x0` of `0x80000`) en `dmesg | grep -i voltage`. Gebruik geen USB-lezer op de Pi
- **Kaartgroottes:** de systeemkaart van de Pi is **14,5 GB** (`mmcblk0`), de reservekaart **31,3 GB** (33,6 GB decimaal)
- **Reservekaart:** de `dd`-kloon (USB-lezer, tijdens onderspanning) startte niet op in de Pi (rode LED aan, groene uit). Daarna schreef Raspberry Pi Imager het `.gz`-bestand naar de kaart, maar de **verificatie mislukte** (data op de kaart wijkt af, aan ~1 MB/s): de kaart of lezer is onbetrouwbaar. Thuis opnieuw proberen met een ander (merk)kaartje. **Bewaar `sjalay-pi-backup.img.gz`** tot een reservekaart bewezen opstart

### Tailscale vanaf mobiele data werkte 23–24/10 niet

iPhone op 5G toont de waarschuwing "Relay Server Unavailable, `no-derp-connection`, DERP Amsterdam (region 14)". De Pi en de andere toestellen staan groen in de lijst, maar zonder relay of directe verbinding komt er geen verkeer door. Controle in volgorde:
1. Tailscale-statuspagina (status.tailscale.com) op een DERP-incident
2. Op de iPhone: Tailscale uit en aan, vliegtuigmodus aan/uit, Wi-Fi uit
3. iPhone-instellingen: **iCloud Private Relay** en **IP-adres-tracking beperken** voor mobiele data uitzetten (kunnen DERP-verbindingen blokkeren)
4. `tailscale ping 100.77.122.107` vanaf de iPhone/Mac en `tailscale netcheck` op de Pi (bereikt de Pi DERP wel?)
5. Pi-kant: subnet-route `192.168.50.0/24` nog goedgekeurd, IP-forwarding aan, geen key-expiry, Pi niet herstart (voeding)
6. Test vanaf een ander netwerk of op Wi-Fi om telefoon- van providerprobleem te scheiden

### Backup van de Pi naar de Mac (gedaan 24/10/2026)

Niet via 4G doen (beperkt datavolume); wel over het lokale netwerk op de Sjalay-locatie, zonder kaartlezer en tweede kaart.

**Resultaat:** `~/Desktop/sjalay-pi-backup.img.gz` op de Mac, 10,2 GB. Gecontroleerd: uitgepakt exact 15552479232 bytes, gelijk aan de kaart. Het is een kopie van een draaiend systeem, dus een noodkopie. Hij is groter dan verwacht omdat een eerder afgebroken, gecomprimeerd bestand nog in de vrije blokken van de SD-kaart zat en dat comprimeert niet meer.

**Commando's (alle op de Mac, aan de prompt `kampendaal@MacBook-Air-4 ~ %`):**

1. Controleer vrije ruimte (minstens 10 GB) en houd de Mac wakker in een apart venster: `df -h ~` en `caffeinate -d`
2. Backup maken (op één regel, duurt ongeveer 20 minuten, geen voortgang zichtbaar):
```
ssh pi@192.168.50.2 "echo 'fidel2026' | sudo -S dd if=/dev/mmcblk0 bs=4M status=none" | gzip -1 > ~/Desktop/sjalay-pi-backup.img.gz
```
3. Controleren (moet **15552479232** geven; dit lees je ook als integriteitstest):
```
caffeinate -i gunzip -c ~/Desktop/sjalay-pi-backup.img.gz | wc -c
```
4. Terugzetten op een kaart: Raspberry Pi Imager ("Eigen image gebruiken"), of `diskutil list`, dan `diskutil unmountDisk /dev/diskN` en `gunzip -c ~/Desktop/sjalay-pi-backup.img.gz | sudo dd of=/dev/rdiskN bs=4m` (N zorgvuldig controleren op grootte)

**Valkuilen:**
- **Niet** `sudo ssh ...` typen: de wachtwoordvraag is dan die van de Mac en ssh gebruikt de verkeerde gebruiker. Altijd `ssh pi@192.168.50.2`
- Voer het backup-commando **op de Mac** uit, niet in een SSH-sessie op de Pi. Op de Pi schreef het bestand naar de eigen SD-kaart tot die vol zat (`No space left on device`). Opruimen: `rm -f ~/Desktop/sjalay-pi-backup.img.gz` en `df -h /`
- Sluit Terminal niet en laat de Mac niet slapen tijdens de backup
- Het wachtwoord komt in de terminalgeschiedenis van de Mac

---

## 4. Hardware — shield pinout (ESP32-C6)

| Pin | Functie | In Sjalay-sketch? |
|---|---|---|
| IO3 | DS18B20 OneWire (T-bus + verlengdraad, meerdere sensoren parallel) | ja |
| IO4 | Pixels data | ja |
| IO5 | PIR MOV1 | ja |
| IO6 | DHT22 | ja |
| IO1 | LDR1 analoog | ja |
| **IO10** | TSTAT-schakelaar (Gnd = aan) → **relais 1 → E1** | ja |
| **IO2** | (was LDR2) → **relais 2 → SF-klem** | ja |
| IO13 / IO11 | I2C SDA / SCL | nee |
| IO12 / IO7 | Sharp dust LED / analoog | nee |
| IO18 | CO2 PWM (MH-Z19, 5 V) | nee |
| IO19 | MOV2 (of Dotstar CLK) | nee |
| IO15, IO20/WKP | Reserve | nee |
| IO0 | BOOT, liever niet gebruiken | nee |

Beide relais zijn fail-safe-laag (`RELAY_ACTIVE_LOW`).

---

## 5. De sketch (v1.3)

De code is de bron van waarheid; dit is een leeswijzer.

### 5.1 Board-instellingen (Arduino IDE)

**Board** ESP32C6 Dev Module, **Flash** 16 MB, **Partition** Custom (`partitions.csv`), **USB CDC On Boot** Enabled. `#define Serial Serial0` bovenaan is verplicht op de C6. Versie in `#define SJALAY_VERSION` (zichtbaar onderaan `/advanced`).

### 5.2 Pagina's en endpoints

**Pagina's:** `/` (landingspagina voor gasten, zie 5.8), `/advanced` (technische status, zie 5.9), `/settings`, `/update` (OTA), `/json` (live data, zie 6).

| Endpoint | Werking |
|---|---|
| `/toggle_heating_auto`, `/toggle_relay_manual` | verwarming Auto/Handmatig, relais 1 handmatig |
| `/set_setpoint?value=` | verwarmings-setpoint, **5–30 °C** |
| `/toggle_sww_auto`, `/toggle_relay2_manual` | SWW Auto/Handmatig, relais 2 handmatig |
| `/set_boiler_setpoint?value=` | boiler-setpoint, **10–60 °C** |
| `/toggle_pixel?idx=` | pixel aan/uit (pixel 0 schakelt impliciet naar MANUEEL) |
| `/toggle_pixel_mode` | pixel 0: AUTO ↔ MANUEEL |
| `/toggle_bed` | bed-modus (pixel 0 altijd uit) |
| `/setcolor?r=&g=&b=` | gedeelde RGB-kleur |
| `/set_fade_duration?value=`, `/set_light_on_min?value=` | dim-snelheid (1–10 s), licht-aan-tijd na PIR (0–30 min) |
| `/rescan_ds`, `/save_settings`, `/factory_reset`, `/clear_crash_log`, `/reboot`, `/capabilities` | beheer |

### 5.3 Settings

- **Algemeen:** room-naam, Wi-Fi, static IP (leeg = DHCP), mDNS-naam, dauwpuntmarge, hysterese verwarming en SWW, LDR donker-drempel, aantal pixels, Google Script-URL, Sheets-interval (1–60 min, default 5; stuurt ook het PIR-telvenster)
- **Pixel-namen** + optionele Shelly-koppeling per pixel (`ip:nickname,ip:nickname`, max 3)
- **DS18B20:** nickname per sensor, **primaire sensor (kamer)**, **boilersensor** (of "geen")
- **Acties:** herscan DS18B20-bus, opslaan & herstart, crash-log wissen, factory reset

mDNS-naam: enkel a-z, 0-9 en `-` (wordt automatisch opgeschoond, terugval `sjalay`); een nieuwe naam vraagt een herstart.

### 5.4 Verwarmingslogica (relais 1)

- **Auto:** `effective_setpoint = max(setpoint, dauwpunt + marge)` (vochtbescherming), met symmetrische hysterese rond die waarde (default 1,0 °C)
- **Handmatig:** relais volgt de schakelaar
- **Altijd actief (ook in Handmatig):** geforceerd UIT bij kamertemperatuur ≥ 30 °C, en bij onbetrouwbare temperatuurdata

### 5.5 SWW-logica (relais 2)

- Boiler-setpoint 10–60 °C (default 40), hysterese default 5,0 °C
- **Altijd actief:** geforceerd UIT bij boilertemperatuur ≥ 60 °C of een ontbrekende/defecte boilersensor (geen terugval mogelijk)

**Sensoren:** elke DS18B20 heeft een uniek ROM-adres, dus rollen blijven correct bij meer sensoren. Kamer: primaire DS18B20 → terugval DHT22 → beide defect = relais 1 UIT. Een sensor geldt pas als ontbrekend na 3 mislukte lezingen. De kelder-DS18B20 (derde sensor, buiten de behuizing) heeft nog geen rol: hij staat enkel in de lijst op `/advanced`, niet in `/json` of de log (volgt in stap B, zie 7).

### 5.6 Verlichting

Pixel 0: **bed-modus** aan = altijd uit; anders **AUTO** (aan bij beweging + donker) of **MANUEEL**. Pixels 1+ zijn altijd manueel. Elke pixel kan 0–3 **Shelly-stopcontacten** meeschakelen via `http://<ip>/relay/0?turn=on|off` (lokaal, fire-and-forget). Vier stopcontacten (`.11`–`.14`) zijn gekoppeld en werken.

### 5.7 Google Sheets-logging

HTTPS POST van de `/json`-payload op het ingestelde interval, zodra een Google Script-URL ingevuld is. Script en kolommen: zie 6.1.

### 5.8 Landingspagina `/`

Bedoeld voor gasten: geen Auto/Handmatig, weinig tekst.
- **Verlichting:** een tegel per pixel (kleur = ingestelde RGB wanneer aan), kleurkiezer, ronde knoppen voor bed-modus (🛏️) en pixel-0 AUTO/MANUEEL (🔄/✋). Pixel 0 is een vergrendelde maan-tegel tijdens bed-modus
- **Verwarming en SWW:** icoon in een statuscirkel (wit = rust, lichtrood met rode rand = ketelvraag/pomp actief), groot de gemeten temperatuur, kleiner `(→ X°)` = effectieve setpoint (verwarming, via `y`) of setpoint (SWW), plus een setpoint-schuifregelaar
- Bij elke load worden beide regelkringen naar Auto gezet
- Tandwiel ⚙️ naar `/advanced`; 🏠-icoon in de zijbalk van de andere pagina's

### 5.9 Statuspagina `/advanced`

Volgorde: Verwarming → SWW boiler → Alle DS18B20-sensoren → Verlichting → Beweging → Logging → Controller (IP, mDNS, RSSI, MAC, heap, crash-teller, firmware).

Live-refresh elke 3 s via `/json`. Waarden die niet in het compacte schema zitten (crash-teller, heap-details, hysterese, Auto/Handmatig-status, duty-cyclus, Sheets-intervaltekst, per-sensor-detail van sensoren zonder rol, meldingen) zijn enkel correct bij het **laden** van de pagina.

### 5.10 Versiegeschiedenis

| Versie | Inhoud |
|---|---|
| v0.1–v0.11 | Platform (Wi-Fi/AP/NTP/OTA), sensoren, verwarmingslogica, powerpixels, AJAX-UI, Sheets-logging, Shelly-koppeling, SWW-relais met boilersensor en veiligheidslagen |
| v0.12–v0.16 | Landingspagina, mDNS, bed-modus- en pixel-0-knop op de landingspagina |
| v1.0 | Opgeruimde release, statuspagina herschikt, instelbaar Sheets-interval, boiler-bereik 10–60 °C |
| v1.1 | Verwarmings-minimum 5 °C, kortere sectietitels, statuscirkels op de landingspagina |
| v1.2 | `/json` herwerkt naar compact schema met letter-sleutels; nieuw Apps Script |
| v1.3 | Veld `y` (effectieve setpoint) terug in `/json`, enkel voor de live-UI, niet gelogd |

---

## 6. JSON-velden (`/json`)

Compact schema met positionele sleutels (enkel `room` voluit), in de stijl van het Zarlar-roomproject. Eén schema voedt zowel de live-UI als de Sheets-log. Booleans zijn `0`/`1`.

| Key | Betekenis | Sheet-kolom |
|---|---|---|
| `room` | Room-naam | Kamer |
| `a` | Uptime (s) | Uptime (s) |
| `b` | Ketelvraag (0/1) | HEAT |
| `c` | Verwarmings-setpoint (°C) | Set (°C) |
| `d` | Kamertemperatuur (effectief gebruikt, 1 decimaal) | Temp1 (°C) |
| `e` | DHT22-temperatuur (rauw; bij leesfout nu `0` i.p.v. leeg, wordt aangepast) | Temp2 (°C) |
| `f` | Vochtigheid (%) | Vocht (%) |
| `g` | Dauwpunt (°C) | Dauwpt (°C) |
| `h` | Dauwpuntcorrectie actief (0/1) | Dew Alert |
| `i` | SWW-pomp (0/1) | SWW pomp |
| `j` | Boiler-setpoint (°C) | SWW Set (°C) |
| `k` | Boilertemperatuur | Boiler (°C) |
| `l` | LDR (0–100, 100 = donker) | Licht LDR |
| `m` | Nacht (LDR voorbij drempel) | Nacht |
| `n` | Bed-modus (0/1) | Bed |
| `o`/`p`/`q` | RGB-kleur | R / G / B |
| `r` | Pixels aan, bitstring met `"P="`-prefix (bv. `"P=10001000"`; prefix bewaart leidende nullen) | Pixels aan |
| `s` | Pixel-0-modus (0 = AUTO, 1 = MANUEEL) | Pixel mode |
| `t` | PIR-triggers in het telvenster | MOV1 |
| `u` | Wi-Fi (dBm) | RSSI (dBm) |
| `v` / `w` | Vrije heap / grootste blok (KB) | Heap vrij / Heap blok |
| `x` | Aantal DS18B20's | DS count |
| `y` | Effectieve setpoint incl. dauwpuntcorrectie (live-UI) | *(niet gelogd)* |

### 6.1 Google Apps Script (`SJALAY_SHEETS_LOGGER.gs`)

Afgeleid van het Zarlar-roomscript (v1.5). Logt kolommen A–Z: Tijdstempel (door het script toegevoegd) + de velden hierboven, behalve `y` (het script negeert extra velden).

**Actieve Web-app-URL (ingevuld in Settings, werkend sinds 1/10):**
`https://script.google.com/macros/s/AKfycby95iiPEI0Zd7wKGgo_FXL5UD9LWJ4vu3GO_RVnOFa9mlxIM8lr7NjRh6w0aTfXKiT_kg/exec`

**Installatie:** Sheet → Extensies → Apps Script → script plakken (apart project, niet het roomproject) → `setupHeaders` eenmalig uitvoeren → Implementeren als Web-app (uitvoeren als "Ik", toegang "Iedereen") → URL in Settings → `test()` voor een proefrij.

**Update van het script:** altijd Implementaties beheren → potlood → nieuwe versie → Implementeren. Nooit "Nieuwe implementatie": dat geeft een nieuwe URL en dan stopt de logging tot Settings is bijgewerkt.

**Rij-limiet:** `MAX_ROWS = 50000`; daarboven wordt bij elke nieuwe log de oudste rij verwijderd.

---

## 7. Plan en openstaande punten

Werkwijze: telkens één stap, **eerst akkoord van FiDel**, daarna sketch/script/README samen bijwerken.

### 7.1 Bevindingen uit de log van 23–24/10 (98 rijen, interval 5 min)

- **Veel herstarts:** de uptime springt ~10 keer terug naar ~293 s in 9 uur (ongeveer 18:43, 19:59, 20:46, 21:19, 21:24, 23:48, 0:23, 1:27, 2:18, 2:57). Looptijden 5–143 min, onregelmatig, ook 's nachts. Oorzaak nog onbekend
- **DHT22 valt weg:** 22:19–22:39 stond Temp2 5 rijen op `0,0`; de DHT22 levert ook de vochtigheid, dus die ontbreekt dan ook. Na een korte stroomonderbreking werkt hij weer. De DS18B20-bus is dus *niet* het probleem. De sketch zet NaN om naar `0` in `/json`, daarom toont de log `0,0` i.p.v. een lege cel
- **Gaten in de log:** ~6 ontbrekende rijen (mislukte POST of herstart; deels tijdens de aansluitwerken). Verder opvolgen
- **Goed:** RSSI ≈ -45 dBm, heap stabiel (~261 KB vrij, geen lek), boiler koelt traag af
- **Opmaak:** de sketch stuurt Temp1, Temp2, Dauwpunt en Boiler al met 1 decimaal (`%.1f`) en het Apps Script geeft de waarden ongewijzigd door, maar de Sheet toont Temp1, Dauwpunt en Boiler als geheel getal: dat is dus de kolomopmaak in de Sheet (te controleren door een cel aan te klikken: de formulebalk toont de echte waarde). "SWW pomp" staat als `1,0` i.p.v. `1`

### 7.2 Stap A — diagnose en DHT22-robuustheid (sketch v1.4)

1. **Reset-reden** (`esp_reset_reason()`: brownout, watchdog, panic, …) en een **herstartteller** (NVS) op `/advanced`, plus één extra logkolom. Na een dag weten we waarom hij herstart
2. **DHT22-monitoring:** aantal opeenvolgende mislukte lezingen, tijd sinds de laatste geldige lezing en totaal aantal fouten, zichtbaar op `/advanced` en gelogd
3. **DHT22-herstel:** na N mislukte lezingen de sensor opnieuw initialiseren (`dht.begin()`); kijken of de lezing ook de oorzaak van de herstarts is (blokkerende lezing met uitgeschakelde interrupts)
4. **Fallbacks:**
   - Temperatuur: primaire DS18B20 → DHT22 → relais 1 UIT (bestaat al, zie 5.5)
   - Vochtigheid/dauwpunt: laatste geldige waarde met een maximumleeftijd; daarna een veilige terugval. **Nog te beslissen met FiDel** welk gedrag
5. **Geen valse nullen:** ongeldige waarden worden `null` in `/json` en een lege cel in de Sheet
6. **Google-POST** met korte time-out en niet-blokkerend; maximale duur en aantal mislukte POSTs bijhouden en tonen
7. Ook de DS18B20's monitoren (uitvalteller), ook al is de bus nu niet de oorzaak

### 7.3 Stap B — logging uitbreiden (sketch + Apps Script)

- **Alle temperatuurmetingen met 1 decimaal in de Sheet** (Temp1, Temp2, Dauwpunt, Boiler, kelder, ketelpomp, retour): in `setupHeaders()` een getalnotatie `0.0` op die kolommen zetten (Sheet-opmaak; de sketch stuurt al 1 decimaal). Setpoints (Set, SWW Set) blijven hele graden. Na de wijziging controleren door cellen aan te klikken
- "SWW pomp" als **0/1** (oorzaak van `1,0` nagaan; mogelijk eveneens kolomopmaak)
- In het Apps Script `|| 0` vervangen door een controle op `null`, zodat een ongeldige meting een lege cel wordt (anders wordt ze `0`)
- **Vijf DS18B20's in totaal:** 1 RoomSense (kamer), 2 boiler, 3 kelder (net buiten de behuizing), en twee nieuwe: **4 ketelpomp** en **5 retourleiding**. Alle vijf krijgen een rol/naam in Settings (dat kan nu al) en komen in `/json` en de log; het Apps Script krijgt er de kolommen voor (kelder, ketelpomp, retour, achteraan na Z). Sensoren zonder rol blijven zichtbaar op `/advanced`
- Nieuwe kolommen komen **achteraan** (AA, AB, …); A–Z en de bestaande data blijven kloppen. Het Apps Script krijgt de nieuwe kolommen en koppen via **dezelfde implementatie** (nieuwe versie, geen nieuwe implementatie)

### 7.4 Stap C — Pi, Tailscale en reservekaart

- Tailscale: checklist in 3 afwerken tot mobiele toegang weer werkt, resultaat in dit document noteren
- Reservekaart: thuis opnieuw met een ander kaartje, Imager met verificatie, daarna een opstarttest in de Pi
- **Tailscale Funnel (of tunnel) voor familie:** een publieke https-URL naar de Sjalay-controller via de Pi, zoals die al bestaat voor Zarlardinge (opzet daar overnemen, zie het Zarlar-document). Familie krijgt die URL; **huurders krijgen enkel de lokale URL** (`http://sjalay.local/` of `http://192.168.50.10/`). **Gekozen aanpak (KISS, 24/10):** `Tailscale Funnel` op de Pi → **Caddy** op de Pi (poort 8080) → de controller (`192.168.50.10`).
  - Caddy vraagt **één gedeeld familiewachtwoord** (basic auth) en laat enkel de landingspagina `/` en de endpoints die zij gebruikt door (`/json`, `/toggle_*`, `/set_*`, `/setcolor`, `/toggle_pixel*`)
  - Geblokkeerd (404): `/advanced`, `/settings`, `/update`, `/save_settings`, `/factory_reset`, `/reboot`, `/rescan_ds`, `/clear_crash_log`, `/capabilities`
  - Reden: Funnel is publiek en de ESP32 heeft geen login; zo is het één kleine configuratie zonder de sketch aan te passen
  - Huurders: enkel lokale URL. Pas uitwerken nadat de DERP/mobiele-data-problemen opgelost zijn (Funnel heeft hetzelfde nodig). Stappen later: Caddy installeren, `Caddyfile`, `tailscale funnel --bg 8080`, familie testen vanaf mobiele data

### 7.5 Nog te doen op locatie

- [ ] CV AAN-kant testen (kamer koelt af; setpoint boven de kamertemperatuur)
- [ ] Bedrading afwerken met kabelgootjes; isolatie onderaan de boiler bijwerken
- [ ] Hysterese-defaults (1,0 °C CV / 5,0 °C SWW) evalueren, rekening houdend met de restwarmte na het uitgaan van de brander

**Meten volgend bezoek: DHT22 en herstarts (met mini-scoop)**
- [ ] Meetsituatie: DHT22 werkt na een power cycle ~30 s en valt dan weg (24/10, voeding op 5,0 V gezet). Meten terwijl hij **uitgevallen** is en ook tijdens het opstarten/Wi-Fi-verbinding
- [ ] Spanning op de **DHT22-pennen** (VCC–GND, in de RoomSense-box) en op de **3,3 V-pen** van de shield: zakt hij onder ~3,3 V op het moment van uitvallen? (scoop: let op pieken bij Wi-Fi-zending)
- [ ] **Datalijn** (IO6) bekijken: signaal bij een leescyclus, storing bij het schakelen van de relais of bij ketel/pomp
- [ ] **5 V** op de shield, onder belasting (relais aan/uit)
- [x] Genoteerd: DHT22 draait op **3,3 V** (onderkant van zijn bereik 3,3–6 V; op 5 V zetten kan niet, want de datalijn zou dan 5 V krijgen). RoomSense-box en sensor zijn **identiek aan thuis**. Het grote verschil met thuis is de **4-relaismodule** (kanalen 1 en 4 in gebruik), die thuis in de testroom-controller niet aanwezig is
- [ ] Mogelijke oorzaken in volgorde: **4-relaismodule** (spoelstroom en ruis op 5 V/massa), spanningsdip bij Wi-Fi-zending, storing van de ketel op de kabels
- [ ] **Test (kan ook voor vertrek):** VCC van de 4-relaismodule loskoppelen (relais vallen af, CV/SWW tijdelijk uit), controller kort stroomloos zetten, na 2–3 min `f` in `/json` bekijken. DHT22 blijft werken → oorzaak is de relaismodule (oplossing: aparte 5 V-voeding of betere voedingsverdeling). Valt hij weg → relaismodule uitsluiten en verder meten met de scoop. Module daarna weer aansluiten

**Materiaallijst volgend bezoek (extra DS18B20's: ketelpomp en retourleiding)**
- [ ] 2 extra **waterdichte DS18B20-sensoren**
- [ ] Een **verbindingsprintje** om de twee nieuwe sensoren in parallel te schakelen op de T-bus, bij de boilersensor
- [ ] 2 **isolatiestrips** om de sensoren op de buizen (ketelpomp, retourleiding) te monteren
- [ ] Soldeergerief en klein materiaal (krimpkousen, tie-wraps, draad)

### 7.6 Later

- [ ] Chrome op Mac: pagina vernieuwt niet goed zonder manuele refresh (Safari werkt). Vermoeden: timer-throttling in achtergrondtabs; eerst bevestigen wat precies gebeurt
- [ ] Landingspagina testen met een echte gast
- [ ] Wintertest vorstbeveiliging
- [ ] Google-koppeling voor Claude testen (alle logrijen lezen; later Docs/Sheets/Slides bewerken)
- [ ] *(idee)* Shelly Flood (waterlek) en H&T (extra temp/vocht), stroommeting per pixel via Shelly's
- [ ] *(apart traject)* Roomsketch aligneren met de Sjalay-sketch en Matter naar de RPi: zie `OVERNAMEDOCUMENT.md`
