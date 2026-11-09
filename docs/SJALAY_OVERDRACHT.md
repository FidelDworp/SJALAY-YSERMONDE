# Overdracht — Sjalay-controller (SJALAY-YSERMONDE)

*Stand 10 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een nieuw gesprek met Claude over de controller van de vakantiewoning **Sjalay (Recht)**: verwarming, warm water, verlichting en sensoren. Alle details: [`README.md`](../README.md) (deel A voor familie en huurders, deel B technisch).

**Werkwijze en afspraken:** zie de [overkoepelende overdracht](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md). Kort: Nederlands · nooit een sketch of script zonder GO · eerst een plan, één stap per versie (versiekop + changelog) en daarna een 48 u-test · alles via OTA · `/json`-keys en bedieningsadressen niet wijzigen zonder akkoord · README en deze overdracht mee bijwerken · na GO compileert Claude en zet de `.bin` in `bin/`.

---

## 1. Wat en waar

| | |
|---|---|
| Plaats | vakantiewoning Sjalay, Recht; internet via 4G, toegang via een RPi 3B+ met Tailscale |
| Sketch | `ESP32-C6_SJALAY_CONTROLLER/ESP32-C6_SJALAY_CONTROLLER.ino` **v1.9** (10/11/2026; v1.8 draait nog tot de OTA), met `partitions.csv` |
| Klaar om te uploaden | `bin/ESP32-C6_SJALAY_CONTROLLER_v1.9.ino.bin` (v1.8 blijft als terugvaloptie tot v1.9 getest is) |
| Logging | Google Sheets via `GoogleScript/SJALAY_SHEETS_LOGGER.gs` v1.2 (met vorstalarm per mail). **Uitzondering:** deze controller post zelf (er is geen Dashboard ter plaatse) |
| Bibliotheken | DHT, OneWireNg, Adafruit NeoPixel, AsyncTCP + ESPAsyncWebServer |

## 2. Stand (7/11)

- v1.7/v1.8 draaien stabiel (op 2/11: 4,7 dagen uptime, geen `INT_WDT` meer na 26/10).
- **DHT22 dood sinds 26/10 02:07.** Opvallend: de `INT_WDT`-herstarts (25/10) stopten rond hetzelfde moment. Dezelfde herstarts gebeurden op room80 (ROOM v3.2, 5–7/11): de DHT22-uitlezing is een verdachte. ROOM v3.3 heeft daarom een "laatste stap" in Diagnose; de Sjalay heeft sinds v1.5 een vergelijkbare broodkruimel (`INT_WDT@<fase>` in de reset-reden).
- De Sjalay is de bron van de "goodies" voor ROOM v3.7/v3.8 (temperatuurbewaking, verwarmingslogica; planning 9/11).

## 3. Open punten

1. Oorzaak van de DHT22-uitval vinden (geen vervanging gewenst: dezelfde sensor werkt elders). Relais 3 (IO19) schakelt de sensorrail als noodoplossing.
2. DS18B20 voor ketelpomp en retour fysiek op de bus hangen.
3. **SCD41 toevoegen (plan goedgekeurd 9/11):** CO2 tonen en loggen + reserve voor vocht en dauwpunt als de DHT22 uitvalt. Vooraf via OTA, SCD41 standaard uit; ter plaatse enkel aanvinken. Volgorde: **v1.9** USB-pinnen vrij → **v1.10** SCD41 + key `H` + logger v1.3 (kolommen AI/AJ) → **v1.11** reserve vocht/dauwpunt + key `I`, elk met testperiode. Details: README §B7 Stap D. **v1.9 klaar (10/11), wacht op OTA en test**; daarna v1.10 (na GO).
4. Zie README §B7 voor het volledige plan.

## 4. Praktisch

- Compileren: ESP32C6 Dev Module, Flash 16 MB, Partition Custom, **USB CDC On Boot Disabled** (sinds v1.9). Seriële monitor/flashen enkel via de poort "UART". In de Claude-sessie: arduino-cli 1.3.1, core 3.3.12, FQBN `esp32:esp32:esp32c6:FlashSize=16M,PartitionScheme=custom,CDCOnBoot=default`, lege `ctags` (getJSON() heeft sinds v1.9 een voorwaartse declaratie).
- OTA: enkel `ESP32-C6_SJALAY_CONTROLLER.ino.bin` via `/update` (van op afstand via Tailscale).
