// ============================================================================
// ESP32-C6_SJALAY_CONTROLLER.ino — v1.0 — 28 sep 2026
// Remote bediening WOLF-ketel (F/CNK/U-25) + SWW-boiler (CB-155) — vakantiehuis
// Sjalay, Recht — via ESP32-C6 + RoomSense shield + 4G (Telenet ONE / Archer MR600)
// Filip Delannoy — Zarlar thuisautomatisering
//
// PARTITIETABEL: Compileer met "partitions.csv" (16 MB) in de schetsmap:
//   nvs (20 KB) / otadata (8 KB) / app0 (6 MB) / app1 (6 MB) / spiffs (~3,9 MB)
// Arduino IDE: Board = ESP32C6 Dev Module, Flash Size = 16MB,
//              Partition Scheme = Custom (partitions.csv), USB CDC On Boot = Enabled
//
// ----------------------------------------------------------------------------
// BOUWSTAPPEN SJALAY-SKETCH (afgeleid van ESP32_C6_MATTER_ROOM + HVAC-sketch,
// sterk vereenvoudigd — zie overnamedossier README.md):
//   [x] STAP 1 — Platformlaag: Wi-Fi STA + AP-fallback/captive portal, NVS-config,
//                NTP, AsyncWebServer, OTA, factory reset (web+serial), crash-log,
//                basis web-UI (Status/OTA/JSON/Settings). GEEN sensoren/relais/pixels.
//   [x] STAP 2 — Sensoren: DHT22, DS18B20 (multi), LDR1, PIR MOV1
//   [x] STAP 3 — Verwarmingslogica + relais IO10 (WOLF E1)
//   [x] STAP 4 — Powerpixels IO4 (fade-engine, AUTO/manueel, bed-modus)
//   [x] STAP 5 — Volledige status-UI + compact /json schema
//   [x] STAP 6 — Google Sheets POST elke 5 min
//   [x] STAP 7 — Settings-uitbreiding + opkuis/review
//   [x] STAP 8 — Optionele Shelly-stopcontacten per pixel (lokale HTTP, geen cloud)
//
// v0.10  (28sep26): Optionele koppeling met Shelly-slimme-stopcontacten per pixel.
//                   Settings: extra veld per pixel "ip:naam,ip:naam" (max 3 Shelly's
//                   per pixel). Bij elke effectieve AAN/UIT-wissel van een pixel
//                   (rand-detectie, niet elke lus) stuurt de sketch een korte lokale
//                   HTTP-GET naar elk gekoppeld IP (http://ip/relay/0?turn=on|off,
//                   compatibel met Shelly Gen1/2/3, geen cloud/account nodig). Op de
//                   statuspagina staat naast elke pixel de naam van de gekoppelde
//                   stopcontacten. Geen retry/terugkoppeling (fire-and-forget, net als
//                   de Sheets-log) — korte timeout (1-1,5s) zodat een onbereikbare
//                   Shelly de lus niet blokkeert.
// v0.9   (27sep26): BUGFIX pixel 0 — geen firmwarebug, maar onzichtbaar gedrag:
//                   bed-modus dwingt pixel 0 altijd uit (ook in MANUEEL, per
//                   ontwerp), maar dat was in de UI niet te zien -> de AAN/UIT-
//                   schakelaar leek "terug te springen" terwijl bed-modus gewoon
//                   nog actief stond van een eerdere test. Nu een aparte rij die
//                   dit expliciet toont zodra bed-modus actief is, met een
//                   snelknop om ze meteen uit te zetten.
// v0.8   (27sep26): STAP 7 afgerond:
//                   - Pixel 0 AAN/UIT-schakelaar herwerkt: gebruikt nu dezelfde
//                     class="cb-pix"/class="pixdot" aanpak als de andere pixels
//                     i.p.v. een apart id-gebaseerd pad — voorheen kon de rij bij
//                     wissel AUTO->MANUEEL onzichtbaar/verward blijven; nu 1 rij
//                     voor MANUEEL (met schakelaar) en 1 statusrij voor AUTO,
//                     beide aangestuurd door dezelfde /json-lus als pixels 1+.
//                   - DS18B20: nicknames en primaire sensor nu echt instelbaar in
//                     Settings (tekstvelden + dropdown), verwerkt in /save_settings.
//                     Placeholder-tekst "volgt in een latere stap" verwijderd.
//                   - LDR donker-drempel nu instelbaar in Settings (was vaste
//                     interne waarde 40).
//                   - Duty-cyclus verwarming: 4u sliding window (12x20min),
//                     percentage in /json ("duty") en Sheets-log, zichtbaar in UI.
//                   - "Nog te bouwen"-blok op statuspagina verwijderd (alles klaar).
// v0.7.1 (26sep26): Compile-fix — HTTPC_STRICT_REDIRECTS bestaat niet in ESP32 core
//                   3.3.12; correcte naam is HTTPC_STRICT_FOLLOW_REDIRECTS.
// v0.7   (26sep26): BUGFIXES — AAN/UIT-labels naast Automatisch/Relais-handmatig/Bed
//                   bleven hangen bij een toggle (ontbrekende id, nu gefixt). Pixel 0
//                   herwerkt: schakelaar wisselt nu tussen AUTO/MANUEEL (was AUTO/AAN);
//                   in MANUEEL verschijnt een aparte AAN/UIT-schakelaar voor pixel 0,
//                   net als bij de andere pixels — voorheen kon pixel 0 in "manueel"
//                   alleen AAN staan. STAP 6 — Google Sheets-logging: compacte JSON-POST
//                   elke 5 min naar een Apps Script-webhook (URL instelbaar, leeg = uit).
// v0.6   (26sep26): UI-stijl nu exact zoals ROOM/HVAC-sketch: witte pagina, gele
//                   header, dunne rode lijn links met witte sidebar + blauwe knoppen
//                   (actief = rood), lichtblauwe waarde-kolom (td.value), 3-koloms
//                   label/waarde/bediening-tabellen. Kleurkiezer past de kleur nu
//                   rechtstreeks toe bij het kiezen (zoals ROOM-sketch), geen aparte
//                   "instellen"-stap meer nodig. BUGFIX: /json gaf de vrije heap en
//                   het grootste blok in bytes door i.p.v. KB (UI toonde ×1024 te
//                   grote waarden) — nu correct gedeeld door 1024.
// v0.5   (26sep26): Volledige live-UI zoals ROOM-sketch — AJAX-schakelaars/sliders
//                   (submitAjax), /json-polling elke 3s (id-based, geen page-reload),
//                   kleurendots op pixels, onafhankelijke klok. Schoonheidsfoutjes
//                   verholpen: kleurkiezer toont/behoudt nu de actuele kleur i.p.v. naar
//                   zwart te springen bij ongewijzigd indienen; setpoint-form zat foutief
//                   rond een <tr> genest (ongeldige HTML) — nu correct binnen de <td>.
// v0.4   (26sep26): Powerpixels IO4 toegevoegd — fade-engine (sin-ease, 1-10s), tot 30
//                   pixels, pixel 0 = AUTO (MOV1+LDR-donker) of manueel AAN, pixels 1+
//                   manueel aan/uit, bed-modus (dwingt pixel 0 uit), RGB-kleurkiezer,
//                   licht-aan-tijd 0-30min. Rechtstreeks overgenomen uit ROOM-sketch,
//                   zonder MOV2 (Sjalay heeft maar 1 PIR).
// v0.3   (26sep26): Verwarmingslogica + relais IO10 (WOLF E1). Fail-safe: relais open
//                   vóór alles bij boot, en blijft UIT tot je hem bewust aanzet. Twee modi:
//                   "Automatisch" (softwarethermostaat, dauwpuntbeveiliging) of "Handmatig"
//                   (default — directe AAN/UIT-schakelaar in UI, voor test zonder sensoren).
//                   LET OP: uitgegaan van actief-laag relaismodule (RELAY_ACTIVE_LOW) —
//                   pas die ene #define aan als jouw module actief-hoog is.
// v0.2   (26sep26): Sensoren toegevoegd — DHT22 (temp+vocht+dauwpunt), DS18B20 multi
//                   (scan/CRC/fallback), LDR1 (0-100, donker=100), PIR MOV1 (trig/min).
//                   Werkt ook zonder aangesloten RoomSense shield: DHT-NaN en DS-count=0
//                   worden gedetecteerd en als "defect/niet gevonden" getoond, geen crash.
// v0.1.1 (26sep26): Fix — AP-SSID "Sjalay-Setup" was verborgen (ssid_hidden niet expliciet
//                   op false gezet). Nu expliciet zichtbaar via volledige softAP()-aanroep.
// v0.1   (26sep26): Initiële platformlaag. Geen Matter, geen TSTAT, geen sensoren.
// ----------------------------------------------------------------------------

// Verplicht voor ESP32-C6 (RISC-V) in Arduino IDE — zonder dit werkt Serial niet correct
#define Serial Serial0

#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <time.h>
#include <Preferences.h>
#include <string.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <DHT.h>
#include <OneWireNg_CurrentPlatform.h>  // C6-compatibel 1-Wire (vervangt OneWire+DallasTemperature)
#include <Adafruit_NeoPixel.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define SJALAY_VERSION "0.10"

// ============== PIN DEFINITIONS (actief) ==============
#define DHT_PIN      6   // IO6  - DHT22 data
#define ONE_WIRE_PIN 3   // IO3  - DS18B20 OneWire
#define LDR_ANALOG   1   // IO1  - LDR1 analog (10k pull-up naar 3V3)
#define PIR_MOV1     5   // IO5  - PIR MOV1 (LOW = beweging)
#define RELAY_PIN   10   // IO10 - Relais -> WOLF ketel E1-ingang
#define PIXEL_PIN    4   // IO4  - NeoPixel / Powerpixels data
// Nog niet actief (volgt in latere stappen):
// IO15 reserve relais

// LET OP: pas dit aan als jouw relaismodule actief-HOOG is (signaal HIGH = relais dicht)
#define RELAY_ACTIVE_LOW true
inline void setRelay(bool on) {
  digitalWrite(RELAY_PIN, on ? (RELAY_ACTIVE_LOW ? LOW : HIGH) : (RELAY_ACTIVE_LOW ? HIGH : LOW));
}

// ============== SENSOR HEALTH THRESHOLDS ==============
#define SENSOR_TEMP_MIN  5.0f   // °C — onder = sensor defect
#define SENSOR_TEMP_MAX 40.0f   // °C — boven = sensor defect
#define SENSOR_HUMI_MIN  10     // %  — onder = sensor defect
#define SENSOR_HUMI_MAX  99     // %  — boven = sensor defect

DHT dht(DHT_PIN, DHT22);
OneWireNg_CurrentPlatform ow(ONE_WIRE_PIN, false);

// DS18B20 multi-sensor — max 4 op 1 bus
#define DS_MAX_SENSORS 4
int ds_count = 0;
OneWireNg::Id ds_addrs[DS_MAX_SENSORS];
float temp_ds_arr[DS_MAX_SENSORS];
char ds_nicknames[DS_MAX_SENSORS][48];
int ds_primary = 0;

// Sensorwaarden
float temp_dht = 0, temp_ds = 0, humi = 0, dew = 0, room_temp = 0;
int light_ldr = 0;
int mov1_triggers = 0;
char temp_melding[48] = "";

// Verwarmingslogica / relais
bool heating_auto = false;        // false = handmatige modus (default, voor test zonder sensoren)
bool relay_manual = false;        // gewenste relaisstaat in handmatige modus
int heating_setpoint = 20;        // gewenste temp in automatische modus (10-30)
float dew_margin = 2.0;           // dauwpunt-veiligheidsmarge (°C)
bool heating_on = false;          // huidige relaisstaat (= ketelvraag)
float effective_setpoint = 20.0;  // laatst berekende effectieve setpoint (voor display)

// ============== DUTY-CYCLUS VERWARMING (4u sliding window, 12x20min) ==============
#define DUTY_SLOTS 12
#define DUTY_WINDOW_MS (20UL * 60UL * 1000UL)  // 20 minuten per slot
float duty_slots[DUTY_SLOTS] = {0};
int duty_slot_count = 0;            // aantal gevulde slots (groeit tot 12)
int duty_slot_idx = 0;              // volgende te schrijven slot (circulair)
unsigned long duty_window_start = 0;
unsigned long duty_on_accum_ms = 0;
unsigned long duty_last_tick = 0;

// ============== GOOGLE SHEETS LOGGING ==============
char gas_url[200] = "";           // Apps Script webhook-URL; leeg = uitgeschakeld
int sheets_last_code = 0;         // laatste HTTP-resultaatcode (0 = nog niet geprobeerd)
unsigned long sheets_last_post = 0;  // millis() van laatste poging

// ============== POWERPIXELS ==============
#define MAX_PIXELS 30
Adafruit_NeoPixel pixels(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);  // lengte wordt in setup() aangepast

char pixel_nicknames[MAX_PIXELS][32];
int pixels_num = 8;                 // configureerbaar via Settings (1-30)
uint8_t neo_r = 255, neo_g = 255, neo_b = 255;
int pixel0_mode = 0;                // 0 = AUTO (PIR+LDR), 1 = MANUEEL
bool pixel0_manual_on = false;      // gewenste pixel 0-staat in MANUEEL-modus
bool pixel_on[MAX_PIXELS] = {false};
int light_on_min = 0;               // licht-aan tijd na PIR-trigger (0-30 min)
bool bed = false;                   // bed-modus: dwingt pixel 0 uit
int LDR_DARK_THRESHOLD = 40;
unsigned long mov1_off_time = 0;
inline unsigned long lightOnDuration() { return (unsigned long)light_on_min * 60000UL + 5000UL; }

// ============== SHELLY-STOPCONTACTEN PER PIXEL (optioneel) ==============
// Elke pixel kan 0-3 Shelly-stopcontacten hebben die simultaan meeschakelen.
// Instelbaar in Settings als "ip:naam,ip:naam" per pixel; hier ontleed naar
// vaste arrays zodat er geen heap-fragmentatie ontstaat tijdens normaal gebruik.
#define MAX_SHELLY_PER_PIXEL 3
#define SHELLY_CFG_LEN 110
char shelly_cfg[MAX_PIXELS][SHELLY_CFG_LEN];                          // ruwe NVS-string, ook getoond in Settings
char shelly_ip[MAX_PIXELS][MAX_SHELLY_PER_PIXEL][16];
char shelly_nick[MAX_PIXELS][MAX_SHELLY_PER_PIXEL][24];
int shelly_count[MAX_PIXELS] = {0};
bool prev_pixel_on[MAX_PIXELS] = {false};   // voor rand-detectie: enkel sturen bij effectieve wissel

// Fade-engine (sin-ease, 1-10s, 20 stappen)
uint8_t currR[MAX_PIXELS], currG[MAX_PIXELS], currB[MAX_PIXELS];
uint8_t targetR[MAX_PIXELS], targetG[MAX_PIXELS], targetB[MAX_PIXELS];
uint8_t startR[MAX_PIXELS], startG[MAX_PIXELS], startB[MAX_PIXELS];
float fade_progress[MAX_PIXELS];
int fade_duration = 2;
const int FADE_NUM_STEPS = 20;
unsigned long fade_interval_ms = 100;
unsigned long lastFadeStep = 0;

// PIR-triggerbuffer (voor trig/min telling)
#define MOV_BUF_SIZE 20
unsigned long mov1Times[MOV_BUF_SIZE] = {0};
int mov1_prev_state = HIGH;  // vorige PIR-staat, voor flankdetectie

Preferences preferences;
AsyncWebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// ============== NVS KEYS ==============
const char* NVS_ROOM_ID    = "room_id";
const char* NVS_WIFI_SSID  = "wifi_ssid";
const char* NVS_WIFI_PASS  = "wifi_password";
const char* NVS_STATIC_IP  = "static_ip";

// ============== RUNTIME CONFIG (char[] i.p.v. String — BSS i.p.v. heap) ==============
char room_id[32]       = "Sjalay";
char wifi_ssid[64]     = "netwerknaam";
char wifi_pass[64]     = "paswoord";
char static_ip_str[20] = "192.168.xx.xx";
char mac_address[20]   = "";

bool ap_mode_active = false;
unsigned long boot_millis = 0;

// Crash-log episode-guard (voorkomt herhaalde logs binnen dezelfde lage-heap periode)
bool crash_logged_this_episode = false;

// ============== HELPER: gedeelde CSS (consistente look doorheen alle pagina's) ==============
void writeSharedCSS(AsyncResponseStream *p) {
  p->print(
    "<style>"
    "body{font-family:Arial,sans-serif;background:#fff;margin:0;padding:0;}"
    ".header{display:flex;background:#ffcc00;color:#000;padding:10px 15px;font-size:18px;font-weight:bold;align-items:center;}"
    ".header-left{flex:1;}.header-right{flex:1;text-align:right;font-size:15px;}"
    ".container{display:flex;min-height:calc(100vh - 44px);}"
    ".sidebar{width:80px;padding:10px 5px;background:#fff;border-right:3px solid #c00;box-sizing:border-box;flex-shrink:0;}"
    ".sidebar a{display:block;background:#369;color:#fff;padding:8px;margin:8px auto;text-decoration:none;font-weight:bold;font-size:12px;border-radius:6px;text-align:center;width:60px;}"
    ".sidebar a:hover{background:#036;}.sidebar a.active{background:#c00;}"
    ".main{flex:1;padding:15px;max-width:600px;overflow-y:auto;}"
    ".group-title{font-size:17px;font-style:italic;font-weight:bold;color:#369;margin:20px 0 8px 0;}"
    "table{width:100%;border-collapse:collapse;margin-bottom:15px;}"
    "td.label{color:#369;font-size:13px;padding:8px 5px;width:38%;border-bottom:1px solid #ddd;vertical-align:middle;}"
    "td.value{background:#e6f0ff;font-size:13px;padding:8px 5px;border-bottom:1px solid #ddd;text-align:center;vertical-align:middle;}"
    "td.control{font-size:13px;padding:8px 5px;border-bottom:1px solid #ddd;text-align:right;vertical-align:middle;}"
    ".slider{width:150px;height:28px;}"
    ".dot{display:inline-block;width:14px;height:14px;border-radius:50%;background:#bbb;vertical-align:middle;}"
    ".switch{position:relative;display:inline-block;width:50px;height:28px;vertical-align:middle;}"
    ".switch input{opacity:0;width:0;height:0;}"
    ".slider-switch{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ccc;transition:.4s;border-radius:28px;}"
    ".slider-switch:before{position:absolute;content:'';height:20px;width:20px;left:4px;bottom:4px;background:#fff;transition:.4s;border-radius:50%;}"
    "input:checked + .slider-switch{background:#369;}"
    "input:checked + .slider-switch:before{transform:translateX(22px);}"
    "input[type=text],input[type=password],input[type=number]{padding:5px;font-size:14px;}"
    "button,.btn{background:#369;color:#fff;border:none;padding:9px 18px;border-radius:6px;"
    "cursor:pointer;font-size:14px;text-decoration:none;display:inline-block;margin:4px 4px 4px 0;}"
    "button:hover,.btn:hover{background:#036;}"
    "button.danger,.btn.danger{background:#c00;}button.danger:hover,.btn.danger:hover{background:#900;}"
    "#status{color:#2a9d2a;font-weight:bold;margin-left:6px;}"
    "@media(max-width:600px){"
    ".container{flex-direction:column;}"
    ".sidebar{width:100%;border-right:none;border-bottom:3px solid #c00;padding:10px 0;display:flex;justify-content:center;box-sizing:border-box;}"
    ".sidebar a{width:70px;margin:0 4px;}"
    ".main{padding:10px;max-width:none;}"
    ".group-title{font-size:16px;margin:15px 0 6px 0;}"
    "td.label{font-size:12px;padding:6px 4px;width:40%;}"
    "td.value{font-size:12px;padding:6px 4px;}"
    "td.control{padding:6px 4px;}"
    ".slider{width:100%;max-width:150px;}"
    "}"
    "</style>"
  );
}

void writeSidebar(AsyncResponseStream *p, const char* active) {
  p->print("<div class=\"sidebar\">");
  p->printf("<a href=\"/\" class=\"%s\">Status</a>", strcmp(active,"status")==0 ? "active":"");
  p->printf("<a href=\"/update\" class=\"%s\">OTA</a>", strcmp(active,"ota")==0 ? "active":"");
  p->print("<a href=\"/json\">JSON</a>");
  p->printf("<a href=\"/settings\" class=\"%s\">Settings</a>", strcmp(active,"settings")==0 ? "active":"");
  p->print("</div>");
}

void writeHeader(AsyncResponseStream *p) {
  unsigned long upt = (millis() - boot_millis) / 1000;
  p->print("<div class=\"header\"><div class=\"header-left\">");
  p->print(room_id);
  p->print("</div><div class=\"header-right\" id=\"hdr-clock\">");
  p->printf("uptime %luu %02lum", upt/3600, (upt%3600)/60);
  p->print(" <span id=\"status\"></span></div></div>");
}

// ============== SENSOR HELPERS ==============
float calculateDewPoint(float t, float h) {
  return isnan(t) || isnan(h) ? 0 : t - ((100 - h) / 5.0);
}

int scaleLDR(int r) { return map(constrain(r, 100, 3800), 100, 3800, 100, 0); }

void pushEvent(unsigned long *buf, int size) {
  for (int i = size - 1; i > 0; i--) buf[i] = buf[i - 1];
  buf[0] = millis();
}

int countRecent(unsigned long *buf, int size) {
  int c = 0;
  unsigned long now = millis();
  for (int i = 0; i < size; i++) if (buf[i] > 0 && now - buf[i] < 60000) c++;
  return c;
}

// ============== DS18B20 MULTI-SENSOR ==============
void scanDS18B20() {
  Serial.println("DS18B20: scanning 1-Wire bus...");
  ds_count = 0;
  for (int i = 0; i < DS_MAX_SENSORS; i++) temp_ds_arr[i] = 0.0;

  OneWireNg::Id id;
  ow.searchReset();
  while (ds_count < DS_MAX_SENSORS) {
    if (ow.search(id) != OneWireNg::EC_SUCCESS) break;
    if (id[0] != 0x28) continue;
    memcpy(ds_addrs[ds_count], id, 8);
    Serial.printf("  Sensor %d: ", ds_count + 1);
    for (int j = 0; j < 8; j++) Serial.printf("%02X ", id[j]);
    Serial.println();
    ds_count++;
  }
  Serial.printf("DS18B20: %d sensor(s) gevonden\n", ds_count);
  if (ds_count == 0) Serial.println("DS18B20: geen sensoren op de bus (RoomSense niet aangesloten?)");

  preferences.begin("sjalay-cfg", false);
  preferences.putInt("ds_count", ds_count);
  for (int i = 0; i < ds_count; i++) {
    char akey[16]; snprintf(akey, sizeof(akey), "ds_addr_%d", i);
    preferences.putBytes(akey, ds_addrs[i], 8);
    char nkey[16]; snprintf(nkey, sizeof(nkey), "ds_nick_%d", i);
    String existing = preferences.getString(nkey, "");
    if (existing.isEmpty()) {
      char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id, i + 1);
      preferences.putString(nkey, defnick);
      strlcpy(ds_nicknames[i], defnick, sizeof(ds_nicknames[i]));
    } else {
      strlcpy(ds_nicknames[i], existing.c_str(), sizeof(ds_nicknames[i]));
    }
  }
  ds_primary = constrain(preferences.getInt("ds_primary", 0), 0, max(ds_count - 1, 0));
  preferences.putInt("ds_primary", ds_primary);
  preferences.end();
}

void loadDS18B20fromNVS() {
  preferences.begin("sjalay-cfg", true);
  ds_count = preferences.getInt("ds_count", 0);
  ds_primary = constrain(preferences.getInt("ds_primary", 0), 0, max(ds_count - 1, 0));
  for (int i = 0; i < ds_count; i++) {
    char akey[16]; snprintf(akey, sizeof(akey), "ds_addr_%d", i);
    preferences.getBytes(akey, ds_addrs[i], 8);
    char nkey[16]; snprintf(nkey, sizeof(nkey), "ds_nick_%d", i);
    char defnick[48]; snprintf(defnick, sizeof(defnick), "%s DS %d", room_id, i + 1);
    String tmp = preferences.getString(nkey, defnick);
    strlcpy(ds_nicknames[i], tmp.c_str(), sizeof(ds_nicknames[i]));
    temp_ds_arr[i] = 0.0;
  }
  preferences.end();
}

void readDS18B20temps() {
  if (ds_count == 0) return;
  ow.reset();
  ow.writeByte(0xCC);  // SKIP ROM - alle sensoren tegelijk
  ow.writeByte(0x44);  // CONVERT T
  delay(750);

  for (int i = 0; i < ds_count; i++) {
    ow.reset();
    ow.writeByte(0x55);  // MATCH ROM
    for (int j = 0; j < 8; j++) ow.writeByte(ds_addrs[i][j]);
    ow.writeByte(0xBE);  // READ SCRATCHPAD

    uint8_t data[9];
    for (int j = 0; j < 9; j++) data[j] = ow.touchByte(0xFF);

    uint8_t crc = OneWireNg::crc8(data, 8);
    if (crc != data[8]) {
      Serial.printf("[DS18B20] CRC fout sensor %d — waarde genegeerd\n", i);
      continue;
    }
    int16_t raw = (int16_t)((data[1] << 8) | data[0]);
    float t = raw / 16.0f;
    if (t >= -55.0f && t <= 125.0f) temp_ds_arr[i] = t;
  }
  temp_ds = temp_ds_arr[ds_primary];
}

void readAllSensors() {
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  readDS18B20temps();
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));

  // Room temp: primair DS18B20, fallback DHT22, anders 0 + melding
  room_temp = temp_ds;
  temp_melding[0] = '\0';
  if (ds_count == 0 || isnan(temp_ds) || temp_ds < SENSOR_TEMP_MIN || temp_ds > SENSOR_TEMP_MAX) {
    room_temp = temp_dht;
    strncpy(temp_melding, ds_count == 0 ? "Geen DS18B20 gevonden - DHT22 gebruikt" : "DS18B20 defect - DHT22 gebruikt", sizeof(temp_melding) - 1);
    if (isnan(temp_dht) || temp_dht < SENSOR_TEMP_MIN || temp_dht > SENSOR_TEMP_MAX) {
      room_temp = 0.0;
      strncpy(temp_melding, "Beide temp-sensoren defect!", sizeof(temp_melding) - 1);
    }
  }

  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE);
}

// ============== VERWARMINGSLOGICA + RELAIS ==============
// Automatisch: softwarethermostaat met dauwpuntbeveiliging (effective = max(setpoint, dew+margin))
// Handmatig:   relais volgt rechtstreeks de UI-schakelaar (voor test zonder werkende sensoren)
void updateHeatingLogic() {
  bool new_state;
  if (heating_auto) {
    effective_setpoint = max((float)heating_setpoint, dew + dew_margin);
    new_state = (room_temp < effective_setpoint - 0.5);
  } else {
    effective_setpoint = heating_setpoint;
    new_state = relay_manual;
  }
  if (new_state != heating_on) {
    heating_on = new_state;
    setRelay(heating_on);
    Serial.printf("[RELAIS] -> %s (modus: %s)\n", heating_on ? "AAN" : "UIT", heating_auto ? "automatisch" : "handmatig");
  }
}

// Duty-cyclus: elke loop-cyclus tijd bijhouden, elke 20 min een sample wegschrijven.
// duty% = gemiddelde "aan"-fractie over de laatste (max 12) samples = laatste 4 uur.
void updateDutyCycle() {
  unsigned long now = millis();
  if (duty_last_tick == 0) { duty_last_tick = now; duty_window_start = now; }
  unsigned long dt = now - duty_last_tick;
  duty_last_tick = now;
  if (heating_on) duty_on_accum_ms += dt;

  if (now - duty_window_start >= DUTY_WINDOW_MS) {
    float frac = (float)duty_on_accum_ms / (float)DUTY_WINDOW_MS;
    if (frac > 1.0f) frac = 1.0f;
    duty_slots[duty_slot_idx] = frac;
    duty_slot_idx = (duty_slot_idx + 1) % DUTY_SLOTS;
    if (duty_slot_count < DUTY_SLOTS) duty_slot_count++;
    duty_on_accum_ms = 0;
    duty_window_start = now;
  }
}

float getDutyPercent() {
  if (duty_slot_count == 0) return 0.0f;
  float sum = 0;
  for (int i = 0; i < duty_slot_count; i++) sum += duty_slots[i];
  return (sum / duty_slot_count) * 100.0f;
}

// ============== FADE-ENGINE (sin-ease, identiek aan ROOM-sketch) ==============
void initFadeEngine() {
  for (int i = 0; i < pixels_num; i++) {
    uint32_t c = pixels.getPixelColor(i);
    currR[i] = (c >> 16) & 0xFF;
    currG[i] = (c >> 8) & 0xFF;
    currB[i] = c & 0xFF;
    targetR[i] = currR[i]; targetG[i] = currG[i]; targetB[i] = currB[i];
    startR[i] = currR[i]; startG[i] = currG[i]; startB[i] = currB[i];
    fade_progress[i] = 1.0f;
  }
}

void setTargetColor(int idx, uint8_t r, uint8_t g, uint8_t b) {
  if (idx < 0 || idx >= pixels_num) return;
  if (targetR[idx] != r || targetG[idx] != g || targetB[idx] != b) {
    targetR[idx] = r; targetG[idx] = g; targetB[idx] = b;
    startR[idx] = currR[idx]; startG[idx] = currG[idx]; startB[idx] = currB[idx];
    fade_progress[idx] = 0.0f;
  }
}

void updateFades() {
  unsigned long now = millis();
  if (now - lastFadeStep < fade_interval_ms) return;
  lastFadeStep = now;
  bool changed = false;

  for (int i = 0; i < pixels_num; i++) {
    if (fade_progress[i] >= 1.0f && currR[i] == targetR[i] && currG[i] == targetG[i] && currB[i] == targetB[i]) continue;

    fade_progress[i] += 1.0f / FADE_NUM_STEPS;
    if (fade_progress[i] > 1.0f) fade_progress[i] = 1.0f;
    float ease = sin(fade_progress[i] * PI / 2.0f);

    currR[i] = constrain(startR[i] + (int)round((targetR[i] - startR[i]) * ease), 0, 255);
    currG[i] = constrain(startG[i] + (int)round((targetG[i] - startG[i]) * ease), 0, 255);
    currB[i] = constrain(startB[i] + (int)round((targetB[i] - startB[i]) * ease), 0, 255);

    if (fade_progress[i] >= 1.0f) { currR[i] = targetR[i]; currG[i] = targetG[i]; currB[i] = targetB[i]; }

    pixels.setPixelColor(i, currR[i], currG[i], currB[i]);
    changed = true;
  }
  if (changed) pixels.show();
}

void updateFadeInterval() {
  fade_duration = constrain(fade_duration, 1, 10);
  fade_interval_ms = (fade_duration * 1000UL) / FADE_NUM_STEPS;
  if (fade_interval_ms < 10) fade_interval_ms = 10;
}

// ============== PIXEL-AANSTURING (elke loop-cyclus) ==============
// Pixel 0: AUTO (PIR MOV1 + donker via LDR) of MANUEEL AAN. Bed-modus dwingt pixel 0 uit.
// Pixel 1+: rechtstreeks manueel aan/uit (pixel_on[]), geen bed-override (enkel de MOV-pixel).
void updatePixelLogic() {
  for (int i = 0; i < pixels_num; i++) {
    if (i == 0) {
      if (bed) {
        setTargetColor(0, 0, 0, 0);
        pixel_on[0] = false;
      } else if (pixel0_mode == 1) {
        bool on = pixel0_manual_on;
        setTargetColor(0, on ? neo_r : 0, on ? neo_g : 0, on ? neo_b : 0);
        pixel_on[0] = on;
      } else {
        bool dark = (light_ldr > LDR_DARK_THRESHOLD);
        bool movement = (millis() < mov1_off_time);
        bool on = dark && movement;
        setTargetColor(0, on ? neo_r : 0, on ? neo_g : 0, on ? neo_b : 0);
        pixel_on[0] = on;
      }
      continue;
    }
    // Normale pixels: rechtstreeks manueel
    if (pixel_on[i]) setTargetColor(i, neo_r, neo_g, neo_b);
    else setTargetColor(i, 0, 0, 0);
  }
}

// ============== SHELLY-KOPPELING: ontleden + versturen ==============
// Ontleedt shelly_cfg[idx] ("ip:naam,ip:naam,...") naar shelly_ip[]/shelly_nick[].
// Gebeurt enkel bij boot en na het opslaan van Settings — niet in de hete lus.
void parseShellyConfig(int idx) {
  shelly_count[idx] = 0;
  if (idx < 0 || idx >= MAX_PIXELS) return;
  char buf[SHELLY_CFG_LEN];
  strlcpy(buf, shelly_cfg[idx], sizeof(buf));
  char *saveptr;
  char *entry = strtok_r(buf, ",", &saveptr);
  while (entry != NULL && shelly_count[idx] < MAX_SHELLY_PER_PIXEL) {
    while (*entry == ' ') entry++;  // spaties na komma negeren
    char *colon = strchr(entry, ':');
    if (colon != NULL && colon != entry) {
      *colon = '\0';
      strlcpy(shelly_ip[idx][shelly_count[idx]], entry, sizeof(shelly_ip[idx][0]));
      strlcpy(shelly_nick[idx][shelly_count[idx]], colon + 1, sizeof(shelly_nick[idx][0]));
      shelly_count[idx]++;
    }
    entry = strtok_r(NULL, ",", &saveptr);
  }
}

void parseAllShellyConfig() {
  for (int i = 0; i < MAX_PIXELS; i++) parseShellyConfig(i);
}

// Simpele lokale HTTP-GET naar de Shelly relay-API (werkt op Gen1 én Gen2/3).
// Korte timeouts zodat een uitgeschakelde/onbereikbare Shelly de loop() niet lang blokkeert.
void sendShellyCommand(const char* ip, bool on) {
  if (strlen(ip) == 0) return;
  WiFiClient client;
  HTTPClient http;
  char url[48];
  snprintf(url, sizeof(url), "http://%s/relay/0?turn=%s", ip, on ? "on" : "off");
  http.setConnectTimeout(1000);
  http.setTimeout(1500);
  if (http.begin(client, url)) {
    int code = http.GET();
    Serial.printf("[Shelly] %s -> HTTP %d\n", url, code);
    http.end();
  } else {
    Serial.printf("[Shelly] http.begin() mislukt voor %s\n", url);
  }
}

void sendShellyForPixel(int idx, bool on) {
  for (int i = 0; i < shelly_count[idx]; i++) sendShellyCommand(shelly_ip[idx][i], on);
}

// Bouwt het label-achtervoegsel voor de statuspagina, bv. " → Keukenlamp, Tafellamp".
void buildShellyLabel(int idx, char *out, size_t outsize) {
  out[0] = '\0';
  if (idx < 0 || idx >= MAX_PIXELS || shelly_count[idx] == 0) return;
  char tmp[128] = "";
  strlcat(tmp, " <span style=\"font-size:10px;color:#888;\">&rarr; ", sizeof(tmp));
  for (int i = 0; i < shelly_count[idx]; i++) {
    if (i > 0) strlcat(tmp, ", ", sizeof(tmp));
    strlcat(tmp, shelly_nick[idx][i], sizeof(tmp));
  }
  strlcat(tmp, "</span>", sizeof(tmp));
  strlcpy(out, tmp, outsize);
}

// ============== GOOGLE SHEETS: POST /json elke 5 min ==============
void postToGoogleSheets() {
  sheets_last_post = millis();
  if (strlen(gas_url) == 0) return;
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[Sheets] geen Wi-Fi, overgeslagen"); return; }

  WiFiClientSecure client;
  client.setInsecure();  // Apps Script gebruikt Google's publieke cert; geen pinning nodig voor dit doel
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Apps Script webhooks doen 302 -> echte URL
  if (!http.begin(client, gas_url)) {
    Serial.println("[Sheets] http.begin() mislukt (ongeldige URL?)");
    sheets_last_code = -1;
    return;
  }
  http.addHeader("Content-Type", "application/json");
  String payload = getJSON();
  sheets_last_code = http.POST(payload);
  http.end();
  Serial.printf("[Sheets] POST -> HTTP %d\n", sheets_last_code);
}
// ============== CRASH-LOG (NVS namespace "crash-log") ==============
void printLastCrashOnBoot() {
  Preferences crashPrefs;
  crashPrefs.begin("crash-log", true);
  char lastCrash[48];
  String tmp = crashPrefs.getString("reason", "geen");
  strlcpy(lastCrash, tmp.c_str(), sizeof(lastCrash));
  uint32_t crashCnt = crashPrefs.getUInt("count", 0);
  crashPrefs.end();
  if (crashCnt > 0) {
    Serial.printf("[BOOT] Vorige crash (#%u): %s\n", crashCnt, lastCrash);
  } else {
    Serial.println("[BOOT] Geen crashes geregistreerd.");
  }
}

void checkCrashLog() {
  uint32_t lb = ESP.getMaxAllocHeap();
  if (lb < 25000 && !crash_logged_this_episode) {
    crash_logged_this_episode = true;
    unsigned long uptime_sec = (millis() - boot_millis) / 1000;
    Preferences crashPrefs;
    crashPrefs.begin("crash-log", false);
    uint32_t cnt = crashPrefs.getUInt("count", 0) + 1;
    crashPrefs.putUInt("count", cnt);
    char reason[48];
    snprintf(reason, sizeof(reason), "heap %uKB @ %lus", lb/1024, uptime_sec);
    crashPrefs.putString("reason", reason);
    crashPrefs.end();
    Serial.printf("[HEAP] Largest block %u KB — crash-log geschreven (#%u)\n", lb/1024, cnt);
  } else if (lb >= 25000) {
    crash_logged_this_episode = false;
  }
}

uint32_t getCrashCount() {
  Preferences crashPrefs;
  crashPrefs.begin("crash-log", true);
  uint32_t cnt = crashPrefs.getUInt("count", 0);
  crashPrefs.end();
  return cnt;
}

// ============== JSON ==============
// Compact schema — controller + sensorvelden. Relais/pixel-velden volgen in latere stappen.
String getJSON() {
  char buf[800];
  unsigned long upt = (millis() - boot_millis) / 1000;
  char tm_esc[48]; strlcpy(tm_esc, temp_melding, sizeof(tm_esc));  // geen quotes/backslashes in melding, dus veilig
  // NaN (sensor niet aangesloten/defect) mag niet in JSON terechtkomen -> naar 0, t2ok/dsok geven de status
  bool t2_ok = !isnan(temp_dht) && !isnan(humi) && temp_dht >= SENSOR_TEMP_MIN && temp_dht <= SENSOR_TEMP_MAX
               && humi >= SENSOR_HUMI_MIN && humi <= SENSOR_HUMI_MAX;
  bool ds_ok = ds_count > 0;
  float t2_j = isnan(temp_dht) ? 0.0f : temp_dht;
  float h_j  = isnan(humi) ? 0.0f : humi;
  char pon[MAX_PIXELS + 1];
  for (int i = 0; i < pixels_num && i < MAX_PIXELS; i++) pon[i] = pixel_on[i] ? '1' : '0';
  pon[pixels_num < MAX_PIXELS ? pixels_num : MAX_PIXELS] = '\0';
  snprintf(buf, sizeof(buf),
    "{\"rid\":\"%s\",\"ver\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,"
    "\"heap\":%u,\"lb\":%u,\"crash\":%u,\"upt\":%lu,\"ap\":%s,"
    "\"t2\":%.1f,\"t2ok\":%s,\"h\":%.1f,\"dp\":%.1f,\"t1\":%.1f,\"dsok\":%s,\"dsc\":%d,\"rt\":%.1f,\"tm\":\"%s\","
    "\"ldr\":%d,\"mov\":%d,"
    "\"hauto\":%s,\"hsp\":%d,\"heff\":%.1f,\"rman\":%s,\"hon\":%s,\"duty\":%.0f,"
    "\"bed\":%s,\"p0m\":%d,\"p0on\":%s,\"pn\":%d,\"fd\":%d,\"lom\":%d,\"pon\":\"%s\",\"nr\":%d,\"ng\":%d,\"nb\":%d,"
    "\"gas\":%s,\"gcode\":%d}",
    room_id, SJALAY_VERSION,
    ap_mode_active ? "192.168.4.1" : WiFi.localIP().toString().c_str(),
    ap_mode_active ? 0 : WiFi.RSSI(),
    (unsigned)(ESP.getFreeHeap()/1024), (unsigned)(ESP.getMaxAllocHeap()/1024),
    (unsigned)getCrashCount(), upt,
    ap_mode_active ? "true" : "false",
    t2_j, t2_ok ? "true" : "false", h_j, dew, temp_ds, ds_ok ? "true" : "false", ds_count, room_temp, tm_esc,
    light_ldr, mov1_triggers,
    heating_auto ? "true" : "false", heating_setpoint, effective_setpoint,
    relay_manual ? "true" : "false", heating_on ? "true" : "false", getDutyPercent(),
    bed ? "true" : "false", pixel0_mode, pixel0_manual_on ? "true" : "false", pixels_num, fade_duration, light_on_min, pon,
    (int)neo_r, (int)neo_g, (int)neo_b,
    strlen(gas_url) > 0 ? "true" : "false", sheets_last_code);
  return String(buf);
}

// ============== NVS: LADEN / OPSLAAN ==============
void loadConfigFromNVS() {
  { String t = preferences.getString(NVS_ROOM_ID, "Sjalay"); strlcpy(room_id, t.c_str(), sizeof(room_id)); }
  { String t = preferences.getString(NVS_WIFI_SSID, "netwerknaam"); strlcpy(wifi_ssid, t.c_str(), sizeof(wifi_ssid)); }
  { String t = preferences.getString(NVS_WIFI_PASS, "paswoord"); strlcpy(wifi_pass, t.c_str(), sizeof(wifi_pass)); }
  { String t = preferences.getString(NVS_STATIC_IP, "192.168.xx.xx"); strlcpy(static_ip_str, t.c_str(), sizeof(static_ip_str)); }
  heating_auto     = preferences.getBool("heat_auto", false);
  relay_manual     = preferences.getBool("relay_man", false);
  heating_setpoint = constrain(preferences.getInt("heat_sp", 20), 10, 30);
  dew_margin       = preferences.getFloat("dew_margin", 2.0);
  LDR_DARK_THRESHOLD = constrain(preferences.getInt("ldr_dark", 40), 0, 100);
  { String t = preferences.getString("gas_url", ""); strlcpy(gas_url, t.c_str(), sizeof(gas_url)); }
}

void loadPixelConfigFromNVS() {
  preferences.begin("sjalay-cfg", true);
  neo_r = preferences.getUChar("neo_r", 255);
  neo_g = preferences.getUChar("neo_g", 255);
  neo_b = preferences.getUChar("neo_b", 255);
  pixels_num = constrain(preferences.getInt("pixels_num", 8), 1, MAX_PIXELS);
  fade_duration = constrain(preferences.getInt("fade_duration", 2), 1, 10);
  light_on_min = constrain(preferences.getInt("light_on_min", 0), 0, 30);
  pixel0_mode = preferences.getInt("pixel_mode_0", 0);
  pixel0_manual_on = preferences.getBool("pixel0_man_on", false);
  bed = preferences.getBool("bed_state", false);
  for (int i = 0; i < pixels_num; i++) {
    char nkey[24]; snprintf(nkey, sizeof(nkey), "pixel_nick_%d", i);
    char defnick[32]; snprintf(defnick, sizeof(defnick), "%s Pixel %d", room_id, i);
    String tmp = preferences.getString(nkey, defnick);
    strlcpy(pixel_nicknames[i], tmp.c_str(), sizeof(pixel_nicknames[i]));
    char okey[24]; snprintf(okey, sizeof(okey), "pixel_on_%d", i);
    pixel_on[i] = preferences.getBool(okey, false);
    prev_pixel_on[i] = pixel_on[i];
    char skey[16]; snprintf(skey, sizeof(skey), "shelly_%d", i);
    String stmp = preferences.getString(skey, "");
    strlcpy(shelly_cfg[i], stmp.c_str(), sizeof(shelly_cfg[i]));
  }
  preferences.end();
  updateFadeInterval();
  parseAllShellyConfig();
}

void factoryResetAndReboot() {
  preferences.begin("sjalay-cfg", false);
  preferences.clear();
  preferences.end();
  delay(300);
  ESP.restart();
}

// ============== WEB-UI: STATUS ==============
void handleStatus(AsyncWebServerRequest *request) {
  AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
  p->print("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Sjalay</title>");
  writeSharedCSS(p);
  p->print("</head><body>");
  writeHeader(p);
  p->print("<div class=\"container\">");
  writeSidebar(p, "status");
  p->print("<div class=\"main\">");

  p->print("<div class=\"group-title\">Controller</div><table>");
  p->printf("<tr><td class=\"label\">IP-adres</td><td class=\"value\" colspan=\"2\">%s</td></tr>",
    ap_mode_active ? "192.168.4.1 (AP-modus)" : WiFi.localIP().toString().c_str());
  p->printf("<tr><td class=\"label\">Wi-Fi RSSI</td><td class=\"value\" id=\"v-rssi\" colspan=\"2\">%d dBm</td></tr>", ap_mode_active ? 0 : WiFi.RSSI());
  p->printf("<tr><td class=\"label\">MAC-adres</td><td class=\"value\" colspan=\"2\">%s</td></tr>", mac_address);
  p->printf("<tr><td class=\"label\">Vrije heap</td><td class=\"value\" id=\"v-heap\" colspan=\"2\">%u KB</td></tr>", (unsigned)(ESP.getFreeHeap()/1024));
  p->printf("<tr><td class=\"label\">Grootste blok</td><td class=\"value\" id=\"v-lb\" colspan=\"2\">%u KB</td></tr>", (unsigned)(ESP.getMaxAllocHeap()/1024));
  p->printf("<tr><td class=\"label\">Crash-teller</td><td class=\"value\" id=\"v-crash\" colspan=\"2\">%u</td></tr>", (unsigned)getCrashCount());
  p->printf("<tr><td class=\"label\">Firmware</td><td class=\"value\" colspan=\"2\">v%s</td></tr>", SJALAY_VERSION);
  p->print("</table>");

  p->print("<div class=\"group-title\">Verwarming / relais (WOLF E1)</div><table>");
  p->printf("<tr><td class=\"label\">Automatische modus</td><td class=\"value\" id=\"v-hauto\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_heating_auto\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-hauto\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    heating_auto ? "AAN" : "UIT", heating_auto ? " checked" : "");

  p->printf("<tr><td class=\"label\">Relais (handmatig)<br><span style=\"font-size:11px;color:#888;\">genegeerd in automatisch</span></td>"
    "<td class=\"value\" id=\"v-rman\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_relay_manual\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-rman\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    relay_manual ? "AAN" : "UIT", relay_manual ? " checked" : "");

  p->printf("<tr><td class=\"label\">Setpoint</td><td class=\"value\" id=\"v-hsp\">%d &deg;C</td>"
    "<td class=\"control\"><form action=\"/set_setpoint\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<input type=\"range\" class=\"slider\" id=\"sl-hsp\" name=\"value\" min=\"10\" max=\"30\" value=\"%d\" onchange=\"submitAjax(this.form);\">"
    "</form></td></tr>", heating_setpoint, heating_setpoint);
  p->printf("<tr><td class=\"label\">Effectieve setpoint</td><td class=\"value\" id=\"v-heff\" colspan=\"2\">%.1f &deg;C</td></tr>", effective_setpoint);
  p->printf("<tr><td class=\"label\">Ketelvraag</td><td class=\"value\" id=\"v-hon\" colspan=\"2\">"
    "<span class=\"dot\" style=\"background:%s\"></span> %s</td></tr>",
    heating_on ? "#e05c00" : "#bbb", heating_on ? "AAN" : "UIT");
  p->printf("<tr><td class=\"label\">Duty-cyclus (laatste 4u)</td><td class=\"value\" id=\"v-duty\" colspan=\"2\">%.0f %%</td></tr>", getDutyPercent());
  p->print("</table>");

  p->print("<div class=\"group-title\">HVAC (sensoren)</div><table>");
  p->printf("<tr><td class=\"label\">DHT22 temp</td><td class=\"value\" id=\"v-t2\" colspan=\"2\">%s</td></tr>",
    isnan(temp_dht) ? "defect (geen sensor?)" : (String(temp_dht, 1) + " &deg;C").c_str());
  p->printf("<tr><td class=\"label\">DHT22 vocht</td><td class=\"value\" id=\"v-h\" colspan=\"2\">%s</td></tr>",
    isnan(humi) ? "defect" : (String(humi, 1) + " %").c_str());
  p->printf("<tr><td class=\"label\">Dauwpunt</td><td class=\"value\" id=\"v-dp\" colspan=\"2\">%.1f &deg;C</td></tr>", dew);
  p->printf("<tr><td class=\"label\">DS18B20 gevonden</td><td class=\"value\" id=\"v-dsc\" colspan=\"2\">%d %s</td></tr>",
    ds_count, ds_count == 0 ? "(RoomSense niet aangesloten?)" : "");
  p->printf("<tr><td class=\"label\">DS18B20 primair (%s)</td><td class=\"value\" id=\"v-t1\" colspan=\"2\">%s</td></tr>",
    ds_count > 0 ? ds_nicknames[ds_primary] : "-", ds_count > 0 ? (String(temp_ds, 1) + " &deg;C").c_str() : "n.v.t.");
  p->printf("<tr><td class=\"label\">Room temp (gebruikt)</td><td class=\"value\" id=\"v-rt\" colspan=\"2\">%.1f &deg;C</td></tr>", room_temp);
  p->printf("<tr><td class=\"label\">Melding</td><td class=\"value\" id=\"v-tm\" colspan=\"2\" style=\"color:#e67e22;\">%s</td></tr>", temp_melding);
  p->print("</table>");

  p->print("<div class=\"group-title\">Verlichting</div><table>");
  p->printf("<tr><td class=\"label\">LDR1 (donker=100)</td><td class=\"value\" id=\"v-ldr\" colspan=\"2\">%d</td></tr>", light_ldr);
  p->printf("<tr><td class=\"label\">Bed-modus<br><span style=\"font-size:11px;color:#888;\">dwingt pixel 0 uit</span></td>"
    "<td class=\"value\" id=\"v-bed\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_bed\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-bed\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    bed ? "AAN" : "UIT", bed ? " checked" : "");
  p->print("</table>");

  char hexcol[8]; snprintf(hexcol, sizeof(hexcol), "#%02x%02x%02x", neo_r, neo_g, neo_b);
  p->print("<div class=\"group-title\">Powerpixels</div><table>");
  p->printf("<tr><td class=\"label\">Kleur</td><td class=\"value\" id=\"rgb_val\">%d, %d, %d</td>"
    "<td class=\"control\"><input type=\"color\" id=\"colorPicker\" value=\"%s\" onchange=\"setNeoColor(this.value);\" "
    "style=\"width:48px;height:34px;border:none;cursor:pointer;padding:2px;\"></td></tr>",
    neo_r, neo_g, neo_b, hexcol);
  p->printf("<tr><td class=\"label\">Dim-snelheid</td><td class=\"value\" id=\"v-fd\">%d s</td>"
    "<td class=\"control\"><form action=\"/set_fade_duration\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<input type=\"range\" class=\"slider\" name=\"value\" min=\"1\" max=\"10\" value=\"%d\" onchange=\"submitAjax(this.form);\">"
    "</form></td></tr>", fade_duration, fade_duration);
  p->printf("<tr><td class=\"label\">Licht-aan tijd</td><td class=\"value\" id=\"v-lom\">%d min</td>"
    "<td class=\"control\"><form action=\"/set_light_on_min\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<input type=\"range\" class=\"slider\" name=\"value\" min=\"0\" max=\"30\" value=\"%d\" onchange=\"submitAjax(this.form);\">"
    "</form></td></tr>", light_on_min, light_on_min);
  p->printf("<tr><td class=\"label\">Pixel 0 (MOV1) modus</td><td class=\"value\" id=\"v-p0m\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_pixel_mode\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-p0m\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    pixel0_mode == 1 ? "MANUEEL" : "AUTO", pixel0_mode == 1 ? " checked" : "");
  // Bed-modus overschrijft pixel 0 altijd (ook in MANUEEL) - dit is een bewuste
  // ontwerpkeuze (zie README sectie F), maar was voorheen onzichtbaar in de UI:
  // een AAN-toggle die genegeerd wordt omdat bed-modus actief staat, leek een bug.
  // Nu een expliciete statusrij + snelknop, zodat dit meteen duidelijk is.
  char shellyLbl[140];
  buildShellyLabel(0, shellyLbl, sizeof(shellyLbl));
  p->printf("<tr id=\"row-p0bed\" style=\"display:%s\"><td class=\"label\">&nbsp;&nbsp;%s%s</td>"
    "<td class=\"value\" colspan=\"2\"><span class=\"dot\" style=\"background:#bbb\"></span> "
    "Bed-modus actief &rarr; geforceerd UIT "
    "<button type=\"button\" style=\"margin-left:6px;padding:2px 8px;font-size:11px;\" "
    "onclick=\"fetch('/toggle_bed').then(()=&gt;updateValues());\">zet bed-modus uit</button></td></tr>",
    bed ? "" : "none", pixel_nicknames[0], shellyLbl);
  // Pixel 0 in MANUEEL (en niet in bed-modus): zelfde class="cb-pix"/class="pixdot"
  // patroon als pixels 1+, zodat de generieke /json-lus hieronder deze rij mee-update.
  p->printf("<tr id=\"row-p0on\" style=\"display:%s\"><td class=\"label\">&nbsp;&nbsp;%s%s</td>"
    "<td class=\"value\"><span class=\"dot pixdot\" data-idx=\"0\" style=\"background:%s\"></span></td>"
    "<td class=\"control\"><form action=\"/toggle_pixel\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<input type=\"hidden\" name=\"idx\" value=\"0\">"
    "<label class=\"switch\"><input type=\"checkbox\" class=\"cb-pix\" data-idx=\"0\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    (!bed && pixel0_mode == 1) ? "" : "none", pixel_nicknames[0], shellyLbl, pixel_on[0] ? hexcol : "#bbb", pixel_on[0] ? " checked" : "");
  // Pixel 0 in AUTO (en niet in bed-modus): alleen statusdot, geen schakelaar
  p->printf("<tr id=\"row-p0auto\" style=\"display:%s\"><td class=\"label\">&nbsp;&nbsp;%s%s</td>"
    "<td class=\"value\" colspan=\"2\"><span class=\"dot pixdot\" data-idx=\"0\" style=\"background:%s\"></span></td></tr>",
    (!bed && pixel0_mode == 0) ? "" : "none", pixel_nicknames[0], shellyLbl, pixel_on[0] ? hexcol : "#bbb");
  for (int i = 1; i < pixels_num; i++) {
    buildShellyLabel(i, shellyLbl, sizeof(shellyLbl));
    p->printf("<tr><td class=\"label\">&nbsp;&nbsp;%s%s</td><td class=\"value\"><span class=\"dot pixdot\" data-idx=\"%d\" style=\"background:%s\"></span></td>"
      "<td class=\"control\"><form action=\"/toggle_pixel\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
      "<input type=\"hidden\" name=\"idx\" value=\"%d\">"
      "<label class=\"switch\"><input type=\"checkbox\" class=\"cb-pix\" data-idx=\"%d\"%s onchange=\"submitAjax(this.form);\">"
      "<span class=\"slider-switch\"></span></label></form></td></tr>",
      pixel_nicknames[i], shellyLbl, i, pixel_on[i] ? hexcol : "#bbb", i, i, pixel_on[i] ? " checked" : "");
  }
  p->print("</table>");

  p->print("<div class=\"group-title\">Beweging</div><table>");
  p->printf("<tr><td class=\"label\">PIR MOV1 trig/min</td><td class=\"value\" id=\"v-mov\" colspan=\"2\">%d</td></tr>", mov1_triggers);
  p->print("</table>");

  p->print("<div class=\"group-title\">Logging</div><table>");
  p->printf("<tr><td class=\"label\">Google Sheets</td><td class=\"value\" id=\"v-gas\" colspan=\"2\">%s</td></tr>",
    strlen(gas_url) == 0 ? "UIT (geen URL ingesteld)" : "AAN (elke 5 min)");
  p->printf("<tr><td class=\"label\">Laatste resultaat</td><td class=\"value\" id=\"v-gcode\" colspan=\"2\">%s</td></tr>",
    sheets_last_post == 0 ? "nog niet geprobeerd" : (sheets_last_code == 200 ? "HTTP 200 (OK)" : String("HTTP " + String(sheets_last_code)).c_str()));
  p->print("</table>");

  // ============== LIVE-REFRESH SCRIPT (loopt in browser, geen ESP32-heap-impact) ==============
  p->print("<script>"
    "function dot(v,col){return '<span class=\"dot\" style=\"background:'+(v?(col||'#2a9d2a'):'#bbb')+'\"></span>';}"
    "function toHex(v){return ('0'+Math.round(v).toString(16)).slice(-2);}"
    "var lastHex='#ffffff';"
    "window.lastUptime=0;"
    "function setNeoColor(hex){"
      "var r=parseInt(hex.slice(1,3),16),g=parseInt(hex.slice(3,5),16),b=parseInt(hex.slice(5,7),16);"
      "var rv=document.getElementById('rgb_val');if(rv)rv.textContent=r+', '+g+', '+b;"
      "lastHex=hex;"
      "fetch('/setcolor?r='+r+'&g='+g+'&b='+b).then(()=>updateValues());}"
    "function updateClock(){"
      "var el=document.getElementById('hdr-clock');if(!el)return;"
      "var n=new Date();"
      "var s=document.getElementById('status');var st=s?s.textContent:'';"
      "el.innerHTML='uptime '+window.lastUptime+' s &nbsp;|&nbsp; '+n.toLocaleDateString('nl-BE')+' '+n.toLocaleTimeString('nl-BE')+' <span id=\"status\">'+st+'</span>';}"
    "function updateValues(){"
      "fetch('/json?'+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(data=>{"
        "window.lastUptime=data.upt;"
        "lastHex='#'+toHex(data.nr)+toHex(data.ng)+toHex(data.nb);"
        "var g=id=>document.getElementById(id);"
        "if(g('v-rssi'))g('v-rssi').textContent=data.rssi+' dBm';"
        "if(g('v-heap'))g('v-heap').textContent=data.heap+' KB';"
        "if(g('v-lb'))g('v-lb').textContent=data.lb+' KB';"
        "if(g('v-crash'))g('v-crash').textContent=data.crash;"
        "if(g('v-t2'))g('v-t2').innerHTML=data.t2ok?data.t2.toFixed(1)+' &deg;C':'defect (geen sensor?)';"
        "if(g('v-h'))g('v-h').innerHTML=data.t2ok?data.h.toFixed(1)+' %':'defect';"
        "if(g('v-dp'))g('v-dp').innerHTML=data.dp.toFixed(1)+' &deg;C';"
        "if(g('v-dsc'))g('v-dsc').textContent=data.dsc+(data.dsc===0?' (RoomSense niet aangesloten?)':'');"
        "if(g('v-t1'))g('v-t1').innerHTML=data.dsok?data.t1.toFixed(1)+' &deg;C':'n.v.t.';"
        "if(g('v-rt'))g('v-rt').innerHTML=data.rt.toFixed(1)+' &deg;C';"
        "if(g('v-tm'))g('v-tm').textContent=data.tm;"
        "if(g('v-ldr'))g('v-ldr').textContent=data.ldr;"
        "if(g('v-mov'))g('v-mov').textContent=data.mov;"
        "if(g('v-gas'))g('v-gas').textContent=data.gas?'AAN (elke 5 min)':'UIT (geen URL ingesteld)';"
        "if(g('v-gcode'))g('v-gcode').textContent=data.gcode===0?'nog niet geprobeerd':('HTTP '+data.gcode+(data.gcode===200?' (OK)':''));"
        "if(g('v-hsp'))g('v-hsp').textContent=data.hsp+' \u00b0C';"
        "if(g('sl-hsp')&&document.activeElement.id!=='sl-hsp')g('sl-hsp').value=data.hsp;"
        "if(g('v-heff'))g('v-heff').innerHTML=data.heff.toFixed(1)+' &deg;C';"
        "if(g('v-hon'))g('v-hon').innerHTML=dot(data.hon,'#e05c00')+' '+(data.hon?'AAN':'UIT');"
        "if(g('v-duty'))g('v-duty').textContent=data.duty.toFixed(0)+' %';"
        "if(g('v-fd'))g('v-fd').textContent=data.fd+' s';"
        "if(g('v-lom'))g('v-lom').textContent=data.lom+' min';"
        "if(g('cb-hauto'))g('cb-hauto').checked=data.hauto;"
        "if(g('v-hauto'))g('v-hauto').textContent=data.hauto?'AAN':'UIT';"
        "if(g('cb-rman'))g('cb-rman').checked=data.rman;"
        "if(g('v-rman'))g('v-rman').textContent=data.rman?'AAN':'UIT';"
        "if(g('cb-bed'))g('cb-bed').checked=data.bed;"
        "if(g('v-bed'))g('v-bed').textContent=data.bed?'AAN':'UIT';"
        "if(g('cb-p0m'))g('cb-p0m').checked=(data.p0m===1);"
        "if(g('v-p0m'))g('v-p0m').textContent=(data.p0m===1)?'MANUEEL':'AUTO';"
        "if(g('row-p0bed'))g('row-p0bed').style.display=data.bed?'':'none';"
        "if(g('row-p0on'))g('row-p0on').style.display=(!data.bed&&data.p0m===1)?'':'none';"
        "if(g('row-p0auto'))g('row-p0auto').style.display=(!data.bed&&data.p0m===0)?'':'none';"
        "if(g('rgb_val'))g('rgb_val').textContent=data.nr+', '+data.ng+', '+data.nb;"
        "if(g('colorPicker')&&document.activeElement.id!=='colorPicker')g('colorPicker').value=lastHex;"
        "document.querySelectorAll('.cb-pix').forEach(cb=>{"
          "var i=parseInt(cb.getAttribute('data-idx'));"
          "cb.checked=(data.pon.charAt(i)==='1');});"
        "document.querySelectorAll('.pixdot').forEach(sp=>{"
          "var i=parseInt(sp.getAttribute('data-idx'));"
          "sp.style.background=(data.pon.charAt(i)==='1')?lastHex:'#bbb';});"
      "}).catch(e=>console.error(e));}"
    "function submitAjax(form){"
      "const p=new URLSearchParams();"
      "for(const el of form.elements){if(el.name)p.append(el.name,el.value);}"
      "const url=p.toString()?form.action+'?'+p.toString():form.action;"
      "fetch(url).then(r=>{if(r.ok){updateValues();const s=document.getElementById('status');"
        "if(s){s.textContent='\\u2713';setTimeout(()=>{if(s)s.textContent='';},1500);}}}).catch(e=>console.error(e));}"
    "document.addEventListener('DOMContentLoaded',function(){"
      "updateValues();setInterval(updateValues,3000);setInterval(updateClock,1000);});"
    "</script>");

  p->print("</div></div></body></html>");
  request->send(p);
}

// ============== WEB-UI: SETTINGS ==============
void handleSettings(AsyncWebServerRequest *request) {
  AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
  p->print("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Sjalay - Settings</title>");
  writeSharedCSS(p);
  p->print("</head><body>");
  writeHeader(p);
  p->print("<div class=\"container\">");
  writeSidebar(p, "settings");
  p->print("<div class=\"main\">");

  p->print("<div class=\"group-title\">Algemeen</div>"
    "<form method=\"GET\" action=\"/save_settings\">"
    "<table>");
  p->printf("<tr><td class=\"label\">Room-naam</td><td class=\"control\"><input type=\"text\" name=\"rid\" value=\"%s\" maxlength=\"31\"></td></tr>", room_id);
  p->printf("<tr><td class=\"label\">Wi-Fi SSID</td><td class=\"control\"><input type=\"text\" name=\"ssid\" value=\"%s\" maxlength=\"63\"></td></tr>", wifi_ssid);
  p->print("<tr><td class=\"label\">Wi-Fi wachtwoord</td><td class=\"control\"><input type=\"password\" name=\"pass\" value=\"\" placeholder=\"(ongewijzigd laten = zelfde)\" maxlength=\"63\"></td></tr>");
  p->printf("<tr><td class=\"label\">Static IP (leeg = DHCP)</td><td class=\"control\"><input type=\"text\" name=\"ip\" value=\"%s\" maxlength=\"19\"></td></tr>", static_ip_str);
  p->printf("<tr><td class=\"label\">Dauwpuntmarge (automatische modus)</td><td class=\"control\"><input type=\"number\" name=\"dew\" step=\"0.5\" min=\"0\" max=\"10\" value=\"%.1f\" style=\"width:60px;\"> &deg;C</td></tr>", dew_margin);
  p->printf("<tr><td class=\"label\">LDR donker-drempel (0-100)</td><td class=\"control\"><input type=\"number\" name=\"ldrdark\" min=\"0\" max=\"100\" value=\"%d\" style=\"width:60px;\"></td></tr>", LDR_DARK_THRESHOLD);
  p->printf("<tr><td class=\"label\">Aantal pixels (1-30, herstart nodig)</td><td class=\"control\"><input type=\"number\" name=\"pnum\" min=\"1\" max=\"30\" value=\"%d\" style=\"width:60px;\"></td></tr>", pixels_num);
  p->printf("<tr><td class=\"label\">Google Script URL<br><span style=\"font-size:11px;color:#888;\">leeg = logging uit</span></td>"
    "<td class=\"control\"><input type=\"text\" name=\"gas\" value=\"%s\" maxlength=\"199\" style=\"width:95%%;\"></td></tr>", gas_url);
  p->printf("<tr><td class=\"label\">MAC-adres</td><td class=\"value\">%s</td></tr>", mac_address);
  p->print("</table>");

  p->print("<div class=\"group-title\">Pixel-namen</div><table>");
  for (int i = 0; i < pixels_num; i++) {
    p->printf("<tr><td class=\"label\">Pixel %d%s</td><td class=\"control\"><input type=\"text\" name=\"pnick%d\" value=\"%s\" maxlength=\"31\"></td></tr>",
      i, i == 0 ? " (MOV1)" : "", i, pixel_nicknames[i]);
    p->printf("<tr><td class=\"label\">&nbsp;&nbsp;Shelly's (ip:naam,ip:naam)</td>"
      "<td class=\"control\"><input type=\"text\" name=\"pshelly%d\" value=\"%s\" maxlength=\"%d\" "
      "placeholder=\"192.168.0.50:Keuken\" style=\"width:95%%;\"></td></tr>",
      i, shelly_cfg[i], SHELLY_CFG_LEN - 1);
  }
  p->print("</table>");

  p->print("<div class=\"group-title\">Sensoren (DS18B20)</div><table>");
  p->printf("<tr><td class=\"label\">DS18B20 gevonden</td><td class=\"control\">%d</td></tr>", ds_count);
  for (int i = 0; i < ds_count; i++) {
    p->printf("<tr><td class=\"label\">Naam (%.1f &deg;C)</td><td class=\"control\"><input type=\"text\" name=\"dsnick%d\" value=\"%s\" maxlength=\"47\"></td></tr>",
      temp_ds_arr[i], i, ds_nicknames[i]);
  }
  if (ds_count > 0) {
    p->print("<tr><td class=\"label\">Primaire sensor</td><td class=\"control\"><select name=\"dsprimary\">");
    for (int i = 0; i < ds_count; i++) {
      p->printf("<option value=\"%d\"%s>%s</option>", i, i == ds_primary ? " selected" : "", ds_nicknames[i]);
    }
    p->print("</select></td></tr>");
  }
  p->print("</table>"
    "<button type=\"submit\">Opslaan &amp; herstart</button>"
    "</form>"
    "<a class=\"btn\" href=\"/rescan_ds\">Herscan DS18B20-bus</a>"
    "<p style=\"font-size:12px;color:#777;\">Herscan navigeert direct weg — sla eerst eventuele wijzigingen hierboven op.</p>");

  p->print("<div class=\"group-title\">Crash-log</div><table>");
  p->printf("<tr><td class=\"label\">Aantal geregistreerd</td><td class=\"value\">%u</td></tr>", (unsigned)getCrashCount());
  p->print("</table><a class=\"btn\" href=\"/clear_crash_log\">Wis crash-log</a>");

  p->print("<div class=\"group-title\">Factory reset</div>"
    "<p style=\"font-size:13px;color:#555;\">Wist alle instellingen (Wi-Fi, room-naam, static IP) en herstart.</p>"
    "<a class=\"btn danger\" href=\"/factory_reset\" onclick=\"return confirm('Zeker? Alle instellingen worden gewist.');\">Factory reset</a>");

  p->print("</div></div></body></html>");
  request->send(p);
}

void handleSaveSettings(AsyncWebServerRequest *request) {
  preferences.begin("sjalay-cfg", false);
  if (request->hasParam("rid")) {
    String v = request->getParam("rid")->value();
    if (v.length() > 0) preferences.putString(NVS_ROOM_ID, v);
  }
  if (request->hasParam("ssid")) {
    String v = request->getParam("ssid")->value();
    if (v.length() > 0) preferences.putString(NVS_WIFI_SSID, v);
  }
  if (request->hasParam("pass")) {
    String v = request->getParam("pass")->value();
    if (v.length() > 0) preferences.putString(NVS_WIFI_PASS, v);
  }
  if (request->hasParam("ip")) {
    String v = request->getParam("ip")->value();
    preferences.putString(NVS_STATIC_IP, v.length() > 0 ? v : String("192.168.xx.xx"));
  }
  if (request->hasParam("dew")) {
    dew_margin = request->getParam("dew")->value().toFloat();
    dew_margin = constrain(dew_margin, 0.0f, 10.0f);
    preferences.putFloat("dew_margin", dew_margin);
  }
  if (request->hasParam("ldrdark")) {
    LDR_DARK_THRESHOLD = constrain(request->getParam("ldrdark")->value().toInt(), 0, 100);
    preferences.putInt("ldr_dark", LDR_DARK_THRESHOLD);
  }
  if (request->hasParam("gas")) {
    String v = request->getParam("gas")->value();
    strlcpy(gas_url, v.c_str(), sizeof(gas_url));
    preferences.putString("gas_url", v);
  }
  if (request->hasParam("pnum")) {
    int newnum = constrain(request->getParam("pnum")->value().toInt(), 1, MAX_PIXELS);
    preferences.putInt("pixels_num", newnum);
  }
  for (int i = 0; i < pixels_num; i++) {
    char pname[16]; snprintf(pname, sizeof(pname), "pnick%d", i);
    if (request->hasParam(pname)) {
      String v = request->getParam(pname)->value();
      if (v.length() > 0) {
        char nkey[24]; snprintf(nkey, sizeof(nkey), "pixel_nick_%d", i);
        preferences.putString(nkey, v);
      }
    }
    // Shelly-veld mag wél leeggemaakt worden (dat betekent: geen koppeling meer) -> altijd opslaan als aanwezig.
    char sname[16]; snprintf(sname, sizeof(sname), "pshelly%d", i);
    if (request->hasParam(sname)) {
      String v = request->getParam(sname)->value();
      char skey[16]; snprintf(skey, sizeof(skey), "shelly_%d", i);
      preferences.putString(skey, v);
    }
  }
  for (int i = 0; i < ds_count; i++) {
    char dname[16]; snprintf(dname, sizeof(dname), "dsnick%d", i);
    if (request->hasParam(dname)) {
      String v = request->getParam(dname)->value();
      if (v.length() > 0) {
        char nkey[16]; snprintf(nkey, sizeof(nkey), "ds_nick_%d", i);
        preferences.putString(nkey, v);
      }
    }
  }
  if (request->hasParam("dsprimary") && ds_count > 0) {
    int p = constrain(request->getParam("dsprimary")->value().toInt(), 0, ds_count - 1);
    preferences.putInt("ds_primary", p);
  }
  preferences.end();
  request->send(200, "text/html",
    "<html><body style='font-family:Arial;text-align:center;padding-top:40px;'>"
    "<h3>Opgeslagen. Herstart...</h3></body></html>");
  delay(300);
  ESP.restart();
}

void handleFactoryReset(AsyncWebServerRequest *request) {
  request->send(200, "text/html",
    "<html><body style='font-family:Arial;text-align:center;padding-top:40px;'>"
    "<h3>Factory reset... herstart</h3></body></html>");
  delay(300);
  factoryResetAndReboot();
}

void handleRescanDS(AsyncWebServerRequest *request) {
  scanDS18B20();
  request->redirect("/settings");
}

void handleToggleHeatingAuto(AsyncWebServerRequest *request) {
  heating_auto = !heating_auto;
  preferences.begin("sjalay-cfg", false);
  preferences.putBool("heat_auto", heating_auto);
  preferences.end();
  updateHeatingLogic();  // relais onmiddellijk aanpassen, niet wachten op volgende cyclus
  request->send(200, "text/plain", "OK");
}

void handleToggleRelayManual(AsyncWebServerRequest *request) {
  relay_manual = !relay_manual;
  preferences.begin("sjalay-cfg", false);
  preferences.putBool("relay_man", relay_manual);
  preferences.end();
  updateHeatingLogic();  // relais onmiddellijk schakelen
  request->send(200, "text/plain", "OK");
}

void handleSetSetpoint(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    heating_setpoint = constrain(request->getParam("value")->value().toInt(), 10, 30);
    preferences.begin("sjalay-cfg", false);
    preferences.putInt("heat_sp", heating_setpoint);
    preferences.end();
    updateHeatingLogic();
  }
  request->send(200, "text/plain", "OK");
}

void handleTogglePixelMode(AsyncWebServerRequest *request) {
  pixel0_mode = (pixel0_mode == 1) ? 0 : 1;
  preferences.begin("sjalay-cfg", false);
  preferences.putInt("pixel_mode_0", pixel0_mode);
  preferences.end();
  updatePixelLogic();
  request->send(200, "text/plain", "OK");
}

void handleTogglePixel(AsyncWebServerRequest *request) {
  if (request->hasParam("idx")) {
    int idx = request->getParam("idx")->value().toInt();
    if (idx == 0) {
      pixel0_manual_on = !pixel0_manual_on;
      preferences.begin("sjalay-cfg", false);
      preferences.putBool("pixel0_man_on", pixel0_manual_on);
      preferences.end();
      updatePixelLogic();
    } else if (idx >= 1 && idx < pixels_num) {
      pixel_on[idx] = !pixel_on[idx];
      preferences.begin("sjalay-cfg", false);
      char okey[24]; snprintf(okey, sizeof(okey), "pixel_on_%d", idx);
      preferences.putBool(okey, pixel_on[idx]);
      preferences.end();
      updatePixelLogic();
    }
  }
  request->send(200, "text/plain", "OK");
}

void handleSetColor(AsyncWebServerRequest *request) {
  if (request->hasParam("r")) neo_r = constrain(request->getParam("r")->value().toInt(), 0, 255);
  if (request->hasParam("g")) neo_g = constrain(request->getParam("g")->value().toInt(), 0, 255);
  if (request->hasParam("b")) neo_b = constrain(request->getParam("b")->value().toInt(), 0, 255);
  preferences.begin("sjalay-cfg", false);
  preferences.putUChar("neo_r", neo_r);
  preferences.putUChar("neo_g", neo_g);
  preferences.putUChar("neo_b", neo_b);
  preferences.end();
  updatePixelLogic();
  request->send(200, "text/plain", "OK");
}

void handleSetFadeDuration(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    fade_duration = constrain(request->getParam("value")->value().toInt(), 1, 10);
    preferences.begin("sjalay-cfg", false);
    preferences.putInt("fade_duration", fade_duration);
    preferences.end();
    updateFadeInterval();
  }
  request->send(200, "text/plain", "OK");
}

void handleSetLightOnMin(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    light_on_min = constrain(request->getParam("value")->value().toInt(), 0, 30);
    preferences.begin("sjalay-cfg", false);
    preferences.putInt("light_on_min", light_on_min);
    preferences.end();
  }
  request->send(200, "text/plain", "OK");
}

void handleToggleBed(AsyncWebServerRequest *request) {
  bed = !bed;
  preferences.begin("sjalay-cfg", false);
  preferences.putBool("bed_state", bed);
  preferences.end();
  updatePixelLogic();
  request->send(200, "text/plain", "OK");
}

void handleClearCrashLog(AsyncWebServerRequest *request) {
  Preferences crashPrefs;
  crashPrefs.begin("crash-log", false);
  crashPrefs.clear();
  crashPrefs.end();
  request->redirect("/settings");
}

// ============== WEB-UI: OTA ==============
void handleOtaPage(AsyncWebServerRequest *request) {
  AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
  p->print("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>Sjalay - OTA</title>");
  writeSharedCSS(p);
  p->print("</head><body>");
  writeHeader(p);
  p->print("<div class=\"container\">");
  writeSidebar(p, "ota");
  p->print("<div class=\"main\">");
  p->print("<div class=\"group-title\">Firmware update (.bin)</div>"
    "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\">"
    "<input type=\"file\" name=\"update\" accept=\".bin\"><br><br>"
    "<button type=\"submit\">Upload &amp; flash</button>"
    "</form>"
    "<p style=\"font-size:13px;color:#555;\">Herstart automatisch na succesvolle upload.</p>");
  p->print("</div></div></body></html>");
  request->send(p);
}

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(1500);
  while (Serial.available()) Serial.read();

  // === FAIL-SAFE: relais UIT vóór alles verder, incl. vóór Wi-Fi/NVS ===
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);
  Serial.println("[RELAIS] Fail-safe: UIT bij boot.");

  printLastCrashOnBoot();

  Serial.println("Commando's: 'R' = NVS reset (binnen 5s na boot)");
  Serial.println("Type 'R' binnen 5 sec voor NVS reset...");
  unsigned long boot_start = millis();
  while (millis() - boot_start < 5000) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'R' || c == 'r') {
        Serial.println("-> NVS reset!");
        preferences.begin("sjalay-cfg", false);
        preferences.clear();
        preferences.end();
        delay(300);
        ESP.restart();
      }
    }
    delay(10);
  }
  Serial.println("(geen reset)");

  preferences.begin("sjalay-cfg", false);
  bool first_boot = preferences.getString(NVS_ROOM_ID, "").isEmpty();
  if (first_boot) {
    Serial.println("*** EERSTE BOOT — DEFAULTS TOEPASSEN ***");
    preferences.putString(NVS_ROOM_ID, "Sjalay");
    preferences.putString(NVS_WIFI_SSID, "netwerknaam");
    preferences.putString(NVS_WIFI_PASS, "paswoord");
    preferences.putString(NVS_STATIC_IP, "192.168.xx.xx");
    preferences.putUChar("neo_r", 255);
    preferences.putUChar("neo_g", 255);
    preferences.putUChar("neo_b", 255);
    preferences.putInt("pixels_num", 8);
    preferences.putInt("fade_duration", 2);
    preferences.putInt("light_on_min", 0);
    preferences.putInt("pixel_mode_0", 0);
    preferences.putBool("bed_state", false);
  }
  loadConfigFromNVS();
  preferences.end();

  boot_millis = millis();

  // === SENSOR-INIT ===
  pinMode(PIR_MOV1, INPUT_PULLUP);  // 3.3V PIR: beweging = LOW
  dht.begin();

  preferences.begin("sjalay-cfg", true);
  int stored_ds_count = preferences.getInt("ds_count", -1);
  preferences.end();
  if (stored_ds_count <= 0) {
    Serial.println("DS18B20: geen sensoren in NVS -> scan uitvoeren...");
    scanDS18B20();
  } else {
    loadDS18B20fromNVS();
    Serial.printf("DS18B20: %d sensor(s) geladen uit NVS\n", ds_count);
  }

  // === POWERPIXELS-INIT ===
  loadPixelConfigFromNVS();
  pixels.updateLength(pixels_num);
  pixels.begin();
  pixels.clear();
  pixels.show();
  initFadeEngine();
  updatePixelLogic();
  Serial.printf("Powerpixels: %d stuks geïnitialiseerd op IO%d\n", pixels_num, PIXEL_PIN);

  updateHeatingLogic();  // pas geladen modus/schakelaar toe op het relais (nog steeds fail-safe UIT als niets gezet was)

  WiFi.mode(WIFI_STA);
  IPAddress local_ip;
  if (local_ip.fromString(static_ip_str)) {
    IPAddress gateway = local_ip; gateway[3] = 1;
    IPAddress subnet(255, 255, 255, 0);
    WiFi.config(local_ip, gateway, subnet, gateway);
    Serial.printf("Static IP: %s (gateway %s)\n", local_ip.toString().c_str(), gateway.toString().c_str());
  } else {
    Serial.println("Geen geldig static IP in NVS -> DHCP");
  }

  Serial.printf("Verbinden met Wi-Fi SSID: %s\n", wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_pass);
  { String t = WiFi.macAddress(); strlcpy(mac_address, t.c_str(), sizeof(mac_address)); }
  Serial.printf("MAC-adres: %s\n", mac_address);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi verbonden!");
    Serial.print("IP-adres: "); Serial.println(WiFi.localIP().toString());
    ap_mode_active = false;
  } else {
    Serial.println("\nWi-Fi mislukt -> Access Point voor configuratie...");
    WiFi.mode(WIFI_AP_STA);
    // Expliciet alle parameters: ssid, password(NULL=open), channel, ssid_hidden(false), max_connection
    WiFi.softAP("Sjalay-Setup", NULL, 1, 0, 4);
    IPAddress ap_ip(192, 168, 4, 1);
    WiFi.softAPConfig(ap_ip, ap_ip, IPAddress(255, 255, 255, 0));
    Serial.println("=== ACCESS POINT GESTART ===");
    Serial.println("SSID: Sjalay-Setup (open)");
    Serial.println("IP: http://192.168.4.1  -> ga naar /settings");
    delay(1000);
    ap_mode_active = true;
    dnsServer.start(DNS_PORT, "*", ap_ip);
    Serial.println("DNS captive portal actief");
  }

  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();
  configTzTime("CET-1CEST,M3.5.0/02,M10.5.0/03", "pool.ntp.org", "time.nist.gov");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");

  server.on("/", HTTP_GET, handleStatus);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/save_settings", HTTP_GET, handleSaveSettings);
  server.on("/factory_reset", HTTP_GET, handleFactoryReset);
  server.on("/clear_crash_log", HTTP_GET, handleClearCrashLog);
  server.on("/rescan_ds", HTTP_GET, handleRescanDS);
  server.on("/toggle_heating_auto", HTTP_GET, handleToggleHeatingAuto);
  server.on("/toggle_relay_manual", HTTP_GET, handleToggleRelayManual);
  server.on("/set_setpoint", HTTP_GET, handleSetSetpoint);
  server.on("/toggle_pixel_mode", HTTP_GET, handleTogglePixelMode);
  server.on("/toggle_pixel", HTTP_GET, handleTogglePixel);
  server.on("/setcolor", HTTP_GET, handleSetColor);
  server.on("/set_fade_duration", HTTP_GET, handleSetFadeDuration);
  server.on("/set_light_on_min", HTTP_GET, handleSetLightOnMin);
  server.on("/toggle_bed", HTTP_GET, handleToggleBed);
  server.on("/capabilities", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *p = request->beginResponseStream("application/json");
    p->print("{\"pixels\":[");
    for (int i = 0; i < pixels_num; i++) {
      if (i > 0) p->print(',');
      p->print('"');
      for (int j = 0; pixel_nicknames[i][j] != '\0' && j < 31; j++) {
        if (pixel_nicknames[i][j] == '"') p->print("\\\"");
        else p->print(pixel_nicknames[i][j]);
      }
      p->print('"');
    }
    p->print("]}");
    request->send(p);
  });
  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getJSON());
  });
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Herstart...");
    delay(300);
    ESP.restart();
  });
  server.on("/update", HTTP_GET, handleOtaPage);
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool ok = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html",
        ok ? "<h2>Update OK — herstart...</h2>" : "<h2 style='color:#f00'>Update mislukt!</h2><a href='/update'>Terug</a>");
      response->addHeader("Connection", "close");
      request->send(response);
      if (ok) { delay(500); ESP.restart(); }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("[OTA] Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      }
      if (Update.write(data, len) != len) Update.printError(Serial);
      if (final) {
        if (Update.end(true)) Serial.printf("[OTA] Klaar: %u bytes\n", (unsigned)(index + len));
        else Update.printError(Serial);
      }
    });

  // Captive portal redirects (alleen relevant in AP-modus)
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect("/settings"); });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect("/settings"); });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect("/settings"); });
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (ap_mode_active) request->redirect("/settings");
    else request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Webserver gestart op poort 80.");
}

// ============== LOOP ==============
void loop() {
  if (ap_mode_active) {
    dnsServer.processNextRequest();
  } else if (WiFi.status() != WL_CONNECTED) {
    static unsigned long last_reconnect_attempt = 0;
    if (millis() - last_reconnect_attempt > 10000) {
      last_reconnect_attempt = millis();
      Serial.println("[WiFi] Verbinding verloren -> reconnect...");
      WiFi.reconnect();
    }
  }

  static unsigned long last_crash_check = 0;
  if (millis() - last_crash_check > 5000) {
    last_crash_check = millis();
    checkCrashLog();
  }

  // PIR continu bewaken (flankdetectie LOW = beweging)
  int mov1_state = digitalRead(PIR_MOV1);
  if (mov1_state == LOW && mov1_prev_state == HIGH) {
    pushEvent(mov1Times, MOV_BUF_SIZE);
    mov1_off_time = millis() + lightOnDuration();
  }
  mov1_prev_state = mov1_state;

  // Powerpixels: logica elke cyclus (voor AUTO-respons), fade-engine elke ~10-500ms
  updatePixelLogic();
  updateFades();

  // Shelly-koppeling: enkel versturen bij een effectieve aan/uit-wissel van een pixel
  // (rand-detectie, goedkoop elke cyclus; de HTTP-call zelf gebeurt dus zelden).
  if (!ap_mode_active) {
    for (int i = 0; i < pixels_num; i++) {
      if (pixel_on[i] != prev_pixel_on[i]) {
        prev_pixel_on[i] = pixel_on[i];
        if (shelly_count[i] > 0) sendShellyForPixel(i, pixel_on[i]);
      }
    }
  }

  // Duty-cyclus verwarming: elke cyclus tijd bijhouden (goedkoop, enkel millis-rekenwerk)
  updateDutyCycle();

  // Trage sensoren (DHT/DS18B20 blokkeert ~750ms) elke 5s, niet in AP-modus setup-flow
  static unsigned long last_sensor_read = 0;
  if (millis() - last_sensor_read > 5000) {
    last_sensor_read = millis();
    readAllSensors();
    updateHeatingLogic();
  }

  // Google Sheets: elke 5 min (HTTPS POST blokkeert kort, ~0.5-2s — aanvaardbaar op deze cadans)
  if (!ap_mode_active && millis() - sheets_last_post > 300000UL) {
    postToGoogleSheets();
  }
}
