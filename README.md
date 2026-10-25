# Sjalay Controller

Verwarming, warm water, verlichting en sensoren van de vakantiewoning **Sjalay (Recht)**, op afstand en lokaal bedienbaar via een eigen webpagina.

| | |
|---|---|
| **Basis** | ESP32-C6 + RoomSense/Zarlar-shield, eigen webinterface, Google Sheets-logging |
| **Toegang van buitenaf** | Raspberry Pi 3B+ met Tailscale; internet in de Sjalay via 4G |
| **Sketch** | v1.4 (`SJALAY_CONTROLLER_v1.0.ino`, bestandsnaam is historisch) |
| **Laatst bijgewerkt** | 25 oktober 2026 |

**Dit document heeft twee delen**
- **Deel A, voor familie en huurders:** hoe gebruik je de verwarming, het warm water en de lampen, en wat doe je als iets niet werkt. Sectie A1 is bedoeld om door te sturen via WhatsApp.
- **Deel B, voor Filip:** techniek, status, plan en open punten.

---

# Deel A — Gebruik (familie en huurders)

## A1. Kort bericht voor WhatsApp

Kopieer onderstaande tekst en stuur ze door.

```
🏠 Verwarming in de Sjalay

1️⃣ De oude WOLF-thermostaat aan de muur naast de keuken moet ALTIJD AAN blijven staan. Niet uitzetten, niet verdraaien.

2️⃣ Bediening gaat via de app: verbind met het wifi van de Sjalay en open
http://sjalay.local/
(lukt dat niet: http://192.168.50.10/)

3️⃣ 🔥 Verwarming: schuif de balk op de gewenste temperatuur. 20–22 °C is ideaal. Het grote getal is de temperatuur in huis, het getal tussen haakjes is de temperatuur waarop geregeld wordt.

4️⃣ 🚿 Warm water: schuif de balk op de gewenste temperatuur van de boiler. Dit staat los van de verwarming.

5️⃣ 👋 Bij vertrek: zet het warm water minstens een halve dag vooraf op 10 °C. Raak verder niets aan, ook de thermostaat niet.

Werkt iets niet? Kijk of het wifi van de Sjalay aanstaat en of de pagina laadt. Anders: laat het Filip weten.
```

## A2. De thermostaat aan de muur (niet aanraken)

De oude WOLF-thermostaat naast de keuken is het "brein" van de ketel. De verwarming draait alleen als hij aanstaat. Filip heeft hem ingesteld:

- **Linkse draaiknop:** het pijltje links op het display staat naast het **zonnetje**. Op het display staan ook het kraantje (warm water) en het "aan"-symbool.
- **Rechtse draaiknop:** het pijltje rechts staat naast de schaal **+4 … 0 … −4**. Dit is de **maximale kamertemperatuur**: naast **0** is het max. **20 °C**, elke stap naar boven is +1 °C (tot +4 = **24 °C**), elke stap naar onder is −1 °C (tot −4 = **16 °C**). Aanbevolen: **20–22 °C** (tussen 0 en +2).

*Foto van de thermostaat: wordt als bijlage toegevoegd.*

## A3. De app (landingspagina) uitgelegd

Open **http://sjalay.local/** met een telefoon, tablet of laptop op het wifi van de Sjalay. Familie krijgt later een aparte internetlink. De pagina ververst vanzelf.

| Wat je ziet | Wat het doet |
|---|---|
| **Gele balk** bovenaan | Naam van de plek ("Sjalay") |
| **Vierkante tegels met 💡 en een naam** | Eén tegel per lamp of lampgroep. **Tik** om aan of uit te zetten. Aan = tegel krijgt de gekozen kleur, uit = wit. Sommige tegels schakelen ook een stopcontact mee |
| **🌙 gedimde tegel** | De eerste lamp is vergrendeld omdat de bed-modus aanstaat |
| **🛏️ ronde knop** | **Bed-modus** (nachtmodus): de eerste lamp blijft uit. Donkerblauw = aan |
| **🔄 / ✋ ronde knop** | Eerste lamp **automatisch** (🔄: gaat aan bij beweging als het donker is) of **handmatig** (✋, groen: blijft zoals jij het zet). Tik om te wisselen |
| **Kleurcirkel** | Kies de kleur van de lampen |
| **🔥 kaart (verwarming)** | Groot: de **gemeten temperatuur** in huis. Tussen haakjes **(→ 21°)**: de temperatuur waarop geregeld wordt. Die kan iets hoger zijn dan jouw keuze om vochtproblemen te voorkomen (normaal). Daaronder de **schuifbalk** (5–30 °C) om de gewenste temperatuur te kiezen |
| **🚿 kaart (warm water)** | Groot: de **temperatuur van de boiler**. Tussen haakjes het doel. De **schuifbalk** (10–60 °C) kiest de boilertemperatuur, los van de verwarming |
| **Rode cirkel rond 🔥 of 🚿** | De verwarming of de boilerverwarming is **nu actief**. Wit = rust |
| **⚙️ onderaan** | Technische pagina voor Filip. Niet nodig voor gebruikers |

**Goed om te weten**
- De verwarming reageert niet onmiddellijk; het huis warmt geleidelijk op.
- Je hoeft nooit "handmatig" te schakelen: de controller regelt automatisch.
- Zonder stroom op de controller (de kast in de kelder) draait de verwarming **niet**. De ketel blijft dan veilig uit; de vorstbeveiliging blijft werken.

## A4. Als iets niet werkt

1. Staat het **wifi** van de Sjalay aan en ben je ermee verbonden?
2. Staat de **WOLF-thermostaat** nog aan (display brandt)?
3. Laadt de pagina niet: probeer **http://192.168.50.10/**, wacht even en probeer opnieuw.
4. Blijft het mislukken: laat het Filip weten. Noodoplossing: A5.

## A5. Terugdraaien naar de oude toestand (zonder controller)

Handgeschreven instructie van Filip (25/10/2026), in de ketelkast gestoken bij de gele en blauwe klemmen. Gebruik dit enkel als de controller definitief niet meer werkt.

**1. In de ketelkast**
1. Zorg dat de **230 V-voeding van de verwarming UIT** staat: grijs bakje op de muur, **rode knop indrukken**.
2. Je ziet in de kast, boven de gekleurde blokjes, een **witte tape** met **2 draadjes en een ijzeren brugje**.
3. Neem een kleine **platte (−) schroevendraaier** en schroef de draadjes in het **gele** en het **blauwe** blokje los. Vervang ze:
   - **gele blokje:** het **brugje** (of een kortsluitdraadje);
   - **blauwe blokje:** het **rode en het witte draadje** (de oorspronkelijke voeler). Geen polariteit van belang.
4. Schroef alle **4 schroefjes** weer vast en doe het deksel dicht met **2 kruisschroeven (+)**.
5. **Stroom verwarming weer aan:** groene knop (grijze bakje).

**2. In de living, aan de WOLF-thermostaat**
1. Druk de **rechtse ronde knop** in: "WEERGAVE".
2. Draai **3 kliks naar rechts**: "VAKMAN". Indrukken: "CODE NR."
3. Draai naar "0", dan **1 klik rechts**: "1". Indrukken: "INSTALLATIE".
4. Draai **1 klik rechts**: "KETEL". Indrukken: "HG00".
5. Draai naar rechts tot "**HG24**" ("3"). Indrukken: "3" knippert.
6. Draai naar links tot "**1**". Indrukken: "1" wordt gekozen.

→ **Nu werkt alles weer als origineel.**

*Voor Filip:* de controllerdraden in het gele en blauwe blokje zijn twee UTP-paren: **oranje / wit-oranje** op E1 (verwarmingsrelais) en **blauw / wit-blauw** op SF (boilerrelais). Losgekoppelde draden mogen met isolatietape afgeplakt worden.

---

# Deel B — Techniek (Filip)

## B1. Status (25/10/2026)

**Werkt**
- Controller in de kelder naast ketel en boiler; RoomSense-box in de living
- Temperaturen: kamer (RoomSense), boiler (DS18B20 in het voelerbuisje) en kelder (DS18B20 op de T-bus)
- 4 Shelly-lichtgroepen bedienbaar; Google Sheets-logging actief
- Relais 1 (E1) en relais 2 (SF) aangesloten. UIT-kant getest. **SWW AAN-kant werkt**: boiler warmde van 23 °C op naar setpoint 30 en 40 °C; na het uitgaan van de brander loopt de temperatuur nog even door (restwarmte). Boiler koelt traag af (52 → 47 °C in 9 u): isolatie goed, enkel onderaan ontbreekt wat
- Tailscale op mobiele data werkt weer (zie B3)

**Open**
- CV AAN-kant nog te testen
- Controller herstart vaak (~10× in 9 u, 23–24/10) en de DHT22 valt weg (zie B7)
- Voeding van de controller van 4,5 V naar 5,0 V gezet (24/10, resultaat in de log volgen)

## B2. Ketel en relais

**Toestellen:** WOLF F/CNK/U-25 (SN 168120/1144, 2014, 20–25 kW), boiler CB-155/FB-155/TB-155 (155 l), wandmodule BM 2744329 (eBUS, living). Vakmanniveau: rechtse knop indrukken → VAKMAN → code 1 → Ketel.

**CV via E1 (relais 1, IO10).** E1 = potentiaalvrije ingang op klemmenstrook X20 (gele klem), parameter **HG13 = 1**. Open = verwarming geblokkeerd (pomp- en vorstbeveiliging blijven), gesloten = verwarming volgens BM. Het relais mag enkel kortsluiten of openen, nooit spanning injecteren (zonder brug staat er 5 V van de ketel zelf). De oorspronkelijke brug is vervangen door relais 1. **HG13 = 2 is niet gebruikt:** bij open contact zou ook de vorstbeveiliging geblokkeerd zijn.

**SWW via SF (relais 2, IO2).** **HG24 = 3**: SF is een extern potentiaalvrij contact (gesloten = SWW-vraag). Klemmenvolgorde op X20: `AF (grijs) — SF (blauw) — E1 (geel) — eBUS (groen)`. De originele WOLF-NTC is van SF losgekoppeld en mag in het voelerbuisje blijven zitten. Gevolg: de BM toont geen boilertemperatuur meer; de DS18B20 van de controller meet die wel. Na een HG24-wijziging ketel uit en weer aan.

**In gebruik:** de BM staat permanent op een comfortschema en blijft aan (zie A2). De controller regelt via E1 en SF.

**Veiligheid**
- Nooit veiligheidscontacten (STB, maximumthermostaat, druk) overbruggen
- Relais zijn fail-safe: onbekrachtigd = open = geblokkeerd. Ze staan al UIT vóór de Wi-Fi-verbinding in `setup()` en bij stroomuitval of crash
- Foto's van de klemmenstrook nemen vóór je iets loskoppelt; wintertest van de vorstbeveiliging blijft aangeraden

## B3. Netwerk, Raspberry Pi en Tailscale

| Onderdeel | Instelling |
|---|---|
| 4G-router | TP-Link Archer MR600 (Telenet ONE data-SIM), `192.168.50.1` |
| Subnet | `192.168.50.0/24`: DHCP-pool `.100–.199`, vaste IP's `.2–.99` |
| Vaste IP's | Pi `.2`; controller `.10`; Shelly-stopcontacten `.11–.14` |
| mDNS | `sjalay.local` (instelbaar in Settings) |

Recht werd op 28/09 hernummerd van `192.168.1.0/24` omdat Zarlardinge hetzelfde subnet gebruikte.

**Raspberry Pi 3B+** (`fidel`) is Tailscale **subnet router** voor `192.168.50.0/24`: het hele Sjalay-netwerk is van overal bereikbaar zonder poort-forwarding.
- Setup: Raspberry Pi OS 64-bit (Debian 13), `sudo tailscale up --advertise-routes=192.168.50.0/24 --accept-dns=false`, IP-forwarding aan, route goedgekeurd in de adminconsole (`rpi-fidel-sjalay`)
- SSH: `ssh pi@192.168.50.2` (wachtwoord `fidel2026`); Tailscale-IP `100.77.122.107`, MagicDNS `rpi-fidel-sjalay.tail3c7f42.ts.net`; headless, geen VNC
- Voeding: tijdelijke onderspanning (met USB-lezer erbij); nu `get_throttled` = `0x80000` (onschuldig). **Geen nieuwe voeding** voor de 3B+; later eventueel een Pi 4/5. Controle: `vcgencmd get_throttled` en `dmesg | grep -i voltage`. Geen USB-lezer op de Pi gebruiken

**Tailscale-incident 23–24/10 (opgelost).** Op de iPhone op 5G gaf Tailscale "Relay Server Unavailable (DERP Amsterdam)". De Pi, de subnet-route en IP-forwarding waren in orde (bewezen: `tailscale netcheck` en `tailscale ping` op de Pi, `curl` naar de controller vanaf een Mac op een hotspot). Oorzaak: een vastgelopen VPN-configuratie op de iPhone (hersteld door de configuratie te verwijderen en opnieuw toe te voegen). Na een netwerkwissel (hotspot) kan een Mac of iPhone even geen verbinding krijgen: Tailscale afsluiten en opnieuw openen volstaat. Op mobiele data loopt het soms via DERP Frankfurt en is het trager.

**Backup van de Pi (gedaan 24/10).** `~/Desktop/sjalay-pi-backup.img.gz` op de Mac (10,2 GB; uitgepakt exact 15552479232 bytes). Het is een kopie van een draaiend systeem, dus een noodkopie. **Bewaar dit bestand** tot een reservekaart bewezen opstart. De reservekaart (31,3 GB) startte niet op na een `dd`-kloon; de Raspberry Pi Imager-verificatie mislukte ook (kaart of lezer onbetrouwbaar): thuis opnieuw met een ander merkkaartje.

Commando's (alle **op de Mac**, via het lokale netwerk, niet via 4G):
```
ssh pi@192.168.50.2 "echo 'fidel2026' | sudo -S dd if=/dev/mmcblk0 bs=4M status=none" | gzip -1 > ~/Desktop/sjalay-pi-backup.img.gz
caffeinate -i gunzip -c ~/Desktop/sjalay-pi-backup.img.gz | wc -c    # moet 15552479232 geven
```
Terugzetten: Raspberry Pi Imager ("Eigen image gebruiken") of `gunzip -c … | sudo dd of=/dev/rdiskN bs=4m` (N controleren met `diskutil list`).
Valkuilen: niet `sudo ssh` typen; het commando niet in een SSH-sessie op de Pi uitvoeren (vult de Pi-kaart); Terminal niet sluiten en de Mac niet laten slapen tijdens de backup.

## B4. Hardware — shield (ESP32-C6)

| Pin | Functie | In sketch? |
|---|---|---|
| IO3 | DS18B20 OneWire (T-bus, meerdere sensoren parallel) | ja |
| IO4 | Pixels data | ja |
| IO5 | PIR MOV1 | ja |
| IO6 | DHT22 (3,3 V) | ja |
| IO1 | LDR1 analoog | ja |
| **IO10** | **relais 1 → E1** (fail-safe laag, `RELAY_ACTIVE_LOW`) | ja |
| **IO2** | **relais 2 → SF** (fail-safe laag) | ja |
| IO13 / IO11 | I2C SDA / SCL | nee |
| IO12 / IO7 | Sharp dust LED / analoog | nee |
| IO18 | CO2 PWM (MH-Z19, 5 V) | nee |
| IO19 | MOV2 | nee |
| IO15, IO20 | Reserve (IO15 is een strapping-pin) | nee |
| IO0 | BOOT | nee |

De relais zitten op een **4-relaismodule** naast de shield (kanalen 1 en 4 in gebruik); die zit thuis in de testroom-controller niet. **Voeding:** VOLTCRAFT USPS-600 (schakelende, instelbare stekkervoeding, uitgang 3/4,5/5/6/7,5/9/12 V DC, **max. 0,6 A / 7,2 VA**), draaiknop op **5 V** gezet (24/10; was 4,5 V), label "5V-ESP32". Aansluiting met een 2-polige SM-connector (rood = +, zwart = −), met een lange, dunne kabel. Op 5 V is dat maximaal 3 W, wat met ESP32-C6 (Wi-Fi-pieken), twee bekrachtigde relais en sensoren krap is: zie het to-do-punt "zwaardere voeding" in B7.

## B5. De sketch (v1.4)

De code is de bron van waarheid; dit is een leeswijzer.

- **Board:** ESP32C6 Dev Module, Flash 16 MB, Partition Custom (`partitions.csv`), USB CDC On Boot Enabled. `#define Serial Serial0` bovenaan is verplicht op de C6. Versie in `SJALAY_VERSION` (onderaan `/advanced`)
- **Pagina's:** `/` (landingspagina, zie A3), `/advanced` (technisch, live-refresh 3 s), `/settings`, `/update` (OTA), `/json` (live data)
- **Endpoints:** `/toggle_heating_auto`, `/toggle_relay_manual`, `/set_setpoint?value=` (5–30 °C), `/toggle_sww_auto`, `/toggle_relay2_manual`, `/set_boiler_setpoint?value=` (10–60 °C), `/toggle_pixel?idx=`, `/toggle_pixel_mode`, `/toggle_bed`, `/setcolor?r=&g=&b=`, `/set_fade_duration`, `/set_light_on_min`, beheer: `/rescan_ds`, `/save_settings`, `/factory_reset`, `/clear_crash_log`, `/reboot`, `/capabilities`
- **Settings:** room-naam, Wi-Fi, static IP, mDNS-naam (a-z, 0-9, `-`), dauwpuntmarge (−10 t/m +10 °C; negatief = je neemt het vochtrisico zelf), hysterese CV/SWW, LDR-drempel, aantal pixels, Google Script-URL, Sheets-interval (1–60 min, default 5), pixel-namen en Shelly-koppeling (`ip:nickname`, max 3), DS18B20-nicknames, primaire sensor (kamer), boilersensor en de rollen kelder, ketelpomp en retour (alleen meten/loggen)
- **Verwarming (relais 1):** Auto: `effective_setpoint = max(setpoint, dauwpunt + marge)` met symmetrische hysterese (default 1,0 °C). Geforceerd UIT bij kamer ≥ 30 °C en bij onbetrouwbare temperatuurdata
- **SWW (relais 2):** setpoint 10–60 °C (default 40), hysterese default 5,0 °C. Geforceerd UIT bij boiler ≥ 60 °C of bij een defecte boilersensor
- **Sensoren:** elke DS18B20 heeft een uniek ROM-adres. Kamer: primaire DS18B20 → terugval DHT22 → beide defect = relais 1 UIT. Een sensor geldt pas als ontbrekend na 3 mislukte lezingen. Kelder, ketelpomp en retour sturen niets: ze worden enkel getoond en gelogd (max. 6 DS18B20's). **DHT22 (v1.4):** bij uitval blijft de laatste geldige waarde gelden voor vocht en dauwpunt; na 5 mislukte lezingen opnieuw `dht.begin()`; als kamer-terugval enkel als de laatste geldige meting jonger is dan 5 min. `/advanced` toont reset-reden, opstartteller, DHT22-status en foutentellers, DS18B20- en POST-fouten
- **Verlichting:** pixel 0 = bed-modus (altijd uit), AUTO (aan bij beweging + donker) of MANUEEL; pixels 1+ altijd manueel. Elke pixel schakelt 0–3 Shelly-stopcontacten mee (`http://<ip>/relay/0?turn=on|off`)
- **Landingspagina:** bij elke load worden beide regelkringen naar Auto gezet
- **Google Sheets:** HTTPS POST van de `/json`-payload op het ingestelde interval

| Versie | Inhoud |
|---|---|
| v0.1–v0.16 | Platform, sensoren, verwarmings- en SWW-logica, powerpixels, Shelly, landingspagina, mDNS, bed-modus |
| v1.0 | Opgeruimde release, instelbaar Sheets-interval, boiler-bereik 10–60 °C |
| v1.1 | Verwarmings-minimum 5 °C, statuscirkels op de landingspagina |
| v1.2 | `/json` naar compact schema met letter-sleutels; nieuw Apps Script |
| v1.3 | Veld `y` (effectief setpoint) in `/json`, enkel voor de live-UI |
| v1.4 | Diagnose (reset-reden, opstartteller), DHT22-bewaking en -herstel, POST-timeout 5 s en foutenteller, 3 extra DS18B20-rollen, `e`/`f` = `null` bij DHT-uitval, negatieve dauwpuntmarge; vereist Apps Script v1.1 |

## B6. JSON (`/json`) en Google Apps Script

Compact schema (enkel `room` voluit); één schema voedt de live-UI en de Sheets-log. Booleans zijn `0`/`1`.

| Key | Betekenis | Sheet-kolom |
|---|---|---|
| `room` | Room-naam | Kamer |
| `a` | Uptime (s) | Uptime (s) |
| `b` | Ketelvraag | HEAT |
| `c` | Verwarmings-setpoint (°C) | Set (°C) |
| `d` | Kamertemperatuur (effectief, 1 dec.) | Temp1 (°C) |
| `e` | DHT22-temperatuur (rauw, 1 dec.; `null` bij leesfout) | Temp2 (°C) |
| `f` | Vochtigheid (%) | Vocht (%) |
| `g` | Dauwpunt (°C) | Dauwpt (°C) |
| `h` | Dauwpuntcorrectie actief | Dew Alert |
| `i` | SWW-pomp | SWW pomp |
| `j` | Boiler-setpoint (°C) | SWW Set (°C) |
| `k` | Boilertemperatuur | Boiler (°C) |
| `l` | LDR (0–100, 100 = donker) | Licht LDR |
| `m` | Nacht | Nacht |
| `n` | Bed-modus | Bed |
| `o`/`p`/`q` | RGB-kleur | R / G / B |
| `r` | Pixels aan, bitstring met `"P="`-prefix (bewaart leidende nullen) | Pixels aan |
| `s` | Pixel-0-modus (0 = AUTO, 1 = MANUEEL) | Pixel mode |
| `t` | PIR-triggers in het telvenster | MOV1 |
| `u` | Wi-Fi (dBm) | RSSI (dBm) |
| `v` / `w` | Vrije heap / grootste blok (KB) | Heap vrij / Heap blok |
| `x` | Aantal DS18B20's | DS count |
| `y` | Effectief setpoint incl. dauwpuntcorrectie | *(niet gelogd)* |
| `z` | Reset-reden (POWERON, SW, BROWNOUT, …) | Reset |
| `A` | Opstartteller (NVS) | Boots |
| `B` | DHT22: opeenvolgende fouten | DHT reeks |
| `C` | DHT22: totaal fouten sinds boot | DHT fouten |
| `D` | Mislukte Google-POSTs sinds boot | POST fouten |
| `E` / `F` / `G` | DS18B20 kelder / ketelpomp / retour (`null` = niet toegewezen of defect) | Kelder / Ketelpomp / Retour (°C) |

**Apps Script (`SJALAY_SHEETS_LOGGER.gs`)**, afgeleid van het Zarlar-roomscript (v1.5): v1.1 logt kolommen A–AH (Tijdstempel + velden hierboven, zonder `y`; ontbrekende waarden als lege cel; temperaturen met 1 decimaal via kolomnotatie), `MAX_ROWS = 50000` (oudste rij verdwijnt bij overschrijding).
- **Web-app-URL (in Settings, werkend sinds 1/10):** `https://script.google.com/macros/s/AKfycby95iiPEI0Zd7wKGgo_FXL5UD9LWJ4vu3GO_RVnOFa9mlxIM8lr7NjRh6w0aTfXKiT_kg/exec`
- **Installatie:** Sheet → Extensies → Apps Script → script plakken → `setupHeaders` eenmalig uitvoeren → Implementeren als Web-app (uitvoeren als "Ik", toegang "Iedereen") → URL in Settings → `test()` voor een proefrij
- **Update naar v1.1:** nieuwe versie van dezelfde implementatie, daarna `setupHeaders()` eenmalig (voegt kolommen AA–AH toe en zet de notatie). Volgorde: eerst script, dan sketch v1.4
- **Update:** altijd Implementaties beheren → potlood → nieuwe versie → Implementeren. **Nooit** "Nieuwe implementatie" (geeft een nieuwe URL en de logging stopt)

## B7. Plan en openstaande punten

Werkwijze: telkens één stap, **eerst akkoord van Filip**, daarna sketch, script en README samen bijwerken.

**Bevindingen uit de log 23–24/10 (98 rijen, interval 5 min)**
- **Herstarts:** de uptime springt ~10× terug naar ~293 s (onregelmatig, 5–143 min, ook 's nachts). De crash-teller blijft 0, dus geen software-crash; een brownout past daarbij. Voeding gezet van 4,5 V naar 5,0 V
- **DHT22 valt weg:** Temp2 stond 5 rijen op `0,0` (de vochtigheid ontbreekt dan ook). De DS18B20-bus is niet het probleem. Na een power cycle werkt de DHT22 ~30 s en valt daarna weer weg. Opgelost in v1.4 (`null` i.p.v. `0`)
- **Gaten in de log:** ~6 ontbrekende rijen (mislukte POST of herstart)
- **Opmaak:** de sketch stuurt Temp1, Temp2, Dauwpunt en Boiler al met 1 decimaal, maar de Sheet toont Temp1, Dauwpunt en Boiler als geheel getal: kolomopmaak (controle: cel aanklikken, de formulebalk toont de echte waarde). "SWW pomp" staat als `1,0`
- **Goed:** RSSI ≈ −45 dBm, heap stabiel (~261 KB)

**Stap A — diagnose en DHT22-robuustheid (sketch v1.4, geschreven 25/10, nog te testen op locatie)**
1. Reset-reden (`esp_reset_reason()`) en herstartteller (NVS) op `/advanced`, plus één logkolom
2. DHT22-monitoring: opeenvolgende mislukte lezingen, tijd sinds de laatste geldige lezing, totaal aantal fouten (op `/advanced` en gelogd)
3. DHT22-herstel: na N mislukte lezingen opnieuw initialiseren (`dht.begin()`)
4. Fallbacks: temperatuur bestaat al (DS18B20 → DHT22 → relais 1 UIT); voor vocht/dauwpunt de laatste geldige waarde met maximumleeftijd en daarna een veilige terugval (besloten: laatste geldige waarde blijft gelden tot de DHT22 terugkeert; als kamer-terugval maximaal 5 min)
5. Geen valse nullen: ongeldige waarden worden `null` in `/json` en een lege cel in de Sheet
6. Google-POST met korte time-out, niet-blokkerend; maximale duur en mislukte POSTs bijhouden
7. Ook de DS18B20's monitoren (uitvalteller)

**Stap B — logging uitbreiden (sketch v1.4 + Apps Script v1.1, geschreven 25/10, nog te testen)**
- Alle temperatuurkolommen met 1 decimaal (kolomnotatie `0.0` in `setupHeaders()`); setpoints blijven hele graden; "SWW pomp" als 0/1; in het script `|| 0` vervangen door een `null`-controle
- **Vijf DS18B20's:** 1 RoomSense (kamer), 2 boiler, 3 kelder, 4 ketelpomp, 5 retourleiding. Alle vijf krijgen een naam in Settings en komen in `/json` en de log. Nieuwe kolommen (kelder, ketelpomp, retour) komen **achteraan** (AF–AH), via dezelfde implementatie. Rollen kelder, ketelpomp en retour worden pas ingesteld zodra de sensoren fysiek op de bus hangen

**Stap C — Pi, Tailscale en reservekaart**
- Reservekaart thuis opnieuw met een ander merkkaartje (Imager met verificatie), daarna opstarttest in de Pi
- **Tailscale Funnel voor familie** (KISS): `Tailscale Funnel` → **Caddy** op de Pi (poort 8080) → controller `192.168.50.10`. Eén gedeeld familiewachtwoord (basic auth); enkel `/` en de gebruikers-endpoints (`/json`, `/toggle_*`, `/set_*`, `/setcolor`) worden doorgelaten; `/advanced`, `/settings`, `/update`, `/save_settings`, `/factory_reset`, `/reboot`, `/rescan_ds`, `/clear_crash_log`, `/capabilities` worden geblokkeerd (404). Huurders krijgen enkel de lokale URL. Stappen: Caddy installeren, `Caddyfile`, `tailscale funnel --bg 8080`, testen vanaf mobiele data

**Op locatie, volgend bezoek**
- [ ] CV AAN-kant testen (setpoint boven de kamertemperatuur)
- [ ] Bedrading afwerken met kabelgootjes; isolatie onderaan de boiler bijwerken
- [ ] Hysterese-defaults (1,0 °C CV / 5,0 °C SWW) evalueren, met de restwarmte van de brander
- [ ] **Zwaardere voeding meebrengen voor de controller** (vermoedelijke oorzaak of medeoorzaak van de herstarts en het uitvallen van de DHT22): de huidige VOLTCRAFT USPS-600 levert max. 0,6 A en heeft een lange, dunne kabel (spanningsverlies, uitgang kan wegzakken bij belasting).
  - **Vaste, geregelde 5 V** (liefst 5,0–5,1 V, geen instelbare), **minstens 2 A, liefst 3 A**
  - **Korte uitgangskabel** (≤ 0,5 m) met dikke draad (AWG 20–22)
  - **Connector die op de bestaande past:** 2-polige SM-stekker (rood = +, zwart = −). Controleer de polariteit met de multimeter vóór het aansluiten
  - Eerst **stroom meten** (multimeter in serie op de + draad, relais 1 en 4 aan) en de spanning op de shield onder belasting (zie de meetpunten hieronder), zodat de voeding goed gedimensioneerd is
  - Ook meebrengen: een stukje 2-aderige kabel met SM-connector (man en vrouw) voor het geval de bestaande connector vervangen moet worden
- [ ] **Meten met de mini-scoop:** spanning op de DHT22-pennen (VCC–GND) en op de 3,3 V-pen van de shield terwijl de DHT22 uitgevallen is en tijdens het opstarten/Wi-Fi-zenden; datalijn IO6 tijdens een leescyclus (storing van relais of ketel); 5 V op de shield onder belasting
- [ ] **Test (ook voor vertrek):** VCC van de 4-relaismodule loskoppelen (relais vallen af, CV/SWW tijdelijk uit), controller kort stroomloos zetten, na 2–3 min `f` in `/json` bekijken. DHT22 blijft werken → oorzaak is de relaismodule (aparte 5 V-voeding of betere voedingsverdeling). Valt hij weg → relaismodule uitsluiten en verder meten. Module daarna weer aansluiten. Mogelijke oorzaken in volgorde: **onderdimensioneerde voeding** (VOLTCRAFT USPS-600, max. 0,6 A, lange dunne kabel), 4-relaismodule, spanningsdip bij Wi-Fi-zending, storing van de ketel op de kabels
- [ ] **Materiaal (extra DS18B20's):** 2 waterdichte DS18B20, een verbindingsprintje om ze parallel te schakelen bij de boilersensor, 2 isolatiestrips voor op de buizen, soldeergerief en klein materiaal (krimpkousen, tie-wraps, draad)

**Later**
- [ ] Chrome op Mac: pagina vernieuwt niet zonder manuele refresh (Safari werkt); vermoeden timer-throttling in achtergrondtabs
- [ ] Landingspagina testen met een echte gast; wintertest van de vorstbeveiliging
- [ ] Google-koppeling voor Claude activeren (Drive is "niet verbonden"): alle logrijen lezen, later Docs/Sheets/Slides
- [ ] *(idee)* Shelly Flood (waterlek) en H&T (extra temp/vocht), stroommeting per pixel
- [ ] *(apart traject)* Roomsketch aligneren met de Sjalay-sketch en Matter naar de RPi: zie `OVERNAMEDOCUMENT.md`
