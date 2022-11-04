#ifndef MQTT_CONNECTION_H
#define MQTT_CONNECTION_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// all library code before endif

class Connection 
{
    public:
        Connection(
            String wifi_ssid, 
            String wifi_passwd, 
            String mqttHost, 
            String mainTopic, 
            std::function<void(char*, uint8_t*, unsigned int)> callback, 
            uint16_t mqttPort, 
            String mqttClientName, 
            int wifiLedPin, 
            int mqttLedPin
        );
      
        void connect();
        void maintain();
        PubSubClient get_mqttClient();
        void printAllParams();
        void sendStatus();
        void debug(String message);
        void debug(int message);
        void debug(unsigned long message);
        void debug(float message, int decimalPlaces);
        int publish(String topic, String message);
        void setStatusLeds();
        void updateJsonDoc();
        void sendCommandResponse(String uniqueId, int commandId, JsonArray args, bool wasAccepted, float returnMeasurement);
        unsigned long getTimestamp();

    private:
        void(*commandFunctionPointer)(String, int, JsonArray, bool);
        String _ssid;
        String _passwd;
        String _host;
        uint16_t _port;
        String _clientName;
        String _mainTopic;
        WiFiClient _wifiClient;
        PubSubClient _mqttClient;
        String _callbackTopic;
        String _debugTopic;
        String _jsonTopic;
        String _crTopic;
        bool _mqttOk;
        bool _wifiOk;
        int _wifiLed;
        int _mqttLed;
        DynamicJsonDocument _jsonDoc(uint16_t);
        WiFiUDP _ntpUDP;
        NTPClient _timeClient(WiFiUDP);
};

#endif