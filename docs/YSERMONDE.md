# Ysermonde (Nieuwpoort)

Vakantiehuisje met een Vaillant-gasdoorstromer (geen boiler) en een Telenet-internetaansluiting (coax, vast publiek IP). Eerste doel: de **energiemeter uitlezen en loggen** (HomeWizard P1: elektriciteit + gas). Daarna verwarming, lampen en sensoren, en toegang van buitenaf via port forwarding met login.

Gemeenschappelijke techniek (sketch, `/json`, Sheet): [TECHNIEK.md](TECHNIEK.md).

**Stand 27/11/2026:** het Ysermonde-bord draait thuis op **v1.14d** (vast IP 192.168.0.90, room-naam "Ysermonde"), 24 u stabiel, logt elke 5 min. Relaismodule met optocoupler getest (schakelt correct mee met een sensor). De HomeWizard-dongle is besteld (25/11) en wordt bij het volgende bezoek geplaatst.

---

## 1. Verschillen met de Sjalay

| | Sjalay | Ysermonde |
|---|---|---|
| Verwarming | WOLF-ketel, relais op E1 | Vaillant-doorstromer, relais als **maakcontact** op de thermostaatklemmen (nu de Proliphix-thermostaat) → zelfde regeling |
| Warm water | boiler 155 l, relais 2 + boilersensor | **geen** (doorstromer) → "SWW-boiler aanwezig" = Nee |
| Lampen | pixels + Shelly's | Shelly-stopcontacten (per pixel `ip:naam`) |
| Meters | — | elektriciteit + gas via **HomeWizard P1**; later water |
| Internet | 4G | Telenet (coax), vast publiek IP |
| Toegang van buitenaf | RPi + Tailscale | **port forwarding** op de Telenet-modem, met login (v1.15), géén RPi |

## 2. Hardware

**Shield v1.0** ("ESP32-C6 shield v1.0 by iTroniX") met een QSZNTEC-devkit (DOIT ESPC6-32, **16 MB**, twee USB-C: "UART" en "USB"). Pinnen zoals de Sjalay, dus zelfde sketch:

| Functie | Pin | Op de print |
|---|---|---|
| DS18B20 T-Bus | IO3 | RoomSense "3 T-Bus" |
| Pixels data | IO4 | OPTIONAL "4 Pixels" + PIXEL-LINE (GND/4/5V) |
| PIR MOV1 | IO5 | RoomSense "5 Mov1" |
| DHT22 | IO6 | RoomSense "6 DHT22" |
| LDR1 | IO1 | RoomSense "1 LDR1" |
| **Relais 1 (verwarming)** | **IO10** | OPTIONAL **"10 TSTAT"** |
| I2C SDA / SCL (SCD41) | IO13 / IO11 | I2C-header (4k7 pull-ups) |

**Aandachtspunten**
- **Geen 5 V op de OPTIONAL-connector.** De relaismodule krijgt 5 V en GND van de **PIXEL-LINE-header**; signaal op "10 TSTAT".
- **Relaismodule MET optocoupler** (817C/PC817 naast elk relais, actief-laag, jumper Vcc–JDVcc laten staan). Bedrading (26/11): Gnd → PIXEL-LINE GND, In1 → "10 TSTAT", Vcc → PIXEL-LINE 5V. COM + NO → thermostaatklemmen van de Vaillant (**eerst de spanning daarop meten**). Geen sketchwijziging nodig.
- **Een nieuwe relaismodule eerst testen:** In1 tegen GND → aan; tegen 3v3 → **uit**; los → uit.
- **Niet gebruiken:** de 2-relaismodule **zonder** optocoupler (PNP-ingang op 5 V: bleef aan bij 3,3 V, In1 stond op 4,24 V) en MOSFET-modules (niet potentiaalvrij).
- **IO2** ("LDR2") is in de sketch relais 2 (SWW): geen LDR2 aansluiten. **IO19** ("MOV2") is relais 3 (uitgang): geen PIR op MOV2.
- Zonder betrouwbare kamertemperatuur blijft relais 1 uit, ook in handmatige modus (veiligheidsregel). Voor een relaistest dus een RoomSense of DS18B20 aansluiten.
- **Voeding:** USB-C met een gewone 5 V-lader volstaat; later de schroefklem "Power IN". Nooit twee voedingen tegelijk.

## 3. Meters en HomeWizard P1

- **Elektriciteit:** Sagemcom Siconia XT211, **driefasig**, Fluvius. P1-poort achter het lichtgroene klepje linksonder (gebruikerspoort, open bij Fluvius; na 2 jaar opnieuw aanvragen). Tarief 1 = dag (1.8.1), 2 = nacht (1.8.2), bevestigd op de meter.
- **Gas:** Apator Metrix UG G4 iSMART2, draadloze M-Bus naar de elektriciteitsmeter → zit in het P1-telegram.
- **Water:** aparte meter onderaan in de kast, niet via P1 (later, methode nog te bekijken).
- **Dongle:** HomeWizard Wi-Fi P1 Meter (HWE-P1), besteld 25/11. Meterkast = keukenkast naast de oven: Wi-Fi-bereik en of het deurtje dicht kan, ter plaatse nakijken.
- **Uitlezing (v1.14):** de controller leest elke 10 s `http://<ip-dongle>/api/v1/data` (lokale API, aan te zetten in de HomeWizard Energy-app). Alle velden staan in TECHNIEK §8 (`pa`–`pv`). Geschreven vóór de dongle er was: na de eerste echte data de veldnamen nakijken.

## 4. Netwerk

**Beslist 26/11: Ysermonde krijgt een eigen netwerk `192.168.30.x`.**

| | Nu | Na de wijziging |
|---|---|---|
| Modem (Telenet) | `192.168.0.1` | **`192.168.30.1`** |
| Controller | — | **`192.168.30.90`** (vast) |
| HomeWizard-dongle | — | **`192.168.30.91`** (vast, als de app dat toelaat) |
| Proliphix-thermostaat | `192.168.0.217` (DHCP) | nieuw adres via DHCP, nakijken |
| Telenet-toestel "Compal" (vermoedelijk tv-box) | `192.168.0.218` (DHCP) | nieuw adres via DHCP |

**Waarom:** thuis (Zarlardinge) is ook `192.168.0.x` en wordt via Tailscale gerouteerd. Op de Wi-Fi in Ysermonde bereik je de thuiscontrollers dan niet (je toestel kiest het lokale netwerk); tot nu toe moest je daarvoor mobiele data gebruiken. Met een eigen reeks verdwijnt die overlap (thuis `0.x`, Sjalay `50.x`, Ysermonde `30.x`), en kan Ysermonde later ook in Tailscale.

**Modem-instellingen:** enkel via **Mijn Telenet → Je thuisnetwerk → Modem → Geavanceerde instellingen** (expertinstellingen ontgrendelen): LAN-subnet (aanpasbaar), port forwarding, UPnP, DMZ, firewall. De DHCP-reeks is er **niet** zichtbaar en er is geen DHCP-reservatie: vaste adressen worden in het toestel zelf ingesteld (huisregel: **onder .100**; Telenet deelt hoge adressen uit, .2xx).

**Port forwarding** pas na v1.15 (login): lokaal IP `192.168.30.90`, een niet-standaard externe poort (bv. 8080; niet 25/53/135/137/139/161/162/445/1080) → interne poort 80, TCP. Poort 80 extern dicht laten.

## 5. Checklist ingebruikname

**Vóór vertrek (thuis)**
1. `/settings`: **vast IP leegmaken** (DHCP). Anders start de controller daar met een thuisadres en is hij onbereikbaar.
2. `/settings`: **P1-IP leegmaken** (het testadres bestaat daar niet).
3. Verwarming: **Ja** als het relais meteen aan de Vaillant komt, anders **Nee**. SWW-boiler: **Nee**.

**Ter plaatse, in deze volgorde**
4. **Subnet wijzigen:** Mijn Telenet → Geavanceerde instellingen → LAN-subnet `192.168.30.1` → opslaan (modem herstart).
5. **Proliphix nakijken:** nieuw adres bij "Verbonden toestellen"? Heeft hij in zijn eigen menu een vast `192.168.0.x`-adres, dat aanpassen naar `192.168.30.x` (of DHCP). Een bestaande port forwarding naar de Proliphix mee aanpassen.
6. **Controller op de Wi-Fi:** start hij als "Vakantiehuis-Setup": verbinden, `http://192.168.4.1/settings`, Wi-Fi van het huisje invullen. Op de Mac "Automatisch verbinden" voor andere netwerken tijdelijk uit en het inlogvenster wegklikken maar verbonden blijven. Daarna `http://ysermonde.local/`.
7. **Vast IP:** `/settings` → `192.168.30.90`. Controle op `/advanced`: regel **DNS** = `192.168.30.1 · 8.8.8.8`.
8. **Dongle:** in de gebruikerspoort, koppelen met de HomeWizard Energy-app (2,4 GHz), **lokale API aan**, liefst vast `192.168.30.91`.
9. **Dongle testen:** in de browser `http://<ip-dongle>/api/v1/data` → JSON doorsturen naar Claude.
10. `/settings`: **HomeWizard P1 IP** invullen.
11. **Nakijken:** `/advanced` → "Energie (P1-meter)" Status OK; landingspagina: energiekaart; ±5 min later een rij "Ysermonde" met P1-waarden in de Sheet.
12. **Tailscale-test:** op de Wi-Fi van het huisje een thuiscontroller openen (bv. `http://192.168.0.80/`), zonder mobiele data.
13. **Login (v1.15)** in `/settings` instellen (op de Wi-Fi van het huis), dan pas de **port forwarding**. Test van op mobiele data: ook de landingspagina vraagt login, en `/advanced` toont "deze verbinding: van buitenaf".

## 6. Open vragen

- Kan het vaste adres van de dongle in de HomeWizard-app ingesteld worden? Zo niet: het adres dat hij krijgt noteren (of later: de sketch zoekt hem zelf).
- Spanning op de thermostaatklemmen van de Vaillant (vóór het relais aan te sluiten).
- Watermeter: type en uitleesmethode.
