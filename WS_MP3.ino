/*=====================================================================================
 * Modul MP3 DFPlayer Mini - Tartil & Tarhim Otomatis
 * Dikontrol via Pin A1 (TX) @ 9600 baud dengan sistem Auto-Relay.
 * Pin A1 dipilih karena 100% terisolasi dari HUB12 P10 (Pin 5/R), RTC (A4/A5), Buzzer (A0).
 * Pin D5 kini bebas murni untuk jalur Pin R (Data Red) panel LED P10.
 * Pin Hardware Serial D0 (RX) & D1 (TX) murni khusus untuk Bluetooth HC-05.
 * Parameter disimpan pada EEPROM alamat 880-913 (termasuk durasi, menit mulai, track).
 *====================================================================================*/

#define PIN_MP3_TX A1
#define PIN_RELAY  A2
#define RELAY_ACTIVE HIGH // Set HIGH untuk active-HIGH (atau LOW jika modul relay active-LOW)

enum Mp3State { MP3_IDLE, MP3_TARTIL, MP3_TARHIM, MP3_PRAYER_ACTIVE, MP3_MANUAL };
Mp3State currentMp3State = MP3_IDLE;
int8_t currentMp3Prayer = -1;
static uint32_t prayerMuteStartMs = 0;

int8_t getMp3Slot(uint8_t prayerIdx);

void relayInit() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, !RELAY_ACTIVE);
}

void relaySet(bool on) {
  digitalWrite(PIN_RELAY, on ? RELAY_ACTIVE : !RELAY_ACTIVE);
}

bool isRelayOn() {
  return digitalRead(PIN_RELAY) == RELAY_ACTIVE;
}

// =========================================
// DFPlayer Mini Binary Command (10-byte) ==
// =========================================

// Kirim 1 byte data serial 9600 baud presisi tinggi via Direct Port Register (A1 = PC1)
// Tanpa overhead digitalWrite (~5us), sehingga lebar bit akurat 104.1us (bebas framing error)
static void mp3SendByte(uint8_t b) {
  uint8_t oldSREG = SREG;
  cli();
  // Start bit (LOW)
  PORTC &= ~_BV(1);
  delayMicroseconds(104);
  // 8 Data bits (LSB first)
  for (uint8_t i = 0; i < 8; i++) {
    if (b & 1) PORTC |= _BV(1);
    else       PORTC &= ~_BV(1);
    delayMicroseconds(104);
    b >>= 1;
  }
  // Stop bit (HIGH) - gunakan 2 stop-bit (210 us) agar sinkronisasi frame 100% stabil
  PORTC |= _BV(1);
  delayMicroseconds(210);
  SREG = oldSREG;
}

void dfSendCmd(uint8_t cmd, uint8_t pHigh, uint8_t pLow) {
  // Set bit HIGH terlebih dahulu sebelum arah pin diubah menjadi OUTPUT
  // untuk mencegah glitch LOW (false start-bit) yang merusak byte pertama 0x7E
  PORTC |= _BV(1);
  DDRC  |= _BV(1);
  delayMicroseconds(1000); // Stabilisasi line IDLE HIGH selama 1ms sebelum paket dikirim

  uint16_t sum = 0xFF + 0x06 + cmd + 0x00 + pHigh + pLow;
  uint16_t checksum = -sum;
  uint8_t packet[10] = {
    0x7E, 0xFF, 0x06, cmd, 0x00, pHigh, pLow,
    (uint8_t)(checksum >> 8),
    (uint8_t)(checksum & 0xFF),
    0xEF
  };
  for (uint8_t i = 0; i < 10; i++) {
    mp3SendByte(packet[i]);
  }

  delayMicroseconds(1000); // Tahan HIGH kuat 1ms agar stop-bit terakhir selesai sempurna
  // Kembalikan pin ke INPUT_PULLUP saat IDLE untuk mencegah dengung (hum/buzz) dari ripple 5V
  DDRC  &= ~_BV(1);
  PORTC |= _BV(1);
}

void dfSetVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  dfSendCmd(0x06, 0, vol);
}

void dfPlayFolder(uint8_t folder, uint8_t track) {
  if (track == 0) track = (folder == 1) ? 3 : 2;

  boolean wasIdle = (currentMp3State == MP3_IDLE);

  // 1. Pastikan relay eksternal Arduino (Pin A2) aktif
  relaySet(true);

  if (wasIdle) {
    // Beri jeda 250ms saat baru menyala dari kondisi IDLE agar power stabil
    delay(250);
  }

  // 2. Pastikan volume diset ke level normal, lalu tunggu 120ms
  //    agar MCU DFPlayer selesai memproses command volume sebelum menerima command Play
  dfSetVolume(Mp3Prm.volume);
  delay(120);

  // 3. Langsung putar file target (Folder 01/02, Track) dengan 1 perintah bersih.
  //    PENTING:
  //    - Jangan kirim 0x16 (Stop) saat transisi Tartil -> Tarhim agar pin BUSY DFPlayer
  //      tetap LOW dan relay amplifier tidak terputus/cetek-cetek!
  //    - Jangan kirim 0x0F ganda berdekatan agar tidak menabrak proses baca SD Card!
  dfSendCmd(0x0F, folder, track);
  delay(100);
}

void dfPlayManual(uint8_t folder, uint8_t track) {
  relaySet(true);
  dfPlayFolder(folder, track);
  currentMp3State = MP3_MANUAL;
  currentMp3Prayer = -1;
}

// Pilih nomor track Tartil (Folder 01) yang pasti ada dan berdurasi panjang (~20-30 menit)
// untuk diputar secara bisu (Volume 0) sebagai penahan pin BUSY (keep-alive relay) selama sholat
static uint8_t getSilentKeepAliveTrack() {
  int8_t pIdx = (currentMp3Prayer >= 0) ? currentMp3Prayer : SholatNow;
  if (pIdx >= 0) {
    int8_t slot = getMp3Slot((uint8_t)pIdx);
    if (slot >= 0 && Mp3Prm.tartilTrack[slot] > 0) {
      return Mp3Prm.tartilTrack[slot];
    }
  }
  return 3; // Default 003.mp3 di Folder 01
}

// Senyapkan suara MP3 (Volume 0), putar file panjang Folder 01 secara bisu agar pin BUSY tetap LOW
// (relay 3-kabel tetap ON tanpa berkedip), dan pertahankan relay sampai selesai waktu tunggu sholat (blinkBlock).
void dfMuteForPrayer() {
  // Cegah eksekusi ganda apabila sudah berada dalam status MP3_PRAYER_ACTIVE
  if (currentMp3State == MP3_PRAYER_ACTIVE) return;

  // 1. Jaga pin relay eksternal Arduino (Pin A2) tetap ON
  relaySet(true);

  // 2. Senyapkan volume audio ke 0 terlebih dahulu agar mikrofon masjid jernih tanpa suara MP3
  dfSetVolume(0);
  delay(150);

  // 3. Putar file Tartil (Folder 01) dari awal dalam kondisi Volume 0 (bisu).
  //    PENTING:
  //    - Jangan gunakan command 0x08 (Root Single Repeat) karena pada SD Card yang menggunakan
  //      Folder 01 & 02, command 0x08 gagal mencari track di root dan membuat DFPlayer
  //      reset berulang-ulang sehingga relay berkedip-kedip lalu mati!
  //    - Dengan command 0x0F ke Folder 01 (file Tartil berdurasi panjang 20+ menit),
  //      DFPlayer berputar senyap (Volume 0) sehingga pin BUSY terkunci LOW dan relay tetap ON stabil
  //      sampai seluruh rangkaian Adzan, Iqomah, dan Waktu Tunggu Sholat (blinkBlock) selesai.
  uint8_t silentTrack = getSilentKeepAliveTrack();
  dfSendCmd(0x0F, 1, silentTrack);
  delay(150);

  currentMp3State = MP3_PRAYER_ACTIVE;
  prayerMuteStartMs = millis();
}

void dfOnAzzan() {
  if (Mp3Prm.enable == 0) return;
  if (currentMp3State != MP3_PRAYER_ACTIVE) {
    dfMuteForPrayer();
  }
}

// Matikan relay dan hentikan pemutaran MP3 secara total saat waktu tunggu sholat (blinkBlock) selesai
void dfStopPrayerRelay() {
  // 1. Matikan pin relay eksternal Arduino (Pin A2)
  relaySet(false);

  if (currentMp3State == MP3_IDLE) return;

  // 2. Pastikan volume tetap 0 sebelum di-stop agar tidak ada bocoran suara sedikit pun
  dfSetVolume(0);
  delay(100);

  // 3. Hentikan pemutaran DFPlayer (pin BUSY naik ke HIGH -> relay 3-kabel otomatis OFF)
  dfSendCmd(0x16, 0x00, 0x00);
  delay(150);
  dfSendCmd(0x16, 0x00, 0x00);
  delay(150);

  // 4. Setelah DFPlayer benar-benar STOP dan relay mati, kembalikan volume ke level normal
  //    sebagai pengaman ganda untuk jadwal Tartil/Tarhim waktu sholat berikutnya
  dfSetVolume(Mp3Prm.volume);
  delay(80);

  currentMp3State = MP3_IDLE;
  currentMp3Prayer = -1;
}

void dfStop() {
  relaySet(false);
  dfSendCmd(0x16, 0x00, 0x00); // Stop playback
  delay(150);
  dfSetVolume(Mp3Prm.volume);
  delay(80);
  currentMp3State = MP3_IDLE;
  currentMp3Prayer = -1;
}

// =========================================
// EEPROM Management =======================
// =========================================

void set_default_mp3_prm() {
  Mp3Prm.version = MP3_PARAM_VERSION;
  Mp3Prm.enable = 1;
  Mp3Prm.volume = 25;

  // 1. Waktu mulai tartil (menit sebelum adzan): semua waktu 20 menit
  for (uint8_t i = 0; i < 6; i++) {
    Mp3Prm.tartilMin[i] = 20;
  }

  // 2. Track Tartil (Folder 01): default 003.mp3 (Track 3) untuk semua waktu
  for (uint8_t i = 0; i < 6; i++) {
    Mp3Prm.tartilTrack[i] = 3;
  }

  // 3. Track Tarhim (Folder 02): default 002.mp3 (Track 2) untuk semua waktu
  for (uint8_t i = 0; i < 6; i++) {
    Mp3Prm.tarhimTrack[i] = 2;
  }

  // 4. Durasi Putar Tarhim (detik sebelum adzan): 7 Menit = 420 detik untuk semua waktu
  for (uint8_t i = 0; i < 6; i++) {
    Mp3Prm.tarhimSec[i] = 420;
  }

  EEPROM.put(ADDR_MP3_PRM, Mp3Prm);
}

void loadMp3Prm() {
  EEPROM.get(ADDR_MP3_PRM, Mp3Prm);
  if (Mp3Prm.version != MP3_PARAM_VERSION) {
    set_default_mp3_prm();
    EEPROM.get(ADDR_MP3_PRM, Mp3Prm);
  }

  // Proteksi nilai wajar agar track dan volume tidak bernilai 0
  if (Mp3Prm.volume == 0 || Mp3Prm.volume > 30) Mp3Prm.volume = 25;
  for (uint8_t i = 0; i < 6; i++) {
    if (Mp3Prm.tartilTrack[i] == 0) Mp3Prm.tartilTrack[i] = 3;
    if (Mp3Prm.tarhimTrack[i] == 0) Mp3Prm.tarhimTrack[i] = 2;
  }
}

void saveMp3Prm() {
  EEPROM.put(ADDR_MP3_PRM, Mp3Prm);
}

// =========================================
// Inisialisasi & Helper Slot ==============
// =========================================

void mp3_init() {
  relayInit();
  pinMode(PIN_MP3_TX, INPUT_PULLUP); // State IDLE serial UART adalah HIGH via internal pullup (anti-dengung)
  loadMp3Prm();
  delay(100);
  dfSendCmd(0x11, 0x00, 0x00); // Pastikan mode repeat mati saat boot
  delay(40);
  dfSetVolume(Mp3Prm.volume);
  delay(50);
  dfStop();
}

// Slot 0: Subuh, 1: Dzuhur, 2: Ashar, 3: Maghrib, 4: Isya, 5: Jum'at
int8_t getMp3Slot(uint8_t prayerIdx) {
  if (prayerIdx == 1) return 0; // Subuh
  if (prayerIdx == 4) {
    if (daynow == 5 && Prm.MT == 1) return 5; // Sholat Jum'at
    return 1; // Dzuhur
  }
  if (prayerIdx == 5) return 2; // Ashar
  if (prayerIdx == 6) return 3; // Maghrib
  if (prayerIdx == 7) return 4; // Isya
  return -1;
}

// =========================================
// Monitoring & Siklus Waktu MP3 ===========
// =========================================

void check_mp3() {
  if (Mp3Prm.enable == 0) {
    if (currentMp3State != MP3_IDLE) dfStop();
    return;
  }

  // Jika sedang fase adzan, iqomah, pesan sholat, atau waktu tunggu sholat (99 s.d. 104):
  if (azzan || (RunSel >= 99 && RunSel <= 104)) {
    // Pastikan audio MP3 dimute dan keep-alive Folder 01 berjalan agar relay tetap ON
    if (currentMp3State == MP3_TARTIL || currentMp3State == MP3_TARHIM) {
      dfMuteForPrayer();
    } else if (currentMp3State == MP3_PRAYER_ACTIVE) {
      // Refresh pemutaran bisu (Volume 0) setiap 10 menit sebelum file Folder 01 habis,
      // menjamin pin BUSY DFPlayer tidak pernah lepas (HIGH) sampai waktu tunggu sholat selesai.
      if ((uint32_t)(millis() - prayerMuteStartMs) >= 600000UL) {
        prayerMuteStartMs = millis();
        dfSetVolume(0);
        delay(120);
        dfSendCmd(0x0F, 1, getSilentKeepAliveTrack());
        delay(100);
      }
    }
    return;
  }

  // Jika waktu tunggu sholat (blinkBlock) sudah selesai dan kembali ke tampilan normal (RunSel < 99),
  // matikan relay dan DFPlayer (dengan pengaman jeda 5 detik agar tidak pernah terputus saat transisi ke Adzan):
  if (currentMp3State == MP3_PRAYER_ACTIVE && !azzan && RunSel >= 1 && RunSel < 99) {
    if ((uint32_t)(millis() - prayerMuteStartMs) >= 5000UL) {
      dfStopPrayerRelay();
    }
  }

  static uint32_t lastMp3CheckMs = 0;
  uint32_t currentMs = millis();
  if ((uint32_t)(currentMs - lastMp3CheckMs) < 1000UL) return;
  lastMp3CheckMs = currentMs;

  uint32_t currentSecondsToday = (uint32_t)now.hour() * 3600UL +
                                 (uint32_t)now.minute() * 60UL +
                                 (uint32_t)now.second();

  for (uint8_t i = 0; i < 8; i++) {
    if (i == 0 || i == 2 || i == 3) continue; // Lewati Imsak, Terbit, Dhuha

    int8_t slot = getMp3Slot(i);
    if (slot < 0) continue;

    uint16_t prayerMinute = (uint16_t)ceil((sholatT[i] * 60.0f) - 0.0001f);
    uint32_t prayerSeconds = (uint32_t)prayerMinute * 60UL;

    int32_t diff = (int32_t)prayerSeconds - (int32_t)currentSecondsToday;

    uint32_t tartilStartSec = (uint32_t)Mp3Prm.tartilMin[slot] * 60UL;
    uint32_t tarhimDurSec = (uint32_t)Mp3Prm.tarhimSec[slot];

    // Jika tartil diaktifkan, pastikan mulainya minimal sama atau lebih awal dari durasi tarhim
    if (tartilStartSec > 0 && tartilStartSec < tarhimDurSec) {
      tartilStartSec = tarhimDurSec;
    }

    // 1. Fase Tartil: diff <= tartilStartSec dan diff > tarhimDurSec
    if (tartilStartSec > 0 && diff <= (int32_t)tartilStartSec && diff > (int32_t)tarhimDurSec) {
      if (currentMp3State != MP3_TARTIL || currentMp3Prayer != i) {
        uint8_t tTrack = Mp3Prm.tartilTrack[slot];
        if (tTrack == 0) tTrack = 3;
        dfPlayFolder(1, tTrack);
        currentMp3State = MP3_TARTIL;
        currentMp3Prayer = i;
      }
      return;
    }

    // 2. Fase Tarhim: diff <= tarhimDurSec dan diff > 0
    if (tarhimDurSec > 0 && diff <= (int32_t)tarhimDurSec && diff > 0) {
      if (currentMp3State != MP3_TARHIM || currentMp3Prayer != i) {
        uint8_t hTrack = Mp3Prm.tarhimTrack[slot];
        if (hTrack == 0) hTrack = 2;
        dfPlayFolder(2, hTrack);
        currentMp3State = MP3_TARHIM;
        currentMp3Prayer = i;
      }
      return;
    }
  }

  // Jika berada di luar jendela waktu Tartil dan Tarhim (misalnya waktu Tarhim selesai tepat menjelang adzan):
  if (currentMp3State == MP3_TARTIL || currentMp3State == MP3_TARHIM) {
    // Jangan matikan relay: senyapkan audio (Volume 0) dan putar keep-alive Folder 01
    // agar relay tetap ON sampai waktu tunggu sholat (blinkBlock) selesai!
    dfMuteForPrayer();
  }
}
