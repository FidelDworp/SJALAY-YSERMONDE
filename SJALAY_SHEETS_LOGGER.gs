// ============================================================
// SJALAY CONTROLLER DATA LOGGER - Google Apps Script v1.1
// Ontvangt JSON-push van de Sjalay-controller (ESP32-C6) en logt
// naar Google Sheet. Gebaseerd op de ROOM CONTROLLER DATA LOGGER
// (v1.5) van het Zarlar-roomproject, aangepast aan het compacte
// Sjalay /json-schema v1.2 (verwarming + SWW i.p.v. enkel 1 circuit).
//
// v1.0 (1okt26): eerste versie voor Sjalay, 26 kolommen (A..Z).
// v1.1 (25okt26): 34 kolommen (A..AH). Nieuw: AA Reset-reden, AB Boots, AC DHT-reeks,
//                 AD DHT-fouten, AE POST-fouten, AF Kelder, AG Ketelpomp, AH Retour.
//                 Temperatuur-/vochtkolommen met notatie 0.0, "SWW pomp" als 0/1.
//                 JSON null -> lege cel (ongeldige meting wordt niet langer 0).
//                 Na het bijwerken: setupHeaders() eenmalig opnieuw uitvoeren.
//                 Kolom E "Set (°C)" logt voortaan het EFFECTIEVE setpoint (veld y,
//                 1 decimaal) i.p.v. het ingestelde setpoint (veld c).
//
// ⚠️  DEPLOYMENT INSTRUCTIE (BELANGRIJK):
//   Gebruik bij elke update: "Implementeren" → "Implementaties beheren"
//   → potlood-icoon (Bewerken) → versienummer verhogen → Implementeren
//   De URL blijft dan DEZELFDE → Settings op de Sjalay-controller
//   (Google Script-URL) hoeft niet aangepast te worden!
//   "Nieuwe implementatie" geeft ALTIJD een nieuwe URL → logging stopt
//   tot je de nieuwe URL ook in Settings invult.
//
// ============================================================

// ============================================================
// ⚙️  CONFIGURATIE — pas hier aan zonder de rest aan te raken
// ============================================================
const MAX_ROWS    = 50000;  // Maximum aantal datarijen (excl. headers)
                           // Oudste rijen worden verwijderd als limiet bereikt
                           // Pas aan naar wens (bv 500, 2000, 5000)
const HEADER_ROWS = 2;     // Rij 1 = titel+URL, Rij 2 = kolomtitels
                           // ⚠️ Niet aanpassen zonder ook setupHeaders() aan te passen
// ============================================================


const NUM_COLS = 34;       // A..AH

// Getalnotatie per kolom (index 0 = A). 0.0 = 1 decimaal, 0 = geheel getal
const COL_FORMATS = (function () {
  const f = [];
  for (let i = 0; i < NUM_COLS; i++) f.push("General");
  f[4] = "0.0";                                                          // E: Set = effectief setpoint
  f[0] = "yyyy-mm-dd hh:mm:ss";                                         // A: tijdstempel (anders toont Sheets een getal)
  [5, 6, 7, 8, 12, 31, 32, 33].forEach(function (i) { f[i] = "0.0"; }); // F,G,H,I,M,AF,AG,AH
  f[10] = "0";                                                           // K: SWW pomp
  return f;
})();

// null/undefined (ongeldige meting) -> lege cel; anders de waarde (ook 0 blijft 0)
function blank(v) {
  return (v === null || v === undefined) ? "" : v;
}

function doPost(e) {
  try {
    const data = JSON.parse(e.postData.contents);
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

    const timestamp = Utilities.formatDate(
      new Date(),
      "Europe/Brussels",
      "yyyy-MM-dd HH:mm:ss"
    );

    // pixelsAanStr: sketch stuurt "P=10001000" - geen pure cijferstring
    // -> appendRow() converteert niet naar getal -> leading zeros bewaard
    const pixelsAanStr = data.r || "P=00000000";

    const row = [
      timestamp,                 // A:  Tijdstempel
      data.a    || 0,            // B:  Uptime (s)
      data.room || "?",          // C:  Kamer
      data.b    || 0,            // D:  HEAT - ketelvraag (0/1)
      (data.y != null ? data.y : (data.c || 0)),  // E:  Set (°C) - EFFECTIEVE setpoint (incl. dauwpuntcorrectie, 1 dec.); terugval op ingestelde setpoint c
      blank(data.d),            // F:  Temp1 (°C) - kamertemperatuur (effectief gebruikt)
      blank(data.e),            // G:  Temp2 (°C) - DHT22 (rauw)
      blank(data.f),            // H:  Vocht (%)
      blank(data.g),            // I:  Dauwpt (°C)
      data.h    || 0,            // J:  Dew Alert (0/1)
      data.i ? 1 : 0,            // K:  SWW pomp (0/1)
      data.j    || 0,            // L:  SWW Set (°C) - boiler-setpoint
      blank(data.k),            // M:  Boiler (°C) - boilertemperatuur
      data.l    || 0,            // N:  Licht LDR (0-100)
      data.m    || 0,            // O:  Nacht (0/1) - donker genoeg
      data.n    || 0,            // P:  Bed (0/1)
      data.o    || 0,            // Q:  R (0-255)
      data.p    || 0,            // R:  G (0-255)
      data.q    || 0,            // S:  B (0-255)
      pixelsAanStr,              // T:  Pixels aan (bv "P=10001000")
      data.s    || 0,            // U:  Pixel mode (0=AUTO, 1=MANUEEL)
      data.t    || 0,            // V:  MOV1 trig/interval
      data.u    || 0,            // W:  RSSI (dBm)
      data.v    || 0,            // X:  Heap vrij (KB)
      data.w    || 0,            // Y:  Heap blok (KB)
      data.x    || 0,            // Z:  DS count
      data.z    || "",           // AA: Reset-reden (tekst, bv. POWERON / BROWNOUT)
      blank(data.A),             // AB: Boots (herstartteller)
      blank(data.B),             // AC: DHT-reeks (opeenvolgende mislukte lezingen)
      blank(data.C),             // AD: DHT-fouten (totaal)
      blank(data.D),             // AE: POST-fouten (totaal)
      blank(data.E),             // AF: Kelder (°C), leeg als geen sensor/ongeldig
      blank(data.F),             // AG: Ketelpomp (°C)
      blank(data.G),             // AH: Retour (°C)
    ];

    sheet.appendRow(row);
    // Getalnotatie op de zonet toegevoegde rij (1 decimaal voor temperaturen)
    sheet.getRange(sheet.getLastRow(), 1, 1, NUM_COLS).setNumberFormats([COL_FORMATS]);

    // MAX_ROWS bewaking: verwijder oudste datarij als limiet overschreden
    // Rij 1 = titelrij, Rij 2 = kolomtitels, datarijen starten op rij 3
    const dataRows = sheet.getLastRow() - HEADER_ROWS;
    if (dataRows > MAX_ROWS) {
      sheet.deleteRow(HEADER_ROWS + 1);  // verwijder oudste rij (eerste datarij)
      Logger.log("MAX_ROWS (" + MAX_ROWS + ") bereikt — oudste rij verwijderd");
    }

    return ContentService
      .createTextOutput(JSON.stringify({
        status:    "success",
        message:   "Sjalay data gelogd",
        timestamp: timestamp,
        room:      data.room || "?",
        uptime:    data.a    || 0
      }))
      .setMimeType(ContentService.MimeType.JSON);

  } catch (error) {
    Logger.log("doPost fout: " + error.toString());
    return ContentService
      .createTextOutput(JSON.stringify({
        status:  "error",
        message: error.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}


// ============================================================
// SETUP — voer eenmalig uit via Uitvoeren → setupHeaders
// ============================================================
function setupHeaders() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const ss    = SpreadsheetApp.getActiveSpreadsheet();

  // --- Verwijder bestaande koprijen als aanwezig ---
  // Controleer rij 2 eerst (kolomtitels), dan rij 1 (titelrij)
  // Altijd van onder naar boven verwijderen om rijnummers correct te houden
  if (sheet.getLastRow() >= 2) {
    const row2 = sheet.getRange(2, 1).getValue();
    if (row2 === "Tijdstempel") {
      sheet.deleteRow(2);
      Logger.log("Bestaande kolomtitelrij (rij 2) verwijderd.");
    }
  }
  if (sheet.getLastRow() >= 1) {
    const row1 = sheet.getRange(1, 1).getValue();
    if (typeof row1 === "string" && row1.startsWith("SJALAY")) {
      sheet.deleteRow(1);
      Logger.log("Bestaande titelrij (rij 1) verwijderd.");
    }
  }

  // --- Rij 1: Titelrij met scriptnaam en sheet-URL ---
  sheet.insertRowBefore(1);
  const titleCell = sheet.getRange(1, 1);
  titleCell.setValue("SJALAY CONTROLLER DATA LOGGER v1.1  |  " + ss.getUrl());
  titleCell.setFontSize(9);
  titleCell.setFontWeight("normal");
  titleCell.setFontStyle("italic");
  titleCell.setFontColor("#cccccc");
  titleCell.setBackground("#222222");
  titleCell.setHorizontalAlignment("left");
  titleCell.setVerticalAlignment("middle");
  sheet.setRowHeight(1, 24);

  // --- Rij 2: Kolomtitels ---
  const headers = [
    "Tijdstempel",    // A  - breed
    "Uptime (s)",     // B
    "Kamer",          // C  - breed, bevroren
    "HEAT",           // D
    "Set (°C)",       // E
    "Temp1 (°C)",     // F
    "Temp2 (°C)",     // G
    "Vocht (%)",      // H
    "Dauwpt (°C)",    // I
    "Dew Alert",      // J
    "SWW pomp",       // K
    "SWW Set (°C)",   // L
    "Boiler (°C)",    // M
    "Licht LDR",      // N
    "Nacht",          // O
    "Bed",            // P
    "R",              // Q
    "G",              // R
    "B",              // S
    "Pixels aan",     // T  - breed
    "Pixel mode",     // U
    "MOV1",           // V
    "RSSI (dBm)",     // W
    "Heap vrij (KB)", // X
    "Heap blok (KB)", // Y
    "DS count",       // Z
    "Reset",          // AA
    "Boots",          // AB
    "DHT reeks",      // AC
    "DHT fouten",     // AD
    "POST fouten",    // AE
    "Kelder (°C)",    // AF
    "Ketelpomp (°C)", // AG
    "Retour (°C)",    // AH
  ];

  sheet.insertRowBefore(2);
  const headerRange = sheet.getRange(2, 1, 1, headers.length);
  headerRange.setValues([headers]);

  // Opmaak: 10pt, wit op zwart, niet vet, niet italic, gecentreerd, wrap
  headerRange.setFontSize(10);
  headerRange.setFontWeight("normal");
  headerRange.setFontStyle("normal");
  headerRange.setFontColor("#ffffff");
  headerRange.setBackground("#000000");
  headerRange.setHorizontalAlignment("center");
  headerRange.setVerticalAlignment("middle");
  headerRange.setWrap(true);  // woorden wrappen → 2 regels mogelijk

  // Rijhoogte kolomtitelrij: genoeg voor 2 regels tekst op 10pt
  sheet.setRowHeight(2, 40);

  // Kolombreedtes
  sheet.setColumnWidth(1,  130);  // A: Tijdstempel
  sheet.setColumnWidth(2,   60);  // B: Uptime
  sheet.setColumnWidth(3,   80);  // C: Kamer
  sheet.setColumnWidth(4,   45);  // D: HEAT
  sheet.setColumnWidth(5,   55);  // E: Set (°C)
  sheet.setColumnWidth(6,   60);  // F: Temp1
  sheet.setColumnWidth(7,   60);  // G: Temp2
  sheet.setColumnWidth(8,   50);  // H: Vocht
  sheet.setColumnWidth(9,   60);  // I: Dauwpt
  sheet.setColumnWidth(10,  55);  // J: Dew Alert
  sheet.setColumnWidth(11,  55);  // K: SWW pomp
  sheet.setColumnWidth(12,  65);  // L: SWW Set
  sheet.setColumnWidth(13,  60);  // M: Boiler
  sheet.setColumnWidth(14,  55);  // N: Licht LDR
  sheet.setColumnWidth(15,  45);  // O: Nacht
  sheet.setColumnWidth(16,  40);  // P: Bed
  sheet.setColumnWidth(17,  35);  // Q: R
  sheet.setColumnWidth(18,  35);  // R: G
  sheet.setColumnWidth(19,  35);  // S: B
  sheet.setColumnWidth(20, 110);  // T: Pixels aan
  sheet.setColumnWidth(21,  70);  // U: Pixel mode
  sheet.setColumnWidth(22,  45);  // V: MOV1
  sheet.setColumnWidth(23,  60);  // W: RSSI
  sheet.setColumnWidth(24,  70);  // X: Heap vrij
  sheet.setColumnWidth(25,  70);  // Y: Heap blok
  sheet.setColumnWidth(26,  55);  // Z: DS count
  sheet.setColumnWidth(27,  80);  // AA: Reset
  sheet.setColumnWidth(28,  50);  // AB: Boots
  sheet.setColumnWidth(29,  55);  // AC: DHT reeks
  sheet.setColumnWidth(30,  55);  // AD: DHT fouten
  sheet.setColumnWidth(31,  60);  // AE: POST fouten
  sheet.setColumnWidth(32,  60);  // AF: Kelder
  sheet.setColumnWidth(33,  65);  // AG: Ketelpomp
  sheet.setColumnWidth(34,  60);  // AH: Retour

  // Getalnotatie voor alle bestaande datarijen (rij 3 t/m einde)
  const lastRow = Math.max(sheet.getLastRow(), 3);
  for (let c = 0; c < NUM_COLS; c++) {
    if (COL_FORMATS[c] !== "General") {
      sheet.getRange(3, c + 1, lastRow - 2, 1).setNumberFormat(COL_FORMATS[c]);
    }
  }

  // 2 rijen bevriezen: rij 1 (titel) + rij 2 (kolomtitels)
  sheet.setFrozenRows(2);
  sheet.setFrozenColumns(1);  // Alleen kolom A (Tijdstempel) bevroren

  Logger.log("Headers aangemaakt! " + headers.length + " kolommen (A t/m AH)");
  Logger.log("Rij 1 = titelrij | Rij 2 = kolomtitels | Data vanaf rij 3");
  Logger.log("MAX_ROWS instelling: " + MAX_ROWS);
}


// ============================================================
// TEST — simuleer een POST zoals de Sjalay-controller die stuurt
// Voer uit via Uitvoeren → test
// ============================================================
function test() {
  const testData = {
    postData: {
      contents: JSON.stringify({
        "room": "SJALAY",
        "a": 3600,
        "b": 1,
        "c": 20,
        "d": 20.4,
        "e": 20.1,
        "f": 58,
        "g": 12.3,
        "h": 0,
        "i": 1,
        "j": 40,
        "k": 38.5,
        "l": 72,
        "m": 0,
        "n": 0,
        "o": 255,
        "p": 128,
        "q": 0,
        "r": "P=10001000",
        "s": 0,
        "t": 1,
        "u": -61,
        "v": 142,
        "w": 38,
        "x": 2,
        "y": 20.4,
        "z": "POWERON",
        "A": 7,
        "B": 0,
        "C": 3,
        "D": 1,
        "E": 14.2,
        "F": null,
        "G": null
      })
    }
  };

  const result = doPost(testData);
  Logger.log(result.getContent());
}
