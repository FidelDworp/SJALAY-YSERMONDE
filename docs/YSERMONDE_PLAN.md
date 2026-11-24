# Ysermonde-variant — plan (één sketch voor Sjalay en Ysermonde)

*Opgesteld 24/11/2026 in overleg met Filip. Code telkens pas na GO, één stap per versie, elk met een testperiode.*

## 1. Doel

Het vakantiehuisje in **Ysermonde (Nieuwpoort)** krijgt dezelfde controller als de Sjalay (Recht): zelfde hardware (ESP32-C6 + Zarlar/RoomSense-shield, zelfde sensoren) en **dezelfde sketch**. De verschillen tussen beide huizen worden **keuzes in `/settings`**, geen aparte sketches. Zo blijft er één sketch om te onderhouden (les van de Photonix-shields: aparte varianten lopen uit elkaar).

## 2. Verschillen tussen beide huizen

| | Sjalay (Recht) | Ysermonde (Nieuwpoort) |
|---|---|---|
| Verwarming | WOLF-ketel, relais als thermostaatcontact | Vaillant-doorstromer, thermostaat = eenvoudig maakcontact → zelfde regeling |
| Sanitair warm water | boiler 155 l, SWW-relais + boilersensor | **geen** (doorstromer) → SWW uit |
| Lampen | pixels + Shelly's | Shelly-stopcontacten (bestaande functie: per pixel `ip:naam`) |
| Tellers | — | **digitale meter Fluvius** (elektriciteit + gas) via **HomeWizard Wi-Fi P1-meter**; later de watermeter (draadloos, Aquaduin) |
| Internet | 4G | Telenet (coax), vast IP-adres |
| Toegang van buitenaf | RPi 3B+ met Tailscale | **port forwarding** op de Telenet-router (zoals de bestaande IP-thermostaat), géén RPi |

Wi-Fi-gegevens staan **niet** in deze (publieke) repo: ze worden ingevuld via het AP-portaal of `/settings` van de controller.

## 3. Beveiliging bij port forwarding (v1.14)

Met port forwarding staat de controller rechtstreeks op het internet (gewone HTTP, geen versleuteling). Daarom:

| | Op de Wi-Fi van het huisje (huurders) | Van buitenaf (via de router) |
|---|---|---|
| Landingspagina: lampen, temperatuur | **open** | login |
| `/advanced`, `/settings`, `/update`, `/json`, reset, reboot, … | login | login |

- Gebruikersnaam en wachtwoord in `/settings`; **leeg = geen login** (Sjalay achter Tailscale blijft zoals nu).
- **Digest-login** (wachtwoord gaat niet leesbaar over het net; de rest van het verkeer wel).
- Controller ziet zelf of een verzoek van het lokale netwerk komt of van buitenaf (IP-adres van de afzender).
- **Blokkering** na 5 foute pogingen (5 min).
- Keuze in `/settings`: landingspagina ook van buitenaf open (standaard: nee).
- Router: een **ongewone externe poort** (bv. 8xxx) doorsturen naar poort 80 van de controller. Vast IP → geen DynDNS nodig.

## 4. HomeWizard P1-meter (v1.13)

- Eén dongle op de P1-poort leest elektriciteit **en** gas (de gasmeter stuurt via de digitale meter). P1-poort is open bij Fluvius (in orde volgens Filip).
- In de HomeWizard Energy-app **"Lokale API" inschakelen**; de dongle best een vast IP geven (DHCP-reservatie in de router).
- De controller leest `http://<ip>/api/v1/data` (JSON) elke 10–30 s, via het lokale netwerk.
- Velden: `active_power_w`, `total_power_import_t1_kwh`, `total_power_import_t2_kwh`, `total_power_export_t1_kwh`, `total_power_export_t2_kwh`, `total_gas_m3`, `gas_timestamp`.
- Tonen op `/advanced` (en compact op de landingspagina), in `/json` en in de Sheet.
- **`/json`-keys en Sheet-kolommen: VAST sinds 24/11** (Apps Script v1.3, eindschema A–BK). Volledige tabel in README §B6. P1: `pa` status, `pb`–`pe` vermogen totaal/L1–L3, `pf`/`pg` afname dag/nacht, `ph`/`pi` injectie dag/nacht, `pj` tarief, `pk` kwartiervermogen, `pl` maandpiek, `pm`–`po` spanning, `pp`–`pr` stroom, `ps` gas m³, `pt` gastijd, `pu` lange stroomonderbrekingen, `pv` Wi-Fi dongle. Verder `H`/`I` (SCD41), `J`/`K` (functies), `wa`–`wc` (watermeter).
- **Sheet:** **één Sheet voor beide huizen** (de bestaande Sjalay-log, logger v1.3, al klaar in de repo), zelfde web-app-URL in `/settings` van elk huis. **Huisfilter: keuzelijst in C1** (ALL / Sjalay / Ysermonde), zoals in de ROOM-Sheet; grafieken volgen het filter. Alarmmails per huis. Verbruik per interval of per dag berekent een extra tabblad uit de meterstanden.

## 5. Volgorde van de versies

Gewijzigd 24/11: de Ysermonde-controller komt eerst, als **P1-logger** (nog zonder verwarming, lampen of sensoren). De SCD41 (Sjalay) schuift op.

| Versie | Inhoud | Test |
|---|---|---|
| **v1.12** | **Functies in `/settings`:** "Verwarming aanwezig" ja/nee en "SWW-boiler aanwezig" ja/nee. Uit = relais blijven uit, bijhorende sensoren/meldingen/UI-blokken verdwijnen, geen vorstalarm voor wat er niet is. Sjalay: alles aan, gedrag ongewijzigd. | Sjalay: alles werkt zoals v1.11 |
| **v1.13** | **HomeWizard P1** (sectie 4); logger v1.3 is al klaar (24/11). Ysermonde-controller in dienst: verwarming uit, SWW uit, P1-IP ingevuld, eigen Sheet. | Sjalay: geen verschil (geen dongle). Ysermonde: meterstanden en vermogen in de Sheet |
| **v1.14** | **Login en beveiliging** (sectie 3). Pas nodig als de router de controller naar buiten brengt. | Ysermonde: login van buitenaf, landingspagina open op Wi-Fi |
| **v1.15** | SCD41: CO2 + key `H` + Sheet-kolommen AI/AJ (was v1.12) | met sensor |
| **v1.16** | SCD41 als reserve voor vocht/dauwpunt + key `I` (was v1.13) | met sensor |
| later | Watermeter (Aquaduin, draadloos): uitleesmethode nog te bekijken | — |

## 6. Ysermonde-controller in gebruik nemen (bij v1.13)

1. ESP32-C6 (later met shield; voor de P1-logger volstaat het devkit alleen).
2. **Eerste flash met kabel** via de USB-C-poort "UART" (USB CDC On Boot = Disabled); daarna alles via OTA.
3. AP-portaal: Wi-Fi, room-naam "Ysermonde", vast IP of DHCP-reservatie.
4. `/settings`: room-naam "Ysermonde", Verwarming **uit**, SWW **uit**, P1-IP invullen, **dezelfde** Google Script-URL als de Sjalay.
5. HomeWizard: lokale API aan, vast IP.
6. Port forwarding pas na v1.14.

## 7. Open vragen

- HomeWizard P1-meter: nog te kopen (24/11).
- Watermeter: type en uitleesmethode.
- Externe poort voor port forwarding (bij v1.14).
