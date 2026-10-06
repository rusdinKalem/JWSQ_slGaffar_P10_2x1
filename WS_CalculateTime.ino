//------------------------------------------
// Function calculate Pray Time
//------------------------------------------
/*Macro Function */
#define d2r(x) ((x)*(float)M_PI/180.0f)
#define r2d(x) ((x)*180.0f/(float)M_PI)

//Constanta 
    const float lunarY          = 354.367068f;

// Main Function
void sholatCal()
  {
    float EJD = E_Julian_date(now.year(),now.month(),now.day(),Prm.L_LO);
    float Decl, EqOfTime;
    SunPosition(EJD, Decl, EqOfTime);
    Pray_Time(Prm.L_TZ, Prm.L_LA, Prm.L_LO,Prm.L_AL,Decl, EqOfTime );
  }

//Julian Date at GMT mid day
float E_Julian_date(int Year,int Month,int Days,float Longitude) // juliant date - 2451545
  {

    if (Month <= 2)
      {
      Year -= 1;
      Month += 12;
      }

    int A = Year / 100;
    int B = 2 - A + (A / 4);

    float CLong = Longitude/(15.0f * 24.0f);
    float JD = floor(365.25f *(float)(Year+ 4716)) 
              - 2451545.0f 
              + floor(30.6001f * (float)(Month + 1)) 
              + (float)Days + (float)B 
              - 1524.5f
              -CLong;
    return JD; 
  }

//Sun Position: Declination & Equation of Time combined
void SunPosition(float EJD, float &Decl, float &EqOfTime) 
  {
    float g = fix_angle(357.529f + 0.98560028f * EJD);
    float q = fix_angle(280.459f + 0.98564736f * EJD);
    float L = fix_angle(q + 1.915f * sin(d2r(g)) + 0.020f * sin(d2r(2.0f * g)));
    float e = (23.439f - 0.00000036f * EJD);

    float sinE = sin(d2r(e));
    float cosE = cos(d2r(e));
    float sinL = sin(d2r(L));
    float cosL = cos(d2r(L));

    Decl = r2d(asin(sinE * sinL));
    float RA = r2d(atan2(cosE * sinL, cosL)) / 15.0f;
    EqOfTime = (q / 15.0f) - fix_hour(RA);
  }

float HourAngle( float Alfa, float Declination, float Latitude)
  {
     float cosH = (-sin(d2r(Alfa))-sin(d2r(Latitude))*sin(d2r(Declination)))
                  / (cos(d2r(Latitude))*cos(d2r(Declination)));
     cosH = constrain(cosH, -1.0f, 1.0f);
     return r2d(acos(cosH))/15.0f;
  }

void Pray_Time(float TimeZone, float Latitude, float Longitude,float Altitude,float Declination, float EquationOfTime )
  { 
    // Dzuhur
    float BaseTime = fix_hour(12.0f + TimeZone - (Longitude / 15.0f) - EquationOfTime);
    sholatT[4] = BaseTime + (float)(Prm.IH + Prm.IL) / 60.0f;

    // Ashr
    float alfa = r2d(-atan(1.0f / (1.0f + tan(d2r(fabs(Latitude - Declination))))));
    float HA = HourAngle(alfa, Declination, Latitude);
    sholatT[5] = BaseTime + HA + (float)(Prm.IH + Prm.IA) / 60.0f;

    // Maghrib
    alfa = 0.8333f + 0.0347f * sqrt(Altitude > 0.0f ? Altitude : 0.0f);
    HA = HourAngle(alfa, Declination, Latitude);
    sholatT[6] = BaseTime + HA + (float)(Prm.IH + Prm.IM) / 60.0f;

    // Terbit
    sholatT[2] = BaseTime - HA;

    // Isya
    HA = HourAngle(18.0f, Declination, Latitude);
    sholatT[7] = BaseTime + HA + (float)(Prm.IH + Prm.II) / 60.0f;

    // Shubuh
    HA = HourAngle(20.0f, Declination, Latitude);
    sholatT[1] = BaseTime - HA + (float)(Prm.IH + Prm.IS) / 60.0f;
    // Imsak
    sholatT[0] = sholatT[1] - (10.0f / 60.0f);  
    // Dhuha
    HA = HourAngle(-4.5f, Declination, Latitude);
    sholatT[3] = BaseTime - HA;
  }

float fix_hour(float a)
  {
    a = a - 24.0f * floor(a / 24.0f);
    a = a < 0.0f ? a + 24.0f : a;
    return a;
  }

float fix_angle(float a)
  {
    a = a - 360.0f * floor(a / 360.0f);
    a = a < 0.0f ? a + 360.0f : a;
    return a;
  }  




//------------------------------------------
// Function calculate Hijriah Date
//------------------------------------------

long Days(uint16_t Y,uint8_t M,uint8_t D)
  {
    if (M < 3)
      { 
      Y -= 1; 
      M +=12;
      }
    Y = Y - 2000;
    long ndays = floor(365.25 * Y) + floor(30.6001 * (M + 1)) + (Y / 100) + (Y / 400) + D + 196;
    //long ndays= d1 + d2 - A + B + D + 196;
    return ndays;
  }

long DaysHijri(uint16_t Y,uint8_t M,uint8_t D)
  {
    Y = Y - 1420;   
    long hari = floor(29.5*M - 28.999)+floor(lunarY*Y)+D ;
    return hari;
  }
  
hijir_date toHijri(uint16_t Y, uint8_t M, uint8_t D,uint8_t cor) // core --> corection date today=0   yesterday=-1 tomorrow=1
  {
    hijir_date BuffDate;
    long nday = Days(Y, M, D)+Prm.CH+ cor;
    
    long tahun = floor(nday/lunarY) + 1420;
    long bulan = 1;
    long harike = 1;
    while(DaysHijri(tahun, bulan, 1) <= nday){tahun++;};
    tahun--;
    while(DaysHijri(tahun, bulan, 1) <= nday){bulan++;};
    bulan--;
    harike = 1 + nday - DaysHijri(tahun, bulan, 1);
    if (bulan == 13){bulan = 12; harike += 29;};
    BuffDate.hD = harike;
    BuffDate.hM = bulan;
    BuffDate.hY = tahun;

    return BuffDate;
  }
