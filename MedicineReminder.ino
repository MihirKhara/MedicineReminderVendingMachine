/*
* Development Board: Arduino MEGA2560 
* Project Name:   <Medicine Reminder and Vending Machine>
* Author List:    <Mihir Khara>
* Filename:     <MedicineReminder.ino>
* Functions:    All functions expect and return void
*                <alarmCheck, regAlarmCheck, dataLoggingMissed, dataLoggingTaken, medFall,
*                medTaken, displayHistory, firstBoot, regAlarm, customMedTime, medTime, 
*                displayTime, isr, timerIsr, encoderButton, readRotaryEncoder>
* Global Variables: <  Booleans-
*                      checkTemp1, checkTemp2 , CcheckTemp1 , CcheckTemp2 ,
*                      isrflag , firstTime, up , down, middle, displaySwitch
*
*                      Integers-
*                      timer, cD, setHour, setMin, setDay, rA, hourAddr, minAddr, ChourAddr, 
*                      CminAddr, dayAddr, checkHour, checkMin, checkHourTempVar, checkMinTempVar,  
*                      CcheckHour, CcheckMin, CcheckHourTempVar, CcheckMinTempVar, checkDay, 
*                      alarmHour, alarmMin, currHour, currMin, currDay, pos, dozeCounter, 
*                      last, value, sd_x, sd_y, dataDay, dataMonth, dataYear, dataHourFall, 
*                      dataMinuteFall, dataHourTaken, dataMinuteTaken
*
*                      Strings-
*                      item1, item2, item3, item4, dataDayOfWeek >
*                           
*/
/* Library credits to all respected developers */
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

//definition of pins
#define clearServo 10
#define r1p1 4
#define r1p2 5
#define r2p1 7
#define r2p2 6 
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
LiquidCrystal lcd(rs, rw, d4, d5, d6, d7); 

//Booleans to control the alarm time which needs to be checked
boolean checkTemp1 = false;
boolean checkTemp2 = true;
boolean CcheckTemp1 = false;
boolean CcheckTemp2 = true;

volatile boolean isrflag = false ;    //variable used to detect the medicine drop
unsigned int timer = 0;
//variables for setting the doze times
int cD;             //custome doze
int setHour;
int setMin;
int setDay;
int rA;             //regular alarm

// In the entire code, similar variables will be found,
// for example hourAddr and ChourAddr 
// these variables have the same use, the normal variable handles the normal doze times, 
// and the variable with a C initial handles the custom doze times

//variables used to hold EEPROM addresses
int hourAddr = 2;
int minAddr  = 3;
int ChourAddr = 11;
int CminAddr  = 12;
int dayAddr   = 13;
boolean firstTime = true;     //varibale to detect if the machine is booted for the first time

//variables to check the current time with time set by care taker
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

//variables to store current time
int currHour = 0;
int currMin  = 0;
int currDay  = 0;

//strings for displaying dozes on LCD
String item1;
String item2;
String item3;
String item4;

//booleans for reading the rotary encoder
boolean up = false;
boolean down = false;
boolean middle = false;
boolean displaySwitch = false;

//variables to write data to the sd card
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

Servo r1s1,r1s2;          //servo objects
Servo r2s1,r2s2;          // r stands for rack, s stands for servo
Servo discardServo;       //object for servo used to empty the medicine rack
int pos = 0;              //variable for controlling servo position
int dozeCounter = 0;      //variable to count number of dozes delivered
RTC_DS3231 rtc;           //rtc object
File myFile;              //sd card file object
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
ClickEncoder *encoder;    //for rotary encoder
int16_t last, value;      

//the following code was found in the df player library and needed to be included in the main code
//it is used to create a df player object
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

//setup begins
void setup () 
{
  //all the objects are started and communication is established between the different modules interfaced
  pinMode(ChipSelect, OUTPUT);
  digitalWrite(ChipSelect, HIGH);
  delay(100);
  rtc.begin();
  lcd.begin(16,4);
  SD.begin(ChipSelect);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

  EEPROM.write(1,1);                  //this is used to check if the machine is booted for the first time or not
  
  attachInterrupt(digitalPinToInterrupt(sensor),isr,RISING);  //interrupt attached to the sensor which detects med drop
  pinMode(rled,OUTPUT);
  digitalWrite(rled,LOW);

  //df player initializations
  mp3.begin();
  uint16_t volume = mp3.getVolume();
  mp3.setVolume(30);  
  uint16_t count = mp3.getTotalTrackCount();

  //rotary encoder initializations
  encoder = new ClickEncoder(dt, clk, sw); 
  encoder->setAccelerationEnabled(false);
  last = encoder->getValue();

  //servo initializations
  r1s1.attach(r1p1);  
  r1s2.attach(r1p2); 
  r2s1.attach(r2p1);  
  r2s2.attach(r2p2); 
  discardServo.attach(clearServo);
  r1s1.write(0);
  r1s2.write(90);
  r2s1.write(0);
  r2s2.write(90);
}

//loop begins
void loop () 
{   

    DateTime now = rtc.now();         //current time is read
    if(EEPROM.read(1))                //this condition checks if the machine is booted for the first time or not
    {
      firstBoot();                    //function to handle the first boot up is called
    }
    
    encoderButton();                  //rotary encoder is read
    if(middle)                        //system detects if encoder button is pressed
    {
      middle = false;
      displaySwitch = !displaySwitch;
    }

    if(displaySwitch)
    {
      displayTime();                  //if the system detects that the user has pressed the encoder button, time is displayed
    }

    if(!displaySwitch)                //if the system detects that the user has pressed the encoder button, doze history is displayed
    {
      displayHistory();
    } 
      
    alarmCheck();                     //function to check the prescription time set is called
    regAlarmCheck();                  //function to check the regular alarm is called


    //this part of the code reads the ISR Flag and IR sensor and decides if the medicine is taken by the user or not
    if(isrflag)                       
    {
      isrflag = false;
      delay(500);
      if(!digitalRead(sensor))
      { 
        medFall();                           //save the current time
        delay(500);
      }
      else if(digitalRead(sensor))
      { 
        medTaken();                          //save the current time
        dataLoggingTaken();                  //write the current time to sd card
        delay(500);
      }
    }
}

/*
*
* Function Name:  alarmCheck
* Input:    None
* Output:   None
* Logic:    This function checks the current time with the data of time set by user to deliver the dozes as per the prescription
            If the time matches, the medicine doze is delivered and user is alerted via a speaker and LED
            It also checks if the user has missed the doze or not and calls function to store data in sd card accordingly
* Example Call:   void alarmCheck()
*
*/
void alarmCheck()
{
  if(firstTime)                           //necessary initializations that need to be done first time the machine is booted
  {
    hourAddr = 2;
    minAddr  = 3;
    ChourAddr = 11;
    CminAddr  = 12;
    dayAddr   = 13;
    firstTime = false;
  }
  DateTime now = rtc.now();

  //this part of code checks the checkTemp2 variable and decides if the address of the eeprom should be incremented or not
  if(checkTemp2)
  { 
    checkHour = EEPROM.read(hourAddr);              
    checkMin  = EEPROM.read(minAddr);
    hourAddr += 3;
    minAddr  += 3;
    if(hourAddr > 8) hourAddr = 2;
    if(minAddr > 9)  minAddr  = 3;
    checkTemp2 = false;
  }

  //this part of code checks the checkTemp2 variable and decides if the address of the eeprom should be incremented or not
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


  //logic to check the current time with the time set by user beigns
  if(currHour == checkHour)
  {
    if(currMin == checkMin)
    {
      //variables for checking are updated everytime a doze is delivered 
      checkMinTempVar = currMin;
      checkHourTempVar = currHour;
      checkTemp2 = true;
      dozeCounter++;              // this counter decides which rack should be activated to deliver the doze
       if(dozeCounter < 15)
        {

                //logic for activating servo motors to deliver the medicine
                //two servos are activated one after the other to drop the medicine box
                for (pos = 90; pos >= 0; pos-= 10)
                {
                  r1s2.write(pos);
                  delay(15);
                }
               
                for (pos = 0; pos <= 90; pos+= 10)
                {
                  r1s2.write(pos);
                  delay(15);
                }
               

                for (pos = 0; pos <= 110; pos+= 10)
                {
                  r1s1.write(pos);
                  delay(15);
                }
                
                for (pos = 110; pos >= 0; pos-= 10)
                {
                  r1s1.write(pos);
                  delay(15);
                }

        }
        else if((dozeCounter > 14) && (dozeCounter < 29))
        {
                for (pos = 90; pos >= 0; pos-= 10)
                {
                  r2s2.write(pos);
                  delay(15);
                }

                for (pos = 0; pos <= 90; pos+= 10)
                {
                  r2s2.write(pos);
                  delay(15);
                }


                for (pos = 0; pos <= 110; pos+= 10)
                {
                  r2s1.write(pos);
                  delay(15);
                }

                for (pos = 110; pos >= 0; pos-= 10)
                {
                  r2s1.write(pos);
                  delay(15);
                }

                if(dozeCounter == 28) dozeCounter = 0;
        }

      //rapid blinking of led
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
      //playing audio or ringtone through the speaker
      mp3.playMp3FolderTrack(4);  
      delay(2000);
      mp3.playMp3FolderTrack(7);  
      delay(60000);
    }
  }

  //if the patient haven't taken the medicine after 5 or 10 minutes, he/she is alerted again
  if(currHour == checkHourTempVar)
  {
    if((currMin == (checkMinTempVar + 5) || currMin == (checkMinTempVar + 10)) && (!digitalRead(sensor)))
    {
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
      dataLoggingMissed();
      for (pos = 0; pos <= 110; pos += 10)
      {
        discardServo.write(pos);
        delay(15);
      }
      for (pos = 110; pos >= 0; pos -= 10)
      {
        discardServo.write(pos);
        delay(15);
      }
    }
  }

  //this part of code has the same logic as above, it checks for custom medicine dozes
  if(currDay == checkDay)
  {
    if(currHour == CcheckHour)
    {
      if(currMin == CcheckMin)
      {
        CcheckMinTempVar = currMin;
        CcheckHourTempVar = currHour;
        CcheckTemp2 = true;
        dozeCounter++;
        if(dozeCounter < 15)
        {
                for (pos = 90; pos >= 0; pos-= 10)
                {
                  r1s2.write(pos);
                  delay(15);
                }
               
                for (pos = 0; pos <= 90; pos+= 10)
                {
                  r1s2.write(pos);
                  delay(15);
                }
               

                for (pos = 0; pos <= 110; pos+= 10)
                {
                  r1s1.write(pos);
                  delay(15);
                }
                
                for (pos = 110; pos >= 0; pos-= 10)
                {
                  r1s1.write(pos);
                  delay(15);
                }

        }
        else if((dozeCounter > 14) && (dozeCounter < 29))
        {
                for (pos = 90; pos >= 0; pos-= 10)
                {
                  r2s2.write(pos);
                  delay(15);
                }

                for (pos = 0; pos <= 90; pos+= 10)
                {
                  r2s2.write(pos);
                  delay(15);
                }


                for (pos = 0; pos <= 110; pos+= 10)
                {
                  r2s1.write(pos);
                  delay(15);
                }

                for (pos = 110; pos >= 0; pos-= 10)
                {
                  r2s1.write(pos);
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
        delay(60000);
      }
    }
    if(currHour == CcheckHourTempVar)
    {
      if((currMin == (CcheckMinTempVar + 5) || currMin == (CcheckMinTempVar + 10)) && (!digitalRead(sensor)))
      {
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


      //this part of code checks that if the patient has not taken the doze after 20 minutes, the rack is cleared 
      // for the next medicine so that confusion to take the correct doze does'nt happen when the next doze drops
      //data is written to sd card that doze is missed
      if(currMin == (CcheckMinTempVar + 15) && (!digitalRead(sensor)))
      {
        dataLoggingMissed();
        for (pos = 0; pos <= 110; pos += 10)
        {
          discardServo.write(pos);
          delay(15);
        }
        for (pos = 110; pos >= 0; pos -= 10)
        {
          discardServo.write(pos);
          delay(15);
        }
      }
    }
  }

}

/*
*
* Function Name:  regAlarmCheck
* Input:    None
* Output:   None
* Logic:    This function checks the current time with the data of time set by user to activate the alarm
* Example Call:   void regAlarmCheck()
*
*/
void regAlarmCheck()
{
  DateTime now = rtc.now();
  alarmHour = now.hour();
  alarmMin = now.minute();
  if(alarmHour == EEPROM.read(32))     //time is checked with the stored time and alarm starts ringing accordingly
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

/*
*
* Function Name:  dataLoggingTaken
* Input:    None
* Output:   None
* Logic:    This function writes data to sd card
* Example Call:   void dataLoggingTaken()
*
*/
void dataLoggingTaken()
{ 
  //this string is written on sd card and it contains all the information of medicine doze
  String dataString = String(dataDay) + "." + String(dataMonth) + "." + String(dataYear) + ", " + String(dataDayOfWeek) + ", " + String(dataHourFall) + ":" + String(dataMinuteFall) + ", " + String(dataHourTaken) + ":" + String(dataMinuteTaken);
  myFile = SD.open("NewData.csv", FILE_WRITE);
    if (myFile) 
    {
      myFile.println(dataString);
      myFile.close();
    }
    EEPROM.write(sd_x, dataHourTaken);
    sd_x = sd_x + 2;
    EEPROM.write(sd_y, dataMinuteTaken);
    sd_y = sd_y + 2;
    if(sd_x > 106) sd_x = 100;
    if(sd_y > 107) sd_y = 101;
}

/*
*
* Function Name:  dataLoggingMissed
* Input:    None
* Output:   None
* Logic:    This function writes data to sd card
* Example Call:   void dataLoggingTMissed()
*
*/
void dataLoggingMissed()
{ 
  String dataString = String(dataDay) + "." + String(dataMonth) + "." + String(dataYear) + ", " + String(dataDayOfWeek) + ", " + String(dataHourFall) + ":" + String(dataMinuteFall) + ", " + "Missed";
  myFile = SD.open("NewData.csv", FILE_WRITE);
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

/*
*
* Function Name:  medFall
* Input:    None
* Output:   None
* Logic:    This function updates the variables with the current time from RTC. It is called when the medicine doze falls
* Example Call:   void medFall()
*
*/
void medFall()
{  
  //reading the time medicine falls
  DateTime now = rtc.now();
  dataDay = now.day();
  dataMonth = now.month();
  dataYear = now.year();
  dataDayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];
  dataHourFall = now.hour();
  dataMinuteFall = now.minute();
}

/*
*
* Function Name:  medTaken
* Input:    None
* Output:   None
* Logic:    This function updates the variables with the current time from RTC. It is called when the medicine doze is taken
* Example Call:   void medTaken()
*
*/
void medTaken()
{
  //reading the time medicine is taken
  DateTime now = rtc.now();
  dataHourTaken = now.hour(); 
  dataMinuteTaken = now.minute();
}

/*
*
* Function Name:  displayHistory
* Input:    None
* Output:   None
* Logic:    This function displays the dozes of the day on the LCD
* Example Call:   void displayHistory()
*
*/
void displayHistory()
{
  //the doze times are displayed on LCD
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
  lcd.print(EEPROM.read(ChourAddr));      //upcoming doze time is displayed
  lcd.print(":");
  lcd.print(EEPROM.read(CminAddr));
  delay(300);
}

/*
*
* Function Name:  firstBoot
* Input:    None
* Output:   None
* Logic:    This function is called only once. It takes the input from the user of the doze and alarm times according to the prescription
* Example Call:   void firstBoot()
*
*/
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
  medTime();                    //function is called to set the time
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
  mp3.playMp3FolderTrack(1);  
  delay(6000);
  for(cD = 0; cD < 7; cD++) customMedTime(); //function is called to set the time

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set time for");
  lcd.setCursor(0,1);
  lcd.print("alarms");
  delay(2000);
  for (rA = 0; rA < 3; rA++) regAlarm();   //function is called to set the time

  mp3.playMp3FolderTrack(2);    
  delay(3000); 
}

/*
*
* Function Name:  regAlarm
* Input:    None
* Output:   None
* Logic:    This function is called only once. It takes the input from the user of the alarm times
* Example Call:   void regAlarm()
*
*/
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

/*
*
* Function Name:  customMedTime
* Input:    None
* Output:   None
* Logic:    This function is called only once. It takes the input from the user of the custom doze times of each day
* Example Call:   void customMedTime()
*
*/
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

/*
*
* Function Name:  medTime
* Input:    None
* Output:   None
* Logic:    This function is to store the input given by user in memory of the microcontroller.
* Example Call:   void medTime()
*
*/
void medTime()
{
  int temp = 1;
  setHour = 0;
  setMin = 0;
  while (temp == 1)           //hour is set
  {
    readRotaryEncoder();
    encoderButton();
    if(EEPROM.read(1))
    { 
      EEPROM.write(1,0);
      middle = false;
    }
    if(up)                  //checks if the encoder is rotated in upwards direction
    {
      setHour++;
      up = false;
      if (setHour > 23) setHour = 0;
      if (setHour < 0) setHour = 23;
    }
    else if(down)           //checks if the encoder is rotated in downwards direction            
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
    if(middle == true)    //checks if button is pressed
    { 
      middle = false;
      EEPROM.write(hourAddr, setHour); //time is set for the doze
      hourAddr += 3;
      temp = 2;
    }
  }
  while (temp == 2)           //minutes are set
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

/*
*
* Function Name:  displayTime
* Input:    None
* Output:   None
* Logic:    This function is called to display the current time and upcoming doze on the LCD
* Example Call:   void displayTime()
*
*/
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

/*
*
* Function Name:  isr
* Input:    None
* Output:   None
* Logic:    This is the ISR that gets executed when medicine drops or is taken and a boolean is updated
* Example Call:   void isr()
*
*/
void isr()
{ 
  if(!isrflag)
  {
    isrflag = true;
  }
}

/*
*
* Function Name:  timerIsr
* Input:    None
* Output:   None
* Logic:    This is a timer ISR which is used to read the rotary encoder
* Example Call:   void timerIsr()
*
*/
void timerIsr()
{
  encoder->service();
}

/*
*
* Function Name:  encoderButton
* Input:    None
* Output:   None
* Logic:    This function is used to detect the clicks of the encoder button
* Example Call:   void encoderButton()
*
*/
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

/*
*
* Function Name:  readRotaryEncoder
* Input:    None
* Output:   None
* Logic:    This function is used to deterct the updates in rotary encoder
* Example Call:   void readRotaryEncoder()
*
*/
void readRotaryEncoder()
{
  //value is updated according to the updates in the encoder position
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
