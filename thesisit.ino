#include <Wire.h>
#include <DS3231.h>

DS3231 clock;
RTCDateTime dt;

#include <OneWire.h>
#include <DallasTemperature.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(A4, A5, A3, A2, A1, A0);

int start_ctr = 1;

int feed_msg_ctr = 0;
int pump_msg_ctr = 0;
int float_msg_ctr = 0;

int display_priority = 0;


int hr_12_ctr = 0;
int hr_24_ctr = 0;
int power_led = 34;
int error_led = 36;
int error_ctr = 0;
int error_count = 0;
int error_seconds = 1;

//GSM variables
int gsm_hour[] = {8, 12, 16};
int gsm_minute[] = {0, 0, 0};
int gsm_second[] = {0, 0, 0};

//Feeding time of fishes. Where index[0] will be for 12 hrs and index[1] will be for 24 hrs.
String feed_hour[] = {"00", "00"}; //(24-hour format 00-23)
String feed_minute[] = {"00", "00"}; //(0-59)
String feed_second[] = {"00", "00"}; //(0-59)
String default_hour = "00";
String default_minute = "00";
String default_second = "00";

//pump pins
int pump1_pin = 48; //pin number for relay that will activate the pump1
int pump2_pin = 46; //pin number for relay that will activate the pump2
int pumpcontrol_pin = 11; //pin number for the button of the pump
int pumpcontrol_led = 28; //pin number for the led indicator of the fishfeeder
int pump_seconds = 60; //convert minutes to seconds
int pump_count = 0;
int pump_ctr = 0;

//float switch
int float_pin = 24;
int float_seconds = 300;
int float_count = 0;
int float_ctr = 0;

//feed pins
int feed_pin = 8; //pin number for relay that will activate the fishfeeder
int feedcontrol_pin = 12; //pin number for the button of the fishfeeder
int feedcontrol_led = 26; //pin number for the led indicator of the fishfeeder
int feed_ctr = 0;
int feed_count = 0;
int feed_seconds = 5;//change for the time process of fishfeeder
int feedcontrol_12_hrs = 9;// button for 12 hrs feeding
int feedcontrol_12_hrs_led = 32;
int feedcontrol_24_hrs = 10;// button for 24 hrs feeding
int feedcontrol_24_hrs_led = 30;

int one_sec = 1000;//interval  for one_sec

int system_display_pin = 13;// button pin for displaying the sensors data
int display_ctr = 0;
int display_count = 0;
int display_seconds = 12;

int temp_sensor = 4; //pin number for temperature sensor
float temperature = 0;

float pHValue,voltage;

OneWire oneWirePin(temp_sensor);
DallasTemperature sensors(&oneWirePin);

#define SensorPin A8            //pH meter Analog output to Arduino Analog Input 0
#define Offset -0.64            //deviation compensate
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth 40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0; 

#include "Adafruit_FONA.h"

#define FONA_RST 5

HardwareSerial *fonaSerial = &Serial3;
// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);


void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.print("Initializing...");
  clock.begin();
  sensors.begin();

  fonaSerial->begin(1234);
  if (! fona.begin(*fonaSerial)) {
    lcd.print("Couldn't find GSM");
    while (1);
  }
  
  //GSM pinmode
  pinMode(8, OUTPUT);// GND of GSM module
  digitalWrite(8, LOW);
    
  //Uncomment to set the time and date
  //clock.setDateTime(__DATE__, __TIME__);
  
  pinMode(power_led, OUTPUT);
  digitalWrite(power_led, HIGH);

  pinMode(error_led, OUTPUT);
  digitalWrite(error_led, LOW);
  
  //pinMode(ph_pin, INPUT_PULLUP);
  pinMode(feedcontrol_pin, INPUT_PULLUP);
  pinMode(feedcontrol_led, OUTPUT);
  pinMode(feedcontrol_12_hrs, INPUT_PULLUP);
  pinMode(feedcontrol_12_hrs_led, OUTPUT);
  pinMode(feedcontrol_24_hrs, INPUT_PULLUP);
  pinMode(feedcontrol_24_hrs_led, OUTPUT);

  digitalWrite(feedcontrol_24_hrs_led, LOW);
  digitalWrite(feedcontrol_12_hrs_led, LOW);
  
  pinMode(pumpcontrol_pin, INPUT_PULLUP);
  pinMode(pumpcontrol_led, OUTPUT);
  
  //Temperature sensor pins
  pinMode(2,OUTPUT);//negative pin of temp sensor
  pinMode(3,OUTPUT);//positive pin of temp sensor
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);

  //Ph Level sensor pins
  pinMode(A10,OUTPUT);//negative pin of temp sensor
  pinMode(A9,OUTPUT);//positive pin of temp sensor
  digitalWrite(A10, LOW);
  digitalWrite(A9, HIGH);
  
  pinMode(system_display_pin, INPUT_PULLUP);// button pin for displaying the sensors data
  
  pinMode(feed_pin, OUTPUT);
  digitalWrite(feed_pin, HIGH);

  pinMode(22, OUTPUT);//negative float sensor
  pinMode(pump1_pin, OUTPUT);//control of relay1
  pinMode(pump2_pin, OUTPUT);//control of relay2
  pinMode(24, INPUT_PULLUP);//pin for float sensor
  digitalWrite(22, LOW);
  digitalWrite(pump1_pin, HIGH);
  digitalWrite(pump2_pin, HIGH);

  //lcd power source
  pinMode(A7, OUTPUT);
  pinMode(A11, OUTPUT);
  digitalWrite(A7, LOW);
  digitalWrite(A11, LOW);
}

double avergearray(int* arr, int number){ //function for getting the average of ph values
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}

String format_seconds(String sec){
  sec = String(dt.second);
  if(sec.length() == 1){
    sec = 0 + sec;
  }
  return sec;
}

String format_minutes(String mint){
  mint = String(dt.minute);
  if(mint.length() == 1){
    mint = 0 + mint;
  }
  return mint;
}
void displayDateTime(){ //function for displaying the time on the LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Date: ");
  lcd.print(String(dt.month)+"-"+String(dt.day)+"-"+String(dt.year)+".");
  lcd.setCursor(0,1);
  lcd.print("Time: ");
  lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"..");
}

void feedfish(){ //function for the fishfeeder
  if(feed_count < feed_seconds){
    if(pump_msg_ctr == 0 && float_msg_ctr == 0 && feed_msg_ctr == 1 && display_priority == 0){
      lcd.setCursor(0,0);
      lcd.print("Feeding Fishes..");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"..");
    }
    digitalWrite(feed_pin, LOW);
    digitalWrite(feedcontrol_led, HIGH);
  } else {
    digitalWrite(feed_pin, HIGH);
    digitalWrite(feedcontrol_led, LOW);
    feed_ctr = 0;
    feed_count = 0;
    feed_msg_ctr = 0;
  }
}

void pumpwater(){ //function for the water pump
  if(pump_count < pump_seconds){
    if(pump_msg_ctr == 1 && float_msg_ctr == 0 && feed_msg_ctr == 0 && display_priority == 0){
      lcd.setCursor(0,0);
      lcd.print("Pumping water..");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"..");
    }
    digitalWrite(pump1_pin, LOW);
    digitalWrite(pump2_pin, LOW);
    digitalWrite(pumpcontrol_led, HIGH);
  } else {
    digitalWrite(pump1_pin, HIGH);
    digitalWrite(pump2_pin, HIGH);
    digitalWrite(pumpcontrol_led, LOW);
    pump_ctr = 0;
    pump_count = 0;
    pump_msg_ctr = 0;
  }
}

void floatswitch(){ //function for the water pump
  if(float_count < float_seconds){
    if(pump_msg_ctr == 0 && float_msg_ctr == 1 && feed_msg_ctr == 0 && display_priority == 0){
      lcd.setCursor(0,0);
      lcd.print("Float Sen active");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"..");
    }
    digitalWrite(pump1_pin, LOW);
    digitalWrite(pump2_pin, LOW);
  } else {
    digitalWrite(pump1_pin, HIGH);
    digitalWrite(pump2_pin, HIGH);
    float_ctr = 0;
    float_count = 0;
    float_msg_ctr = 0;
  }
}

float getTempe(){ //function for getting the temperature
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  return temperature;
}

float getPh(){ //function for getting the ph value
  pHArray[pHArrayIndex++]=analogRead(SensorPin);
  if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
  voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
  pHValue = 3.5*voltage+Offset;
  return pHValue;
}

void getSystemData(){
  if(display_count < 6){
    lcd.setCursor(0,0);
    lcd.print("*Temp: " + String(getTempe()) + "***");
    lcd.setCursor(0,1);
    lcd.print("*PH Level: " + String(getPh()) + "**");
  } else if(display_count < 10){
    lcd.setCursor(0,0);
    String feed_setting = "--Not-yet-set---";
    if(digitalRead(feedcontrol_12_hrs_led) == HIGH){
      feed_setting = "12 hours feeding";
    }else if(digitalRead(feedcontrol_24_hrs_led) == HIGH){
      feed_setting = "24 hours feeding";
    }
    lcd.print("--Feed-Setting--");
    lcd.setCursor(0,1);
    lcd.print(feed_setting);
  } else if(display_count < 12){
    lcd.setCursor(0,0);
    lcd.print("-Next-Feed-Time-");
    lcd.setCursor(0,1);
    String feed_time = "--Not-yet-set---";
    if(digitalRead(feedcontrol_12_hrs_led) == HIGH){
      feed_time = String("----"+feed_hour[0]) +":"+ String(feed_minute[0]) +":"+ String(feed_second[0]+"-----"); 
    }else if(digitalRead(feedcontrol_24_hrs_led) == HIGH){
      feed_time = String("----"+feed_hour[1]) +":"+ String(feed_minute[1]) +":"+ String(feed_second[1]+"-----");
    }
    lcd.print(feed_time);
  } else {
    display_priority = 0;
    display_ctr = 0;
    display_count = 0;
  }
}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}

void sendSMS(char num[], int len_num, char msg[], int len_msg){//need for testing char num[], int len_num, 
  char sendto[len_num+1], message[len_msg+1];
  flushSerial();
  for(int x=0;x<len_num;x++){
    sendto[x] = num[x];
  }
  sendto[len_num] = '\0';
  for(int x=0;x<len_msg;x++){
    message[x] = msg[x];
  }
  message[len_msg] = '\0';
  
  if (!fona.sendSMS(sendto, message)) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SMS Report sent!");
    delay(1000);
  }
}

void errorled(){
  if(float_count < float_seconds){
    digitalWrite(error_led, HIGH);
  } else {
    digitalWrite(error_led, LOW);
    error_ctr = 0;
    error_count = 0;
  }
}

void loop() {
  static unsigned long samplingTime = millis();
  static unsigned long samplingTime2 = millis();
  
  dt = clock.getDateTime();
  if(start_ctr == 0){
    if((dt.hour == gsm_hour[0] && dt.minute == gsm_minute[0] && dt.second == gsm_second[0]) || 
        (dt.hour == gsm_hour[1] && dt.minute == gsm_minute[1] && dt.second == gsm_second[1]) || 
        (dt.hour == gsm_hour[2] && dt.minute == gsm_minute[2] && dt.second == gsm_second[2])){
      String msg_alert = "'ph':'" + String(getPh()) + "', 'temp':'" + String(getTempe()) + "'";
      String snum = "21583883";
      int len_num = snum.length();
      char cnum[len_num];
      for(int x=0;x<len_num;x++){
        cnum[x] = snum.charAt(x);
      }
      int len_msg = msg_alert.length();
      char cmsg[len_msg];
      for(int x=0;x<len_msg;x++){
        cmsg[x] = msg_alert.charAt(x);
      }
      sendSMS(cnum, len_num, cmsg, len_msg);
    }
    
    if(digitalRead(feedcontrol_pin) == LOW || feed_ctr == 1){ //will check if the button for fishfeeder is pressed
          if(digitalRead(feedcontrol_12_hrs_led) == LOW && digitalRead(feedcontrol_24_hrs_led) == LOW){
            default_hour = dt.hour;
            default_minute = format_minutes(String(dt.minute));
            default_second = format_seconds(String(dt.second));
            feed_hour[1] = default_hour;
            feed_minute[1] = default_minute;
            feed_second[1] = default_second;
            digitalWrite(feedcontrol_24_hrs_led, HIGH);
            hr_24_ctr = 1;
          } else if(feed_ctr == 0){
            default_hour = dt.hour;
            default_minute = format_minutes(String(dt.minute));
            default_second = format_seconds(String(dt.second));
            if(hr_24_ctr == 1){
              feed_hour[1] = dt.hour;
              feed_minute[1] = format_minutes(String(dt.minute));
              feed_second[1] = format_seconds(String(dt.second));
            } else if(hr_12_ctr == 1){
              feed_hour[0] = dt.hour;
              feed_minute[0] = format_minutes(String(dt.minute));
              feed_second[0] = format_seconds(String(dt.second));
            }
          }
          feed_msg_ctr = 1;
          feed_ctr = 1;
          feedfish();
    }
  
    if(digitalRead(pumpcontrol_pin) == LOW || pump_ctr == 1){ //will check if the button for water pump is pressed
      pump_msg_ctr = 1;
      pump_ctr = 1;
      pumpwater();
    }
  
    if(digitalRead(system_display_pin) == LOW || display_ctr == 1){
      display_priority = 1;
      display_ctr = 1;
      getSystemData();
    }
  
    if(digitalRead(float_pin) == LOW || float_ctr == 1){//will check if float switch activated
      float_msg_ctr = 1;
      float_ctr = 1;
      floatswitch();
    }
  
    if(getPh() < 4.6){
        error_ctr = 1;
        errorled();
        pump_ctr = 1;
        pumpwater();
    }
    
    if(digitalRead(feedcontrol_12_hrs) == LOW){ //will check if the button for water pump is pressed
      digitalWrite(feedcontrol_12_hrs_led, HIGH);
      digitalWrite(feedcontrol_24_hrs_led, LOW);
      feed_hour[0] = String(default_hour.toInt() + 12);
      feed_minute[0] = default_minute;
      feed_second[0] = default_second;
      if(feed_hour[0].toInt() > 23){
        feed_hour[0] = String(feed_hour[0].toInt() - 24);
      }
      hr_12_ctr = 1;
      hr_24_ctr = 0;
    }
  
    if(digitalRead(feedcontrol_24_hrs) == LOW){ //will check if the button for water pump is pressed
      digitalWrite(feedcontrol_24_hrs_led, HIGH);
      digitalWrite(feedcontrol_12_hrs_led, LOW);
      feed_hour[1] = default_hour;
      feed_minute[1] = default_minute;
      feed_second[1] = default_second;
      hr_12_ctr = 0;
      hr_24_ctr = 1;
    }
  
    if((hr_12_ctr == 1 && dt.hour == feed_hour[0].toInt() && format_minutes(String(dt.minute)).toInt() == feed_minute[0].toInt() && format_seconds(String(dt.second)).toInt() == feed_second[0].toInt()) || feed_ctr == 1){
      feed_ctr = 1;
      feedfish();
    }else if((hr_24_ctr == 1 && dt.hour == feed_hour[1].toInt() && format_minutes(String(dt.minute)).toInt() == feed_minute[1].toInt() && format_seconds(String(dt.second)).toInt() == feed_second[1].toInt()) || feed_ctr == 1){
      feed_ctr = 1;
      feedfish();
    }
  }
  if(millis()-samplingTime > samplingInterval) //used for getting the ph level
  {
      pHArray[pHArrayIndex++]=analogRead(SensorPin);
      if(pHArrayIndex==ArrayLenth)pHArrayIndex=0;
      voltage = avergearray(pHArray, ArrayLenth)*5.0/1024;
      pHValue = 3.5*voltage+Offset;
      samplingTime=millis();
  }
  
  if(millis()-samplingTime2 > one_sec){ //non-blocking one second delay
    lcd.begin(16,2);
    if(display_ctr == 1){
      if(display_count < display_seconds){
        display_count++;
      }
    }
    
    if(feed_ctr == 1){
      if(feed_count < feed_seconds){
        feed_count++;
      }
    }

    if(pump_ctr == 1){
      if(pump_count < pump_seconds){
        pump_count++;
      }
    }

    if(float_ctr == 1){
      if(float_count < float_seconds){
        float_count++;
      }
    }

    if(error_ctr == 1){
      if(error_count < error_seconds){
        error_count++;
      }
    }
    
    if(feed_ctr == 0 && pump_ctr == 0 && display_ctr == 0 && float_ctr == 0){
      displayDateTime();
      start_ctr = 0;
    }
    samplingTime2=millis();
  }
}
