/*------------------------------------------
// Function Setup Parameter EEPROM
------------------------------------------*/
/* Struktur Data EEPROM 
 *      1   byte add  0   uint8_t       Parameter Version
 *      4   byte add  1   float         Latitude         code NLA
 *      4   byte add  5   float         Longitude        code NLO
 *      4   byte add  9   float         Altitude         code NAL   
 *      4   byte add 13   float         TimeZone         code NTZ
 *      1   byte add 17   uint8_t       Masjid Type      code NMT
 *      1   byte add 18   uint8_t       Led Brighnest    code NBL
 *      1   byte add 19   uint8_t       Running Time     code NRT
 *      1   byte add 20   uint8_t       Ihtiyati         code NIH
 *      1   byte add 21   uint8_t       Durasi Adzan     code NAD
 *      1   byte add 22   uint8_t       Sholat Time      code NSO
 *      1   byte add 23   uint8_t       Jum'at Time      code NJM
 *      1   byte add 24   uint8_t       IQ Subuh         code NI1
 *      1   byte add 25   uint8_t       IQ Dzuhur        code NI4
 *      1   byte add 26   uint8_t       IQ Ashar         code NI5
 *      1   byte add 27   uint8_t       IQ Maghrib       code NI6
 *      1   byte add 28   uint8_t       IQ Isya          code NI7
 *      1   byte add 29   uint8_t       Buzzer           code NBZ
 *      1   byte add 30   uint8_t       Show Imsak       code NSI
 *      1   byte add 31   uint8_t       Show Terbit      code NST
 *      1   byte add 32   uint8_t       Show Dhuha       code NSU
 *      1   byte add 33   int8_t        Koreksi Subuh    code NIS
 *      1   byte add 34   int8_t        Koreksi Dzuhur   code NIL
 *      1   byte add 35   int8_t        Koreksi Ashar    code NIA
 *      1   byte add 36   int8_t        Koreksi Maghrib  code NIM
 *      1   byte add 37   int8_t        Koreksi Isya     code NII
 *      1   byte add 38   int8_t        Koreksi Hijriah  code NCH
 *        
 *      40  byte add 40   char          Masjid Name      code CMN
 *      50  byte add 80   char          Masjid Address   code CMA    
 *      150 byte add 130  char          Info 1           code CN1
 *      150 byte add 280  char          Info 2           code CN2
 *      150 byte add 430  char          Info 3           code CN3
 *      150 byte add 580  char          Sholat Message   code CSM  -- pesan mejelang sholat biasa
 *      150 byte add 730  char          Jumat Message    code CJM  -- pesan setelah azan 
 *      144 byte add 880  -------  EMPTY  --------
 *  
 *      Memory Size arduino Uno
 *        Flash  32k bytes (of which .5k is used for the bootloader)
 *        SRAM   2k bytes
 *        EEPROM 1k byte
 */

const uint8_t PARAM_VERSION = 100;
const uint16_t SERIAL_RX_TIMEOUT_MS = 500;

enum ParameterValueType {
  VALUE_NONE = 0,
  VALUE_FLOAT = 1,
  VALUE_UINT8 = 2,
  VALUE_INT8 = 3
};

static uint8_t serialRxIndex = 0;
static uint32_t serialLastByteMs = 0;
static boolean serialReceiving = false;
static boolean serialDiscarding = false;

boolean commandCodeIs(char firstCode, char secondCode)
{
  return CH_Prm[1] == firstCode && CH_Prm[2] == secondCode;
}

void resetSerialParser()
{
  serialRxIndex = 0;
  serialReceiving = false;
  serialDiscarding = false;
  CH_Prm[0] = '\0';
}

void serviceBluetooth()
{
  uint32_t currentMs = millis();

  while (Serial.available() > 0) {
    const char incoming = (char)Serial.read();
    currentMs = millis();

    if (serialDiscarding) {
      if (incoming == '\n' || incoming == '\r') resetSerialParser();
      continue;
    }

    if (!serialReceiving) {
      if (incoming != 'C' && incoming != 'N' && incoming != 'S' && incoming != 'P') continue;
      serialReceiving = true;
      serialRxIndex = 0;
    }

    serialLastByteMs = currentMs;

    if (incoming == '\r' || incoming == '\n') {
      if (serialReceiving && serialRxIndex >= 2) {
        CH_Prm[serialRxIndex] = '\0';
        LoadPrm();
      }
      resetSerialParser();
      continue;
    }

    if (serialRxIndex < sizeof(CH_Prm) - 1U) {
      CH_Prm[serialRxIndex++] = incoming;
    }
    else {
      serialReceiving = false;
      serialDiscarding = true;
    }
  }

  if ((serialReceiving || serialDiscarding) &&
      (uint32_t)(currentMs - serialLastByteMs) > SERIAL_RX_TIMEOUT_MS) {
    resetSerialParser();
  }
}

boolean getTextTarget(uint16_t &address, uint8_t &maxCharacters)
{
  if (commandCodeIs('M', 'N')) { address = 40;  maxCharacters = 39;  }
  else if (commandCodeIs('M', 'A')) { address = 80;  maxCharacters = 49;  }
  else if (commandCodeIs('N', '1')) { address = 130; maxCharacters = 149; }
  else if (commandCodeIs('N', '2')) { address = 280; maxCharacters = 149; }
  else if (commandCodeIs('N', '3')) { address = 430; maxCharacters = 149; }
  else if (commandCodeIs('S', 'M')) { address = 580; maxCharacters = 149; }
  else if (commandCodeIs('J', 'M')) { address = 730; maxCharacters = 149; }
  else return false;
  return true;
}

boolean getNumericTarget(uint8_t &address, uint8_t &valueType)
{
  if (commandCodeIs('L', 'A')) { address = 1;  valueType = VALUE_FLOAT; }
  else if (commandCodeIs('L', 'O')) { address = 5;  valueType = VALUE_FLOAT; }
  else if (commandCodeIs('A', 'L')) { address = 9;  valueType = VALUE_FLOAT; }
  else if (commandCodeIs('T', 'Z')) { address = 13; valueType = VALUE_FLOAT; }
  else if (commandCodeIs('M', 'T')) { address = 17; valueType = VALUE_UINT8; }
  else if (commandCodeIs('B', 'L')) { address = 18; valueType = VALUE_UINT8; }
  else if (commandCodeIs('R', 'T')) { address = 19; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', 'H')) { address = 20; valueType = VALUE_UINT8; }
  else if (commandCodeIs('A', 'D')) { address = 21; valueType = VALUE_UINT8; }
  else if (commandCodeIs('S', 'O')) { address = 22; valueType = VALUE_UINT8; }
  else if (commandCodeIs('J', 'M')) { address = 23; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', '1')) { address = 24; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', '4')) { address = 25; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', '5')) { address = 26; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', '6')) { address = 27; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', '7')) { address = 28; valueType = VALUE_UINT8; }
  else if (commandCodeIs('B', 'Z')) { address = 29; valueType = VALUE_UINT8; }
  else if (commandCodeIs('S', 'I')) { address = 30; valueType = VALUE_UINT8; }
  else if (commandCodeIs('S', 'T')) { address = 31; valueType = VALUE_UINT8; }
  else if (commandCodeIs('S', 'U')) { address = 32; valueType = VALUE_UINT8; }
  else if (commandCodeIs('I', 'S')) { address = 33; valueType = VALUE_INT8; }
  else if (commandCodeIs('I', 'L')) { address = 34; valueType = VALUE_INT8; }
  else if (commandCodeIs('I', 'A')) { address = 35; valueType = VALUE_INT8; }
  else if (commandCodeIs('I', 'M')) { address = 36; valueType = VALUE_INT8; }
  else if (commandCodeIs('I', 'I')) { address = 37; valueType = VALUE_INT8; }
  else if (commandCodeIs('C', 'H')) { address = 38; valueType = VALUE_INT8; }
  else return false;
  return true;
}

void writeEepromText(uint16_t address, uint8_t maxCharacters, const char *text)
{
  uint8_t index = 0;
  while (index < maxCharacters && text[index] != '\0') {
    EEPROM.update(address + index, text[index]);
    index++;
  }
  EEPROM.update(address + index, '\0');
}

boolean writeTextCommand()
{
  uint16_t address;
  uint8_t maxCharacters;
  if (!getTextTarget(address, maxCharacters)) return false;

  writeEepromText(address, maxCharacters, CH_Prm + 3);
  return true;
}

boolean floatValueIsValid(double value)
{
  if (commandCodeIs('L', 'A')) return value >= -90.0 && value <= 90.0;
  if (commandCodeIs('L', 'O')) return value >= -180.0 && value <= 180.0;
  if (commandCodeIs('A', 'L')) return value >= -500.0 && value <= 10000.0;
  if (commandCodeIs('T', 'Z')) return value >= -12.0 && value <= 14.0;
  return false;
}

boolean unsignedValueIsValid(long value)
{
  if (value < 0 || value > 255) return false;
  if (commandCodeIs('M', 'T')) return value >= 1 && value <= 4;
  if (commandCodeIs('B', 'Z') || commandCodeIs('S', 'I') ||
      commandCodeIs('S', 'T') || commandCodeIs('S', 'U')) {
    return value <= 1;
  }
  return true;
}

static boolean parseSimpleFloat(const char *s, float &result) {
  if (s == NULL || *s == '\0') return false;
  float sign = 1.0f;
  if (*s == '-') { sign = -1.0f; s++; }
  else if (*s == '+') { s++; }
  if (*s == '\0' || (*s != '.' && (*s < '0' || *s > '9'))) return false;
  float val = 0.0f;
  while (*s >= '0' && *s <= '9') {
    val = val * 10.0f + (*s - '0');
    s++;
  }
  if (*s == '.') {
    s++;
    float factor = 0.1f;
    while (*s >= '0' && *s <= '9') {
      val += (*s - '0') * factor;
      factor *= 0.1f;
      s++;
    }
  }
  if (*s != '\0') return false;
  result = sign * val;
  return true;
}

boolean writeNumericCommand()
{
  uint8_t address;
  uint8_t valueType;
  if (!getNumericTarget(address, valueType)) return false;

  char *endPointer;
  const char *valueText = CH_Prm + 3;

  if (valueType == VALUE_FLOAT) {
    float storedValue = 0.0f;
    if (!parseSimpleFloat(valueText, storedValue) ||
        !floatValueIsValid(storedValue)) return false;
    EEPROM.put(address, storedValue);
    return true;
  }

  const long parsedValue = strtol(valueText, &endPointer, 10);
  if (endPointer == valueText || *endPointer != '\0') return false;

  if (valueType == VALUE_UINT8) {
    if (!unsignedValueIsValid(parsedValue)) return false;
    const uint8_t storedValue = (uint8_t)parsedValue;
    EEPROM.put(address, storedValue);
    return true;
  }

  if (parsedValue < -128 || parsedValue > 127) return false;
  const int8_t storedValue = (int8_t)parsedValue;
  EEPROM.put(address, storedValue);
  return true;
}

boolean parseTwoDigits(const char *text, uint8_t &value)
{
  if (text[0] < '0' || text[0] > '9' ||
      text[1] < '0' || text[1] > '9') return false;
  value = (uint8_t)((text[0] - '0') * 10 + (text[1] - '0'));
  return true;
}

boolean setRtcCommand()
{
  if (strlen(CH_Prm) != 16) return false;

  uint8_t date, month, year, hour, minute, second;
  if (!parseTwoDigits(CH_Prm + 3, date) ||
      !parseTwoDigits(CH_Prm + 5, month) ||
      !parseTwoDigits(CH_Prm + 7, year) ||
      !parseTwoDigits(CH_Prm + 9, hour) ||
      !parseTwoDigits(CH_Prm + 11, minute) ||
      !parseTwoDigits(CH_Prm + 13, second) ||
      CH_Prm[15] < '1' || CH_Prm[15] > '7') return false;

  if (date < 1 || date > 31 || month < 1 || month > 12 ||
      hour > 23 || minute > 59 || second > 59) return false;

  Clock.setClockMode(false);
  Clock.setDate(date);
  Clock.setMonth(month);
  Clock.setYear(year);
  Clock.setHour(hour);
  Clock.setMinute(minute);
  Clock.setSecond(second);
  Clock.setDoW((uint8_t)(CH_Prm[15] - '0'));

  rtcTimeValid = false;
  updateTime();
  return true;
}

int8_t getSlotFromCode(char code) {
  if (code == '1') return 0; // Subuh
  if (code == '4') return 1; // Dzuhur
  if (code == '5') return 2; // Ashar
  if (code == '6') return 3; // Maghrib
  if (code == '7') return 4; // Isya
  if (code == '8') return 5; // Jumat
  return -1;
}

boolean writeMp3Command() {
  const char c1 = CH_Prm[1];
  const char c2 = CH_Prm[2];

  // 1. Stop MP3: NPS atau NP0
  if (c1 == 'P' && (c2 == 'S' || c2 == '0')) {
    dfStop();
    return true;
  }

  // 2. Play Manual Folder & Track: NPF<folder>,<track> (Contoh: NPF1,2 atau NPF2,1)
  if (c1 == 'P' && c2 == 'F') {
    int f = 0, t = 0;
    if (sscanf(CH_Prm + 3, "%d,%d", &f, &t) == 2 ||
        sscanf(CH_Prm + 3, "%d-%d", &f, &t) == 2) {
      if (f >= 1 && f <= 99 && t >= 1 && t <= 255) {
        dfPlayManual((uint8_t)f, (uint8_t)t);
        return true;
      }
    }
    return false;
  }

  char *endPointer;
  const char *valText = CH_Prm + 3;
  const long val = strtol(valText, &endPointer, 10);
  if (endPointer == valText || *endPointer != '\0') return false;

  // 3. Play Manual Tartil: NPT<track> atau NPP<track> (Folder 01)
  if (c1 == 'P' && (c2 == 'T' || c2 == 'P')) {
    if (val < 1 || val > 99) return false;
    dfPlayManual(1, (uint8_t)val);
    return true;
  }

  // 4. Play Manual Tarhim: NPH<track> (Folder 02)
  if (c1 == 'P' && c2 == 'H') {
    if (val < 1 || val > 99) return false;
    dfPlayManual(2, (uint8_t)val);
    return true;
  }

  // Master Enable: NPM (1=On, 0=Off)
  if (c1 == 'P' && c2 == 'M') {
    if (val < 0 || val > 1) return false;
    Mp3Prm.enable = (uint8_t)val;
    saveMp3Prm();
    if (Mp3Prm.enable == 0) dfStop();
    return true;
  }

  // Volume: NPV (0-30)
  if (c1 == 'P' && c2 == 'V') {
    if (val < 0 || val > 30) return false;
    Mp3Prm.volume = (uint8_t)val;
    saveMp3Prm();
    dfSetVolume(Mp3Prm.volume);
    return true;
  }

  const int8_t slot = getSlotFromCode(c2);
  if (slot < 0) return false;

  // Menit Mulai Tartil: NT1 s.d. NT8 (0-60 menit)
  if (c1 == 'T') {
    if (val < 0 || val > 60) return false;
    Mp3Prm.tartilMin[slot] = (uint8_t)val;
    saveMp3Prm();
    return true;
  }

  // Durasi Tarhim Detik: ND1 s.d. ND8 (0-999 detik)
  if (c1 == 'D') {
    if (val < 0 || val > 999) return false;
    Mp3Prm.tarhimSec[slot] = (uint16_t)val;
    saveMp3Prm();
    return true;
  }

  // Track Tartil: NF1 s.d. NF8 (1-99)
  if (c1 == 'F') {
    if (val < 1 || val > 99) return false;
    Mp3Prm.tartilTrack[slot] = (uint8_t)val;
    saveMp3Prm();
    return true;
  }

  // Track Tarhim: NH1 s.d. NH8 (1-99)
  if (c1 == 'H') {
    if (val < 1 || val > 99) return false;
    Mp3Prm.tarhimTrack[slot] = (uint8_t)val;
    saveMp3Prm();
    return true;
  }

  return false;
}

boolean writeDirectPlayCommand() {
  // 1. Stop: PS atau P0
  if (CH_Prm[1] == 'S' || CH_Prm[1] == '0') {
    dfStop();
    return true;
  }

  // 2. Play Tartil: PT<track> (Folder 01)
  if (CH_Prm[1] == 'T') {
    int t = atoi(CH_Prm + 2);
    if (t >= 1 && t <= 99) {
      dfPlayManual(1, (uint8_t)t);
      return true;
    }
  }

  // 3. Play Tarhim: PH<track> (Folder 02)
  if (CH_Prm[1] == 'H') {
    int t = atoi(CH_Prm + 2);
    if (t >= 1 && t <= 99) {
      dfPlayManual(2, (uint8_t)t);
      return true;
    }
  }

  // 4. Play Custom: PF<folder>,<track> atau P<folder>,<track>
  int f = 0, t = 0;
  const char *sub = (CH_Prm[1] == 'F') ? (CH_Prm + 2) : (CH_Prm + 1);
  if (sscanf(sub, "%d,%d", &f, &t) == 2 || sscanf(sub, "%d-%d", &f, &t) == 2) {
    if (f >= 1 && f <= 99 && t >= 1 && t <= 255) {
      dfPlayManual((uint8_t)f, (uint8_t)t);
      return true;
    }
  }

  // 5. Play Track Folder 01: P<track> (Contoh: P1, P2, P6)
  if (CH_Prm[1] >= '1' && CH_Prm[1] <= '9') {
    int trk = atoi(CH_Prm + 1);
    if (trk >= 1 && trk <= 99) {
      dfPlayManual(1, (uint8_t)trk);
      return true;
    }
  }

  return false;
}

void LoadPrm()
{
  boolean updated = false;

  if (CH_Prm[0] == 'C') updated = writeTextCommand();
  else if (CH_Prm[0] == 'N') {
    updated = writeNumericCommand();
    if (!updated) updated = writeMp3Command();
  }
  else if (CH_Prm[0] == 'P') updated = writeDirectPlayCommand();
  else if (CH_Prm[0] == 'S' && commandCodeIs('D', 'T')) updated = setRtcCommand();

  if (updated) {
    GetPrm();
    if (Prm.BZ == 1) tone(BUZZ, 2000, 300);
  }
}

void GetPrm() {
  loadMp3Prm();
  EEPROM.get(0, Prm);
  if (Prm.state != PARAM_VERSION) {
    set_default_prm();
    EEPROM.get(0, Prm);
  }

  if (now.year() < 2018) {
    set_default_time();
    rtcTimeValid = false;
    updateTime();
  }

  Iqomah[1] = Prm.I1;
  Iqomah[4] = Prm.I4;
  Iqomah[5] = Prm.I5;
  Iqomah[6] = Prm.I6;
  Iqomah[7] = Prm.I7;

  loadDisplayCache();
  if (displayReady) setBrightness((int)Prm.BL);
  update_All_data();
}

void set_default_prm() {
  // Default strings disimpan di PROGMEM (hemat SRAM)
        static const char D_MASJID[] PROGMEM = "AL-MUHAJIRIN";
        static const char D_ALAMAT[] PROGMEM = "SULAWESI TENGGARA";
        static const char D_INFO1[]  PROGMEM = "Info 1";
        static const char D_INFO2[]  PROGMEM = "Info 2";
        static const char D_INFO3[]  PROGMEM = "Info 3";
        static const char D_SHOLAT[] PROGMEM = "SAATNYA SHALAT BERJAMAAH, RAPAT DAN LURUSKAN SHAF AGAR SHOLAT KITA SEMPURNA";
        static const char D_JUMAT[]  PROGMEM = "SAATNYA RANGKAIAN IBADAH JUM'AT, HARAP TENANG DAN KHIDMAT";
        char buf[50] = {0};

  Prm = (struct_param){PARAM_VERSION, -4.054413, 121.598583, 67.7, 8,
                       1, 50, 40, 2, 5, 10, 30, 15, 10, 10, 5, 10,
                       1, 1, 1, 1, 0, 0, 0, 0, 0, 0};
  EEPROM.put(0, Prm);
  strcpy_P(buf, D_MASJID); EEPROM.put(40, buf);
  strcpy_P(buf, D_ALAMAT); EEPROM.put(80, buf);
  strcpy_P(buf, D_INFO1);  EEPROM.put(130, buf);
  strcpy_P(buf, D_INFO2);  EEPROM.put(280, buf);
  strcpy_P(buf, D_INFO3);  EEPROM.put(430, buf);
  strcpy_P(buf, D_SHOLAT); EEPROM.put(580, buf);
  strcpy_P(buf, D_JUMAT);  EEPROM.put(730, buf);
}

void set_default_time() {
  Clock.setClockMode(false);  // set to 24h
  Clock.setYear(byte(18));
  Clock.setMonth(byte(1));
  Clock.setDate(byte(1));
  Clock.setDoW(byte(2));
  Clock.setHour(byte(12));
  Clock.setMinute(byte(0));
  Clock.setSecond(byte(0));
  Clock.turnOffAlarm(1);
  Clock.turnOffAlarm(2);
}
