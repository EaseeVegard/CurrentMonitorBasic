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
#define button 13
#define pulse 15

#define mqttServer "10.90.6.12"
#define mainTopic "QA/500AKiller"
#define mqttPort 1883
#define clientName "500AkillingMachine"
#define wifiLedPin 2
#define mqttLedPin 3

float lastamps = 0;
unsigned long timestart = 0;
unsigned long lasttime = 0;
bool current = false;
float amps = 0;
float maxAmps = 0;
unsigned long relayintervall = 1;
unsigned long samplesToRead = 1;
//unsigned long waitTime = 180000;
float waitTime_s = 180.0;
int repetitions = 3;

Adafruit_ADS1115 ads;


TFT_eSPI tft = TFT_eSPI();

int findDelimiter(String text, char delimiter) {
  int i;
  for (i = 0; i < text.length(); i++) {
    if(text[i] == delimiter) {
      return(i);
    }
  }
  return(-1);
}

void mqttCallback(char* callbackTopic, byte* payload, unsigned int payloadLength);

Connection conn(ssid, passwd, mqttServer, mainTopic, mqttCallback, mqttPort, clientName, wifiLedPin, mqttLedPin);

void mqttCallback(char* callbackTopic, byte* payload, unsigned int payloadLength){
  String message = "";
  for(int i = 0; i < payloadLength; i++) {
    message += (char) payload[i];
  }
  message.trim();
  int argPos = findDelimiter(message, ':');
  String command = message.substring(0, argPos);
  String arg = message.substring(argPos+1);
  if(command == "samplesToRead"){
    samplesToRead = arg.toInt();
    conn.debug("Samples to read set to: "+ String(samplesToRead));
  }else if(command == "waitTime_s"){
    waitTime_s = arg.toInt();
    conn.debug("waitTime set to: "+ String(waitTime_s)+"s");
  }else if(command == "repetitions"){
    repetitions = arg.toInt();
    conn.debug("repetitions set to: "+ String(repetitions));
  }else{
    conn.debug("command not found! command="+command+" arg="+arg);
  }
}

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

void waitScreen (float maxAmps, int testNumber){
  for (int i = waitTime_s; i>0.0; i--){
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(2,30);
    tft.print("maxAmps: ");
    tft.drawFloat(maxAmps,0,190,20,4);
    tft.setCursor(2,70);
    tft.print("Next test: ");
    tft.drawFloat((i),0,190,50,4);
    tft.setCursor(2,90);
    tft.print("Test number: ");
    tft.drawFloat(testNumber,0,190,90,4);
    String waitTimeTopic = mainTopic "/waitTime_s";
    String waitTimeValue = String(i)+("s");
    conn.publish(waitTimeTopic,waitTimeValue);
    delay(1000);
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

float readAmps(int n){
  int16_t results;
  float volts;
  float arr[n];
  
  for(int i =0; i < n; i++){
    results = ads.readADC_Differential_0_1();
    volts = ads.computeVolts(results);
    arr[i] = volts;
    }
     
  float root = rmsValue(arr,n);

  float amps = ((root*1e6)*0.01542);

  return amps;
}

void setup() {
   Serial.begin(115200);
   tft.begin();
   tft.setRotation(3); //Landscape

   pinMode(button,INPUT);
   pinMode(pulse,OUTPUT);

   digitalWrite (pulse, LOW);

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
  unsigned long now = millis();
  int n = 86;
  amps = readAmps(n);

  if(amps > maxAmps){
  maxAmps = amps;      
  }

  if (digitalRead(button) == LOW){
    for (int i=0; i<repetitions; i++){
      maxAmps = 0;
      lasttime = millis();
      maxAmps = 0;
      int testNumber = i+1;
      digitalWrite (pulse, HIGH);
      while(millis() < (lasttime + relayintervall)){
        amps = readAmps(samplesToRead);
        if(amps > maxAmps){
          maxAmps = amps;
        }
      }
      digitalWrite(pulse,LOW);
      unsigned long actualPulseLength = millis() - lasttime;
      conn.debug("pulse completed with: " + String(actualPulseLength) + "ms");
      conn.publish(mainTopic "/actualPulseLength_ms",String(actualPulseLength));
      String maxCurrent = mainTopic "/maxCurrent_A";
      String maxAmps_A = String(maxAmps);
      conn.publish(maxCurrent,maxAmps_A);
      waitScreen(maxAmps,i);
    }
  }
  else {
    if(amps>=10.0 && lastamps < 10.0){
      timestart = millis();
      conn.debug("Starting timer!");
      maxAmps = 0;
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
  }
  lastamps = amps;

  printStatus(amps);
  String amps_S = String(amps,1);
  String topic = mainTopic "/current_A";
  conn.publish(topic,amps_S);
  if(maxAmps>5){
    String maxCurrent = mainTopic "/maxCurrent_A";
    String maxAmps_A = String(maxAmps);
    conn.publish(maxCurrent,maxAmps_A);
  }
}