/* =====================================================================
   BioNova giyilebilir sensör · ESP32-S3 (Seeed XIAO ESP32-S3) · SADECE MPU6050
   MPU6050 hareket/duruş/adım/düşme. Nabız ve SpO2 sensörü (HW-605) arızalı
   olduğu için bu sürümde yok: panel nabız/SpO2/EKG'yi "Simülasyon" olarak gösterir.
   Panel protokolü: WebSocket ws://<ip>:81  ve  USB seri 115200 (JSON satırları)
     {"type":"imu",...} 50 Hz  · eğim, hareket yoğunluğu, adım, düşme
   Ekranlar arası mesajlar ({"topic":...}) bağlı diğer sayfalara aktarılır.

   Kütüphane (Arduino IDE > Library Manager):
     - "WebSockets" by Markus Sattler (LGPL-2.1)
   Kart: "XIAO_ESP32S3" (esp32 by Espressif, 3.x)  ·  USB CDC On Boot: Enabled
   ===================================================================== */
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>

// ---------- AYARLAR ----------
const char* WIFI_SSID = "TELEFON_HOTSPOT_ADI";   // demo ağı (telefon hotspot'u önerilir)
const char* WIFI_PASS = "HOTSPOT_SIFRESI";
const char* AP_SSID   = "BioNova-S3";            // Wi-Fi bulunamazsa kart kendi ağını açar
const char* AP_PASS   = "bionova2026";           // en az 8 karakter
#define I2C_SDA 5   // XIAO ESP32-S3: D4 = GPIO5
#define I2C_SCL 6   // XIAO ESP32-S3: D5 = GPIO6
#define LED_PIN LED_BUILTIN
// -----------------------------

WebSocketsServer ws(81);
bool imuOk = false;

/* ---------- MPU6050 (doğrudan I2C) ---------- */
const uint8_t MPU = 0x68;
float pitch = 0, roll = 0, yaw = 0, gbx = 0, gby = 0, gbz = 0, dynEma = 0, ax = 0, ay = 0, az = 1, mBase = 1.0f;  // mBase: sensörün durgun 1 g okuması (kalibrasyon sapmasını yok eder)
uint32_t steps = 0, lastStep = 0, stepT[32]; uint8_t stepI = 0;
uint32_t stillMs = 0, fallUntil = 0;

/* ---------- Mahony yön filtresi: kart nasıl takılırsa takılsın gerçek 3B yön (kuaterniyon) ---------- */
float qw = 1, qx = 0, qy = 0, qz = 0;
void qFromAccel(float x, float y, float z) {       // açılışta yerçekimine göre başlangıç yönü
  float r = atan2f(y, z) * 0.5f, p = atan2f(-x, sqrtf(y * y + z * z)) * 0.5f;
  float cr = cosf(r), sr = sinf(r), cp = cosf(p), sp = sinf(p);
  qw = cr * cp; qx = sr * cp; qy = cr * sp; qz = -sr * sp;
}
void mahony(float gx, float gy, float gz, float x, float y, float z, float dt) {   // g: rad/s, a: g
  const float KP = 1.2f;                           // ivmeyle düzeltme gücü (yüksek = sert, düşük = kayar)
  float n = sqrtf(x * x + y * y + z * z);
  if (n > 0.9f && n < 1.1f) {                    // sadece sakin anlarda yerçekimine güven: darbe/sarsıntı yönü bozmaz
    x /= n; y /= n; z /= n;
    float vx = 2 * (qx * qz - qw * qy), vy = 2 * (qw * qx + qy * qz), vz = qw * qw - qx * qx - qy * qy + qz * qz;
    gx += KP * (y * vz - z * vy); gy += KP * (z * vx - x * vz); gz += KP * (x * vy - y * vx);
  }
  gx *= 0.5f * dt; gy *= 0.5f * dt; gz *= 0.5f * dt;
  float a = qw, b = qx, c = qy;
  qw += -b * gx - c * gy - qz * gz; qx += a * gx + c * gz - qz * gy; qy += a * gy - b * gz + qz * gx; qz += a * gz + b * gy - c * gx;
  n = sqrtf(qw * qw + qx * qx + qy * qy + qz * qz); qw /= n; qx /= n; qy /= n; qz /= n;
}

void mpuWrite(uint8_t r, uint8_t v) { Wire.beginTransmission(MPU); Wire.write(r); Wire.write(v); Wire.endTransmission(); }
bool mpuRead(float& x, float& y, float& z, float& gx, float& gy, float& gz) {
  Wire.beginTransmission(MPU); Wire.write(0x3B); if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU, 14) != 14) return false;
  int16_t v[7]; for (int i = 0; i < 7; i++) v[i] = (Wire.read() << 8) | Wire.read();
  x = v[0] / 8192.0f; y = v[1] / 8192.0f; z = v[2] / 8192.0f; gx = v[4] / 65.5f - gbx; gy = v[5] / 65.5f - gby; gz = v[6] / 65.5f - gbz;
  return true;
}
bool mpuInit() {
  Wire.beginTransmission(MPU); if (Wire.endTransmission() != 0) return false;
  mpuWrite(0x6B, 0x00);  // uyandır
  mpuWrite(0x1A, 0x03);  // alçak geçiren filtre ~44 Hz
  mpuWrite(0x1B, 0x08);  // jiroskop ±500 °/s
  mpuWrite(0x1C, 0x08);  // ivme ±4 g
  delay(50);
  float x, y, z, gx, gy, gz;
  gbx = gby = gbz = 0;                           // jiroskop sıfır kalibrasyonu: kart sabitken 1 sn ölç, oynatılırsa tekrar dene
  for (int tries = 0; tries < 5; tries++) {
    float sx = 0, sy = 0, sz = 0, qq = 0; int n = 0;
    for (int i = 0; i < 200; i++) { if (mpuRead(x, y, z, gx, gy, gz)) { sx += gx; sy += gy; sz += gz; qq += gx * gx + gy * gy + gz * gz; n++; } delay(5); }
    if (!n) break;
    float mx = sx / n, my = sy / n, mz = sz / n, var = qq / n - (mx * mx + my * my + mz * mz);
    gbx = mx; gby = my; gbz = mz;
    if (var < 0.5f) { Serial.println("# Jiroskop kalibre edildi"); break; }
    Serial.println("# Kart oynadi, kalibrasyon tekrarlaniyor: sabit tut");
  }
  yaw = 0;
  if (mpuRead(x, y, z, gx, gy, gz)) {               // başlangıç eğimi ve 1 g tabanı
    mBase = sqrtf(x * x + y * y + z * z); qFromAccel(x, y, z);
    pitch = atan2f(-x, sqrtf(y * y + z * z)) * 57.2958f; roll = atan2f(y, z) * 57.2958f;
  }
  return true;
}
void imuStep(float dt) {
  float gx, gy, gz;
  if (!mpuRead(ax, ay, az, gx, gy, gz)) return;
  float aP = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2958f, aR = atan2f(ay, az) * 57.2958f;
  pitch = 0.98f * (pitch + gy * dt) + 0.02f * aP;   // jiroskop: hızlı ve pürüzsüz, ivme: uzun vadeli düzeltme
  roll  = 0.98f * (roll + gx * dt) + 0.02f * aR;
  if (fabsf(gz) > 0.6f) yaw += gz * dt;              // dönüş (pusula yok: zamanla az kayabilir)
  if (yaw > 180) yaw -= 360; else if (yaw < -180) yaw += 360;
  mahony(gx * 0.0174533f, gy * 0.0174533f, gz * 0.0174533f, ax, ay, az, dt);
  float m = sqrtf(ax * ax + ay * ay + az * az);
  static uint16_t calm = 0;                        // kart tamamen sabitken jiroskop sıfırını yavaşça güncelle: zamanla kayma olmaz
  if (fabsf(gx) < 2 && fabsf(gy) < 2 && fabsf(gz) < 2 && fabsf(m - 1) < 0.06f) { if (calm < 1000) calm++; } else calm = 0;
  if (calm > 200) { gbx += 0.002f * gx; gby += 0.002f * gy; gbz += 0.002f * gz; }
  mBase = mBase * 0.99875f + m * 0.00125f;          // yavaş taban: sensör 0,96 g da ölçse 0 hareket sayılır
  float dyn = fabsf(m - mBase);
  dynEma = dynEma * 0.975f + dyn * 0.025f;   // 200 Hz için eski 50 Hz zamanlaması
  uint32_t now = millis();
  if (m > mBase + 0.25f && now - lastStep > 280) { lastStep = now; steps++; stepT[stepI] = now; stepI = (stepI + 1) % 32; }
  if (m > mBase + 1.5f) fallUntil = now + 5000;   // ~2,5 g sert çarpma / düşme
  if (dynEma * 250 < 3) stillMs += (uint32_t)(dt * 1000); else stillMs = 0;
}
int spm() { uint32_t now = millis(); int n = 0; for (int i = 0; i < 32; i++) if (stepT[i] && now - stepT[i] < 10000) n++; return n * 6; }

/* ---------- IMU görevi: ESP32-S3'ün ayrı görevinde tam 200 Hz, Wi-Fi gecikmesinden etkilenmez ---------- */
void imuTask(void*) {
  TickType_t last = xTaskGetTickCount(); uint32_t tPrev = micros(), tRetry = 0;
  for (;;) {
    vTaskDelayUntil(&last, pdMS_TO_TICKS(5));
    uint32_t t = micros(); float dt = (t - tPrev) * 1e-6f; tPrev = t;
    if (!imuOk) { if (millis() - tRetry > 5000) { tRetry = millis(); imuOk = mpuInit(); if (imuOk) { Serial.println("# MPU6050 bulundu"); tPrev = micros(); } } continue; }
    if (dt > 0.05f) dt = 0.05f;
    imuStep(dt);
  }
}

/* ---------- gönderme ---------- */
bool usbJson = true;
void out(const char* s) { ws.broadcastTXT(s); if (usbJson && Serial) Serial.println(s); }   // powerbankte USB yokken seriye yazma

void sendImu() {
  int in = constrain((int)(dynEma * 250), 0, 100), sp = spm(), still = stillMs / 1000;
  const char* act = sp > 120 ? "kosu" : in > 25 ? "gecis" : still > 20 ? "hareketsiz" : in < 3 ? "dinlenme" : "istasyon";
  char b[340];
  snprintf(b, sizeof b, "{\"type\":\"imu\",\"p\":%.1f,\"r\":%.1f,\"y\":%.1f,\"q\":[%.4f,%.4f,%.4f,%.4f],\"int\":%d,\"spm\":%d,\"st\":%lu,\"still\":%d,\"act\":\"%s\",\"fall\":%d,\"a\":[%.2f,%.2f,%.2f]}",
           pitch, roll, yaw, qw, qx, qy, qz, in, sp, (unsigned long)steps, still, act, millis() < fallUntil ? 1 : 0, ax, ay, az);
  out(b);
}

/* ---------- WebSocket: ekranlar arası aktarma ---------- */
void onWs(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_CONNECTED) { Serial.printf("# ekran baglandi: %u\n", num); }
  else if (type == WStype_DISCONNECTED) { Serial.printf("# ekran ayrildi: %u\n", num); }
  else if (type == WStype_TEXT) {
    for (uint8_t i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++)
      if (i != num && ws.clientIsConnected(i)) ws.sendTXT(i, payload, len);
  }
}

void wifiStart() {
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("# Wi-Fi: %s baglaniliyor", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) { delay(300); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    // Sabit adres: ağ ne olursa olsun son hane hep .200 (ör. 192.168.43.200). Powerbankle her açılışta aynı adres.
    IPAddress gw = WiFi.gatewayIP(), mask = WiFi.subnetMask(), fix(gw[0], gw[1], gw[2], 200);
    WiFi.disconnect(); delay(100);
    WiFi.config(fix, gw, mask, gw); WiFi.begin(WIFI_SSID, WIFI_PASS);
    t0 = millis(); while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(200);
    if (WiFi.status() != WL_CONNECTED) {          // sabit adres tutmazsa normal (DHCP) adrese dön
      WiFi.disconnect(); delay(100); WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); WiFi.begin(WIFI_SSID, WIFI_PASS);
      t0 = millis(); while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) delay(200);
    }
    Serial.printf("# Panelde ESP32 adresi: %s  (veya bionova.local)\n", WiFi.localIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("# Wi-Fi yok. Kartin agi: %s / %s  ·  Panelde adres: %s\n", AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  }
  if (MDNS.begin("bionova")) MDNS.addService("ws", "tcp", 81);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  uint32_t t0 = millis(); while (!Serial && millis() - t0 < 1500) delay(10);
  Serial.println("# BioNova S3 basliyor (sadece MPU6050)");
  Wire.begin(I2C_SDA, I2C_SCL, 400000);   // sadece MPU: hızlı I2C
  Serial.println("# Kalibrasyon: karti 1 sn sabit tut");
  imuOk = mpuInit();
  Serial.println(imuOk ? "# MPU6050 bulundu" : "# HATA: MPU6050 bulunamadi (adres 0x68). Kart 5 sn'de bir yeniden deneyecek.");
  xTaskCreatePinnedToCore(imuTask, "imu", 4096, nullptr, 3, nullptr, 1);
  wifiStart();
  ws.begin(); ws.onEvent(onWs);
}

void loop() {
  ws.loop();
  static uint32_t tImuTx = 0, tLed = 0;
  uint32_t now = millis();
  if (imuOk && now - tImuTx >= 20) { tImuTx = now; sendImu(); }      // 50 Hz: 3B astronot gecikmesiz
  if (now - tLed >= (imuOk ? 1000 : 150)) { tLed = now; digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }   // yavaş: çalışıyor, hızlı: MPU yok
  while (Serial.available()) {                    // panel USB'de 'J' gonderir; 'S' JSON'u susturur
    char c = Serial.read(); if (c == 'J') usbJson = true; else if (c == 'S') usbJson = false;
  }
}
