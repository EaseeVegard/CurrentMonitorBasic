#include <Arduino.h>
#include "mqttConnection.h"
#include <PubSubClient.h>
#include <WiFi.h>

Connection::Connection
(
    String wifi_ssid, 
    String wifi_passwd, 
    String mqttHost, 
    String mainTopic, 
    std::function<void(char*, uint8_t*, unsigned int)> callback,
    uint16_t mqttPort=1883, 
    String mqttClientName="MqttClient", 
    int wifiLedPin=4, 
    int mqttLedPin=5
    )
{
    _ssid = wifi_ssid;
    _passwd = wifi_passwd;
    _host = mqttHost;
    _port = mqttPort;
    _clientName = mqttClientName;
    _callbackTopic = mainTopic + "/command";
    _debugTopic = mainTopic + "/debug";
    _jsonTopic = mainTopic + "/json";
    _crTopic = mainTopic + "/commandResponse";
    _mqttClient.setServer(_host.c_str(), _port );
    _mqttClient.setClient(_wifiClient);
    _mqttClient.setCallback(callback);
    _wifiOk = false;
    _mqttOk = false;
    _wifiLed = wifiLedPin;
    _mqttLed = mqttLedPin;
    pinMode(_wifiLed, OUTPUT);
    pinMode(_mqttLed, OUTPUT);
}

DynamicJsonDocument _jsonDoc(uint16_t size) 
{
    DynamicJsonDocument doc(size);
    return(doc);
}

void WiFiStationWifiReady(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_READY");
}

void WiFiStationWifiScanDone(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_SCAN_DONE");
}

void WiFiStationStaStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_START");
}

void WiFiStationStaStop(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_START");
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_CONNECTED");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_GOT_IP");
}

void WiFiStationAuthmodeChange(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_GOT_IP");
}

void WiFiStationGotIp(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_GOT_IP");
}

void WiFiStationGotIp6(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_GOT_IP");
}

void WiFiStationLostIp(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_STA_LOST_IP");
}

void WiFiApStart(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_START");
}

void WiFiApStop(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_STOP");
}

void WiFiApStaConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_STACONNECTED");
}

void WiFiApStaDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_STADISCONNECTED");
}

void WiFiApStaIpasSigned(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED");
}

void WiFiApProbeEwqRecved(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED");
}

void WiFiApGotIp6(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_AP_GOT_IP6");
}

void WiFiFtmReport(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("ARDUINO_EVENT_WIFI_FTM_REPORT");
}



void Connection::sendCommandResponse(String uniqueId, int commandId, JsonArray args, bool wasAccepted, float returnMeasurement)
{
    // command response json example
    // {
    //         uniqueId: "1660893867362",
    //         commandId: 4,
    //         wasAccepted: true
    //         timestamp: 1660893867
    // }
    
    // if (uniqueId == "" || commandId in commands) 

    String crMessage;
    DynamicJsonDocument crJson(1024);
    crJson["uniqueId"] = uniqueId;
    crJson["commandId"] = commandId;
    crJson["wasAccepted"] = wasAccepted;
    crJson["timestamp"] = getTimestamp();
    crJson["measurement"] = "";
    if (returnMeasurement != __FLT_MIN__) {
        crJson["measurement"] = returnMeasurement;
    }

    size_t n_args = args.size();
    //debug("Argument array size: " + String(n_args));
    for ( int i = 0; i < n_args; i++) {
        String arg = args[i];
        debug("Argument " + String(i) + ": " + arg);
    }

    serializeJson(crJson, crMessage); 
    _mqttClient.publish(_crTopic.c_str(), crMessage.c_str());
    crJson.clear();

    // send to commandExecute function in main.cpp for handling of commands.
    //commandFunction(uniqueId, commandId, args, wasAccepted);
    
}

void Connection::setStatusLeds()
{
    if (_wifiOk) {
        digitalWrite(_wifiLed, HIGH);
        // debug("wifiLed ON");
    }
    else {
        digitalWrite(_wifiLed, LOW);
        // debug("wifiLed OFF");
    }

    if (_mqttOk) {
        digitalWrite(_mqttLed, HIGH);
        // debug("mqttLed ON");
    }
    else {
        digitalWrite(_mqttLed, LOW);
        // debug("mqttLed OFF");
    }
}

void Connection::printAllParams()
{
    Serial.print("SSID: ");
    Serial.println(_ssid);
    Serial.print("Passwd: ");
    Serial.println(_passwd);
    Serial.print("MQTT port ");
    Serial.println(_port);
    Serial.print("MQTT host: ");
    Serial.println(_host);
}

void Connection::sendStatus()
{
    debug("SSID: " + _ssid);
    debug("Passwd: " + _passwd);
    debug("MQTT host : " + _host + ":" + _port);
    debug("RSSI:" + String(WiFi.RSSI()));
    debug("IP: " + String(WiFi.localIP().toString()));

}


void Connection::connect()
{
    // initalization function for establishing wifi connection
    Serial.print("Connecting to ");
    Serial.println(_ssid);
    WiFi.mode(WIFI_STA);
    
    // setup Wifi events
    WiFi.onEvent(WiFiStationWifiReady, ARDUINO_EVENT_WIFI_READY);
    WiFi.onEvent(WiFiStationWifiScanDone, ARDUINO_EVENT_WIFI_SCAN_DONE);
    WiFi.onEvent(WiFiStationStaStart, ARDUINO_EVENT_WIFI_STA_START);
    WiFi.onEvent(WiFiStationStaStop, ARDUINO_EVENT_WIFI_STA_STOP);
    WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(WiFiStationAuthmodeChange, ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE);
    WiFi.onEvent(WiFiStationGotIp, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(WiFiStationGotIp6, ARDUINO_EVENT_WIFI_STA_GOT_IP6);
    WiFi.onEvent(WiFiStationLostIp, ARDUINO_EVENT_WIFI_STA_LOST_IP);
    WiFi.onEvent(WiFiApStart, ARDUINO_EVENT_WIFI_AP_START);
    WiFi.onEvent(WiFiApStop, ARDUINO_EVENT_WIFI_AP_STOP);
    WiFi.onEvent(WiFiApStaConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(WiFiApStaDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    WiFi.onEvent(WiFiApStaIpasSigned, ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);
    WiFi.onEvent(WiFiApProbeEwqRecved, ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED);
    WiFi.onEvent(WiFiApGotIp6, ARDUINO_EVENT_WIFI_AP_GOT_IP6);
    WiFi.onEvent(WiFiFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);


    WiFi.begin(_ssid.c_str(), _passwd.c_str());
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED) { 
        Serial.print(".");
        digitalWrite(_wifiLed, HIGH);
        delay(200);
        digitalWrite(_wifiLed, LOW);
        delay(800);
        tries++;
        if (tries > 10) {
            Serial.println("Connection Failed! Rebooting...");
            delay(5000);
            ESP.restart();
        }
        }
    Serial.println();
    _wifiOk = true;
    setStatusLeds();


    ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        })
        .onEnd([]() {
        Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

    ArduinoOTA.begin();

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    // set SSL/TLS certificate
    //wifiClient.setCACert(caCert);

    _mqttClient.connect(_clientName.c_str() );
    _mqttClient.subscribe(_callbackTopic.c_str() );
    debug("Connected to broker as " + _clientName);
}

void Connection::maintain()
{
    _mqttClient.loop();
    ArduinoOTA.handle();

    // check mqtt connection
    if ( ! _mqttClient.state() == 0 ) {
        _mqttOk = false;
        setStatusLeds();
        Serial.println(String("MQTT connection failed with code " + String(_mqttClient.state() )));
        Serial.println("Rebooting in 3 seconds...");
        delay(3);
        ESP.restart();
    }
    else
    {
        _mqttOk = true;
    }

    // check wifi connection
    if ( WiFi.status() != WL_CONNECTED ) {
        _wifiOk = false;
        setStatusLeds();
        Serial.println("Wifi not connected");
        Serial.println("Rebooting in 3 seconds...");
        delay(3);
        ESP.restart();
    }
    else
    {
        _mqttOk = true;
    }
    setStatusLeds();
}

PubSubClient Connection::get_mqttClient()
{
    return(_mqttClient);
}

int Connection::publish(String topic, String message)
{
    if (_mqttClient.publish(topic.c_str(), message.c_str()) ) {
        _mqttOk = true;
        setStatusLeds();
        digitalWrite(_mqttLed, !digitalRead(_mqttLed));
        delay(20);
        digitalWrite(_mqttLed, !digitalRead(_mqttLed));
        delay(20);
        return(0);
    }
    else {
        // something went wrong with mqtt publishing
        _mqttOk = false;
        setStatusLeds();
        return(1);
    }
    
}

void Connection::debug(String message)
{
    _mqttClient.publish( _debugTopic.c_str(), message.c_str() );
    Serial.print("DEBUG:");
    Serial.println(message);
}

void Connection::debug(int value)
{
    String message = String(value);
    _mqttClient.publish( _debugTopic.c_str(), message.c_str() );
    Serial.print("DEBUG:");
    Serial.println(message);
}

void Connection::debug(unsigned long value)
{
    String message = String(value);
    _mqttClient.publish(_debugTopic.c_str(), message.c_str());
    Serial.print("DEBUG:");
    Serial.println(message);
}

void Connection::debug(float value, int decimalPlaces = 2)
{
    String message = String(value, decimalPlaces);
    _mqttClient.publish(_debugTopic.c_str(), message.c_str() );
    Serial.print("DEBUG:");
    Serial.println(message);
}

unsigned long Connection::getTimestamp()
{
    NTPClient timeClient(_ntpUDP);
    timeClient.begin();
    timeClient.update();
    unsigned long timestamp = timeClient.getEpochTime();
    return(timestamp);
}

void Connection::updateJsonDoc()
{
    DynamicJsonDocument jsonDoc(1024);

    jsonDoc["ip"] = WiFi.localIP().toString();
    jsonDoc["mac"] = WiFi.macAddress();
    jsonDoc["uptime"] = millis();
    jsonDoc["freeHeap"] = ESP.getFreeHeap();
    jsonDoc["totalHeap"] = ESP.getHeapSize();
    jsonDoc["jsonMemoryUsage"] = jsonDoc.memoryUsage();
    jsonDoc["timestamp"] = getTimestamp();

    String json_s;
    serializeJson(jsonDoc, json_s);
    _mqttClient.publish(_jsonTopic.c_str(), json_s.c_str() );
    jsonDoc.clear();

}
