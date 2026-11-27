# Overdracht — Sjalay- en Ysermonde-controller (repo VAKANTIEHUIZEN, vroeger SJALAY-YSERMONDE)

*Stand 11 november 2026 — Filip Delannoy (FiDel), Zarlardinge (BE)*

Dit document start een nieuw gesprek met Claude over de controller van de vakantiewoning **Sjalay (Recht)**: verwarming, warm water, verlichting en sensoren. Alle details: [`README.md`](../README.md) (deel A voor familie en huurders, deel B technisch).

**Werkwijze en afspraken:** zie de [overkoepelende overdracht](https://github.com/FidelDworp/Zarlardinge-Home-Automation/blob/main/docs/ZARLAR_OVERDRACHT.md). Kort: Nederlands · nooit een sketch of script zonder GO · eerst een plan, één stap per versie (versiekop + changelog) en daarna een 48 u-test · alles via OTA · `/json`-keys en bedieningsadressen niet wijzigen zonder akkoord · README en deze overdracht mee bijwerken · na GO compileert Claude en zet de `.bin` in `bin/`.

---

## 1. Wat en waar

| | |
|---|---|
| Plaats | vakantiewoning Sjalay, Recht; internet via 4G, toegang via een RPi 3B+ met Tailscale |
| Sketch | `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER/ESP32-C6_VAKANTIEHUIZEN_CONTROLLER.ino` **v1.14d** (27/11/2026 ±20:35 via OTA in de Sjalay én op het Ysermonde-bord; v1.14 draaide in de Sjalay 25/11 23:50 → 27/11, 44 u zonder herstart). Ysermonde-bord v1.14c/d: 24 u stabiel (26→27/11, thuis, vast IP .90, zonder RoomSense), met `partitions.csv`. Map en `.ino` hernoemd door Filip op 25/11 (was `ESP32-C6_SJALAY_CONTROLLER`) |
| Klaar om te uploaden | `bin/ESP32-C6_VAKANTIEHUIZEN_CONTROLLER_v1.14d.ino.bin` (26/11: v1.14d DNS-reserve 8.8.8.8 bij een vast IP + regel DNS op `/advanced`; v1.14a compileerfix + changelog naar `docs/VERSIEGESCHIEDENIS.md`; v1.14b setup-AP "Vakantiehuis-Setup"; v1.14c SNTP-crash weg, geen warmtevraag bij de opstart, titels met room-naam). Na een geslaagde test van ±1 u op het Ysermonde-bord ook naar de Sjalay (OTA): ook daar draait de onnodige SNTP en krijgt de WOLF bij elke herstart enkele seconden warmtevraag. v1.14 blijft als terugvaloptie |
| Logging | Google Sheets via `GoogleScript/SJALAY_SHEETS_LOGGER.gs` v1.2 (met vorstalarm per mail). **Uitzondering:** deze controller post zelf (er is geen Dashboard ter plaatse) |
| Bibliotheken | OneWireNg (DHT-bibliotheek niet meer sinds v1.10), Adafruit NeoPixel, AsyncTCP + ESPAsyncWebServer |

## 2. Stand (11/11)

- 11/11: v1.8 draait 8,9 dagen zonder herstart (DHT22 dood: de bibliotheek faalt dan meteen, dus geen `INT_WDT`). Google-POST-fouten ±4,5 % (4G). Power cycle via relais 3: 215 keer "mislukt" omdat relais 3/PCB-snede nog niet bedraad zijn.

- v1.7/v1.8 draaien stabiel (op 2/11: 4,7 dagen uptime, geen `INT_WDT` meer na 26/10).
- **DHT22 dood sinds 26/10 02:07.** Opvallend: de `INT_WDT`-herstarts (25/10) stopten rond hetzelfde moment. Dezelfde herstarts gebeurden op room80 (ROOM v3.2, 5–7/11): de DHT22-uitlezing is een verdachte. ROOM v3.3 heeft daarom een "laatste stap" in Diagnose; de Sjalay heeft sinds v1.5 een vergelijkbare broodkruimel (`INT_WDT@<fase>` in de reset-reden).
- De Sjalay is de bron van de "goodies" voor ROOM v3.7/v3.8 (temperatuurbewaking, verwarmingslogica; planning 9/11).

## 3. Open punten

0. **Ysermonde-variant (24/11, plan goedgekeurd door Filip, code nog na GO):** één sketch voor Sjalay en Ysermonde (Nieuwpoort), verschillen als keuzes in `/settings`. Eerst een Ysermonde-controller die enkel de HomeWizard P1-meter uitleest en naar een eigen Sheet logt. Volgorde: **v1.12** functies (verwarming/SWW aan-uit, geflasht 25/11, OK) → **v1.13** OTA met voortgang + versie in de kop → **v1.14** HomeWizard P1 → **v1.15** login (landingspagina open op Wi-Fi, rest achter login; port forwarding, géén RPi) → **v1.16/v1.17** SCD41. Alles in [`docs/YSERMONDE_PLAN.md`](YSERMONDE_PLAN.md). Wi-Fi-gegevens NIET in de repo. **Stand 25/11 avond: v1.13 draait in de Sjalay (kop "Sjalay · Vakantiehuis v1.13" OK). Apps Script v1.3a uitgerold: oude data in tabblad "Archief v1.2" (9485 rijen), "Data" met nieuwe kolomvolgorde, nieuwe rijen komen binnen. Grafiektabbladen moeten nog naar de nieuwe kolommen. **26/11: v1.14 (HomeWizard P1) klaar, geschreven vóór de dongle (besteld 25/11); ingebruikname Ysermonde: `docs/YSERMONDE_PLAN.md` §6.** (Vorige stand: Apps Script v1.3 klaar.)**

1. Oorzaak van de DHT22-uitval vinden (geen vervanging gewenst: dezelfde sensor werkt elders). Relais 3 (IO19) schakelt de sensorrail als noodoplossing.
2. DS18B20 voor ketelpomp en retour fysiek op de bus hangen.
3. **Volgorde (GO 11/11), elk met testperiode, code telkens na GO:** **v1.9** USB-pinnen vrij → **v1.10** eigen DHT22-uitlezing + NVS tonen (Stap E; oorzaak `INT_WDT` = Adafruit-DHT-bibliotheek) → (NVS-wisknop vervallen: NVS proper) → **v1.11** vorstfix (20/11) → **v1.12** SCD41 + key `H` + logger v1.3 (kolommen AI/AJ) → **v1.13** reserve vocht/dauwpunt via SCD41 + key `I`. Details: README §B7 Stap E en D. **Stand 18/11: v1.10 draait stabiel (geen herstart sinds 11/11 ±11:18, uptime 6,9 d, Boots 16; POST-fouten 91 ≈ 4,6 %). NVS proper → wisknop vervalt. Volgende: v1.12 (SCD41 + lege cel i.p.v. 0,0 bij kamersensor-uitval) — **on hold (18/11, Filip): wachten tot er nog een andere wijziging nodig is**, dan samen in één versie.** **Correctie 20/11 (patroon gezien door Filip):** de "uitval" van de kamer-DS18B20 op 15/11 02:16–11:01 was GEEN sensorfout maar een firmwaregrens: `SENSOR_TEMP_MIN` = 5,0 °C → een kamertemperatuur onder 5 °C wordt als "sensor defect" behandeld; met de DHT22 dood → kamertemp 0,0 en **verwarming geblokkeerd, net wanneer vorstbeveiliging nodig is**. Fix = **v1.11** (20/11, GO): grenzen kamer en boiler naar −10 °C; **via OTA (webpagina `/update`) geflasht 20/11 ±19:15, draait**; **test geslaagd (22/11):** nacht 21–22/11 ±12 u onder 5 °C (min. 3,8 °C om 06:40), kamertemp bleef correct gelogd (geen 0,0), geen herstart (Boots 17), verwarming terecht uit (setpoint 2 °C). De echte DHT22-uitvallen (23–26/10) gebeurden bij 15–22 °C en hebben hiermee niets te maken. OTA-les: mislukt een upload halverwege, dan weigert v1.8/v1.9 elke volgende upload tot een `/reboot`; na een geslaagde upload geeft curl vaak "Connection reset" omdat de controller meteen herstart.
4. **Power cycle van op afstand:** beslist 11/11 → **Shelly Plug** op de voeding van de controller bij het volgende bezoek; blijft staan tot alles stabiel draait. De nieuwe DHT-uitlezing voorkomt herstarts, maar maakt een vastgelopen DHT22 niet wakker.
5. Zie README §B7 voor het volledige plan.

## 4. Praktisch

- Compileren: ESP32C6 Dev Module, Flash 16 MB, Partition Custom, **USB CDC On Boot Disabled** (sinds v1.9). Seriële monitor/flashen enkel via de poort "UART". In de Claude-sessie: arduino-cli 1.3.1, core 3.3.12, FQBN `esp32:esp32:esp32c6:FlashSize=16M,PartitionScheme=custom,CDCOnBoot=default`, lege `ctags` (getJSON() heeft sinds v1.9 een voorwaartse declaratie).
- OTA: enkel `ESP32-C6_VAKANTIEHUIZEN_CONTROLLER.ino.bin` via `/update` (sinds v1.13 met voortgangsbalk en automatisch "terug online") (van op afstand via Tailscale).
