/***************************************************
 * ESP32-WROOM FINAL
 * ABSENSI KARYAWAN
 * - UART trigger ke ESP32-CAM
 * - TANPA menerima link foto
 ***************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ================= WIFI =================
const char* ssid = "Infinix_HOT_50";
const char* pass = "12345678";

// ================= GOOGLE SCRIPT =================
String scriptURL = "https://script.google.com/macros/s/AKfycbwWlAQxAhdnQk1mzAfZhZI3BzyB5FFLo65J5ve_VBz3gdYLID47dNxh3DQ-YLemufuD/exec";

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= BUZZER =================
#define BUZZER_PIN 34

// ================= STATE =================
String npm = "";
String nama = "";
char mode = 0;

// ================= BUZZER =================
void buzzerOK() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(120);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerError() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(600);
  digitalWrite(BUZZER_PIN, LOW);
}

// ================= LCD =================
void lcdMsg(String l1, String l2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

// ================= KEYPAD =================
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= CEK NPM =================
bool cekNIK() {
  HTTPClient http;
  String url = scriptURL + "?mode=cek&npm=" + npm;

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  DynamicJsonDocument doc(256);
  deserializeJson(doc, http.getString());
  http.end();

  if (doc["status"] != "OK") return false;

  nama = doc["nama"].as<String>();
  return true;
}

// ================= TRIGGER FOTO (UART) =================
bool triggerCamUART() {
  Serial.println("CAPTURE");   // perintah ke ESP32-CAM

  unsigned long t = millis();
  while (millis() - t < 15000) {
    if (!Serial.available()) continue;

    String res = Serial.readStringUntil('\n');
    res.trim();

    if (res == "OK")  return true;
    if (res == "ERR") return false;
  }
  return false;
}

// ================= KIRIM ABSENSI =================
bool kirimAbsensi(String jenis) {
  String body =
    "mode=absen"
    "&npm=" + npm +
    "&nama=" + nama +
    "&jenis=" + jenis;

  HTTPClient http;
  http.begin(scriptURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int code = http.POST(body);
  String res = http.getString();
  http.end();

  return (code == 200 && res == "OK");
}

// ================= INPUT NIK =================
bool inputNPM() {
  npm = "";
  lcdMsg("INPUT NPM");

  unsigned long timeout = millis();
  while (millis() - timeout < 30000) {
    char k = keypad.getKey();
    if (!k) continue;

    timeout = millis();

    if (isdigit(k) && npm.length() < 16) {
      npm += k;
      lcd.setCursor(0, 1);
      lcd.print(npm);
    }

    if (k == '#') {
      if (npm.length() > 0) npm.remove(npm.length() - 1);
      lcdMsg("INPUT NPM", npm);
    }

    if (k == '*') {
      npm = "";
      lcdMsg("INPUT NPM");
    }

    if (k == 'D') {
      return (npm.length() > 0);
    }
  }
  return false;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);   // UART ke ESP32-CAM
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  WiFi.begin(ssid, pass);
  lcdMsg("CONNECT WIFI");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  lcdMsg("SELAMAT DATANG!!", "KLIK A UTK ABSEN");
}

// ================= LOOP =================
void loop() {
  char k = keypad.getKey();
  if (!k) return;

  if (k == 'A') {
    mode = k;

    if (!inputNPM()) return;

    lcdMsg("CEK NPM...");
    if (!cekNIK()) {
      lcdMsg("NPM TIDAK", "TERDAFTAR");
      buzzerError();
      delay(2000);
      lcdMsg("SELAMAT DATANG!!", "KLIK A UTK ABSEN");
      return;
    }

    lcdMsg("NAMA:", nama);
    delay(1500);

    lcdMsg("AMBIL FOTO");
    if (!triggerCamUART()) {
      lcdMsg("FOTO GAGAL");
      buzzerError();
    }
    else if (kirimAbsensi(mode == 'A' ? "MASUK" : "KELUAR")) {
      lcdMsg("ABSENSI OK");
      buzzerOK();
    }
    else {
      lcdMsg("ABSENSI BERHASIL");
      buzzerOK();
    }

    delay(2000);
    lcdMsg("SELAMAT DATANG!!", "KLIK A UTK ABSEN");
  }
}
