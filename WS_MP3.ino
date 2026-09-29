/*=====================================================================================
 * Modul MP3 DFPlayer Mini - Tartil & Tarhim Otomatis (JWS 2x1)
 * Dikontrol via Hardware Serial Pin D1 (TX) @ 9600 baud dengan sistem Auto-Relay.
 * Parameter disimpan pada EEPROM alamat 880-913 (termasuk durasi, menit mulai, track).
 *====================================================================================*/

enum Mp3State { MP3_IDLE, MP3_TARTIL, MP3_TARHIM, MP3_MANUAL };
Mp3State currentMp3State = MP3_IDLE;
int8_t currentMp3Prayer = -1;

// =========================================
// DFPlayer Mini Binary Command (10-byte) ==
// =========================================

void dfSendCmd(uint8_t cmd, uint8_t pHigh, uint8_t pLow) {
  uint16_t sum = 0xFF + 0x06 + cmd + 0x00 + pHigh + pLow;
  uint16_t checksum = -sum;
  uint8_t packet[10] = {
    0x7E, 0xFF, 0x06, cmd, 0x00, pHigh, pLow,
    (uint8_t)(checksum >> 8),
    (uint8_t)(checksum & 0xFF),
    0xEF
  };
  Serial.write(packet, 10);
}

void dfPlayFolder(uint8_t folder, uint8_t track) {
  dfSendCmd(0x0F, folder, track);
}

void dfPlayManual(uint8_t folder, uint8_t track) {
  dfPlayFolder(folder, track);
  currentMp3State = MP3_MANUAL;
  currentMp3Prayer = -1;
}

void dfStop() {
  dfSendCmd(0x16, 0, 0);
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

  for (uint8_t i = 0; i < 6; i++) {
    Mp3Prm.tartilMin[i] = 20;       // Default mulai Tartil: 20 menit sebelum adzan
    Mp3Prm.tartilTrack[i] = i + 1;   // Track 1 s.d. 6
    Mp3Prm.tarhimTrack[i] = 1;       // Track 1
    Mp3Prm.tarhimSec[i] = 390;       // Default durasi Tarhim: 390 detik (6 menit 30 detik)
  }

  EEPROM.put(ADDR_MP3_PRM, Mp3Prm);
}

void loadMp3Prm() {
  EEPROM.get(ADDR_MP3_PRM, Mp3Prm);
  if (Mp3Prm.version != MP3_PARAM_VERSION) {
    set_default_mp3_prm();
    EEPROM.get(ADDR_MP3_PRM, Mp3Prm);
  }
}

void saveMp3Prm() {
  EEPROM.put(ADDR_MP3_PRM, Mp3Prm);
}

// =========================================
// Inisialisasi & Helper Slot ==============
// =========================================

void mp3_init() {
  loadMp3Prm();
  delay(100);
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

  // Jika sedang fase adzan, iqomah, atau sholat, pastikan audio mati
  if (RunSel >= 99 && RunSel <= 104) {
    if (currentMp3State != MP3_IDLE) dfStop();
    return;
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
        dfPlayFolder(1, Mp3Prm.tartilTrack[slot]);
        currentMp3State = MP3_TARTIL;
        currentMp3Prayer = i;
      }
      return;
    }

    // 2. Fase Tarhim: diff <= tarhimDurSec dan diff > 0
    if (tarhimDurSec > 0 && diff <= (int32_t)tarhimDurSec && diff > 0) {
      if (currentMp3State != MP3_TARHIM || currentMp3Prayer != i) {
        dfPlayFolder(2, Mp3Prm.tarhimTrack[slot]);
        currentMp3State = MP3_TARHIM;
        currentMp3Prayer = i;
      }
      return;
    }
  }

  // Jika tidak ada sholat dalam jendela Tartil maupun Tarhim, pastikan audio otomatis dalam posisi STOP
  if (currentMp3State == MP3_TARTIL || currentMp3State == MP3_TARHIM) {
    dfStop();
  }
}
