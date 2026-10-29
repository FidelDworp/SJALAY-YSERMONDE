# Sjalay Controller

Verwarming, warm water, verlichting en sensoren van de vakantiewoning **Sjalay (Recht)**, op afstand en lokaal bedienbaar via een eigen webpagina.

| | |
|---|---|
| **Basis** | ESP32-C6 + RoomSense/Zarlar-shield, eigen webinterface, Google Sheets-logging |
| **Toegang van buitenaf** | Raspberry Pi 3B+ met Tailscale; internet in de Sjalay via 4G |
| **Sketch** | v1.7 (`SJALAY_CONTROLLER_v1.7.ino`; map en bestand moeten dezelfde naam hebben) |
| **Apps Script** | v1.1 (`SJALAY_SHEETS_LOGGER.gs`) |
| **Laatst bijgewerkt** | 29 oktober 2026 |

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

## B1. Status (28/10/2026)

**Werkt**
- Controller in de kelder naast ketel en boiler; RoomSense-box in de living
- Temperaturen: kamer (RoomSense, Temp1), boiler (DS18B20 in het voelerbuisje) en kelder (DS18B20 op de T-bus, in Settings toegewezen)
- 4 Shelly-lichtgroepen bedienbaar; Google Sheets-logging actief (sketch v1.4 + Apps Script v1.1, sinds 25/10; rijen komen regelmatig binnen, ook de nieuwe kolommen)
- Relais 1 (E1) en relais 2 (SF) aangesloten. UIT-kant getest. **SWW AAN-kant werkt**: boiler warmde van 23 °C op naar setpoint 30 en 40 °C; na het uitgaan van de brander loopt de temperatuur nog even door (restwarmte). Boiler koelt traag af (52 → 47 °C in 9 u): isolatie goed, enkel onderaan ontbreekt wat
- Tailscale op mobiele data werkt weer (zie B3); OTA-update van op afstand is mogelijk via `/update`
- Google Drive is gekoppeld aan Claude (28/10): de volledige log kan rechtstreeks gelezen worden

**Open**
- **DHT22 is sinds 26/10 02:07 dood** (Temp2 en Vocht leeg, dauwpunt 0; DHT reeks 37.404 op 28/10) en komt na een software-herstart niet terug, enkel na een power cycle (zie B7). Zonder vochtmeting is de dauwpuntbeveiliging niet actief; de regeling gebruikt het setpoint
- **Twee keer `INT_WDT`-herstart** (25/10 21:11 en 23:17): een softwarevastloper, geen brownout (zie B7)
- "POST fouten" telt elke POST als mislukt (HTTP 400) terwijl de rijen wel binnenkomen: oorzaak bekend, oplossing in de sketch-todo (v1.5)
- CV AAN-kant nog te testen (huis is nu 12 °C, ketel UIT, setpoint 5 °C, Sjalay staat leeg)
- **Voedingsonderzoek loopt** (29/10): testplan en bevindingen in B7 "Voedingsonderzoek DHT22"
- Voeding van de controller op 5,0 V (VOLTCRAFT USPS-600): of de herstarts echt stopten, wordt in de log gevolgd

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
| **IO19** | **relais 3 → sensor-3V3-rail** (DHT22 + T-Bus + RoomSense; NC-contact, fail-safe: onbekrachtigd = gevoed; was MOV2) | ja (v1.5, rail sinds v1.6) |
| IO15, IO20 | Reserve (IO15 is een strapping-pin) | nee |
| IO0 | BOOT | nee |

De relais zitten op een **4-relaismodule** naast de shield (kanalen 1 en 4 in gebruik); die zit thuis in de testroom-controller niet. **Voeding:** VOLTCRAFT USPS-600 (schakelende, instelbare stekkervoeding, uitgang 3/4,5/5/6/7,5/9/12 V DC, **max. 0,6 A / 7,2 VA**), draaiknop op **5 V** gezet (24/10; was 4,5 V), label "5V-ESP32". Aansluiting met een 2-polige SM-connector (rood = +, zwart = −), met een lange, dunne kabel. Op 5 V is dat maximaal 3 W, wat met ESP32-C6 (Wi-Fi-pieken), twee bekrachtigde relais en sensoren krap is: zie het to-do-punt "zwaardere voeding" in B7.

**PCB-aanpassing voor relais 3 (beslist 28/10, op basis van `ESP32_shield_v2.0_BRD`).** De 3V3-baan loopt van de controllerpin door de PTC (500 mA) langs de bovenkant naar het UART-blokje (3V3-pad), via het 3v3-pinnetje (naast GND) naar de T-Bus (DS18B20's) en naar pin 1 van de RoomSense-connector. **Snede:** de dikke koperbaan op de bovenlaag tussen het UART-3V3-pad en het 3v3-pinnetje. Daarachter hangt enkel de sensorrail (3v3-pinnetje, T-Bus, RoomSense: DHT22, LDR); ervoor blijven controller, I2C-header, decoupling en de 4k7-pull-up van de T-Bus gevoed. Aansluiting: **COM** aan de UART-3V3-kant, **NC** aan het 3v3-pinnetje (onbekrachtigd = rail gevoed, fail-safe). Maak de snede herstelbaar (tweepolige header of draadbrug over de snede) en controleer na het snijden met de multimeter dat het 3v3-pinnetje en RoomSense-pin 1 nergens anders aan 3V3 hangen (de kleine via bij RoomSense-pin 1 is waarschijnlijk een testpunt). Aandachtspunten: (1) de 4k7-pull-up op IO3 blijft aan de gevoede kant, waardoor de DS18B20's tijdens de OFF-fase via hun datapin half gevoed kunnen blijven en niet echt resetten; (2) een DS18B20 geeft na het inschakelen 85,0 °C (v1.6 negeert dat enkel na een power cycle); (3) dit helpt niet als de oorzaak een wegzakkende 5 V of 3V3 van de hele controller is: de metingen in "Op locatie" blijven nodig.

**Over de ELIX-voeding (3/4,5/6/7,5/9/12 V, 800 mA):** niet gebruiken zonder meting. De 4,5 V-stand is te laag (de problemen waren er bij 4,5 V) en 6 V ligt waarschijnlijk boven wat de 5V-ingang van de kaart verdraagt. Zulke draaiknopvoedingen zijn vaak niet gestabiliseerd: bij weinig belasting (de ESP32 trekt ± 0,1–0,2 A) ligt de echte spanning makkelijk 30–50 % boven de ingestelde waarde. Alleen na meting met de multimeter én als er 5 V instelbaar is.

## B5. De sketch (v1.7)

De code is de bron van waarheid; dit is een leeswijzer.

- **Board:** ESP32C6 Dev Module, **Flash Size 16 MB**, Partition Custom (`partitions.csv`), USB CDC On Boot Enabled. `#define Serial Serial0` bovenaan is verplicht op de C6. Versie in `SJALAY_VERSION` (onderaan `/advanced`)
- **OTA-bestand maken (Arduino IDE 2.x):** Sketch → Export Compiled Binary, daarna enkel `SJALAY_CONTROLLER_v….ino.bin` uit de map `build/esp32.esp32.esp32c6/` uploaden via `/update`. **Niet** `.bootloader.bin`, `.partitions.bin` of `.merged.bin`. Staat Flash Size per ongeluk op 4M (zichtbaar in de compile-uitvoer als `FlashSize=4M`), dan kan de OTA mislukken: eerst op 16 MB zetten en opnieuw exporteren. "Not connected" in de IDE bij het exporteren is normaal
- **Pagina's:** `/` (landingspagina, zie A3), `/advanced` (technisch, live-refresh 3 s), `/settings`, `/update` (OTA), `/json` (live data)
- **Endpoints:** `/toggle_heating_auto`, `/toggle_relay_manual`, `/set_setpoint?value=` (5–30 °C), `/toggle_sww_auto`, `/toggle_relay2_manual`, `/set_boiler_setpoint?value=` (10–60 °C), `/toggle_pixel?idx=`, `/toggle_pixel_mode`, `/toggle_bed`, `/setcolor?r=&g=&b=`, `/set_fade_duration`, `/set_light_on_min`, beheer: `/dht_reset` (DHT22 power cycle), `/rescan_ds`, `/save_settings`, `/factory_reset`, `/clear_crash_log`, `/reboot`, `/capabilities`
- **Settings:** room-naam, Wi-Fi, static IP, mDNS-naam (a-z, 0-9, `-`), dauwpuntmarge (−10 t/m +10 °C; negatief = je neemt het vochtrisico zelf), **dauwpuntbeveiliging aan/uit** (v1.5, standaard aan), hysterese CV/SWW, LDR-drempel, aantal pixels, Google Script-URL, Sheets-interval (1–60 min, default 5), pixel-namen en Shelly-koppeling (`ip:nickname`, max 3), DS18B20-nicknames, primaire sensor (kamer), boilersensor en de rollen kelder, ketelpomp en retour (alleen meten/loggen)
- **Verwarming (relais 1):** Auto: `effective_setpoint = max(setpoint, dauwpunt + marge)` (met dauwpuntbeveiliging uit: `= setpoint`) en symmetrische hysterese (default 1,0 °C). Geforceerd UIT bij kamer ≥ 30 °C en bij onbetrouwbare temperatuurdata. Het getal tussen haakjes naast de grote temperatuur op de landingspagina is het effectieve setpoint (`y`). Voorbeeld: setpoint 5 °C, dauwpunt 14,5 °C, marge −5 → effectief 9,5 °C (ondergrens, ook als de schuiver op 5 staat)
- **SWW (relais 2):** setpoint 10–60 °C (default 40), hysterese default 5,0 °C. Geforceerd UIT bij boiler ≥ 60 °C of bij een defecte boilersensor
- **Sensoren:** elke DS18B20 heeft een uniek ROM-adres. Kamer: primaire DS18B20 → terugval DHT22 → beide defect = relais 1 UIT. Een sensor geldt pas als ontbrekend na 3 mislukte lezingen. Kelder, ketelpomp en retour sturen niets: ze worden enkel getoond en gelogd (max. 6 DS18B20's). **DHT22 (v1.4):** bij uitval blijft de laatste geldige waarde gelden voor vocht en dauwpunt (zolang er sinds de start minstens één geldige meting was); na 5 mislukte lezingen opnieuw `dht.begin()`; als kamer-terugval enkel als de laatste geldige meting jonger is dan 5 min. `/advanced` toont reset-reden, opstartteller, DHT22-status en foutentellers, DS18B20- en POST-fouten
- **LDR (v1.7):** schaal 0 = donker, 100 = fel. Donker als `light_ldr < drempel` (Settings: "donker onder deze waarde", standaard 60). De oudere rijen in de Sheet zijn op 28/10 handmatig omgezet naar de nieuwe schaal (hulpkolom `100 − LDR`, plakken als waarden), dus de hele kolom Licht LDR is consistent. Roomcontrollers (Zarlar) hebben nog de oude schaal: zie het to-do-punt in B7
- **Verlichting:** pixel 0 = bed-modus (altijd uit), AUTO (aan bij beweging + donker) of MANUEEL; pixels 1+ altijd manueel. Elke pixel schakelt 0–3 Shelly-stopcontacten mee (`http://<ip>/relay/0?turn=on|off`)
- **Landingspagina:** bij elke load worden beide regelkringen naar Auto gezet
- **Google Sheets:** HTTPS POST van de `/json`-payload op het ingestelde interval, timeout 5 s. Sinds v1.5 wordt de 302-doorverwijzing niet meer gevolgd; HTTP 200, 302 en 303 gelden als geslaagd (`/advanced`: "OK (HTTP nnn)"), alles anders telt als fout
- **Broodkruimel (v1.5):** de sketch bewaart in RTC-geheugen de laatste fase (`IDLE`, `WIFI`, `PIR`, `PIX`, `SHELLY`, `DHT`, `DS`, `HEAT`, `SWW`, `POST`, `CRASH`, `OTA`, `SET`, `DHTPW`). Na een herstart door `INT_WDT`, `TASK_WDT`, `WDT` of `PANIC` staat die in de reset-reden, bv. `INT_WDT@POST` (`/advanced` en kolom Reset). `@IDLE` = het gebeurde buiten de loop (webserver- of Wi-Fi-taak)
- **Power cycle (v1.5, relais 3 op IO19; v1.6: hele sensorrail):** hardware: NC-contact van relais 3 in de sensor-3V3-rail (zie B4). Sinds v1.6 worden tijdens uit en opwarmen ook de DS18B20's (en de LDR) niet gelezen: laatste geldige waarden blijven staan, de DS-faalteller blijft ongemoeid en `room_temp_reliable` blijft true; de eerste DS-lezing na het inschakelen negeert 85,0 °C (alleen dan). Trigger: 6 mislukte lezingen op rij (`dht.begin()` na 5 blijft). Procedure: voeding 5 s uit (datapin IO6 dan als uitgang laag, anders houdt de pull-up de sensor via zijn beschermdiode half gevoed), voeding aan + `dht.begin()`, 3 s opwarmen, controle met 3 lezingen om de 5 s. Mislukt: de eerste 3 pogingen minstens 5 min uit elkaar, daarna max. 1 per uur; bij succes begint de teller opnieuw. Tijdens uit en opwarmen wordt de DHT22 niet gelezen en blijven de tellers staan; vocht en dauwpunt houden de laatste geldige waarde. `/advanced`: aantal power cycles, toestand, tijd sinds de laatste poging en de knop "DHT22 nu resetten" (`/dht_reset`). Niet in de Sheet

| Versie | Inhoud |
|---|---|
| v0.1–v0.16 | Platform, sensoren, verwarmings- en SWW-logica, powerpixels, Shelly, landingspagina, mDNS, bed-modus |
| v1.0 | Opgeruimde release, instelbaar Sheets-interval, boiler-bereik 10–60 °C |
| v1.1 | Verwarmings-minimum 5 °C, statuscirkels op de landingspagina |
| v1.2 | `/json` naar compact schema met letter-sleutels; nieuw Apps Script |
| v1.3 | Veld `y` (effectief setpoint) in `/json`, enkel voor de live-UI |
| v1.4 (25/10) | Diagnose (reset-reden, opstartteller), DHT22-bewaking en -herstel, POST-timeout 5 s en foutenteller, 3 extra DS18B20-rollen, `e`/`f` = `null` bij DHT-uitval, negatieve dauwpuntmarge; vereist Apps Script v1.1. Via OTA uitgerold |
| v1.7 (28/10) | LDR-schaal omgekeerd: 0 = donker, 100 = fel (`/advanced`, JSON `l`, Sheet). Donkertest `light_ldr < drempel` (pixel 0 AUTO en nacht, nu beide strikt kleiner dan). Drempel in Settings = "donker onder deze waarde", standaard 60; nieuwe NVS-sleutel `ldr_dk2`, een oude drempel (`ldr_dark`) wordt omgerekend naar 100 − oud. Apps Script blijft v1.1. Op 28/10 zijn de oudere Sheet-rijen met een hulpkolom (`100 − LDR`, daarna plakken als waarden) omgezet, dus er is geen breuk in de grafiek. Getest: logische lichtwaarden |
| v1.6 (28/10) | Relais 3 schakelt de hele sensor-3V3-rail (PCB-snede, zie B4): tijdens de power cycle worden de DS18B20's en de LDR niet gelezen (laatste waarden blijven, geen faalteller, verwarming niet geblokkeerd); de eerste DS-lezing erna negeert de power-on-waarde 85,0 °C. Verder ongewijzigd |
| v1.5 (28/10) | POST: redirect niet meer volgen (200/302/303 = OK); broodkruimel voor `INT_WDT`/`PANIC`; dauwpuntbeveiliging aan/uit; DHT22 power cycle via relais 3 (IO19). Apps Script blijft v1.1 |

## B6. JSON (`/json`) en Google Apps Script

Compact schema (enkel `room` voluit); één schema voedt de live-UI en de Sheets-log. Booleans zijn `0`/`1`.

| Key | Betekenis | Sheet-kolom |
|---|---|---|
| `room` | Room-naam | Kamer |
| `a` | Uptime (s) | Uptime (s) |
| `b` | Ketelvraag | HEAT |
| `c` | Verwarmings-setpoint (°C), ingesteld met de schuiver | *(niet gelogd)* |
| `d` | Kamertemperatuur (effectief, 1 dec.) | Temp1 (°C) |
| `e` | DHT22-temperatuur (rauw, 1 dec.; `null` bij leesfout) | Temp2 (°C) |
| `f` | Vochtigheid (%) | Vocht (%) |
| `g` | Dauwpunt (°C) | Dauwpt (°C) |
| `h` | Dauwpuntcorrectie actief: `1` als jouw ingestelde setpoint **lager** is dan dauwpunt + marge. De regeling stookt dan naar die hogere waarde (zie `y`). Dit zegt dus niets over de gemeten kamertemperatuur | Dew Alert |
| `i` | SWW-pomp | SWW pomp |
| `j` | Boiler-setpoint (°C) | SWW Set (°C) |
| `k` | Boilertemperatuur | Boiler (°C) |
| `l` | LDR (0–100, **0 = donker, 100 = fel** sinds v1.7; daarvoor 100 = donker) | Licht LDR |
| `m` | Nacht | Nacht |
| `n` | Bed-modus | Bed |
| `o`/`p`/`q` | RGB-kleur | R / G / B |
| `r` | Pixels aan, bitstring met `"P="`-prefix (bewaart leidende nullen) | Pixels aan |
| `s` | Pixel-0-modus (0 = AUTO, 1 = MANUEEL) | Pixel mode |
| `t` | PIR-triggers in het telvenster | MOV1 |
| `u` | Wi-Fi (dBm) | RSSI (dBm) |
| `v` / `w` | Vrije heap / grootste blok (KB) | Heap vrij / Heap blok |
| `x` | Aantal DS18B20's | DS count |
| `y` | Effectief setpoint incl. dauwpuntcorrectie (1 dec.) | Set (°C) |
| `z` | Reset-reden (POWERON, SW, INT_WDT, BROWNOUT, …); sinds v1.5 bij watchdog/panic met fase, bv. `INT_WDT@POST` | Reset |
| `A` | Opstartteller (NVS) | Boots |
| `B` | DHT22: opeenvolgende fouten | DHT reeks |
| `C` | DHT22: totaal fouten sinds boot | DHT fouten |
| `D` | Mislukte Google-POSTs sinds boot (telt nu elke POST, zie sketch-todo) | POST fouten |
| `E` / `F` / `G` | DS18B20 kelder / ketelpomp / retour (`null` = niet toegewezen of defect) | Kelder / Ketelpomp / Retour (°C) |

**Apps Script (`SJALAY_SHEETS_LOGGER.gs`)**, afgeleid van het Zarlar-roomscript (v1.5): v1.1 logt kolommen A–AH (Tijdstempel + velden hierboven, met `y` als Set en zonder `c`; ontbrekende waarden als lege cel; temperaturen met 1 decimaal en kolom A met notatie `yyyy-mm-dd hh:mm:ss` via kolomnotatie), `MAX_ROWS = 50000` (oudste rij verdwijnt bij overschrijding).
- **Sheet:** `*R-SJALAY_Log`, tabblad "Data" (rij 1 titel, rij 2 koppen, data vanaf rij 3)
- **Web-app-URL (in Settings, werkend sinds 1/10):** `https://script.google.com/macros/s/AKfycby95iiPEI0Zd7wKGgo_FXL5UD9LWJ4vu3GO_RVnOFa9mlxIM8lr7NjRh6w0aTfXKiT_kg/exec`
- **Installatie:** Sheet → Extensies → Apps Script → script plakken → `setupHeaders` eenmalig uitvoeren → Implementeren als Web-app (uitvoeren als "Ik", toegang "Iedereen") → URL in Settings → `test()` voor een proefrij. `doPost` zelf niet uitvoeren in de editor (geeft `postData` van `undefined`: dat is normaal)
- **Update (volgorde):** code plakken en opslaan → **Implementeren → Implementaties beheren → potlood → Nieuwe versie → Implementeren** (anders blijft de web-app de oude code draaien!) → `setupHeaders()` eenmalig (voegt koppen toe en zet de notatie ook op bestaande rijen). **Nooit** "Nieuwe implementatie" (geeft een nieuwe URL en de logging stopt)
- Bij een update van script én sketch: eerst het script, dan de sketch

## B7. Plan en openstaande punten

Werkwijze: telkens één stap, **eerst akkoord van Filip**, daarna sketch, script en README samen bijwerken.

**Bevindingen uit de log 23–28/10 (1323 rijen, interval 5 min)**
- **Herstarts (23–25/10):** ± 31 herstarts in 26 uur (elke 30–90 min, ook 's nachts); de crash-teller bleef 0. Na het zetten van de voeding op 5 V (24/10) en de aanpassingen op locatie liep de controller op 25/10 10,7 uur ononderbroken (02:12 → 11:56)
- **`INT_WDT` (nieuw in v1.4):** de reset-reden toont twee keer `INT_WDT` (25/10 21:11 en 23:17). Dat is de interrupt-watchdog: de processor zat te lang vast met onderbroken interrupts. Dat is een **softwarevastloper**, geen brownout. De oorzaak is nog niet bekend. Alle andere herstarts sinds de OTA waren `SW` (zelf gedaan: OTA, Settings, remote reboot)
- **DHT22 dood sinds 26/10 02:07:** tot dan werkte hij sinds 25/10 12:00 stabiel (daarvoor: 214 rijen met `0,0`). Sinds 26/10 02:07 komt er geen enkele geldige meting meer, ook niet na de `SW`-herstart van 26/10 07:25: DHT reeks 37.404 op 28/10. Een softwareherstart of `dht.begin()` herstelt hem niet; enkel een power cycle (zoals eerder vastgesteld). Gevolg: Temp2/Vocht leeg, dauwpunt 0, Dew Alert 0, de dauwpuntbeveiliging is niet actief (de regeling gebruikt het setpoint). Temp1 (DS18B20) werkt gewoon
- **Sensorvervanging is niet gewenst:** alle room-sensors en Photon-controllers gebruiken een DHT22 en werken wel goed. De oorzaak is dus specifiek aan deze opstelling en moet worden gevonden en bevestigd. Hypothese: de DHT22 (min. 3,3 V) hangt aan de 3,3 V-lijn van de ESP32-kaart, die bij Wi-Fi-pieken of een zwakke 5 V-voeding kan wegzakken, waarna de sensor zich vastzet. Te bevestigen met de mini-scoop (zie "Op locatie")
- **POST-fouten:** de teller loopt bij elke POST met 1 op (624 op 28/10) en `/advanced` toont `HTTP 400`, terwijl de rijen wel in de Sheet staan. Oorzaak: de sketch volgt de 302-doorverwijzing van Apps Script en stuurt een tweede verzoek dat met 400 faalt (zie sketch-todo)
- **Gaten in de log:** af en toe 1–2 ontbrekende rijen (herstart of mislukte POST), o.a. 27/10 05:01, 07:56 en 08:31
- **Huidige toestand (28/10):** huis 12,4 °C, kelder 11,9 °C, boiler 17,8 °C, HEAT en SWW UIT sinds 26/10 (setpoint 5 °C, SWW-setpoint 10 °C), RSSI ≈ −50 dBm, heap stabiel (~260 KB)

**Stap A — diagnose en DHT22-robuustheid (sketch v1.4, uitgerold 25/10)**
1. Reset-reden (`esp_reset_reason()`) en herstartteller (NVS) op `/advanced`, plus één logkolom ✔ (levert o.a. de `INT_WDT`-vondst)
2. DHT22-monitoring: opeenvolgende mislukte lezingen, tijd sinds de laatste geldige lezing, totaal aantal fouten (op `/advanced` en gelogd) ✔
3. DHT22-herstel: na 5 mislukte lezingen opnieuw initialiseren (`dht.begin()`) ✔ (blijkt niet voldoende: de sensor herstelt enkel met een power cycle)
4. Fallbacks: temperatuur bestaat al (DS18B20 → DHT22 → relais 1 UIT); voor vocht/dauwpunt blijft de laatste geldige waarde gelden tot de DHT22 terugkeert; als kamer-terugval maximaal 5 min ✔
5. Geen valse nullen: ongeldige waarden worden `null` in `/json` en een lege cel in de Sheet ✔
6. Google-POST met korte time-out (5 s) en teller ✔ (de teller telt nu elke POST, zie v1.5)
7. Ook de DS18B20's monitoren (uitvalteller) ✔

**Stap B — logging uitbreiden (sketch v1.4 + Apps Script v1.1, uitgerold 25/10)**
- Alle temperatuurkolommen met 1 decimaal; kolom Set toont het effectieve setpoint; tijdstempel met notatie `yyyy-mm-dd hh:mm:ss` ✔
- **Vijf DS18B20's:** 1 RoomSense (kamer), 2 boiler, 3 kelder, 4 ketelpomp, 5 retourleiding. Kelder is toegewezen (15,5 °C → 11,9 °C). Ketelpomp en retour blijven leeg tot de sensoren fysiek op de bus hangen (zie materiaal bij "Op locatie")

**Sketch v1.7 (LDR-schaal) en v1.6 (sensorrail) — geschreven 28/10, nog te testen (OTA van thuis)**
v1.5 draait en is getest (Firmware v1.5, "Laatste resultaat" OK (HTTP 302), POST fouten 0, Settings-veld aanwezig). v1.6 is de aanpassing voor de gedeelde sensorrail; hij heeft pas zin zodra de PCB-snede en relais 3 bedraad zijn, maar is veilig ook zonder (relais 3 is dan niet aangesloten). Installatie: `SJALAY_CONTROLLER_v1.7.ino` in de map `SJALAY_CONTROLLER_v1.7`, compileren met Flash Size 16 MB en Partition Custom, `.ino.bin` exporteren en uploaden via `/update`. Daarna controleren: Firmware v1.7, Licht in `/advanced` met "donker=0" (hand voor de LDR → waarde daalt), drempel in Settings = 60 (was 40), nachtregeling gedraagt zich als voorheen, "Laatste resultaat" = OK (HTTP 302), POST fouten blijft 0, relais 3 bedraad voor de DHT22-reset. Hardware: relais 3 (IO19) op de 4-relaismodule bedraden, NC-contact in de 3,3 V-lijn van de DHT22. De punten hieronder zijn uitgevoerd, behalve wat "nog te beslissen" is.
- [x] **Google-POST: redirect niet meer volgen.** Aanleiding (25/10): "POST fouten" loopt bij elke POST met 1 op en `/advanced` toont "Laatste resultaat: HTTP 400", terwijl alle rijen wel in de Sheet staan. Oorzaak: Apps Script verwerkt de POST en antwoordt met een doorverwijzing (302). De sketch volgt die (`HTTPC_STRICT_FOLLOW_REDIRECTS`) en stuurt een tweede verzoek naar de doorverwijs-URL, dat met 400 wordt afgewezen. De gegevens zijn dan al gelogd
  - In `postToGoogleSheets()`: `http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS)`
  - Succes = HTTP 200 of 302; alle andere codes (time-out `-11`, 4xx, 5xx) blijven een fout en tellen mee in `sheets_fail_total` (JSON `D`)
  - `/advanced` "Laatste resultaat": 200 en 302 tonen als "OK (HTTP nnn)"; andere codes als "HTTP nnn"
  - POST-timeout blijft 5 s; nu ook minder blokkeertijd, want het tweede verzoek vervalt
  - Test: na de update moet "POST fouten" op 0 blijven en "Laatste resultaat" "OK" tonen
- [x] **`INT_WDT`-broodkruimel.** Aanleiding (28/10): twee `INT_WDT`-herstarts zonder zichtbare oorzaak. Voorstel: de sketch bewaart in het RTC-geheugen (`RTC_NOINIT_ATTR`) welke functie als laatste draaide (bv. sensoren lezen, DHT, DS18B20, POST, OTA, Settings opslaan, webverzoek). Na een `INT_WDT`-herstart staat die tag in de log (extra kolom of in de Reset-kolom), zodat we zien waar het vastliep
- [x] **Aan/uit-schakelaar voor de dauwpuntbeveiliging** in Settings. Bij uit: `effective_setpoint = setpoint`. Aanleiding: ook met marge −5 °C blijft de ondergrens ≈ 9,5 °C bij dauwpunt 14,5 °C; de marge-ondergrens van −10 °C helpt enkel bij een dauwpunt tot ≈ 15 °C
- [x] DHT22 van een schakelbare voeding voorzien: relais 3 op IO19 (zie B5); v1.6: schakelt de hele sensorrail, DS-waarden blijven tijdens de cycle staan. Niet in de Sheet. Dit is een noodoplossing en geen oorzaak: het meetplan met de mini-scoop blijft nodig
- Bij elke sketchwijziging: kopregel, versietabel (B5), `SJALAY_VERSION`, README en OTA-upload via `/update`

**Roomsketch aanpassen aan de nieuwe LDR-schaal (todo, nog niet gedaan)**
Doel: dezelfde schaal in alle controllers: **0 = donker, 100 = fel**, zodat Sheet, grafieken en drempels overal hetzelfde betekenen. De roomsketch (en de Photon-code) moet eerst bij Claude aangeleverd worden; zonder code geen wijziging. Wat er per sketch aangepast moet worden (zoals in de Sjalay-sketch v1.7):
- [ ] **Schaalfunctie:** de LDR-omzetting (bij de Sjalay `scaleLDR()`, `map(constrain(raw,100,3800),100,3800, 100,0)`) omkeren naar `…, 0,100)`. Controleer de ruwe ADC-grenzen van de roomsketch (kunnen afwijken)
- [ ] **Alle vergelijkingen met de donkerdrempel omkeren:** `licht > drempel` / `>= drempel` (donker) wordt `licht < drempel`; maak ze gelijk (strikt kleiner dan) om verschillen tussen aan- en uitschakelen te vermijden. Zoek alle plaatsen waar de LDR-waarde gebruikt wordt: automatische verlichting/pixels, nachtvlag, eventuele dimlogica
- [ ] **Opgeslagen drempel omrekenen:** de bestaande NVS-drempel (oude schaal, bv. 40) eenmalig omzetten naar `100 − oud` via een nieuwe NVS-sleutel (bij de Sjalay `ldr_dk2`; als die ontbreekt en de oude sleutel bestaat: `100 − oud`, anders standaard 60). Zonder deze stap slaat de nachtregeling na de update om
- [ ] **UI en teksten:** label "(donker=100)" wordt "(donker=0)", Settings-tekst "donker onder deze waarde", standaarddrempel 60
- [ ] **JSON en Sheet:** de waarde in `/json` (sleutel `l` bij de Sjalay) volgt de nieuwe schaal. Een apart Apps Script is niet nodig, wel de oude Sheet-rijen omzetten (hulpkolom `100 − LDR` en plakken als waarden, zoals op 28/10 in de Sjalay-Sheet)
- [ ] **Versiekop, changelog en README** van die sketch bijwerken, OTA-test: hand voor de LDR → waarde daalt bij donker; nachtregeling gedraagt zich als voorheen
- [ ] Ook andere plaatsen waar de oude schaal staat nalopen (Zarlar-documentatie, Matter/HomeKit-koppeling in `OVERNAMEDOCUMENT.md`: lichtsensor 0 = donker kan een ander eenheidsmodel vragen)

**Voedingsonderzoek DHT22 (29/10) — wat we leerden en wat te testen**

*Wat we vaststelden (feiten)*
- **PCB `ESP32_shield_v2.0`:** er staan **twee 500 mA-PTC's**: één in de 3V3-baan (linksboven, naast de controller) en één in de 5V-baan, net na de optionele buck-module (VCC_5V). De OPTION-connector (+5 V en de relaissignalen) hangt **achter de 5V-PTC**: in de **Sjalay** én op de **HVAC-controller** loopt de relaisstroom dus door de PTC. De optionele buck-module (6–23 V → 5 V) wordt alleen geplaatst bij een 12 V-voeding; dan wordt de + lijn eronder doorgesneden. Bij 5 V op de SM-connector: geen module, baan intact.
- **Testroom (R-EETPL):** dezelfde shield, geen buck-module, baan intact, geen relaismodule, **5 V via de USB-C van de devkit** (Apple A1400, Salcomp, 5 V / 1 A). Deze weg omzeilt de SM-connector en de 5V-PTC. Log 1/5 – 29/10 (5014 rijen): DHT22 geldig op 5013 rijen (één slechte rij op 11/6), maar **94 herstarts** (mediane uptime 1,2 dag, max 7,9 dagen) en de minimale vrije heap staat soms op 0. De DHT22 overleeft dus herstarts en valt niet spontaan uit op deze opstelling.
- **HVAC-controller (testopstelling thuis):** 10 relais via I2C (8+2 modules) op een Velleman PSSE510 (5 V / 1 A, geschakeld), relaisvoeding achter de 5V-PTC, **geen** RoomSense/DHT22.
- **Sjalay-controller:** relaissignalen en +5 V zitten op de OPTION-pinnen (achter de 5V-PTC); 5 V op de SM-connector via de USPS-600 (lange dunne kabel).
- **De uitval van 26/10 02:07 gebeurde zonder relaisactiviteit en zonder herstart** (HEAT = 0 en SWW-pomp = 0 sinds minstens 25/10 23:00, uptime liep door). Een dip door schakelende relaisspoelen kan deze uitval dus **niet** verklaren.

*Hypothesen (nog niet bewezen)*
- De ESP32-C6 werkt tot ± 3,0 V op de 3,3 V-lijn; de DHT22 heeft minstens 3,3 V nodig. Zakt de 5 V onder ± 4,3–4,4 V (spanningsval over de lange dunne kabel + SM-connector + 5V-PTC, Wi-Fi-pieken ± 250–350 mA, relais, pixels), dan zakt de 3,3 V van de (LDO-)regelaar van de devkit onder 3,3 V en kan de DHT22 vastlopen terwijl de ESP gewoon doorloopt (geen reset, past bij het beeld).
- Een 500 mA-PTC is een hold-stroom en geen harde limiet: warm (ketelruimte) daalt die en stijgt de weerstand. Dit verklaart mogelijk het verschil met de testroom, waar USB-C de PTC omzeilt.
- Nog open: de DHT22 of zijn kabel zelf, storing van ketel of relais op de kabels, de Sjalay-sketch (leescode, 750 ms DS-lezing).

*Uitkomst per test*
- Stabiel op de testroom via de SM-connector → het voedingspad is niet de oorzaak; zoeken bij relaismodule, ketelstoring, kabel naar de box, sketch.
- Valt uit via de SM-connector → het pad (SM, PTC, VCC_5V) of de voeding is de oorzaak; dan USPS-600 + lange kabel (test 2) om pad en voeding te scheiden.

*Testplan thuis (Zarlardinge, nu)* — telkens één verandering, minstens 3–5 dagen, noteer het tijdstip en de `POWERON`-reset. Zelfde sketch, zelfde kabel.
1. **Testroom omgekeerd:** USB-C los, alleen de A1400 (korte, dikke kabel, polariteit meten, rood = +) op de **SM-connector**. Nooit twee voedingen tegelijk. Meet meteen: 5 V aan de 5V-pin van de devkit, spanning **vóór en ná de 5V-PTC**, 3,3 V aan de DHT-pin (scoop single-shot, trigger onder ± 3,2 V).
2. **Zelfde, met de USPS-600 en de lange dunne kabel** (scheidt pad en voeding).
3. **PTC meten op de HVAC-controller:** 5 V aan de devkit en de spanning over de 5V-PTC bij rust, 1, 5 en 10 relais aan (scoop single-shot, trigger 4,5 V). Doel: hoeveel zakt de PSSE510 en hoe veel valt er over de PTC bij 0,6–0,9 A? Interpretatie: 5 V ≥ 4,75 V en 3,3 V ≥ 3,25 V = ok; 5 V < 4,5 V of 3,3 V < 3,2 V = DHT22-risico.
4. **Dip-test met de DPS150** op de testroom (limiet 1 A, OVP instellen, start 5,0 V): 4,6 → 4,4 → 4,2 V en noteer bij welke spanning de DHT22 uitvalt of de ESP reset. Dit bepaalt de marge.
5. **Relaisbelasting op de testroom** (optioneel): relaismodule op dezelfde 5 V (achter de PTC), met de hand schakelen; meet de dip en kijk of `Temp2`/`Vocht` wegvalt.
6. Test 4 (DHT22 op de HVAC-controller met testsketch) alleen als 1–3 iets verdachts tonen; sketch pas na akkoord.

*In de Sjalay, volgend bezoek* (volgorde, één ding tegelijk, daarna telkens 3–5 dagen)
1. Meet eerst: 5 V aan de 5V-pin van de devkit, vóór en ná de PTC en de 3,3 V aan de DHT-pin onder normale belasting (Wi-Fi, relais aan en uit).
2. **Voed de controller via een USB-C-poort** van de devkit met de A1400 (korte kabel), SM-connector los (nooit beide). De relaisstroom loopt dan via de devkit in plaats van de PTC. Zelfde poort als in de testroom. Dit is ook een echte power cycle voor de DHT22.
3. Pas daarna **de relaisvoeding verplaatsen naar de + lijn onder de optionele regelaar** (vóór de 5V-PTC) om de PTC te ontlasten en de spoelinschakeling van de logica-rail te houden.
4. Pas daarna de PCB-snede + relais 3 (zie B4/B7) als noodoplossing, en/of een zwaardere voeding (≥ 2 A, korte dikke kabel).
5. Optioneel, kortstondig en met zorg: tijdelijke brug over de 5V-PTC als diagnose (haalt de beveiliging weg).

*Te onthouden:* de DHT22 is niet kapot of ongeschikt (testroom: 5 mnd goed); de oorzaak zit in de Sjalay-opstelling (voedingspad, warmte, bedrading, sketch). Eén variabele per test.

**Stap C — Pi, Tailscale en reservekaart**
- Reservekaart thuis opnieuw met een ander merkkaartje (Imager met verificatie), daarna opstarttest in de Pi
- **Tailscale Funnel voor familie** (KISS): `Tailscale Funnel` → **Caddy** op de Pi (poort 8080) → controller `192.168.50.10`. Eén gedeeld familiewachtwoord (basic auth); enkel `/` en de gebruikers-endpoints (`/json`, `/toggle_*`, `/set_*`, `/setcolor`) worden doorgelaten; `/advanced`, `/settings`, `/update`, `/save_settings`, `/factory_reset`, `/reboot`, `/rescan_ds`, `/clear_crash_log`, `/capabilities` worden geblokkeerd (404). Huurders krijgen enkel de lokale URL. Stappen: Caddy installeren, `Caddyfile`, `tailscale funnel --bg 8080`, testen vanaf mobiele data

**Op locatie, volgend bezoek**
- [ ] **PCB-snede + relais 3 bedraden** (zie B4): snede tussen UART-3V3-pad en 3v3-pinnetje, COM/NC op de 4-relaismodule, daarna `/dht_reset` testen en meekijken of Temp1 en kelder tijdens de cycle blijven staan
- [ ] CV AAN-kant testen (setpoint boven de kamertemperatuur)
- [ ] Bedrading afwerken met kabelgootjes; isolatie onderaan de boiler bijwerken
- [ ] Hysterese-defaults (1,0 °C CV / 5,0 °C SWW) evalueren, met de restwarmte van de brander
- [ ] **Zwaardere voeding meebrengen voor de controller** (vermoedelijke oorzaak of medeoorzaak van de DHT22-uitval en een deel van de herstarts): de huidige VOLTCRAFT USPS-600 levert max. 0,6 A en heeft een lange, dunne kabel (spanningsverlies, uitgang kan wegzakken bij belasting).
  - **Vaste, geregelde 5 V** (liefst 5,0–5,1 V, geen instelbare), **minstens 2 A, liefst 3 A** (een gewone USB-lader kan ook)
  - **Korte uitgangskabel** (≤ 0,5 m) met dikke draad (AWG 20–22)
  - **Connector die op de bestaande past:** 2-polige SM-stekker (rood = +, zwart = −). Controleer de polariteit met de multimeter vóór het aansluiten. Eerst nagaan hoe de voeding op de shield binnenkomt (schroefklem, DC-jack of USB-C) om de specificatie vast te leggen
  - De **ELIX-voeding** (3/4,5/6/7,5/9/12 V, 800 mA) alleen gebruiken na meting en als 5 V instelbaar is (zie B4)
  - Eerst **stroom meten** (multimeter in serie op de + draad, relais 1 en 4 aan) en de spanning op de shield onder belasting (zie de meetpunten hieronder), zodat de voeding goed gedimensioneerd is
  - Ook meebrengen: een stukje 2-aderige kabel met SM-connector (man en vrouw) voor het geval de bestaande connector vervangen moet worden
- [ ] **Meten met de mini-scoop** (single shot, trigger op een val onder ± 3,2 V): (1) de 3,3 V-lijn aan de DHT22-pin, (2) de datalijn IO6 met de 10 kΩ pull-up tijdens een uitval (hangt hij permanent laag?) en tijdens een leescyclus (storing van relais of ketel), (3) de 5 V-ingang van de ESP32 onder belasting, ook tijdens het opstarten en Wi-Fi-zenden. Ook de kabel en de pull-up van de DHT22 controleren
- [ ] **Test (ook voor vertrek):** VCC van de 4-relaismodule loskoppelen (relais vallen af, CV/SWW tijdelijk uit), controller kort stroomloos zetten, na 2–3 min `f` in `/json` bekijken. DHT22 blijft werken → oorzaak is de relaismodule (aparte 5 V-voeding of betere voedingsverdeling). Valt hij weg → relaismodule uitsluiten en verder meten. Module daarna weer aansluiten. Mogelijke oorzaken in volgorde: **onderdimensioneerde voeding** (VOLTCRAFT USPS-600, max. 0,6 A, lange dunne kabel), zakkende 3,3 V-lijn bij Wi-Fi-zending, 4-relaismodule, storing van de ketel op de kabels
- [ ] **Materiaal (extra DS18B20's):** 2 waterdichte DS18B20, een verbindingsprintje om ze parallel te schakelen bij de boilersensor, 2 isolatiestrips voor op de buizen, soldeergerief en klein materiaal (krimpkousen, tie-wraps, draad)

**Later**
- [ ] Chrome op Mac: pagina vernieuwt niet zonder manuele refresh (Safari werkt); vermoeden timer-throttling in achtergrondtabs
- [ ] Landingspagina testen met een echte gast; wintertest van de vorstbeveiliging
- [x] Google-koppeling voor Claude (Drive, 28/10 gedaan): de log kan rechtstreeks gelezen worden; later Docs/Sheets/Slides
- [ ] *(idee)* Shelly Flood (waterlek) en H&T (extra temp/vocht), stroommeting per pixel
- [ ] *(apart traject)* Roomsketch aligneren met de Sjalay-sketch en Matter naar de RPi: zie `OVERNAMEDOCUMENT.md`
