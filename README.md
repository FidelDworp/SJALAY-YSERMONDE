# Vakantiehuizen-controller

Eén ESP32-C6-controller met **één gedeelde sketch** voor de vakantiehuizen van Filip Delannoy. De controller bedient verwarming, warm water, lampen en sensoren, leest de energiemeters uit en logt alles naar één Google Sheet. De verschillen tussen de huizen zijn **keuzes in `/settings`**, geen aparte sketches.

| Huis | Plaats | In gebruik | Wat de controller doet |
|---|---|---|---|
| **Sjalay** | Recht | sinds oktober 2026 | verwarming (WOLF-ketel), warm water (boiler 155 l), 4 lichtgroepen (Shelly), temperaturen, vorstalarm |
| **Ysermonde** | Nieuwpoort | in voorbereiding (bord draait thuis sinds 26/11/2026) | energiemeter (HomeWizard P1: elektriciteit + gas), later verwarming (Vaillant), lampen en sensoren |

**Stand 27/11/2026:** beide controllers draaien **v1.14d** en loggen elke 5 min naar de Sheet.

---

## Instellingen per huis

Zoals ze nu in `/settings` staan. Wi-Fi-gegevens, wachtwoorden en het publieke IP-adres staan bewust **niet** in deze publieke repo.

| Instelling (`/settings`) | Sjalay | Ysermonde: nu (thuis, test) | Ysermonde: ter plaatse |
|---|---|---|---|
| Room-naam | `Sjalay` | `Ysermonde` | `Ysermonde` |
| mDNS-naam | `sjalay.local` | `ysermonde.local` | `ysermonde.local` |
| Vast IP | `192.168.50.10` | `192.168.0.90` | `192.168.30.90` (eerst subnet wijzigen, zie [YSERMONDE](docs/YSERMONDE.md)) |
| Router (= DNS 1) · DNS 2 | `192.168.50.1` (4G-router) · 8.8.8.8 | `192.168.0.1` · 8.8.8.8 | `192.168.30.1` · 8.8.8.8 |
| Verwarming aanwezig | **Ja** (relais 1 → WOLF E1) | Ja (relaistest) | **Ja** zodra het relais aan de Vaillant hangt, anders Nee |
| SWW-boiler aanwezig | **Ja** (relais 2 → WOLF SF) | Nee | **Nee** (doorstromer) |
| HomeWizard P1 IP | leeg (geen meter) | `192.168.0.100` (test, antwoordt niet) | `192.168.30.91` |
| Google Script-URL | gemeenschappelijke URL (zie [TECHNIEK §7](docs/TECHNIEK.md#7-google-sheet-en-apps-script)) | idem | idem |
| Sheets-interval | 5 min | 5 min | 5 min |
| DS18B20-rollen | kamer (RoomSense), boiler, kelder | 1 sensor (RoomSense) | kamer |
| Pixels / Shelly's | 4 lichtgroepen, Shelly's `.11–.14` | 8 pixels (test) | Shelly-stopcontacten (later) |
| Toegang van buitenaf | Tailscale via RPi (`192.168.50.2`) | Tailscale thuisnetwerk | port forwarding op de Telenet-modem, **pas na v1.15 (login)** |
| Internet | 4G (TP-Link Archer MR600) | thuis (Telenet) | Telenet (coax) |

---

## Documenten

| Document | Voor wie | Inhoud |
|---|---|---|
| [`docs/SJALAY.md`](docs/SJALAY.md) | familie, huurders, Filip | **Gebruik** (WhatsApp-bericht, thermostaat, app, als iets niet werkt, terugdraaien zonder controller) en **installatie** Sjalay: ketel en relais, netwerk/RPi/Tailscale, voeding, openstaande punten ter plaatse |
| [`docs/YSERMONDE.md`](docs/YSERMONDE.md) | Filip | Installatie Ysermonde: shield v1.0 en relais, meters en HomeWizard P1, netwerk (lessen + beslissing 192.168.30.x), **checklist vóór vertrek en ter plaatse** |
| [`docs/TECHNIEK.md`](docs/TECHNIEK.md) | Filip, Claude | Gemeenschappelijke techniek: compileren en OTA, pinnen, pagina's en endpoints, instellingen, regeling, sensoren en robuustheid, `/json`-sleutels, Google Sheet en Apps Script, netwerkregels, geleerde lessen |
| [`docs/OVERDRACHT.md`](docs/OVERDRACHT.md) | Claude (nieuw gesprek) | Werkwijze, huidige stand, planning (v1.15 login → SCD41 → watermeter) en open punten |
| [`docs/VERSIEGESCHIEDENIS.md`](docs/VERSIEGESCHIEDENIS.md) | Filip, Claude | Alle versies van sketch en Apps Script |
| `docs/Verwarming_in_de_Sjalay.pdf` | Filip | Achtergrond verwarming Sjalay |

## Repo

| Map / bestand | Inhoud |
|---|---|
| `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER/` | sketch (`.ino`, versie in `VH_VERSION`) + `partitions.csv` (16 MB); rechtstreeks te openen in de Arduino IDE |
| `bin/` | klaar-om-te-uploaden firmware via `/update`: **v1.14d** (huidig) en v1.14 (terugval) |
| `GoogleScript/VAKANTIEHUIZEN_SHEETS_LOGGER.gs` | Apps Script **v1.3b** voor de gemeenschappelijke Sheet `*R-VAKANTIEHUIZEN_Log` |
| `docs/` | documentatie (tabel hierboven) |
