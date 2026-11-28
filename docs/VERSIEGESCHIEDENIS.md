# Versiegeschiedenis

Sketch `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER` en Apps Script `VAKANTIEHUIZEN_SHEETS_LOGGER.gs`. Nieuwste bovenaan. Oudere details staan in de git-geschiedenis.

## Sketch

### v1.16 (28nov26) — config-export
- Nieuw adres **`/config.json`** (admin, achter login): alle sleutels van de NVS-naamruimte `sjalay-cfg`, automatisch overlopen (`nvs_entry_find`), elk met **label** en waarde: `"hmin": { "label": "Minimum kamer-setpoint (graden C)", "value": 5 }`. Kop met room, versie, datum (van de browser), MAC en IP.
- Volgorde = volgorde van `/settings`, daarna de bedieningsstanden; sleutels zonder label achteraan met "(geen label)". Labeltabel `CFG_LABELS` bovenaan de sketch (reeksen zoals `pixel_nick_*`).
- Wachtwoorden (Wi-Fi, login) als `***`. Floats (dauwpuntmarge, hysterese) als getal, andere blobs (DS18B20-adressen) als hex.
- `/settings`: blok "Configuratie bewaren" met **Bekijken** en **Downloaden** (`<room>_config_<datum>.json`). Enkel exporteren, niets terugzetten.
- Map `config/` in de repo voor de bestanden.

### v1.15 (27nov26) — login en beveiliging
- Instelbare login in `/settings`: gebruikersnaam, wachtwoord (leeg laten = niet wijzigen), "Landingspagina van buitenaf" (login vragen / open). NVS `au_user`, `au_pass`, `au_ext`. Gebruikersnaam of wachtwoord leeg = geen login (gedrag zoals v1.14d).
- Digest-login (realm "Vakantiehuis"). Eigen netwerk (zelfde subnet): landingspagina en haar bediening open, al de rest login. Van buitenaf: alles login (behalve met de optie "open"). `/ver` en het setup-AP blijven open.
- OTA-upload: zonder geldige login wordt er niets geschreven.
- 5 foute pogingen per IP → 5 min blokkering (HTTP 429). `/advanced` → Controller: regel "Login" met de stand, de soort verbinding en de tellers.
- `/json`, Sheet en de bediening zelf ongewijzigd.

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

### v1.0 – v1.14

| Versie | Inhoud |
|---|---|
| v1.14 (26/11) | HomeWizard P1-meter: IP in `/settings` (leeg = geen P1), elke 10 s `GET /api/v1/data`, eigen kleine JSON-lezer (ook gas via `external`). `/json` + `pa`–`pv`, `wa` = 2. `/advanced`: blok "Energie (P1-meter)"; landingspagina: energiekaart. Enkel zichtbaar met een P1-IP |
| v1.13 (25/11) | Map en `.ino` hernoemd naar `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER`. Versie in de kop van elke pagina. `/update` met voortgangsbalk en "terug online" (via nieuw `GET /ver`) |
| v1.12 (25/11) | "Verwarming aanwezig" en "SWW-boiler aanwezig" in `/settings` (uit = relais altijd uit, bediening verborgen). `/json` + `J`/`K`. Eerste stap voor Ysermonde |
| v1.11 (20/11) | Vorstfix: meetondergrens kamer en boiler naar −10 °C (onder 5 °C gold de kamersensor als defect → verwarming geblokkeerd, 15/11). Patroon gezien door Filip |
| v1.10 (11/11) | Eigen DHT22-uitlezing `dhtRead22()` i.p.v. de Adafruit-bibliotheek (oorzaak `INT_WDT`-herstarts). Teller "fouten op rij" 32-bit. NVS-bezetting op `/advanced`. OTA robuuster (vastgelopen update afbreken, reden tonen, herstart vanuit `loop()`) |
| v1.9 (10/11) | USB CDC On Boot uit en `releaseUsbPins()`: IO12/IO13 vrij voor I2C (SCD41) |
| v1.8 (2/11) | Minimum kamer- en boiler-setpoint instelbaar (2–25 / 3–50 °C) |
| v1.7 (28/10) | LDR-schaal omgekeerd: 0 = donker, 100 = fel (oude Sheet-rijen omgerekend) |
| v1.6 (28/10) | Relais 3 schakelt de hele sensorrail; DS18B20/LDR niet lezen tijdens de cycle, 85,0 °C erna negeren |
| v1.5 (28/10) | POST-doorverwijzing niet meer volgen (200/302/303 = OK); reset-reden met fase; dauwpuntbeveiliging aan/uit; DHT22 power cycle via relais 3 (IO19) |
| v1.4 (25/10) | Diagnose (reset-reden, opstartteller), DHT22-bewaking, POST-timeout 5 s en foutenteller, 3 extra DS18B20-rollen, `null` bij DHT-uitval, negatieve dauwpuntmarge |
| v1.3 | Veld `y` (effectief setpoint) in `/json` |
| v1.2 | `/json` met compacte letter-sleutels; nieuw Apps Script |
| v1.1 (1/10) | Verwarmingsminimum 5 °C, statuscirkels op de landingspagina |
| v1.0 (1/10) | Opgeruimde release, Sheets-interval instelbaar, boilerbereik 10–60 °C |
| v0.1–v0.16 | Platform, sensoren, verwarmings- en SWW-logica, powerpixels, Shelly, landingspagina, mDNS, bed-modus |

## Apps Script

| Versie | Inhoud |
|---|---|
| v1.3b (27/11) | Huisfilter begint op rij 1 (zwarte balk) i.p.v. de koprij: de filterknopjes staan niet meer in de kolomkoppen (die wrapten). Criterium = formule "koprij of gekozen huis" |
| v1.3a (25/11) | Archief-functie: eerst vastgezette rijen/kolommen losmaken, archief overslaan als "Data" leeg is |
| v1.3 (24/11) | Eindschema 63 kolommen A–BK per soort data, één Sheet voor alle huizen met huisfilter in C1, alarm per huis, `testYsermonde()`; overstap met `archiveerEnNieuweKoppen()` (oude rijen naar "Archief v1.2") |
| v1.2 (2/11) | Vorstalarm per mail (kamer/boiler/kelder) + "geen data"-melding; logt altijd naar tabblad "Data" |
| v1.1 (25/10) | Kolommen voor sketch v1.4, notaties (1 decimaal, tijdstempel) |
