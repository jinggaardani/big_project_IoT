/****************************************************
 * GOOGLE APPS SCRIPT FINAL FIX
 * ESP32-CAM + ESP32-WROOM
 * FOTO DRIVE + ABSENSI
 ****************************************************/

const SHEET_MAHASISWA = "MAHASISWA";
const SHEET_ABSENSI  = "ABSENSI";
const TZ = "Asia/Jakarta";
const PROP_LAST_IMAGE = "LAST_IMAGE_URL";

/****************************************************
 * doGet
 * mode=cek&npm=xxxxxxxx
 ****************************************************/
function doGet(e) {
  try {
    if (!e.parameter || e.parameter.mode !== "cek") {
      return json({ status: "ERROR", msg: "MODE_INVALID" });
    }

    const npm = String(e.parameter.npm || "").trim();
    if (!npm) {
      return json({ status: "ERROR", msg: "NPM_KOSONG" });
    }

    const sh = SpreadsheetApp.getActive()
      .getSheetByName(SHEET_MAHASISWA);
    if (!sh) {
      return json({ status: "ERROR", msg: "SHEET_MAHASISWA_TIDAK_ADA" });
    }

    const data = sh.getRange(2, 1, sh.getLastRow() - 1, 2).getValues();
    for (let i = 0; i < data.length; i++) {
      if (String(data[i][0]) === npm) {
        return json({
          status: "OK",
          npm: npm,
          nama: data[i][1]
        });
      }
    }

    return json({ status: "ERROR", msg: "NPM_TIDAK_TERDAFTAR" });

  } catch (err) {
    return json({ status: "ERROR", msg: err.message });
  }
}

/****************************************************
 * doPost
 * 1️⃣ ESP32-CAM : upload foto
 * 2️⃣ ESP32-WROOM : absensi
 ****************************************************/
function doPost(e) {
  try {

    /******************** MODE UPLOAD FOTO ********************/
    if (e.postData && e.postData.contents && !e.parameter.mode) {

      const name = Utilities.formatDate(
        new Date(), TZ, "yyyyMMdd-HHmmss"
      ) + ".jpg";

      const subFolderName = Utilities.formatDate(
        new Date(), TZ, "yyyyMMdd"
      );

      const folderName = e.parameter.folder || "ESP32-CAM";

      const bytes = Utilities.base64Decode(e.postData.contents);
      const blob  = Utilities.newBlob(bytes, "image/jpeg", name);

      let folder;
      const folders = DriveApp.getFoldersByName(folderName);
      folder = folders.hasNext()
        ? folders.next()
        : DriveApp.createFolder(folderName);

      let subFolder;
      const subs = folder.getFoldersByName(subFolderName);
      subFolder = subs.hasNext()
        ? subs.next()
        : folder.createFolder(subFolderName);

      const file = subFolder.createFile(blob);
      file.setSharing(
        DriveApp.Access.ANYONE_WITH_LINK,
        DriveApp.Permission.VIEW
      );

      const driveUrl =
        "https://drive.google.com/file/d/" +
        file.getId() + "/view";

      // 🔥 SIMPAN LINK FOTO TERAKHIR
      PropertiesService.getScriptProperties()
        .setProperty(PROP_LAST_IMAGE, driveUrl);

      return text("OK");
    }

    /******************** MODE ABSENSI ********************/
    if (e.parameter.mode !== "absen") {
      return text("ERROR");
    }

    const npm   = String(e.parameter.npm || "").trim();
    const nama  = String(e.parameter.nama || "").trim();
    const jenis = String(e.parameter.jenis || "").trim(); // MASUK 

    if (!npm || !nama || !jenis) {
      return text("ERROR");
    }

    // 🔥 AMBIL LINK FOTO TERAKHIR
    const imageUrl = PropertiesService
      .getScriptProperties()
      .getProperty(PROP_LAST_IMAGE);

    if (!imageUrl) {
      return text("ERROR");
    }

    const sh = SpreadsheetApp.getActive()
      .getSheetByName(SHEET_ABSENSI);
    if (!sh) return text("ERROR");

    const now     = new Date();
    const tanggal = Utilities.formatDate(now, TZ, "yyyy-MM-dd");
    const jam     = Utilities.formatDate(now, TZ, "HH:mm:ss");

    // ===== CEK DATA HARI INI =====
    let rowToday = -1;
    const lastRow = sh.getLastRow();

    if (lastRow >= 2) {
      const data = sh.getRange(2, 1, lastRow - 1, 7).getValues();
      for (let i = 0; i < data.length; i++) {
        if (data[i][0] === tanggal && String(data[i][1]) === nik) {
          rowToday = i + 2;
          break;
        }
      }
    }

    // ===== MASUK =====
    if (jenis === "MASUK") {
      if (rowToday !== -1) return text("ERROR");

      sh.appendRow([
        tanggal,   // A
        npm,       // B
        nama,      // C
        jam,       // D
        imageUrl,  // E
        ]);
    }

    return text("OK");

  } catch (err) {
    return text("ERROR");
  }
}

/****************************************************
 * HELPER
 ****************************************************/
function json(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}

function text(msg) {
  return ContentService
    .createTextOutput(msg)
    .setMimeType(ContentService.MimeType.TEXT);
}
