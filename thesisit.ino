#include <Wire.h>
#include <DS3231.h>
#include <Servo.h>

Servo myservo;
DS3231 clock;
RTCDateTime dt;

#include <OneWire.h>
#include <DallasTemperature.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(A4, A5, A3, A2, A1, A0);

int start_ctr = 1;
int start_count = 0;
int start_seconds = 22;

int feed_msg_ctr = 0;
int pump_msg_ctr = 0;
int float_msg_ctr = 0;

int display_priority = 0;
int disable_pump_ctr = 0;
int disable_float_ctr = 0;

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
String gsm_msg = "";
String gsm_cmd = "";

//Feeding time of fishes. Where index[0] will be for 12 hrs and index[1] will be for 24 hrs.
int feed_hour[] = {9, 16}; //(24-hour format 00-23)
int feed_minute[] = {0,30}; //(0-59)
int feed_second[] = {0, 0}; //(0-59)

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
int float_seconds = 180;
int float_count = 0;
int float_ctr = 0;

//feed pins
int feed_pin = 8; //pin number for servo that will activate the fishfeeder
int feedcontrol_pin = 12; //pin number for the button of the fishfeeder
int feedcontrol_led = 26; //pin number for the led indicator of the fishfeeder
int feed_ctr = 0;
int feed_count = 0;
int feed_seconds = 2;//change for the time process of fishfeeder
int feedcontrol_12_hrs = 9;// button for 12 hrs feeding
int feedcontrol_12_hrs_led = 32;
int feedcontrol_24_hrs = 10;// button for 24 hrs feeding
int feedcontrol_24_hrs_led = 30;
int feed_num = 1;
int feed_times = 0;
int feed_ctr_once = 0;

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
#define Offset 1.39               //deviation compensate
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

char replybuffer[255];

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  clock.begin();
  sensors.begin();

  fonaSerial->begin(1234);
  if (! fona.begin(*fonaSerial)) {
    lcd.print("Couldn't find GSM");
    while (1);
  }
   
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
  digitalWrite(feedcontrol_12_hrs_led, HIGH);
  
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

  pinMode(float_pin, INPUT);
  digitalWrite(float_pin, HIGH);
  pinMode(feed_pin, OUTPUT);

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
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
  myservo.attach(feed_pin);
  myservo.write(0);
  delay(1000);
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
  lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"...");
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
    digitalWrite(feedcontrol_led, HIGH);
    myservo.write(180);
    /*if(feed_ctr_once == 0){
      feed_ctr_once = 1;  
      feed_times = feed_num;
    }*/
  } else {
    /*if(feed_times != 0 && feed_ctr_once == 1){
      myservo.write(0);
      digitalWrite(feedcontrol_led, LOW);
      feed_times--;
      feed_count = 0;
    } else {
      feed_ctr_once = 0;
      feed_times = 0;
      feed_ctr = 0;
      feed_count = 0;
      feed_msg_ctr = 0;
      myservo.write(0);
      digitalWrite(feedcontrol_led, LOW);
    }*/
    feed_ctr = 0;
    feed_count = 0;
    feed_msg_ctr = 0;
    myservo.write(0);
    digitalWrite(feedcontrol_led, LOW);
  }
}

void pumpwater(){ //function for the water pump
  if(pump_count < pump_seconds){
    if(pump_msg_ctr == 1 && float_msg_ctr == 0 && feed_msg_ctr == 0 && display_priority == 0){
      lcd.setCursor(0,0);
      lcd.print("Pumping water...");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+format_minutes(String(dt.minute))+":"+format_seconds(String(dt.second))+"..");
    }
    //digitalWrite(pump1_pin, LOW);
    disable_float_ctr = 0;
    delay(500);
    digitalWrite(pump1_pin, LOW);
    digitalWrite(pumpcontrol_led, HIGH);
  } else {
    digitalWrite(pump1_pin, HIGH);
    delay(1000);
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
    //digitalWrite(pump1_pin, LOW);
    disable_pump_ctr = 0;
    delay(500);
    digitalWrite(pump1_pin, LOW);
    digitalWrite(pump2_pin, LOW);
  } else {
    digitalWrite(pump1_pin, HIGH);
    digitalWrite(pump2_pin, HIGH);
    delay(1000);
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
    lcd.print("*Temp: " + String(getTempe()) + "****");
    lcd.setCursor(0,1);
    lcd.print("*PH Level: " + String(getPh()) + "**");
  } else if(display_count < 10){
    lcd.setCursor(0,0);
    String feed_setting = "--Not-yet-set---";
    if(digitalRead(feedcontrol_12_hrs_led) == HIGH){
      feed_setting = "2 times feeding.";
    }else if(digitalRead(feedcontrol_24_hrs_led) == HIGH){
      feed_setting = "1 time feeding..";
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
      if(dt.hour < 9 || dt.hour > 15){
        feed_time = String("----"+feed_hour[0]) +":"+ String(feed_minute[0]) +":"+ String(feed_second[0]+"-----");
      } else {
        feed_time = String("----"+feed_hour[1]) +":"+ String(feed_minute[1]) +":"+ String(feed_second[1]+"-----");
      }  
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

void sendSMS(char num[], int len_num, char msg[], int len_msg){//need for testing char num[], int len_num, 
  char sendto[len_num+1], message[len_msg+1];
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

String read_SMS(){
  uint8_t smsn = 1;
  if (fona.getSMSSender(smsn, replybuffer, 250)) {
    uint16_t smslen;
    if (! fona.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
      Serial.println("Failed!");
    }
    if (fona.deleteSMS(smsn)) {
      lcd.clear();
      lcd.println("Text Received!!!");
      delay(1000);
    }
    return String(replybuffer);
  }
}

void errorled(){
  if(error_count < error_seconds){
    digitalWrite(error_led, HIGH);
  } else {
    digitalWrite(error_led, LOW);
    error_ctr = 0;
    error_count = 0;
  }
}

void start(){
    if(start_count < 1){
      lcd.print("Initializing.");
    } else if(start_count < 6){
      lcd.print(".");
    } else if(start_count < 7){
      lcd.print(".");
    } else if(start_count < 8){
      lcd.clear();
      lcd.print("Preparing GSM.");
    } else if(start_count < 12){
      lcd.print(".");
    } else if(start_count < 13){
      lcd.print(".");
    } else if(start_count < 14){
      lcd.clear();
      lcd.print("Starting in ");
    } else if(start_count < 15){
      lcd.setCursor(0,1);
      lcd.print("3");
    } else if(start_count < 16){
      lcd.clear();
      lcd.print("Starting in ");
      lcd.setCursor(0,1);
      lcd.print("2");
    } else if(start_count < 17){
      lcd.clear();
      lcd.print("Starting in ");
      lcd.setCursor(0,1);
      lcd.print("1");
    } else if(start_count < 18){
      lcd.clear();
      lcd.print(".");
    } else if(start_count < 19){
      lcd.print(".");
    } else if(start_count < 20){
      lcd.print(".");
    } else {
    start_ctr = 0;
    start_count = 0;
  }
}

void loop() {
  static unsigned long samplingTime = millis();
  static unsigned long samplingTime2 = millis();
  
  while (fona.available()) {
    Serial.write(fona.read());
    String x = String(fona.readString());
    String msg = x.substring(1,2) + x.substring(2,13);
    if(msg.equalsIgnoreCase("+CMTI: \"SM\",")){
      gsm_msg = read_SMS();
      if(!gsm_msg.equalsIgnoreCase("Feed") && !gsm_msg.equalsIgnoreCase("Pump") && !gsm_msg.equalsIgnoreCase("Once") && !gsm_msg.equalsIgnoreCase("Twice")){
        feed_num = gsm_msg.toInt();
        lcd.clear();
        lcd.print("Gsm_msg:");
        lcd.setCursor(0,1);
        lcd.print(gsm_msg);
        delay(1000);
        lcd.clear();
        lcd.print("Feed_num:");
        lcd.setCursor(0,1);
        lcd.print(feed_num);
        delay(1000);
      } else {
        lcd.clear();
        lcd.print("Gsm_msg:");
        lcd.setCursor(0,1);
        lcd.print(gsm_msg);
        delay(1000);
        gsm_cmd = gsm_msg;
        String msg_alert = "Received Cmd: " + gsm_msg;
        String snum = "09753724217";
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
    }
    msg = "";
  }
  
  dt = clock.getDateTime();
  if(start_ctr == 1){
    start();
    delay(1000);
  } else {
    if((dt.hour == gsm_hour[0] && dt.minute == gsm_minute[0] && dt.second == gsm_second[0]) || 
        (dt.hour == gsm_hour[1] && dt.minute == gsm_minute[1] && dt.second == gsm_second[1]) || 
        (dt.hour == gsm_hour[2] && dt.minute == gsm_minute[2] && dt.second == gsm_second[2])){
      String msg_alert = "'ph':'" + String(getPh()) + "', 'temp':'" + String(getTempe()) + "'";
      String snum = "21586723";
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

    if((hr_12_ctr == 1 && dt.hour == feed_hour[0] && dt.minute == feed_minute[0] && dt.second == feed_second[0]) ||
      (dt.hour == feed_hour[1] && dt.minute == feed_minute[1] && dt.second == feed_second[1])){
      feed_msg_ctr = 1;
      feed_ctr = 1;
      feedfish();
    }
    
    if(hr_24_ctr == 1 && dt.hour == feed_hour[1] && dt.minute == feed_minute[1] && dt.second == feed_second[1]){
      feed_msg_ctr = 1;
      feed_ctr = 1;
      feedfish();
    }
    
    if(digitalRead(feedcontrol_pin) == LOW || feed_ctr == 1 || gsm_cmd.equalsIgnoreCase("Feed")){ //will check if the button for fishfeeder is pressed
      gsm_cmd = "";
      feed_msg_ctr = 1;
      feed_ctr = 1;
      feedfish();
    }
  
    if((digitalRead(pumpcontrol_pin) == LOW || pump_ctr == 1) && disable_pump_ctr == 5 || gsm_cmd.equalsIgnoreCase("Pump")){ //will check if the button for water pump is pressed
      gsm_cmd = "";
      pump_msg_ctr = 1;
      pump_ctr = 1;
      pumpwater();
    }
  
    if(digitalRead(system_display_pin) == LOW || display_ctr == 1){
      display_priority = 1;
      display_ctr = 1;
      getSystemData();
    }
  
    if((digitalRead(float_pin) == LOW || float_ctr == 1) && disable_float_ctr == 5){//will check if float switch activated
      float_msg_ctr = 1;
      float_ctr = 1;
      floatswitch();
    }
  
    if(getPh() < 4.6 || error_ctr == 1){
      error_ctr = 1;
      errorled();
      pump_ctr = 1;
      pumpwater();
    }
    
    if(digitalRead(feedcontrol_12_hrs) == LOW || gsm_cmd.equalsIgnoreCase("Twice")){ 
      gsm_cmd = "";
      digitalWrite(feedcontrol_12_hrs_led, HIGH);
      digitalWrite(feedcontrol_24_hrs_led, LOW);
      hr_12_ctr = 1;
      hr_24_ctr = 0;
    }
  
    if(digitalRead(feedcontrol_24_hrs) == LOW || gsm_cmd.equalsIgnoreCase("Once")){
      gsm_cmd = "";
      digitalWrite(feedcontrol_24_hrs_led, HIGH);
      digitalWrite(feedcontrol_12_hrs_led, LOW);
      hr_12_ctr = 0;
      hr_24_ctr = 1;
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
    if(start_ctr == 1){
      if(start_count < start_seconds){
        start_count++;
      }
    }
    
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
    
    if(feed_ctr == 0 && pump_ctr == 0 && display_ctr == 0 && float_ctr == 0 && start_ctr == 0){
      displayDateTime();
    }

    if(disable_pump_ctr != 5){
      disable_pump_ctr++;
    }

    if(disable_float_ctr != 5){
      disable_float_ctr++;
    }
    samplingTime2=millis();
  }
}
