# Overdracht — Vakantiehuizen-controller

*Om een nieuw gesprek met Claude te starten. Stand 27/11/2026 — Filip Delannoy (FiDel), Zarlardinge (BE).*

Repo **FidelDworp/VAKANTIEHUIZEN** (vroeger SJALAY-YSERMONDE): één ESP32-C6-sketch voor de vakantiehuizen **Sjalay** (Recht) en **Ysermonde** (Nieuwpoort). Overzicht en instellingen per huis: [README](../README.md).

## 1. Werkwijze

- Nederlands. **Nooit een sketch of script schrijven zonder GO van Filip**: eerst een voorstel, dan vragen.
- Eén stap per versie, daarna een testperiode (±24–48 u in de Sheet nakijken).
- Bij elke sketchwijziging: kopregel en `VH_VERSION`, [VERSIEGESCHIEDENIS](VERSIEGESCHIEDENIS.md), betrokken docs. Claude compileert en zet de `.bin` in `bin/` (huidige + één terugvalversie). Filip compileert ook lokaal als controle.
- Alles via OTA (`/update`). `/json`-sleutels en bedieningsadressen niet wijzigen zonder akkoord.
- Docs in `docs/`, firmware in `bin/`. .ino/.md niet in de chat tonen tenzij gevraagd; rechtstreeks op GitHub bijwerken. Commando's voor de Mac kant-en-klaar geven.
- **Publieke repo:** geen Wi-Fi-gegevens, wachtwoorden, publieke IP-adressen of klantgegevens.
- Overkoepelende afspraken: [ZARLAR_OVERDRACHT](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md).

## 2. Stand

| | Sjalay | Ysermonde |
|---|---|---|
| Firmware | **v1.14d** (OTA 27/11 ±20:35) | **v1.14d** (bord thuis, sinds 26/11) |
| Stabiliteit | v1.14: 44 u zonder herstart; v1.14d: eerste rijen OK | 24 u zonder herstart (26→27/11) |
| Bijzonder | DHT22 dood sinds 26/10 (power cycle nodig, Shelly Plug bij volgend bezoek) | geen RoomSense aangesloten (sensoren leeg, relais geblokkeerd); P1-dongle besteld |

**Sheet:** `*R-VAKANTIEHUIZEN_Log`, Apps Script v1.3b, beide huizen elke 5 min. Claude kan de Sheet lezen via de Google-koppeling (kolom C = huis, BF = reset-reden, BG = boots, BH = POST-fouten).

## 3. Planning

| Versie | Inhoud | Status |
|---|---|---|
| **v1.15** | **Login en beveiliging** voor port forwarding (TECHNIEK §10) | 27/11 op het Ysermonde-bord: login werkt op het eigen netwerk; test van buitenaf ter plaatse (YSERMONDE §5 stap 13) |
| v1.16 | SCD41: CO2 tonen en loggen (key `H`) | na v1.15 |
| v1.17 | SCD41 als reserve voor vocht/dauwpunt (key `I`) | na v1.16 |
| later | Watermeter Ysermonde (`wa`–`wc`) | methode nog te bekijken |

**Kleine punten** (meenemen in een volgende versie, telkens na GO):
- Lege cel in plaats van 0,0 als er geen kamertemperatuur is.
- Optie in `/settings` om IO19 als PIR (MOV2) te gebruiken in plaats van relais 3.
- P1-velden nakijken op de eerste echte data van de dongle.
- Grafiektabbladen in de Sheet naar de nieuwe kolommen (Claude kan een functie schrijven die ze opnieuw maakt).

## 4. Open punten per huis

- **Ysermonde:** ingebruikname volgens [YSERMONDE §5](YSERMONDE.md#5-checklist-ingebruikname) (eerst vast IP en P1-IP leegmaken vóór vertrek; ter plaatse subnet naar 192.168.30.x). **Claude haalt die checklist boven zodra Filip in Ysermonde is.**
- **Sjalay:** volgend bezoek volgens [SJALAY §B4](SJALAY.md#b4-openstaande-punten-ter-plaatse): Shelly Plug, meten, zwaardere voeding, CV AAN-kant testen, SCD41 en extra DS18B20's meebrengen.

## 5. Voor Claude

- Compileren in de sessie: arduino-cli, core 3.3.12, FQBN `esp32:esp32:esp32c6:CDCOnBoot=default,FlashSize=16M,PartitionScheme=custom`. Geen ctags in de sessie: na een wijziging nakijken of een nieuw type als functieparameter vóór de eerste functie staat (zie TECHNIEK §11).
- Een crash analyseren: backtrace uit de seriële monitor + het `.elf` van de build die op het bord staat, met `riscv32-esp-elf-addr2line -pfiaC -e <elf> <adressen>`.
