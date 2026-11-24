// ============================================================
// SJALAY / YSERMONDE CONTROLLER DATA LOGGER - Google Apps Script v1.3
// Ontvangt JSON-push van de vakantiehuis-controllers (ESP32-C6, één sketch voor
// Sjalay en Ysermonde) en logt naar ÉÉN Google Sheet, tabblad "Data", voor alle huizen
// (zelfde URL in /settings van elk huis). Huisfilter: keuzelijst in C1 (ALL of één huis),
// zoals in de ROOM-Sheet (ROOM-script v1.6).
//
// v1.0 (1okt26): eerste versie voor Sjalay, 26 kolommen (A..Z).
// v1.1 (25okt26): 34 kolommen (A..AH). Nieuw: AA Reset-reden, AB Boots, AC DHT-reeks,
//                 AD DHT-fouten, AE POST-fouten, AF Kelder, AG Ketelpomp, AH Retour.
//                 JSON null -> lege cel. Kolom E = EFFECTIEF setpoint (veld y).
// v1.2 (2nov26): Vorstalarm per mail + "geen data"-melding; logt naar tabblad "Data".
// v1.3 (24nov26): VOLLEDIG SCHEMA (bedoeld als eindschema, ook voor latere sketchversies):
//                 63 kolommen (A..BK). Kolommen A..AH ongewijzigd. Nieuw:
//                 AI..AJ  SCD41: CO2 (H), vocht-bron (I)
//                 AK..AL  functies: verwarming aanwezig (J), SWW-boiler aanwezig (K)
//                 AM..BH  HomeWizard P1-meter (keys pa..pv): status, vermogen (totaal + L1..L3),
//                         meterstanden afname/injectie dag/nacht, tarief, kwartiervermogen,
//                         maandpiek (capaciteitstarief), spanning en stroom L1..L3, gas (m³ + tijd),
//                         lange stroomonderbrekingen, Wi-Fi-sterkte dongle
//                 BI..BK  watermeter (keys wa..wc): status, meterstand (m³), debiet (l/min)
//                 Ontbrekende keys (oudere sketch, functie niet aanwezig) -> lege cel.
//                 Kolommen worden uit één tabel (COLUMNS) opgebouwd: kop, key, notatie, breedte.
//                 Eén Sheet voor alle huizen: keuzelijst in C1 (ALL + elke room-naam die ooit
//                 postte) zet een filter op kolom C (verbergt enkel rijen, wist niets); titel
//                 staat nu in D1. Grafieken op andere tabbladen tonen dan enkel het gekozen huis
//                 (zolang "verborgen gegevens tonen" in de grafiek uit staat).
//                 Alarm PER HUIS: eigen alarmtoestand en "geen data"-controle per room-naam,
//                 room-naam in het onderwerp; boileralarm enkel als SWW aanwezig is (K ≠ 0).
//                 Na het bijwerken: setupHeaders() eenmalig opnieuw uitvoeren (nieuwe koppen
//                 en keuzelijst; bestaande data blijft staan).
//
// ⚠️  DEPLOYMENT INSTRUCTIE (BELANGRIJK):
//   Gebruik bij elke update: "Implementeren" → "Implementaties beheren"
//   → potlood-icoon (Bewerken) → Versie: "Nieuwe versie" → Implementeren
//   De URL blijft dan DEZELFDE → Settings op de controller hoeft niet aangepast te worden!
//   "Nieuwe implementatie" geeft ALTIJD een nieuwe URL → logging stopt
//   tot je de nieuwe URL ook in Settings invult.
// ============================================================

// ============================================================
// ⚙️  CONFIGURATIE — pas hier aan zonder de rest aan te raken
// ============================================================
const MAX_ROWS    = 50000;  // Maximum aantal datarijen (excl. headers), ALLE huizen samen; oudste rij verdwijnt
                            // (2 huizen om de 5 min ≈ 87 dagen per huis)
const HEADER_ROWS = 2;      // Rij 1 = keuzelijst (C1) + titel (D1), Rij 2 = kolomtitels
const SHEET_NAME  = "Data"; // naam van het tabblad met de logrijen
const FILTER_CELL = "C1";   // keuzelijst huisfilter (boven de kolom "Kamer")
const ROOM_COL    = 3;      // kolom C = Kamer (room-naam van de controller)
const TITLE       = "SJALAY / YSERMONDE CONTROLLER DATA LOGGER v1.3";
// ============================================================

// null/undefined (ongeldige of ontbrekende meting) -> lege cel; anders de waarde (0 blijft 0)
function blank(v) {
  return (v === null || v === undefined) ? "" : v;
}

// HomeWizard gas_timestamp: "yymmddhhmmss" (getal of tekst) -> "20yy-mm-dd hh:mm:ss"; tekst met "-" blijft
function gasTime_(v) {
  if (v === null || v === undefined || v === "") return "";
  const s = String(v);
  if (/^\d{12}$/.test(s)) {
    return "20" + s.substr(0, 2) + "-" + s.substr(2, 2) + "-" + s.substr(4, 2) + " " +
           s.substr(6, 2) + ":" + s.substr(8, 2) + ":" + s.substr(10, 2);
  }
  return s;
}

// ------------------------------------------------------------
// KOLOMMEN — één tabel = kopregel, waarde, notatie en breedte.
// val(d, ts) geeft de celwaarde; d = JSON van de controller, ts = tijdstempel.
// ⚠️ Volgorde = kolomvolgorde. Nooit een kolom tussenvoegen of verwijderen:
//    nieuwe kolommen enkel ACHTERAAN (anders verschuift de bestaande data).
// ------------------------------------------------------------
function k_(key) { return function (d) { return blank(d[key]); }; }

const COLUMNS = [
  // ---- A..AH: zoals v1.1/v1.2 (ongewijzigd) ----
  { h: "Tijdstempel",    w: 130, f: "yyyy-mm-dd hh:mm:ss", val: function (d, ts) { return ts; } },       // A
  { h: "Uptime (s)",     w: 60,  val: function (d) { return d.a || 0; } },                                // B  a
  { h: "Kamer",          w: 80,  val: function (d) { return d.room || "?"; } },                           // C  room
  { h: "HEAT",           w: 45,  val: function (d) { return d.b || 0; } },                                // D  b ketelvraag
  { h: "Set (°C)",       w: 55,  f: "0.0", val: function (d) { return d.y != null ? d.y : (d.c || 0); } },// E  y effectief setpoint
  { h: "Temp1 (°C)",     w: 60,  f: "0.0", val: k_("d") },                                                // F  d kamertemp (effectief)
  { h: "Temp2 (°C)",     w: 60,  f: "0.0", val: k_("e") },                                                // G  e DHT22 rauw
  { h: "Vocht (%)",      w: 50,  f: "0.0", val: k_("f") },                                                // H  f
  { h: "Dauwpt (°C)",    w: 60,  f: "0.0", val: k_("g") },                                                // I  g
  { h: "Dew Alert",      w: 55,  val: function (d) { return d.h || 0; } },                                // J  h
  { h: "SWW pomp",       w: 55,  f: "0",   val: function (d) { return d.i ? 1 : 0; } },                  // K  i
  { h: "SWW Set (°C)",   w: 65,  val: function (d) { return d.j || 0; } },                                // L  j
  { h: "Boiler (°C)",    w: 60,  f: "0.0", val: k_("k") },                                                // M  k
  { h: "Licht LDR",      w: 55,  val: function (d) { return d.l || 0; } },                                // N  l
  { h: "Nacht",          w: 45,  val: function (d) { return d.m || 0; } },                                // O  m
  { h: "Bed",            w: 40,  val: function (d) { return d.n || 0; } },                                // P  n
  { h: "R",              w: 35,  val: function (d) { return d.o || 0; } },                                // Q  o
  { h: "G",              w: 35,  val: function (d) { return d.p || 0; } },                                // R  p
  { h: "B",              w: 35,  val: function (d) { return d.q || 0; } },                                // S  q
  { h: "Pixels aan",     w: 110, val: function (d) { return d.r || "P=00000000"; } },                     // T  r (tekst, bewaart nullen)
  { h: "Pixel mode",     w: 70,  val: function (d) { return d.s || 0; } },                                // U  s
  { h: "MOV1",           w: 45,  val: function (d) { return d.t || 0; } },                                // V  t
  { h: "RSSI (dBm)",     w: 60,  val: function (d) { return d.u || 0; } },                                // W  u
  { h: "Heap vrij (KB)", w: 70,  val: function (d) { return d.v || 0; } },                                // X  v
  { h: "Heap blok (KB)", w: 70,  val: function (d) { return d.w || 0; } },                                // Y  w
  { h: "DS count",       w: 55,  val: function (d) { return d.x || 0; } },                                // Z  x
  { h: "Reset",          w: 80,  val: function (d) { return d.z || ""; } },                               // AA z
  { h: "Boots",          w: 50,  val: k_("A") },                                                          // AB A
  { h: "DHT reeks",      w: 55,  val: k_("B") },                                                          // AC B
  { h: "DHT fouten",     w: 55,  val: k_("C") },                                                          // AD C
  { h: "POST fouten",    w: 60,  val: k_("D") },                                                          // AE D
  { h: "Kelder (°C)",    w: 60,  f: "0.0", val: k_("E") },                                                // AF E
  { h: "Ketelpomp (°C)", w: 65,  f: "0.0", val: k_("F") },                                                // AG F
  { h: "Retour (°C)",    w: 60,  f: "0.0", val: k_("G") },                                                // AH G

  // ---- AI..AL: SCD41 en functies (v1.3) ----
  { h: "CO2 (ppm)",      w: 60,  f: "0",   val: k_("H") },  // AI H  CO2, leeg zonder sensor
  { h: "Vocht-bron",     w: 60,  f: "0",   val: k_("I") },  // AJ I  0 = DHT22, 1 = SCD41, 2 = laatste geldige
  { h: "Verwarming",     w: 70,  f: "0",   val: k_("J") },  // AK J  verwarming aanwezig (0/1)
  { h: "SWW-boiler",     w: 70,  f: "0",   val: k_("K") },  // AL K  SWW-boiler aanwezig (0/1)

  // ---- AM..BH: HomeWizard P1-meter (v1.3) ----
  { h: "P1 status",          w: 55, f: "0",     val: k_("pa") }, // AM pa 0 OK, 1 geen antwoord, 2 niet ingesteld, 3 ongeldige data
  { h: "Vermogen (W)",       w: 70, f: "0",     val: k_("pb") }, // AN pb active_power_w (negatief = injectie)
  { h: "L1 (W)",             w: 55, f: "0",     val: k_("pc") }, // AO pc active_power_l1_w
  { h: "L2 (W)",             w: 55, f: "0",     val: k_("pd") }, // AP pd active_power_l2_w (leeg bij 1 fase)
  { h: "L3 (W)",             w: 55, f: "0",     val: k_("pe") }, // AQ pe active_power_l3_w (leeg bij 1 fase)
  { h: "Afname dag (kWh)",   w: 80, f: "0.000", val: k_("pf") }, // AR pf total_power_import_t1_kwh
  { h: "Afname nacht (kWh)", w: 80, f: "0.000", val: k_("pg") }, // AS pg total_power_import_t2_kwh
  { h: "Injectie dag (kWh)", w: 80, f: "0.000", val: k_("ph") }, // AT ph total_power_export_t1_kwh
  { h: "Injectie nacht (kWh)",w: 80, f: "0.000", val: k_("pi") }, // AU pi total_power_export_t2_kwh
  { h: "Tarief",             w: 50, f: "0",     val: k_("pj") }, // AV pj active_tariff (1 = dag, 2 = nacht)
  { h: "Kwartier (W)",       w: 65, f: "0",     val: k_("pk") }, // AW pk active_power_average_w (capaciteitstarief)
  { h: "Maandpiek (W)",      w: 70, f: "0",     val: k_("pl") }, // AX pl montly_power_peak_w
  { h: "U L1 (V)",           w: 55, f: "0.0",   val: k_("pm") }, // AY pm active_voltage_l1_v
  { h: "U L2 (V)",           w: 55, f: "0.0",   val: k_("pn") }, // AZ pn active_voltage_l2_v
  { h: "U L3 (V)",           w: 55, f: "0.0",   val: k_("po") }, // BA po active_voltage_l3_v
  { h: "I L1 (A)",           w: 55, f: "0.00",  val: k_("pp") }, // BB pp active_current_l1_a
  { h: "I L2 (A)",           w: 55, f: "0.00",  val: k_("pq") }, // BC pq active_current_l2_a
  { h: "I L3 (A)",           w: 55, f: "0.00",  val: k_("pr") }, // BD pr active_current_l3_a
  { h: "Gas (m³)",           w: 75, f: "0.000", val: k_("ps") }, // BE ps total_gas_m3
  { h: "Gas tijd",           w: 125, val: function (d) { return gasTime_(d.pt); } }, // BF pt gas_timestamp
  { h: "Stroomuitval",       w: 65, f: "0",     val: k_("pu") }, // BG pu long_power_fail_count
  { h: "Dongle Wi-Fi (%)",   w: 65, f: "0",     val: k_("pv") }, // BH pv wifi_strength van de dongle

  // ---- BI..BK: watermeter (v1.3, uitleesmethode nog te bepalen) ----
  { h: "Water status",       w: 60, f: "0",     val: k_("wa") }, // BI wa 0 OK, 1 geen antwoord, 2 niet ingesteld
  { h: "Water (m³)",         w: 75, f: "0.000", val: k_("wb") }, // BJ wb meterstand
  { h: "Debiet (l/min)",     w: 65, f: "0.0",   val: k_("wc") }, // BK wc
];
const NUM_COLS = COLUMNS.length;  // 63 = A..BK

function getLogSheet_() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  const named = ss.getSheetByName(SHEET_NAME);
  if (named) return named;
  const grids = ss.getSheets().filter(function (s) { return s.getType() === SpreadsheetApp.SheetType.GRID; });
  if (grids.length === 0) throw new Error("Geen rasterblad gevonden");
  return grids[0];
}
function colFormats_() {
  return COLUMNS.map(function (c) { return c.f || "General"; });
}

function doPost(e) {
  try {
    const data = JSON.parse(e.postData.contents);
    const sheet = getLogSheet_();
    const timestamp = Utilities.formatDate(new Date(), "Europe/Brussels", "yyyy-MM-dd HH:mm:ss");

    const row = COLUMNS.map(function (c) { return c.val(data, timestamp); });
    sheet.appendRow(row);
    sheet.getRange(sheet.getLastRow(), 1, 1, NUM_COLS).setNumberFormats([colFormats_()]);

    // vorstalarm per mail; een fout hier mag het loggen nooit stoppen
    try { sjalayAlarmCheck(data); } catch (err) { Logger.log("Alarm fout: " + err); }

    // MAX_ROWS bewaking: verwijder oudste datarij als limiet overschreden
    if (sheet.getLastRow() - HEADER_ROWS > MAX_ROWS) {
      sheet.deleteRow(HEADER_ROWS + 1);
      Logger.log("MAX_ROWS (" + MAX_ROWS + ") bereikt — oudste rij verwijderd");
    }

    // Huisfilter: nieuw huis in de keuzelijst; actief filter ook over de nieuwe rij
    try {
      const lijstGewijzigd = addRoomToList_(sheet, data.room);
      const keuze = String(sheet.getRange(FILTER_CELL).getValue() || "ALL");
      if (keuze !== "ALL" || lijstGewijzigd) applyRoomFilter_(sheet, keuze);
    } catch (err) { Logger.log("Filter fout: " + err); }

    return ContentService
      .createTextOutput(JSON.stringify({
        status: "success", message: "Data gelogd", timestamp: timestamp,
        room: data.room || "?", uptime: data.a || 0
      }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    Logger.log("doPost fout: " + error.toString());
    return ContentService
      .createTextOutput(JSON.stringify({ status: "error", message: error.toString() }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

// ============================================================
// HUISFILTER — keuzelijst in C1 (zelfde werking als de ROOM-Sheet)
// ============================================================

// Simple trigger: draait vanzelf als iemand de keuzelijst wijzigt.
function onEdit(e) {
  if (!e || !e.range) return;
  const sheet = e.range.getSheet();
  if (sheet.getName() !== SHEET_NAME) return;
  if (e.range.getA1Notation() !== FILTER_CELL) return;
  applyRoomFilter_(sheet, String(e.value || "ALL"));
}

// Filter op kolom C: één huis tonen, of alles (ALL). Verbergt alleen rijen, wist niets.
function applyRoomFilter_(sheet, keuze) {
  const bestaand = sheet.getFilter();
  if (bestaand) bestaand.remove();
  if (!keuze || keuze === "ALL") return;
  const lastRow = Math.max(sheet.getLastRow(), HEADER_ROWS + 1);
  const range = sheet.getRange(HEADER_ROWS, 1, lastRow - HEADER_ROWS + 1, NUM_COLS);
  range.createFilter().setColumnFilterCriteria(ROOM_COL,
    SpreadsheetApp.newFilterCriteria().whenTextEqualTo(keuze).build());
}

// Huis toevoegen aan de keuzelijst als het er nog niet in staat. true = lijst gewijzigd.
function addRoomToList_(sheet, room) {
  if (!room || room === "?") return false;
  room = String(room);
  const props = PropertiesService.getDocumentProperties();
  const rooms = JSON.parse(props.getProperty("rooms") || "[]");
  if (rooms.indexOf(room) !== -1) return false;
  rooms.push(room);
  rooms.sort();
  props.setProperty("rooms", JSON.stringify(rooms));
  setRoomDropdown_(sheet, rooms);
  return true;
}

function setRoomDropdown_(sheet, rooms) {
  const cell = sheet.getRange(FILTER_CELL);
  cell.setDataValidation(SpreadsheetApp.newDataValidation()
    .requireValueInList(["ALL"].concat(rooms), true).setAllowInvalid(false).build());
  if (!cell.getValue()) cell.setValue("ALL");
}

// Eenmalig handig na de overstap: keuzelijst opbouwen uit alle room-namen in kolom C.
function rebuildRoomList() {
  const sheet = getLogSheet_();
  const n = sheet.getLastRow() - HEADER_ROWS;
  const rooms = [];
  if (n > 0) {
    sheet.getRange(HEADER_ROWS + 1, ROOM_COL, n, 1).getValues().forEach(function (r) {
      const v = String(r[0] || "");
      if (v && v !== "?" && rooms.indexOf(v) === -1) rooms.push(v);
    });
  }
  rooms.sort();
  PropertiesService.getDocumentProperties().setProperty("rooms", JSON.stringify(rooms));
  setRoomDropdown_(sheet, rooms);
  Logger.log("Keuzelijst: ALL, " + rooms.join(", "));
}

// ============================================================
// ALARM — vorstmail + "geen data"-melding
//  v1.3: alles PER HUIS (room-naam): eigen alarmtoestand, eigen "laatste POST".
//  1) sjalayAlarmCheck(data) wordt bij elke POST aangeroepen vanuit doPost.
//     Zakt kamer/boiler/kelder onder zijn grens -> mail; hoogstens 1 per ALARM_PAUSE_H
//     uur per alarm; boven grens + ALARM_HYST -> herstelmail.
//     v1.3: boileralarm enkel als de SWW-boiler aanwezig is (K = 1 of ontbreekt).
//  2) sjalayNoDataCheck() draait elk uur (tijd-trigger, installTrigger()) en meldt
//     als er NODATA_AFTER_H uur geen POST was; bij nieuwe data volgt een herstelmail.
// Lege of onbetrouwbare waarden (leeg of exact 0) geven géén alarm.
// v1.3: de room-naam (Sjalay / Ysermonde) staat in het onderwerp van elke mail.
// ============================================================
const ALARM_MAIL     = 'filip.delannoy@gmail.com';
const ALARM_PAUSE_H  = 6;     // uren tussen twee mails voor hetzelfde alarm
const NODATA_AFTER_H = 3;     // uren zonder POST voordat "geen data" gemeld wordt
const ALARM_HYST     = 1.5;   // °C boven de alarmgrens voor de herstelmelding
const ALARM_TZ       = 'Europe/Brussels';

// key = sleutel in de JSON (d = kamer, k = boiler, E = kelder); needs = enkel als die functie-key niet 0 is
const ALARM_SENSORS = [
  { id: 'kamer',  key: 'd', label: 'Kamer',  limit: 3 },
  { id: 'boiler', key: 'k', label: 'Boiler', limit: 4, needs: 'K' },
  { id: 'kelder', key: 'E', label: 'Kelder', limit: 2 }
];

function alarmNum_(v) {
  if (v === null || v === undefined || v === '') return null;
  const n = Number(String(v).replace(',', '.'));
  return isFinite(n) ? n : null;
}
function alarmFmt_(v) { return v === null ? '—' : v.toFixed(1).replace('.', ',') + ' °C'; }
function alarmNow_() { return Utilities.formatDate(new Date(), ALARM_TZ, 'yyyy-MM-dd HH:mm'); }
function alarmSend_(subject, body) { MailApp.sendEmail(ALARM_MAIL, subject, body); }
function roomKey_(room) { return String(room || '?').replace(/[^A-Za-z0-9_-]/g, '_'); }
function alarmValuesText_(vals) {
  return ALARM_SENSORS.filter(function (s) { return vals[s.id] !== undefined; })
    .map(function (s) { return s.label + ': ' + alarmFmt_(vals[s.id]); }).join('\n');
}

// ---------- 1) controle bij elke POST ----------
function sjalayAlarmCheck(data) {
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(5000)) return;
  try {
    const props = PropertiesService.getScriptProperties();
    const now = Date.now();
    const house = String(data.room || '?');
    const rk = roomKey_(house) + '_';                         // prefix: elk huis eigen toestand
    props.setProperty(rk + 'lastPost', String(now));
    props.setProperty(rk + 'name', house);

    if (props.getProperty(rk + 'nodata_active') === '1') {
      props.deleteProperty(rk + 'nodata_active');
      props.deleteProperty(rk + 'nodata_last');
      alarmSend_('✅ ' + house + ' stuurt weer gegevens', 'De controller stuurt weer gegevens (' + alarmNow_() + ').');
    }

    const vals = {};
    ALARM_SENSORS.forEach(function (s) {
      if (s.needs && data[s.needs] === 0) return;           // functie niet aanwezig: geen alarm
      vals[s.id] = alarmNum_(data[s.key]);
    });

    ALARM_SENSORS.forEach(function (s) {
      if (!(s.id in vals)) return;
      const v = vals[s.id];
      if (v === null || v === 0) return;                      // geen betrouwbare waarde
      const active = props.getProperty(rk + 'al_' + s.id) === '1';
      const last = Number(props.getProperty(rk + 'last_' + s.id) || 0);
      if (v < s.limit) {
        if (!active || (now - last) >= ALARM_PAUSE_H * 3600 * 1000) {
          alarmSend_('⚠️ ' + house + ': ' + s.label + ' ' + alarmFmt_(v) + ' (grens ' + s.limit + ' °C)',
            s.label + ' is onder ' + s.limit + ' °C gezakt (vorstgevaar).\n\n' + alarmValuesText_(vals) +
            '\n\nTijdstip: ' + alarmNow_() + '\nVolgende mail voor dit alarm na ' + ALARM_PAUSE_H + ' uur.');
          props.setProperty(rk + 'al_' + s.id, '1');
          props.setProperty(rk + 'last_' + s.id, String(now));
        }
      } else if (active && v >= s.limit + ALARM_HYST) {
        alarmSend_('✅ ' + house + ': ' + s.label + ' weer ' + alarmFmt_(v),
          s.label + ' is weer boven ' + (s.limit + ALARM_HYST).toString().replace('.', ',') + ' °C.\n\n' +
          alarmValuesText_(vals) + '\n\nTijdstip: ' + alarmNow_());
        props.deleteProperty(rk + 'al_' + s.id);
        props.deleteProperty(rk + 'last_' + s.id);
      }
    });
  } finally {
    lock.releaseLock();
  }
}

// ---------- 2) "geen data"-controle (tijd-trigger, elk uur), per huis ----------
function sjalayNoDataCheck() {
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(5000)) return;
  try {
    const props = PropertiesService.getScriptProperties();
    const all = props.getProperties();
    const now = Date.now();
    Object.keys(all).forEach(function (key) {
      if (!/_lastPost$/.test(key)) return;
      const rk = key.slice(0, -'lastPost'.length);           // bv. "Sjalay_"
      const house = all[rk + 'name'] || rk.slice(0, -1);
      const lastPost = Number(all[key] || 0);
      if (!lastPost) return;
      const hours = (now - lastPost) / 3600000;
      if (hours < NODATA_AFTER_H) return;
      const active = all[rk + 'nodata_active'] === '1';
      const last = Number(all[rk + 'nodata_last'] || 0);
      if (!active || (now - last) >= ALARM_PAUSE_H * 3600 * 1000) {
        const since = Utilities.formatDate(new Date(lastPost), ALARM_TZ, 'yyyy-MM-dd HH:mm');
        alarmSend_('⚠️ ' + house + ' stuurt geen gegevens meer',
          'Laatste gegevens ontvangen: ' + since + ' (' + Math.floor(hours) + ' uur geleden).\n' +
          'Mogelijke oorzaken: stroom, Wi-Fi/internet, een vastgelopen controller.\n\nTijdstip: ' + alarmNow_());
        props.setProperty(rk + 'nodata_active', '1');
        props.setProperty(rk + 'nodata_last', String(now));
      }
    });
  } finally {
    lock.releaseLock();
  }
}

// Huis niet meer opvolgen voor "geen data" (bv. controller definitief weg): Uitvoeren met de room-naam
// aanpassen in de regel hieronder.
function stopNoDataFor() {
  const house = 'NAAM_HIER';
  const rk = roomKey_(house) + '_';
  const props = PropertiesService.getScriptProperties();
  ['lastPost', 'name', 'nodata_active', 'nodata_last'].forEach(function (k) { props.deleteProperty(rk + k); });
}

// ---------- eenmalig uit te voeren ----------
function installTrigger() {   // maakt de uurlijkse "geen data"-trigger (verwijdert eerst een oude)
  ScriptApp.getProjectTriggers().forEach(function (t) {
    if (t.getHandlerFunction() === 'sjalayNoDataCheck') ScriptApp.deleteTrigger(t);
  });
  ScriptApp.newTrigger('sjalayNoDataCheck').timeBased().everyHours(1).create();
}
function testMail() {         // proefmail + vraagt eenmalig toestemming voor Mail
  alarmSend_('Vakantiehuizen alarm: proefmail', 'Dit is een proefmail van het alarmscript (' + alarmNow_() + ').\n\n' +
    'Grenzen: kamer ' + ALARM_SENSORS[0].limit + ' °C, boiler ' + ALARM_SENSORS[1].limit + ' °C, kelder ' + ALARM_SENSORS[2].limit + ' °C.');
}
function testAlarm() {        // simuleert kamer 2,5 / boiler 3,5 / kelder 1,5 °C voor huis "TEST": 3 alarmmails, wist daarna
  sjalayAlarmCheck({ room: 'TEST', d: 2.5, k: 3.5, E: 1.5 });
  resetAlarmState();
}
function resetAlarmState() {  // wist ALLE alarmtoestand (na tests); "geen data" start opnieuw bij de volgende POST
  PropertiesService.getScriptProperties().deleteAllProperties();
}

// ============================================================
// SETUP — voer eenmalig uit via Uitvoeren → setupHeaders (ook na elke update met nieuwe kolommen)
// Bestaande data blijft staan: enkel rij 1 (titel) en rij 2 (koppen) worden vervangen.
// ============================================================
function setupHeaders() {
  const sheet = getLogSheet_();
  const ss    = SpreadsheetApp.getActiveSpreadsheet();
  if (sheet.getFilter()) sheet.getFilter().remove();

  // Oude koprijen weg: rij 2 = kolomtitels; rij 1 = titel (oud: in A1, nieuw: in D1)
  if (sheet.getLastRow() >= 2 && sheet.getRange(2, 1).getValue() === "Tijdstempel") {
    sheet.deleteRow(2);
  }
  if (sheet.getLastRow() >= 1) {
    const a1 = String(sheet.getRange(1, 1).getValue() || "");
    const d1 = String(sheet.getRange(1, 4).getValue() || "");
    if (a1.indexOf("CONTROLLER DATA LOGGER") >= 0 || d1.indexOf("CONTROLLER DATA LOGGER") >= 0) sheet.deleteRow(1);
  }
  if (sheet.getMaxColumns() < NUM_COLS) sheet.insertColumnsAfter(sheet.getMaxColumns(), NUM_COLS - sheet.getMaxColumns());

  // Rij 1: A1 "Huisfilter →", C1 keuzelijst, D1 titel + URL
  sheet.insertRowBefore(1);
  sheet.getRange(1, 1, 1, sheet.getMaxColumns())
       .setBackground("#222222").setFontColor("#cccccc").setFontSize(9).setVerticalAlignment("middle");
  sheet.getRange("A1").setValue("Huisfilter →").setHorizontalAlignment("right").setFontStyle("italic");
  sheet.getRange(FILTER_CELL).setBackground("#ffffff").setFontColor("#000000").setFontSize(10)
       .setFontWeight("bold").setHorizontalAlignment("center");
  sheet.getRange("D1").setValue(TITLE + "  |  " + ss.getUrl()).setFontStyle("italic").setHorizontalAlignment("left");
  sheet.setRowHeight(1, 24);

  // Rij 2: kolomtitels
  const headers = COLUMNS.map(function (c) { return c.h; });
  sheet.insertRowBefore(2);
  const headerRange = sheet.getRange(2, 1, 1, NUM_COLS);
  headerRange.setValues([headers]);
  headerRange.setFontSize(10).setFontWeight("normal").setFontStyle("normal")
             .setFontColor("#ffffff").setBackground("#000000")
             .setHorizontalAlignment("center").setVerticalAlignment("middle").setWrap(true);
  sheet.setRowHeight(2, 40);

  COLUMNS.forEach(function (c, i) { sheet.setColumnWidth(i + 1, c.w || 60); });

  // Getalnotatie voor alle bestaande datarijen
  const lastRow = Math.max(sheet.getLastRow(), 3);
  COLUMNS.forEach(function (c, i) {
    if (c.f) sheet.getRange(3, i + 1, lastRow - 2, 1).setNumberFormat(c.f);
  });

  sheet.setFrozenRows(2);
  sheet.setFrozenColumns(1);
  rebuildRoomList();                         // keuzelijst uit de bestaande data (bv. "Sjalay")
  sheet.getRange(FILTER_CELL).setValue("ALL");
  Logger.log("Headers aangemaakt: " + NUM_COLS + " kolommen (A t/m BK). MAX_ROWS = " + MAX_ROWS);
}

// ============================================================
// TEST — simuleer een POST. Uitvoeren → test (Sjalay) of testYsermonde.
// Daarna de testrij(en) in het blad verwijderen.
// ============================================================
function test() {
  doPostTest_({
    room: "SJALAY", a: 3600, b: 1, c: 20, d: 20.4, e: 20.1, f: 58, g: 12.3, h: 0, i: 1, j: 40, k: 38.5,
    l: 72, m: 0, n: 0, o: 255, p: 128, q: 0, r: "P=10001000", s: 0, t: 1, u: -61, v: 142, w: 38, x: 2,
    y: 20.4, z: "POWERON", A: 7, B: 0, C: 3, D: 1, E: 14.2, F: null, G: null,
    H: 812, I: 0, J: 1, K: 1, pa: 2, wa: 2
  });
}
function testYsermonde() {
  doPostTest_({
    room: "YSERMONDE", a: 7200, b: 0, d: null, e: null, f: null, g: null, i: 0, k: null,
    u: -55, v: 150, w: 40, x: 0, z: "POWERON", A: 1, D: 0, J: 0, K: 0,
    pa: 0, pb: 412, pc: 412, pd: null, pe: null, pf: 1234.567, pg: 2345.678, ph: 0, pi: 0, pj: 1,
    pk: 380, pl: 2650, pm: 231.4, pn: null, po: null, pp: 1.85, pq: null, pr: null,
    ps: 456.789, pt: 261124230500, pu: 2, pv: 78, wa: 2
  });
}
function doPostTest_(obj) {
  const result = doPost({ postData: { contents: JSON.stringify(obj) } });
  Logger.log(result.getContent());
}
