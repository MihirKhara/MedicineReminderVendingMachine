/*
lcd        - 8,9,4,5,6,7
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
stepper - 24-31
med detector sensor - 18
df player -

*/
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <ClickEncoder.h>
#include <TimerOne.h>


#define yled 36
#define rled 37
#define sensor 18
#define clk A0
#define dt A1
#define sw A2
#define rs 8
#define rw 9
#define d4 4
#define d5 5
#define d6 6
#define d7 7
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

int hourAddr = 2;
int minAddr  = 3;
int ChourAddr = 11;
int CminAddr  = 12;
int dayAddr   = 13;

int checkHour;
int checkMin;
int CcheckHour;
int CcheckMin;
int checkDay;

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

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
ClickEncoder *encoder;
int16_t last, value; 
byte arrow[8] = { 0x00 , 0x04, 0x0C, 0x1F, 0x0C, 0x04, 0x00};
byte blank[8] = { 0x00 , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void setup () 
{

  Serial.begin(9600);
  rtc.begin();
  lcd.begin(16,4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

  EEPROM.write(1,1);
  
  attachInterrupt(digitalPinToInterrupt(sensor),isr,RISING);
  pinMode(yled,OUTPUT);
  pinMode(rled,OUTPUT);
  digitalWrite(rled,LOW);
  digitalWrite(yled,LOW);


  lcd.createChar(0, arrow);
  lcd.createChar(1, blank);

  encoder = new ClickEncoder(dt, clk, sw); 
  encoder->setAccelerationEnabled(false);
  last = encoder->getValue();

  EEPROM.write(1, 1);


  if (rtc.lostPower()) 
  {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

  /////////////////////////////////////////////
 //                Loop                 //
/////////////////////////////////////////////

void loop () 
{   

    DateTime now = rtc.now();
    if(EEPROM.read(1))
    {
       firstBoot();
    } 
    Serial.print(EEPROM.read(2));
  Serial.print("/");
  Serial.print(EEPROM.read(3));
  Serial.print("/");
  Serial.println(daysOfTheWeek[EEPROM.read(0)]);

    Serial.print(EEPROM.read(11));
  Serial.print("/");
  Serial.print(EEPROM.read(12));
  Serial.print("/");
  Serial.println(daysOfTheWeek[EEPROM.read(13)]);
 /*   int x = 2, y = 3, z = 4;
   for(int i = 0; i < 11; i++)
    {
    Serial.print(EEPROM.read(x));
    Serial.print("/");
    Serial.print(EEPROM.read(y));
    Serial.print("/");
    Serial.println(daysOfTheWeek[EEPROM.read(z)]);
    x += 3;
    y += 3;
    z += 3;
  }
*/
  encoderButton();
  if(middle)
  {
    middle = !middle;
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
  
    if(isrflag)
    {
      isrflag = false;
      delay(500);
      if(!digitalRead(sensor))
      { 
        digitalWrite(yled,HIGH);
        Serial.println("med drop");
        delay(500);
      }
      else if(digitalRead(sensor))
      { 
        digitalWrite(rled,HIGH);
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
  hourAddr = 2;
  minAddr  = 3;
  ChourAddr = 11;
  CminAddr  = 12;
  dayAddr   = 13;
  DateTime now = rtc.now();
  if(checkTemp1 == !checkTemp2)
  { 
    checkHour = EEPROM.read(hourAddr);
    checkMin  = EEPROM.read(minAddr);
    hourAddr += 3;
    minAddr  += 3;
    if(hourAddr > 8) hourAddr = 2;
    if(minAddr > 9)  minAddr  = 3;
    checkTemp2 = false;
  }
  if(CcheckTemp1 == !CcheckTemp2)
  {
    Serial.println("Positive");
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
    Serial.println(currDay);
    
  if(currHour == checkHour)
  {
    if(currMin == checkMin || currMin == (checkMin + 5) || currMin == (checkMin + 10) || currMin == (checkMin + 15))
    {
      checkTemp2 = true;
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
    }
  }

  if(currDay == checkDay)
  {
    if(currHour == CcheckHour)
    {
      if(currMin == CcheckMin || currMin == (CcheckMin + 5) || currMin == (CcheckMin + 10) || currMin == (CcheckMin + 15))
      {
        CcheckTemp2 = true;
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
      }
    }
  }

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
  lcd.print(EEPROM.read(100));
  lcd.print(EEPROM.read(101));
  lcd.setCursor(0,1);
  lcd.print(item2);
  lcd.print(EEPROM.read(102));
  lcd.print(EEPROM.read(103));
  lcd.setCursor(0,2);
  lcd.print(item3);
  lcd.print(EEPROM.read(104));
  lcd.print(EEPROM.read(105));
  lcd.setCursor(0,3);
  lcd.print(item4);
  lcd.print(EEPROM.read(106));
  lcd.print(EEPROM.read(107));
  delay(300);
}

void firstBoot()
{
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Welcome New User");
  delay(5000);
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

  lcd.setCursor(0,0);
  lcd.print("Set time for");
  lcd.setCursor(0,1);
  lcd.print("custom dozes of");
  lcd.setCursor(0,2);
  lcd.print("each day");
  delay(5000);
    for(cD = 0; cD < 7; cD++) customMedTime();
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
   lcd.print(EEPROM.read(100));
   lcd.print('.');
   lcd.print(EEPROM.read(101));
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