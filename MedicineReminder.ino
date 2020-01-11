/*
reset button-2
reset Pin - 12
sevos - 4,5
        6,7
        10
lcd        - 8,9,22,23,24,25
             display-26
             backlight-27
prototype leds- yellow- 36
                red   - 37
rtc        - scl-21
             sda-20
rotary encoder - clk-A0
                 dt -A1
                 sw -A2
sd card module - cs   -53
                 sck  -52
                 mosi -51
                 miso -50
med detector sensor - 18
df player - tx - 0
            rx - 1
*/

#include <Wire.h>
#include "RTClib.h"
#include "LowPower.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

#define clearServo 10
#define r1p1 4
#define r1p2 5
#define r2p1 7
#define r2p2 6 
#define resetPin 12
#define resetButton 2
#define ChipSelect 53
#define SCK 52
#define MOSI 51
#define MISO 50
#define yled 36
#define rled 37
#define sensor 18
#define clk A0
#define dt A1
#define sw A2
#define rs 8
#define rw 9
#define d4 22
#define d5 23
#define d6 24
#define d7 25
#define Display 26
#define BackLight 27
LiquidCrystal lcd(rs, rw, d4, d5, d6, d7); 

boolean checkTemp1 = false;
boolean checkTemp2 = true;
boolean CcheckTemp1 = false;
boolean CcheckTemp2 = true;

volatile boolean isrflag = false ;
unsigned int timer = 0;
int cD;
int setHour;
int setMin;
int setDay;
int rA;

int hourAddr = 2;
int minAddr  = 3;
int ChourAddr = 11;
int CminAddr  = 12;
int dayAddr   = 13;
boolean firstTime = true;

int checkHour;
int checkMin;
int checkHourTempVar;
int checkMinTempVar;
int CcheckHour;
int CcheckMin;
int CcheckHourTempVar;
int CcheckMinTempVar;
int checkDay;
int alarmHour;
int alarmMin;

int currHour = 0;
int currMin  = 0;
int currDay  = 0;

String item1;
String item2;
String item3;
String item4;

boolean up = false;
boolean down = false;
boolean middle = false;
boolean displaySwitch = false;

int sd_x = 100;
int sd_y = 101;
int dataDay = 0; 
int dataMonth = 0;
int dataYear = 0; 
int dataHourFall = 0; 
int dataMinuteFall = 0;
int dataHourTaken = 0; 
int dataMinuteTaken = 0;
String dataDayOfWeek;

Servo r1s1,r1s2;  
Servo r2s1,r2s2;
Servo discardServo;
int pos = 0;
int dozeCounter = 0;
RTC_DS3231 rtc;
File myFile;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
ClickEncoder *encoder;
int16_t last, value; 
byte arrow[8] = { 0x00 , 0x04, 0x0C, 0x1F, 0x0C, 0x04, 0x00};
byte blank[8] = { 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


int LCDPowerTimeMin = 0;
int LCDPowerTimeMinTemp = 0;
boolean LCDPowerBool = true;

class Mp3Notify
{
public:
  static void OnError(uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }

  static void OnPlayFinished(uint16_t globalTrack)
  {
    Serial.println();
    Serial.print("Play finished for #");
    Serial.println(globalTrack);   
  }

  static void OnCardOnline(uint16_t code)
  {
    Serial.println();
    Serial.print("Card online ");
    Serial.println(code);     
  }

  static void OnCardInserted(uint16_t code)
  {
    Serial.println();
    Serial.print("Card inserted ");
    Serial.println(code); 
  }

  static void OnCardRemoved(uint16_t code)
  {
    Serial.println();
    Serial.print("Card removed ");
    Serial.println(code);  
  }
};
DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial);


  /////////////////////////////////////////////
 //               Setup                     //
/////////////////////////////////////////////

void setup () 
{
  digitalWrite(resetPin, HIGH);
  pinMode(resetPin, OUTPUT);
  pinMode(resetButton,INPUT);
  attachInterrupt(digitalPinToInterrupt(resetButton),resetISR,RISING);

  pinMode(ChipSelect, OUTPUT);
  Serial.begin(115200);
  rtc.begin();

  pinMode(Display, OUTPUT);
  pinMode(BackLight, OUTPUT);
  lcd.begin(16,4);

  if(!SD.begin(ChipSelect)) Serial.println("Failed sd card");
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

  EEPROM.write(1,1);
  
  attachInterrupt(digitalPinToInterrupt(sensor),isr,RISING);
  pinMode(yled,OUTPUT);
  pinMode(rled,OUTPUT);
  digitalWrite(rled,LOW);
  digitalWrite(yled,LOW);

  mp3.begin();
  uint16_t volume = mp3.getVolume();
  mp3.setVolume(30);  
  uint16_t count = mp3.getTotalTrackCount();

  lcd.createChar(0, arrow);
  lcd.createChar(1, blank);

  encoder = new ClickEncoder(dt, clk, sw); 
  encoder->setAccelerationEnabled(false);
  last = encoder->getValue();

  EEPROM.write(1, 1);

  if (rtc.lostPower()) 
  {
    Serial.println("rtc begin");
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  r1s1.attach(r1p1);  
  r1s2.attach(r1p2); 
  r2s1.attach(r2p1);  
  r2s2.attach(r2p2); 
  discardServo.attach(clearServo);
  discardServo.write(0);
  for (pos = 270; pos >= 90; pos -= 5) 
  { 
      r1s2.write(pos);              
      delay(15);                      
  }
  for (pos = 90; pos <= 270; pos += 5) 
  {
      r1s2.write(pos);              
      delay(15);                     
    }
  for (pos = 180; pos >= 0; pos -= 5) 
  {
      r1s1.write(pos);              
      delay(15);                      
    }
    for (pos = 180; pos <= 0; pos += 5) 
    { 
      r1s1.write(pos);             
      delay(15);                     
    }


  for (pos = 90; pos >= 0; pos -= 5)
  {
      r2s1.write(pos);             
      delay(15);                       
    } 
    for (pos = 0; pos <= 90; pos += 5) 
    { 
      r2s1.write(pos);             
      delay(15);                       
    }
    for (pos = 90; pos <= 180; pos += 5) 
    { 
      r2s2.write(pos);              
      delay(15);                     
    }
  for (pos = 180; pos >= 90; pos -= 5) 
  { 
      r2s2.write(pos);             
      delay(15);                      
  }

}

  /////////////////////////////////////////////
 //                Loop                     //
/////////////////////////////////////////////

void loop () 
{   

    DateTime now = rtc.now();
    if(EEPROM.read(1))
    {
      LCDPowerUp();
      firstBoot();
      LCDPowerTimeMinTemp = LCDPowerTimeMin + 2;
      if(LCDPowerTimeMin == 58) LCDPowerTimeMinTemp = 0;
      if(LCDPowerTimeMin == 59) LCDPowerTimeMinTemp = 1;
    }
    
    LCDPowerDown(); 
    encoderButton();
    if(middle)
    {
      LCDPowerUp();
      middle = false;
      displaySwitch = !displaySwitch;
    }

    if(displaySwitch)
    {
      displayTime();
    }

    if(!displaySwitch)
    {
      displayHistory();
    } 
      
    alarmCheck(); 
    regAlarmCheck();
    if(isrflag)   // gets executed only after delivery of medicine to rack
    {
      isrflag = false;
      delay(500);
      if(!digitalRead(sensor))
      { 
        medFall();
        Serial.println("med drop");
        delay(500);
      }
      else if(digitalRead(sensor))
      { 
        medTaken();
        dataLoggingTaken();
        Serial.println("med taken");
        delay(500);
      }
    }
}

  /////////////////////////////////////////////
 //            Functions begin              //
/////////////////////////////////////////////

void alarmCheck()
{
  if(firstTime)
  {
    hourAddr = 2;
    minAddr  = 3;
    ChourAddr = 11;
    CminAddr  = 12;
    dayAddr   = 13;
    firstTime = false;
  }
  DateTime now = rtc.now();
  if(checkTemp2)
  { 
    Serial.println(hourAddr);
    Serial.println(minAddr);
    checkHour = EEPROM.read(hourAddr);
    checkMin  = EEPROM.read(minAddr);
    hourAddr += 3;
    minAddr  += 3;
    if(hourAddr > 8) hourAddr = 2;
    if(minAddr > 9)  minAddr  = 3;
    checkTemp2 = false;
  }
  if(CcheckTemp2)
  {
    CcheckHour = EEPROM.read(ChourAddr);
    CcheckMin  = EEPROM.read(CminAddr); 
    checkDay   = EEPROM.read(dayAddr); 
    ChourAddr += 3;
    CminAddr  += 3;
    dayAddr   += 3;
    if(ChourAddr > 29)  ChourAddr = 11;
    if(CminAddr  > 30)  CminAddr  = 12;
    if(dayAddr   > 31)  dayAddr   = 13;
    CcheckTemp2 = false;
  }
  currHour = now.hour();
  currMin  = now.minute(); 
  currDay  = now.dayOfTheWeek();
  //Serial.println(currDay);
    
  if(currHour == checkHour)
  {
    if(currMin == checkMin)
    {
      checkMinTempVar = currMin;
      checkHourTempVar = currHour;
      Serial.println("should enter once only normal");
      checkTemp2 = true;
      delay(60000);
       if(dozeCounter < 15)
        {
                for (pos = 270; pos >= 90; pos -= 5) 
                { 
                  r1s2.write(pos);             
                  delay(15);                     
                }
                for (pos = 90; pos <= 270; pos += 5) 
                { 
                  r1s2.write(pos);             
                  delay(15);                     
                }

                for (pos = 180; pos >= 0; pos -= 5)
                {
                  r1s1.write(pos);             
                  delay(15);                       
                }
                for (pos = 180; pos <= 0; pos += 5) 
                { 
                  r1s1.write(pos);             
                  delay(15);                     
                }

        }
        else if((dozeCounter > 14) && (dozeCounter < 29))
        {
                for (pos = 90; pos >= 0; pos -= 5) 
                { 
                  r2s1.write(pos);              
                  delay(15);                       
                } 
                for (pos = 0; pos <= 90; pos += 5) 
                { 
                  r2s1.write(pos);           
                  delay(15);                      
                }

                for (pos = 90; pos <= 180; pos += 5) 
                {  
                  r2s2.write(pos);             
                  delay(15); 
                }                     
                for (pos = 180; pos >= 90; pos -= 5) 
                { 
                  r2s2.write(pos);              
                  delay(15);                      
                }
                if(dozeCounter == 28) dozeCounter = 0;
        }
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      mp3.playMp3FolderTrack(4);  
      delay(2000);
      mp3.playMp3FolderTrack(7);  
      delay(30000);
    }
  }
  if(currHour == checkHourTempVar)
  {
    if((currMin == (checkMinTempVar + 5) || currMin == (checkMinTempVar + 10)) && (!digitalRead(sensor)))
    {
      Serial.println("Normal doze");
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);
      digitalWrite(rled,HIGH);
      delay(100);
      digitalWrite(rled,LOW);
      delay(100);

      mp3.playMp3FolderTrack(4);  
      delay(2000);
      mp3.playMp3FolderTrack(7);  
      delay(30000);
    }
    if(currMin == (checkMinTempVar + 15) && (!digitalRead(sensor)))
    {
      digitalWrite(yled,HIGH);
      dataLoggingMissed();
      for (pos = 0; pos <= 90; pos += 5) 
      { 
          discardServo.write(pos);              
          delay(15);                      
      }
      delay(2000);
       for (pos = 90; pos >= 0; pos -= 5) 
      { 
          discardServo.write(pos);              
          delay(15);                      
      }
    }
  }

  if(currDay == checkDay)
  {
    if(currHour == CcheckHour)
    {
      if(currMin == CcheckMin)
      {
        CcheckMinTempVar = currMin;
        CcheckHourTempVar = currHour;
        Serial.println("should enter once only custom");
        CcheckTemp2 = true;
        delay(60000);
        dozeCounter++;
        if(dozeCounter < 15)
        {
                for (pos = 270; pos >= 90; pos -= 5) 
                { 
                  r1s2.write(pos);             
                  delay(15);                     
                }
                for (pos = 90; pos <= 270; pos += 5) 
                { 
                  r1s2.write(pos);             
                  delay(15);                     
                }

                for (pos = 180; pos >= 0; pos -= 5)
                {
                  r1s1.write(pos);             
                  delay(15);                       
                }
                for (pos = 180; pos <= 0; pos += 5) 
                { 
                  r1s1.write(pos);             
                  delay(15);                     
                }

        }
        else if((dozeCounter > 14) && (dozeCounter < 29))
        {
                for (pos = 90; pos >= 0; pos -= 5) 
                { 
                  r2s1.write(pos);              
                  delay(15);                       
                } 
                for (pos = 0; pos <= 90; pos += 5) 
                { 
                  r2s1.write(pos);           
                  delay(15);                      
                }

                for (pos = 90; pos <= 180; pos += 5) 
                {  
                  r2s2.write(pos);             
                  delay(15); 
                }                     
                for (pos = 180; pos >= 90; pos -= 5) 
                { 
                  r2s2.write(pos);              
                  delay(15);                      
                }
                if(dozeCounter == 28) dozeCounter = 0;
        }
       
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);

        mp3.playMp3FolderTrack(4);  
        delay(2000);
        mp3.playMp3FolderTrack(7);  
        delay(30000);
      }
    }
    if(currHour == CcheckHourTempVar)
    {
      if((currMin == (CcheckMinTempVar + 5) || currMin == (CcheckMinTempVar + 10)) && (!digitalRead(sensor)))
      {
        Serial.println("Custom doze");
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);
        digitalWrite(rled,HIGH);
        delay(100);
        digitalWrite(rled,LOW);
        delay(100);

        mp3.playMp3FolderTrack(4);  
        delay(2000);
        mp3.playMp3FolderTrack(7);  
        delay(30000);
      }

      if(currMin == (CcheckMinTempVar + 15) && (!digitalRead(sensor)))
      {
        digitalWrite(yled, HIGH);
        dataLoggingMissed();
        for (pos = 0; pos <= 90; pos += 5) 
        {   
            discardServo.write(pos);              
            delay(15);                      
        }
        delay(2000);
         for (pos = 90; pos >= 0; pos -= 5) 
        { 
            discardServo.write(pos);              
            delay(15);                      
        }
      }
    }
  }

}

void regAlarmCheck()
{
  DateTime now = rtc.now();
  alarmHour = now.hour();
  alarmMin = now.minute();
  if(alarmHour == EEPROM.read(32))
  {
    if(alarmMin == EEPROM.read(33))
    {
        mp3.playMp3FolderTrack(3);  
        delay(2000);
        mp3.playMp3FolderTrack(5);  
        delay(30000);
    }
  }
    if(alarmHour == EEPROM.read(35))
  {
    if(alarmMin == EEPROM.read(36))
    {
        mp3.playMp3FolderTrack(6);  
        delay(30000);
    }
  }
    if(alarmHour == EEPROM.read(38))
  {
    if(alarmMin == EEPROM.read(39))
    {
        mp3.playMp3FolderTrack(6);  
        delay(30000);
    }
  }
}

void resetISR()
{
  delay(500);
  if(digitalRead(resetButton))
  {
    resetFunction();
  }
}

void resetFunction()
{
  while(1)
  {
    LCDPowerUp();
    readRotaryEncoder();
    encoderButton();
    delay(500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Confirm Reset?");
    lcd.setCursor(0,1);
    lcd.print("Press for Yes");
    lcd.setCursor(0,2);
    lcd.print("Rotate for No");
    if(middle)
    {
      Serial.println("reset");
      digitalWrite(resetPin ,LOW);
    }
    else if(up || down)
    { 
      Serial.println("No reset");
      up = false;
      down = false;
      break;
    }
  }
}

void LCDPowerUp()
{
    if(LCDPowerBool)
    {
      DateTime now = rtc.now();
      LCDPowerTimeMin = now.minute();
      digitalWrite(Display, HIGH);
      digitalWrite(BackLight, HIGH);
      LCDPowerBool = false;
    }
    LCDPowerTimeMinTemp = LCDPowerTimeMin + 5;
    if(LCDPowerTimeMin == 58) LCDPowerTimeMinTemp = 0;
    if(LCDPowerTimeMin == 59) LCDPowerTimeMinTemp = 1;
}
void LCDPowerDown()
{     
    DateTime now = rtc.now();
    LCDPowerTimeMin = now.minute();
    if(LCDPowerTimeMin >= LCDPowerTimeMinTemp)
    {
      digitalWrite(Display, LOW);
      digitalWrite(BackLight, LOW);
      LCDPowerBool = true;
    }
}

void dataLoggingTaken()
{ 
  String dataString = String(dataDay) + "." + String(dataMonth) + "." + String(dataYear) + ", " + String(dataDayOfWeek) + ", " + String(dataHourFall) + ":" + String(dataMinuteFall) + ", " + String(dataHourTaken) + ":" + String(dataMinuteTaken);
  Serial.println("taken logged");

  myFile = SD.open("NewData.txt", FILE_WRITE);
    if (myFile) 
    {
      myFile.println(dataString);
      myFile.close();
      Serial.println("logger on");
    }
    EEPROM.write(sd_x, dataHourTaken);
    sd_x = sd_x + 2;
    EEPROM.write(sd_y, dataMinuteTaken);
    sd_y = sd_y + 2;
    if(sd_x > 106) sd_x = 100;
    if(sd_y > 107) sd_y = 101;
}

void dataLoggingMissed()
{ 
  String dataString = String(dataDay) + "." + String(dataMonth) + "." + String(dataYear) + ", " + String(dataDayOfWeek) + ", " + String(dataHourFall) + ":" + String(dataMinuteFall) + ", " + "Missed";
  Serial.println("missed logged");

  myFile = SD.open("NewData.txt", FILE_WRITE);
    if (myFile) 
    {
      myFile.println(dataString);
      myFile.close();
    }
    EEPROM.write(sd_x, 0);
    sd_x = sd_x + 2;
    EEPROM.write(sd_y, 0);
    sd_y = sd_y + 2;
    if(sd_x > 106) sd_x = 100;
    if(sd_y > 107) sd_y = 101;
}

void medFall()
{ 
  DateTime now = rtc.now();
  dataDay = now.day();
  dataMonth = now.month();
  dataYear = now.year();
  dataDayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];
  dataHourFall = now.hour();
  dataMinuteFall = now.minute();
}

void medTaken()
{
  DateTime now = rtc.now();
  dataHourTaken = now.hour(); 
  dataMinuteTaken = now.minute();
}

void displayHistory()
{
  item1 = "Doze 1:";
  item2 = "Doze 2:";
  item3 = "Doze 3:";
  item4 = "Doze 4:";
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(item1);
  lcd.print(EEPROM.read(2));
  lcd.print(":");
  lcd.print(EEPROM.read(3));
  lcd.setCursor(0,1);
  lcd.print(item2);
  lcd.print(EEPROM.read(5));
  lcd.print(":");
  lcd.print(EEPROM.read(6));
  lcd.setCursor(0,2);
  lcd.print(item3);
  lcd.print(EEPROM.read(8));
  lcd.print(":");
  lcd.print(EEPROM.read(9));
  lcd.setCursor(0,3);
  lcd.print(item4);
  lcd.print(EEPROM.read(ChourAddr));
  lcd.print(":");
  lcd.print(EEPROM.read(CminAddr));
  delay(300);
}

void firstBoot()
{
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Welcome New User");
  mp3.playMp3FolderTrack(8);  
  delay(6000);
  lcd.clear();
  delay(300);
  lcd.setCursor(0,0);
  lcd.print("Set time");
  lcd.setCursor(0,1);
  lcd.print("Doze 1");
  medTime();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set time");
  lcd.setCursor(0,1);
  lcd.print("Doze 2");
  medTime();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set time");
  lcd.setCursor(0,1);
  lcd.print("Doze 3");
  medTime();
  lcd.clear();
  delay(300);
/*
  lcd.setCursor(0,0);
  lcd.print("Set time for");
  lcd.setCursor(0,1);
  lcd.print("custom dozes of");
  lcd.setCursor(0,2);
  lcd.print("each day");
  mp3.playMp3FolderTrack(1);  
  delay(6000);
  for(cD = 0; cD < 7; cD++) customMedTime();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set time for");
  lcd.setCursor(0,1);
  lcd.print("alarms");
  delay(2000);
  for (rA = 0; rA < 3; rA++) regAlarm();

  mp3.playMp3FolderTrack(2);  
  delay(3000); */
}

void regAlarm()
{
    lcd.clear();
    delay(300);
    setHour = 0;
    setMin = 0;
    lcd.setCursor(0,0);
    lcd.print("Set time");
    lcd.setCursor(0,2);
    lcd.print(setHour);
    lcd.print(":");
    lcd.print(setMin);
    medTime(); 
}
void customMedTime()
{   
    lcd.clear();
    delay(300);
    setHour = 0;
    setMin = 0;
    lcd.setCursor(0,0);
    lcd.print("Set time");
    lcd.setCursor(0,1);
    lcd.print(daysOfTheWeek[cD]);
    lcd.setCursor(0,2);
    lcd.print(setHour);
    lcd.print(":");
    lcd.print(setMin);
    medTime();
    EEPROM.write(dayAddr,cD);
    dayAddr  += 3;
}

void medTime()
{
  int temp = 1;
  setHour = 0;
  setMin = 0;
  while (temp == 1)
  {
    readRotaryEncoder();
    encoderButton();
    if(EEPROM.read(1))
    { 
      EEPROM.write(1,0);
      middle = false;
    }
    if(up)
    {
      setHour++;
      up = false;
      if (setHour > 23) setHour = 0;
      if (setHour < 0) setHour = 23;
    }
    else if(down)
    {
      setHour--;
      down = false;
      if (setHour < 0) setHour = 23;
      if (setHour > 23) setHour = 0;
    }
    lcd.setCursor(0,2);
    lcd.print(setHour);
    lcd.print(":");
    lcd.print(setMin);
    if(middle == true)
    { 
      middle = false;
      EEPROM.write(hourAddr, setHour);
      hourAddr += 3;
      temp = 2;
    }
  }
  while (temp == 2)
  {
    readRotaryEncoder();
    encoderButton();
    if(up)
    {
      setMin++;
      up = false;
      if (setMin > 59) setMin = 0;
      if (setMin < 0) setMin = 59;
    }
    else if(down)
    {
      setMin--;
      down = false;
      if (setMin < 0) setMin = 59;
      if (setMin > 59) setMin = 0;
    }
    lcd.setCursor(0,2);
    lcd.print(setHour);
    lcd.print(":");
    lcd.print(setMin);
    if(middle == true)
    {
      middle = false;
      EEPROM.write(minAddr, setMin);
      minAddr += 3;
      temp = 0;
    }
  }
}

void displayTime()
{  
   DateTime now = rtc.now();
   lcd.clear();
   lcd.setCursor(4,0);
   lcd.print(now.hour());
   lcd.print('.');
   lcd.print(now.minute());
   lcd.setCursor(4,1);
   lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
   lcd.setCursor(4,2);
   lcd.print(now.day());
   lcd.print('/');
   lcd.print(now.month());
   lcd.print('/');
   lcd.print(now.year());
   lcd.setCursor(0,3);
   lcd.print("Upcoming:");
   lcd.print(EEPROM.read(hourAddr-3));
   lcd.print('.');
   lcd.print(EEPROM.read(minAddr-3));
   delay(300);
}

void isr()
{ 
  if(!isrflag)
  {
    isrflag = true;
    Serial.println("isr got executed");
  }
}

void timerIsr()
{
  encoder->service();
}

void encoderButton()
{
    ClickEncoder::Button b = encoder->getButton();
     if (b != ClickEncoder::Open) 
     {
     switch (b) 
     {
        case ClickEncoder::Clicked:
           middle = !middle;
          break;
     }
     }
}

void readRotaryEncoder()
{
  value += encoder->getValue();
  
  if (value/2 > last) 
  {
    last = value/2;
    down = true;
    delay(150);
  }
  else if (value/2 < last) 
  {
    last = value/2;
    up = true;
    delay(150);
  }
}
