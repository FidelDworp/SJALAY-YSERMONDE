// ============================================================
// SJALAY CONTROLLER DATA LOGGER - Google Apps Script v1.0
// Ontvangt JSON-push van de Sjalay-controller (ESP32-C6) en logt
// naar Google Sheet. Gebaseerd op de ROOM CONTROLLER DATA LOGGER
// (v1.5) van het Zarlar-roomproject, aangepast aan het compacte
// Sjalay /json-schema v1.2 (verwarming + SWW i.p.v. enkel 1 circuit).
//
// v1.0 (1okt26): eerste versie voor Sjalay, 26 kolommen (A..Z).
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
      data.c    || 0,            // E:  Set (°C) - verwarmings-setpoint
      data.d    || 0,            // F:  Temp1 (°C) - kamertemperatuur (effectief gebruikt)
      data.e    || 0,            // G:  Temp2 (°C) - DHT22 (rauw)
      data.f    || 0,            // H:  Vocht (%)
      data.g    || 0,            // I:  Dauwpt (°C)
      data.h    || 0,            // J:  Dew Alert (0/1)
      data.i    || 0,            // K:  SWW pomp (0/1)
      data.j    || 0,            // L:  SWW Set (°C) - boiler-setpoint
      data.k    || 0,            // M:  Boiler (°C) - boilertemperatuur
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
    ];

    sheet.appendRow(row);

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
  titleCell.setValue("SJALAY CONTROLLER DATA LOGGER v1.0  |  " + ss.getUrl());
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

  // 2 rijen bevriezen: rij 1 (titel) + rij 2 (kolomtitels)
  sheet.setFrozenRows(2);
  sheet.setFrozenColumns(1);  // Alleen kolom A (Tijdstempel) bevroren

  Logger.log("Headers aangemaakt! " + headers.length + " kolommen (A t/m Z)");
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
        "x": 2
      })
    }
  };

  const result = doPost(testData);
  Logger.log(result.getContent());
}
