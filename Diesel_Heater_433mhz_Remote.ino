// Copyright (c) 2023, Shaun aka Chillichump
// chillichump.com
// youtube.com/chillichump
// All rights reserved.

// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. 

#include "DHT.h"
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();
char rfSend[4][25] = { {"011011101011110010110100"}, {"011011101011110010111000"}, {"011011101011110010110001"}, {"011011101011110010110010"}}; //On, Off, Up, Down
// make sure you use the correct binary codes for your remote above
#define send433 3 //433mhz sender pin
#define button 5 //button pin

#define DHTPIN 2 //temperature sensor
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//-- Frost Settings
#define frostL 4 //lower temperature in celsius
#define frostH 8 //higher temperature in celsius
#define frostD 5 //delay before switching on again in minutes
#define frostHO 0 //heat output of the diesel heater -- 0-Low  1-High

//-- Normal Heating Settings
#define heatL 14 //lower temperature in celsius
#define heatH 21 //higher temperature in celsius
#define heatD 15 //delay before switching on again in minutes
#define heatHO 1 //heat output of diesel heater -- 0-Low  1-High


bool heatMode;
bool heaterOn=0;

int tLow;
int tHigh;
int retryCount=0; //how many times to retry starting the heater (this is in case fuel has run out, or some other technical issue)
unsigned long tDelay=0;
bool tHO;
unsigned long lastOff=millis();
unsigned long lastOn=millis();
unsigned long lastRead=millis()+10000; //waiting 10 seconds until first Temperature reading
unsigned long justChecking=millis(); //variable to repeat on or off action, just in case the 433mhz signal didn't get through before
float t;

void setup() {
  Serial.begin(9600);
  pinMode(button,INPUT_PULLUP);
  heatMode = !digitalRead(button);
  //button is used to decide on which method is used -> Frost Protection (unpressed: 1); Or Heating (pressed: 0)
  dht.begin();

  mySwitch.enableTransmit(send433);
  mySwitch.setPulseLength(369);

}

void loop() {
  
  if (millis()-lastRead>4000){
    t = dht.readTemperature();
    lastRead=millis();
    Serial.print("Temperature: ");
    Serial.println(t);
  }

  if (heatMode!=digitalRead(button)){
    delay(300);
    heatMode = digitalRead(button);
    retryCount = 0; //reset retryCount when button pressed

    Serial.println("-----------------");
    Serial.print("Button Pressed: ");
    Serial.println(heatMode);
    if (heatMode==1){//Frost Protection
      tLow = frostL;
      tHigh = frostH;
      tDelay = frostD*60000; //milliseconds
      tHO = frostHO;
    }else{
      tLow = heatL;
      tHigh = heatH;
      tDelay = heatD*60000; //milliseconds
      tHO = heatHO;
    }
    Serial.print("Low Temp: ");
    Serial.println(tLow);
    Serial.print("High Temp: ");
    Serial.println(tHigh);
    Serial.print("Delay: ");
    Serial.println(tDelay);
    Serial.print("Heat Output: ");
    Serial.println(tHO);
    Serial.println("-----------------");
    Serial.println();
  }
  
  if (isnan(t)){
    Serial.println("No Valid Temperature Readings...");
  }else{
    if (t < tLow && !heaterOn && (lastOff<20000 || millis()-lastOff>tDelay)){  //turn on heater
      Serial.println("Turning on heater...");
      rfOn();
      lastOn = millis();
      heaterOn = 1;
    }

    if (t > tHigh && heaterOn){ //turn off heater
      Serial.println("Turning off heater...");
      rfOff();
      lastOff = millis();
      heaterOn = 0;
    }

    if (heaterOn && millis()-lastOn>180000 && t < tLow && retryCount < 4){ //checking that the heater is working
      rfOn();
      lastOn = millis();
      retryCount++;
    }else{
      retryCount=0;
    }
  }

  if (millis()-justChecking>180000 && t > tHigh){ //just checking that the heater is in fact off if temperature threshold reached...so as not to waste fuel!
     justChecking=millis();
     if (!heaterOn){
       rfOff();
     }
  }

}

void rfOn() {
  mySwitch.send(rfSend[0]);
  delay(300);
  mySwitch.send(rfSend[0]);
  delay(300);
  mySwitch.send(rfSend[0]);
  delay(300);
  mySwitch.send(rfSend[0]);
  if (tHO){
      rfHO();
  }else{
      rfLO();
  }
}

void rfOff() {
  mySwitch.send(rfSend[1]);
  delay(300);
  mySwitch.send(rfSend[1]);
  delay(300);
  mySwitch.send(rfSend[1]);
  delay(300);
  mySwitch.send(rfSend[1]);
}

void rfHO() {
  int i=0;
  while (i<15){
    mySwitch.send(rfSend[2]);
    delay(50);
    i++;
  }
}

void rfLO() {
  int i=0;
  while (i<15){
    mySwitch.send(rfSend[3]);
    delay(50);
    i++;
  }
}
