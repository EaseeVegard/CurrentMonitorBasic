#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

#include "mqttConnection.h"

#define ssid "EaseeTest"
#define passwd "TestingEasee"

#define I2C_SDA 25
#define I2C_SCL 26

#define mqttServer "10.90.6.12"
#define mainTopic "QA/500AKiller"
#define mqttPort 1883
#define clientName "500AkillingMachine"
#define wifiLedPin 2
#define mqttLedPin 3
float lastamps = 0;
unsigned long timestart = 0;
bool current = false;
float maxAmps = 0;

Adafruit_ADS1115 ads;


TFT_eSPI tft = TFT_eSPI();

void mqttCallback(char* callbackTopic, byte* payload, unsigned int payloadLength){

}
Connection conn(ssid, passwd, mqttServer, mainTopic, mqttCallback, mqttPort, clientName, wifiLedPin, mqttLedPin);

void startScreen (){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(30, 50);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.print("easee 500A");
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(10, 90);
  tft.print("Killing machine");
  delay(5000);
  tft.fillScreen(TFT_BLACK);
    }

void printStatus(float amps){
  tft.fillScreen(TFT_BLACK); 
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
      if (amps<100){
      tft.drawFloat(amps,0,60,30,8); 
      }else{
      tft.drawFloat(amps,0,30,30,8); 
      }
      

        
}

// Function to calculate rms value
float rmsValue(float arr[], int n){
  float square = 0;
  float mean = 0.0;
  float root = 0.0;
  

  //calculate square.
  for (int i = 0; i < n; i++){
    square += pow(arr[i],2);
  }
  //calculate mean
  mean = (square/(float)(n));

  //calculate root.
  root = sqrt(mean);


  return root;
}

void setup() {
   Serial.begin(115200);
   tft.begin();
   tft.setRotation(3); //Landscape

   startScreen();

    Wire.begin(I2C_SDA, I2C_SCL); // define I2C pins
    Wire.setClock(3400000);
    // standard mode:     100 000
    // fast mode:         400 000
    // fast mode plus   1 000 000
    // high speed mode  3 400 000
    // set higher SPS (ADC1115 supports up to 860sps )
    ads.setDataRate(RATE_ADS1115_860SPS);

   conn.connect();
   conn.debug("Hello!");

   ads.setGain(GAIN_SIXTEEN);

   ads.begin();
}

void loop() {
  conn.maintain();
  int16_t results;
     unsigned long now = millis();
     float volts;
     int n = 86;
     float arr[n];
     
    
     for(int i =0; i < n; i++){
      results = ads.readADC_Differential_0_1();
      volts = ads.computeVolts(results);
       arr[i] = volts;
      
     }

     float root = rmsValue(arr,n);

     float amps = ((root*1e6)*0.01542);

     if(amps > maxAmps){
      maxAmps = amps;
     }

     String maxCurrent = mainTopic "/MaxCurent";
     String maxAmps_A = String(maxAmps);
     conn.publish(maxCurrent,maxAmps_A);

     if(amps>=10.0 && lastamps < 10.0){
      timestart = millis();
      conn.debug("Starting timer!");
      bool current = true;
      String currentflowing = mainTopic "/currentflowing_bool";
      String currentflowing_bool = String(current);
      conn.publish(currentflowing,currentflowing_bool);
     }else if(amps<=10.0 && lastamps > 10.0){
      unsigned long cycletime = millis() - timestart;
      String timetopic = mainTopic "/cycletime_s";
      String cycletime_S = String(cycletime/1000.0,2);
      conn.publish(timetopic,cycletime_S);
      conn.debug("Timer stopped!");
      current = false;
      String currentflowing = mainTopic "/currentflowing_bool";
      String currentflowing_bool = String(current);
      conn.publish(currentflowing,currentflowing_bool);
     }
     lastamps = amps;

     printStatus(amps);
     String amps_S = String(amps,1);
     String topic = mainTopic "/current_A";
     conn.publish(topic,amps_S);


          
}