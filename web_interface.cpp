#include "web_interface.h"

WebInterface webInterface;

WebInterface::WebInterface() : server(80), ws("/ws") {
}

void WebInterface::begin() {
    // Setup WebSocket
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&ws);
    
    setupRoutes();
    server.begin();
    
    Serial.println("Web server started on port 80");
}

void WebInterface::setupRoutes() {
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
    
    // API endpoints
    server.on("/api/vitals", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", getVitalSignsJSON());
    });
    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", getSystemStatusJSON());
    });
    
    server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/logs/vitals.json")) {
            request->send(SPIFFS, "/logs/vitals.json", "application/json");
        } else {
            request->send(404, "text/plain", "Log file not found");
        }
    });
    
    // Settings endpoint
    server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // Handle settings update
        request->send(200, "text/plain", "Settings updated");
    });
    // Settings endpoint
    server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // Handle settings update
        request->send(200, "text/plain", "Settings updated");
    });
    
    // Handle 404
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}

void WebInterface::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebInterface::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {
            String command = doc["command"];
            if (command == "getVitals") {
                ws.textAll(getVitalSignsJSON());
            } else if (command == "getStatus") {
                ws.textAll(getSystemStatusJSON());
            }
        }
    }
}

void WebInterface::broadcastVitalSigns(float heartRate, float spO2, float battery, bool fingerDetected) {
    DynamicJsonDocument doc(512);
    doc["type"] = "vitals";
    doc["heartRate"] = heartRate;
    doc["spO2"] = spO2;
    doc["battery"] = battery;
    doc["fingerDetected"] = fingerDetected;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);
}

void WebInterface::sendAlert(String alertMessage) {
    DynamicJsonDocument doc(256);
    doc["type"] = "alert";
    doc["message"] = alertMessage;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);
}

String WebInterface::getVitalSignsJSON() {
    extern VitalSigns currentVitals;
    
    DynamicJsonDocument doc(512);
    doc["heartRate"] = currentVitals.heartRate;
    doc["spO2"] = currentVitals.spO2;
    doc["batteryLevel"] = currentVitals.batteryLevel;
    doc["isFingerDetected"] = currentVitals.isFingerDetected;
    doc["timestamp"] = currentVitals.timestamp;
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

String WebInterface::getSystemStatusJSON() {
    DynamicJsonDocument doc(512);
    doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis();
    doc["version"] = "1.0.0";
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}
