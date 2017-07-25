#include <Wire.h>
#include <DS3231.h>

DS3231 clock;
RTCDateTime dt;

#include <OneWire.h>
#include <DallasTemperature.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(A4, A5, A3, A2, A1, A0);

int msg_ctr = 0;

int feed_day[] = {6, 18}; //(24-hour format 0-24)
int feed_minute[] = {0, 0}; //(0-59)
int feed_second[] = {0, 0}; //(0-59)

int pump_pin = 10; //pin number for relay that will activate the pump
int pumpcontrol_pin = 11; //pin number for the button of the pump
int pump_seconds = 10; //convert minutes to seconds
int pump_count = 0;
int pump_ctr = 0;

int pumpstopcontrol_pin = 12; //pin number for the emergency stop for the pump

int feed_pin = 8; //pin number for relay that will activate the pump
int feedcontrol_pin = 9; //pin number for the button of the fishfeeder
int feed_ctr = 0;
int feed_count = 0;
int feed_seconds = 5;//change for the time process of fishfeeder

int one_sec = 1000;//interval  for one_sec

int ph_pin = 7; //pin number for the ph sensor
int tempe_pin = 6; //button for temp
int temp_sensor = 5; //pin number for temperature sensor
float temperature = 0;

float pHValue,voltage;

OneWire oneWirePin(temp_sensor);
DallasTemperature sensors(&oneWirePin);

#define SensorPin A6            //pH meter Analog output to Arduino Analog Input 0
#define Offset -0.64            //deviation compensate
#define LED 13
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth 40    //times of collection
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex=0; 

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  clock.begin();
  sensors.begin();
  
  //Uncomment to set the time and date
  //clock.setDateTime(__DATE__, __TIME__);
  pinMode(tempe_pin, INPUT);
  pinMode(ph_pin, INPUT);
  pinMode(feedcontrol_pin, INPUT_PULLUP);
  pinMode(pumpcontrol_pin, INPUT_PULLUP);
  pinMode(pumpstopcontrol_pin, INPUT_PULLUP);
  
  pinMode(feed_pin, OUTPUT);
  pinMode(pump_pin, OUTPUT);
  
  digitalWrite(feed_pin, HIGH);
  digitalWrite(pump_pin, HIGH);
}

double avergearray(int* arr, int number){ //function for getting the average of ph values
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    Serial.println("Error number for the array to avraging!/n");
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

void displayDateTime(){ //function for displaying the time on the LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Date: ");
  lcd.print(String(dt.month)+"-"+String(dt.day)+"-"+String(dt.year));
  lcd.setCursor(0,1);
  lcd.print("Time: ");
  lcd.print(String(dt.hour)+":"+String(dt.minute)+":"+String(dt.second));
}

void feedfish(){ //function for the fishfeeder
  if(feed_count < feed_seconds){
    if(pump_ctr == 0){
      lcd.setCursor(0,0);
      lcd.print("Feeding Fishes..");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+String(dt.minute)+":"+String(dt.second));
    } else if(msg_ctr == 0) {
      msg_ctr = 1;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Feeding Fishes..");
      lcd.setCursor(0,1);
      lcd.print("Pumping water..");
    }
    digitalWrite(feed_pin, LOW);
  } else {
    digitalWrite(feed_pin, HIGH);
    feed_ctr = 0;
    feed_count = 0;
    msg_ctr = 0;
  }
}

void pumpwater(){ //function for the water pump
  if(pump_count < pump_seconds){
    if(feed_ctr == 0){
      lcd.setCursor(0,0);
      lcd.print("Pumping water..");
      lcd.setCursor(0,1);
      lcd.print("Time: ");
      lcd.print(String(dt.hour)+":"+String(dt.minute)+":"+String(dt.second));
    } else if(msg_ctr == 0){
      msg_ctr = 1;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Pumping water..");
      lcd.setCursor(0,1);
      lcd.print("Feeding Fishes..");
    }
    digitalWrite(pump_pin, LOW);
  } else {
    digitalWrite(pump_pin, HIGH);
    pump_ctr = 0;
    pump_count = 0;
    msg_ctr = 0;
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

void loop() {
  static unsigned long samplingTime = millis();
  static unsigned long samplingTime2 = millis();
  
  dt = clock.getDateTime();
  
  if((digitalRead(feedcontrol_pin) == LOW || feed_ctr == 1) || 
     (dt.hour == feed_day[0] && dt.minute == feed_minute[0] && dt.second == feed_second[0]) || 
     (dt.hour == feed_day[1] && dt.minute == feed_minute[1] && dt.second == feed_second[0])){ //will check if the button for fishfeeder is pressed or there is a specified alarm
        feed_ctr = 1;
        feedfish();
  }

  if(digitalRead(pumpcontrol_pin) == LOW || pump_ctr == 1){ //will check if the button for water pump is pressed
    pump_ctr = 1;
    pumpwater();
  }

  if(digitalRead(tempe_pin) == HIGH){ //will check if the button for getting the temperature is pressed
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temperature:");
    lcd.setCursor(0,1);
    lcd.print(String(getTempe()));
    msg_ctr = 0;
    delay(1000);
  }

  if(digitalRead(ph_pin) == HIGH){ //will check if the button for getting the ph level is pressed
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Ph Value:");
    lcd.setCursor(0,1);
    lcd.print(String(getPh()));
    msg_ctr = 0;
    delay(1000);
  }

  if(digitalRead(pumpstopcontrol_pin) == LOW){ //will check if the button for stopping the pump is pressed
    if(pump_count != 0){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("PUMPING STOPPED");
      digitalWrite(pump_pin, HIGH);
      delay(1000);
      pump_count = 0;
      pump_ctr = 0;
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
    if(feed_ctr == 0 && pump_ctr == 0){
      displayDateTime();   
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
    samplingTime2=millis();
  }
}
