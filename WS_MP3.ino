/*=====================================================================================
 * Modul MP3 DFPlayer Mini - Tartil & Tarhim Otomatis (JWS 2x1)
 * Dikontrol via Pin A1 (TX) @ 9600 baud dengan sistem Auto-Relay.
 * Pin A1 dipilih karena 100% terisolasi dari HUB12 P10 (Pin 5/R), RTC (A4/A5), Buzzer (A0).
 * Pin D5 kini bebas murni untuk jalur Pin R (Data Red) panel LED P10.
 * Pin Hardware Serial D0 (RX) & D1 (TX) murni khusus untuk Bluetooth HC-05.
 * Parameter disimpan pada EEPROM alamat 880-913 (termasuk durasi, menit mulai, track).
 *====================================================================================*/

#define PIN_MP3_TX A1
#define MP3_RELAY_DELAY_MS 5000UL // Jeda 5 detik untuk stabilisasi power relay amplifier

enum Mp3State { MP3_IDLE, MP3_TARTIL, MP3_TARHIM, MP3_MANUAL };
Mp3State currentMp3State = MP3_IDLE;
int8_t currentMp3Prayer = -1;

static boolean mp3UnmutePending = false;
static boolean mp3HalfVolDone = false;
static uint32_t mp3PlayStartMs = 0;

// =========================================
// DFPlayer Mini Binary Command (10-byte) ==
// =========================================

// Kirim 1 byte data serial 9600 baud bit-banging presisi tanpa membebani Hardware Serial D0/D1
static void mp3SendByte(uint8_t b) {
  uint8_t oldSREG = SREG;
  cli();
  // Start bit (LOW)
  digitalWrite(PIN_MP3_TX, LOW);
  delayMicroseconds(104);
  // 8 Data bits (LSB first)
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(PIN_MP3_TX, (b & 1) ? HIGH : LOW);
    delayMicroseconds(104);
    b >>= 1;
  }
  // Stop bit (HIGH)
  digitalWrite(PIN_MP3_TX, HIGH);
  delayMicroseconds(104);
  SREG = oldSREG;
}

void dfSendCmd(uint8_t cmd, uint8_t pHigh, uint8_t pLow) {
  pinMode(PIN_MP3_TX, OUTPUT);
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
  // Pasca-kirim: Kembalikan pin ke INPUT_PULLUP saat IDLE (tidak transmisi).
  // Mencegah injeksi tegangan 5V terus-menerus ke chip 3.3V DFPlayer yang menyebabkan
  // ripple switching PWM LED P10 bocor ke DAC audio dan memicu dengung (hum/buzz).
  pinMode(PIN_MP3_TX, INPUT_PULLUP);
}

void dfPlayFolder(uint8_t folder, uint8_t track) {
  if (track == 0) track = (folder == 1) ? 3 : 2;

  boolean alreadyPlaying = (currentMp3State != MP3_IDLE);

  // 1. Batalkan semua mode perulangan (repeat/loop) sebelumnya
  dfSendCmd(0x11, 0x00, 0x00);
  delay(40);

  // 2. Hentikan pemutaran sebelumnya agar chip DFPlayer siap menerima folder baru secara bersih
  dfSendCmd(0x16, 0x00, 0x00);
  delay(100);

  if (!alreadyPlaying) {
    // Solusi A: Mute volume terlebih dahulu hanya saat relay baru aktif menyalakan amplifier
    // dari kondisi IDLE agar tidak menimbulkan suara lonjakan/letupan (thump/pop).
    dfSetVolume(0);
    delay(40);
  } else {
    // Saat transisi Tartil -> Tarhim, amplifier sudah hidup dan stabil,
    // langsung set volume normal agar awal shalawat Tarhim tidak terpotong 5 detik
    dfSetVolume(Mp3Prm.volume);
    delay(40);
  }

  // 3. Putar file target (Folder 01/02, Track) secara bersih dengan konfirmasi ganda
  dfSendCmd(0x0F, folder, track);
  delay(60);
  dfSendCmd(0x0F, folder, track);
  delay(40);

  if (!alreadyPlaying) {
    mp3UnmutePending = true;
    mp3HalfVolDone = false;
    mp3PlayStartMs = millis();
  } else {
    mp3UnmutePending = false;
  }
}

void dfPlayManual(uint8_t folder, uint8_t track) {
  dfPlayFolder(folder, track);
  currentMp3State = MP3_MANUAL;
  currentMp3Prayer = -1;
}

void dfStop() {
  mp3UnmutePending = false;
  mp3HalfVolDone = false;
  dfSendCmd(0x11, 0, 0); // Batalkan mode loop
  delay(40);
  dfSendCmd(0x16, 0, 0); // Stop playback
  currentMp3State = MP3_IDLE;
  currentMp3Prayer = -1;
}

void dfSetVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  dfSendCmd(0x06, 0, vol);
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
    if (Mp3Prm.tarhimSec[i] == 0)   Mp3Prm.tarhimSec[i] = 420;
  }
}

void saveMp3Prm() {
  EEPROM.put(ADDR_MP3_PRM, Mp3Prm);
}

// =========================================
// Inisialisasi & Helper Slot ==============
// =========================================

void mp3_init() {
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
  // Non-blocking soft unmute: Cegah lonjakan arus mendadak pada amplifier
  // yang dapat menyebabkan drop tegangan 5V (brownout) pada panel LED P10
  if (mp3UnmutePending) {
    uint32_t elapsed = (uint32_t)(millis() - mp3PlayStartMs);
    if (elapsed >= (MP3_RELAY_DELAY_MS + 350UL)) {
      dfSetVolume(Mp3Prm.volume);
      mp3UnmutePending = false;
    } else if (elapsed >= MP3_RELAY_DELAY_MS && !mp3HalfVolDone) {
      uint8_t midVol = (Mp3Prm.volume > 10) ? (Mp3Prm.volume / 2) : Mp3Prm.volume;
      dfSetVolume(midVol);
      mp3HalfVolDone = true;
    }
  }

  if (Mp3Prm.enable == 0) {
    if (currentMp3State != MP3_IDLE || mp3UnmutePending) dfStop();
    return;
  }

  // Jika sedang fase adzan, iqomah, atau sholat, pastikan audio mati
  if (RunSel >= 99 && RunSel <= 104) {
    if (currentMp3State != MP3_IDLE || mp3UnmutePending) dfStop();
    return;
  }

  static uint32_t lastMp3CheckMs = 0;
  uint32_t currentMs = millis();
  if ((uint32_t)(currentMs - lastMp3CheckMs) < 1000UL) return;
  lastMp3CheckMs = currentMs;

  uint32_t currentSecondsToday = (uint32_t)now.hour() * 3600UL +
                                 (uint32_t)now.minute() * 60UL +
                                 (uint32_t)now.second();

  static uint8_t tarhimConfirmCount = 0;

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
        tarhimConfirmCount = 0;
      }
      return;
    }

    // 2. Fase Tarhim: diff <= tarhimDurSec dan diff > 0
    if (tarhimDurSec > 0 && diff <= (int32_t)tarhimDurSec && diff > 0) {
      uint8_t hTrack = Mp3Prm.tarhimTrack[slot];
      if (hTrack == 0) hTrack = 2;

      if (currentMp3State != MP3_TARHIM || currentMp3Prayer != i) {
        dfPlayFolder(2, hTrack);
        currentMp3State = MP3_TARHIM;
        currentMp3Prayer = i;
        tarhimConfirmCount = 1;
      } else if (tarhimConfirmCount > 0 && tarhimConfirmCount < 3) {
        // Konfirmasi ulang pada 2 detik awal fase Tarhim untuk menjamin 100% DFPlayer
        // telah beralih ke folder 02 (Tarhim) dan tidak tertinggal di folder 01 (Tartil)
        dfSendCmd(0x0F, 2, hTrack);
        tarhimConfirmCount++;
      }
      return;
    }
  }

  // Jika tidak ada sholat dalam jendela Tartil maupun Tarhim, pastikan audio otomatis dalam posisi STOP
  if (currentMp3State == MP3_TARTIL || currentMp3State == MP3_TARHIM) {
    tarhimConfirmCount = 0;
    dfStop();
  }
}
