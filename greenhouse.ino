#include <Arduino.h>
#include <Wire.h>
#include "SHT31.h"
#include "rgb_lcd.h"
#include "timer-api.h"
#include "DS1307.h"
#include <Grove_LED_Bar.h>
#define DEBOUNCE_MS 200
#define disk0 (0x50)

rgb_lcd lcd;
DS1307 clock;
SHT31 sht31 = SHT31();

const int color = 128;
const int button1pin = 9;
const int button2pin = 8;
const int button3pin = 10;
const int button4pin = 11;
const int buzzer1pin =  2;
const int led1pin =  4; //нагрев
const int led2pin =  5; // вентиляция
const int led3pin =  6; // свет
const int led4pin =  7; // полив
const int parameters = 4;
const int incr = 4;
const int timer_adr = 50;
const int day = 86400;
const int notalarmperiod = 10;




volatile float maxtemp = 24;
volatile float maxhum = 35;
volatile float mintemp = 20;
volatile float minhum = 25;
volatile unsigned long minlum = 0;

volatile float newmaxtemp = maxtemp;
volatile float newmaxhum = maxhum;
volatile float newmintemp = mintemp;
volatile float newminhum = minhum;
volatile unsigned long newminlum = minlum;

// last time the button was pressed in ms for buttons debounce
volatile unsigned long prev1_ms;
volatile unsigned long prev2_ms;
volatile unsigned long prev3_ms;
volatile unsigned long prev4_ms;
volatile unsigned long current_ms;
volatile long notalarmcurrent = 0;
volatile long seconds = 0;



volatile bool button1State = HIGH;
volatile bool button2State = HIGH;
volatile bool button3State = HIGH;
volatile bool button4State = HIGH;

volatile int counter = 0;//для смены отображаемых параметров
volatile int setcounter = 0;//для смены отображаемых параметров в режиме setmode
volatile bool changeflag = false;//Сохранил ли пользователь изменения
volatile bool  changeRGBflag = false;//Контроль цвета 
volatile bool EEPromflag = false;
volatile bool Can_Use_Alarm = true;
volatile bool alarmflag = true;
unsigned int limits_adr = 0;

volatile long watertime = 0;
volatile long waterperiod = 10;
volatile long alarm_time = 0;
volatile long alarm_dur = 0;
const int wateringdur = 2;

enum mode {normal, setmode, changemode,alarmmode};
enum alert {none, bighumidity,lowhumidity, bigtemp, lowtemp, dark, bright};

mode state = normal; 
alert wrong = none;

void showbar(){
  lcd.setCursor(3,1);
  lcd.print("<  >  s  p ");
}

void setup() {
  
  
  cli();
  PCIFR |= 0b00000001;
   // turn on port b:
  // https://thewanderingengineer.com/2014/08/11/arduino-pin-change-interrupts/
  PCICR |= 0b00000001;    
  // turn on pin PB0-PB4, which are PCINT0 - PCINT3
  PCMSK0 |= 0b00001111;
  // enable interrupts:
  sei();
  
  Wire.begin();

  //EEPROMWritelong(limits_adr,maxtemp);
  newmaxtemp = EEPROMReadlong(limits_adr);
  newmintemp = EEPROMReadlong(limits_adr+incr);
  newmaxhum = EEPROMReadlong(limits_adr+(incr*2));
  newminhum =  EEPROMReadlong(limits_adr+(incr*3));
  newminlum =  EEPROMReadlong(limits_adr+(incr*4));
  
  maxtemp = newmaxtemp;
  mintemp = newmintemp;
  maxhum = newmaxhum;
  minhum = newminhum;  
    

  timer_init_ISR_1Hz(TIMER_DEFAULT);
  
  pinMode(button1pin, INPUT_PULLUP);
  pinMode(button2pin, INPUT_PULLUP);
  pinMode(button3pin, INPUT_PULLUP);
  pinMode(button4pin, INPUT_PULLUP);
  pinMode(buzzer1pin, OUTPUT);
  pinMode(led1pin, OUTPUT);
  pinMode(led2pin, OUTPUT);
  pinMode(led3pin, OUTPUT);
  pinMode(led4pin, OUTPUT);
  lcd.begin(16, 2);
  lcd.setRGB(0, color, 0);
  lcd.print("Getting started...");
  sht31.begin();  
  delay(1000);
  lcd.clear(); 
}

void stopalarm(){
  digitalWrite(buzzer1pin, LOW);
}

void alarm(){
  digitalWrite(buzzer1pin, HIGH);
  alarm_time = seconds;  
}

void watering(){
  digitalWrite(led4pin, HIGH);
  watertime = seconds;  
}

void Button1Pressed() {
  switch(state){
    case normal:
        if (counter == 2){
            counter = 0;
          }
        else{
            counter++;
          }
        break;
    case setmode: 
        if (setcounter == 4){
          setcounter = 0;
        }
        else{
          setcounter++;
        }
        break;
    case changemode:
        if (setcounter == 0){
            newmaxtemp+=0.5;
        }
        if (setcounter == 1){
            newmaxhum+=0.5;
        }
        if (setcounter == 2){
            newmintemp+=0.5;
        }
        if (setcounter == 3){
            newminhum+=0.5;
        }
        if (setcounter == 4){
            newminlum+=10;
        }
        break;
    /*case alarmmode:
         state = normal;
         stopalarm();  
         changeRGBflag = false;
         Can_Use_Alarm = false;
         notalarmcurrent = seconds;
         break;*/
      
  }
}

void Button2Pressed() {
  switch(state){
    case normal:
        if (counter == 0){
            counter = 2;
          }
        else{
            counter--;
          }
        break;
    case setmode: 
        if (setcounter == 0){
          setcounter = 4;
        }
        else{
          setcounter--;
        }
        break;
    case changemode:
        if (setcounter == 0){
            newmaxtemp-=0.5;
        }
        if (setcounter == 1){
            newmaxhum-=0.5;
        }
        if (setcounter == 2){
            newmintemp-=0.5;
        }
        if (setcounter == 3){
            newminhum-=0.5;
        }
        if (setcounter == 4){
            newminlum-=10;
        }
        break;
  }
}

void Button3Pressed() { //работа кнопки "s"
   switch(state){
      case normal:
         state = setmode;
         changeRGBflag = false;
         break;
      case setmode:
         state = normal;
         changeRGBflag = false;
         break;
      case changemode:
         newmaxtemp = maxtemp;
         newmaxhum = maxhum;
         newmintemp = mintemp;
         newminhum = minhum;
         newminlum = minlum;
         changeflag = false;
         state = setmode;
         changeRGBflag = false;
         break;
      
  }
}

void Button4Pressed() { //работа кнопки "p"
    switch(state){
       case normal:
          state = normal;
          break;
       case setmode:
          state = changemode;
          changeRGBflag = false;
          break;
       case changemode:
          maxtemp = newmaxtemp;
          maxhum = newmaxhum;
          mintemp = newmintemp;
          minhum = newminhum;
          minlum = newminlum;
          state = setmode; 
          changeRGBflag = false;
          EEPromflag = true;
         break;    
    
    }
}




void loop() {
 
 float temp = sht31.getTemperature();
 float hum = sht31.getHumidity();
 int light = analogRead(A0);


 if ((seconds - watertime) == wateringdur){
  digitalWrite(led4pin, LOW);
 }
 if ((seconds - watertime) > waterperiod){
  watering();
 }
/*if ((seconds - notalarmcurrent)>notalarmperiod){
  Can_Use_Alarm = true;
}
else {
  Can_Use_Alarm = false;
}

   //ВОТ ЭТО ВСЁ ПЕРЕДЕЛАТЬ К ХУЯМ, ПОХОДУ ПЬЯНЫМ ЭТО ДЕРЬМИЩЕ НАПИСАЛ
 
  if (light < minlum){
    wrong = dark;
    state = alarmmode;
    changeRGBflag = false;
    alarmflag = true;
  }
  else if(light > 1000){
    state = alarmmode;
    changeRGBflag = false;
    wrong = bright;
    alarmflag = true;
  }
 
  else if (temp < mintemp){
    wrong = lowtemp;
    state = alarmmode;
    changeRGBflag = false;
    alarmflag = true;
  }
  else if(temp > maxtemp){
    wrong = bigtemp;
    state = alarmmode;
    changeRGBflag = false;
    alarmflag = true;
  }
 
  else {
      wrong = none;
      alarmflag = false;
  }*/


 switch(state){
     case normal:     //обычный режим
        if (!changeRGBflag){
          lcd.setRGB(0, color, 0);
          changeRGBflag = 0;
          changeRGBflag = true; 
        }
        switch(counter){
          case 0:
            lcd.clear();
            lcd.setCursor(0,0);    
            lcd.print("Hum = ");
            lcd.print(hum);  
            lcd.print("%");  
            break;
          case 1:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Temp = ");
            lcd.print(temp);
            lcd.print(" "); 
            lcd.print((char)223);
            lcd.print("C");    
            break;
          case 2:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("luminosity = ");
            lcd.print(light);
            break;
         } 
         break;  
   

 
     case setmode:   //режим setmode 

         if (EEPromflag){
          EEPROMWritelong(limits_adr,newmaxtemp);
          EEPROMWritelong(limits_adr+incr,newmintemp);
          EEPROMWritelong(limits_adr+(2*incr),newmaxhum);
          EEPROMWritelong(limits_adr+(3*incr),newminhum);
          EEPROMWritelong(limits_adr+(4*incr),newminlum);
          EEPromflag = false;
         }
         
         if (!changeRGBflag){
              lcd.setRGB(0, color, color);
              changeRGBflag = true;
         }
         switch(setcounter){
             case 0:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("maxtemp = "); 
                lcd.print(newmaxtemp);
                break;
              case 1:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("maxhum = "); 
                lcd.print(newmaxhum);
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("mintemp = "); 
                lcd.print(newmintemp);
                break;
              case 3:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("minhum = "); 
                lcd.print(newminhum);
                break;
              case 4:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("minlumin = "); 
                lcd.print(newminlum);
                break;
         }
         break;
     case changemode:
       if (!changeRGBflag){
           lcd.setRGB(0, 0, color);
           changeRGBflag = true;
       }
        switch(setcounter){
             case 0:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("maxtemp = "); 
                lcd.print(newmaxtemp);
                break;
              case 1:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("maxhum = "); 
                lcd.print(newmaxhum);
                break;
              case 2:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("mintemp = "); 
                lcd.print(newmintemp);
                break;
              case 3:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("minhum = "); 
                lcd.print(newminhum);
                break;
              case 4:
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("minlumin = "); 
                lcd.print(newminlum);
                break;
         }
         /*case alarmmode:
            if (!changeRGBflag){
              lcd.setRGB(color, 0, 0);
              changeRGBflag = true;
            }
            if (alarmflag == false){
              state = normal; 
              changeRGBflag = false;
            }
            if (((seconds - alarm_time) > alarm_dur) and (Can_Use_Alarm == true)){
              //alarm();
            }
            else if ((seconds - alarm_time) < alarm_dur)  {
              stopalarm();  
            }
            switch(wrong){
              case bigtemp:
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("TOO HOT!"); 
                  break;  
              case lowtemp:
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("TOO COLD!"); 
                  break;  
              case bright:
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("TOO BRIGHT!");
                  break;  
              case dark:
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("TOO DARK!");     
                   break;    
                       
            }*/
            
         
         break;
      
 }
   showbar();
   delay(400);  
}
 

ISR(PCINT0_vect) {

  current_ms = millis();
  if (button1State && (digitalRead(button1pin) == LOW)) {
       if (current_ms - prev1_ms > DEBOUNCE_MS) {
          Button1Pressed();
          button1State = LOW;
          prev1_ms = current_ms;
       }  
  } else if (!button1State && (digitalRead(button1pin) == HIGH)) {
    button1State = HIGH;
  }
 
  
  if (button2State && (digitalRead(button2pin) == LOW)) {
       if (current_ms - prev2_ms > DEBOUNCE_MS) {
          Button2Pressed();
          button2State = LOW;
          prev2_ms = current_ms;
       }  
  } else if (!button2State && (digitalRead(button2pin) == HIGH)) {
    button2State = HIGH;
  }

  
  
  if (button3State && (digitalRead(button3pin) == LOW)) {
       if (current_ms - prev3_ms > DEBOUNCE_MS) {
          Button3Pressed();
          button3State = LOW;
          prev3_ms = current_ms;
       }  
  } else if (!button3State && (digitalRead(button3pin) == HIGH)) {
    button3State = HIGH;
  }
  
  
  if (button4State && (digitalRead(button4pin) == LOW)) {
       if (current_ms - prev4_ms > DEBOUNCE_MS) {
          Button4Pressed();
          button4State = LOW;
          prev4_ms = current_ms;
       }  
  } else if (!button4State && (digitalRead(button4pin) == HIGH)) {
    button4State = HIGH;
  }
}

void timer_handle_interrupts(int timer) {
    seconds+=1;
}
