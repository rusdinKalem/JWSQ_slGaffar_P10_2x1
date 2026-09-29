# JWSQ P10 2x1 OK - Jam Waktu Sholat Arduino P10 (2x1 Panel)

Program Jam Waktu Sholat (JWS) otomatis berbasis Arduino dengan tampilan LED Matrix P10 konfigurasi 2x1 (64x16 piksel), dilengkapi modul DFPlayer Mini untuk pemutar MP3 Tartil & Tarhim otomatis sebelum adzan, serta modul Bluetooth (HC-05 / HC-06 / ESP32) untuk konfigurasi melalui aplikasi Android **alGaffar**.

---

## 🕌 Fitur Utama
- **Tampilan LED Matrix P10**: Konfigurasi 2 panel (64x16 piksel) menggunakan pustaka DMD / DMD3asis.
- **Perhitungan Waktu Sholat Otomatis**: Berdasarkan koordinat lintang (*latitude*), bujur (*longitude*), dan zona waktu (GMT+7, GMT+8, GMT+9).
- **Jadwal Lengkap**: Subuh, Terbit, Dhuha, Dzuhur, Ashar, Maghrib, Isya, dan Imsak.
- **Modul DFPlayer Mini (MP3 Tartil & Tarhim)**:
  - Pemutaran murottal otomatis menjelang waktu sholat.
  - Pengaturan ON/OFF tartil untuk masing-masing waktu sholat.
  - Komunikasi Serial ke DFPlayer (Serial2).
- **Hitung Mundur Iqomah & Alarm Buzzer**:
  - Durasi jeda iqomah dapat diatur per waktu sholat.
  - Alarm buzzer saat masuk waktu sholat dan setelah iqomah selesai.
- **Running Text Informasi**:
  - Menampilkan Nama Masjid dan Alamat Masjid (`CMA`).
  - Berbagai teks pesan informasi / hadits berjalan.
  - Tanggal Masehi dan Kalender Hijriyah otomatis.
- **Kompatibel Penuh dengan Aplikasi Android `alGaffar`**:
  - Konfigurasi nirkabel melalui Bluetooth (HC-05, HC-06, ESP32).

---

## 📁 Struktur File

```text
├── JWSQ_P10_2x1_ok.ino     # Sketch utama Arduino (Setup, Loop, State Machine tampilan 2x1)
├── WS_CalculateTime.ino    # Perhitungan waktu sholat astronomis
├── WS_Drawing.ino          # Render grafis teks dan angka pada panel 64x16
├── WS_LoadPrm.ino          # Parsing protokol serial Bluetooth & EEPROM
├── WS_MP3.ino              # Driver komunikasi modul MP3 DFPlayer Mini
├── WS_NameIdx.ino          # Definisi konstanta indeks waktu sholat
├── .gitignore
└── README.md
```

---

## 🛠️ Kebutuhan Hardware
- **Mikrokontroler**: Arduino Mega 2560 (atau mikrokontroler dengan minimal 2 Hardware UART).
- **RTC**: DS3231 (I2C: SDA pin 20, SCL pin 21).
- **Bluetooth**: HC-05 / HC-06 (Serial1: RX1 pin 19, TX1 pin 18).
- **Audio MP3**: DFPlayer Mini (Serial2: RX2 pin 17, TX2 pin 16).
- **Display**: 2 Panel LED Matrix P10 Single Color (HUB12).

---

## 👤 Author
- **Rusdin** ([@rusdinKalem](https://github.com/rusdinKalem))
