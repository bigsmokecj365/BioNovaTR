# BioNova S3 · pertinaks devresi

Parçalar: Seeed XIAO ESP32-S3 · HW-605 (MAX30102) · GY-521 (MPU6050) ·
2 × 7'li dişi pin başlığı (XIAO için) · 2 × 4'lü dişi başlık ya da JST (modüller için) ·
100 nF seramik + 10 µF kondansatör (önerilir) · tek damarlı bakır tel.

## Bağlantılar (hepsi 3,3 V, ikisi de aynı I2C hattında)

| XIAO ESP32-S3 | HW-605 (MAX30102) | GY-521 (MPU6050) |
|---|---|---|
| 3V3           | VIN               | VCC              |
| GND           | GND               | GND, **AD0**     |
| D4 (GPIO5) SDA| SDA               | SDA              |
| D5 (GPIO6) SCL| SCL               | SCL              |
| –             | INT (boş)         | INT, XDA, XCL (boş) |

I2C adresleri çakışmaz: MAX30102 = 0x57, MPU6050 = 0x68 (AD0 GND'de).
5V pinini modüllere BAĞLAMA: hat 3,3 V kalsın.

## Pertinaks yerleşimi (üstten)

```
        3V3 rayı ═══════════════════════════════════
        GND rayı ═══════════════════════════════════
        SDA hattı ──────────────────────────────────
        SCL hattı ──────────────────────────────────

   ┌──── XIAO ESP32-S3 ────┐      ┌─ HW-605 ─┐   ┌─ GY-521 ─┐
   │ D0            5V      │      │ VIN ─3V3 │   │ VCC ─3V3 │
   │ D1            GND ─GND│      │ GND ─GND │   │ GND ─GND │
   │ D2            3V3 ─3V3│      │ SDA ─SDA │   │ SCL ─SCL │
   │ D3            D10     │      │ SCL ─SCL │   │ SDA ─SDA │
   │ D4 ─SDA       D9      │      └──────────┘   │ AD0 ─GND │
   │ D5 ─SCL       D8      │   [100nF+10µF       └──────────┘
   │ D6            D7      │    3V3–GND arası,
   └──── USB-C aşağıda ────┘    modüllerin yanına]
```

- XIAO'yu doğrudan lehimleme; dişi başlığa tak (bozulursa değişir).
- USB-C kenara baksın, kablo rahat takılsın.
- MPU6050'yi kartın üstüne sabit vida/sıcak silikonla tuttur: oynarsa duruş verisi titrer.
  Kartı göğse taktığında Y ekseni yukarı bakacak şekilde yerleştir.
- HW-605 kablosu kulak klipsine gidiyor; kabloyu pertinaksa bir kelepçe/silikonla sabitle.

## Güç
- Demo: USB-C → dizüstü ya da powerbank. Dizüstü şarjdayken de sorun yok
  (vücuda sadece optik sensör değiyor, elektrot yok).
- Kablosuz: 3,7 V LiPo → XIAO'nun arkasındaki BAT+/BAT− pedleri (kart şarj eder).

## Yükleme
1. Arduino IDE → Boards Manager: "esp32 by Espressif" (3.x) → kart: **XIAO_ESP32S3**.
   Tools → USB CDC On Boot: **Enabled**.
2. Library Manager: **SparkFun MAX3010x Pulse and Proximity Sensor Library**,
   **WebSockets** (Markus Sattler).
3. `BioNova_S3.ino` başındaki WIFI_SSID / WIFI_PASS'a telefon hotspot'unu yaz, yükle.
4. Seri Monitör (115200): `# MAX30102 bulundu`, `# MPU6050 bulundu` ve IP adresini görmelisin.
   Sonra Seri Monitörü KAPAT (panel USB'yi kullanabilsin).
5. Panel (astronot sayfası) → ESP32 düğmesi → IP adresini yaz (veya `bionova.local`).
   Aynı Wi-Fi'da olmak gerekir. Wi-Fi yoksa kart kendi ağını açar:
   `BioNova-S3` / `bionova2026`, adres `192.168.4.1`.
   Kablo ile: ESP32 penceresinde "USB" düğmesi (Chrome/Edge).

## Panelde ne görünür
- Parmak/kulak sensörde: canlı nabız, SpO₂ (4 sn sonra), nabız aralıklarından HRV (8 vuruştan sonra).
- Sensörden ayrılınca: "temas yok", panel simülasyona döner.
- Hareket: 3B astronot eğilir, hareket yoğunluğu, adım/dk, 20 sn hareketsizlik uyarısı,
  sert çarpma (2,5 g) → düşme uyarısı.
- HRV bu kartta EKG'den değil, optik nabız aralıklarından (PRV) hesaplanır.

## Sorun çözme
- "MAX30102 bulunamadı": SDA/SCL yer değişmiş olabilir. Bazı mor MAX30102 modüllerinde
  I2C pull-up dirençleri 1,8 V'a bağlıdır; hat kararsızsa modül üstündeki 1,8 V pull-up
  bağlantısını iptal etmek gerekir (GY-521'in 3,3 V pull-up'ları hattı taşır).
- "MPU6050 bulunamadı": AD0 GND'de mi, VCC 3V3'te mi?
- SpO₂ hep 0: parmağı sabit ve hafif bastırarak tut, ortam ışığını kapat; ilk 4–5 sn bekle.
- Nabız zıplıyor: kablo hareket ediyor; sensörü bantla sabitle.
- Panel bağlanmıyor: dizüstü ile kart aynı ağda mı? Sayfa https ile açıldıysa ws:// engellenir;
  sayfayı http (sunucu Tailscale adresi) veya dosyadan aç.
