# Sjalay Controller — vakantiewoning op afstand monitoren en besturen (Recht)

**Locatie:** Sjalay (Recht) — geen permanente WiFi/fiber, gevoed via 4G  
**Basis:** ESP32-C6 + RoomSense/Zarlar-shield, eigen webinterface, Google Sheets-logging, Tailscale voor toegang van buitenaf  
**Laatst bijgewerkt:** 30 sep 2026

Dit is geen los "ketel-projectje" meer: de Sjalay-controller doet ondertussen vier dingen tegelijk in het vakantiehuis:

1. **Verwarming én SWW op afstand aan/uit** — relais 1 op de E1-ingang van de WOLF-ketel, relais 2 op de SF-klem (Speicherfühler-ingang)
2. **Sensoren monitoren** — temperatuur (meerdere DS18B20-rollen), vocht, licht, beweging, met logging naar Google Sheets
3. **Verlichting aansturen** — powerpixels, optioneel gekoppeld aan Shelly-stopcontacten
4. **Overal bereikbaar** — via Tailscale, zonder vaste internet-IP of poort-forwarding, en lokaal ook via mDNS (`http://sjalay.local/`)

---

## 1. Verwarming & warmwater op afstand

### 1.1 Toestellen

- **Ketel:** WOLF F/CNK/U-25 — SN 168120/1144, bouwjaar 2014, 20–25 kW, Mat.-Nr. 8906850
- **SWW-boiler:** CB-155/FB-155/TB-155, 155 liter, SN 1214
- **Bediening:** BM 2744329 (wandmodule in de living, communiceert via eBUS)

### 1.2 Aansluitpunt: E1-ingang op de ketel (gevonden en bevestigd ✅)

De ketel heeft een parametreerbare, potentiaalvrije ingang **E1** op de klemmenstrook van de ketel zelf (niet op de BM-wandsokkel). De functie wordt bepaald door parameter **HG13** (fachmannebene, instelbereik 1–11).

**HG13 = 1 (Raumthermostat) — bevestigd op de BM (29/09) ✅** Vakmanniveau bereikt via: rechtse ronde knop indrukken → roteren naar "VAKMAN" → bevestigen → code invoeren (**code = 1**, staat in de Montageanleitung) → instellingen "Ketel" (Heizgerät) → HG13. Waarde stond effectief op **1**, dus fabrieksinstelling bevestigd, geen wijziging nodig geweest.

| E1-toestand | Effect |
|---|---|
| **Open** | Verwarming (Heizbetrieb) geblokkeerd — "Sommerbetrieb" |
| **Gesloten** | Verwarming draait normaal, volgens het schema/setpoint van de BM |

Deze functie raakt **uitsluitend de CV-verwarming**. Warmwaterbereiding (SWW) wordt er niet door beïnvloed; die wordt apart geregeld via de SF-klem, zie 1.4.

**Fysiek gevonden en elektrisch bevestigd (29/09):** op de klemmenstrook boven de regelmodule zit een rij van 4 klemblokjes (groen/geel/blauw/wit). Het **gele blokje** droeg enkel een kortsluitbrug en geen enkele sensor — de andere drie (groen/blauw/wit) hebben elk een echte 2-draads sensor erop aangesloten. Meting bevestigde dit definitief:
- **Met brug: 0V** (kortgesloten = "gesloten" = verwarming toegelaten)
- **Zonder brug: 5V** (zwevend/pull-up = "open" = verwarming geblokkeerd)

Dit klopt exact met een potentiaalvrije logica-ingang met interne pull-up naar 5V — de ketel levert zelf de sensorspanning, het relais moet enkel kortsluiten/openen (nooit zelf spanning injecteren). **Het gele blokje is dus bevestigd als E1.**

**Overwogen, niet gekozen:**
- *HG13 = 2 (Maximalthermostat)* — bij open contact blijft de brander volledig geblokkeerd, óók tijdens Schornsteinfeger-, kaskade- en **vorstbeveiligingsbedrijf**, voor zowel CV als SWW (letterlijk uit de manual: "Bei geöffnetem Kontakt bleibt der Brenner auch im Schornsteinfeger-, Kaskaden-, und Frostschutzbetrieb für Warmwasser und Heizung gesperrt"). **Bewust afgewezen (29/09):** dit zou onze fail-safe-logica omdraaien — de veilige rusttoestand van het relais (onbekrachtigd/open) zou dan net vorstbeveiliging uitschakelen, op het moment dat ze het hardst nodig is. Bovendien zouden CV en SWW dan aan hetzelfde signaal vastzitten.
- *Fernschaltkontakt op de BM-wandsokkel* (klemmen 3-4, potentiaalvrij) — functioneel gelijkaardig, maar stuurt altijd CV **en** SWW samen, niet apart regelbaar. Bewaard als alternatief/backup-aansluitpunt.
- *HG13 = 5–11* — specifieke technische functies, niet geschikt als eenvoudige aan/uit-schakelaar.

**⚡ Bestaande brug over E1 — eerst verwijderen!**

De brug simuleert momenteel een permanent "gesloten contact" (verwarming toegelaten). Die brug moet **verwijderd** worden vóór je het relais aansluit — parallel zetten werkt niet, een gesloten brug + relais blijft altijd "gesloten". Knip/verwijder de brug en sluit in de plaats daarvan de twee relaisdraden aan op diezelfde twee E1-klemmen (het gele blokje).

**Veiligheid — nooit vergeten:**
- Nooit veiligheidscontacten (STB, maximumthermostaat, druk…) overbruggen
- Galvanische scheiding via relais — de ketel levert zelf de 5V, het relais mag geen eigen spanning op de lijn zetten
- Relais **standaard open** (niet-bekrachtigd): bij stroomuitval, ESP-crash of vóór de Wi-Fi-verbinding staat, valt het systeem altijd terug naar de veilige "geblokkeerd"-toestand
- Foto's maken van de volledige klemmenstrook vóór je iets loskoppelt

### 1.3 Bedrijfsstrategie voor een vakantiewoning (belangrijk!)

Uitgangspunt: een dure stookolieketel mag **niet blijven draaien terwijl er niemand is**, behalve voor vorstbeveiliging — maar bij aankomst/vertrek moet er wél volledige controle op afstand zijn, zonder dat iemand fysiek aan de BM moet komen.

E1 is de **hoofdschakelaar** die alles overstijgt: zolang E1 **open** staat, blijft de verwarming geblokkeerd, ongeacht wat de BM's eigen klok/schema zegt.

**Vorstbeveiliging bevestigd (29/09) ✅** — de Kurzbedienungsanleitung van de BM vermeldt expliciet dat in "Sommerbetrieb (Heizung aus)" de **Pumpenstandschutz en Frostschutz actief blijven**. Onze E1-open-toestand (HG13=1) schakelt de ketel precies in dat Sommerbetrieb. Vorstbeveiliging blijft dus gegarandeerd actief in de veilige rusttoestand. Praktische wintertest blijft aangeraden.

**Praktische regel — zo werkt het in gebruik:**

1. **De BM-module blijft fysiek gewoon staan** op een normale, comfortabele instelling. Er moet **nooit iemand aan de BM zelf** komen bij aankomst of vertrek.
2. **Standaard (niemand aanwezig): E1 open** (relais uit) → verwarming geblokkeerd, vorstbeveiliging blijft actief.
3. **Vóór aankomst:** relais **sluiten** via de Sjalay-webinterface — verwarming start en volgt vanaf dan het normale BM-schema/setpoint.
4. **Tijdens verblijf:** relais gesloten laten — dagelijkse regeling gebeurt door de BM zelf.
5. **Bij vertrek:** relais weer **openen** via de webinterface.

Zo wordt de BM een "domme" thermostaat die altijd hetzelfde comfortschema aanhoudt, en is E1 de enige externe aan/uit-knop.

### 1.4 SWW op afstand — relais 2 op de SF-klem (nog fysiek te bedraden)

E1/HG13=1 regelt enkel CV. Voor SWW werd overwogen: HG13=2 (**afgewezen**, zie 1.2 — breekt de vorstbeveiliging-fail-safe en koppelt CV/SWW onlosmakelijk aaneen).

*(Een eerder plan om i.p.v. de SF-klem de 230V-laadpomp zelf te onderbreken is verlaten: dat blokkeert enkel de pomp, niet de warmtevraag, en laat de brander dan nutteloos kortcyclen tegen een dichte klep.)*

**De warmtevraag zelf blokkeren via de SF-klem (Speicherfühler), i.p.v. de pomp:**

Uit de WOLF Regelung-R2-handleiding (§Fachmannebene Parameter, HG24 "Warmwasser-Fühler-Betriebsart", gecontroleerd 30/09):
- **Betriebsart 1** (fabrieksinstelling): normale elektronische Speicherfühler (NTC) op de SF-klem.
- **Betriebsart 3**: de SF-klem wordt in plaats daarvan bediend door een **extern, potentiaalvrij thermostaatcontact** — exact zoals E1 werkt voor CV. Letterlijk uit de handleiding: *"Fühlereingang geschlossen: Pumpe ein / Fühlereingang offen: Pumpe aus"* — en belangrijker: de brander verwarmt de ketel enkel tot Speichersolltemperatuur zolang dat contact **gesloten** is. Staat het contact **open**, dan is er voor de ketel gewoon geen SWW-vraag — de brander wordt dus nooit nutteloos gestart, in tegenstelling tot het pomp-onderbreken-plan.
- Let op: *"Nach Änderung der Fühlerbetriebsart muss die Anlage aus- und wieder eingeschaltet werden"* — een herstart van de ketel is nodig na het wijzigen van HG24.

**HG24 bevestigd bereikbaar (30/09) ✅** — staat momenteel op **1** (fabriekswaarde), klaar om bij de volgende keer naar **3** te zetten. Navigatie identiek aan HG13 (zie 1.2): rechtse ronde knop → "VAKMAN" → bevestigen → **code 1** → "Ketel" (Heizgerät) → HG24. **Tip:** zonder eerst code 1 in te voeren kom je niet door naar het "Ketel"-menu — die stap is dus niet optioneel.

**Praktisch:**
- **Bedrading:** relais 2 (aangestuurd via **IO2**, zie 4) gaat niet meer naar de 230V-fasedraad van de laadpomp, maar naar de **SF-klem** — een lichte, potentiaalvrije signaaldraad, geen 230V-schakeling meer nodig voor dit circuit (veiliger, eenvoudiger te bedraden).
- **Op de ketel:** HG24 moet van 1 naar **3** gezet worden (Fachmannebene, net als HG13 in 1.2), en de bestaande fabrieks-Speicherfühler (indien aanwezig op de SF-klem) moet losgekoppeld/vervangen worden.
- **Sketch:** **geen enkele wijziging nodig.** Relais 2 + de eigen boilersensor (DS18B20, zie 5.10) + de bestaande hysterese-logica blijven functioneel identiek — enkel *waar* relais 2 fysiek naartoe gaat verandert, niet de software die het aanstuurt.

**Klemlocatie bevestigd (30/09, uit het Schaltplan Heizkesselregelung R2):** SF zit op dezelfde klemmenstrook **X20** als E1, AF en eBUS — het middelste klemmenpaar, vlak naast E1: `AF (grijs) — *SF (blauw) — *E1 (geel) — eBUS (groen)`. Beide (`*SF` en `*E1`) staan gemarkeerd als "Zubehör" (optioneel accessoire).

**Let op — verschil met E1:** de handleiding vermeldt enkel bij E1 expliciet *"Brücke... (Parameter HG13) entfernen"* — bij SF staat geen soortgelijke brug-voetnoot. Niet zomaar aannemen dat SF hetzelfde brugje heeft als E1 had.

**Fabriekssensor bevestigd aanwezig (30/09) ✅** — op de blauwe klem (SF) hangt effectief een echte 2-draads sensor (wit/rood), geen brugje. Spanningsmeting over de klem: **2,52V bij een actuele SWW-temperatuur van 49°C** — bevestigt een levende, temperatuurafhankelijke sensor (vermoedelijk gemeten via een interne pull-up-spanningsdeler op de regelprint, net als bij E1's 5V-pull-up). Ter vergelijking, geïnterpoleerd uit de NTC-tabel (20°C≈6247Ω, 60°C≈1244Ω, 80°C≈628Ω) verwacht je bij 49°C een weerstand rond **~1900-2000Ω** — de exacte omrekening van de gemeten 2,52V naar Ω is niet mogelijk zonder de pull-up-waarde van de regelprint te kennen, maar dat is ook niet nodig: in Betriebsart 3 leest de ketel geen analoge waarde meer, enkel open/dicht.

**⚡ Belangrijk — de fabriekssensor moet eraf, niet ernaast blijven hangen:** uit de handleiding is Betriebsart 3 nadrukkelijk "een extern thermostaat **of** elektronische Speichertemperaturfühler" — niet allebei tegelijk op dezelfde klem. Reden: in Betriebsart 3 verwacht de regelmodule op SF een eenvoudig open/dicht-signaal, geen analoge weerstand. Blijft de bestaande NTC-sensor (~1900-6000Ω, afhankelijk van temperatuur) parallel aan het relais hangen, dan ziet de ketel een tussenwaarde die niet ondubbelzinnig "open" (oneindig) of "gesloten" (~0Ω) is — dat geeft onvoorspelbaar gedrag: verkeerde interpretatie, permanent "gesloten" gelezen, of willekeurig schakelen.

**Bedradingsvolgorde bij de effectieve omschakeling (volgende keer ter plaatse):**
1. HG24 op de BM eerst van 1 naar 3 zetten (herstart van de ketel nadien, zie hierboven)
2. De bestaande wit/rode sensor **loskoppelen** van het blauwe blokje (SF)
3. De twee relais-2-draden (potentiaalvrij, net als bij E1) in de plaats daarvan op diezelfde SF-klem aansluiten
4. Ketel herstarten (al vereist door de HG24-wijziging zelf)

**Neveneffect om bewust van te zijn:** zodra de fabriekssensor eraf is, heeft de ketel/BM zelf geen boilertemperatuur-uitlezing meer — zijn eigen interne weergave/logica voor SWW verdwijnt, want SF is dan puur een schakelaar. Geen probleem voor Sjalay: de eigen DS18B20-boilersensor blijft onafhankelijk de temperatuur meten/loggen/tonen op de landingspagina en `/advanced` — maar de ketel/BM zelf toont dan geen boilertemp meer, mocht daar ooit naar gekeken worden.

**Status:** plan vastgelegd en volledig fysiek voorverkend (30/09) — fabriekssensor, klemlocatie én HG24-toegang alle drie bevestigd (zie hierboven). Sketch-logica (v0.11) blijft ongewijzigd bruikbaar. Enkel de effectieve omschakeling (HG24 op 3, sensor loskoppelen, relais 2 aansluiten, herstarten) en toewijzing van de boilersensor in Settings staan nog te gebeuren; pas daarna in bedrijf te nemen.

### 1.5 Voor te bereiden thuis (Zarlardinge, vóór de volgende keer)

- [ ] **ESP32-C6-shield + 4-kanaals relaismodule inbouwen** in de bestelde wandmontagebox (20 × 12 × 7,5 cm)
- [ ] **Houten plank (20 × 40 cm)** voorbereiden om de box op de muur (kelder Sjalay) te bevestigen
- [ ] **Binnenplankje (20 × 12 cm)** maken om de PCB's (shield + relaismodule) op te schroeven, in de box
- [ ] **Alle doorvoeropeningen** al voorboren in de box (voeding, RoomSense-RJ45, relais-1-bedrading naar E1, relais-2-bedrading naar SF, DS18B20-boilersensor/T-bus-verlengdraad) — vermijdt improviseren ter plaatse
- [ ] **DS18B20 solderen met 3 draadjes van 10 cm** op de T-bus, zodat de sensor net buiten de nieuwe box uitsteekt — voor het meten van de kelder-/omgevingstemperatuur (dit is de sensor bedoeld in 5.10 als "T-bus-testsensor")

### 1.6 Mee te brengen naar Sjalay (volgende keer)

**Materiaal**
- **Meeraderige kabel, min. 2 paar (4 aders)** — één kabel voor zowel relais 1 → E1 (geel) als relais 2 → SF (blauw), elk paar één circuit
- **5V voedingsblokje + rood/zwart voedingskabeltje** voor de ESP32-C6-shield
- **Behuizing** voor controller-shield + relaismodule (zie 1.5 — best al thuis ingebouwd meebrengen)
- **RoomSense-kabel met RJ45 aan beide kanten**
- DS18B20 waterdichte sensor (boiler), indien nog niet in bezit
- OneWire-verlengkabel (3-aderig: DATA/VCC/GND) voor de boilersensor-aftakking
- WAGO-lasklemmetjes (2- en 3-voudig)
- Krimpkousjes/isolatietape (voor de losgekoppelde fabriekssensor-draadjes, zie 1.4)
- Reserveschroefjes voor de klemmenstrook

**Gereedschap**
- Kleine platte + kruiskop schroevendraaier (klemmenstrook X20/E1)
- Multimeter
- Draadstripper/afstriptang + zijkniptang
- WAGO-krimptang of soldeerbout+tin
- **RJ45-krimptang** (bij Maarten ophalen!)
- **RJ45-kabeltester**
- Dymo-labelprinter (of thuis geprinte labels, zie lijstje hieronder)
- Hoofdlamp/zaklamp
- Kabelbinders

**Labellijstje (Dymo of thuis printen):**
1. `RELAIS 1 → E1 (VERWARMING)` — x2 (beide kabeluiteinden)
2. `RELAIS 2 → SF (SWW/BOILER)` — x2 (beide kabeluiteinden)
3. `5V VOEDING ESP32-C6` — x2
4. `ROOMSENSE (RJ45)` — x2 (beide zijden van de kabel)
5. `BOILERSENSOR DS18B20` — x2
6. `SJALAY CONTROLLER` — x1 (op de nieuwe behuizing zelf)

**Niet vergeten:** Vakman-toegangscode (**1**)

---

## 2. Netwerk

| Onderdeel | Instelling |
|---|---|
| 4G-router | TP-Link Archer MR600 (Telenet ONE data-SIM, uit oude iPhone 5) — lokaal IP `192.168.50.1` |
| Snelheid | ≈125/25 Mb/s |
| Subnet Sjalay | **192.168.50.0/24** — gateway `.50.1`, DHCP-pool `.100–.199`, vaste IP's `.2–.99` |
| mDNS-naam controller | **sjalay.local** (default, instelbaar in Settings — zie 5.12) |
| Vaste IP's toegekend | Raspberry Pi (Tailscale-router) `.2`; Sjalay-controller (ESP32) `.10`; Shelly-stopcontacten `.11`–`.14` |

*Hernummerd van `192.168.1.0/24` op 28/09 — zie hoofdstuk 3 voor de reden.*

---

## 3. Toegang van buitenaf — Raspberry Pi + Tailscale

Een **Raspberry Pi 3B+** in Recht fungeert als **Tailscale subnet router**: het hele lokale Sjalay-netwerk is zo van overal bereikbaar, zonder poort-forwarding of vast internet-IP.

**Setup (28 sep 2026):**
- Raspberry Pi OS 64-bit (Debian 13), hostname `fidel` (mDNS: `fidel.local`)
- Tailscale: `curl -fsSL https://tailscale.com/install.sh | sh`, dan `sudo tailscale up --advertise-routes=192.168.50.0/24 --accept-dns=false`
- Route goedgekeurd in Tailscale-adminconsole (machine **rpi-fidel-sjalay**)
- IP-forwarding ingeschakeld

**Subnet-conflict opgelost:** het thuisnetwerk in Zarlardinge gebruikte hetzelfde `192.168.1.0/24`. Opgelost door Recht te hernummeren naar `192.168.50.0/24`.

**Resultaat:** bevestigd werkend — bereikbaar vanaf een iPhone met wifi uitgeschakeld, via de Tailscale-app (zowel de subnet-route naar het Sjalay-netwerk zelf — bv. de ESP32-landingspagina — als rechtstreekse SSH naar de Pi, zie 3.1).

**Thuis (Zarlardinge) — zelfde aanpak nu ook daar actief (30/09) ✅:** naast de bestaande Funnel-opzet adverteert de Zarlar-Pi (`rpi-raspberrypi-zarlar`) sinds 30/09 ook het volledige lokale subnet `192.168.0.0/24` (`sudo tailscale up --advertise-routes=192.168.0.0/24 --accept-dns=false`, IP-forwarding aangezet via `/etc/sysctl.d/99-tailscale.conf`, route goedgekeurd in de Tailscale-adminconsole). Bevestigd bereikbaar van op afstand (mobiele data, wifi uit): alle lokale wifi-toestellen in Zarlardinge zijn nu net als in Recht rechtstreeks via hun `192.168.0.x`-adres bereikbaar, zonder per toestel iets te moeten instellen. Detailstappen en netwerkspecifieke achtergrond horen thuis in het aparte Zarlar-document, niet hier.

### 3.1 SSH-toegang tot de Pi (bevestigd werkend, 30/09)

**Adressen van de Pi:**
- **Lokaal LAN-IP:** `192.168.50.2` — **vast ingesteld** (30/09), geen DHCP meer
- **Tailscale-IP (vast, onafhankelijk van het lokale netwerk):** `100.77.122.107`
- **Tailscale MagicDNS-naam:** `rpi-fidel-sjalay.tail3c7f42.ts.net`

**SSH-login:** user `pi`, wachtwoord **`fidel2026`** (gewijzigd op 30/09 via `passwd` op de Pi zelf — het oude fabriekswachtwoord `raspberry` werkt niet meer). Termius-hostprofielen (zowel het lokale-IP- als het Tailscale-IP-profiel) zijn bijgewerkt en bevestigd werkend met het nieuwe wachtwoord.

**Bevestigd (30/09):** zowel het Tailscale-IP als het lokale IP werken van op afstand — het Tailscale-subnetrouting-mechanisme functioneert prima. (Wat eerst leek op een subnet-route-probleem bleek gewoon een verouderd/verkeerd geraden lokaal IP in het Termius-host-profiel — geen Tailscale-fout.)

**Lokaal IP vastgezet op `.2` (30/09):** de Pi kreeg voorheen een dynamisch DHCP-adres (was `.103`, binnen de pool `.100`-`.199`), wat kon wijzigen bij een lease-vernieuwing. Vastgezet via NetworkManager (deze Pi gebruikt netplan met NetworkManager als renderer — de `netplan-wlan0-sjalayke`-connectie is hier het gezag, niet de auto-gegenereerde `/etc/netplan/90-NM-*.yaml`-bestanden):
```
sudo nmcli con mod "netplan-wlan0-sjalayke" ipv4.addresses 192.168.50.2/24 ipv4.gateway 192.168.50.1 ipv4.dns 192.168.50.1 ipv4.method manual
sudo nmcli con up "netplan-wlan0-sjalayke"
```
Bevestigd met `ip addr show wlan0` → `inet 192.168.50.2/24 ... valid_lft forever`. `.2` gekozen als eerste vrije adres net boven de gateway (`.1`), buiten het bereik van de ESP32/Shelly's (`.10`-`.14`) en de DHCP-pool.

**Wachtwoord gewijzigd (30/09) ✅** — het fabriekswachtwoord `raspberry` is vervangen door `fidel2026` via `passwd` (rechtstreeks op de Pi, over SSH). Hiermee is het eerder gesignaleerde beveiligingsrisico (bekend/geraden fabriekswachtwoord bereikbaar vanop afstand via Tailscale) opgelost.

---

## 4. Hardware — shield pinout (ESP32-C6)

| Pin | Device/Functie | Gebruikt door Sjalay-sketch? |
|-----|-----------------|-------------------------------|
| IO13 | I2C SDA (pull-up 4.7k → 5V) | nee |
| IO11 | I2C SCL (pull-up 4.7k → 5V) | nee |
| IO3 | DS18B20 OneWire (T-bus + verlengdraad, meerdere sensoren parallel via uniek ROM-adres) | **ja** — temperatuur (kamer + boiler + evt. overige) |
| IO4 | Pixels data | **ja** — powerpixels |
| IO5 | PIR MOV1 | **ja** — beweging |
| IO6 | DHT22 data | **ja** — temp/vocht |
| IO12 | Sharp dust LED out | nee |
| IO7 | Sharp dust analog | nee |
| IO1 | LDR1 analog (10k pull-up → 3V3) | **ja** — licht |
| IO18 | CO2 PWM input (MH-Z19, 5V!) | nee |
| IO19 | MOV2 PIR (of Dotstar CLK) | nee |
| **IO10** | **TSTAT switch (Gnd = ON)** | **ja — relais 1 → WOLF E1** |
| **IO2** | (was: LDR2 analog, ongebruikt) | **ja — relais 2 → SF-klem (Speicherfühler-ingang ketel)** (zie 1.4) |
| IO15 | Reserve 2 (Output) | vrije reserve (was eerst gepland voor relais 2, IO2 gekozen i.p.v. wegens bedradingsgemak) |
| IO20/WKP | Reserve 3 | nee |
| IO0 | Reserve 1 (BOOT pin) | nee — liever niet gebruiken |
| GND / VIN / 3V3 | Voeding | — |

Beide relais zijn fail-safe-laag (`RELAY_ACTIVE_LOW`) en staan vóór Wi-Fi-verbinding al UIT in `setup()`.

---

## 5. De Sjalay-sketch — huidige implementatie (v0.16, 30 sep 2026)

Bij twijfel is de code in de sketch (`SJALAY_CONTROLLER_v0.16.ino`, intern versienummer `SJALAY_VERSION` bijgewerkt naar 0.16) de bron van waarheid; dit is een leeswijzer erbij.

**Getest op het thuisnetwerk (30/09) ✅** — controller naar fabrieksinstellingen gereset en op het Zarlardinge-netwerk gezet, met een volledige RoomSense-shield via RJ45-kabel. Bevestigd werkend: PIR-beweging in AUTO-modus schakelt pixel 0 aan, en de nieuwe AUTO/MANUEEL-knop (v0.16, zie 5.11) werkt zoals bedoeld.

### 5.1 Bestand en board-instellingen

- Arduino IDE: **Board** = ESP32C6 Dev Module, **Flash Size** = 16MB, **Partition Scheme** = Custom (`partitions.csv`), **USB CDC On Boot** = Enabled
- `#define Serial Serial0` bovenaan — verplicht op de C6
- Versienummer in `#define SJALAY_VERSION`, zichtbaar in UI (Controller-groep op `/advanced`) en `/json` (`ver`)
- Nieuw sinds v0.14: `#include <ESPmDNS.h>` (standaard onderdeel van de ESP32-Arduino-core, geen extra library-installatie nodig)

### 5.2 Webinterface — pagina's en endpoints

**Pagina's:** `/` (eenvoudige landingspagina voor gasten/huurders, **nieuw in v0.12, bijgeschaafd in v0.13/v0.15/v0.16** — zie 5.11), `/advanced` (technische Status-pagina, **v0.12: was voorheen `/`**), `/update` (OTA), `/settings` (Settings), `/json` (ruwe data). Sinds v0.14 zijn `/` en `/advanced` ook bereikbaar via `http://<mdns-naam>.local/` (zie 5.12), naast het gewone IP-adres.

| Endpoint | Werking |
|---|---|
| `/save_settings` | verwerkt Settings-formulier, herstart |
| `/factory_reset` | wist alle NVS-instellingen, herstart |
| `/clear_crash_log` | wist crash-teller |
| `/rescan_ds` | herscant DS18B20-bus |
| `/toggle_heating_auto` | wisselt Automatisch/Handmatig voor verwarming (relais 1) |
| `/toggle_relay_manual` | wisselt relais-1-stand (handmatige modus) |
| `/set_setpoint?value=` | verwarmings-setpoint-slider (10-30 °C) — ook gebruikt door de landingspagina |
| `/toggle_sww_auto` | wisselt Automatisch/Handmatig voor SWW (relais 2) |
| `/toggle_relay2_manual` | wisselt relais-2-stand (handmatige modus) |
| `/set_boiler_setpoint?value=` | boiler-setpoint-slider (40-60 °C) — ook gebruikt door de landingspagina |
| `/toggle_pixel_mode` | pixel 0: AUTO ↔ MANUEEL — **sinds v0.16 ook rechtstreeks bereikbaar vanop de landingspagina** (voorheen enkel op `/advanced`) |
| `/toggle_pixel?idx=` | pixel `idx` aan/uit — **v0.12:** `idx=0` schakelt impliciet naar MANUEEL als hij nog in AUTO stond (relevant voor de landingspagina; geen gedragswijziging op `/advanced`) |
| `/setcolor?r=&g=&b=` | powerpixel-kleur — ook gebruikt door de landingspagina |
| `/set_fade_duration?value=` | dim-snelheid (1-10 s) |
| `/set_light_on_min?value=` | licht-aan tijd na PIR (0-30 min) |
| `/toggle_bed` | bed-modus (dwingt pixel 0 uit) |
| `/capabilities` | JSON met pixelnamen |
| `/reboot` | herstart direct |

### 5.3 Settings-velden

- **Algemeen:** room-naam, Wi-Fi SSID/wachtwoord, static IP (leeg = DHCP), **mDNS-naam (nieuw in v0.14, zie 5.12)**, dauwpuntmarge, hysterese verwarming, hysterese SWW, LDR donker-drempel, aantal pixels, Google Script-URL, MAC (read-only)
- **Pixel-namen:** naam + optioneel Shelly-koppeling (`ip:nickname,ip:nickname`, max 3) per pixel
- **Sensoren (DS18B20):** nickname per sensor, keuze **primaire sensor (kamer)**, keuze **boilersensor (apart, optie "geen")**
- Los: herscan DS18B20-bus, crash-log wissen, factory reset

### 5.4 Verwarmingslogica (relais 1) — echte hysterese + veiligheidslagen

- **Automatisch:** `effective_setpoint = max(setpoint, dauwpunt + dew_margin)`; **symmetrische hysteresisband** rond die effectieve setpoint: AAN onder `effective_setpoint - hyst_cv/2`, UIT boven `effective_setpoint + hyst_cv/2`, ertussen blijft de stand behouden. Default `hyst_cv = 1,0°C`, instelbaar in Settings (0,2-5°C).
- **Handmatig:** relais volgt rechtstreeks de `/toggle_relay_manual`-schakelaar
- **Veiligheidslagen (ALTIJD actief, ongeacht Auto/Handmatig):**
  1. Geforceerd UIT zodra `room_temp` de bovengrens van de setpoint-slider bereikt (**30°C**)
  2. Geforceerd UIT bij onbetrouwbare temperatuurdata (kamer-DS én DHT22 beide defect/ontbrekend)
- Relais wordt **onmiddellijk** aangepast bij elke wijziging
- **Duty-cyclus:** 4-uur sliding window (12 × 20 min), zichtbaar in UI en `/json`/Sheets

### 5.5 Powerpixels — pixel 0 en bed-modus

Volgorde van voorrang:
1. **Bed-modus** aan → pixel 0 altijd uit, ongeacht modus
2. **AUTO/MANUEEL** (enkel relevant als bed-modus uit): AUTO = aan bij beweging + donker genoeg; MANUEEL = losse schakelaar

Pixels 1+ zijn altijd rechtstreeks manueel, persistent, zonder bed-override.

### 5.6 Google Sheets-logging

Actief zodra `gas_url` ingevuld is. Elke 5 minuten HTTPS POST van de volledige `/json`-payload. Laatste resultaatcode (`gcode`) zichtbaar in UI.

### 5.7 Shelly-stopcontacten per pixel

Elke pixel kan gekoppeld worden aan 0-3 Shelly-stopcontacten die simultaan meeschakelen — lokaal, geen cloud. Formaat in Settings: `192.168.50.11:Keukenlamp,192.168.50.12:Tafellamp`. Bij een effectieve aan/uit-wissel van de pixel stuurt de sketch `http://<ip>/relay/0?turn=on|off` (Gen1/2/3-compatibel), fire-and-forget met korte timeout.

**Uitgerold en bevestigd werkend (29/09):** 4 Shelly-stopcontacten gekoppeld en getest, vaste IP's toegekend in `192.168.50.x` (`.11`–`.14`).

**Toekomstidee (niet gepland, nu niet nodig):** Shelly's (Gen2/3, Matter) hebben ingebouwde vermogensmeting via `http://<ip>/rpc/Switch.GetStatus?id=0` — zou ooit stroom-/spanningsmeting per pixel kunnen tonen. Blijft een idee, geen actieve to-do.

**Andere Shelly-toestellen (toekomstidee, 30/09) — zelfde lokale HTTP-API, geen cloud nodig:**

Naast de 4 stopcontacten heeft Shelly een breed assortiment WiFi-toestellen die met dezelfde aanpak (Gen1: `/relay/0?turn=on|off`, Gen2/3/4: `/rpc/Switch.Set`) rechtstreeks door de sketch aan te spreken zijn:

- **Relais/schakelaars (inbouw):** Shelly Plus 1 / Plus 1PM (1-kanaals, achter bestaande schakelaar), Shelly Plus 2PM (2-kanaals, bv. rolluiken), Shelly Pro-serie (DIN-rail, meterkast)
- **Dimmen & verlichting:** Shelly Dimmer (dimbare LED/halogeen), Shelly RGBW2 (ledstrips, eventueel aanvullend op de powerpixel-aanpak)
- **Sensoren:** Shelly H&T (temp/vocht, batterij — aanvulling op de DS18B20's), Shelly Motion (alternatief voor PIR MOV1/MOV2), **Shelly Flood** (waterlekdetectie — relevant voor een leegstaande vakantiewoning), Shelly Door/Window (deur-/raamcontact)
- **Verwarming:** Shelly TRV (thermostatische radiatorkraan op kamerniveau — de huidige E1/SF-aanpak via de ketel zelf blijft fundamenteler)
- **Energie:** Shelly EM / 3EM (stroom-/verbruiksmeting, sluit aan bij het idee hierboven)

Meest kansrijk om ooit toe te voegen: **Shelly Flood** (waterlek-detectie in een leegstaand huis) en **Shelly H&T** (extra temp/vocht-meetpunten zonder bekabeling) — beide lokaal, geen cloud, passend bij de bestaande filosofie. Volledig assortiment en aankoop: [shelly.com](https://www.shelly.com/) (of [us.shelly.com](https://us.shelly.com/) voor de Amerikaanse site) — geen actieve to-do.

### 5.8 Gekende aandachtspunten

- NVS-instellingen overleven firmware-updates (namespace `"sjalay-cfg"`); enkel factory reset wist alles
- Zonder RoomSense-shield: sketch crasht niet, sensoren tonen gewoon NaN/0 met melding
- Shelly-koppeling los te testen zonder sensoren (serial toont `[Shelly] ... -> HTTP ...`)
- mDNS (`.local`) werkt enkel op apparaten/netwerken die het ondersteunen — vrijwel altijd het geval (macOS/iOS ingebouwd via Bonjour, Android en Windows meestal ook), maar als `sjalay.local` ooit niet oplost is het kale IP-adres (Status-pagina) altijd de garantie die werkt

### 5.9 Versiegeschiedenis (kort)

| Versie | Inhoud |
|---|---|
| v0.1–v0.1.1 | Platformlaag (Wi-Fi/AP/NTP/OTA/crash-log/factory-reset) |
| v0.2 | Sensoren: DHT22, DS18B20, LDR1, PIR |
| v0.3 | Verwarmingslogica + relais IO10 |
| v0.4 | Powerpixels (fade, AUTO/manueel, bed-modus) |
| v0.5 | Volledige AJAX-live-UI + compact `/json` |
| v0.6 | UI-stijl, heap-KB-bug gefixt |
| v0.7–v0.7.1 | Pixel-0 herwerking, Google Sheets-logging |
| v0.8 | DS18B20-nicknames/primaire sensor, LDR-drempel instelbaar, duty-cyclus 4u |
| v0.9 | Bugfix bed-modus/pixel-0-interactie zichtbaar in UI |
| v0.10 | Optionele Shelly-stopcontacten per pixel |
| v0.11 | SWW-relais (IO2) + boilersensor-rol + boiler-setpoint (40-60°C) + Auto/Handmatig voor relais 2; echte hysterese voor beide circuits; twee veiligheidslagen altijd actief; alle DS18B20's zichtbaar; sensor pas "ontbrekend" na 3 mislukte lezingen |
| v0.12 | Eenvoudige landingspagina op `/` voor gasten/huurders: grote licht-tegels + kleurkiezer, en voor verwarming/SWW een groot cijfer met de huidige (gemeten) temperatuur plus een kleine "→ X°"-regel eronder met de effectieve/gevraagde doeltemp; geen Auto/Handmatig zichtbaar. Forceert bij elke load beide circuits naar Auto; technische Status-pagina verhuisd naar `/advanced`, bereikbaar via tandwiel-icoon; 🏠-icoon in sidebar van Status/OTA/Settings; pixel 0 schakelt impliciet naar MANUEEL bij aanraking vanop de landingspagina |
| v0.13 | Landingspagina bijgeschaafd na feedback: IST (huidige gemeten temp) en SOLL (effectieve/gevraagde doeltemp) samen op één lijn i.p.v. twee regels — groot IST in de kaartkleur (oranje/blauw), SOLL kleiner tussen haakjes met een spatie ervoor, bv. "21.3° (→ 21°)"; overbodige sectie-iconen boven de verwarmings-/SWW-kaart en het lampje-icoon boven de lichten-tegels verwijderd (dubbelop met de kaart-/tegel-iconen zelf) |
| v0.14 | mDNS/Bonjour toegevoegd (`<ESPmDNS.h>`) — de controller is voortaan ook bereikbaar via `http://<naam>.local/` i.p.v. enkel het kale IP-adres. Naam instelbaar in Settings (nieuw veld "mDNS-naam", default "sjalay"), enkel a-z/0-9/streepjes toegelaten (ongeldige invoer wordt automatisch opgeschoond, terugval "sjalay" bij een lege/volledig ongeldige naam). Start na een geslaagde Wi-Fi-verbinding (niet in AP-setup-modus); een nieuwe naam vereist een herstart, net als de andere Settings-velden. Zichtbaar op `/advanced` (naast IP-adres) en in `/json` ("mdns") |
| v0.15 | Bed-modus/nachtmodus toegevoegd aan de landingspagina: nieuwe ronde knop (🛏️), even groot als de kleurkiezer (52px) en er links naast geplaatst. Voorkomt dat pixel 0 (beweging/schemer) 's nachts automatisch aangaat. Hergebruikt volledig de bestaande bed-variabele/NVS-veld en het `/toggle_bed`-endpoint (al aanwezig sinds v0.9) — geen nieuwe backend-logica. Pixel 0 wordt tijdens bed-modus getoond als vergrendelde tegel (maan-icoon, gedimd, geen tik-actie); de bed-knop licht op zolang bed-modus actief is. Sync via het bestaande `/json`-veld `"bed"` |
| **v0.16** | **Nieuwe ronde knop op de landingspagina om Pixel 0 (MOV1) ook terug naar AUTO te kunnen zetten — voorheen kon je op `/` enkel (impliciet, door de tegel aan te raken) naar MANUEEL schakelen, zonder weg terug. Zelfde vorm/grootte als de bed-knop (52px), er onmiddellijk naast geplaatst. Icoon: 🔄 in AUTO, ✋ in MANUEEL (opgelicht, groen). Hergebruikt volledig het bestaande `/toggle_pixel_mode`-endpoint (al aanwezig sinds v0.2) en het `/json`-veld `"p0m"` — geen nieuwe backend-logica. Getest op het thuisnetwerk (30/09) en bevestigd werkend** |

### 5.10 SWW-regeling, boilersensor en veiligheidsontwerp

**Aanleiding:** naast de bestaande kamersensor (DS18B20, UTP/RoomSense) test je een tweede DS18B20 in de T-bus-stekker uit (voorlopig kelder/ESP-box-temp, evt. later daar vast te solderen) — en er komt een **boilersensor** bij op een verlengdraad, parallel op dezelfde OneWire-bus. Elke DS18B20 heeft een uniek 64-bit ROM-adres, dus nicknames/rollen blijven correct gekoppeld ongeacht hoeveel sensoren er bijkomen.

**Sensorrollen (Settings):**
- **Primaire sensor (kamer)** — bestond al, stuurt relais 1
- **Boilersensor** — apart instelbaar, stuurt relais 2; optie "(geen)" mogelijk
- Sensoren zonder rol (bv. de T-bus/kelder-testsensor) krijgen gewoon geen speciale functie, maar staan wel mee in de "Alle DS18B20-sensoren"-lijst op `/advanced`/`json`

**Boiler-setpoint:** slider (op `/advanced`, en sinds v0.12 ook direct aanpasbaar op de landingspagina), 40-60°C, default 40°C, stappen van 1°C. Stuurt relais 2 automatisch aan (Auto-modus).

**Hysterese (beide circuits):** een echte symmetrische band rond de setpoint. Default **1,0°C** voor verwarming, **5,0°C** voor SWW (grotere boilermassa = trager systeem, minder pompcycli nodig) — beide instelbaar in Settings.

**Terugvalketen bij ontbrekende sensor:**
- **Kamer:** primaire DS18B20 ontbreekt/defect → terugval op DHT22 → als die **ook** defect is: geen betrouwbare data meer → relais 1 geforceerd UIT
- **Boiler:** geen terugval mogelijk (geen alternatieve sensor voor boilerwater) → boilersensor ontbreekt/defect → relais 2 geforceerd UIT
- Detectie pas na **3 opeenvolgende mislukte lezingen** (CRC-fout of buiten geldig bereik), zodat een toevallige glitch niet meteen de verwarming/SWW stilzet

**Twee onafhankelijke veiligheidslagen, voor BEIDE relais, ALTIJD actief (ongeacht Auto/Handmatig):**
1. **Bovengrens-cutoff:** geforceerd UIT zodra de gemeten temperatuur de bovengrens van de bijhorende setpoint-slider bereikt (30°C kamer, 60°C boiler) — voorkomt dat een vergeten "Handmatig AAN" de temperatuur ongelimiteerd laat oplopen
2. **Sensor-betrouwbaarheid-cutoff:** geforceerd UIT bij een ontbrekende/defecte (en niet-terugvalbare) sensor

Beide lagen worden gecontroleerd **na** de Auto/Handmatig-logica en overschrijven die indien nodig.

### 5.11 Eenvoudige landingspagina (nieuw in v0.12, bijgeschaafd in v0.13, bed-modus toegevoegd in v0.15, pixel-0-modusknop toegevoegd in v0.16)

**Aanleiding:** gasten/huurders hebben niets aan Auto/Handmatig-schakelaars, hysterese-instellingen of sensordiagnostiek — enkel licht aan/uit + kleur, en een comfortabele temperatuur voor verwarming/warm water, met zo weinig mogelijk tekst en iconen.

- **`/`** is nu deze landingspagina (`handleLanding()`); de vroegere Status-pagina verhuisde **ongewijzigd** naar **`/advanced`** (`handleStatus()`, enkel het pad veranderde). Bereikbaar vanop de landingspagina via een klein tandwiel-icoon ⚙️ onderaan, zonder tekst.
- **Verlichting:** grote tegels in een grid, één per pixel (inclusief pixel 0/MOV1) — tegelkleur = de ingestelde RGB-kleur wanneer aan, grijs/wit wanneer uit, pixelnaam eronder (geen apart lampje-icoon boven het grid meer sinds v0.13 — dat stond dubbelop met de lampjes op de tegels zelf). Aanraken = direct schakelen (`/toggle_pixel`). Eén kleurkiezer onder het grid voor de gedeelde RGB-kleur van alle pixels (`/setcolor`).
  - Pixel 0 kan nog in AUTO (PIR+donker) staan; aanraken vanop de landingspagina schakelt hem dan automatisch naar MANUEEL (zie `/toggle_pixel`-wijziging in 5.2) — zo werkt de tegel altijd als eenvoudige aan/uit-knop, zonder dat een gast het AUTO/MANUEEL-onderscheid moet kennen. Sinds v0.16 kan je via de nieuwe modusknop (zie hieronder) ook expliciet terug naar AUTO schakelen.
- **Verwarming/SWW:** elke kaart heeft één eigen icoon (🔥/🚿, met een klein statuslampje ernaast voor ketelvraag/pomp actief) — geen los sectie-icoon ernaast (dubbelop met het kaart-icoon). Daaronder, sinds v0.13 op **één lijn** in de kaartkleur (oranje voor verwarming, blauw voor SWW): eerst groot de **huidige, gemeten temperatuur** (`room_temp`/`temp_boiler` — "IST", waar een gast eerst naar kijkt), gevolgd door de **effectieve/gevraagde doeltemperatuur** kleiner tussen haakjes ("SOLL": `effective_setpoint` voor verwarming, `boiler_setpoint` voor SWW), bv. `21.3° (→ 21°)`. Bij een onbetrouwbare sensor toont het IST-cijfer "n.v.t." i.p.v. een onzinnige waarde. Daaronder de setpoint-schuifregelaar. Géén Auto/Handmatig-schakelaar, géén los relais zichtbaar.
- **Veiligheid:** bij **elke** keer dat `/` geladen wordt, worden beide regelkringen geforceerd naar Auto gezet — ook als iemand ze op `/advanced` bewust op Handmatig had gezet. NVS wordt enkel effectief beschreven bij een échte wissel (niet bij elke herlaad/refresh), om onnodige flash-writes te vermijden.
- Geen nieuwe `/json`-velden nodig: de landingspagina hergebruikt exact dezelfde live-refresh-payload (elke 3s) als `/advanced` — `rt`/`bt` voeden het grote IST-cijfer, `heff`/`bsp` het kleine SOLL-stuk tussen haakjes.
- 🏠-icoon toegevoegd aan de sidebar van `/advanced`, `/update` en `/settings` (terug naar `/`) — verschijnt bewust **niet** op de landingspagina zelf.
- **Bed-modus/nachtmodus (nieuw in v0.15):** een ronde knop (🛏️, `&#128719;`), exact even groot als de kleurkiezer (52px), onder het lichten-grid. Voorkomt dat pixel 0 (de bewegings-/schemergestuurde lichtgroep) 's nachts automatisch aangaat — handig als gasten in die kamer slapen. Hergebruikt volledig de bestaande `bed`-variabele/NVS-instelling en het `/toggle_bed`-endpoint (al aanwezig sinds v0.9, voordien enkel bereikbaar via `/advanced`); er is geen nieuwe backend-logica nodig, want `updatePixelLogic()` forceerde pixel 0 al uit tijdens bed-modus. Op de landingspagina wordt pixel 0 tijdens bed-modus getoond als een "vergrendelde" tegel (maan-icoon 🌙, gedimd, geen tik-actie) in plaats van een tegel die een tik toch zou negeren — dat zou verwarrend zijn. De bed-knop zelf licht donkerblauw op zolang bed-modus actief is. Alles synct live mee (elke 3s) via het bestaande `/json`-veld `"bed"` (al aanwezig sinds v0.9), dus ook als bed-modus via `/advanced` wordt aan/uitgezet, past de landingspagina zich automatisch aan.
- **Pixel-0-modusknop AUTO/MANUEEL (nieuw in v0.16):** een tweede ronde knop, exact even groot als de bed-knop (52px), er onmiddellijk naast geplaatst — zo staan de twee gerelateerde pixel-0-knoppen samen in één rij. Toont 🔄 in AUTO, ✋ in MANUEEL (opgelicht, groen zolang MANUEEL actief is — zelfde stijl als de bed-knop, andere kleur). Een tik wisselt tussen de twee via het bestaande `/toggle_pixel_mode`-endpoint (al aanwezig sinds v0.2, voordien enkel op `/advanced`) — geen nieuwe backend-logica. Deze knop is enkel de ontbrekende weg terug naar AUTO vanaf de landingspagina; het bestaande gedrag (een tik op de pixel-0-tegel zelf schakelt nog steeds impliciet naar MANUEEL) blijft ongewijzigd. Live sync via het bestaande `/json`-veld `"p0m"` (al aanwezig). Getest op het thuisnetwerk (30/09) en bevestigd werkend.

### 5.12 mDNS/Bonjour — `http://<naam>.local/` (nieuw in v0.14)

**Aanleiding:** het kale IP-adres (bv. `192.168.50.10`) onthouden of steeds opzoeken is onhandig, zeker voor gasten/huurders die de landingspagina op hun eigen toestel willen bookmarken. Een vaste, leesbare naam lost dat op.

- Gebruikt de standaard `<ESPmDNS.h>`-library uit de ESP32-Arduino-core (geen extra installatie).
- Default naam: **`sjalay`** → `http://sjalay.local/`. Instelbaar in Settings (§5.3), nieuw veld "mDNS-naam", opgeslagen in NVS (`mdns_name`).
- **Validatie (`sanitizeMdnsName()`):** invoer wordt automatisch omgezet naar kleine letters, enkel `a-z`, `0-9` en `-` blijven staan (andere tekens, spaties, punten worden weggegooid), geen leidend/sluitend streepje, en bij een lege of volledig ongeldige invoer valt de naam terug op `"sjalay"` — een wijziging in Settings kan dus nooit een onbruikbare hostnaam opleveren.
- **Opstart:** `MDNS.begin(mdns_name)` + `MDNS.addService("http","tcp",80)` gebeurt in `setup()`, ná een geslaagde Wi-Fi-verbinding (`!ap_mode_active`) — in de AP-setup-modus blijft het vaste `192.168.4.1` het aanspreekpunt, daar voegt mDNS niets toe.
- **Een nieuwe naam vereist een herstart** (net als elk ander Settings-veld — `/save_settings` herstart altijd).
- Zichtbaar op `/advanced` (nieuwe rij "mDNS-naam" naast "IP-adres") en in `/json` als `"mdns"`. Niet apart getoond op de landingspagina (die blijft bewust tekstarm), maar werkt daar uiteraard even goed: `http://sjalay.local/` opent rechtstreeks de landingspagina.
- **Beperking om te weten:** mDNS werkt enkel binnen hetzelfde lokale netwerk (niet via Tailscale/internet) en vereist mDNS-ondersteuning op het toestel dat verbindt — in de praktijk vrijwel altijd aanwezig (macOS/iOS: Bonjour ingebouwd; Android en Windows 10/11: meestal ook), maar bij twijfel blijft het IP-adres op `/advanced` de garantie die altijd werkt.

---

## 6. JSON-velden (`/json`)

Compact schema, gebruikt door live-UI (elke 3s, zowel `/` als `/advanced`) en Google Sheets-log (elke 5min). Booleans als `true`/`false`.

| Veld | Type | Beschrijving |
|---|---|---|
| `rid` | string | Room-naam |
| `ver` | string | Firmwareversie |
| `ip` | string | IP-adres |
| `mdns` | string | mDNS-hostnaam zonder `.local` (v0.14) — volledig adres is `http://<mdns>.local/` |
| `rssi` | int | Wi-Fi signaal (dBm) |
| `heap` | uint | Vrije heap (KB) |
| `lb` | uint | Grootste vrije geheugenblok (KB) |
| `crash` | uint | Aantal geregistreerde crashes |
| `upt` | ulong | Uptime (s) |
| `ap` | bool | AP/setup-modus? |
| `t2`/`t2ok` | float/bool | DHT22-temp + geldigheid |
| `h` | float | DHT22-vochtigheid (%) |
| `dp` | float | Dauwpunt (°C) |
| `t1` | float | DS18B20 primaire (kamer) temp |
| `dsok`/`dsc` | bool/int | DS18B20 gevonden? / aantal |
| `rt`/`rtok` | float/bool | Effectieve room_temp (na terugvalketen) + betrouwbaarheid — `rt` voedt het grote IST-cijfer op de landingspagina |
| `tm` | string | Sensor-waarschuwing (kamer) |
| `ldr` | int | Lichtwaarde 0-100 (100=donker) |
| `mov` | int | PIR-triggers laatste minuut |
| `hauto`/`hsp`/`heff` | bool/int/float | Verwarming auto? / setpoint / effectieve setpoint — `heff` voedt het kleine SOLL-stuk (tussen haakjes) op de landingspagina |
| `rman`/`hon` | bool/bool | Handmatige relais-1-stand / ketelvraag actief |
| `duty` | float | Duty-cyclus verwarming 4u (%) |
| `hcv` | float | Hysteresisband verwarming (°C) |
| `dsb` | int | Index van de boilersensor (-1 = geen) |
| `bt`/`btok` | float/bool | Boilertemperatuur + betrouwbaarheid — `bt` voedt het grote IST-cijfer op de landingspagina |
| `btm` | string | SWW-sensor-waarschuwing |
| `swauto`/`bsp` | bool/int | SWW auto? / boiler-setpoint — `bsp` voedt het kleine SOLL-stuk (tussen haakjes) op de landingspagina |
| `sw2man`/`swon` | bool/bool | Handmatige relais-2-stand / pomp actief |
| `hsww` | float | Hysteresisband SWW (°C) |
| `dsl` | array | Alle gevonden DS18B20's: `[{"n":naam,"t":temp,"ok":bool,"role":"kamer"/"boiler"/""}]` |
| `bed`/`p0m`/`p0on` | bool/int/bool | Bed-modus / pixel-0-modus / pixel-0-staat — `bed` stuurt de vergrendelde weergave van pixel-0-tegel + bed-knop, `p0m` stuurt sinds v0.16 ook het icoon van de nieuwe AUTO/MANUEEL-knop op de landingspagina |
| `pn`/`fd`/`lom` | int | Aantal pixels / fade-snelheid / licht-aan-tijd — `pn` bepaalt hoeveel licht-tegels de landingspagina tekent |
| `pon` | string | Bitstring aan/uit-status per pixel |
| `nr`/`ng`/`nb` | int | Huidige RGB-kleur |
| `gas`/`gcode` | bool/int | Sheets-logging aan? / laatste HTTP-code |

---

## 7. Openstaande punten

- [x] E1 fysiek gelokaliseerd op de ketel (geel klemblokje) + elektrisch bevestigd (0V=gesloten/5V=open)
- [x] HG13 op de BM nagekeken en bevestigd op waarde 1 (vakmancode = 1, uit Montageanleitung; 29/09)
- [x] Vorstbeveiliging bij E1-open bevestigd (Sommerbetrieb = Pumpenstandschutz + Frostschutz actief; 29/09) — praktische wintertest blijft aangeraden
- [ ] **Bestaande brug over E1 verwijderen** vóór het relais aan te sluiten (zie 1.2)
- [ ] Relais-1-test met ESP32 (IO10) op de echte E1-klemmen
- [x] Shelly-stopcontacten vast IP toegekend en gekoppeld via `/settings` (4 stuks, 29/09, meteen werkend)
- [x] **Sketch v0.11 gebouwd:** SWW-relais (IO2), boilersensor-rol, boiler-setpoint, hysterese (beide circuits), veiligheidslagen, alle-DS-sensoren-lijst (29/09)
- [x] **SWW-blokkeerstrategie vastgelegd (30/09):** relais 2 → SF-klem (Speicherfühler, HG24=3), zie 1.4
- [x] Fabriekssensor op SF-klem bevestigd aanwezig + doorgemeten (2,52V bij 49°C, 30/09) — zie 1.4
- [x] HG24 bevestigd bereikbaar op de BM, staat nog op 1 (30/09) — zie 1.4
- [ ] **Omschakeling uitvoeren:** HG24 op 3 zetten, fabriekssensor loskoppelen, relais 2 op de SF-klem aansluiten, ketel herstarten — zie 1.4
- [ ] **Thuis (Zarlardinge):** wandmontagebox inbouwen (shield + relaismodule op plankje, doorvoeropeningen voorboren) — zie 1.5
- [ ] **Thuis (Zarlardinge):** DS18B20 met 3× 10cm draad solderen op de T-bus (kelder-/omgevingssensor, net buiten de box) — zie 1.5
- [ ] **Boilersensor fysiek bedraden** op de verlengdraad (parallel OneWire) en toewijzen in Settings
- [ ] T-bus-testsensor (kelder/ESP-box) evt. definitief vastsolderen indien behouden
- [ ] Na bedrading: hysterese-defaults (1,0°C CV / 5,0°C SWW) in de praktijk evalueren, bijstellen indien nodig
- [ ] Definitieve montage (behuizing in de kelder)
- [x] **Zarlar-Pi omgeschakeld naar volledige subnet-router** (naast de bestaande Funnel) — subnet `192.168.0.0/24` geadverteerd, goedgekeurd en bevestigd bereikbaar vanop afstand (30/09) — zie 3
- [x] **Sketch v0.12 gebouwd:** eenvoudige landingspagina op `/` voor gasten/huurders, Status-pagina verhuisd naar `/advanced` (29/09)
- [x] **Sketch v0.13 gebouwd:** IST+SOLL samen op één lijn in kaartkleur, overbodige lampje-/sectie-iconen weg (29/09)
- [x] **Sketch v0.14 gebouwd:** mDNS/Bonjour (`sjalay.local`, naam instelbaar in Settings) (29/09)
- [x] **Sketch v0.15 gebouwd:** bed-modus/nachtmodus-knop toegevoegd aan de landingspagina (29/09)
- [x] **SSH-toegang tot de Pi bevestigd vanop afstand**, zowel via Tailscale-IP als lokaal IP (30/09) — zie 3.1
- [x] **Lokaal IP van de Pi vastgezet op `192.168.50.2`** via NetworkManager (30/09) — zie 3.1
- [x] **Pi-wachtwoord gewijzigd** naar `fidel2026` via `passwd` (30/09) — zie 3.1
- [x] **Sketch v0.16 gebouwd:** AUTO/MANUEEL-knop voor pixel 0 toegevoegd aan de landingspagina (30/09) — zie 5.11
- [x] **Landingspagina functioneel getest op het thuisnetwerk** (factory reset, volledige RoomSense via RJ45): PIR/AUTO-modus en de nieuwe AUTO/MANUEEL-knop bevestigd werkend (30/09)
- [ ] Landingspagina in de praktijk testen met een echte gast/huurder, UI eventueel verder bijstellen
- [ ] mDNS in de praktijk testen (na flashen): `http://sjalay.local/` openen vanaf een gewone smartphone/laptop op het Sjalay-netwerk
- [ ] *(toekomst, niet urgent)* Stroom-/spanningsmeting per pixel via Shelly's tonen in UI — zie 5.7, nog niet nodig
- [ ] *(toekomst, niet urgent)* Overige Shelly-toestellen (Flood, H&T, …) evalueren als aanvulling — zie 5.7, nog niet nodig
