// =========================================
// Drawing Content Block====================
// =========================================
void drawAzzan(int DrawAdd)
{
  // check RunSelector
  if (!dwDo(DrawAdd)) return;
  uint16_t           ct_limit ;
  uint16_t           ad=Prm.AD;
  uint16_t           jm=Prm.IN; 
  static uint16_t    ct;
  static uint32_t    lsRn;
  static boolean     bzEndDone = false;
  uint32_t           Tmr = millis();

  if(jumat)       {ct_limit=jm*60;}
    else            {ct_limit=ad*60;}

  if (ct == 0) {
    bzEndDone = false;
  }

  if ((Tmr - lsRn) > 500 and ct <= ct_limit)
  {
    lsRn = Tmr;
    
    if ((ct % 2) == 0)
    {      
      Disp.drawFilledRect(15, 4, 16, 6);
      Disp.drawFilledRect(15, 10, 16, 12);
      if (jumat) {
        fType(0);
        dwCtr(32, 0, "ACARA");
      }
      else      {
        fType(0);
        dwCtr(32, 0, "ADZAN");
      }
    }else {

    }

    if (jumat) {
        fType(0); dwCtr(32, 8, "JUMATAN");
      }
      else      {
        fType(0); dwCtr(32, 8, sholatN(SholatNow));
      }
    
    Jam(0);

    // Mau selesai azan: bunyikan 3 kali bip dengan jeda 500ms
    if (ct_limit >= 6 && ct == (ct_limit - 6) && !bzEndDone) {
      startBuzzer(3);
      bzEndDone = true;
    }

    ct++;
    DoSwap = true;
  }
  if ((Tmr - lsRn) > 2000 and (ct > ct_limit))
  { 
    dwDone(DrawAdd);
    ct = 0;
    bzEndDone = false;
    Buzzer(0);
  }
}

static void format2d(char *buf, uint8_t val) {
  buf[0] = (char)('0' + (val / 10));
  buf[1] = (char)('0' + (val % 10));
  buf[2] = '\0';
}

static void formatTime2(char *buf, uint8_t h, uint8_t m) {
  buf[0] = (char)('0' + (h / 10));
  buf[1] = (char)('0' + (h % 10));
  buf[2] = ':';
  buf[3] = (char)('0' + (m / 10));
  buf[4] = (char)('0' + (m % 10));
  buf[5] = '\0';
}

static void formatTime3(char *buf, uint8_t h, uint8_t m, uint8_t s) {
  formatTime2(buf, h, m);
  buf[5] = ':';
  buf[6] = (char)('0' + (s / 10));
  buf[7] = (char)('0' + (s % 10));
  buf[8] = '\0';
}

void drawIqomah(int DrawAdd)  
{
  // check RunSelector
  if (!dwDo(DrawAdd)) return;

  static uint32_t   lsRn;
  uint32_t          Tmr = millis();
  static int        ct;
  static boolean    bzEndDone = false;
  int               mnt, scd, cn_l ;
  char              BuffM[5];
  char              BuffD[5];

  cn_l  = (Iqomah[SholatNow] * 60);  
  
  if (ct == 0) {
    bzEndDone = false;
  }

  fType(0);
  if ((Tmr - lsRn) > 1000 and ct <= cn_l)
  {
    lsRn = Tmr;
    mnt = (cn_l - ct) / 60;
    scd = (cn_l - ct) % 60;
    format2d(BuffM, (uint8_t)mnt);
    format2d(BuffD, (uint8_t)scd);

    if (mnt > 0) {
      fType(3);
      Disp.drawFilledRect(15, 4, 16, 5);
      Disp.drawFilledRect(15, 10, 16, 11);
      Disp.drawText(1,0,BuffM);
      Disp.drawText(18,0,BuffD); 

    } else {
      fType(3);
      Disp.drawText(9,0,BuffD);      
    }
    
    fType(0);
    dwCtr(32, 0, "TUNGGU");
    dwCtr(32, 8, "IQOMAH");

    // Waktu iqomah akan berakhir (5 detik tersisa): bunyikan 5 kali bip dengan jeda 500ms
    if (cn_l >= 5 && ct == (cn_l - 5) && !bzEndDone) {
      startBuzzer(5);
      bzEndDone = true;
    }

    ct++;
    DoSwap = true;
  }
  if (ct > cn_l)
  {
    dwDone(DrawAdd);
    ct = 0;    
    bzEndDone = false;
    Buzzer(0);
  }
}

void drawSholat_S(int sNum, int c)
{
  char BuffTime[6];

  uint16_t prayerMinute =
      (uint16_t)ceil((sholatT[sNum] * 60.0f) - 0.0001f);

  // Antisipasi jika melewati pukul 24:00
  prayerMinute %= 1440U;

  uint8_t shour   = prayerMinute / 60U;
  uint8_t sminute = prayerMinute % 60U;

  formatTime2(BuffTime, shour, sminute);

  fType(0);
  dwCtr(33, 0, sholatN(sNum));

  fType(2);
  dwCtr(33, 9, BuffTime);

  DoSwap = true;
}

void drawSholat(int DrawAdd)
{
  if (!dwDo(DrawAdd)) return;
  static uint8_t    x;
  static uint8_t    s; // 0=in, 1=out
  static uint8_t    sNum;
  static uint32_t   lsRn;
  uint32_t          Tmr = millis();
  uint8_t           c = 32; 
  int               DrawWd = DWidth - c;

  if ((Tmr - lsRn) > 50)
  {
    if (s == 0 and x < (DrawWd / 2)) { x++; lsRn = Tmr; }
    if (s == 1 and x > 0) { x--;  lsRn = Tmr; }
  }

  if ((Tmr - lsRn) > 5000 and x == (DrawWd / 2)) { s = 1;  }
  if (x == 0 and s == 1){if (sNum < 7) {sNum++;}
    else
    {dwDone(DrawAdd); sNum = 0;}
    s = 0;
  }

  if (Prm.SI == 0 and sNum == 0) {sNum = 1;}
  if (Prm.ST == 0 and sNum == 2) {sNum = 3;}
  if (Prm.SU == 0 and sNum == 3) {sNum = 4;}
  drawSholat_S(sNum, c);

  Disp.drawFilledRect(c, 0, c + DrawWd / 2 - x, 15, 0);
  Disp.drawFilledRect(DrawWd / 2 + x + c, 0, 64, 15, 0);
  // Disp.drawLine(DrawWd/2-x-1,0,DrawWd/2-x-1,15,1);
  // Disp.drawLine(DrawWd/2+x,0,DrawWd/2+x,15,1);
  Jam(0); 
}


void Jam(uint16_t y) 
{
  static uint32_t ss;
  uint32_t Tmr = millis();
  char BuffH[3];
  char BuffM[3];
  char BuffD[3];
  format2d(BuffH, now.hour());
  format2d(BuffM, now.minute());
  format2d(BuffD, now.second());
  fType(3);
  Disp.drawText(1, y, BuffH);
  fType(2);
  Disp.drawText(18, y+1, BuffM);
  Disp.drawText(18, y+9, BuffD);
  if (Tmr - ss < 500 and y == 0) {
    Disp.drawFilledRect(15, 4, 16, 6);
    Disp.drawFilledRect(15, 10, 16, 12);
  }
  if (Tmr - ss > 1000) ss = Tmr;

  DoSwap = true;
}

void drawSalam(uint16_t y)   //Draw Date
{ 
  fType(2); 
  dwCtr(0, y+1, "ASSALAMU");
  dwCtr(0, y+9, "ALAIKUM"); 
  DoSwap = true;
}

void anim_SALAM(int DrawAdd)
{
  // check RunSelector
  if (!dwDo(DrawAdd)) return;

  static uint8_t    y;
  static uint8_t    s; // 0=in, 1=out
  static uint32_t   lsRn;
  uint32_t          Tmr = millis();

  if ((Tmr - lsRn) > 100)   
  {
    if (s == 0 and y < 15) {lsRn = Tmr; y++; }
    if (s == 1 and y > 0) {lsRn = Tmr; y--; }
  }
  if ((Tmr - lsRn) > 10000 and y == 15) {s = 1;}
  if (y == 7) { }
  if (y == 0 and s == 1) { dwDone(DrawAdd); s = 0; }
  
  fType(1);  
  drawSalam(y-15);

}


void drawGreg_TS(uint16_t y)   // Draw Time
{
  char  Buff[9];
  formatTime3(Buff, now.hour(), now.minute(), now.second());
  fType(2);
  dwCtr(0, y, Buff);
  DoSwap = true;
}

void dwMrq(const char* msg, int Speed, int dDT, int DrawAdd)
{
  // check RunSelector
  static uint16_t   x;
  if (!dwDo(DrawAdd)) return;
  if (reset_x != 0) {x = 0; reset_x = 0;}


  static uint32_t   lsRn;
  int fullScroll = Disp.textWidth(msg) + DWidth;
  uint32_t          Tmr = millis();
  if ((Tmr - lsRn) > Speed)
  { lsRn = Tmr;
    if (x < fullScroll) {++x;}
    else {dwDone(DrawAdd); x = 0; return; }
    
    if (dDT == 1)
      {
      fType(1);     
      Disp.drawText(DWidth - x, 1, msg); //Atas
      fType(4);
      if (x <= 6){drawGreg_TS(15 - x);}
      else if (x >= (fullScroll-6))   {drawGreg_TS(15-(fullScroll - x));}
      else {drawGreg_TS(9); }
      }
    else if (dDT == 2)
      {
      fType(4);
      if (x <= 6) {drawGreg_TS(x - 6);}
      else if (x >= (fullScroll - 6))   {drawGreg_TS((fullScroll - x) - 6);}
      else  { drawGreg_TS(1); }
      fType(1);
      Disp.drawText(DWidth - x, 8 , msg);//Bawah
      }
    else if (dDT == 3)
      {
      fType(1);
      if (x <= 6) {dwCtr(0, x-6, drawDayName());}
      else if (x >= (fullScroll - 6))   {dwCtr(0, (fullScroll - x) - 6, drawDayName());}
      else  { dwCtr(0, 0, drawDayName()); }
      fType(1);
      Disp.drawText(DWidth - x, 8 , msg);//Bawah
      }    
    else
    {
      fType(1);
      Disp.drawLine(0, 1, 63, 1); 
      Disp.drawLine(0, 14, 63, 14); 
      Disp.drawText(DWidth - x, 5, msg);
    }
    DoSwap = true;
  }
}

void blinkBlock(int DrawAdd)
{
 // check RunSelector
 if (!dwDo(DrawAdd)) return;

 static uint32_t   lsRn;
 static uint16_t   ct, ct_l;
 uint32_t          Tmr = millis();

 if (jumat)       {
   ct_l = (Prm.JM * 60) + (Prm.SO * 60);
 }
 else            {
   ct_l = Prm.SO * 60;
 }
 //    jumat =false;

 if ((Tmr - lsRn) > 1000)
 { lsRn = Tmr;

   if ((ct % 2) == 0)
   {

     Disp.drawFilledRect(DWidth - 3, DHeight - 3, DWidth - 2, DHeight - 2);
   }
   DoSwap = true;
   ct++;
 }
if (ct > ct_l)
  { dwDone(DrawAdd);
    azzan = false;
    setJumat(false);
    ct = 0;
  }
}

// =========================================
// Drawing Tools============================
// =========================================
boolean dwDo(int DrawAdd)
{ if (RunSel == DrawAdd) {
    return true;
  }
  else return false;
}

void dwDone(int DrawAdd)
{ RunFinish = DrawAdd;
  RunSel = 0;
}

void dwCtr(int x, int y, const char* Msg)
{ int   tw = Disp.textWidth(Msg);
  int   th = Disp.textHeight();
  int   c = int((DWidth - x - tw) / 2);
  Disp.drawFilledRect(x + c - 1, y, x + tw + c, y + th, 0);
  Disp.drawText(x + c, y, Msg);
}

void Buzzer(uint8_t state)
{
 if (state == 1)
 {
   if (!isBuzzerActive()) startBuzzer(1);
 }
 else
 {
   stopBuzzer();
 }
}

void fType(int x)
{
  if (x == 0) Disp.setFont(Font0);
  else if (x == 1)    Disp.setFont(Font1);
  else if (x == 2)    Disp.setFont(Font2);
  else if (x == 3)    Disp.setFont(Font3);
  else Disp.setFont(Font4);
}
