#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

//#include "mqttConnection.h"
//#include "commands.h"

#define ssid "EaseeTest"
#define passwd "TestingEasee"

#define I2C_SDA 21
#define I2C_SCL 22

//#define mqttServer "10.90.6.12"
//#define mainTopic "QA/500AKiller"
//#define mqttPort 1883
//#define clientName "500AkillingMachine"
//#define wifiLedPin 2
//#define mqttLedPin 3

Adafruit_ADS1115 ads;


TFT_eSPI tft = TFT_eSPI();

//void mqttCallback(char* callbackTopic, byte* payload, unsigned int payloadLength);
//Connection conn(ssid, passwd, mqttServer, mainTopic, mqttCallback, mqttPort, clientName, wifiLedPin, mqttLedPin);

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
      if (amps<100){
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      }else{
      tft.setTextColor(TFT_RED, TFT_BLACK);
      }

      tft.drawFloat(amps,0,60,30,8);   
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
   tft.setRotation(1); //Landscape

   startScreen();

    Wire.begin(I2C_SDA, I2C_SCL); // define I2C pins
    Wire.setClock(3400000);
    // standard mode:     100 000
    // fast mode:         400 000
    // fast mode plus   1 000 000
    // high speed mode  3 400 000
    // set higher SPS (ADC1115 supports up to 860sps )
    ads.setDataRate(RATE_ADS1115_860SPS);

  // conn.connect();
  // conn.debug("Hello!");

   ads.setGain(GAIN_SIXTEEN);

   ads.begin();
}

void loop() {
  // conn.maintain();
   int16_t results;

     results = ads.readADC_Differential_0_1();
     
     float volts;
     int n = 1500;
     float arr[n];
    
     for(int i =0; i < n; i++){
      volts = ads.computeVolts(results);
       arr[i] = volts;
     }
     
     float root = rmsValue(arr,n);

     float amps = ((root*1e6)*0.0014);

     printStatus(amps);
     delay(1000);
       

     
}