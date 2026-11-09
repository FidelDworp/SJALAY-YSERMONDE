# Overdracht — Sjalay-controller (SJALAY-YSERMONDE)

*Stand 7 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een nieuw gesprek met Claude over de controller van de vakantiewoning **Sjalay (Recht)**: verwarming, warm water, verlichting en sensoren. Alle details: [`README.md`](../README.md) (deel A voor familie en huurders, deel B technisch).

**Werkwijze en afspraken:** zie de [overkoepelende overdracht](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md). Kort: Nederlands · nooit een sketch of script zonder GO · eerst een plan, één stap per versie (versiekop + changelog) en daarna een 48 u-test · alles via OTA · `/json`-keys en bedieningsadressen niet wijzigen zonder akkoord · README en deze overdracht mee bijwerken · na GO compileert Claude en zet de `.bin` in `bin/`.

---

## 1. Wat en waar

| | |
|---|---|
| Plaats | vakantiewoning Sjalay, Recht; internet via 4G, toegang via een RPi 3B+ met Tailscale |
| Sketch | `ESP32-C6_SJALAY_CONTROLLER/ESP32-C6_SJALAY_CONTROLLER.ino` **v1.8** (2/11/2026), met `partitions.csv` |
| Klaar om te uploaden | `bin/ESP32-C6_SJALAY_CONTROLLER_v1.8.ino.bin` |
| Logging | Google Sheets via `GoogleScript/SJALAY_SHEETS_LOGGER.gs` v1.2 (met vorstalarm per mail). **Uitzondering:** deze controller post zelf (er is geen Dashboard ter plaatse) |
| Bibliotheken | DHT, OneWireNg, Adafruit NeoPixel, AsyncTCP + ESPAsyncWebServer |

## 2. Stand (7/11)

- v1.7/v1.8 draaien stabiel (op 2/11: 4,7 dagen uptime, geen `INT_WDT` meer na 26/10).
- **DHT22 dood sinds 26/10 02:07.** Opvallend: de `INT_WDT`-herstarts (25/10) stopten rond hetzelfde moment. Dezelfde herstarts gebeurden op room80 (ROOM v3.2, 5–7/11): de DHT22-uitlezing is een verdachte. ROOM v3.3 heeft daarom een "laatste stap" in Diagnose; de Sjalay heeft sinds v1.5 een vergelijkbare broodkruimel (`INT_WDT@<fase>` in de reset-reden).
- De Sjalay is de bron van de "goodies" voor ROOM v3.7/v3.8 (temperatuurbewaking, verwarmingslogica; planning 9/11).

## 3. Open punten

1. Oorzaak van de DHT22-uitval vinden (geen vervanging gewenst: dezelfde sensor werkt elders). Relais 3 (IO19) schakelt de sensorrail als noodoplossing.
2. DS18B20 voor ketelpomp en retour fysiek op de bus hangen.
3. Zie README §B7 voor het volledige plan.

## 4. Praktisch

- Compileren: ESP32C6 Dev Module, Flash 16 MB, Partition Custom, USB CDC On Boot Enabled; `#define Serial Serial0` bovenaan is verplicht op de C6.
- OTA: enkel `ESP32-C6_SJALAY_CONTROLLER.ino.bin` via `/update` (van op afstand via Tailscale).
