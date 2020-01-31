For using the code, following components will be needed:
Arduino mega
Df player
SD card module
Speaker
DS3231 RTC module
16x4 LCD
Rotary encoders
IR sensor
5 Servo motors
2 SD cards
DC adapter

Connections are as follows:
Servos - 4,5    rack servos
         6,7    rack servos
         10     clear servo
LCD    - 8,9,22,23,24,25   (rs, rw, d4, d5, d6, d7)
LED    - 37
RTC    - scl-21
             sda-20
Rotary Encoder - clk-A0
                 dt -A1
                 sw -A2
SD card module - cs   -53
                 sck  -52
                 mosi -51
                 miso -50
IR sensor - 18
DF player - tx - 0
            rx - 1


The libraries in the zip file should also be installed in the Arduino libraries folder
Path is : Documents-Arduino-Libraries