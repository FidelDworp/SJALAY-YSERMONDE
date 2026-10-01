// ============================================================================
// SJALAY CONTROLLER — v1.2 — 1 okt 2026
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
// v1.0 (1okt26): Eerste opgekuiste release — volledige versiegeschiedenis (v0.1-v0.16)
//                en de bouwstappen-checklist uit deze header verwijderd; zie README.md
//                voor de volledige ontwikkelgeschiedenis. Wijzigingen t.o.v. v0.16:
//                - Controller-sectie verhuisd naar helemaal onderaan de statuspagina.
//                - SWW-sectie hernoemd naar "SWW boiler"; setpoint-bereik 10-60°C
//                  (was 40-60°C), zowel op /advanced als op de landingspagina.
//                - HVAC-sectie volledig opgesplitst en opgeheven: DHT22-/DS18B20-
//                  kamerdata verhuisd naar de Verwarmingssectie, incl. een nieuwe
//                  samengevoegde "Kamertemperatuur (bron: ...)"-regel (i.p.v. de twee
//                  losse regels "DS18B20 primair" + "Room temp") en een "Effectieve
//                  setpoint met % vochtigheid"-regel; de Melding-regel toont voortaan
//                  altijd een statuszin i.p.v. enkel bij een fout. "DS18B20 gevonden"
//                  verhuisd naar "Alle DS18B20-sensoren".
//                - Verlichting samengevoegd met Powerpixels tot 1 sectie; "LDR1
//                  (donker=100)" hernoemd naar "OMGEVINGSLICHT (donker=100)".
//                - Google Sheets-interval nu instelbaar in Settings (was vast 5 min);
//                  het PIR-telvenster en de labels ("trig/X min") volgen die waarde.
// ----------------------------------------------------------------------------
// v1.1 (1okt26): - Verwarmings-setpoint-slider: minimum 5°C (was 10°C), op /advanced
//                  en de landingspagina.
//                - Sectietitel "Verwarming / relais (WOLF E1)" vereenvoudigd naar
//                  "Verwarming"; "Powerpixels" hernoemd naar "Verlichting".
//                - /settings: volgorde "Herscan DS18B20-bus" en "Opslaan & herstart"
//                  omgewisseld.
//                - Landingspagina: vlam-/douche-icoon (verwarming/SWW) nu in een
//                  statuscirkel (wit = rust, lichtrood/rode rand = ketelvraag/pomp
//                  actief) i.p.v. een los kleurbolletje naast het icoon.
// ----------------------------------------------------------------------------
// v1.2 (1okt26): - /json volledig herwerkt naar een compact schema met positionele
//                  sleutels (a, b, c, ... + "room"), in dezelfde stijl als het
//                  Zarlar-roomproject — i.p.v. de lange beschrijvende sleutels.
//                  Dit ene schema voedt zowel de live-UI (/advanced + landingspagina)
//                  als de Google Sheets-log.
//                - Bewust weggelaten uit de live-JSON (blijven wel zichtbaar bij het
//                  laden van de pagina): betrouwbaarheids-/fallback-vlaggen, tekstuele
//                  meldingen, IP/mDNS/crash-teller/firmwareversie, Auto/Handmatig-
//                  togglestatus, hysterese-waarden, per-sensor-detail van niet-
//                  toegewezen DS18B20's, Sheets-/PIR-intervaltekst en duty-cyclus.
//                - Nieuw Apps Script (zie README) hoort bij dit schema — het vorige
//                  script (lange sleutelnamen) is niet langer compatibel.
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
#include <ESPmDNS.h>

#define SJALAY_VERSION "1.2"

// ============== PIN DEFINITIONS (actief) ==============
#define DHT_PIN      6   // IO6  - DHT22 data
#define ONE_WIRE_PIN 3   // IO3  - DS18B20 OneWire
#define LDR_ANALOG   1   // IO1  - LDR1 analog (10k pull-up naar 3V3)
#define PIR_MOV1     5   // IO5  - PIR MOV1 (LOW = beweging)
#define RELAY_PIN   10   // IO10 - Relais 1 -> WOLF ketel E1-ingang
#define RELAY2_PIN   2   // IO2  - Relais 2 -> SWW-laadpomp (230V, via 4-relaismodule)
#define PIXEL_PIN    4   // IO4  - NeoPixel / Powerpixels data
// IO15 blijft vrije reserve (was hiervoor gepland voor relais 2, IO2 gekozen i.p.v.
// omwille van fysieke bedradingsgemak naast IO10 op de shield).

// LET OP: pas dit aan als jouw relaismodule actief-HOOG is (signaal HIGH = relais dicht)
#define RELAY_ACTIVE_LOW true
inline void setRelay(bool on) {
  digitalWrite(RELAY_PIN, on ? (RELAY_ACTIVE_LOW ? LOW : HIGH) : (RELAY_ACTIVE_LOW ? HIGH : LOW));
}
inline void setRelay2(bool on) {
  digitalWrite(RELAY2_PIN, on ? (RELAY_ACTIVE_LOW ? LOW : HIGH) : (RELAY_ACTIVE_LOW ? HIGH : LOW));
}

// ============== SENSOR HEALTH THRESHOLDS ==============
#define SENSOR_TEMP_MIN  5.0f   // °C — onder = sensor defect (kamer)
#define SENSOR_TEMP_MAX 40.0f   // °C — boven = sensor defect (kamer)
#define SENSOR_HUMI_MIN  10     // %  — onder = sensor defect
#define SENSOR_HUMI_MAX  99     // %  — boven = sensor defect
#define SENSOR_BOILER_MIN 0.0f  // °C — onder = boilersensor defect (ander bereik dan kamer!)
#define SENSOR_BOILER_MAX 90.0f // °C — boven = boilersensor defect
#define DS_FAIL_THRESHOLD 3     // opeenvolgende mislukte lezingen vóór "sensor ontbreekt"
#define HEATING_SETPOINT_MAX 30 // bovengrens verwarmings-slider = harde veiligheidsgrens
#define BOILER_SETPOINT_MAX  60 // bovengrens SWW-slider = harde veiligheidsgrens

DHT dht(DHT_PIN, DHT22);
OneWireNg_CurrentPlatform ow(ONE_WIRE_PIN, false);

// DS18B20 multi-sensor — max 4 op 1 bus
#define DS_MAX_SENSORS 4
int ds_count = 0;
OneWireNg::Id ds_addrs[DS_MAX_SENSORS];
float temp_ds_arr[DS_MAX_SENSORS];
char ds_nicknames[DS_MAX_SENSORS][48];
int ds_primary = 0;
int ds_boiler = -1;                          // index van de boilersensor, -1 = geen toegewezen
bool ds_valid_this_read[DS_MAX_SENSORS];     // laatste-lezing geldig? (voor UI/json)
int ds_fail_streak[DS_MAX_SENSORS] = {0};    // opeenvolgende mislukte lezingen per sensor

// Sensorwaarden
float temp_dht = 0, temp_ds = 0, humi = 0, dew = 0, room_temp = 0;
float temp_boiler = 0;
int light_ldr = 0;
int mov1_triggers = 0;
char temp_melding[48] = "";
char room_source[40] = "";         // naam van de sensor die room_temp momenteel levert (DS-naam of "DHT22 (fallback)")
char boiler_melding[48] = "";
bool room_temp_reliable = true;    // false = kamer-DS én DHT22 beide defect -> verwarming geblokkeerd
bool room_temp_fallback = false;   // true = DS18B20 (kamer) uitgevallen, DHT22 neemt het over
bool boiler_temp_reliable = true;  // false = geen bruikbare boilersensor -> SWW-pomp geblokkeerd

// Verwarmingslogica / relais 1 (WOLF E1)
bool heating_auto = false;        // false = handmatige modus (default, voor test zonder sensoren)
bool relay_manual = false;        // gewenste relaisstaat in handmatige modus
int heating_setpoint = 20;        // gewenste temp in automatische modus (5-30)
float dew_margin = 2.0;           // dauwpunt-veiligheidsmarge (°C)
bool heating_on = false;          // huidige relaisstaat (= ketelvraag)
float effective_setpoint = 20.0;  // laatst berekende effectieve setpoint (voor display)
float hyst_cv = 1.0;              // hysteresisband verwarming (°C, symmetrisch rond setpoint)

// SWW-logica / relais 2 (laadpomp)
bool sww_auto = false;            // false = handmatige modus (default)
bool relay2_manual = false;       // gewenste relais2-staat in handmatige modus
int boiler_setpoint = 40;         // gewenste boilertemp (40-60)
bool sww_on = false;              // huidige relais2-staat
float hyst_sww = 5.0;             // hysteresisband SWW (°C, symmetrisch rond setpoint)

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
int sheets_interval_min = 5;      // instelbaar in Settings (1-60 min), default 5 — PIR-telvenster volgt deze waarde
inline unsigned long sheetsIntervalMs() { return (unsigned long)sheets_interval_min * 60000UL; }

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

// PIR-triggerbuffer (voor trig/X-min telling, venster = sheets_interval_min).
// Vergroot t.o.v. v0.16 (was 20) zodat ook bij een langer interval en frequente
// beweging de teller niet ondertelt doordat oudere triggers al verdrongen zijn.
#define MOV_BUF_SIZE 60
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
char mdns_name[32]     = "sjalay";  // -> http://sjalay.local/, instelbaar in Settings
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
  p->print("<a href=\"/\" style=\"font-size:20px;padding:6px;\" title=\"Eenvoudige weergave\">&#127968;</a>");
  p->printf("<a href=\"/advanced\" class=\"%s\">Status</a>", strcmp(active,"status")==0 ? "active":"");
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

int countRecent(unsigned long *buf, int size, unsigned long windowMs) {
  int c = 0;
  unsigned long now = millis();
  for (int i = 0; i < size; i++) if (buf[i] > 0 && now - buf[i] < windowMs) c++;
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
  ds_boiler = preferences.getInt("ds_boiler", -1);
  if (ds_boiler >= ds_count) ds_boiler = -1;  // ongeldig geworden na herscan (minder sensoren) -> resetten
  preferences.putInt("ds_boiler", ds_boiler);
  for (int i = 0; i < DS_MAX_SENSORS; i++) ds_fail_streak[i] = 0;
  preferences.end();
}

void loadDS18B20fromNVS() {
  preferences.begin("sjalay-cfg", true);
  ds_count = preferences.getInt("ds_count", 0);
  ds_primary = constrain(preferences.getInt("ds_primary", 0), 0, max(ds_count - 1, 0));
  ds_boiler = preferences.getInt("ds_boiler", -1);
  if (ds_boiler >= ds_count) ds_boiler = -1;
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
    ds_valid_this_read[i] = false;
    ow.reset();
    ow.writeByte(0x55);  // MATCH ROM
    for (int j = 0; j < 8; j++) ow.writeByte(ds_addrs[i][j]);
    ow.writeByte(0xBE);  // READ SCRATCHPAD

    uint8_t data[9];
    for (int j = 0; j < 9; j++) data[j] = ow.touchByte(0xFF);

    uint8_t crc = OneWireNg::crc8(data, 8);
    if (crc != data[8]) {
      Serial.printf("[DS18B20] CRC fout sensor %d — waarde genegeerd\n", i);
      if (ds_fail_streak[i] < 255) ds_fail_streak[i]++;
      continue;
    }
    int16_t raw = (int16_t)((data[1] << 8) | data[0]);
    float t = raw / 16.0f;
    if (t >= -55.0f && t <= 125.0f) {
      temp_ds_arr[i] = t;
      ds_valid_this_read[i] = true;
      ds_fail_streak[i] = 0;
    } else if (ds_fail_streak[i] < 255) {
      ds_fail_streak[i]++;
    }
  }
  temp_ds     = (ds_primary >= 0 && ds_primary < ds_count) ? temp_ds_arr[ds_primary] : 0.0f;
  temp_boiler = (ds_boiler  >= 0 && ds_boiler  < ds_count) ? temp_ds_arr[ds_boiler]  : 0.0f;
}

void readAllSensors() {
  humi = dht.readHumidity();
  temp_dht = dht.readTemperature();
  dew = calculateDewPoint(temp_dht, humi);
  readDS18B20temps();
  light_ldr = scaleLDR(analogRead(LDR_ANALOG));

  // --- Kamertemperatuur: primaire DS18B20 -> terugval DHT22 -> anders onbetrouwbaar ---
  bool primary_missing = (ds_count == 0) || (ds_primary < 0) || (ds_primary >= ds_count)
                          || (ds_fail_streak[ds_primary] >= DS_FAIL_THRESHOLD)
                          || (temp_ds < SENSOR_TEMP_MIN) || (temp_ds > SENSOR_TEMP_MAX);
  room_temp = temp_ds;
  temp_melding[0] = '\0';
  room_source[0] = '\0';
  room_temp_reliable = true;
  room_temp_fallback = primary_missing;
  if (primary_missing) {
    room_temp = temp_dht;
    strncpy(room_source, "DHT22 (fallback)", sizeof(room_source) - 1);
    strncpy(temp_melding, ds_count == 0 ? "Geen DS18B20 gevonden - DHT22 actief (fallback)" : "DS18B20 (kamer) defect - DHT22 actief (fallback)", sizeof(temp_melding) - 1);
    if (isnan(temp_dht) || temp_dht < SENSOR_TEMP_MIN || temp_dht > SENSOR_TEMP_MAX) {
      room_temp = 0.0;
      room_temp_reliable = false;
      strncpy(room_source, "geen betrouwbare sensor", sizeof(room_source) - 1);
      strncpy(temp_melding, "Beide temp-sensoren defect - verwarming geblokkeerd!", sizeof(temp_melding) - 1);
    }
  } else {
    strncpy(room_source, ds_nicknames[ds_primary], sizeof(room_source) - 1);
    strncpy(temp_melding, "DS18B20 (kamer) actief", sizeof(temp_melding) - 1);
  }

  // --- Boilertemperatuur: enkel de toegewezen DS-sensor, GEEN terugval mogelijk ---
  boiler_melding[0] = '\0';
  boiler_temp_reliable = true;
  bool boiler_missing = (ds_boiler < 0) || (ds_boiler >= ds_count)
                         || (ds_fail_streak[ds_boiler] >= DS_FAIL_THRESHOLD)
                         || (temp_boiler < SENSOR_BOILER_MIN) || (temp_boiler > SENSOR_BOILER_MAX);
  if (boiler_missing) {
    boiler_temp_reliable = false;
    strncpy(boiler_melding, ds_boiler < 0 ? "Geen boilersensor toegewezen - SWW-pomp geblokkeerd" : "Boilersensor defect/ontbreekt - SWW-pomp geblokkeerd", sizeof(boiler_melding) - 1);
  }

  mov1_triggers = countRecent(mov1Times, MOV_BUF_SIZE, sheetsIntervalMs());
}

// ============== VERWARMINGSLOGICA + RELAIS 1 (WOLF E1) ==============
// Automatisch: softwarethermostaat met dauwpuntbeveiliging (effective = max(setpoint, dew+margin))
//              en een ECHTE symmetrische hysteresisband rond de effectieve setpoint
//              (AAN onder setpoint-hyst/2, UIT boven setpoint+hyst/2, ertussen: stand behouden).
//              V0.10 en vroeger gebruikte enkel een vaste -0.5°C-offset zonder aparte
//              uitschakeldrempel -> geen echte band -> pendelde bij grensgevallen.
// Handmatig:   relais volgt rechtstreeks de UI-schakelaar (voor test zonder werkende sensoren)
// Veiligheid (ALTIJD actief, ongeacht modus):
//   1) geforceerd UIT zodra room_temp de bovengrens van de setpoint-slider bereikt
//   2) geforceerd UIT bij onbetrouwbare temperatuurdata (room_temp_reliable == false)
void updateHeatingLogic() {
  bool new_state;
  if (heating_auto) {
    effective_setpoint = max((float)heating_setpoint, dew + dew_margin);
    float half = hyst_cv / 2.0f;
    if (room_temp < effective_setpoint - half) new_state = true;
    else if (room_temp > effective_setpoint + half) new_state = false;
    else new_state = heating_on;  // binnen de band: huidige stand behouden (dit IS de hysterese)
  } else {
    effective_setpoint = heating_setpoint;
    new_state = relay_manual;
  }

  if (room_temp >= HEATING_SETPOINT_MAX) new_state = false;  // veiligheidsgrens
  if (!room_temp_reliable) new_state = false;                // geen betrouwbare data

  if (new_state != heating_on) {
    heating_on = new_state;
    setRelay(heating_on);
    Serial.printf("[RELAIS1] -> %s (modus: %s)\n", heating_on ? "AAN" : "UIT", heating_auto ? "automatisch" : "handmatig");
  }
}

// ============== SWW-LOGICA + RELAIS 2 (LAADPOMP) ==============
// Zelfde opzet als de verwarming: Automatisch (hysteresisband rond boiler_setpoint) of
// Handmatig, met dezelfde twee veiligheidslagen, ALTIJD actief ongeacht modus.
void updateSWWLogic() {
  bool new_state;
  if (sww_auto) {
    float half = hyst_sww / 2.0f;
    if (temp_boiler < boiler_setpoint - half) new_state = true;
    else if (temp_boiler > boiler_setpoint + half) new_state = false;
    else new_state = sww_on;
  } else {
    new_state = relay2_manual;
  }

  if (temp_boiler >= BOILER_SETPOINT_MAX) new_state = false;  // veiligheidsgrens
  if (!boiler_temp_reliable) new_state = false;                // geen bruikbare boilersensor

  if (new_state != sww_on) {
    sww_on = new_state;
    setRelay2(sww_on);
    Serial.printf("[RELAIS2] -> %s (modus: %s)\n", sww_on ? "AAN" : "UIT", sww_auto ? "automatisch" : "handmatig");
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
// Compact schema (v1.2) — zelfde stijl als het Zarlar-roomproject: korte positionele
// sleutels (a, b, c, ...), enkel "room" voluit. Dit ene schema voedt zowel de live-UI
// (/json, elke 3s op /advanced en /) als de Google Sheets-log (zelfde payload gepost).
// Leesbare namen staan enkel in het Apps Script / de sheet-kolomkoppen, niet hier.
// Bewust weggelaten t.o.v. de vorige (uitgebreide) versie: betrouwbaarheids-/fallback-
// vlaggen, tekstuele meldingen, IP/mDNS/crash/firmware (zelden/nooit live relevant),
// Auto/Handmatig-togglestatus en hysterese (enkel wijzigbaar via Settings, wat toch al
// een herstart+redirect veroorzaakt), en per-sensor-detail van niet-toegewezen
// DS18B20's. Die informatie blijft wel zichtbaar bij het laden van de pagina zelf.
String getJSON() {
  unsigned long upt = (millis() - boot_millis) / 1000;
  float t2_j = isnan(temp_dht) ? 0.0f : temp_dht;   // NaN mag niet in JSON -> 0
  float h_j  = isnan(humi) ? 0.0f : humi;
  bool dew_alert = effective_setpoint > (float)heating_setpoint + 0.01f;  // dauwpuntcorrectie actief?
  bool nacht = light_ldr >= LDR_DARK_THRESHOLD;
  char pon[MAX_PIXELS + 1];
  for (int i = 0; i < pixels_num && i < MAX_PIXELS; i++) pon[i] = pixel_on[i] ? '1' : '0';
  pon[pixels_num < MAX_PIXELS ? pixels_num : MAX_PIXELS] = '\0';
  char pixAan[MAX_PIXELS + 3];
  snprintf(pixAan, sizeof(pixAan), "P=%s", pon);  // "P="-prefix: appendRow() in Sheets bewaart zo leidende nullen
  char buf[420];
  snprintf(buf, sizeof(buf),
    "{\"room\":\"%s\","
    "\"a\":%lu,\"b\":%d,\"c\":%d,\"d\":%.1f,\"e\":%.1f,\"f\":%.1f,\"g\":%.1f,\"h\":%d,"
    "\"i\":%d,\"j\":%d,\"k\":%.1f,"
    "\"l\":%d,\"m\":%d,\"n\":%d,\"o\":%d,\"p\":%d,\"q\":%d,\"r\":\"%s\",\"s\":%d,\"t\":%d,"
    "\"u\":%d,\"v\":%u,\"w\":%u,\"x\":%d}",
    room_id,
    upt, heating_on ? 1 : 0, heating_setpoint, room_temp, t2_j, h_j, dew, dew_alert ? 1 : 0,
    sww_on ? 1 : 0, boiler_setpoint, temp_boiler,
    light_ldr, nacht ? 1 : 0, bed ? 1 : 0, (int)neo_r, (int)neo_g, (int)neo_b, pixAan, pixel0_mode, mov1_triggers,
    ap_mode_active ? 0 : WiFi.RSSI(), (unsigned)(ESP.getFreeHeap()/1024), (unsigned)(ESP.getMaxAllocHeap()/1024), ds_count);
  return String(buf);
}

// mDNS-hostnamen mogen enkel a-z/0-9/streepjes bevatten (RFC-conform, en wat ESPmDNS
// effectief accepteert): kleine letters forceren, ongeldige tekens weggooien, geen leidend/
// sluitend streepje, "sjalay" als terugval bij een lege of volledig ongeldige invoer.
void sanitizeMdnsName(String &v) {
  v.trim();
  v.toLowerCase();
  String out = "";
  for (size_t i = 0; i < v.length() && out.length() < 31; i++) {
    char c = v[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') out += c;
  }
  while (out.length() > 0 && out[0] == '-') out.remove(0, 1);
  while (out.length() > 0 && out[out.length() - 1] == '-') out.remove(out.length() - 1, 1);
  if (out.length() == 0) out = "sjalay";
  v = out;
}

// ============== NVS: LADEN / OPSLAAN ==============
void loadConfigFromNVS() {
  { String t = preferences.getString(NVS_ROOM_ID, "Sjalay"); strlcpy(room_id, t.c_str(), sizeof(room_id)); }
  { String t = preferences.getString(NVS_WIFI_SSID, "netwerknaam"); strlcpy(wifi_ssid, t.c_str(), sizeof(wifi_ssid)); }
  { String t = preferences.getString(NVS_WIFI_PASS, "paswoord"); strlcpy(wifi_pass, t.c_str(), sizeof(wifi_pass)); }
  { String t = preferences.getString(NVS_STATIC_IP, "192.168.xx.xx"); strlcpy(static_ip_str, t.c_str(), sizeof(static_ip_str)); }
  { String t = preferences.getString("mdns_name", "sjalay"); strlcpy(mdns_name, t.c_str(), sizeof(mdns_name)); }
  heating_auto     = preferences.getBool("heat_auto", false);
  relay_manual     = preferences.getBool("relay_man", false);
  heating_setpoint = constrain(preferences.getInt("heat_sp", 20), 5, 30);
  dew_margin       = preferences.getFloat("dew_margin", 2.0);
  hyst_cv          = constrain(preferences.getFloat("hyst_cv", 1.0), 0.2f, 5.0f);
  LDR_DARK_THRESHOLD = constrain(preferences.getInt("ldr_dark", 40), 0, 100);
  { String t = preferences.getString("gas_url", ""); strlcpy(gas_url, t.c_str(), sizeof(gas_url)); }
  sheets_interval_min = constrain(preferences.getInt("sheets_min", 5), 1, 60);

  sww_auto        = preferences.getBool("sww_auto", false);
  relay2_manual   = preferences.getBool("relay2_man", false);
  boiler_setpoint = constrain(preferences.getInt("boiler_sp", 40), 10, 60);
  hyst_sww        = constrain(preferences.getFloat("hyst_sww", 5.0), 0.5f, 15.0f);
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

// ============== WEB-UI: LANDINGSPAGINA (eenvoudige weergave voor huurders/gasten) ==============
// Bewust minimalistisch: enkel de knoppen die een gast echt nodig heeft (licht aan/uit +
// kleur, verwarmings-/SWW-setpoint), geen tekstlabels waar een icoon/kleur volstaat, geen
// Auto/Handmatig-schakelaars of los relais. Bij elke keer laden wordt BEIDE regelkringen
// geforceerd naar Auto gezet (veiligheid: een gast mag nooit per ongeluk in Handmatig
// vastzitten) - NVS wordt enkel beschreven bij een effectieve wissel, niet bij elke reload.
void handleLanding(AsyncWebServerRequest *request) {
  bool nvs_dirty = false;
  preferences.begin("sjalay-cfg", false);
  if (!heating_auto) { heating_auto = true; preferences.putBool("heat_auto", true); nvs_dirty = true; }
  if (!sww_auto)     { sww_auto     = true; preferences.putBool("sww_auto", true);  nvs_dirty = true; }
  preferences.end();
  if (nvs_dirty) { updateHeatingLogic(); updateSWWLogic(); }

  AsyncResponseStream *p = request->beginResponseStream("text/html; charset=utf-8");
  p->print("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<title>");
  p->print(room_id);
  p->print("</title><style>"
    "*{box-sizing:border-box;}"
    "body{font-family:Arial,sans-serif;background:#f2f2f2;margin:0;padding:0;color:#222;}"
    ".hdr{background:#ffcc00;padding:18px 10px;text-align:center;font-size:24px;font-weight:bold;}"
    ".wrap{max-width:480px;margin:0 auto;padding:16px;}"
    ".sec{font-size:12px;color:#999;text-transform:uppercase;letter-spacing:1px;margin:22px 4px 8px;}"
    ".lights{display:grid;grid-template-columns:repeat(auto-fill,minmax(90px,1fr));gap:12px;margin-top:6px;}"
    ".ltile{aspect-ratio:1/1;border-radius:16px;border:none;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.15);"
    "display:flex;flex-direction:column;align-items:center;justify-content:center;cursor:pointer;padding:6px;"
    "transition:background .2s;}"
    ".ltile .ic{font-size:30px;filter:grayscale(1) opacity(.45);}"
    ".ltile.on .ic{filter:none;}"
    ".ltile .nm{font-size:11px;margin-top:4px;color:#555;text-align:center;line-height:1.2;}"
    ".ltile.on .nm{color:#fff;text-shadow:0 1px 2px rgba(0,0,0,.4);}"
    ".ltile.locked{opacity:.55;cursor:default;}"
    ".colorwrap{display:flex;align-items:center;justify-content:center;gap:14px;margin-top:10px;}"
    ".colorwrap input[type=color]{width:52px;height:52px;border:none;border-radius:50%;padding:0;cursor:pointer;background:none;}"
    ".bedbtn{width:52px;height:52px;border-radius:50%;border:2px solid #ccc;background:#fff;"
    "font-size:26px;line-height:1;cursor:pointer;display:flex;align-items:center;justify-content:center;padding:0;}"
    ".bedbtn.on{background:#2c3968;border-color:#2c3968;}"
    ".p0btn{width:52px;height:52px;border-radius:50%;border:2px solid #ccc;background:#fff;"
    "font-size:26px;line-height:1;cursor:pointer;display:flex;align-items:center;justify-content:center;padding:0;}"
    ".p0btn.on{background:#1b7a43;border-color:#1b7a43;}"
    ".card{background:#fff;border-radius:16px;box-shadow:0 1px 3px rgba(0,0,0,.15);padding:18px;text-align:center;margin-bottom:16px;}"
    ".icring{width:58px;height:58px;border-radius:50%;display:inline-flex;align-items:center;justify-content:center;"
    "font-size:30px;background:#fff;border:3px solid #fff;box-shadow:0 1px 3px rgba(0,0,0,.2);transition:background .2s,border-color .2s;}"
    ".icring.active{background:#ffe3e3;border-color:#e03131;}"
    ".card .big{font-size:40px;font-weight:bold;margin:10px 0 14px;}"
    ".card .big .soll{font-size:20px;font-weight:normal;}"
    ".card.heat .big{color:#e05c00;}"
    ".card.sww .big{color:#0077cc;}"
    "input[type=range]{width:90%;height:34px;}"
    ".gearwrap{text-align:center;margin:28px 0 10px;}"
    ".gearwrap a{font-size:26px;text-decoration:none;opacity:.5;}"
    "</style></head><body>");

  p->print("<div class=\"hdr\">");
  p->print(room_id);
  p->print("</div><div class=\"wrap\">");

  // ---- Lichten ----
  p->print("<div class=\"lights\" id=\"lights\">");
  char hexcol[8]; snprintf(hexcol, sizeof(hexcol), "#%02x%02x%02x", neo_r, neo_g, neo_b);
  for (int i = 0; i < pixels_num; i++) {
    // Pixel 0 tijdens Bed-modus: "vergrendeld" getoond (maan-icoon, gedimd, geen tik-
    // actie) i.p.v. een normale tegel die toch genegeerd wordt door de bestaande
    // bed-logica in updatePixelLogic() - anders lijkt een tik ten onrechte niets te doen.
    bool locked = (i == 0 && bed);
    p->printf("<button type=\"button\" class=\"ltile%s%s\" id=\"lt-%d\" data-idx=\"%d\" style=\"background:%s\" "
      "onclick=\"toggleLight(%d)\"><span class=\"ic\">%s</span><span class=\"nm\">%s</span></button>",
      (!locked && pixel_on[i]) ? " on" : "", locked ? " locked" : "", i, i,
      (!locked && pixel_on[i]) ? hexcol : "#fff", i,
      locked ? "&#127769;" : "&#128161;", pixel_nicknames[i]);
  }
  p->print("</div>");
  // Bed-modus (nachtmodus): zelfde grootte als de kleurkiezer, links ernaast. Herbruikt de
  // bestaande bed-variabele/NVS-veld en het /toggle_bed-endpoint (al aanwezig sinds v0.9,
  // voorheen enkel op /advanced) - hier enkel een knop toegevoegd, geen nieuwe backend-logica.
  // Pixel-0-modusknop (AUTO/MANUEEL), zelfde vorm/grootte als de bed-knop, ernaast geplaatst.
  // Hergebruikt het bestaande /toggle_pixel_mode-endpoint (al aanwezig sinds v0.2) - dit is
  // enkel de ontbrekende weg terug naar AUTO vanaf de landingspagina (zie v0.16-changelog).
  p->printf("<div class=\"colorwrap\">"
    "<button type=\"button\" id=\"bedToggle\" class=\"bedbtn%s\" onclick=\"toggleBed()\">&#128719;</button>"
    "<button type=\"button\" id=\"p0modeToggle\" class=\"p0btn%s\" onclick=\"toggleP0Mode()\">%s</button>"
    "<input type=\"color\" id=\"colorPicker\" value=\"%s\" onchange=\"setNeoColor(this.value)\">"
    "</div>", bed ? " on" : "", pixel0_mode == 1 ? " on" : "",
    pixel0_mode == 1 ? "&#9995;" : "&#128260;", hexcol);

  // ---- Verwarming ----
  // IST (huidige, gemeten temperatuur) en SOLL (effectieve/gevraagde doeltemp, incl.
  // dauwpuntcorrectie) samen op één lijn, in de kaartkleur: groot IST eerst, dan kleiner
  // "(-> X°)" tussen haakjes met spatie ervoor - live bijgewerkt tijdens het schuiven en
  // nadien gesynchroniseerd met de echte effectieve setpoint via /json.
  p->printf("<div class=\"card heat\"><div class=\"icring%s\" id=\"ic-heat\">&#128293;</div>"
    "<div class=\"big\"><span id=\"v-rt\">%s</span><span class=\"soll\" id=\"v-heff-t\"> (&rarr; %d&deg;)</span></div>"
    "<form action=\"/set_setpoint\" method=\"get\" onsubmit=\"event.preventDefault();\">"
    "<input type=\"range\" id=\"sl-hsp\" min=\"5\" max=\"30\" value=\"%d\" "
    "oninput=\"document.getElementById('v-heff-t').textContent=' (\xe2\x86\x92 '+this.value+'\xc2\xb0)'\" "
    "onchange=\"setVal('/set_setpoint',this.value)\"></form></div>",
    heating_on ? " active" : "",
    room_temp_reliable ? (String(room_temp, 1) + "&deg;").c_str() : "n.v.t.",
    heating_setpoint, heating_setpoint);

  // ---- SWW ----
  // Idem: groot IST (gemeten boilertemp), klein "(-> X°)" = SOLL (setpoint; voor SWW
  // momenteel gelijk aan de effectieve doeltemp, geen aparte correctie zoals bij verwarming).
  p->printf("<div class=\"card sww\"><div class=\"icring%s\" id=\"ic-sww\">&#128703;</div>"
    "<div class=\"big\"><span id=\"v-bt\">%s</span><span class=\"soll\" id=\"v-bsp-t\"> (&rarr; %d&deg;)</span></div>"
    "<form action=\"/set_boiler_setpoint\" method=\"get\" onsubmit=\"event.preventDefault();\">"
    "<input type=\"range\" id=\"sl-bsp\" min=\"10\" max=\"60\" value=\"%d\" "
    "oninput=\"document.getElementById('v-bsp-t').textContent=' (\xe2\x86\x92 '+this.value+'\xc2\xb0)'\" "
    "onchange=\"setVal('/set_boiler_setpoint',this.value)\"></form></div>",
    sww_on ? " active" : "",
    boiler_temp_reliable ? (String(temp_boiler, 1) + "&deg;").c_str() : "n.v.t.",
    boiler_setpoint, boiler_setpoint);

  // ---- Geavanceerd (enkel icoon, geen tekst) ----
  p->print("<div class=\"gearwrap\"><a href=\"/advanced\">&#9881;&#65039;</a></div>");

  p->printf("<script>"
    "var pixelsNum=%d;"
    "function toHex(v){return ('0'+Math.round(v).toString(16)).slice(-2);}"
    "var lastHex='#ffffff';"
    "var bedOn=false;",
    pixels_num);
  p->print(
    "function setVal(url,v){fetch(url+'?value='+v).then(refresh);}"
    "function toggleLight(i){if(i==0&&bedOn)return;fetch('/toggle_pixel?idx='+i).then(refresh);}"
    "function toggleBed(){fetch('/toggle_bed').then(refresh);}"
    "function toggleP0Mode(){fetch('/toggle_pixel_mode').then(refresh);}"
    "function setNeoColor(hex){"
      "var r=parseInt(hex.slice(1,3),16),g=parseInt(hex.slice(3,5),16),b=parseInt(hex.slice(5,7),16);"
      "lastHex=hex;"
      "fetch('/setcolor?r='+r+'&g='+g+'&b='+b).then(refresh);}"
    "function refresh(){"
      "fetch('/json?'+Date.now(),{cache:'no-store'}).then(r=>r.json()).then(data=>{"
        "lastHex='#'+toHex(data.o)+toHex(data.p)+toHex(data.q);"
        "if(document.activeElement.id!=='colorPicker')document.getElementById('colorPicker').value=lastHex;"
        "bedOn=!!data.n;"
        "var bb=document.getElementById('bedToggle');if(bb)bb.className='bedbtn'+(bedOn?' on':'');"
        "var pb=document.getElementById('p0modeToggle');if(pb){pb.className='p0btn'+(data.s===1?' on':'');pb.innerHTML=(data.s===1?'&#9995;':'&#128260;');}"
        "var ponBits=(data.r||'P=').slice(2);"
        "for(var i=0;i<pixelsNum;i++){"
          "var t=document.getElementById('lt-'+i);if(!t)continue;"
          "var locked=(i==0&&bedOn);"
          "var on=!locked&&ponBits.charAt(i)==='1';"
          "t.className='ltile'+(on?' on':'')+(locked?' locked':'');"
          "t.style.background=on?lastHex:'#fff';"
          "if(i==0){var ic=t.querySelector('.ic');if(ic)ic.innerHTML=locked?'&#127769;':'&#128161;';}"
        "}"
        "var vr=document.getElementById('v-rt');if(vr)vr.textContent=data.d.toFixed(1)+'°';"
        "var ht=document.getElementById('v-heff-t');if(ht&&document.activeElement.id!=='sl-hsp')ht.textContent=' (→ '+data.c+'°)';"
        "var hs=document.getElementById('sl-hsp');if(hs&&document.activeElement.id!=='sl-hsp')hs.value=data.c;"
        "var ih=document.getElementById('ic-heat');if(ih)ih.className='icring'+(data.b?' active':'');"
        "var vb=document.getElementById('v-bt');if(vb)vb.textContent=data.k.toFixed(1)+'°';"
        "var bt=document.getElementById('v-bsp-t');if(bt&&document.activeElement.id!=='sl-bsp')bt.textContent=' (→ '+data.j+'°)';"
        "var bs=document.getElementById('sl-bsp');if(bs&&document.activeElement.id!=='sl-bsp')bs.value=data.j;"
        "var is=document.getElementById('ic-sww');if(is)is.className='icring'+(data.i?' active':'');"
      "}).catch(e=>console.error(e));}"
    "document.addEventListener('DOMContentLoaded',function(){refresh();setInterval(refresh,3000);});"
    "</script>");

  p->print("</div></body></html>");
  request->send(p);
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

  p->print("<div class=\"group-title\">Verwarming</div><table>");
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
    "<input type=\"range\" class=\"slider\" id=\"sl-hsp\" name=\"value\" min=\"5\" max=\"30\" value=\"%d\" onchange=\"submitAjax(this.form);\">"
    "</form></td></tr>", heating_setpoint, heating_setpoint);
  { char humTxt[16];
    if (isnan(humi)) strlcpy(humTxt, "n.v.t.", sizeof(humTxt));
    else snprintf(humTxt, sizeof(humTxt), "%.0f%%", humi);
    p->printf("<tr><td class=\"label\">Effectieve setpoint met %% vochtigheid</td><td class=\"value\" id=\"v-heff\" colspan=\"2\">%.1f &deg;C (vocht %s)</td></tr>",
      effective_setpoint, humTxt);
  }
  p->printf("<tr><td class=\"label\">DHT22 temp</td><td class=\"value\" id=\"v-t2\" colspan=\"2\">%s</td></tr>",
    isnan(temp_dht) ? "defect (geen sensor?)" : (String(temp_dht, 1) + " &deg;C").c_str());
  p->printf("<tr><td class=\"label\">DHT22 vocht</td><td class=\"value\" id=\"v-h\" colspan=\"2\">%s</td></tr>",
    isnan(humi) ? "defect" : (String(humi, 1) + " %").c_str());
  p->printf("<tr><td class=\"label\">Dauwpunt</td><td class=\"value\" id=\"v-dp\" colspan=\"2\">%.1f &deg;C</td></tr>", dew);
  p->printf("<tr><td class=\"label\">Kamertemperatuur<br><span style=\"font-size:11px;color:#888;\">bron: <span id=\"v-rtsrc\">%s</span></span></td><td class=\"value\" id=\"v-rt\" colspan=\"2\">%s</td></tr>",
    room_source, room_temp_reliable ? (String(room_temp, 1) + " &deg;C").c_str() : "n.v.t.");
  { const char* tm_color = !room_temp_reliable ? "#c00" : (room_temp_fallback ? "#e67e22" : "#2a9d2a");
    p->printf("<tr><td class=\"label\">Melding</td><td class=\"value\" id=\"v-tm\" colspan=\"2\" style=\"color:%s;\">%s</td></tr>", tm_color, temp_melding);
  }
  p->printf("<tr><td class=\"label\">Ketelvraag</td><td class=\"value\" id=\"v-hon\" colspan=\"2\">"
    "<span class=\"dot\" style=\"background:%s\"></span> %s</td></tr>",
    heating_on ? "#e05c00" : "#bbb", heating_on ? "AAN" : "UIT");
  p->printf("<tr><td class=\"label\">Duty-cyclus (laatste 4u)</td><td class=\"value\" id=\"v-duty\" colspan=\"2\">%.0f %%</td></tr>", getDutyPercent());
  p->printf("<tr><td class=\"label\">Hysterese<br><span style=\"font-size:11px;color:#888;\">instelbaar in Settings</span></td><td class=\"value\" id=\"v-hcv\" colspan=\"2\">&plusmn;%.1f &deg;C</td></tr>", hyst_cv);
  p->print("</table>");

  p->print("<div class=\"group-title\">SWW boiler</div><table>");
  p->printf("<tr><td class=\"label\">Automatische modus</td><td class=\"value\" id=\"v-swauto\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_sww_auto\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-swauto\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    sww_auto ? "AAN" : "UIT", sww_auto ? " checked" : "");
  p->printf("<tr><td class=\"label\">Relais 2 (handmatig)<br><span style=\"font-size:11px;color:#888;\">genegeerd in automatisch</span></td>"
    "<td class=\"value\" id=\"v-sw2man\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_relay2_manual\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-sw2man\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    relay2_manual ? "AAN" : "UIT", relay2_manual ? " checked" : "");
  p->printf("<tr><td class=\"label\">Boiler-setpoint</td><td class=\"value\" id=\"v-bsp\">%d &deg;C</td>"
    "<td class=\"control\"><form action=\"/set_boiler_setpoint\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<input type=\"range\" class=\"slider\" id=\"sl-bsp\" name=\"value\" min=\"10\" max=\"60\" value=\"%d\" onchange=\"submitAjax(this.form);\">"
    "</form></td></tr>", boiler_setpoint, boiler_setpoint);
  p->printf("<tr><td class=\"label\">Boilertemperatuur (%s)</td><td class=\"value\" id=\"v-bt\" colspan=\"2\">%s</td></tr>",
    ds_boiler >= 0 ? ds_nicknames[ds_boiler] : "geen sensor", boiler_temp_reliable ? (String(temp_boiler, 1) + " &deg;C").c_str() : "n.v.t.");
  p->printf("<tr><td class=\"label\">SWW-pomp</td><td class=\"value\" id=\"v-swon\" colspan=\"2\">"
    "<span class=\"dot\" style=\"background:%s\"></span> %s</td></tr>",
    sww_on ? "#0077cc" : "#bbb", sww_on ? "AAN" : "UIT");
  p->printf("<tr><td class=\"label\">Hysterese<br><span style=\"font-size:11px;color:#888;\">instelbaar in Settings</span></td><td class=\"value\" id=\"v-hsww\" colspan=\"2\">&plusmn;%.1f &deg;C</td></tr>", hyst_sww);
  p->printf("<tr><td class=\"label\">Melding</td><td class=\"value\" id=\"v-btm\" colspan=\"2\" style=\"color:#e67e22;\">%s</td></tr>", boiler_melding);
  p->print("</table>");

  p->print("<div class=\"group-title\">Alle DS18B20-sensoren</div><table id=\"ds-all-table\">");
  p->printf("<tr><td class=\"label\">DS18B20 gevonden</td><td class=\"value\" id=\"v-dsc\" colspan=\"2\">%d %s</td></tr>",
    ds_count, ds_count == 0 ? "(RoomSense niet aangesloten?)" : "");
  if (ds_count == 0) {
    p->print("<tr><td class=\"label\" colspan=\"3\">Geen sensoren gevonden</td></tr>");
  }
  for (int i = 0; i < ds_count; i++) {
    const char* role = (i == ds_primary) ? " (kamer)" : (i == ds_boiler) ? " (boiler)" : "";
    p->printf("<tr><td class=\"label\">%s%s</td><td class=\"value\" colspan=\"2\" id=\"v-dsall-%d\">%s &deg;C%s</td></tr>",
      ds_nicknames[i], role, i, String(temp_ds_arr[i], 1).c_str(), ds_fail_streak[i] >= DS_FAIL_THRESHOLD ? " &mdash; ontbreekt" : "");
  }
  p->print("</table>");

  char hexcol[8]; snprintf(hexcol, sizeof(hexcol), "#%02x%02x%02x", neo_r, neo_g, neo_b);
  p->print("<div class=\"group-title\">Verlichting</div><table>");
  p->printf("<tr><td class=\"label\">OMGEVINGSLICHT (donker=100)</td><td class=\"value\" id=\"v-ldr\" colspan=\"2\">%d</td></tr>", light_ldr);
  p->printf("<tr><td class=\"label\">Bed-modus<br><span style=\"font-size:11px;color:#888;\">dwingt pixel 0 uit</span></td>"
    "<td class=\"value\" id=\"v-bed\">%s</td>"
    "<td class=\"control\"><form action=\"/toggle_bed\" method=\"get\" onsubmit=\"event.preventDefault();submitAjax(this);\">"
    "<label class=\"switch\"><input type=\"checkbox\" id=\"cb-bed\"%s onchange=\"submitAjax(this.form);\">"
    "<span class=\"slider-switch\"></span></label></form></td></tr>",
    bed ? "AAN" : "UIT", bed ? " checked" : "");
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
  p->printf("<tr><td class=\"label\" id=\"v-movlbl\">PIR MOV1 trig/%d min</td><td class=\"value\" id=\"v-mov\" colspan=\"2\">%d</td></tr>", sheets_interval_min, mov1_triggers);
  p->print("</table>");

  p->print("<div class=\"group-title\">Logging</div><table>");
  { char gasTxt[32];
    if (strlen(gas_url) == 0) strlcpy(gasTxt, "UIT (geen URL ingesteld)", sizeof(gasTxt));
    else snprintf(gasTxt, sizeof(gasTxt), "AAN (elke %d min)", sheets_interval_min);
    p->printf("<tr><td class=\"label\">Google Sheets</td><td class=\"value\" id=\"v-gas\" colspan=\"2\">%s</td></tr>", gasTxt);
  }
  p->printf("<tr><td class=\"label\">Laatste resultaat</td><td class=\"value\" id=\"v-gcode\" colspan=\"2\">%s</td></tr>",
    sheets_last_post == 0 ? "nog niet geprobeerd" : (sheets_last_code == 200 ? "HTTP 200 (OK)" : String("HTTP " + String(sheets_last_code)).c_str()));
  p->print("</table>");

  p->print("<div class=\"group-title\">Controller</div><table>");
  p->printf("<tr><td class=\"label\">IP-adres</td><td class=\"value\" colspan=\"2\">%s</td></tr>",
    ap_mode_active ? "192.168.4.1 (AP-modus)" : WiFi.localIP().toString().c_str());
  p->printf("<tr><td class=\"label\">mDNS-naam</td><td class=\"value\" colspan=\"2\">%s</td></tr>",
    ap_mode_active ? "n.v.t. (AP-modus)" : (String("http://") + mdns_name + ".local/").c_str());
  p->printf("<tr><td class=\"label\">Wi-Fi RSSI</td><td class=\"value\" id=\"v-rssi\" colspan=\"2\">%d dBm</td></tr>", ap_mode_active ? 0 : WiFi.RSSI());
  p->printf("<tr><td class=\"label\">MAC-adres</td><td class=\"value\" colspan=\"2\">%s</td></tr>", mac_address);
  p->printf("<tr><td class=\"label\">Vrije heap</td><td class=\"value\" id=\"v-heap\" colspan=\"2\">%u KB</td></tr>", (unsigned)(ESP.getFreeHeap()/1024));
  p->printf("<tr><td class=\"label\">Grootste blok</td><td class=\"value\" id=\"v-lb\" colspan=\"2\">%u KB</td></tr>", (unsigned)(ESP.getMaxAllocHeap()/1024));
  p->printf("<tr><td class=\"label\">Crash-teller</td><td class=\"value\" id=\"v-crash\" colspan=\"2\">%u</td></tr>", (unsigned)getCrashCount());
  p->printf("<tr><td class=\"label\">Firmware</td><td class=\"value\" colspan=\"2\">v%s</td></tr>", SJALAY_VERSION);
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
        "window.lastUptime=data.a;"
        "lastHex='#'+toHex(data.o)+toHex(data.p)+toHex(data.q);"
        "var g=id=>document.getElementById(id);"
        "if(g('v-rssi'))g('v-rssi').textContent=data.u+' dBm';"
        "if(g('v-heap'))g('v-heap').textContent=data.v+' KB';"
        "if(g('v-lb'))g('v-lb').textContent=data.w+' KB';"
        "if(g('v-t2'))g('v-t2').innerHTML=data.e.toFixed(1)+' &deg;C';"
        "if(g('v-h'))g('v-h').innerHTML=data.f.toFixed(1)+' %';"
        "if(g('v-dp'))g('v-dp').innerHTML=data.g.toFixed(1)+' &deg;C';"
        "if(g('v-dsc'))g('v-dsc').textContent=data.x+(data.x===0?' (RoomSense niet aangesloten?)':'');"
        "if(g('v-rt'))g('v-rt').innerHTML=data.d.toFixed(1)+' &deg;C';"
        "if(g('v-bsp'))g('v-bsp').textContent=data.j+' \u00b0C';"
        "if(g('sl-bsp')&&document.activeElement.id!=='sl-bsp')g('sl-bsp').value=data.j;"
        "if(g('v-bt'))g('v-bt').innerHTML=data.k.toFixed(1)+' &deg;C';"
        "if(g('v-swon'))g('v-swon').innerHTML=dot(data.i,'#0077cc')+' '+(data.i?'AAN':'UIT');"
        "if(g('v-ldr'))g('v-ldr').textContent=data.l;"
        "if(g('v-mov'))g('v-mov').textContent=data.t;"
        "if(g('v-hsp'))g('v-hsp').textContent=data.c+' \u00b0C';"
        "if(g('sl-hsp')&&document.activeElement.id!=='sl-hsp')g('sl-hsp').value=data.c;"
        "if(g('v-hon'))g('v-hon').innerHTML=dot(data.b,'#e05c00')+' '+(data.b?'AAN':'UIT');"
        "if(g('cb-bed'))g('cb-bed').checked=!!data.n;"
        "if(g('v-bed'))g('v-bed').textContent=data.n?'AAN':'UIT';"
        "if(g('cb-p0m'))g('cb-p0m').checked=(data.s===1);"
        "if(g('v-p0m'))g('v-p0m').textContent=(data.s===1)?'MANUEEL':'AUTO';"
        "if(g('row-p0bed'))g('row-p0bed').style.display=data.n?'':'none';"
        "if(g('row-p0on'))g('row-p0on').style.display=(!data.n&&data.s===1)?'':'none';"
        "if(g('row-p0auto'))g('row-p0auto').style.display=(!data.n&&data.s===0)?'':'none';"
        "if(g('rgb_val'))g('rgb_val').textContent=data.o+', '+data.p+', '+data.q;"
        "if(g('colorPicker')&&document.activeElement.id!=='colorPicker')g('colorPicker').value=lastHex;"
        "var ponBits=(data.r||'P=').slice(2);"
        "document.querySelectorAll('.cb-pix').forEach(cb=>{"
          "var i=parseInt(cb.getAttribute('data-idx'));"
          "cb.checked=(ponBits.charAt(i)==='1');});"
        "document.querySelectorAll('.pixdot').forEach(sp=>{"
          "var i=parseInt(sp.getAttribute('data-idx'));"
          "sp.style.background=(ponBits.charAt(i)==='1')?lastHex:'#bbb';});"
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
  p->printf("<tr><td class=\"label\">mDNS-naam<br><span style=\"font-size:11px;color:#888;\">enkel a-z/0-9/streepjes, herstart nodig</span></td>"
    "<td class=\"control\"><input type=\"text\" name=\"mdns\" value=\"%s\" maxlength=\"31\"> .local</td></tr>", mdns_name);
  p->printf("<tr><td class=\"label\">Dauwpuntmarge (automatische modus)</td><td class=\"control\"><input type=\"number\" name=\"dew\" step=\"0.5\" min=\"0\" max=\"10\" value=\"%.1f\" style=\"width:60px;\"> &deg;C</td></tr>", dew_margin);
  p->printf("<tr><td class=\"label\">Hysterese verwarming<br><span style=\"font-size:11px;color:#888;\">band rond setpoint, min. 0,2&deg;C</span></td><td class=\"control\"><input type=\"number\" name=\"hystcv\" step=\"0.1\" min=\"0.2\" max=\"5\" value=\"%.1f\" style=\"width:60px;\"> &deg;C</td></tr>", hyst_cv);
  p->printf("<tr><td class=\"label\">Hysterese SWW<br><span style=\"font-size:11px;color:#888;\">band rond boiler-setpoint</span></td><td class=\"control\"><input type=\"number\" name=\"hystsww\" step=\"0.5\" min=\"0.5\" max=\"15\" value=\"%.1f\" style=\"width:60px;\"> &deg;C</td></tr>", hyst_sww);
  p->printf("<tr><td class=\"label\">LDR donker-drempel (0-100)</td><td class=\"control\"><input type=\"number\" name=\"ldrdark\" min=\"0\" max=\"100\" value=\"%d\" style=\"width:60px;\"></td></tr>", LDR_DARK_THRESHOLD);
  p->printf("<tr><td class=\"label\">Aantal pixels (1-30, herstart nodig)</td><td class=\"control\"><input type=\"number\" name=\"pnum\" min=\"1\" max=\"30\" value=\"%d\" style=\"width:60px;\"></td></tr>", pixels_num);
  p->printf("<tr><td class=\"label\">Google Script URL<br><span style=\"font-size:11px;color:#888;\">leeg = logging uit</span></td>"
    "<td class=\"control\"><input type=\"text\" name=\"gas\" value=\"%s\" maxlength=\"199\" style=\"width:95%%;\"></td></tr>", gas_url);
  p->printf("<tr><td class=\"label\">Sheets-interval<br><span style=\"font-size:11px;color:#888;\">ook PIR-telvenster (trig/X min)</span></td>"
    "<td class=\"control\"><input type=\"number\" name=\"sheetsint\" min=\"1\" max=\"60\" value=\"%d\" style=\"width:60px;\"> min</td></tr>", sheets_interval_min);
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
    p->print("<tr><td class=\"label\">Primaire sensor (kamer)</td><td class=\"control\"><select name=\"dsprimary\">");
    for (int i = 0; i < ds_count; i++) {
      p->printf("<option value=\"%d\"%s>%s</option>", i, i == ds_primary ? " selected" : "", ds_nicknames[i]);
    }
    p->print("</select></td></tr>");
    p->print("<tr><td class=\"label\">Boilersensor<br><span style=\"font-size:11px;color:#888;\">stuurt SWW-relais, apart van kamersensor</span></td><td class=\"control\"><select name=\"dsboiler\">");
    p->printf("<option value=\"-1\"%s>(geen)</option>", ds_boiler < 0 ? " selected" : "");
    for (int i = 0; i < ds_count; i++) {
      p->printf("<option value=\"%d\"%s>%s</option>", i, i == ds_boiler ? " selected" : "", ds_nicknames[i]);
    }
    p->print("</select></td></tr>");
  }
  p->print("</table>"
    "<a class=\"btn\" href=\"/rescan_ds\">Herscan DS18B20-bus</a>"
    "<p style=\"font-size:12px;color:#777;\">Herscan navigeert direct weg — sla eerst eventuele wijzigingen hierboven op.</p>"
    "<button type=\"submit\">Opslaan &amp; herstart</button>"
    "</form>");

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
  if (request->hasParam("mdns")) {
    String v = request->getParam("mdns")->value();
    sanitizeMdnsName(v);  // altijd geldig na sanitize (terugval "sjalay" bij lege/ongeldige invoer)
    preferences.putString("mdns_name", v);
  }
  if (request->hasParam("dew")) {
    dew_margin = request->getParam("dew")->value().toFloat();
    dew_margin = constrain(dew_margin, 0.0f, 10.0f);
    preferences.putFloat("dew_margin", dew_margin);
  }
  if (request->hasParam("hystcv")) {
    hyst_cv = constrain(request->getParam("hystcv")->value().toFloat(), 0.2f, 5.0f);
    preferences.putFloat("hyst_cv", hyst_cv);
  }
  if (request->hasParam("hystsww")) {
    hyst_sww = constrain(request->getParam("hystsww")->value().toFloat(), 0.5f, 15.0f);
    preferences.putFloat("hyst_sww", hyst_sww);
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
  if (request->hasParam("sheetsint")) {
    sheets_interval_min = constrain(request->getParam("sheetsint")->value().toInt(), 1, 60);
    preferences.putInt("sheets_min", sheets_interval_min);
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
  if (request->hasParam("dsboiler") && ds_count > 0) {
    int b = request->getParam("dsboiler")->value().toInt();
    b = (b < 0) ? -1 : constrain(b, 0, ds_count - 1);
    preferences.putInt("ds_boiler", b);
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
    heating_setpoint = constrain(request->getParam("value")->value().toInt(), 5, 30);
    preferences.begin("sjalay-cfg", false);
    preferences.putInt("heat_sp", heating_setpoint);
    preferences.end();
    updateHeatingLogic();
  }
  request->send(200, "text/plain", "OK");
}

void handleToggleSWWAuto(AsyncWebServerRequest *request) {
  sww_auto = !sww_auto;
  preferences.begin("sjalay-cfg", false);
  preferences.putBool("sww_auto", sww_auto);
  preferences.end();
  updateSWWLogic();  // relais onmiddellijk aanpassen
  request->send(200, "text/plain", "OK");
}

void handleToggleRelay2Manual(AsyncWebServerRequest *request) {
  relay2_manual = !relay2_manual;
  preferences.begin("sjalay-cfg", false);
  preferences.putBool("relay2_man", relay2_manual);
  preferences.end();
  updateSWWLogic();
  request->send(200, "text/plain", "OK");
}

void handleSetBoilerSetpoint(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    boiler_setpoint = constrain(request->getParam("value")->value().toInt(), 10, 60);
    preferences.begin("sjalay-cfg", false);
    preferences.putInt("boiler_sp", boiler_setpoint);
    preferences.end();
    updateSWWLogic();
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
      // Aangeraakt vanop de landingspagina kan pixel 0 nog in AUTO staan -> dan
      // impliciet naar MANUEEL schakelen (op de Advanced-pagina is deze knop
      // sowieso enkel zichtbaar wanneer al MANUEEL, dus geen gedragswijziging daar).
      if (pixel0_mode != 1) {
        pixel0_mode = 1;
        preferences.putInt("pixel_mode_0", pixel0_mode);
      }
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

  // === FAIL-SAFE: beide relais UIT vóór alles verder, incl. vóór Wi-Fi/NVS ===
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);
  pinMode(RELAY2_PIN, OUTPUT);
  setRelay2(false);
  Serial.println("[RELAIS] Fail-safe: beide relais UIT bij boot.");

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
    preferences.putString("mdns_name", "sjalay");
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

  updateHeatingLogic();  // pas geladen modus/schakelaar toe op relais 1 (nog steeds fail-safe UIT als niets gezet was)
  updateSWWLogic();      // idem voor relais 2 (SWW-laadpomp)

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

  // === mDNS (Bonjour) — enkel zinvol met een echte Wi-Fi-verbinding, niet in AP-setup-modus ===
  if (!ap_mode_active) {
    if (MDNS.begin(mdns_name)) {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[mDNS] actief: http://%s.local/\n", mdns_name);
    } else {
      Serial.println("[mDNS] starten mislukt");
    }
  }

  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();
  configTzTime("CET-1CEST,M3.5.0/02,M10.5.0/03", "pool.ntp.org", "time.nist.gov");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
  DefaultHeaders::Instance().addHeader("Pragma", "no-cache");
  DefaultHeaders::Instance().addHeader("Expires", "-1");

  server.on("/", HTTP_GET, handleLanding);
  server.on("/advanced", HTTP_GET, handleStatus);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/save_settings", HTTP_GET, handleSaveSettings);
  server.on("/factory_reset", HTTP_GET, handleFactoryReset);
  server.on("/clear_crash_log", HTTP_GET, handleClearCrashLog);
  server.on("/rescan_ds", HTTP_GET, handleRescanDS);
  server.on("/toggle_heating_auto", HTTP_GET, handleToggleHeatingAuto);
  server.on("/toggle_relay_manual", HTTP_GET, handleToggleRelayManual);
  server.on("/set_setpoint", HTTP_GET, handleSetSetpoint);
  server.on("/toggle_sww_auto", HTTP_GET, handleToggleSWWAuto);
  server.on("/toggle_relay2_manual", HTTP_GET, handleToggleRelay2Manual);
  server.on("/set_boiler_setpoint", HTTP_GET, handleSetBoilerSetpoint);
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
    updateSWWLogic();
  }

  // Google Sheets: elke 5 min (HTTPS POST blokkeert kort, ~0.5-2s — aanvaardbaar op deze cadans)
  if (!ap_mode_active && millis() - sheets_last_post > sheetsIntervalMs()) {
    postToGoogleSheets();
  }
}
