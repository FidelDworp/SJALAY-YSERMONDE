# Sjalay (Recht)

Vakantiewoning met WOLF-gasketel en boiler. De controller staat in de kelder naast ketel en boiler; de RoomSense-box (temperatuur, vocht, licht, beweging) hangt in de living.

- **Deel A, voor familie en huurders:** hoe je de verwarming, het warm water en de lampen gebruikt. A1 is bedoeld om door te sturen via WhatsApp.
- **Deel B, voor Filip:** installatie en openstaande punten ter plaatse.

Gemeenschappelijke techniek (sketch, `/json`, Sheet): [TECHNIEK.md](TECHNIEK.md).

---

# Deel A — Gebruik

## A1. Kort bericht voor WhatsApp

```
🏠 Verwarming in de Sjalay

1️⃣ De oude WOLF-thermostaat aan de muur naast de keuken moet ALTIJD AAN blijven staan. Niet uitzetten, niet verdraaien.

2️⃣ Bediening gaat via de app: verbind met het wifi van de Sjalay en open
http://sjalay.local/
(lukt dat niet: http://192.168.50.10/)

3️⃣ 🔥 Verwarming: schuif de balk op de gewenste temperatuur. 20–22 °C is ideaal. Het grote getal is de temperatuur in huis, het getal tussen haakjes is de temperatuur waarop geregeld wordt.

4️⃣ 🚿 Warm water: schuif de balk op de gewenste temperatuur van de boiler. Dit staat los van de verwarming.

5️⃣ 👋 Bij vertrek: zet het warm water minstens een halve dag vooraf op 10 °C. Raak verder niets aan, ook de thermostaat niet.

Werkt iets niet? Kijk of het wifi van de Sjalay aanstaat en of de pagina laadt. Anders: laat het Filip weten.
```

## A2. De thermostaat aan de muur (niet aanraken)

De oude WOLF-thermostaat naast de keuken is het "brein" van de ketel. De verwarming draait alleen als hij aanstaat. Filip heeft hem ingesteld:

- **Linkse draaiknop:** het pijltje links op het display staat naast het **zonnetje**. Op het display staan ook het kraantje (warm water) en het "aan"-symbool.
- **Rechtse draaiknop:** het pijltje rechts staat naast de schaal **+4 … 0 … −4**. Dit is de **maximale kamertemperatuur**: naast **0** is het max. **20 °C**, elke stap naar boven +1 °C (tot +4 = **24 °C**), elke stap naar onder −1 °C (tot −4 = **16 °C**). Aanbevolen: **20–22 °C** (tussen 0 en +2).

## A3. De app (landingspagina)

Open **http://sjalay.local/** met een telefoon, tablet of laptop op het wifi van de Sjalay. De pagina ververst vanzelf.

| Wat je ziet | Wat het doet |
|---|---|
| **Gele balk** bovenaan | Naam van de plek ("Sjalay") |
| **Vierkante tegels met 💡 en een naam** | Eén tegel per lamp of lampgroep. **Tik** om aan of uit te zetten. Aan = tegel in de gekozen kleur, uit = wit. Sommige tegels schakelen ook een stopcontact mee |
| **🌙 gedimde tegel** | De eerste lamp is vergrendeld omdat de bed-modus aanstaat |
| **🛏️ ronde knop** | **Bed-modus** (nachtmodus): de eerste lamp blijft uit. Donkerblauw = aan |
| **🔄 / ✋ ronde knop** | Eerste lamp **automatisch** (🔄: aan bij beweging als het donker is) of **handmatig** (✋, groen). Tik om te wisselen |
| **Kleurcirkel** | Kies de kleur van de lampen |
| **🔥 kaart (verwarming)** | Groot: de **gemeten temperatuur** in huis. Tussen haakjes **(→ 21°)**: de temperatuur waarop geregeld wordt; die kan iets hoger zijn dan jouw keuze om vochtproblemen te voorkomen (normaal). Daaronder de **schuifbalk** om de gewenste temperatuur te kiezen |
| **🚿 kaart (warm water)** | Groot: de **temperatuur van de boiler**. Tussen haakjes het doel. De **schuifbalk** (10–60 °C) kiest de boilertemperatuur, los van de verwarming |
| **Rode cirkel rond 🔥 of 🚿** | De verwarming of de boilerverwarming is **nu actief**. Wit = rust |
| **⚙️ onderaan** | Technische pagina voor Filip, niet nodig voor gebruikers |

**Goed om te weten**
- De verwarming reageert niet onmiddellijk; het huis warmt geleidelijk op.
- Je hoeft nooit "handmatig" te schakelen: de controller regelt automatisch.
- Zonder stroom op de controller (de kast in de kelder) draait de verwarming **niet**. De ketel blijft dan veilig uit; de vorstbeveiliging van de ketel blijft werken.

## A4. Als iets niet werkt

1. Staat het **wifi** van de Sjalay aan en ben je ermee verbonden?
2. Staat de **WOLF-thermostaat** nog aan (display brandt)?
3. Laadt de pagina niet: probeer **http://192.168.50.10/**, wacht even en probeer opnieuw.
4. Blijft het mislukken: laat het Filip weten. Noodoplossing: A5.

## A5. Terugdraaien naar de oude toestand (zonder controller)

Handgeschreven instructie van Filip (25/10/2026), ook in de ketelkast bij de gele en blauwe klemmen. Enkel gebruiken als de controller definitief niet meer werkt.

**1. In de ketelkast**
1. Zet de **230 V-voeding van de verwarming UIT**: grijs bakje op de muur, **rode knop indrukken**.
2. In de kast, boven de gekleurde blokjes, zit een **witte tape** met **2 draadjes en een ijzeren brugje**.
3. Schroef met een kleine **platte (−) schroevendraaier** de draadjes in het **gele** en het **blauwe** blokje los en vervang ze:
   - **gele blokje:** het **brugje** (of een kortsluitdraadje);
   - **blauwe blokje:** het **rode en het witte draadje** (de oorspronkelijke voeler). Polariteit maakt niet uit.
4. Schroef alle **4 schroefjes** weer vast en sluit het deksel met **2 kruisschroeven (+)**.
5. **Stroom verwarming weer aan:** groene knop (grijs bakje).

**2. In de living, aan de WOLF-thermostaat**
1. Druk de **rechtse ronde knop** in: "WEERGAVE".
2. Draai **3 kliks naar rechts**: "VAKMAN". Indrukken: "CODE NR."
3. Draai naar "0", dan **1 klik rechts**: "1". Indrukken: "INSTALLATIE".
4. Draai **1 klik rechts**: "KETEL". Indrukken: "HG00".
5. Draai naar rechts tot "**HG24**" ("3"). Indrukken: "3" knippert.
6. Draai naar links tot "**1**". Indrukken: "1" wordt gekozen.

→ **Nu werkt alles weer zoals oorspronkelijk.**

*Voor Filip:* de controllerdraden in het gele en blauwe blokje zijn twee UTP-paren: **oranje / wit-oranje** op E1 (verwarmingsrelais) en **blauw / wit-blauw** op SF (boilerrelais). Losgekoppelde draden afplakken met isolatietape.

---

# Deel B — Installatie (Filip)

## B1. Ketel en relais

**Toestellen:** WOLF F/CNK/U-25 (SN 168120/1144, 2014, 20–25 kW), boiler CB-155/FB-155/TB-155 (155 l), wandmodule BM 2744329 (eBUS, living). Vakmanniveau: rechtse knop indrukken → VAKMAN → code 1 → Ketel.

- **Verwarming via E1 (relais 1, IO10).** E1 = potentiaalvrije ingang op klemmenstrook X20 (gele klem), parameter **HG13 = 1**: open = verwarming geblokkeerd (pomp- en vorstbeveiliging blijven), gesloten = verwarming volgens de BM. Het relais mag enkel kortsluiten of openen, nooit spanning injecteren (op E1 staat 5 V van de ketel zelf). HG13 = 2 is bewust niet gebruikt: dan zou ook de vorstbeveiliging geblokkeerd zijn bij een open contact.
- **Warm water via SF (relais 2, IO2).** **HG24 = 3**: SF is een extern potentiaalvrij contact (gesloten = SWW-vraag). Klemmen op X20: `AF (grijs) — SF (blauw) — E1 (geel) — eBUS (groen)`. De WOLF-NTC is van SF losgekoppeld (mag in het voelerbuisje blijven); de BM toont dus geen boilertemperatuur meer, de DS18B20 van de controller meet die wel. Na een HG24-wijziging ketel uit en weer aan.
- **In gebruik:** de BM staat permanent op een comfortschema en blijft aan (A2). De controller regelt via E1 en SF.
- **Veiligheid:** nooit STB, maximumthermostaat of drukbeveiliging overbruggen. Relais zijn fail-safe: onbekrachtigd = open = geblokkeerd; ze staan uit vóór de Wi-Fi start, bij stroomuitval en bij een crash. Foto's van de klemmen nemen vóór je iets loskoppelt.
- **Relais 3 (IO19):** NC-contact in de 3,3 V-sensorrail voor een power cycle van DHT22, T-Bus en RoomSense (zie TECHNIEK §6). **Nog niet bedraad** (PCB-snede nodig, zie B4).

## B2. Netwerk, Raspberry Pi en Tailscale

| Onderdeel | Instelling |
|---|---|
| 4G-router | TP-Link Archer MR600 (Telenet ONE data-SIM), `192.168.50.1` |
| Subnet | `192.168.50.0/24`: DHCP-pool `.100–.199`, vaste IP's `.2–.99` (op 28/09 hernummerd van `192.168.1.0/24`, omdat Zarlardinge hetzelfde subnet gebruikte) |
| Vaste IP's | Pi `.2` · controller `.10` · Shelly-stopcontacten `.11–.14` |
| mDNS | `sjalay.local` |

- **Raspberry Pi 3B+** is **Tailscale subnet router** voor `192.168.50.0/24`: het hele Sjalay-netwerk is van overal bereikbaar zonder port forwarding. Setup: Raspberry Pi OS 64-bit, `sudo tailscale up --advertise-routes=192.168.50.0/24 --accept-dns=false`, IP-forwarding aan, route goedgekeurd in de adminconsole (`rpi-fidel-sjalay`). SSH: `ssh pi@192.168.50.2` (headless, geen VNC). Voeding: `vcgencmd get_throttled` = `0x80000` (onschuldig); geen USB-lezer op de Pi gebruiken.
- **Als Tailscale niet verbindt:** na een netwerkwissel Tailscale afsluiten en opnieuw openen. Helpt dat niet op de iPhone: de VPN-configuratie verwijderen en opnieuw toevoegen (incident 23–24/10). Op mobiele data loopt het soms via een relay (DERP) en is het trager.
- **Backup van de Pi** (24/10): `~/Desktop/sjalay-pi-backup.img.gz` op de Mac (10,2 GB, uitgepakt exact 15 552 479 232 bytes). Bewaren tot een reservekaart bewezen opstart (de eerste reservekaart startte niet; opnieuw met een ander merk en Imager-verificatie). Maken (op de Mac, via het lokale netwerk): `ssh pi@192.168.50.2 "echo '<wachtwoord>' | sudo -S dd if=/dev/mmcblk0 bs=4M status=none" | gzip -1 > ~/Desktop/sjalay-pi-backup.img.gz` (wachtwoord van de Pi: niet in deze publieke repo). Terugzetten met Raspberry Pi Imager ("Eigen image gebruiken").

## B3. Voeding van de controller

- **Nu:** VOLTCRAFT USPS-600 op **5 V** (max. 0,6 A), 2-polige SM-connector (rood +, zwart −), lange dunne kabel. Dat is krap voor ESP32 (Wi-Fi-pieken) + relais + sensoren.
- **Meebrengen:** vaste, geregelde **5 V, minstens 2 A**, korte dikke kabel (≤ 0,5 m), passende SM-stekker; polariteit meten vóór aansluiten. De ELIX-voeding (3–12 V, 800 mA) niet gebruiken zonder meting (niet gestabiliseerd, 4,5 V te laag, 6 V te hoog).
- **Shield v2.0:** twee 500 mA-PTC's (3V3- en 5V-baan). De relaisvoeding op de OPTION-connector loopt achter de 5V-PTC.
- **Shelly Plug** op de voeding van de controller (beslist 11/11): echte power cycle van op afstand, o.a. om een vastgelopen DHT22 te resetten.

## B4. Openstaande punten (ter plaatse)

**DHT22 dood sinds 26/10/2026 02:07.** Hij komt niet terug na een softwareherstart, enkel na een echte power cycle. Zonder vochtmeting is de dauwpuntbeveiliging niet actief (de regeling gebruikt dan het setpoint); de kamertemperatuur komt van de DS18B20. De sensor zelf is niet verdacht (dezelfde DHT22 werkt elders maanden zonder fout): de oorzaak zit in de opstelling. Vermoeden: de 3,3 V zakt weg door een zwakke voeding of het voedingspad (lange kabel, SM-connector, 5V-PTC). Relaisactiviteit is uitgesloten (geen relais actief bij de uitval).

Volgend bezoek, één ding tegelijk, daarna telkens enkele dagen kijken:
- [ ] **Shelly Plug** op de voeding van de controller (Wi-Fi op de 4G-router) → meteen een power cycle: komt de DHT22 terug?
- [ ] **Meten** (multimeter, mini-scoop single shot, trigger onder ± 3,2 V): 5 V aan de devkit, vóór en ná de 5V-PTC, 3,3 V aan de DHT-pin, onder normale belasting (Wi-Fi, relais aan/uit).
- [ ] **Zwaardere voeding** plaatsen (B3).
- [ ] Pas daarna, als noodoplossing: **PCB-snede + relais 3** (snede in de 3V3-baan tussen het UART-3V3-pad en het 3v3-pinnetje; COM aan de UART-kant, NC aan het 3v3-pinnetje; herstelbaar maken en nameten). Daarna `/dht_reset` testen.
- [ ] **CV AAN-kant testen** (setpoint boven de kamertemperatuur).
- [ ] **SCD41** meebrengen en op de I2C-header aansluiten (SDA–SCL–GND–3V3, nooit 5 V, 10–20 cm van de controller); code pas in v1.16.
- [ ] **2 extra DS18B20's** (ketelpomp, retour) op de T-Bus: waterdicht, verbindingsprintje, isolatiestrips.
- [ ] Bedrading afwerken (kabelgootjes), isolatie onderaan de boiler bijwerken, hysterese-defaults evalueren (1,0 °C CV, 5,0 °C SWW).

**Later:** landingspagina testen met een echte gast; wintertest van de vorstbeveiliging; eventueel Shelly Flood (waterlek) en H&T.
