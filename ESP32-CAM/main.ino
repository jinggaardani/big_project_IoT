/****************************************************
 * ESP32-CAM FINAL
 * FOTO ON DEMAND via UART (RX TX)
 * Upload Google Drive (NO LINK RETURN)
 ****************************************************/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "Base64.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ================= CAMERA AI THINKER =================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN 4

// ================= WIFI =================
const char* ssid = "Infinix_HOT_50";
const char* password = "12345678";

// ================= GOOGLE SCRIPT =================
String deploymentID = "AKfycbwWlAQxAhdnQk1mzAfZhZI3BzyB5FFLo65J5ve_VBz3gdYLID47dNxh3DQ-YLemufuD";
String folderName   = "ESP32-CAM";

// ================= GLOBAL =================
WiFiClientSecure client;
bool LED_Flash_ON = true;

// =====================================================
bool uploadPhoto() {
  const char* host = "script.google.com";
  client.setInsecure();

  if (!client.connect(host, 443)) {
    return false;
  }

  if (LED_Flash_ON) digitalWrite(FLASH_LED_PIN, HIGH);
  delay(120);

  // 🔥 BUANG FRAME LAMA
  camera_fb_t *old_fb = esp_camera_fb_get();
  if (old_fb) esp_camera_fb_return(old_fb);
  delay(120);

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return false;

  digitalWrite(FLASH_LED_PIN, LOW);

  String url = "/macros/s/" + deploymentID + "/exec?folder=" + folderName;

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: script.google.com");
  client.println("Transfer-Encoding: chunked");
  client.println();

  int chunkSize = 3000;
  char *input = (char*)fb->buf;
  int fbLen = fb->len;
  char output[base64_enc_len(chunkSize)];

  for (int i = 0; i < fbLen; i += chunkSize) {
    int len = base64_encode(
      output,
      input,
      min(chunkSize, fbLen - i)
    );
    client.print(len, HEX); client.print("\r\n");
    client.print(output);   client.print("\r\n");
    input += chunkSize;
  }

  client.print("0\r\n\r\n");
  esp_camera_fb_return(fb);

  // cukup tunggu response (tidak dibaca)
  unsigned long t = millis();
  while (!client.available() && millis() - t < 8000) delay(10);
  client.stop();

  return true;
}

// =====================================================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);   // UART ke ESP32-WROOM
  delay(1000);

  pinMode(FLASH_LED_PIN, OUTPUT);

  // ===== WIFI =====
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  // ===== CAMERA =====
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SXGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  esp_camera_init(&config);

  Serial.println("READY"); // tanda siap
}

// =====================================================
void loop() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd == "CAPTURE") {
    bool ok = uploadPhoto();
    Serial.println(ok ? "OK" : "ERR");
  }
}
