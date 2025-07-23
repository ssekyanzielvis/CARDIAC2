/*
 * Cardiac Monitor ESP32 - Complete System (C Version)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ==================== PIN DEFINITIONS ====================
#define TFT_CS    5
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_RST   4
#define TFT_MISO  19

#define TOUCH_CS  15
#define TOUCH_IRQ 21

#define SDA_PIN   21
#define SCL_PIN   22

#define BATTERY_PIN 36
#define BUZZER_PIN  25

// ==================== CONFIGURATION ====================
#define FIRMWARE_VERSION "2.0.0"
#define DEVICE_NAME "CardiacMonitor"
#define SENSOR_UPDATE_INTERVAL 100  // ms
#define DISPLAY_UPDATE_INTERVAL 100 // ms
#define DATA_LOG_INTERVAL 1000      // ms

#define SAMPLE_RATE 100
#define BUFFER_SIZE 500
#define FINGER_THRESHOLD 50000
#define SPO2_BUFFER_SIZE 100

#define AP_SSID "CardiacMonitor_Setup"
#define AP_PASSWORD "12345678"
#define CONNECTION_TIMEOUT 10000
#define MAX_RETRY_ATTEMPTS 3

#define MAX_ALERTS 10
#define MAX_HISTORY 50
#define DATA_BUFFER_SIZE 100
#define MAX_STRING 128

// ==================== DATA STRUCTURES ====================
typedef struct {
    float heartRateMin;
    float heartRateMax;
    float spO2Min;
    float batteryMin;
    bool enabled;
} AlertThresholds;

typedef struct {
    float heartRate;
    float spO2;
    float batteryLevel;
    bool isFingerDetected;
    unsigned long timestamp;
} VitalSigns;

#define ALERT_LEVEL_INFO 0
#define ALERT_LEVEL_WARNING 1
#define ALERT_LEVEL_CRITICAL 2

typedef struct {
    int level;
    char message[MAX_STRING];
    unsigned long timestamp;
    bool acknowledged;
} Alert;

#define SYSTEM_STATE_INITIALIZING 0
#define SYSTEM_STATE_RUNNING 1
#define SYSTEM_STATE_SETTINGS 2
#define SYSTEM_STATE_HISTORY 3
#define SYSTEM_STATE_ERROR 4
#define SYSTEM_STATE_SLEEP 5

#define SCREEN_TYPE_MAIN 0
#define SCREEN_TYPE_SETTINGS 1
#define SCREEN_TYPE_HISTORY 2
#define SCREEN_TYPE_WIFI_CONFIG 3

// ==================== GLOBAL OBJECTS ====================
Adafruit_ILI9341 tft = {TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO};
XPT2046_Touchscreen ts = {TOUCH_CS, TOUCH_IRQ};
MAX30105 particleSensor;
WebServer server = {80};
DNSServer dnsServer;

// ==================== GLOBAL VARIABLES ====================
int currentState = SYSTEM_STATE_INITIALIZING;
int currentScreen = SCREEN_TYPE_MAIN;
VitalSigns currentVitals;
AlertThresholds alertThresholds;

unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDataLog = 0;
unsigned long lastAlertCheck = 0;

char wifiSSID[MAX_STRING] = "";
char wifiPassword[MAX_STRING] = "";
bool wifiConnected = false;
bool configModeActive = false;
int connectionAttempts = 0;

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int32_t bufferLength = BUFFER_SIZE;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
int bufferIndex = 0;
bool fingerDetected = false;

int screenBrightness = 128;
bool displayOn = true;
unsigned long lastTouchTime = 0;
const unsigned long SCREEN_TIMEOUT = 30000;

Alert activeAlerts[MAX_ALERTS];
int activeAlertsCount = 0;
Alert alertHistory[MAX_HISTORY];
int alertHistoryCount = 0;
unsigned long lastAlertTime = 0;
const unsigned long ALERT_COOLDOWN = 5000;

VitalSigns dataBuffer[DATA_BUFFER_SIZE];
int dataBufferCount = 0;

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

// Simulated preferences for C
typedef struct {
    float hr_min;
    float hr_max;
    float spo2_min;
    float bat_min;
    bool alerts_en;
    int brightness;
    char wifi_ssid[MAX_STRING];
    char wifi_pass[MAX_STRING];
} Preferences;

Preferences preferences;

// ==================== FUNCTION PROTOTYPES ====================
void setup(void);
void loop(void);
bool initializeDisplay(void);
void showSplashScreen(void);
void showMainScreen(void);
void drawStatusBar(void);
void drawVitalSignsLayout(void);
void drawMainButtons(void);
void updateVitalSigns(void);
void drawWaveform(void);
void showError(const char* title, const char* message);
void showSettingsScreen(void);
void showHistoryScreen(void);
void drawHeart(int x, int y, uint16_t color);
bool initializeSensor(void);
void updateSensors(void);
float readBatteryLevel(void);
void handleTouch(void);
void handleTouchEvent(int x, int y);
void handleMainScreenTouch(int x, int y);
void handleSettingsScreenTouch(int x, int y);
void handleHistoryScreenTouch(int x, int y);
void handleScreenTimeout(void);
void initializeWiFi(void);
void connectToWiFi(void);
void handleWiFiConnection(void);
void startConfigMode(void);
void setupWebServer(void);
void setupConfigServer(void);
void handleRoot(void);
void handleConfigRoot(void);
void handleConfigSave(void);
void handleWiFiScan(void);
void handleDataRequest(void);
void handleExportRequest(void);
void logData(void);
void saveDataToFile(void);
void loadDataFromFile(void);
void exportData(void);
void clearData(void);
void checkAlerts(void);
void triggerAlert(int level, const char* message);
void playAlertSound(int level);
void showAlert(const char* message, int level);
void removeOldAlerts(void);
void loadSettings(void);
void saveSettings(void);
void updateDisplay(void);
void formatTime(unsigned long timestamp, char* output);
void printSystemInfo(void);
void performSelfTest(void);
void handleSerialCommands(void);
void watchdogFeed(void);
void handleLowPowerMode(void);
void checkMemoryUsage(void);
void initializeSystem(void);
void handleSystemError(const char* errorMessage);

// Simulated preferences functions
void preferences_begin(const char* name, bool readOnly);
float preferences_getFloat(const char* key, float defaultValue);
bool preferences_getBool(const char* key, bool defaultValue);
int preferences_getInt(const char* key, int defaultValue);
void preferences_getString(const char* key, char* output, const char* defaultValue);
void preferences_putFloat(const char* key, float value);
void preferences_putBool(const char* key, bool value);
void preferences_putInt(const char* key, int value);
void preferences_putString(const char* key, const char* value);

// ==================== SETUP FUNCTION ====================
void setup() {
    Serial_begin(115200);
    printf("\n=== Cardiac Monitor v2.0 ===\n");
    printf("Initializing system...\n");
    
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    preferences_begin("cardiac", false);
    loadSettings();
    
    if (!initializeDisplay()) {
        printf("FATAL: Display initialization failed\n");
        while (true) delay(1000);
    }
    
    showSplashScreen();
    delay(2000);
    
    if (!SPIFFS_begin(true)) {
        printf("SPIFFS initialization failed\n");
        showError("Storage Error", "Failed to initialize storage");
        delay(3000);
    }
    
    if (!initializeSensor()) {
        showError("Sensor Error", "Failed to initialize MAX30102");
        delay(5000);
    }
    
    initializeWiFi();
    
    currentState = SYSTEM_STATE_RUNNING;
    currentScreen = SCREEN_TYPE_MAIN;
    showMainScreen();
    
    printf("System initialization complete\n");
    printf("=================================\n");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long currentTime = millis();
    
    if (configModeActive) {
        DNSServer_processNextRequest(&dnsServer);
        WebServer_handleClient(&server);
    } else {
        handleWiFiConnection();
    }
    
    handleTouch();
    
    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        updateSensors();
    }
    
    if (currentTime - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = currentTime;
        updateDisplay();
    }
    
    if (currentTime - lastDataLog >= DATA_LOG_INTERVAL) {
        lastDataLog = currentTime;
        logData();
    }
    
    if (currentTime - lastAlertCheck >= 1000) {
        lastAlertCheck = currentTime;
        checkAlerts();
    }
    
    handleScreenTimeout();
    
    handleSerialCommands();
    watchdogFeed();
    checkMemoryUsage();
    handleLowPowerMode();
    
    delay(10);
}

// ==================== DISPLAY FUNCTIONS ====================
bool initializeDisplay() {
    printf("Initializing display...\n");
    
    Adafruit_ILI9341_begin(&tft);
    Adafruit_ILI9341_setRotation(&tft, 1);
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    
    if (!XPT2046_Touchscreen_begin(&ts)) {
        printf("Touch screen initialization failed\n");
        return false;
    }
    
    XPT2046_Touchscreen_setRotation(&ts, 1);
    
    printf("Display initialized successfully\n");
    return true;
}

void showSplashScreen() {
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 3);
    
    int16_t x1, y1;
    uint16_t w, h;
    Adafruit_ILI9341_getTextBounds(&tft, "Cardiac Monitor", 0, 0, &x1, &y1, &w, &h);
    Adafruit_ILI9341_setCursor(&tft, (320 - w) / 2, 80);
    Adafruit_ILI9341_println(&tft, "Cardiac Monitor");
    
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_getTextBounds(&tft, "v2.0", 0, 0, &x1, &y1, &w, &h);
    Adafruit_ILI9341_setCursor(&tft, (320 - w) / 2, 120);
    Adafruit_ILI9341_println(&tft, "v2.0");
    
    drawHeart(160, 160, COLOR_RED);
    
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_GRAY);
    Adafruit_ILI9341_setCursor(&tft, 10, 220);
    Adafruit_ILI9341_println(&tft, "Educational use only - Not for medical diagnosis");
}

void showMainScreen() {
    currentScreen = SCREEN_TYPE_MAIN;
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    
    Adafruit_ILI9341_fillRect(&tft, 0, 0, 320, 30, COLOR_BLUE);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_setCursor(&tft, 10, 8);
    Adafruit_ILI9341_println(&tft, "Cardiac Monitor");
    
    drawStatusBar();
    drawVitalSignsLayout();
    drawMainButtons();
}

void drawStatusBar() {
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setTextColor(&tft, wifiConnected ? COLOR_GREEN : COLOR_RED);
    Adafruit_ILI9341_setCursor(&tft, 250, 10);
    Adafruit_ILI9341_println(&tft, wifiConnected ? "WiFi" : "No WiFi");
    
    float batteryLevel = readBatteryLevel();
    uint16_t batteryColor = batteryLevel > 20 ? COLOR_GREEN : COLOR_RED;
    Adafruit_ILI9341_setTextColor(&tft, batteryColor);
    Adafruit_ILI9341_setCursor(&tft, 280, 20);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (int)batteryLevel);
    Adafruit_ILI9341_println(&tft, buf);
}

void drawVitalSignsLayout() {
    Adafruit_ILI9341_drawRect(&tft, 10, 40, 140, 80, COLOR_WHITE);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 15, 45);
    Adafruit_ILI9341_println(&tft, "Heart Rate (BPM)");
    
    Adafruit_ILI9341_drawRect(&tft, 170, 40, 140, 80, COLOR_WHITE);
    Adafruit_ILI9341_setCursor(&tft, 175, 45);
    Adafruit_ILI9341_println(&tft, "SpO2 (%)");
    
    Adafruit_ILI9341_drawRect(&tft, 10, 130, 300, 60, COLOR_WHITE);
    Adafruit_ILI9341_setCursor(&tft, 15, 135);
    Adafruit_ILI9341_println(&tft, "Waveform");
}

void drawMainButtons() {
    Adafruit_ILI9341_fillRect(&tft, 10, 200, 90, 30, COLOR_GRAY);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 35, 212);
    Adafruit_ILI9341_println(&tft, "Settings");
    
    Adafruit_ILI9341_fillRect(&tft, 115, 200, 90, 30, COLOR_GRAY);
    Adafruit_ILI9341_setCursor(&tft, 145, 212);
    Adafruit_ILI9341_println(&tft, "History");
    
    Adafruit_ILI9341_fillRect(&tft, 220, 200, 90, 30, COLOR_GRAY);
    Adafruit_ILI9341_setCursor(&tft, 245, 212);
    Adafruit_ILI9341_println(&tft, "WiFi");
}

void updateVitalSigns() {
    Adafruit_ILI9341_fillRect(&tft, 15, 55, 130, 60, COLOR_BLACK);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_RED);
    Adafruit_ILI9341_setTextSize(&tft, 3);
    Adafruit_ILI9341_setCursor(&tft, 20, 70);
    char buf[16];
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        snprintf(buf, sizeof(buf), "%d", (int)currentVitals.heartRate);
    } else {
        strcpy(buf, "--");
    }
    Adafruit_ILI9341_print(&tft, buf);
    
    Adafruit_ILI9341_fillRect(&tft, 175, 55, 130, 60, COLOR_BLACK);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_BLUE);
    Adafruit_ILI9341_setCursor(&tft, 180, 70);
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        snprintf(buf, sizeof(buf), "%d", (int)currentVitals.spO2);
    } else {
        strcpy(buf, "--");
    }
    Adafruit_ILI9341_print(&tft, buf);
    
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setTextColor(&tft, currentVitals.isFingerDetected ? COLOR_GREEN : COLOR_RED);
    Adafruit_ILI9341_setCursor(&tft, 15, 105);
    Adafruit_ILI9341_fillRect(&tft, 15, 105, 100, 10, COLOR_BLACK);
    Adafruit_ILI9341_println(&tft, currentVitals.isFingerDetected ? "Finger detected" : "Place finger");
}

void drawWaveform() {
    static int waveformX = 15;
    static int lastY = 160;
    
    Adafruit_ILI9341_drawPixel(&tft, waveformX, lastY, COLOR_BLACK);
    
    if (currentVitals.isFingerDetected) {
        int waveY = 160 + (int)(sin(millis() * 0.01) * 20);
        Adafruit_ILI9341_drawPixel(&tft, waveformX, waveY, COLOR_GREEN);
        lastY = waveY;
    }
    
    waveformX++;
    if (waveformX > 305) {
        waveformX = 15;
        Adafruit_ILI9341_fillRect(&tft, 15, 140, 290, 45, COLOR_BLACK);
    }
}

void showError(const char* title, const char* message) {
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_RED);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    
    int16_t x1, y1;
    uint16_t w, h;
    Adafruit_ILI9341_getTextBounds(&tft, title, 0, 0, &x1, &y1, &w, &h);
    Adafruit_ILI9341_setCursor(&tft, (320 - w) / 2, 80);
    Adafruit_ILI9341_println(&tft, title);
    
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setCursor(&tft, 10, 120);
    Adafruit_ILI9341_println(&tft, message);
}

void showSettingsScreen() {
    currentScreen = SCREEN_TYPE_SETTINGS;
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    
    Adafruit_ILI9341_fillRect(&tft, 0, 0, 320, 30, COLOR_BLUE);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_setCursor(&tft, 10, 8);
    Adafruit_ILI9341_println(&tft, "Settings");
    
    Adafruit_ILI9341_fillRect(&tft, 250, 5, 60, 20, COLOR_GRAY);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 270, 10);
    Adafruit_ILI9341_println(&tft, "Back");
    
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    
    Adafruit_ILI9341_setCursor(&tft, 10, 50);
    Adafruit_ILI9341_println(&tft, "Alert Thresholds:");
    
    char buf[32];
    Adafruit_ILI9341_setCursor(&tft, 20, 70);
    snprintf(buf, sizeof(buf), "Heart Rate: %d - %d BPM", 
             (int)alertThresholds.heartRateMin, (int)alertThresholds.heartRateMax);
    Adafruit_ILI9341_println(&tft, buf);
    
    Adafruit_ILI9341_setCursor(&tft, 20, 90);
    snprintf(buf, sizeof(buf), "SpO2 Min: %d%%", (int)alertThresholds.spO2Min);
    Adafruit_ILI9341_println(&tft, buf);
    
    Adafruit_ILI9341_setCursor(&tft, 20, 110);
    snprintf(buf, sizeof(buf), "Battery Min: %d%%", (int)alertThresholds.batteryMin);
    Adafruit_ILI9341_println(&tft, buf);
    
    Adafruit_ILI9341_setCursor(&tft, 10, 140);
    snprintf(buf, sizeof(buf), "Brightness: %d", screenBrightness);
    Adafruit_ILI9341_println(&tft, buf);
    
    Adafruit_ILI9341_fillRect(&tft, 10, 170, 100, 30, COLOR_GREEN);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setCursor(&tft, 35, 182);
    Adafruit_ILI9341_println(&tft, "Export Data");
    
    Adafruit_ILI9341_fillRect(&tft, 120, 170, 100, 30, COLOR_RED);
    Adafruit_ILI9341_setCursor(&tft, 145, 182);
    Adafruit_ILI9341_println(&tft, "Clear Data");
}

void showHistoryScreen() {
    currentScreen = SCREEN_TYPE_HISTORY;
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    
    Adafruit_ILI9341_fillRect(&tft, 0, 0, 320, 30, COLOR_BLUE);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_setCursor(&tft, 10, 8);
    Adafruit_ILI9341_println(&tft, "Data History");
    
    Adafruit_ILI9341_fillRect(&tft, 250, 5, 60, 20, COLOR_GRAY);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 270, 10);
    Adafruit_ILI9341_println(&tft, "Back");
    
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 10, 40);
    Adafruit_ILI9341_println(&tft, "Recent Readings:");
    
    int y = 60;
    int count = dataBufferCount < 8 ? dataBufferCount : 8;
    
    for (int i = dataBufferCount - count; i < dataBufferCount && y < 200; i++) {
        if (i >= 0) {
            VitalSigns* data = &dataBuffer[i];
            char timeStr[16];
            formatTime(data->timestamp, timeStr);
            char buf[64];
            snprintf(buf, sizeof(buf), "%s HR:%d SpO2:%d Bat:%d%%",
                     timeStr, (int)data->heartRate, (int)data->spO2, (int)data->batteryLevel);
            Adafruit_ILI9341_setCursor(&tft, 10, y);
            Adafruit_ILI9341_println(&tft, buf);
            y += 15;
        }
    }
    
    if (dataBufferCount == 0) {
        Adafruit_ILI9341_setCursor(&tft, 10, 60);
        Adafruit_ILI9341_println(&tft, "No data available");
    }
}

void drawHeart(int x, int y, uint16_t color) {
    Adafruit_ILI9341_fillCircle(&tft, x - 8, y - 5, 8, color);
    Adafruit_ILI9341_fillCircle(&tft, x + 8, y - 5, 8, color);
    Adafruit_ILI9341_fillTriangle(&tft, x - 15, y, x + 15, y, x, y + 15, color);
}

// ==================== SENSOR FUNCTIONS ====================
bool initializeSensor() {
    printf("Initializing MAX30102 sensor...\n");
    
    Wire_begin(SDA_PIN, SCL_PIN);
    
    if (!MAX30105_begin(&particleSensor)) {
        printf("MAX30102 not found\n");
        return false;
    }
    
    MAX30105_setup(&particleSensor);
    MAX30105_setPulseAmplitudeRed(&particleSensor, 0x0A);
    MAX30105_setPulseAmplitudeGreen(&particleSensor, 0);
    
    printf("MAX30102 initialized successfully\n");
    return true;
}

void updateSensors() {
    currentVitals.batteryLevel = readBatteryLevel();
    currentVitals.timestamp = millis();
    
    if (MAX30105_available(&particleSensor)) {
        redBuffer[bufferIndex] = MAX30105_getRed(&particleSensor);
        irBuffer[bufferIndex] = MAX30105_getIR(&particleSensor);
        
        fingerDetected = (irBuffer[bufferIndex] > FINGER_THRESHOLD);
        currentVitals.isFingerDetected = fingerDetected;
        
        bufferIndex++;
        
        if (bufferIndex >= BUFFER_SIZE) {
            bufferIndex = 0;
            
            if (fingerDetected) {
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
        
        MAX30105_nextSample(&particleSensor);
    }
}

float readBatteryLevel() {
    int rawValue = analogRead(BATTERY_PIN);
    float voltage = (rawValue / 4095.0) * 3.3 * 2;
    float percentage = ((voltage - 3.0) / 1.2) * 100.0;
    return percentage < 0 ? 0 : (percentage > 100 ? 100 : percentage);
}

// ==================== TOUCH HANDLING ====================
void handleTouch() {
    if (XPT2046_Touchscreen_touched(&ts)) {
        TS_Point p;
        XPT2046_Touchscreen_getPoint(&ts, &p);
        
        int x = (p.x - 200) * 320 / (3700 - 200);
        int y = (p.y - 240) * 240 / (3800 - 240);
        
        lastTouchTime = millis();
        
        if (!displayOn) {
            displayOn = true;
            return;
        }
        
        handleTouchEvent(x, y);
        
        delay(200);
    }
}

void handleTouchEvent(int x, int y) {
    switch (currentScreen) {
        case SCREEN_TYPE_MAIN:
            handleMainScreenTouch(x, y);
            break;
        case SCREEN_TYPE_SETTINGS:
            handleSettingsScreenTouch(x, y);
            break;
        case SCREEN_TYPE_HISTORY:
            handleHistoryScreenTouch(x, y);
            break;
        case SCREEN_TYPE_WIFI_CONFIG:
            break;
    }
}

void handleMainScreenTouch(int x, int y) {
    if (x >= 10 && x <= 100 && y >= 200 && y <= 230) {
        showSettingsScreen();
    }
    else if (x >= 115 && x <= 205 && y >= 200 && y <= 230) {
        showHistoryScreen();
    }
    else if (x >= 220 && x <= 310 && y >= 200 && y <= 230) {
        startConfigMode();
    }
}

void handleSettingsScreenTouch(int x, int y) {
    if (x >= 250 && x <= 310 && y >= 5 && y <= 25) {
        showMainScreen();
    }
    else if (x >= 10 && x <= 110 && y >= 170 && y <= 200) {
        exportData();
    }
    else if (x >= 120 && x <= 220 && y >= 170 && y <= 200) {
        clearData();
    }
}

void handleHistoryScreenTouch(int x, int y) {
    if (x >= 250 && x <= 310 && y >= 5 && y <= 25) {
        showMainScreen();
    }
}

void handleScreenTimeout() {
    if (displayOn && (millis() - lastTouchTime > SCREEN_TIMEOUT)) {
        displayOn = false;
    }
}

// ==================== WIFI FUNCTIONS ====================
void initializeWiFi() {
    printf("Initializing WiFi...\n");
    
    preferences_getString("wifi_ssid", wifiSSID, "");
    preferences_getString("wifi_pass", wifiPassword, "");
    
    if (strlen(wifiSSID) > 0) {
        connectToWiFi();
    } else {
        printf("No WiFi credentials found\n");
    }
}

void connectToWiFi() {
    if (strlen(wifiSSID) == 0) return;
    
    printf("Connecting to WiFi: %s\n", wifiSSID);
    
    WiFi_begin(wifiSSID, wifiPassword);
    
    unsigned long startTime = millis();
    while (WiFi_status() != WL_CONNECTED && millis() - startTime < CONNECTION_TIMEOUT) {
        delay(500);
        printf(".");
    }
    
    if (WiFi_status() == WL_CONNECTED) {
        wifiConnected = true;
        char ip[16];
        WiFi_localIP(ip);
        printf("\nWiFi connected! IP: %s\n", ip);
        setupWebServer();
    } else {
        wifiConnected = false;
        printf("\nWiFi connection failed\n");
        connectionAttempts++;
        
        if (connectionAttempts >= MAX_RETRY_ATTEMPTS) {
            printf("Max retry attempts reached, starting config mode\n");
            startConfigMode();
        }
    }
}

void handleWiFiConnection() {
    if (!wifiConnected && WiFi_status() == WL_CONNECTED) {
        wifiConnected = true;
        printf("WiFi reconnected\n");
        setupWebServer();
    } else if (wifiConnected && WiFi_status() != WL_CONNECTED) {
        wifiConnected = false;
        printf("WiFi disconnected\n");
    }
}

void startConfigMode() {
    printf("Starting WiFi configuration mode...\n");
    
    configModeActive = true;
    
    WiFi_softAP(AP_SSID, AP_PASSWORD);
    
    char ip[16];
    WiFi_softAPIP(ip);
    DNSServer_start(&dnsServer, 53, "*", ip);
    
    setupConfigServer();
    
    printf("Config mode active. Connect to '%s' and go to http://%s\n", AP_SSID, ip);
}

void setupWebServer() {
    WebServer_on(&server, "/", handleRoot);
    WebServer_on(&server, "/data", handleDataRequest);
    WebServer_on(&server, "/export", handleExportRequest);
    WebServer_begin(&server);
    printf("Web server started\n");
}

void setupConfigServer() {
    WebServer_on(&server, "/", handleConfigRoot);
    WebServer_on(&server, "/save", handleConfigSave);
    WebServer_on(&server, "/scan", handleWiFiScan);
    WebServer_begin(&server);
    printf("Config server started\n");
}

void handleRoot() {
    char html[512];
    snprintf(html, sizeof(html),
             "<!DOCTYPE html><html><head><title>Cardiac Monitor</title></head><body>"
             "<h1>Cardiac Monitor Status</h1>"
             "<p>Heart Rate: %d BPM</p>"
             "<p>SpO2: %d%%</p>"
             "<p>Battery: %d%%</p>"
             "<p>Finger Detected: %s</p>"
             "<p><a href='/data'>View Data</a> | <a href='/export'>Export Data</a></p>"
             "</body></html>",
             (int)currentVitals.heartRate, (int)currentVitals.spO2,
             (int)currentVitals.batteryLevel, currentVitals.isFingerDetected ? "Yes" : "No");
    
    WebServer_send(&server, 200, "text/html", html);
}

void handleConfigRoot() {
    char html[512];
    snprintf(html, sizeof(html),
             "<!DOCTYPE html><html><head><title>WiFi Configuration</title>"
             "<style>body{font-family:Arial;margin:40px;} .btn{background:#007cba;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;} .btn:hover{background:#005a87;}</style>"
             "</head><body>"
             "<h1>WiFi Configuration</h1>"
             "<form action='/save' method='post'>"
             "<p><label>Network Name (SSID):</label><br><input type='text' name='ssid' style='width:300px;padding:5px;'></p>"
             "<p><label>Password:</label><br><input type='password' name='password' style='width:300px;padding:5px;'></p>"
             "<p><input type='submit' value='Save Configuration' class='btn'></p>"
             "</form>"
             "<p><a href='/scan' class='btn'>Scan Networks</a></p>"
             "<p><strong>Current Status:</strong> %s</p>"
             "</body></html>",
             wifiConnected ? "Connected" : "Not Connected");
    
    WebServer_send(&server, 200, "text/html", html);
}

void handleConfigSave() {
    char ssid[MAX_STRING];
    char password[MAX_STRING];
    WebServer_arg(&server, "ssid", ssid, sizeof(ssid));
    WebServer_arg(&server, "password", password, sizeof(password));
    
    if (strlen(ssid) > 0) {
        preferences_putString("wifi_ssid", ssid);
        preferences_putString("wifi_pass", password);
        
        strncpy(wifiSSID, ssid, MAX_STRING);
        strncpy(wifiPassword, password, MAX_STRING);
        
        char html[256];
        snprintf(html, sizeof(html),
                 "<!DOCTYPE html><html><head><title>Configuration Saved</title></head><body>"
                 "<h1>Configuration Saved</h1>"
                 "<p>WiFi credentials have been saved. The device will restart and attempt to connect.</p>"
                 "</body></html>");
        
        WebServer_send(&server, 200, "text/html", html);
        
        delay(2000);
        // Placeholder for ESP.restart()
        printf("Restarting system...\n");
    } else {
        WebServer_send(&server, 400, "text/plain", "SSID cannot be empty");
    }
}

void handleWiFiScan() {
    char html[1024];
    snprintf(html, sizeof(html),
             "<!DOCTYPE html><html><head><title>Available Networks</title></head><body>"
             "<h1>Available WiFi Networks</h1>");
    
    int n = WiFi_scanNetworks();
    if (n == 0) {
        strncat(html, "<p>No networks found</p>", sizeof(html) - strlen(html) - 1);
    } else {
        strncat(html, "<ul>", sizeof(html) - strlen(html) - 1);
        char buf[128];
        for (int i = 0; i < n; ++i) {
            snprintf(buf, sizeof(buf), "<li>%s (%d dBm)%s</li>",
                     WiFi_SSID(i), WiFi_RSSI(i),
                     WiFi_encryptionType(i) == WIFI_AUTH_OPEN ? " [Open]" : " [Secured]");
            strncat(html, buf, sizeof(html) - strlen(html) - 1);
        }
        strncat(html, "</ul>", sizeof(html) - strlen(html) - 1);
    }
    
    strncat(html, "<p><a href='/'>Back to Configuration</a></p></body></html>",
            sizeof(html) - strlen(html) - 1);
    
    WebServer_send(&server, 200, "text/html", html);
}

void handleDataRequest() {
    char response[2048];
    snprintf(response, sizeof(response),
             "{\"current\":{\"heartRate\":%.1f,\"spO2\":%.1f,\"batteryLevel\":%.1f,"
             "\"fingerDetected\":%s,\"timestamp\":%lu},\"history\":[",
             currentVitals.heartRate, currentVitals.spO2, currentVitals.batteryLevel,
             currentVitals.isFingerDetected ? "true" : "false", currentVitals.timestamp);
    
    for (int i = 0; i < dataBufferCount; i++) {
        char entry[128];
        snprintf(entry, sizeof(entry),
                 "{\"heartRate\":%.1f,\"spO2\":%.1f,\"batteryLevel\":%.1f,\"timestamp\":%lu}%s",
                 dataBuffer[i].heartRate, dataBuffer[i].spO2, dataBuffer[i].batteryLevel,
                 dataBuffer[i].timestamp, i < dataBufferCount - 1 ? "," : "");
        strncat(response, entry, sizeof(response) - strlen(response) - 1);
    }
    
    strncat(response, "]}", sizeof(response) - strlen(response) - 1);
    WebServer_send(&server, 200, "application/json", response);
}

void handleExportRequest() {
    char csv[2048];
    snprintf(csv, sizeof(csv), "Timestamp,HeartRate,SpO2,BatteryLevel\n");
    
    for (int i = 0; i < dataBufferCount; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%lu,%.1f,%.1f,%.1f\n",
                 dataBuffer[i].timestamp, dataBuffer[i].heartRate,
                 dataBuffer[i].spO2, dataBuffer[i].batteryLevel);
        strncat(csv, line, sizeof(csv) - strlen(csv) - 1);
    }
    
    WebServer_send(&server, 200, "text/csv", csv);
}

// ==================== DATA LOGGING ====================
void logData() {
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        if (dataBufferCount < DATA_BUFFER_SIZE) {
            dataBuffer[dataBufferCount++] = currentVitals;
        } else {
            memmove(dataBuffer, dataBuffer + 1, (DATA_BUFFER_SIZE - 1) * sizeof(VitalSigns));
            dataBuffer[DATA_BUFFER_SIZE - 1] = currentVitals;
        }
        
        static int saveCounter = 0;
        saveCounter++;
        if (saveCounter >= 10) {
            saveCounter = 0;
            saveDataToFile();
        }
    }
}

void saveDataToFile() {
    File file;
    if (SPIFFS_open(&file, "/data.csv", "w")) {
        File_println(&file, "Timestamp,HeartRate,SpO2,BatteryLevel");
        for (int i = 0; i < dataBufferCount; i++) {
            char line[128];
            snprintf(line, sizeof(line), "%lu,%.1f,%.1f,%.1f",
                     dataBuffer[i].timestamp, dataBuffer[i].heartRate,
                     dataBuffer[i].spO2, dataBuffer[i].batteryLevel);
            File_println(&file, line);
        }
        File_close(&file);
        printf("Data saved to file\n");
    } else {
        printf("Failed to save data to file\n");
    }
}

void loadDataFromFile() {
    File file;
    if (SPIFFS_open(&file, "/data.csv", "r")) {
        dataBufferCount = 0;
        char line[128];
        File_readStringUntil(&file, '\n', line, sizeof(line)); // Skip header
        
        while (File_available(&file)) {
            File_readStringUntil(&file, '\n', line, sizeof(line));
            if (strlen(line) > 0 && dataBufferCount < DATA_BUFFER_SIZE) {
                VitalSigns data;
                char* token = strtok(line, ",");
                data.timestamp = token ? atol(token) : 0;
                token = strtok(NULL, ",");
                data.heartRate = token ? atof(token) : 0;
                token = strtok(NULL, ",");
                data.spO2 = token ? atof(token) : 0;
                token = strtok(NULL, ",");
                data.batteryLevel = token ? atof(token) : 0;
                dataBuffer[dataBufferCount++] = data;
            }
        }
        File_close(&file);
        printf("Loaded %d data entries from file\n", dataBufferCount);
    }
}

void exportData() {
    printf("Exporting data...\n");
    saveDataToFile();
    
    Adafruit_ILI9341_fillRect(&tft, 50, 100, 220, 60, COLOR_GREEN);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_setCursor(&tft, 80, 120);
    Adafruit_ILI9341_println(&tft, "Data Exported!");
    
    delay(2000);
    showSettingsScreen();
}

void clearData() {
    printf("Clearing data...\n");
    dataBufferCount = 0;
    SPIFFS_remove("/data.csv");
    
    Adafruit_ILI9341_fillRect(&tft, 50, 100, 220, 60, COLOR_RED);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    Adafruit_ILI9341_setCursor(&tft, 90, 120);
    Adafruit_ILI9341_println(&tft, "Data Cleared!");
    
    delay(2000);
    showSettingsScreen();
}

// ==================== ALERT SYSTEM ====================
void checkAlerts() {
    if (!alertThresholds.enabled) return;
    
    char message[MAX_STRING];
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        if (currentVitals.heartRate < alertThresholds.heartRateMin || 
            currentVitals.heartRate > alertThresholds.heartRateMax) {
            snprintf(message, sizeof(message), "Heart rate: %d BPM", (int)currentVitals.heartRate);
            int level = (currentVitals.heartRate < 50 || currentVitals.heartRate > 120) ? 
                        ALERT_LEVEL_CRITICAL : ALERT_LEVEL_WARNING;
            triggerAlert(level, message);
        }
    }
    
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        if (currentVitals.spO2 < alertThresholds.spO2Min) {
            snprintf(message, sizeof(message), "Low SpO2: %d%%", (int)currentVitals.spO2);
            int level = (currentVitals.spO2 < 90) ? ALERT_LEVEL_CRITICAL : ALERT_LEVEL_WARNING;
            triggerAlert(level, message);
        }
    }
    
    if (currentVitals.batteryLevel < alertThresholds.batteryMin) {
        snprintf(message, sizeof(message), "Low battery: %d%%", (int)currentVitals.batteryLevel);
        int level = (currentVitals.batteryLevel < 10) ? ALERT_LEVEL_CRITICAL : ALERT_LEVEL_WARNING;
        triggerAlert(level, message);
    }
    
    removeOldAlerts();
}

void triggerAlert(int level, const char* message) {
    if (millis() - lastAlertTime < ALERT_COOLDOWN) return;
    
    if (activeAlertsCount < MAX_ALERTS) {
        Alert alert;
        alert.level = level;
        strncpy(alert.message, message, MAX_STRING);
        alert.timestamp = millis();
        alert.acknowledged = false;
        activeAlerts[activeAlertsCount++] = alert;
    }
    
    if (alertHistoryCount < MAX_HISTORY) {
        Alert alert;
        alert.level = level;
        strncpy(alert.message, message, MAX_STRING);
        alert.timestamp = millis();
        alert.acknowledged = false;
        alertHistory[alertHistoryCount++] = alert;
    } else {
        memmove(alertHistory, alertHistory + 1, (MAX_HISTORY - 1) * sizeof(Alert));
        Alert alert;
        alert.level = level;
        strncpy(alert.message, message, MAX_STRING);
        alert.timestamp = millis();
        alert.acknowledged = false;
        alertHistory[MAX_HISTORY - 1] = alert;
    }
    
    lastAlertTime = millis();
    
    playAlertSound(level);
    showAlert(message, level);
    
    printf("ALERT [%s]: %s\n", 
           level == ALERT_LEVEL_CRITICAL ? "CRITICAL" : 
           level == ALERT_LEVEL_WARNING ? "WARNING" : "INFO", 
           message);
}

void playAlertSound(int level) {
    int beepCount = 1;
    int beepDuration = 200;
    
    switch (level) {
        case ALERT_LEVEL_CRITICAL:
            beepCount = 3;
            beepDuration = 500;
            break;
        case ALERT_LEVEL_WARNING:
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

void showAlert(const char* message, int level) {
    uint16_t alertColor = COLOR_YELLOW;
    switch (level) {
        case ALERT_LEVEL_CRITICAL:
            alertColor = COLOR_RED;
            break;
        case ALERT_LEVEL_WARNING:
            alertColor = COLOR_ORANGE;
            break;
        default:
            alertColor = COLOR_YELLOW;
            break;
    }
    
    Adafruit_ILI9341_fillRect(&tft, 0, 30, 320, 25, alertColor);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_BLACK);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 5, 38);
    Adafruit_ILI9341_println(&tft, message);
    
    static unsigned long alertDisplayTime = millis();
    if (millis() - alertDisplayTime > 5000) {
        Adafruit_ILI9341_fillRect(&tft, 0, 30, 320, 25, COLOR_BLACK);
        alertDisplayTime = millis();
    }
}

void removeOldAlerts() {
    unsigned long currentTime = millis();
    int newCount = 0;
    for (int i = 0; i < activeAlertsCount; i++) {
        if (!activeAlerts[i].acknowledged && 
            (currentTime - activeAlerts[i].timestamp <= 30000)) {
            activeAlerts[newCount++] = activeAlerts[i];
        }
    }
    activeAlertsCount = newCount;
}

// ==================== SETTINGS MANAGEMENT ====================
void loadSettings() {
    alertThresholds.heartRateMin = preferences_getFloat("hr_min", 60.0f);
    alertThresholds.heartRateMax = preferences_getFloat("hr_max", 100.0f);
    alertThresholds.spO2Min = preferences_getFloat("spo2_min", 95.0f);
    alertThresholds.batteryMin = preferences_getFloat("bat_min", 20.0f);
    alertThresholds.enabled = preferences_getBool("alerts_en", true);
    screenBrightness = preferences_getInt("brightness", 128);
    
    printf("Settings loaded from preferences\n");
}

void saveSettings() {
    preferences_putFloat("hr_min", alertThresholds.heartRateMin);
    preferences_putFloat("hr_max", alertThresholds.heartRateMax);
    preferences_putFloat("spo2_min", alertThresholds.spO2Min);
    preferences_putFloat("bat_min", alertThresholds.batteryMin);
    preferences_putBool("alerts_en", alertThresholds.enabled);
    preferences_putInt("brightness", screenBrightness);
    
    printf("Settings saved to preferences\n");
}

// ==================== UTILITY FUNCTIONS ====================
void updateDisplay() {
    if (!displayOn) return;
    
    switch (currentScreen) {
        case SCREEN_TYPE_MAIN:
            updateVitalSigns();
            drawWaveform();
            drawStatusBar();
            break;
        case SCREEN_TYPE_SETTINGS:
        case SCREEN_TYPE_HISTORY:
        case SCREEN_TYPE_WIFI_CONFIG:
            break;
    }
}

void formatTime(unsigned long timestamp, char* output) {
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;
    
    snprintf(output, 16, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

void printSystemInfo() {
    printf("\n=== System Information ===\n");
    printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    printf("Device Name: %s\n", DEVICE_NAME);
    printf("Free Heap: %d bytes\n", ESP_getFreeHeap());
    printf("Flash Size: %d bytes\n", ESP_getFlashChipSize());
    printf("CPU Frequency: %d MHz\n", ESP_getCpuFreqMHz());
    
    if (wifiConnected) {
        char ssid[32];
        char ip[16];
        WiFi_SSID(ssid);
        WiFi_localIP(ip);
        printf("WiFi SSID: %s\n", ssid);
        printf("IP Address: %s\n", ip);
        printf("Signal Strength: %d dBm\n", WiFi_RSSI());
    } else {
        printf("WiFi: Not connected\n");
    }
    
    printf("Sensor Status: %s\n", MAX30105_begin(&particleSensor) ? "Connected" : "Disconnected");
    printf("Display Status: Active\n");
    printf("Touch Status: %s\n", XPT2046_Touchscreen_begin(&ts) ? "Active" : "Inactive");
    printf("Data Buffer: %d/%d entries\n", dataBufferCount, DATA_BUFFER_SIZE);
    printf("Active Alerts: %d\n", activeAlertsCount);
    printf("========================\n");
}

void performSelfTest() {
    printf("Performing system self-test...\n");
    
    bool testsPassed = true;
    
    printf("Testing display... ");
    Adafruit_ILI9341_fillScreen(&tft, COLOR_RED);
    delay(500);
    Adafruit_ILI9341_fillScreen(&tft, COLOR_GREEN);
    delay(500);
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLUE);
    delay(500);
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    printf("OK\n");
    
    printf("Testing touch controller... ");
    if (XPT2046_Touchscreen_begin(&ts)) {
        printf("OK\n");
    } else {
        printf("FAILED\n");
        testsPassed = false;
    }
    
    printf("Testing MAX30102 sensor... ");
    if (MAX30105_begin(&particleSensor)) {
        printf("OK\n");
    } else {
        printf("FAILED\n");
        testsPassed = false;
    }
    
    printf("Testing file system... ");
    if (SPIFFS_begin(true)) {
        printf("OK\n");
    } else {
        printf("FAILED\n");
        testsPassed = false;
    }
    
    printf("Testing buzzer... ");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    printf("OK\n");
    
    printf("Testing battery monitor... ");
    float batteryLevel = readBatteryLevel();
    if (batteryLevel >= 0 && batteryLevel <= 100) {
        printf("OK (%.1f%%)\n", batteryLevel);
    } else {
        printf("WARNING - Unusual reading\n");
    }
    
    printf("Self-test %s\n", testsPassed ? "PASSED" : "FAILED");
    
    if (testsPassed) {
        for (int i = 0; i < 3; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
            delay(100);
        }
    } else {
        for (int i = 0; i < 5; i++) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(200);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
        }
    }
}

void handleSerialCommands() {
    char command[32];
    if (Serial_available()) {
        Serial_readStringUntil('\n', command, sizeof(command));
        for (int i = 0; command[i]; i++) {
            if (command[i] >= 'A' && command[i] <= 'Z') {
                command[i] += 32; // Convert to lowercase
            }
        }
        while (command[strlen(command) - 1] == '\r' || command[strlen(command) - 1] == '\n') {
            command[strlen(command) - 1] = '\0';
        }
        
        if (strcmp(command, "help") == 0) {
            printf("\n=== Available Commands ===\n");
            printf("help - Show this help message\n");
            printf("info - Show system information\n");
            printf("test - Perform self-test\n");
            printf("reset - Reset system\n");
            printf("wifi - Show WiFi status\n");
            printf("data - Show current readings\n");
            printf("export - Export data to serial\n");
            printf("clear - Clear data buffer\n");
            printf("alerts - Show active alerts\n");
            printf("config - Enter configuration mode\n");
            printf("========================\n");
        }
        else if (strcmp(command, "info") == 0) {
            printSystemInfo();
        }
        else if (strcmp(command, "test") == 0) {
            performSelfTest();
        }
        else if (strcmp(command, "reset") == 0) {
            printf("Resetting system...\n");
            // Placeholder for ESP.restart()
        }
        else if (strcmp(command, "wifi") == 0) {
            printf("WiFi Status: %s\n", wifiConnected ? "Connected" : "Disconnected");
            if (wifiConnected) {
                char ssid[32];
                char ip[16];
                WiFi_SSID(ssid);
                WiFi_localIP(ip);
                printf("SSID: %s\n", ssid);
                printf("IP: %s\n", ip);
                printf("RSSI: %d dBm\n", WiFi_RSSI());
            }
        }
        else if (strcmp(command, "data") == 0) {
            printf("Heart Rate: %.1f BPM\n", currentVitals.heartRate);
            printf("SpO2: %.1f%%\n", currentVitals.spO2);
            printf("Battery: %.1f%%\n", currentVitals.batteryLevel);
            printf("Finger Detected: %s\n", currentVitals.isFingerDetected ? "Yes" : "No");
        }
        else if (strcmp(command, "export") == 0) {
            printf("Timestamp,HeartRate,SpO2,BatteryLevel\n");
            for (int i = 0; i < dataBufferCount; i++) {
                printf("%lu,%.1f,%.1f,%.1f\n", 
                       dataBuffer[i].timestamp, dataBuffer[i].heartRate,
                       dataBuffer[i].spO2, dataBuffer[i].batteryLevel);
            }
        }
        else if (strcmp(command, "clear") == 0) {
            dataBufferCount = 0;
            printf("Data buffer cleared\n");
        }
        else if (strcmp(command, "alerts") == 0) {
            printf("Active Alerts: %d\n", activeAlertsCount);
            for (int i = 0; i < activeAlertsCount; i++) {
                printf("- %s: %s\n", 
                       activeAlerts[i].level == ALERT_LEVEL_CRITICAL ? "CRITICAL" :
                       activeAlerts[i].level == ALERT_LEVEL_WARNING ? "WARNING" : "INFO",
                       activeAlerts[i].message);
            }
        }
        else if (strcmp(command, "config") == 0) {
            startConfigMode();
            printf("Configuration mode started\n");
        }
        else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
}

void watchdogFeed() {
    yield();
}

void handleLowPowerMode() {
    if (currentVitals.batteryLevel < 10 && !wifiConnected) {
        printf("Entering low power mode...\n");
        
        screenBrightness = 50;
        
        if (!wifiConnected) {
            WiFi_mode(WIFI_OFF);
        }
        
        Adafruit_ILI9341_fillRect(&tft, 0, 0, 320, 20, COLOR_RED);
        Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
        Adafruit_ILI9341_setTextSize(&tft, 1);
        Adafruit_ILI9341_setCursor(&tft, 5, 5);
        Adafruit_ILI9341_println(&tft, "LOW POWER MODE - Connect charger");
    }
}

void checkMemoryUsage() {
    static unsigned long lastMemCheck = 0;
    
    if (millis() - lastMemCheck > 30000) {
        lastMemCheck = millis();
        
        size_t freeHeap = ESP_getFreeHeap();
        if (freeHeap < 10000) {
            printf("WARNING: Low memory - %d bytes free\n", freeHeap);
            
            if (dataBufferCount > 50) {
                memmove(dataBuffer, dataBuffer + 25, (dataBufferCount - 25) * sizeof(VitalSigns));
                dataBufferCount -= 25;
                printf("Cleaned up data buffer to free memory\n");
            }
            
            if (alertHistoryCount > 25) {
                memmove(alertHistory, alertHistory + 10, (alertHistoryCount - 10) * sizeof(Alert));
                alertHistoryCount -= 10;
                printf("Cleaned up alert history to free memory\n");
            }
        }
    }
}

void initializeSystem() {
    printf("Reinitializing system...\n");
    
    currentState = SYSTEM_STATE_INITIALIZING;
    currentScreen = SCREEN_TYPE_MAIN;
    
    bufferIndex = 0;
    memset(irBuffer, 0, sizeof(irBuffer));
    memset(redBuffer, 0, sizeof(redBuffer));
    
    lastSensorUpdate = 0;
    lastDisplayUpdate = 0;
    lastDataLog = 0;
    lastAlertCheck = 0;
    
    loadDataFromFile();
    
    currentState = SYSTEM_STATE_RUNNING;
    showMainScreen();
    
    printf("System reinitialization complete\n");
}

void handleSystemError(const char* errorMessage) {
    printf("SYSTEM ERROR: %s\n", errorMessage);
    
    currentState = SYSTEM_STATE_ERROR;
    
    showError("System Error", errorMessage);
    
    for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
    
    delay(5000);
    
    printf("Attempting system recovery...\n");
    initializeSystem();
}

// ==================== SIMULATED PREFERENCES ====================
void preferences_begin(const char* name, bool readOnly) {
    // Simulate preferences initialization
    memset(&preferences, 0, sizeof(preferences));
}

float preferences_getFloat(const char* key, float defaultValue) {
    if (strcmp(key, "hr_min") == 0) return preferences.hr_min;
    if (strcmp(key, "hr_max") == 0) return preferences.hr_max;
    if (strcmp(key, "spo2_min") == 0) return preferences.spo2_min;
    if (strcmp(key, "bat_min") == 0) return preferences.bat_min;
    return defaultValue;
}

bool preferences_getBool(const char* key, bool defaultValue) {
    if (strcmp(key, "alerts_en") == 0) return preferences.alerts_en;
    return defaultValue;
}

int preferences_getInt(const char* key, int defaultValue) {
    if (strcmp(key, "brightness") == 0) return preferences.brightness;
    return defaultValue;
}

void preferences_getString(const char* key, char* output, const char* defaultValue) {
    if (strcmp(key, "wifi_ssid") == 0) {
        strncpy(output, preferences.wifi_ssid, MAX_STRING);
    } else if (strcmp(key, "wifi_pass") == 0) {
        strncpy(output, preferences.wifi_pass, MAX_STRING);
    } else {
        strncpy(output, defaultValue, MAX_STRING);
    }
}

void preferences_putFloat(const char* key, float value) {
    if (strcmp(key, "hr_min") == 0) preferences.hr_min = value;
    else if (strcmp(key, "hr_max") == 0) preferences.hr_max = value;
    else if (strcmp(key, "spo2_min") == 0) preferences.spo2_min = value;
    else if (strcmp(key, "bat_min") == 0) preferences.bat_min = value;
}

void preferences_putBool(const char* key, bool value) {
    if (strcmp(key, "alerts_en") == 0) preferences.alerts_en = value;
}

void preferences_putInt(const char* key, int value) {
    if (strcmp(key, "brightness") == 0) preferences.brightness = value;
}

void preferences_putString(const char* key, const char* value) {
    if (strcmp(key, "wifi_ssid") == 0) {
        strncpy(preferences.wifi_ssid, value, MAX_STRING);
    } else if (strcmp(key, "wifi_pass") == 0) {
        strncpy(preferences.wifi_pass, value, MAX_STRING);
    }
}

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