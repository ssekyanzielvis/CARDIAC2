#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

class WebInterface {
private:
    AsyncWebServer server;
    AsyncWebSocket ws;
    
public:
    WebInterface();
    void begin();
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void broadcastVitalSigns(float heartRate, float spO2, float battery, bool fingerDetected);
    void sendAlert(String alertMessage);
    
private:
    void setupRoutes();
    String getVitalSignsJSON();
    String getSystemStatusJSON();
};

extern WebInterface webInterface;

#endif
