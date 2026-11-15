# BioNova — Astronot Sağlık Paneli

**NASA Space Apps Challenge 2026 · Bursa · Görev: Create Your Own Health Monitoring**
Takım **BioNovaTR**: Eymen AKCA · Oğuz FENLİ · Ömer Abdullah DEMİR

Mars'ta yardım 20 dakika uzakta. BioNova, astronotun Dünya'yı beklemeden kendi sağlık verilerini görüp
ne yapacağını bilmesini sağlayan internetsiz bir panel ve giyilebilir sensördür.
Her uyarı kimin harekete geçeceğini söyler: **kendin uygula · ekibe bildir · uçuş cerrahına danış**.

## Dosyalar

| Dosya / klasör | Ne işe yarar |
|---|---|
| `BioNova_Panel.html` | Tek dosyalık panel (rol seçimli). Tarayıcıda çift tıklayıp açın, kurulum gerekmez. |
| `BioNova_Astronot.html` | Sadece astronot ekranı |
| `BioNova_YerKontrol.html` | Sadece yer kontrol ekranı (veriyi bağlantı gecikmesiyle alır) |
| `sunucu/` | İki ekranı ağ üzerinden bağlayan küçük Python sunucusu (port 8765) |
| `aktarici/` | Sunucu yoksa aynı Wi-Fi'deki iki bilgisayarı bağlayan yerel aktarıcı |
| `firmware/` | XIAO ESP32-S3 kodu, bağlantı şeması ve pertinaks yerleşimi |
| `medya/` | Tanıtım videosu ve fikir şeması |

## Çalıştırma

- **Sadece panel:** `BioNova_Panel.html` dosyasını Chrome veya Edge ile açın. Sensör bağlı değilken veriler "Simülasyon" etiketiyle gösterilir.
- **Sensörle (USB):** Arduino IDE ile `firmware/BioNova_S3/BioNova_S3.ino` dosyasını yükleyin (kütüphaneler: SparkFun MAX3010x, WebSockets). Bağlantılar `firmware/BAGLANTI.md` içinde.
- **İki ekran (sunucu):** `sunucu/` klasörüne üç HTML dosyasını da kopyalayın, sonra Ubuntu/Debian'da `bash kur.sh`. Ayrıntılar `sunucu/KURULUM.txt` içinde.
- **İki ekran (sunucusuz):** `aktarici/BASLAT.bat`, ayrıntılar `aktarici/NASIL_KULLANILIR.txt` içinde.

## Demoda ne gerçek

- **Canlı:** nabız, SpO₂ (MAX30102), hareket (MPU6050), NASA DONKI uzay havası
- **Elle girilen:** tansiyon, kan şekeri, uyku, ruh hali
- **Simülasyon:** EKG ve HRV, sıcaklık, solunum, radyasyon dozu, 30 günlük örnek geçmiş, 8 senaryo. Mars gecikmesi demoda 8 sn'ye kısaltıldı.
- Eşikler genel klinik aralıklardır; tıbbi cihaz değildir, teşhis için kullanılmaz.

## Kaynaklar ve lisanslar

**NASA:** DONKI uzay havası API'si (api.nasa.gov) · The Human Body in Space, HRP · Human System Risks ·
NASA-STD-3001 teknik notları (radyasyon OCHMO-TB-020, CO₂ OCHMO-TB-004, uyku) · NTRS 20230003916 ·
Risk of Spaceflight-Induced Bone Changes · Counteracting Bone and Muscle Loss ·
Space Faring: The Radiation Challenge · NASA 3D Resources EMU modeli.

**Üçüncü taraf:** BodyParts3D © DBCLS, CC BY-SA 2.1 JP · three.js + GLTFLoader, MIT ·
SparkFun MAX3010x, MIT · arduinoWebSockets, LGPL-2.1 · Oswald ve Inter yazı tipleri, SIL OFL.

Tanıtım videosundaki seslendirme yapay zekâ ile üretilmiştir.

**Kendi kodumuz:** MIT Lisansı — bkz. `LICENSE`. Üçüncü taraf içerikler kendi lisanslarıyla kalır.
