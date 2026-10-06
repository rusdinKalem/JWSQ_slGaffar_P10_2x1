/*************************************************************************************
  Waktu Sholat P10   - Program jam petunjuk waktu Sholat otomatis yang dilekapi
dengan Copyright (C) Des 2017 MFH Robotic. (info
https://www.facebook.com/MFH.Robotic/)

  Fitur :
  1. Perhitungan waktu sholat otomatis berdasarkan data geografis lokasi
    (Latitude, Longitude, Altitude, Time Zone dan Prameter pengaman/
    Ihtiyati)
  2. Fasilitas remote menggunakan aplikasi yang berjalan pada HP Android.
  3. Fasilitas running text informasi yang berjumlah 3 x 150 karakter.
    Running text ini cukup besar untuk di isi dengan hadis atau informasi
lainnya melalui fasilitas remote Aplikasi Android.
  4. Perhitungan Hari dan tanggal Hijriah secara otomatis dan perubahan
tanggalnya sesuai sesuai standar Hijriah yaitu setelah azzan magrib
  5. Fasilitas nama Masjid dan jenis masjid nya:
        a. Masjid
        b. Musholla
        c. Surau
        d. Langgar
  6. Pengingat waktu sholat mulai dari Azzan, menunggu iqomah dan mulai Sholat
    termasuk pengingat sholat Jumat jika parameter jenis nya di set sebagai
Masjid.
  7. Program ini sangat baik dipakai sebagai sarana belajar programing Arduino
karena menggunakan banyak librari dan teknik pemrogramn yang asik seperti:
       - Library yang digunakan adalah:
              #include <SPI.h>            --> komunikasi ke modul P10
              #include <DMD3.h>           --> library untuk modul P10
              #include <Wire.h>           --> komunikasi ke modul RTC DS3231
              #include <DS3231.h>         --> library modul RTC DS 3231
              #include <Timer.h>          --> library Timer untuk mengatur
timing tampilan #include <EEPROM.h>         --> library untuk penggunaan EEPROM
sebagai sarana penyimpan parameter #include <avr/pgmspace.h>   --> library untuk
penggunaan PROGMEM agar utilisasi SRAM bisa optimal
  8. Program dipecah menjadi 4 File sehingga maintenance bisa lebih mudah dan
aman dan tidak mengganggu bagian lain.

  Created by Wardi Utari
  30 Des 2017

  ---

  Progarm ini TIDAK ROYALTY FREE, program ini FREE jika dipergunakan untuk
    1. Jika dipergunakan di sekolah/lembaga latihan/perorangan untuk pendidikan
atau proses belajar
    2. Dipergunakan dalam proyek pembuatan Jam pengingat waktu sholat yang
disumbangkan ke masjid, musholla dll.

  Jika program ini dipergunakan secara komersial baik keseluruhan atau sebagian,
anda diwajibkan membayar ROYALTY FEE sebesar 2.5%. Royalty tersebut agar
dibayarkan ke Masjid terdekat dengan lokasi anda, dalam bentuk Sedekah yang
diniatkan untuk semua orang yang telah turut urun rembug dalam membuat dan
  menyempurnakan program ini.

  Semoga Bermanfaat
  Salam Mujahid 212.

**************************************************************************************/
#include <BigNumber.h>
#include <DMD3asis.h>
#include <DS3231.h>
#include <EEPROM.h>
#include <Font3x7.h>
#include <SPI.h>
#include <TimerOne.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <font/Font4x6.h>
#include <font/Font5x6.h>
#include <font/SystemFont5x7.h>

#define BUZZ A0
#define Font0 Font4x6
#define Font1 SystemFont5x7
#define Font2 Font5x6
#define Font3 BigNumber
#define Font4 Font3x7

// Object Declarations
DMD3 Disp(2, 1);

RTClib RTC;
DS3231 Clock;

// Structure of Variable
typedef struct // loaded to EEPROM
{
  uint8_t state; // 1 byte  add 0
  float L_LA;    // 4 byte  add 1
  float L_LO;    // 4 byte  add 5
  float L_AL;    // 4 byte  add 9
  float L_TZ;    // 4 byte  add 13
  uint8_t
      MT; // 1 byte  add 17  // value 1-masjid  2-mushollah 3-surau 4-langgar
  uint8_t BL; // 1 byte  add 18
  uint8_t RT; // 1 byte  add 19
  uint8_t IH; // 1 byte  add 20
  uint8_t AD; // 1 byte  add 21
  uint8_t SO; // 1 byte  add 22
  uint8_t JM; // 1 byte  add 23
  uint8_t I1; // 1 byte  add 24
  uint8_t I4; // 1 byte  add 25
  uint8_t I5; // 1 byte  add 26
  uint8_t I6; // 1 byte  add 27
  uint8_t I7; // 1 byte  add 28
  uint8_t BZ; // 1 byte  add 29
  uint8_t SI; // 1 byte  add 30
  uint8_t ST; // 1 byte  add 31
  uint8_t SU; // 1 byte  add 32
  int8_t IS;  // 1 byte  add 33
  int8_t IL;  // 1 byte  add 34
  int8_t IA;  // 1 byte  add 35
  int8_t IM;  // 1 byte  add 36
  int8_t II;  // 1 byte  add 37
  int8_t CH;  // 1 byte  add 38
  uint8_t IN; // 1 byte  add 39
} struct_param;

typedef struct {
  uint8_t hD;
  uint8_t hM;
  uint16_t hY;
} hijir_date;

// Variable by Structure
struct_param Prm;
hijir_date nowH;

#define ADDR_MP3_PRM 880
#define MP3_PARAM_VERSION 103

typedef struct {
  uint8_t version;        // 1 byte  add 880
  uint8_t enable;         // 1 byte  add 881
  uint8_t volume;         // 1 byte  add 882
  uint8_t tartilMin[6];   // 6 byte  add 883-888 (Subuh, Dzuhur, Ashar, Maghrib, Isya, Jumat)
  uint8_t tartilTrack[6]; // 6 byte  add 889-894
  uint8_t tarhimTrack[6]; // 6 byte  add 895-900
  uint16_t tarhimSec[6];  // 12 byte add 901-912
} struct_mp3_prm;

struct_mp3_prm Mp3Prm;

void dfStop();
void dfSetVolume(uint8_t vol);
void dfPlayManual(uint8_t folder, uint8_t track);
void startBuzzer(uint8_t count);
void stopBuzzer();
void serviceBuzzer();
bool isBuzzerActive();
void Buzzer(uint8_t state);

// Alamat 1022-1023 bebas (area EMPTY: 880-1023)
#define ADDR_JUMAT 1022
#define ADDR_RUNSEL 1023

// Time Variable
DateTime now;
float floatnow = 0;
uint8_t daynow = 0;
uint8_t ty_puasa = 0;
uint8_t hd_puasa = 0;
int8_t SholatNow = -1;
boolean jumat = false;
boolean azzan = false;
uint8_t reset_x = 0;
boolean rtcTimeValid = false;
boolean displayReady = false;
uint32_t lastRtcReadMs = 0;

// Other Variable
float sholatT[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t Iqomah[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Blue tooth Pram Receive
char CH_Prm[155];
int DWidth = Disp.width();
int DHeight = Disp.height();
boolean DoSwap;
int RunSel = 1; //
int RunFinish = 0;
uint8_t buzzCount = 0;
uint32_t buzzLastMs = 0;
boolean buzzToneActive = false;

void stopBuzzer() {
  buzzCount = 0;
  buzzToneActive = false;
  noTone(BUZZ);
  digitalWrite(BUZZ, LOW);
}

void startBuzzer(uint8_t count) {
  if (Prm.BZ != 1 || count == 0) {
    stopBuzzer();
    return;
  }
  buzzCount = count;
  buzzToneActive = true;
  buzzLastMs = millis();
  digitalWrite(BUZZ, HIGH);
  tone(BUZZ, 2500);
}

void serviceBuzzer() {
  if (buzzCount == 0) {
    if (buzzToneActive) {
      noTone(BUZZ);
      digitalWrite(BUZZ, LOW);
      buzzToneActive = false;
    }
    return;
  }

  uint32_t currentMs = millis();
  if (buzzToneActive) {
    // Sedang berbunyi selama 300ms
    if ((uint32_t)(currentMs - buzzLastMs) >= 300UL) {
      noTone(BUZZ);
      digitalWrite(BUZZ, LOW);
      buzzToneActive = false;
      buzzLastMs = currentMs;
      buzzCount--;
    }
  } else {
    // Jeda hening 300ms antar bunyi
    if ((uint32_t)(currentMs - buzzLastMs) >= 300UL) {
      if (buzzCount > 0 && Prm.BZ == 1) {
        buzzToneActive = true;
        buzzLastMs = currentMs;
        digitalWrite(BUZZ, HIGH);
        tone(BUZZ, 2500);
      } else {
        buzzCount = 0;
      }
    }
  }
}

bool isBuzzerActive() {
  return (buzzCount > 0 || buzzToneActive);
}

//=======================================
//===SETUP===============================
//=======================================
void setup() { // init comunications
  MCUSR = 0;
  wdt_disable();
  Wire.begin();
  Serial.begin(9600);
  pinMode(BUZZ, OUTPUT);
  stopBuzzer();
  startBuzzer(1);
  delay(2000);
  updateTime();
  GetPrm();
  mp3_init();

  RunSel = 1;
  jumat = false;
  EEPROM.update(ADDR_RUNSEL, 1);
  EEPROM.update(ADDR_JUMAT, 0);

  Disp_init();
  update_All_data();
  startBuzzer(2);
  wdt_enable(WDTO_4S);
}

//=======================================
//===MAIN LOOP Function =================
//=======================================
void loop() {
  wdt_reset();
  serviceBluetooth();
  serviceBuzzer();

  // Reset & Init Display State
  updateTime();  // maksimal satu pembacaan RTC per detik
  check_mp3();   // monitoring Tartil & Tarhim
  check_azzan(); // check Sholah Time for Azzan
  DoSwap = false;
  fType(1);
  Disp.clear();

  // Timer Function every 10 Minutes
  // Up All function with Timer in this fuction
  Timer_Minute(1);

  // =========================================
  // List of Display Component Block =========
  // =========================================

  static boolean firstBoot = true;
  if (firstBoot) {
    anim_SALAM(1);
    if (RunFinish == 1)
      firstBoot = false;
  } else {
    if (RunSel == 1)
      RunSel = 2;
  }

  if (RunSel == 2)
    dwMrq(drawWelcome(), int(Prm.RT), 0, 2);
  if (RunSel == 3)
    dwMrq(drawDateH(), int(Prm.RT), 3, 3);
  if (RunSel == 4)
    dwMrq(drawDateM(), int(Prm.RT), 3, 4);
  if (RunSel == 5)
    dwMrq(drawInfo(130), int(Prm.RT), 0, 5);
  drawSholat(6);
  drawSholat(7);
  if (RunSel == 8)
    dwMrq(drawInfo(280), int(Prm.RT), 1, 8);
  drawSholat(9);
  drawSholat(10);
  if (RunSel == 11)
    dwMrq(drawInfo(430), int(Prm.RT), 2, 11);
  drawSholat(12);
  drawSholat(13);

  drawAzzan(100);  // addr: 100 show Azzan
  drawIqomah(101); // addr: 101 show Iqomah
  if (RunSel == 102)
    dwMrq(drawInfo(580), int(Prm.RT), 0, 102);
  if (RunSel == 103)
    dwMrq(drawInfo(730), int(Prm.RT), 0, 103);
  blinkBlock(104); // addr: 104 show Blink  Sholat

  // =========================================
  // Display Control Block ===================
  // =========================================
  switch (RunFinish) {
  case 1:
    setRunSel(2);
    break;
  case 2:
    setRunSel(3);
    break;
  case 3:
    setRunSel(4);
    break;
  case 4:
    setRunSel(5);
    break;
  case 5:
    setRunSel(6);
    break;
  case 6:
    setRunSel(7);
    break;
  case 7:
    setRunSel(8);
    break;
  case 8:
    setRunSel(9);
    break;
  case 9:
    setRunSel(10);
    break;
  case 10:
    setRunSel(11);
    break;
  case 11:
    setRunSel(12);
    break;
  case 12:
    setRunSel(13);
    break;
  case 13:
    setRunSel(1);
    break;
  case 99:
    setRunSel(100);
    break;
  case 100:
    if (jumat) {
      setRunSel(103);
      reset_x = 1;
    } else {
      setRunSel(101);
    }
    break;
  case 101:
    setRunSel(102);
    reset_x = 1;
    break;
  case 102:
    setRunSel(104);
    break;
  case 103:
    setRunSel(104);
    break;
  case 104:
    setRunSel(1);
    reset_x = 1;
    break;
  default:
    break;
  }
  RunFinish = 0;

  // =========================================
  // Swap Display if Change===================
  // =========================================
  if (DoSwap) {
    Disp.swapBuffers();
  } // Swap Buffer if Change
}

// =========================================
// DMD3 P10 utility Function================
// =========================================

void setRunSel(int val) {
  RunSel = val;
}

void setJumat(bool val) {
  jumat = val;
}

void Disp_init() {
  Disp.setDoubleBuffer(true);
  Timer1.initialize(2000);
  Timer1.attachInterrupt(scan);
  displayReady = true;
  setBrightness(int(Prm.BL));
  fType(1);
  Disp.clear();
  Disp.swapBuffers();
}

void setBrightness(int bright) {
  if (bright < 15) bright = 15;
  if (bright > 1023) bright = 1023;
  Timer1.pwm(9, bright);
}

void scan() { Disp.refresh(); }

// =========================================
// Time Calculation Block===================
// =========================================
// Sakamoto's algorithm: menghitung hari dari tanggal secara mandiri (0=Minggu, 1=Senin, ..., 6=Sabtu)
static uint8_t calcDayOfWeek(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

void updateTime() {
  const uint32_t currentMs = millis();
  if (rtcTimeValid && (uint32_t)(currentMs - lastRtcReadMs) < 1000UL) {
    return;
  }

  now = RTC.now();
  floatnow =
      (float)now.hour() + (float)now.minute() / 60 + (float)now.second() / 3600;
  daynow = ((calcDayOfWeek(now.year(), now.month(), now.day()) + 6) % 7) + 1;
  lastRtcReadMs = currentMs;
  rtcTimeValid = true;
}

void Timer_Minute(int repeat_time) // load every  1 minute
{
  static uint32_t lsRn;
  uint32_t Tmr = millis();
  if ((Tmr - lsRn) > ((uint32_t)repeat_time * 60000UL)) {
    lsRn = Tmr;
    update_All_data();
  }
}

void update_All_data() {
  uint8_t date_cor = 0;
  updateTime();
  sholatCal(); // load Sholah Time
  //  check_puasa();                                              // check
  //  jadwal Puasa Besok
  if (floatnow > sholatT[6]) {
    date_cor = 1; // load Hijr Date + corection next day after Mhagrib
  }
  nowH =
      toHijri(now.year(), now.month(), now.day(), date_cor); // load Hijir Date

  if (displayReady) {
    uint8_t baseBright = (Prm.BL < 20) ? 50 : Prm.BL;
    if ((floatnow > 21.0f) || (floatnow < 3.5f)) {
      uint8_t nightBright = baseBright / 3;
      if (nightBright < 15) nightBright = 15;
      setBrightness(nightBright);
    } else {
      setBrightness(baseBright);
    }
  }
}

void check_azzan() {
  static uint8_t lastAzzanDay = 0;
  static int8_t  lastAzzanPrayer = -1;
  SholatNow = -1;

  uint16_t currentMinute = (uint16_t)now.hour() * 60U + (uint16_t)now.minute();

  for (uint8_t i = 0; i < 8; i++) {
    // Lewati Imsak, Terbit, dan Dhuha
    if (i == 0 || i == 2 || i == 3) {
      continue;
    }

    // Bulatkan jadwal ke menit berikutnya
    uint16_t prayerMinute = (uint16_t)ceil((sholatT[i] * 60.0f) - 0.0001f);

    if (currentMinute >= prayerMinute) {
      SholatNow = i;
    }

    // Azan aktif maksimal dalam rentang 5 menit dan hanya 1x per waktu sholat per hari
    if (!azzan && (daynow != lastAzzanDay || i != lastAzzanPrayer) &&
        currentMinute >= prayerMinute &&
        currentMinute < prayerMinute + 5U) {
      lastAzzanDay = daynow;
      lastAzzanPrayer = i;
      setJumat(daynow == 5 && i == 4 && Prm.MT == 1);
      SholatNow = i;
      azzan = true;
      dfStop();
      startBuzzer(5);

      // drawAzzan terdaftar menggunakan nomor 100
      setRunSel(100);
      break;
    }
  }
}
