# Versiegeschiedenis — VAKANTIEHUIZEN CONTROLLER

Volledige changelog zoals hij tot v1.14 in de kop van de sketch stond (verhuisd in v1.14a).
Oudere geschiedenis (v0.1–v0.16): zie README.md. Nieuwe versies worden hier bovenaan de lijst "Na v1.14" toegevoegd.

## Na v1.14

### v1.14d (26nov26) — DNS bij een vast IP
- **Aanleiding:** na v1.14c crashte het Ysermonde-bord niet meer, maar elke POST faalde met HTTP -1 na 12 s zolang er een vast IP ingesteld was (.212, dat ook aan een Apple Watch werd uitgedeeld, én .90, zonder conflict). Met DHCP: 8 geslaagde POSTs op rij (18:32–19:07). Besluit: de naamopzoeking (DNS) werkt niet met de DNS-instelling van het vaste IP.
- Vast IP: `WiFi.config(ip, router, 255.255.255.0, router, 8.8.8.8)`; nieuwe functie `checkStaticDns()` na het verbinden en elke minuut: is DNS 1 leeg (0.0.0.0), dan `WiFi.setDNS(router, 8.8.8.8)`.
- `/advanced`, Controller: regel **DNS** met `WiFi.dnsIP(0)` · `WiFi.dnsIP(1)` (vast IP of DHCP).
- Huisregel (Filip): vaste IP's onder .100, buiten de DHCP-reeks van de router.

### v1.14c (26nov26) — SNTP-crash, opstart zonder warmtevraag
- **Aanleiding:** het Ysermonde-bord (thuis, op het eigen netwerk) herstartte elke ±5 min met `PANIC@POST`; daardoor kwam er nooit een rij in de Sheet. Backtrace (vertaald met de .elf van de v1.14b-build): `sntp_request` → `dns_gethostbyname` → `dns_alloc_pcb` → `udp_new_ip_type` → assert "Required to lock TCPIP core functionality!". "@POST" was toeval: de loop was aan het posten toen de lwIP-taak crashte. Ook met Claude's build (Async TCP 4.0.0), dus niet de bibliotheekversie. De Sjalay (4G) had er geen last van.
- **Oplossing:** `setenv("TZ")`, `tzset()` en `configTzTime(...)` verwijderd. De sketch gebruikte de klok nergens (geen `localtime`/`strftime`): de tijdstempel in de Sheet komt van Google, de klok op de pagina's van de browser.
- `room_temp_reliable` en `boiler_temp_reliable` starten op `false`: pas betrouwbaar na de eerste geldige meting. Voorheen stond de kamertemperatuur bij de opstart op 0 °C en "betrouwbaar", zodat relais 1 in automatische modus enkele seconden AAN ging (`[RELAIS1] -> AAN` meteen na de opstart, ook in de Sjalay → korte warmtevraag aan de WOLF bij elke herstart).
- Relaispinnen: eerst `gpio_set_level(..., rust)`, daarna `pinMode(OUTPUT)`.
- `/advanced`: AAN/UIT-tekst naast "Automatische modus" en "Relais (handmatig)" (verwarming en SWW) wordt na het schakelen meteen bijgewerkt.
- Titels van `/advanced`, `/settings` en `/update`: room-naam (was vast "Sjalay").
- `/json`, Settings en Sheet ongewijzigd.

### v1.14b (26nov26) — stabiel setup-AP
- Aanleiding: bij de eerste setup van de Ysermonde-controller (26/11) viel de verbinding met het AP weg tijdens het invullen van `/settings`.
- Het AP heet nu **"Vakantiehuis-Setup"** (was "Sjalay-Setup"), naam in `#define AP_SSID`.
- AP-modus = `WIFI_AP` (was `WIFI_AP_STA`): `WiFi.disconnect(true)` vóór het AP start, zodat de client niet meer op de achtergrond het opgeslagen netwerk zoekt en de radio niet meer van kanaal springt.
- Omdat de client nu niet meer zelf terugverbindt: zit er 5 min (`AP_RETRY_MS`) geen toestel op het AP, dan herstart de controller en probeert hij de Wi-Fi opnieuw (nodig als de router na een stroompanne later klaar is dan de controller). Met een toestel op het AP nooit.
- `/json`, Settings en Sheet ongewijzigd.

### v1.14a (26nov26) — opkuis + compileerfix
- De sketch compileerde niet in de Arduino IDE: de automatische prototypes komen vóór de eerste functie, maar `struct NvsNs` (sinds v1.10) stond pas verderop → "'NvsNs' was not declared in this scope". De struct staat nu bovenaan bij de andere vroege declaraties. Werking ongewijzigd.
- Versiegeschiedenis uit de kop van de sketch verhuisd naar dit bestand.

## v1.0 – v1.14 (oorspronkelijke kop)

```
----------------------------------------------------------------------------
v1.0 (1okt26): Eerste opgekuiste release — volledige versiegeschiedenis (v0.1-v0.16)
               en de bouwstappen-checklist uit deze header verwijderd; zie README.md
               voor de volledige ontwikkelgeschiedenis. Wijzigingen t.o.v. v0.16:
               - Controller-sectie verhuisd naar helemaal onderaan de statuspagina.
               - SWW-sectie hernoemd naar "SWW boiler"; setpoint-bereik 10-60°C
                 (was 40-60°C), zowel op /advanced als op de landingspagina.
               - HVAC-sectie volledig opgesplitst en opgeheven: DHT22-/DS18B20-
                 kamerdata verhuisd naar de Verwarmingssectie, incl. een nieuwe
                 samengevoegde "Kamertemperatuur (bron: ...)"-regel (i.p.v. de twee
                 losse regels "DS18B20 primair" + "Room temp") en een "Effectieve
                 setpoint met % vochtigheid"-regel; de Melding-regel toont voortaan
                 altijd een statuszin i.p.v. enkel bij een fout. "DS18B20 gevonden"
                 verhuisd naar "Alle DS18B20-sensoren".
               - Verlichting samengevoegd met Powerpixels tot 1 sectie; "LDR1
                 (donker=100)" hernoemd naar "OMGEVINGSLICHT (donker=100)".
               - Google Sheets-interval nu instelbaar in Settings (was vast 5 min);
                 het PIR-telvenster en de labels ("trig/X min") volgen die waarde.
----------------------------------------------------------------------------
v1.1 (1okt26): - Verwarmings-setpoint-slider: minimum 5°C (was 10°C), op /advanced
                 en de landingspagina.
               - Sectietitel "Verwarming / relais (WOLF E1)" vereenvoudigd naar
                 "Verwarming"; "Powerpixels" hernoemd naar "Verlichting".
               - /settings: volgorde "Herscan DS18B20-bus" en "Opslaan & herstart"
                 omgewisseld.
               - Landingspagina: vlam-/douche-icoon (verwarming/SWW) nu in een
                 statuscirkel (wit = rust, lichtrood/rode rand = ketelvraag/pomp
                 actief) i.p.v. een los kleurbolletje naast het icoon.
----------------------------------------------------------------------------
v1.2 (1okt26): - /json volledig herwerkt naar een compact schema met positionele
                 sleutels (a, b, c, ... + "room"), in dezelfde stijl als het
                 Zarlar-roomproject — i.p.v. de lange beschrijvende sleutels.
                 Dit ene schema voedt zowel de live-UI (/advanced + landingspagina)
                 als de Google Sheets-log.
               - Bewust weggelaten uit de live-JSON (blijven wel zichtbaar bij het
                 laden van de pagina): betrouwbaarheids-/fallback-vlaggen, tekstuele
                 meldingen, IP/mDNS/crash-teller/firmwareversie, Auto/Handmatig-
                 togglestatus, hysterese-waarden, per-sensor-detail van niet-
                 toegewezen DS18B20's, Sheets-/PIR-intervaltekst en duty-cyclus.
               - Nieuw Apps Script (zie README) hoort bij dit schema — het vorige
                 script (lange sleutelnamen) is niet langer compatibel.
----------------------------------------------------------------------------
v1.3 (1okt26): - Extra veld "y" (effectieve setpoint incl. dauwpuntcorrectie) terug
                 toegevoegd aan /json — enkel voor de live-UI (landingspagina toont
                 hiermee weer meteen de echte doeltemp i.p.v. de ruwe ingestelde
                 setpoint, net als vóór v1.2). Bewust NIET opgenomen in het Apps
                 Script, dus niet gelogd naar Google Sheets.
----------------------------------------------------------------------------
v1.4 (25okt26): - Diagnose: reset-reden (esp_reset_reason) + opstartteller (NVS) op
                 /advanced en in /json (z, A) -> zichtbaar in Sheets (Reset, Boots).
               - DHT22-bewaking: fouten-reeks (B), totaal fouten (C), leeftijd van
                 laatste geldige meting op /advanced. Bij uitval blijft de laatste
                 geldige waarde gelden voor vocht/dauwpunt tot de sensor terugkeert.
                 Na 5 opeenvolgende fouten wordt dht.begin() opnieuw uitgevoerd.
                 Ruwe e/f staan op null in /json zolang de DHT22 faalt (was 0).
                 Als kamer-terugval wordt de DHT22 enkel gebruikt als de laatste
                 geldige meting jonger is dan 5 min.
               - Google-POST: timeout 5 s, mislukkingsteller (D) en laatste duur.
               - 3 extra DS18B20-rollen (kelder, ketelpomp, retour) in /settings;
                 JSON E, F, G (null als niet toegewezen of defect). Max 6 sensoren.
               - Nieuw Apps Script v1.1 vereist (kolommen AA t/m AH).
               - Dauwpuntmarge nu ook negatief instelbaar (-10 t/m +10 °C): effectieve
                 setpoint = max(setpoint, dauwpunt + marge); met een negatieve marge
                 grijpt de dauwpuntcorrectie pas later of niet in (vochtrisico voor eigen rekening).
----------------------------------------------------------------------------
v1.5 (28okt26): - Google-POST volgt de 302-doorverwijzing van Apps Script niet meer
                 (het tweede verzoek gaf steeds HTTP 400 en telde als fout, terwijl de
                 gegevens al gelogd waren). HTTP 200, 302 en 303 gelden als geslaagd;
                 alles anders (time-out -11, 4xx, 5xx) blijft een fout. /advanced toont
                 dan "OK (HTTP nnn)".
               - INT_WDT-broodkruimel: de sketch bewaart in RTC-geheugen (RTC_NOINIT)
                 welke fase als laatste draaide (IDLE, WIFI, PIR, PIX, SHELLY, DHT, DS,
                 HEAT, SWW, POST, CRASH, OTA, SET, DHTPW). Na een herstart door INT_WDT,
                 TASK_WDT, WDT of PANIC staat die in de reset-reden, bv. "INT_WDT@POST"
                 (/advanced en kolom Reset in de Sheet). "@IDLE" = buiten de loop
                 (webserver/Wi-Fi-taak).
               - Dauwpuntbeveiliging aan/uit in /settings (standaard AAN = gedrag zoals
                 voorheen). Uit: effectief setpoint = ingesteld setpoint.
               - DHT22 power cycle via relais 3 (IO19, actief-laag, NC-contact in de
                 3,3 V-voeding van de DHT22: onbekrachtigd = DHT22 gevoed). Na 6 mislukte
                 lezingen op rij: 5 s voeding uit (datapin IO6 dan laag), daarna opwarmen
                 3 s en controle (3 lezingen). Mislukt het: eerste 3 pogingen minstens
                 5 min uit elkaar, daarna max. 1 per uur. Teller, toestand en knop
                 "DHT22 nu resetten" (/dht_reset) op /advanced. Niet in de Sheet.
               - Apps Script blijft v1.1 (geen nieuwe kolommen).
----------------------------------------------------------------------------
v1.6 (28okt26): - Relais 3 schakelt voortaan de hele SENSOR-3V3-rail (PCB-snede tussen
                 UART-connector en het 3v3-pinnetje): DHT22, DS18B20's op de T-Bus en
                 RoomSense-box. Tijdens de power cycle (voeding uit + opwarmen) worden
                 de DS18B20's NIET gelezen: laatste geldige waarden blijven staan,
                 de DS-faalteller blijft ongemoeid en de verwarming wordt niet
                 geblokkeerd. De eerste DS-lezing na het terug inschakelen negeert de
                 power-on-waarde 85,0 °C (alleen dan; elders is 85 °C geldig, bv.
                 ketelpomp). De LDR (op de uitgeschakelde rail) wordt tijdens de
                 cycle eveneens vastgehouden.
               - Verder ongewijzigd t.o.v. v1.5 (JSON, Sheet, Apps Script v1.1).
----------------------------------------------------------------------------
v1.7 (28okt26): - LDR-schaal omgekeerd: 0 = donker, 100 = fel (was 100 = donker). Geldt voor
                 /advanced, /json (sleutel l) en dus de Google Sheet (kolom Licht LDR);
                 Apps Script blijft v1.1. Bestaande Sheet-rijen blijven op de oude schaal
                 (breuk in de grafiek).
               - Donkertest: light_ldr < drempel (pixel 0 AUTO) en nacht = light_ldr <
                 drempel (nu allebei strikt kleiner dan; voorheen > en >=).
               - Drempel in Settings: "donker onder deze waarde", standaard 60 (was 40 op
                 de oude schaal). Nieuwe NVS-sleutel "ldr_dk2"; bij de eerste start wordt
                 een bestaande oude drempel (sleutel "ldr_dark") eenmalig omgerekend naar
                 100 - oud, zodat de nachtregeling hetzelfde blijft gedragen.
----------------------------------------------------------------------------
v1.8 (2nov26): - Minimum van beide setpoints instelbaar in /settings (bewakingsdrempel):
                 "Minimum kamer-setpoint" (2-25 °C, standaard 5) en "Minimum boiler-
                 setpoint" (3-50 °C, standaard 10). Dat minimum is meteen het laagste
                 setpoint dat de regeling nog aanhoudt: de schuivers (landingspagina en
                 /advanced), /set_setpoint en /set_boiler_setpoint beginnen daar. Wordt
                 het minimum boven het opgeslagen setpoint gezet, dan wordt het
                 setpoint verhoogd (en bewaard). Nieuwe NVS-sleutels "heat_min" en
                 "sww_min"; zonder die sleutels is het gedrag gelijk aan v1.7 (5 en 10).
                 Maximum blijft 30 °C (kamer) en 60 °C (boiler).
               - Geen nieuwe Sheet-kolommen, Apps Script blijft v1.1.
----------------------------------------------------------------------------
v1.9 (10nov26): USB-PINNEN VRIJ (stap 1 van 3 voor de SCD41, zie README §B7 Stap D).
               IO12/IO13 zijn op de ESP32-C6 de datalijnen D-/D+ van de ingebouwde USB
               (USB-serieel/JTAG). Het shield gebruikt IO13 als I2C-SDA (SCL = IO11), dus de USB
               moet die pinnen loslaten — zelfde oplossing als ROOM v3.4.2:
               (1) compileren met USB CDC On Boot = Disabled -> Serial = UART0 = poort "UART";
                   compileren met CDC aan geeft voortaan een foutmelding;
               (2) bij het opstarten de USB-functie van IO12/IO13 uitschakelen (pad + interne
                   pull-ups uit): releaseUsbPins(), als eerste stap in setup().
               Verder NIETS gewijzigd: geen I2C, geen SCD41, /json, Settings en Sheet zoals v1.8
               (Apps Script blijft v1.2). Test: alles werkt zoals in v1.8.
               Technisch: voorwaartse declaratie van getJSON() bovenaan, zodat de sketch ook
               compileert zonder automatische prototypes.
----------------------------------------------------------------------------
v1.10 (11nov26): EIGEN DHT22-UITLEZING + NVS IN DE CONTROLLER-SECTIE (Stap E, les uit ROOM v3.4.4).
               (1) Oorzaak van de INT_WDT-herstarts (25/10): de Adafruit-DHT-bibliotheek zet de
                   interrupts uit en wacht per flank tot 160 000 lussen, zonder te stoppen bij een
                   fout. Hapert de DHT22, dan staan de interrupts seconden uit; de C6 heeft één kern
                   -> interrupt-watchdog. Nu een eigen uitlezing dhtRead22() (zelfde als ROOM v3.4.4):
                   tijd via de klokteller van de processor, max. 100 µs per flank, stoppen bij de
                   eerste fout -> interrupts hooguit ±5 ms uit. Bibliotheek DHT niet meer nodig;
                   "herinit" (na 5 fouten op rij) zet nu enkel de pin opnieuw als ingang met pull-up.
                   DHT-monitoring, vasthouden van de laatste geldige waarde en de power cycle via
                   relais 3 blijven zoals ze waren.
               (2) Teller "fouten op rij" (dht_fail_streak, /json B) is nu 32-bit: bleef op 65535
                   steken, en omdat 65535 deelbaar is door 5 gebeurde dan bij ELKE lezing een herinit.
               (3) /advanced, Controller: "NVS-geheugen" met vakjes gebruikt/totaal/vrij en de
                   bezetting per naamruimte; naamruimtes die niet van deze sketch of het systeem zijn,
                   krijgen "(niet van deze sketch)". Enkel tonen, niets wissen.
               (4) OTA robuuster (les van 11/11: een halverwege afgebroken upload liet de update
                   "bezig" staan, zodat elke volgende upload faalde tot een /reboot):
                   - een nog lopende/vastgelopen update wordt bij een nieuwe upload eerst afgebroken;
                   - "Update mislukt!" toont voortaan de reden (bv. "Not Enough Space", "Abort");
                   - herstart pas 3 s na "Update OK" (vanuit loop(), niet meer met delay() in de
                     webserver-taak), zodat het antwoord ook via 4G/Tailscale aankomt.
               /json-keys, Settings en Sheet ongewijzigd (Apps Script blijft v1.2).
----------------------------------------------------------------------------
v1.11 (20nov26): VORSTFIX — plausibiliteitsgrenzen verlaagd (patroon gezien door Filip, 20/11).
               SENSOR_TEMP_MIN stond op 5,0 °C: een kamer kouder dan 5 °C gold als "sensor defect".
               Met de DHT22 ook weg -> kamertemp 0,0 en verwarming GEBLOKKEERD, net wanneer de
               vorstbeveiliging nodig is (gebeurd op 15/11 02:16-11:01, living op 5,0 °C).
               Nu: kamer (DS18B20 en DHT22-terugval) en boiler pas "defect" onder -10 °C.
               Een echt defecte DS18B20 wordt nog altijd herkend via CRC-fout, buiten -55..125 °C
               en DS_FAIL_THRESHOLD; de DHT22 via zijn eigen controlesom en fouttellers.
               Verder niets gewijzigd: /json, Settings en Sheet zoals v1.10 (Apps Script v1.2).
----------------------------------------------------------------------------
v1.12 (25nov26): FUNCTIES IN /settings — één sketch voor Sjalay én Ysermonde (docs/YSERMONDE_PLAN.md).
               Nieuw blok "Functies" in /settings: "Verwarming aanwezig" en "SWW-boiler aanwezig"
               (NVS "f_heat" / "f_sww", standaard JA -> Sjalay werkt exact zoals v1.11).
               Functie UIT:
                - relais 1 (verwarming) resp. relais 2 (SWW) blijft ALTIJD uit, ook in handmatige modus;
                - verwarming: bedieningsrijen (modus, relais, setpoint, effectief setpoint, ketelvraag,
                  duty, hysterese) en de verwarmingskaart op de landingspagina verborgen; de metingen
                  (DHT22, dauwpunt, kamertemperatuur) blijven zichtbaar als "Binnenklimaat";
                - SWW: het hele SWW-blok op /advanced en de SWW-kaart op de landingspagina verborgen.
               /json: nieuwe keys J = verwarming aanwezig (0/1), K = SWW-boiler aanwezig (0/1)
               (eindschema Apps Script v1.3). Geen andere keys gewijzigd.
----------------------------------------------------------------------------
v1.13 (25nov26): OTA ZICHTBAAR + VERSIE PROMINENT (vraag Filip na de OTA van v1.12, die "leek te hangen").
               (1) Map en .ino heten nu ESP32-C6_VAKANTIEHUIZEN_CONTROLLER (Arduino: map = .ino-naam);
                   #define SJALAY_VERSION -> VH_VERSION; .bin-bestanden voortaan ..._VAKANTIEHUIZEN_CONTROLLER_vX.
               (2) Versie in de kop van ELKE pagina, zoals de ROOM-sketch: "<room> · Vakantiehuis v1.13".
               (3) /update: upload via de browser MET VOORTGANGSBALK (%, KB), daarna "Controller herstart..."
                   met teller; de pagina vraagt elke 2 s /ver op tot de controller terug is (nieuwe uptime),
                   toont "Terug online: firmware vX (na N s)" en gaat naar /advanced. Draait na 30 s nog de
                   oude firmware zonder herstart -> "update niet gelukt". Na 3 min zonder antwoord -> melding.
                   De upload blijft ook gewoon met curl werken (zelfde POST, zelfde antwoord).
               (4) Nieuw: GET /ver -> tekst "versie|uptime_s" (klein, voor de OTA-pagina; /json ongewijzigd).
----------------------------------------------------------------------------
v1.14 (26nov26): HOMEWIZARD P1-METER (Ysermonde; docs/YSERMONDE_PLAN.md sectie 4). Geschreven op basis van
               de HomeWizard-documentatie, VOOR de dongle er is: na de eerste echte gegevens nakijken.
               - /settings: "HomeWizard P1 IP" (NVS "p1_ip", leeg = geen P1). Liefst vast IP in de router.
               - loop(): elke 10 s HTTP GET http://<ip>/api/v1/data (lokale API, inschakelen in de
                 HomeWizard Energy-app), max ±3,5 s wachten. Platte JSON, eigen kleine lezer (geen bibliotheek).
                 Gas: total_gas_m3 + gas_timestamp; ontbreken die (nieuwere firmware), dan het "gas_meter"-
                 item in "external" (value + timestamp).
               - Waarden gelden 60 s na de laatste geslaagde lezing; daarna null in /json (lege cel in de Sheet).
               - /json (eindschema Apps Script v1.3): pa status (0 OK, 1 geen antwoord, 2 niet ingesteld,
                 3 ongeldige data), pb vermogen W (negatief = injectie), pc/pd/pe L1/L2/L3 W, pf/pg afname
                 dag/nacht kWh, ph/pi injectie dag/nacht kWh, pj tarief (1 = dag, 2 = nacht), pk kwartier-
                 vermogen W, pl maandpiek W, pm-po spanning V, pp-pr stroom A, ps gas m³, pt gastijd
                 ("yymmddhhmmss"), pu lange stroomonderbrekingen, pv Wi-Fi dongle %. wa = 2 (watermeter nog
                 niet ingesteld).
               - /advanced: blok "Energie (P1-meter)" met live-update; landingspagina: kaart met vermogen en gas.
                 Beide enkel zichtbaar als een P1-IP is ingevuld (Sjalay: geen verschil).
               - Diagnose-stap "P1" in de herstart-breadcrumb.
----------------------------------------------------------------------------
```
