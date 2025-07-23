/*
 * Cardiac Monitor ESP32 - Complete System
 * A comprehensive cardiac monitoring system with heart rate, SpO2 monitoring,
 * touch display interface, WiFi connectivity, and data logging.
 * 
 * Hardware Requirements:
 * - ESP32 Development Board
 * - MAX30102 Heart Rate/SpO2 Sensor
 * - ILI9341 3.2" TFT Display with Touch
 * - XPT2046 Touch Controller
 * - LiPo Battery with TP4056 Charger
 * 
 * WARNING: This is for educational purposes only. Not for medical use.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ==================== PIN DEFINITIONS ====================
// Display Pins
#define TFT_CS    5
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_RST   4
#define TFT_MISO  19

// Touch Pins
#define TOUCH_CS  15
#define TOUCH_IRQ 21

// Sensor Pins (I2C)
#define SDA_PIN   21
#define SCL_PIN   22

// Battery and Buzzer
#define BATTERY_PIN 36
#define BUZZER_PIN  25

// ==================== CONFIGURATION ====================
// System Configuration
const char* FIRMWARE_VERSION = "2.0.0";
const char* DEVICE_NAME = "CardiacMonitor";
const int SENSOR_UPDATE_INTERVAL = 100;  // ms
const int DISPLAY_UPDATE_INTERVAL = 100; // ms
const int DATA_LOG_INTERVAL = 1000;      // ms

// Sensor Configuration
const int SAMPLE_RATE = 100;
const int BUFFER_SIZE = 500;
const int FINGER_THRESHOLD = 50000;
const int SPO2_BUFFER_SIZE = 100;

// WiFi Configuration
const char* AP_SSID = "CardiacMonitor_Setup";
const char* AP_PASSWORD = "12345678";
const int CONNECTION_TIMEOUT = 10000;
const int MAX_RETRY_ATTEMPTS = 3;

// Alert Thresholds
struct AlertThresholds {
    float heartRateMin = 60.0f;
    float heartRateMax = 100.0f;
    float spO2Min = 95.0f;
    float batteryMin = 20.0f;
    bool enabled = true;
};

// ==================== DATA STRUCTURES ====================
struct VitalSigns {
    float heartRate = 0;
    float spO2 = 0;
    float batteryLevel = 0;
    bool isFingerDetected = false;
    unsigned long timestamp = 0;
};

enum class AlertLevel {
    INFO,
    WARNING,
    CRITICAL
};

struct Alert {
    AlertLevel level;
    String message;
    unsigned long timestamp;
    bool acknowledged;
};

enum class SystemState {
    INITIALIZING,
    RUNNING,
    SETTINGS,
    HISTORY,
    ERROR,
    SLEEP
};

enum class ScreenType {
    MAIN,
    SETTINGS,
    HISTORY,
    WIFI_CONFIG
};

// ==================== GLOBAL OBJECTS ====================
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
MAX30105 particleSensor;
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

// ==================== GLOBAL VARIABLES ====================
// System State
SystemState currentState = SystemState::INITIALIZING;
ScreenType currentScreen = ScreenType::MAIN;
VitalSigns currentVitals;
AlertThresholds alertThresholds;

// Timing Variables
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDataLog = 0;
unsigned long lastAlertCheck = 0;

// WiFi Variables
String wifiSSID = "";
String wifiPassword = "";
bool wifiConnected = false;
bool configModeActive = false;
int connectionAttempts = 0;

// Sensor Variables
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int32_t bufferLength = BUFFER_SIZE;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
int bufferIndex = 0;
bool fingerDetected = false;

// Display Variables
int screenBrightness = 128;
bool displayOn = true;
unsigned long lastTouchTime = 0;
const unsigned long SCREEN_TIMEOUT = 30000; // 30 seconds

// Alert Variables
std::vector<Alert> activeAlerts;
std::vector<Alert> alertHistory;
unsigned long lastAlertTime = 0;
const unsigned long ALERT_COOLDOWN = 5000; // 5 seconds

// Data Logging
std::vector<VitalSigns> dataBuffer;
const int DATA_BUFFER_SIZE = 100;

// Colors
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20
#define COLOR_PURPLE    0x780F
#define COLOR_GRAY      0x7BEF
#define COLOR_DARKGRAY  0x39E7

// ==================== SETUP FUNCTION ====================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Cardiac Monitor v2.0 ===");
    Serial.println("Initializing system...");
    
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Initialize preferences
    preferences.begin("cardiac", false);
    loadSettings();
    
    // Initialize display
    if (!initializeDisplay()) {
        Serial.println("FATAL: Display initialization failed");
        while (true) delay(1000);
    }
    
    showSplashScreen();
    delay(2000);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        showError("Storage Error", "Failed to initialize storage");
        delay(3000);
    }
    
    // Initialize sensor
    if (!initializeSensor()) {
        showError("Sensor Error", "Failed to initialize MAX30102");
        delay(5000);
        // Continue without sensor for testing
    }
    
    // Initialize WiFi
    initializeWiFi();
    
    // System ready
    currentState = SystemState::RUNNING;
    currentScreen = ScreenType::MAIN;
    showMainScreen();
    
    Serial.println("System initialization complete");
    Serial.println("=================================");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long currentTime = millis();
    
    // Handle WiFi and web server
    if (configModeActive) {
        dnsServer.processNextRequest();
        server.handleClient();
    } else {
        handleWiFiConnection();
    }
    
    // Handle touch input
    handleTouch();
    
    // Update sensors
    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        updateSensors();
    }
    
    // Update display
    if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = currentTime;
        updateDisplay();
    }
    
    // Log data
    if (currentTime - lastDataLog >= DATA_LOG_INTERVAL) {
        lastDataLog = currentTime;
        logData();
    }
    
    // Check alerts
    if (currentTime - lastAlertCheck >= 1000) { // Check every second
        lastAlertCheck = currentTime;
        checkAlerts();
    }
    
    // Handle screen timeout
    handleScreenTimeout();
    
    delay(10); // Prevent watchdog timeout
}

// ==================== DISPLAY FUNCTIONS ====================
bool initializeDisplay() {
    Serial.println("Initializing display...");
    
    tft.begin();
    tft.setRotation(1); // Landscape
    tft.fillScreen(COLOR_BLACK);
    
    if (!ts.begin()) {
        Serial.println("Touch screen initialization failed");
        return false;
    }
    
    ts.setRotation(1);
    
    Serial.println("Display initialized successfully");
    return true;
}

void showSplashScreen() {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(3);
    
    // Center the title
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds("Cardiac Monitor", 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((320 - w) / 2, 80);
    tft.println("Cardiac Monitor");
    
    tft.setTextSize(2);
    tft.getTextBounds("v2.0", 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((320 - w) / 2, 120);
    tft.println("v2.0");
    
    // Draw heart icon
    drawHeart(160, 160, COLOR_RED);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_GRAY);
    tft.setCursor(10, 220);
    tft.println("Educational use only - Not for medical diagnosis");
}

void showMainScreen() {
    currentScreen = ScreenType::MAIN;
    tft.fillScreen(COLOR_BLACK);
    
    // Draw header
    tft.fillRect(0, 0, 320, 30, COLOR_BLUE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 8);
    tft.println("Cardiac Monitor");
    
    // Draw status indicators
    drawStatusBar();
    
    // Draw vital signs areas
    drawVitalSignsLayout();
    
    // Draw buttons
    drawMainButtons();
}

void drawStatusBar() {
    // WiFi status
    tft.setTextSize(1);
    tft.setTextColor(wifiConnected ? COLOR_GREEN : COLOR_RED);
    tft.setCursor(250, 10);
    tft.println(wifiConnected ? "WiFi" : "No WiFi");
    
    // Battery level
    float batteryLevel = readBatteryLevel();
    uint16_t batteryColor = batteryLevel > 20 ? COLOR_GREEN : COLOR_RED;
    tft.setTextColor(batteryColor);
    tft.setCursor(280, 20);
    tft.print((int)batteryLevel);
    tft.println("%");
}

void drawVitalSignsLayout() {
    // Heart Rate Box
    tft.drawRect(10, 40, 140, 80, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(15, 45);
    tft.println("Heart Rate (BPM)");
    
    // SpO2 Box
    tft.drawRect(170, 40, 140, 80, COLOR_WHITE);
    tft.setCursor(175, 45);
    tft.println("SpO2 (%)");
    
    // Waveform area
    tft.drawRect(10, 130, 300, 60, COLOR_WHITE);
    tft.setCursor(15, 135);
    tft.println("Waveform");
}

void drawMainButtons() {
    // Settings button
    tft.fillRect(10, 200, 90, 30, COLOR_GRAY);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(35, 212);
    tft.println("Settings");
    
    // History button
    tft.fillRect(115, 200, 90, 30, COLOR_GRAY);
    tft.setCursor(145, 212);
    tft.println("History");
    
    // WiFi Config button
    tft.fillRect(220, 200, 90, 30, COLOR_GRAY);
    tft.setCursor(245, 212);
    tft.println("WiFi");
}

void updateVitalSigns() {
    // Update heart rate display
    tft.fillRect(15, 55, 130, 60, COLOR_BLACK);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(3);
    tft.setCursor(20, 70);
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        tft.print((int)currentVitals.heartRate);
    } else {
        tft.print("--");
    }
    
    // Update SpO2 display
    tft.fillRect(175, 55, 130, 60, COLOR_BLACK);
    tft.setTextColor(COLOR_BLUE);
    tft.setCursor(180, 70);
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        tft.print((int)currentVitals.spO2);
    } else {
        tft.print("--");
    }
    
    // Update finger detection status
    tft.setTextSize(1);
    tft.setTextColor(currentVitals.isFingerDetected ? COLOR_GREEN : COLOR_RED);
    tft.setCursor(15, 105);
    tft.fillRect(15, 105, 100, 10, COLOR_BLACK);
    tft.println(currentVitals.isFingerDetected ? "Finger detected" : "Place finger");
}

void drawWaveform() {
    static int waveformX = 15;
    static int lastY = 160;
    
    // Clear old waveform point
    tft.drawPixel(waveformX, lastY, COLOR_BLACK);
    
    // Draw new point if finger detected
    if (currentVitals.isFingerDetected) {
        // Simulate waveform based on heart rate
        int waveY = 160 + sin(millis() * 0.01) * 20;
        tft.drawPixel(waveformX, waveY, COLOR_GREEN);
        lastY = waveY;
    }
    
   
    waveformX++;
    if (waveformX > 305) {
        waveformX = 15;
        // Clear waveform area
        tft.fillRect(15, 140, 290, 45, COLOR_BLACK);
    }
}

void showError(const String& title, const String& message) {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(2);
    
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((320 - w) / 2, 80);
    tft.println(title);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10, 120);
    tft.println(message);
}

void showSettingsScreen() {
    currentScreen = ScreenType::SETTINGS;
    tft.fillScreen(COLOR_BLACK);
    
    // Header
    tft.fillRect(0, 0, 320, 30, COLOR_BLUE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 8);
    tft.println("Settings");
    
    // Back button
    tft.fillRect(250, 5, 60, 20, COLOR_GRAY);
    tft.setTextSize(1);
    tft.setCursor(270, 10);
    tft.println("Back");
    
    // Settings options
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    
    // Alert thresholds
    tft.setCursor(10, 50);
    tft.println("Alert Thresholds:");
    
    tft.setCursor(20, 70);
    tft.print("Heart Rate: ");
    tft.print((int)alertThresholds.heartRateMin);
    tft.print(" - ");
    tft.print((int)alertThresholds.heartRateMax);
    tft.println(" BPM");
    
    tft.setCursor(20, 90);
    tft.print("SpO2 Min: ");
    tft.print((int)alertThresholds.spO2Min);
    tft.println("%");
    
    tft.setCursor(20, 110);
    tft.print("Battery Min: ");
    tft.print((int)alertThresholds.batteryMin);
    tft.println("%");
    
    // Brightness setting
    tft.setCursor(10, 140);
    tft.print("Brightness: ");
    tft.print(screenBrightness);
    
    // Data export button
    tft.fillRect(10, 170, 100, 30, COLOR_GREEN);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(35, 182);
    tft.println("Export Data");
    
    // Clear data button
    tft.fillRect(120, 170, 100, 30, COLOR_RED);
    tft.setCursor(145, 182);
    tft.println("Clear Data");
}

void showHistoryScreen() {
    currentScreen = ScreenType::HISTORY;
    tft.fillScreen(COLOR_BLACK);
    
    // Header
    tft.fillRect(0, 0, 320, 30, COLOR_BLUE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 8);
    tft.println("Data History");
    
    // Back button
    tft.fillRect(250, 5, 60, 20, COLOR_GRAY);
    tft.setTextSize(1);
    tft.setCursor(270, 10);
    tft.println("Back");
    
    // Show recent data
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(10, 40);
    tft.println("Recent Readings:");
    
    int y = 60;
    int count = min(8, (int)dataBuffer.size());
    
    for (int i = dataBuffer.size() - count; i < dataBuffer.size() && y < 200; i++) {
        if (i >= 0) {
            VitalSigns& data = dataBuffer[i];
            tft.setCursor(10, y);
            tft.print(formatTime(data.timestamp));
            tft.print(" HR:");
            tft.print((int)data.heartRate);
            tft.print(" SpO2:");
            tft.print((int)data.spO2);
            tft.print(" Bat:");
            tft.print((int)data.batteryLevel);
            tft.println("%");
            y += 15;
        }
    }
    
    if (dataBuffer.empty()) {
        tft.setCursor(10, 60);
        tft.println("No data available");
    }
}

void drawHeart(int x, int y, uint16_t color) {
    // Simple heart shape using filled circles and triangle
    tft.fillCircle(x - 8, y - 5, 8, color);
    tft.fillCircle(x + 8, y - 5, 8, color);
    tft.fillTriangle(x - 15, y, x + 15, y, x, y + 15, color);
}

// ==================== SENSOR FUNCTIONS ====================
bool initializeSensor() {
    Serial.println("Initializing MAX30102 sensor...");
    
    Wire.begin(SDA_PIN, SCL_PIN);
    
    if (!particleSensor.begin()) {
        Serial.println("MAX30102 not found");
        return false;
    }
    
    // Configure sensor
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    
    Serial.println("MAX30102 initialized successfully");
    return true;
}

void updateSensors() {
    // Read battery level
    currentVitals.batteryLevel = readBatteryLevel();
    currentVitals.timestamp = millis();
    
    // Read sensor data
    if (particleSensor.available()) {
        // Read and store data in buffers
        redBuffer[bufferIndex] = particleSensor.getRed();
        irBuffer[bufferIndex] = particleSensor.getIR();
        
        // Check finger detection
        fingerDetected = (irBuffer[bufferIndex] > FINGER_THRESHOLD);
        currentVitals.isFingerDetected = fingerDetected;
        
        bufferIndex++;
        
        // Process data when buffer is full
        if (bufferIndex >= BUFFER_SIZE) {
            bufferIndex = 0;
            
            if (fingerDetected) {
                // Calculate heart rate and SpO2
                maxim_heart_rate_and_oxygen_saturation(
                    irBuffer, bufferLength, redBuffer,
                    &spo2, &validSPO2, &heartRate, &validHeartRate
                );
                
                if (validHeartRate && heartRate > 0 && heartRate < 200) {
                    currentVitals.heartRate = heartRate;
                }
                
                if (validSPO2 && spo2 > 0 && spo2 <= 100) {
                    currentVitals.spO2 = spo2;
                }
            } else {
                currentVitals.heartRate = 0;
                currentVitals.spO2 = 0;
            }
        }
        
        particleSensor.nextSample();
    }
}

float readBatteryLevel() {
    int rawValue = analogRead(BATTERY_PIN);
    float voltage = (rawValue / 4095.0) * 3.3 * 2; // Voltage divider
    
    // Convert to percentage (3.0V = 0%, 4.2V = 100%)
    float percentage = ((voltage - 3.0) / 1.2) * 100.0;
    return constrain(percentage, 0, 100);
}

// ==================== TOUCH HANDLING ====================
void handleTouch() {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        
        // Map touch coordinates to screen coordinates
        int x = map(p.x, 200, 3700, 0, 320);
        int y = map(p.y, 240, 3800, 0, 240);
        
        lastTouchTime = millis();
        
        // Wake up display if sleeping
        if (!displayOn) {
            displayOn = true;
            // Re-enable display
            return;
        }
        
        handleTouchEvent(x, y);
        
        // Debounce
        delay(200);
    }
}

void handleTouchEvent(int x, int y) {
    switch (currentScreen) {
        case ScreenType::MAIN:
            handleMainScreenTouch(x, y);
            break;
        case ScreenType::SETTINGS:
            handleSettingsScreenTouch(x, y);
            break;
        case ScreenType::HISTORY:
            handleHistoryScreenTouch(x, y);
            break;
        case ScreenType::WIFI_CONFIG:
            // WiFi config handled by web interface
            break;
    }
}

void handleMainScreenTouch(int x, int y) {
    // Settings button (10, 200, 90, 30)
    if (x >= 10 && x <= 100 && y >= 200 && y <= 230) {
        showSettingsScreen();
    }
    // History button (115, 200, 90, 30)
    else if (x >= 115 && x <= 205 && y >= 200 && y <= 230) {
        showHistoryScreen();
    }
    // WiFi button (220, 200, 90, 30)
    else if (x >= 220 && x <= 310 && y >= 200 && y <= 230) {
        startConfigMode();
    }
}

void handleSettingsScreenTouch(int x, int y) {
    // Back button (250, 5, 60, 20)
    if (x >= 250 && x <= 310 && y >= 5 && y <= 25) {
        showMainScreen();
    }
    // Export data button (10, 170, 100, 30)
    else if (x >= 10 && x <= 110 && y >= 170 && y <= 200) {
        exportData();
    }
    // Clear data button (120, 170, 100, 30)
    else if (x >= 120 && x <= 220 && y >= 170 && y <= 200) {
        clearData();
    }
}

void handleHistoryScreenTouch(int x, int y) {
    // Back button (250, 5, 60, 20)
    if (x >= 250 && x <= 310 && y >= 5 && y <= 25) {
        showMainScreen();
    }
}

void handleScreenTimeout() {
    if (displayOn && (millis() - lastTouchTime > SCREEN_TIMEOUT)) {
        displayOn = false;
        // Could implement display sleep here
    }
}

// ==================== WIFI FUNCTIONS ====================
void initializeWiFi() {
    Serial.println("Initializing WiFi...");
    
    // Load WiFi credentials
    wifiSSID = preferences.getString("wifi_ssid", "");
    wifiPassword = preferences.getString("wifi_pass", "");
    
    if (wifiSSID.length() > 0) {
        connectToWiFi();
    } else {
        Serial.println("No WiFi credentials found");
    }
}

void connectToWiFi() {
    if (wifiSSID.length() == 0) return;
    
    Serial.printf("Connecting to WiFi: %s\n", wifiSSID.c_str());
    
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < CONNECTION_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
        setupWebServer();
    } else {
        wifiConnected = false;
        Serial.println("\nWiFi connection failed");
        connectionAttempts++;
        
        if (connectionAttempts >= MAX_RETRY_ATTEMPTS) {
            Serial.println("Max retry attempts reached, starting config mode");
            startConfigMode();
        }
    }
}

void handleWiFiConnection() {
    if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("WiFi reconnected");
        setupWebServer();
    } else if (wifiConnected && WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        Serial.println("WiFi disconnected");
    }
}

void startConfigMode() {
    Serial.println("Starting WiFi configuration mode...");
    
    configModeActive = true;
    
    // Start access point
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    // Start DNS server
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Setup web server for configuration
    setupConfigServer();
    
    Serial.printf("Config mode active. Connect to '%s' and go to http://192.168.4.1\n", AP_SSID);
}

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/data", handleDataRequest);
    server.on("/export", handleExportRequest);
    server.begin();
    Serial.println("Web server started");
}

void setupConfigServer() {
    server.on("/", handleConfigRoot);
    server.on("/save", handleConfigSave);
    server.on("/scan", handleWiFiScan);
    server.begin();
    Serial.println("Config server started");
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>Cardiac Monitor</title></head><body>";
    html += "<h1>Cardiac Monitor Status</h1>";
    html += "<p>Heart Rate: " + String((int)currentVitals.heartRate) + " BPM</p>";
    html += "<p>SpO2: " + String((int)currentVitals.spO2) + "%</p>";
    html += "<p>Battery: " + String((int)currentVitals.batteryLevel) + "%</p>";
    html += "<p>Finger Detected: " + String(currentVitals.isFingerDetected ? "Yes" : "No") + "</p>";
    html += "<p><a href='/data'>View Data</a> | <a href='/export'>Export Data</a></p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleConfigRoot() {
    String html = "<!DOCTYPE html><html><head><title>WiFi Configuration</title>";
    html += "<style>body{font-family:Arial;margin:40px;} .btn{background:#007cba;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;} .btn:hover{background:#005a87;}</style>";
    html += "</head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form action='/save' method='post'>";
    html += "<p><label>Network Name (SSID):</label><br><input type='text' name='ssid' style='width:300px;padding:5px;'></p>";
    html += "<p><label>Password:</label><br><input type='password' name='password' style='width:300px;padding:5px;'></p>";
    html += "<p><input type='submit' value='Save Configuration' class='btn'></p>";
    html += "</form>";
    html += "<p><a href='/scan' class='btn'>Scan Networks</a></p>";
    html += "<p><strong>Current Status:</strong> " + String(wifiConnected ? "Connected" : "Not Connected") + "</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleConfigSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    if (ssid.length() > 0) {
        preferences.putString("wifi_ssid", ssid);
        preferences.putString("wifi_pass", password);
        
        wifiSSID = ssid;
        wifiPassword = password;
        
        String html = "<!DOCTYPE html><html><head><title>Configuration Saved</title></head><body>";
        html += "<h1>Configuration Saved</h1>";
        html += "<p>WiFi credentials have been saved. The device will restart and attempt to connect.</p>";
        html += "</body></html>";
        
        server.send(200, "text/html", html);
        
        delay(2000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "SSID cannot be empty");
    }
}

void handleWiFiScan() {
    String html = "<!DOCTYPE html><html><head><title>Available Networks</title></head><body>";
    html += "<h1>Available WiFi Networks</h1>";
    
    int n = WiFi.scanNetworks();
    if (n == 0) {
        html += "<p>No networks found</p>";
    } else {
        html += "<ul>";
        for (int i = 0; i < n; ++i) {
            html += "<li>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
            html += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [Open]" : " [Secured]";
            html += "</li>";
        }
        html += "</ul>";
    }
    
    html += "<p><a href='/'>Back to Configuration</a></p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleDataRequest() {
    DynamicJsonDocument doc(2048);
    JsonObject current = doc.createNestedObject("current");
    
    current["heartRate"] = currentVitals.heartRate;
    current["spO2"] = currentVitals.spO2;
    current["batteryLevel"] = currentVitals.batteryLevel;
    current["fingerDetected"] = currentVitals.isFingerDetected;
    current["timestamp"] = currentVitals.timestamp;
    
    JsonArray history = doc.createNestedArray("history");
    for (const auto& data : dataBuffer) {
        JsonObject entry = history.createNestedObject();
        entry["heartRate"] = data.heartRate;
        entry["spO2"] = data.spO2;
        entry["batteryLevel"] = data.batteryLevel;
        entry["timestamp"] = data.timestamp;
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleExportRequest() {
    String csv = "Timestamp,HeartRate,SpO2,BatteryLevel\n";
    
    for (const auto& data : dataBuffer) {
        csv += String(data.timestamp) + ",";
        csv += String(data.heartRate) + ",";
        csv += String(data.spO2) + ",";
        csv += String(data.batteryLevel) + "\n";
    }
    
    server.send(200, "text/csv", csv);
}

// ==================== DATA LOGGING ====================
void logData() {
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        dataBuffer.push_back(currentVitals);
        
        // Limit buffer size
        if (dataBuffer.size() > DATA_BUFFER_SIZE) {
            dataBuffer.erase(dataBuffer.begin());
        }
        
        // Save to SPIFFS periodically
        static int saveCounter = 0;
        saveCounter++;
        if (saveCounter >= 10) { // Save every 10 readings
            saveCounter = 0;
            saveDataToFile();
        }
    }
}

void saveDataToFile() {
    File file = SPIFFS.open("/data.csv", "w");
    if (file) {
        file.println("Timestamp,HeartRate,SpO2,BatteryLevel");
        for (const auto& data : dataBuffer) {
            file.print(data.timestamp);
            file.print(",");
            file.print(data.heartRate);
            file.print(",");
            file.print(data.spO2);
            file.print(",");
            file.println(data.batteryLevel);
        }
        file.close();
        Serial.println("Data saved to file");
    } else {
        Serial.println("Failed to save data to file");
    }
}

void loadDataFromFile() {
    File file = SPIFFS.open("/data.csv", "r");
    if (file) {
        dataBuffer.clear();
        String line = file.readStringUntil('\n'); // Skip header
        
        while (file.available()) {
            line = file.readStringUntil('\n');
            if (line.length() > 0) {
                VitalSigns data;
                int commaIndex = 0;
                int lastIndex = 0;
                
                // Parse CSV line
                commaIndex = line.indexOf(',', lastIndex);
                data.timestamp = line.substring(lastIndex, commaIndex).toInt();
                lastIndex = commaIndex + 1;
                
                commaIndex = line.indexOf(',', lastIndex);
                data.heartRate = line.substring(lastIndex, commaIndex).toFloat();
                lastIndex = commaIndex + 1;
                
                commaIndex = line.indexOf(',', lastIndex);
                data.spO2 = line.substring(lastIndex, commaIndex).toFloat();
                lastIndex = commaIndex + 1;
                
                data.batteryLevel = line.substring(lastIndex).toFloat();
                
                dataBuffer.push_back(data);
            }
        }
        file.close();
        Serial.printf("Loaded %d data entries from file\n", dataBuffer.size());
    }
}

void exportData() {
    Serial.println("Exporting data...");
    saveDataToFile();
    
    // Show confirmation on display
    tft.fillRect(50, 100, 220, 60, COLOR_GREEN);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(80, 120);
    tft.println("Data Exported!");
    
    delay(2000);
    showSettingsScreen();
}

void clearData() {
    Serial.println("Clearing data...");
    dataBuffer.clear();
    SPIFFS.remove("/data.csv");
    
    // Show confirmation on display
    tft.fillRect(50, 100, 220, 60, COLOR_RED);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    tft.setCursor(90, 120);
    tft.println("Data Cleared!");
    
    delay(2000);
    showSettingsScreen();
}

// ==================== ALERT SYSTEM ====================
void checkAlerts() {
    if (!alertThresholds.enabled) return;
    
    // Check heart rate
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        if (currentVitals.heartRate < alertThresholds.heartRateMin || 
            currentVitals.heartRate > alertThresholds.heartRateMax) {
            
            String message = "Heart rate: " + String((int)currentVitals.heartRate) + " BPM";
            AlertLevel level = (currentVitals.heartRate < 50 || currentVitals.heartRate > 120) ? 
                              AlertLevel::CRITICAL : AlertLevel::WARNING;
            triggerAlert(level, message);
        }
    }
    
    // Check SpO2
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        if (currentVitals.spO2 < alertThresholds.spO2Min) {
            String message = "Low SpO2: " + String((int)currentVitals.spO2) + "%";
            AlertLevel level = (currentVitals.spO2 < 90) ? AlertLevel::CRITICAL : AlertLevel::WARNING;
            triggerAlert(level, message);
        }
    }
    
    // Check battery
    if (currentVitals.batteryLevel < alertThresholds.batteryMin) {
        String message = "Low battery: " + String((int)currentVitals.batteryLevel) + "%";
        AlertLevel level = (currentVitals.batteryLevel < 10) ? AlertLevel::CRITICAL : AlertLevel::WARNING;
        triggerAlert(level, message);
    }
    
    // Remove old alerts
    removeOldAlerts();
}

void triggerAlert(AlertLevel level, const String& message) {
    // Check cooldown
    if (millis() - lastAlertTime < 5000) return; // 5 second cooldown
    
    Alert alert;
    alert.level = level;
    alert.message = message;
    alert.timestamp = millis();
    alert.acknowledged = false;
    
    activeAlerts.push_back(alert);
    alertHistory.push_back(alert);
    
    // Limit history size
    if (alertHistory.size() > 50) {
        alertHistory.erase(alertHistory.begin());
    }
    
    lastAlertTime = millis();
    
    // Play alert sound
    playAlertSound(level);
    
    // Show alert on display
    showAlert(message, level);
    
    Serial.printf("ALERT [%s]: %s\n", 
        level == AlertLevel::CRITICAL ? "CRITICAL" : 
        level == AlertLevel::WARNING ? "WARNING" : "INFO", 
        message.c_str());
}

void playAlertSound(AlertLevel level) {
    int beepCount = 1;
    int beepDuration = 200;
    
    switch (level) {
        case AlertLevel::CRITICAL:
            beepCount = 3;
            beepDuration = 500;
            break;
        case AlertLevel::WARNING:
            beepCount = 2;
            beepDuration = 300;
            break;
        default:
            beepCount = 1;
            beepDuration = 200;
            break;
    }
    
    for (int i = 0; i < beepCount; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(beepDuration);
        digitalWrite(BUZZER_PIN, LOW);
        if (i < beepCount - 1) delay(200);
    }
}

void showAlert(const String& message, AlertLevel level) {
    uint16_t alertColor = COLOR_YELLOW;
    switch (level) {
        case AlertLevel::CRITICAL:
            alertColor = COLOR_RED;
            break;
        case AlertLevel::WARNING:
            alertColor = COLOR_ORANGE;
            break;
        default:
            alertColor = COLOR_YELLOW;
            break;
    }
    
    // Show alert banner
    tft.fillRect(0, 30, 320, 25, alertColor);
    tft.setTextColor(COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 38);
    tft.println(message);
    
    // Auto-hide after 5 seconds
    static unsigned long alertDisplayTime = millis();
    if (millis() - alertDisplayTime > 5000) {
        tft.fillRect(0, 30, 320, 25, COLOR_BLACK);
        alertDisplayTime = millis();
    }
}

void removeOldAlerts() {
    unsigned long currentTime = millis();
    
    activeAlerts.erase(
        std::remove_if(activeAlerts.begin(), activeAlerts.end(),
            [currentTime](const Alert& alert) {
                return alert.acknowledged || 
                       (currentTime - alert.timestamp > 30000); // 30 seconds
            }),
        activeAlerts.end()
    );
}

// ==================== SETTINGS MANAGEMENT ====================
void loadSettings() {
    alertThresholds.heartRateMin = preferences.getFloat("hr_min", 60.0f);
    alertThresholds.heartRateMax = preferences.getFloat("hr_max", 100.0f);
    alertThresholds.spO2Min = preferences.getFloat("spo2_min", 95.0f);
    alertThresholds.batteryMin = preferences.getFloat("bat_min", 20.0f);
    alertThresholds.enabled = preferences.getBool("alerts_en", true);
    screenBrightness = preferences.getInt("brightness", 128);
    
    Serial.println("Settings loaded from preferences");
}

void saveSettings() {
    preferences.putFloat("hr_min", alertThresholds.heartRateMin);
    preferences.putFloat("hr_max", alertThresholds.heartRateMax);
    preferences.putFloat("spo2_min", alertThresholds.spO2Min);
    preferences.putFloat("bat_min", alertThresholds.batteryMin);
    preferences.putBool("alerts_en", alertThresholds.enabled);
    preferences.putInt("brightness", screenBrightness);
    
    Serial.println("Settings saved to preferences");
}

// ==================== UTILITY FUNCTIONS ====================
void updateDisplay() {
    if (!displayOn) return;
    
    switch (currentScreen) {
        case ScreenType::MAIN:
            updateVitalSigns();
            drawWaveform();
            drawStatusBar();
            break;
        case ScreenType::SETTINGS:
            // Settings screen is static, no updates needed
            break;
        case ScreenType::HISTORY:
            // History screen is static, no updates needed
            break;
        case ScreenType::WIFI_CONFIG:
            // WiFi config handled by web interface
            break;
    }
}


String formatTime(unsigned long timestamp) {
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours) + ":";
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes) + ":";
    if (seconds < 10) timeStr += "0";
    timeStr += String(seconds);
    
    return timeStr;
}

void printSystemInfo() {
    Serial.println("\n=== System Information ===");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("Device Name: %s\n", DEVICE_NAME);
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    
    if (wifiConnected) {
        Serial.printf("WiFi SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("WiFi: Not connected");
    }
    
    Serial.printf("Sensor Status: %s\n", particleSensor.begin() ? "Connected" : "Disconnected");
    Serial.printf("Display Status: Active\n");
    Serial.printf("Touch Status: %s\n", ts.begin() ? "Active" : "Inactive");
    Serial.printf("Data Buffer: %d/%d entries\n", dataBuffer.size(), DATA_BUFFER_SIZE);
    Serial.printf("Active Alerts: %d\n", activeAlerts.size());
    Serial.println("========================\n");
}

void performSelfTest() {
    Serial.println("Performing system self-test...");
    
    bool testsPassed = true;
    
    // Test display
    Serial.print("Testing display... ");
    tft.fillScreen(COLOR_RED);
    delay(500);
    tft.fillScreen(COLOR_GREEN);
    delay(500);
    tft.fillScreen(COLOR_BLUE);
    delay(500);
    tft.fillScreen(COLOR_BLACK);
    Serial.println("OK");
    
    // Test touch
    Serial.print("Testing touch controller... ");
    if (ts.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
        testsPassed = false;
    }
    
    // Test sensor
    Serial.print("Testing MAX30102 sensor... ");
    if (particleSensor.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
        testsPassed = false;
    }
    
    // Test SPIFFS
    Serial.print("Testing file system... ");
    if (SPIFFS.begin(true)) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
        testsPassed = false;
    }
    
    // Test buzzer
    Serial.print("Testing buzzer... ");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("OK");
    
    // Test battery reading
    Serial.print("Testing battery monitor... ");
    float batteryLevel = readBatteryLevel();
    if (batteryLevel >= 0 && batteryLevel <= 100) {
        Serial.printf("OK (%.1f%%)\n", batteryLevel);
    } else {
        Serial.println("WARNING - Unusual reading");
    }
    
    Serial.printf("Self-test %s\n", testsPassed ? "PASSED" : "FAILED");
    
    if (testsPassed) {
        // Success indication
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            delay(100);
        }
    } else {
        // Failure indication
        for (int i = 0; i < 5; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        command.toLowerCase();
        
        if (command == "help") {
            Serial.println("\n=== Available Commands ===");
            Serial.println("help - Show this help message");
            Serial.println("info - Show system information");
            Serial.println("test - Perform self-test");
            Serial.println("reset - Reset system");
            Serial.println("wifi - Show WiFi status");
            Serial.println("data - Show current readings");
            Serial.println("export - Export data to serial");
            Serial.println("clear - Clear data buffer");
            Serial.println("alerts - Show active alerts");
            Serial.println("config - Enter configuration mode");
            Serial.println("========================\n");
        }
        else if (command == "info") {
            printSystemInfo();
        }
        else if (command == "test") {
            performSelfTest();
        }
        else if (command == "reset") {
            Serial.println("Resetting system...");
            ESP.restart();
        }
        else if (command == "wifi") {
            Serial.printf("WiFi Status: %s\n", wifiConnected ? "Connected" : "Disconnected");
            if (wifiConnected) {
                Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
                Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
                Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
            }
        }
        else if (command == "data") {
            Serial.printf("Heart Rate: %.1f BPM\n", currentVitals.heartRate);
            Serial.printf("SpO2: %.1f%%\n", currentVitals.spO2);
            Serial.printf("Battery: %.1f%%\n", currentVitals.batteryLevel);
            Serial.printf("Finger Detected: %s\n", currentVitals.isFingerDetected ? "Yes" : "No");
        }
        else if (command == "export") {
            Serial.println("Timestamp,HeartRate,SpO2,BatteryLevel");
            for (const auto& data : dataBuffer) {
                Serial.printf("%lu,%.1f,%.1f,%.1f\n", 
                    data.timestamp, data.heartRate, data.spO2, data.batteryLevel);
            }
        }
        else if (command == "clear") {
            dataBuffer.clear();
            Serial.println("Data buffer cleared");
        }
        else if (command == "alerts") {
            Serial.printf("Active Alerts: %d\n", activeAlerts.size());
            for (const auto& alert : activeAlerts) {
                Serial.printf("- %s: %s\n", 
                    alert.level == AlertLevel::CRITICAL ? "CRITICAL" :
                    alert.level == AlertLevel::WARNING ? "WARNING" : "INFO",
                    alert.message.c_str());
            }
        }
        else if (command == "config") {
            startConfigMode();
            Serial.println("Configuration mode started");
        }
        else {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
}

void watchdogFeed() {
    // Feed the watchdog timer to prevent system reset
    // This is automatically handled by the ESP32 framework
    // but we can add explicit feeding if needed
    yield();
}

void handleLowPowerMode() {
    if (currentVitals.batteryLevel < 10 && !wifiConnected) {
        // Enter low power mode
        Serial.println("Entering low power mode...");
        
        // Reduce display brightness
        // Note: Actual brightness control would require PWM on backlight pin
        screenBrightness = 50;
        
        // Reduce sensor sampling rate
        // This would require modifying the sensor update interval
        
        // Disable WiFi if not connected
        if (!wifiConnected) {
            WiFi.mode(WIFI_OFF);
        }
        
        // Show low power indicator
        tft.fillRect(0, 0, 320, 20, COLOR_RED);
        tft.setTextColor(COLOR_WHITE);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("LOW POWER MODE - Connect charger");
    }
}

void checkMemoryUsage() {
    static unsigned long lastMemCheck = 0;
    
    if (millis() - lastMemCheck > 30000) { // Check every 30 seconds
        lastMemCheck = millis();
        
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 10000) { // Less than 10KB free
            Serial.printf("WARNING: Low memory - %d bytes free\n", freeHeap);
            
            // Clean up old data if memory is low
            if (dataBuffer.size() > 50) {
                dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + 25);
                Serial.println("Cleaned up data buffer to free memory");
            }
            
            // Clean up old alerts
            if (alertHistory.size() > 25) {
                alertHistory.erase(alertHistory.begin(), alertHistory.begin() + 10);
                Serial.println("Cleaned up alert history to free memory");
            }
        }
    }
}

// ==================== ENHANCED MAIN LOOP ====================
void loop() {
    unsigned long currentTime = millis();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Feed watchdog
    watchdogFeed();
    
    // Check memory usage
    checkMemoryUsage();
    
    // Handle low power mode
    handleLowPowerMode();
    
    // Handle WiFi and web server
    if (configModeActive) {
        dnsServer.processNextRequest();
        server.handleClient();
    } else {
        handleWiFiConnection();
        if (wifiConnected) {
            server.handleClient();
        }
    }
    
    // Handle touch input
    handleTouch();
    
    // Update sensors
    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        updateSensors();
    }
    
    // Update display
    if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = currentTime;
        updateDisplay();
    }
    
    // Log data
    if (currentTime - lastDataLog >= DATA_LOG_INTERVAL) {
        lastDataLog = currentTime;
        logData();
    }
    
    // Check alerts
    if (currentTime - lastAlertCheck >= 1000) { // Check every second
        lastAlertCheck = currentTime;
        checkAlerts();
    }
    
    // Handle screen timeout
    handleScreenTimeout();
    
    // Periodic system maintenance
    static unsigned long lastMaintenance = 0;
    if (currentTime - lastMaintenance > 60000) { // Every minute
        lastMaintenance = currentTime;
        
        // Save settings periodically
        saveSettings();
        
        // Print status update
        if (Serial.available() == 0) { // Only if no serial input pending
            Serial.printf("Status: HR=%.1f SpO2=%.1f Bat=%.1f%% Mem=%dKB\n",
                currentVitals.heartRate, currentVitals.spO2, 
                currentVitals.batteryLevel, ESP.getFreeHeap()/1024);
        }
    }
    
    delay(10); // Prevent watchdog timeout and allow other tasks
}

// ==================== INITIALIZATION HELPERS ====================
void initializeSystem() {
    // This function can be called to reinitialize the system
    Serial.println("Reinitializing system...");
    
    // Reset state
    currentState = SystemState::INITIALIZING;
    currentScreen = ScreenType::MAIN;
    
    // Clear buffers
    bufferIndex = 0;
    memset(irBuffer, 0, sizeof(irBuffer));
    memset(redBuffer, 0, sizeof(redBuffer));
    
    // Reset timing
    lastSensorUpdate = 0;
    lastDisplayUpdate = 0;
    lastDataLog = 0;
    lastAlertCheck = 0;
    
    // Load data from file
    loadDataFromFile();
    
    // System ready
    currentState = SystemState::RUNNING;
    showMainScreen();
    
    Serial.println("System reinitialization complete");
}

// ==================== ERROR HANDLING ====================
void handleSystemError(const String& errorMessage) {
    Serial.printf("SYSTEM ERROR: %s\n", errorMessage.c_str());
    
    currentState = SystemState::ERROR;
    
    // Show error on display
    showError("System Error", errorMessage);
    
    // Sound error alert
    for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
    
    // Try to recover after 5 seconds
    delay(5000);
    
    // Attempt recovery
    Serial.println("Attempting system recovery...");
    initializeSystem();
}

// ==================== END OF CODE ====================

/*
 * USAGE INSTRUCTIONS:
 * 
 * 1. Hardware Setup:
 *    - Connect MAX30102 sensor to I2C pins (SDA=21, SCL=22)
 *    - Connect ILI9341 display to SPI pins as defined above
 *    - Connect XPT2046 touch controller
 *    - Connect battery monitor to pin 36
 *    - Connect buzzer to pin 25
 * 
 * 2. First Boot:
 *    - Device will create WiFi hotspot "CardiacMonitor_Setup"
 *    - Connect with password "12345678"
 *    - Navigate to http://192.168.4.1 to configure WiFi
 * 
 * 3. Operation:
 *    - Place finger on MAX30102 sensor
 *    - View real-time heart rate and SpO2 on display
 *    - Use touch interface to navigate menus
 *    - Access web interface for remote monitoring
 * 
 * 4. Serial Commands:
 *    - Connect at 115200 baud
 *    - Type "help" for available commands
 * 
 * 5. Data Export:
 *    - Use web interface or serial commands
 *    - Data saved in CSV format
 * 
 * SAFETY WARNING:
 * This device is for educational purposes only.
 * Do not use for medical diagnosis or treatment.
 * Always consult healthcare professionals for medical advice.
 */
