/*
 * Cardiac Monitor for Arduino Uno
 * A simplified cardiac monitoring system with heart rate, SpO2 monitoring,
 * and TFT display interface, adapted for Arduino Uno's limited resources.
 * 
 * Hardware Requirements:
 * - Arduino Uno
 * - MAX30102 Heart Rate/SpO2 Sensor (I2C: SDA=A4, SCL=A5)
 * - ILI9341 3.2" TFT Display (SPI: CS=10, DC=9, MOSI=11, CLK=13, RST=8)
 * - Buzzer (pin 7)
 * - Battery Monitor (A0)
 * - Two push buttons (pins 2, 3) for navigation
 * 
 * WARNING: This is for educational purposes only. Not for medical use.
 */

#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ==================== PIN DEFINITIONS ====================
#define TFT_CS    10
#define TFT_DC    9
#define TFT_MOSI  11
#define TFT_CLK   13
#define TFT_RST   8

#define BUZZER_PIN 7
#define BATTERY_PIN A0
#define BUTTON_1 2
#define BUTTON_2 3

// ==================== CONFIGURATION ====================
#define FIRMWARE_VERSION "2.0.0"
#define DEVICE_NAME "CardiacMonitor"
#define SENSOR_UPDATE_INTERVAL 200  // ms
#define DISPLAY_UPDATE_INTERVAL 200 // ms
#define DATA_LOG_INTERVAL 2000     // ms

#define SAMPLE_RATE 50
#define BUFFER_SIZE 50
#define FINGER_THRESHOLD 50000
#define SPO2_BUFFER_SIZE 50

#define MAX_ALERTS 5
#define MAX_HISTORY 10
#define DATA_BUFFER_SIZE 20
#define MAX_STRING 32

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

#define SCREEN_TYPE_MAIN 0
#define SCREEN_TYPE_SETTINGS 1
#define SCREEN_TYPE_HISTORY 2

// ==================== GLOBAL OBJECTS ====================
Adafruit_ILI9341 tft = {TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, -1};
MAX30105 particleSensor;

// ==================== GLOBAL VARIABLES ====================
int currentState = SYSTEM_STATE_INITIALIZING;
int currentScreen = SCREEN_TYPE_MAIN;
VitalSigns currentVitals;
AlertThresholds alertThresholds;

unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDataLog = 0;
unsigned long lastAlertCheck = 0;

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
unsigned long lastButtonPress = 0;
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
#define COLOR_GRAY      0x7BEF

// EEPROM addresses
#define EEPROM_HR_MIN 0
#define EEPROM_HR_MAX 4
#define EEPROM_SPO2_MIN 8
#define EEPROM_BAT_MIN 12
#define EEPROM_ALERTS_EN 16
#define EEPROM_BRIGHTNESS 17
#define EEPROM_DATA_START 20

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
void showError(const char* title, const char* message);
void showSettingsScreen(void);
void showHistoryScreen(void);
void drawHeart(int x, int y, uint16_t color);
bool initializeSensor(void);
void updateSensors(void);
float readBatteryLevel(void);
void handleButtons(void);
void handleMainScreenButtons(void);
void handleSettingsScreenButtons(void);
void handleHistoryScreenButtons(void);
void handleScreenTimeout(void);
void logData(void);
void saveDataToEEPROM(void);
void loadDataFromEEPROM(void);
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

// ==================== SETUP FUNCTION ====================
void setup() {
    Serial_begin(9600);
    printf("\n=== Cardiac Monitor v2.0 (Arduino Uno) ===\n");
    printf("Initializing system...\n");
    
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);
    digitalWrite(BUZZER_PIN, LOW);
    
    loadSettings();
    
    if (!initializeDisplay()) {
        printf("FATAL: Display initialization failed\n");
        while (true) delay(1000);
    }
    
    showSplashScreen();
    delay(2000);
    
    if (!initializeSensor()) {
        showError("Sensor Error", "Failed to initialize MAX30102");
        delay(5000);
    }
    
    loadDataFromEEPROM();
    
    currentState = SYSTEM_STATE_RUNNING;
    currentScreen = SCREEN_TYPE_MAIN;
    showMainScreen();
    
    printf("System initialization complete\n");
    printf("=================================\n");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long currentTime = millis();
    
    handleButtons();
    
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
    
    printf("Display initialized successfully\n");
    return true;
}

void showSplashScreen() {
    Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 2);
    
    int16_t x1, y1;
    uint16_t w, h;
    Adafruit_ILI9341_getTextBounds(&tft, "Cardiac Monitor", 0, 0, &x1, &y1, &w, &h);
    Adafruit_ILI9341_setCursor(&tft, (320 - w) / 2, 80);
    Adafruit_ILI9341_println(&tft, "Cardiac Monitor");
    
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_getTextBounds(&tft, "v2.0", 0, 0, &x1, &y1, &w, &h);
    Adafruit_ILI9341_setCursor(&tft, (320 - w) / 2, 120);
    Adafruit_ILI9341_println(&tft, "v2.0");
    
    drawHeart(160, 160, COLOR_RED);
    
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_GRAY);
    Adafruit_ILI9341_setCursor(&tft, 10, 220);
    Adafruit_ILI9341_println(&tft, "Educational use only");
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
    float batteryLevel = readBatteryLevel();
    uint16_t batteryColor = batteryLevel > 20 ? COLOR_GREEN : COLOR_RED;
    Adafruit_ILI9341_setTextColor(&tft, batteryColor);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 280, 10);
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
}

void drawMainButtons() {
    Adafruit_ILI9341_fillRect(&tft, 10, 200, 90, 30, COLOR_GRAY);
    Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
    Adafruit_ILI9341_setTextSize(&tft, 1);
    Adafruit_ILI9341_setCursor(&tft, 35, 212);
    Adafruit_ILI9341_println(&tft, "Settings (Btn 1)");
    
    Adafruit_ILI9341_fillRect(&tft, 115, 200, 90, 30, COLOR_GRAY);
    Adafruit_ILI9341_setCursor(&tft, 145, 212);
    Adafruit_ILI9341_println(&tft, "History (Btn 2)");
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
    Adafruit_ILI9341_println(&tft, "Back (Btn 1)");
    
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
    Adafruit_ILI9341_println(&tft, "Back (Btn 1)");
    
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
            snprintf(buf, sizeof(buf), "%s HR:%d SpO2:%d", 
                     timeStr, (int)data->heartRate, (int)data->spO2);
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
    
    Wire_begin();
    
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
    float voltage = (rawValue / 1023.0) * 5.0;
    float percentage = ((voltage - 3.0) / 1.2) * 100.0;
    return percentage < 0 ? 0 : (percentage > 100 ? 100 : percentage);
}

// ==================== BUTTON HANDLING ====================
void handleButtons() {
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 200;
    
    if (millis() - lastDebounceTime < debounceDelay) return;
    
    if (digitalRead(BUTTON_1) == LOW) {
        lastButtonPress = millis();
        if (!displayOn) {
            displayOn = true;
            return;
        }
        switch (currentScreen) {
            case SCREEN_TYPE_MAIN:
            case SCREEN_TYPE_HISTORY:
                showSettingsScreen();
                break;
            case SCREEN_TYPE_SETTINGS:
                showMainScreen();
                break;
        }
        lastDebounceTime = millis();
    }
    
    if (digitalRead(BUTTON_2) == LOW) {
        lastButtonPress = millis();
        if (!displayOn) {
            displayOn = true;
            return;
        }
        if (currentScreen == SCREEN_TYPE_MAIN) {
            showHistoryScreen();
        }
        lastDebounceTime = millis();
    }
}

void handleScreenTimeout() {
    if (displayOn && (millis() - lastButtonPress > SCREEN_TIMEOUT)) {
        displayOn = false;
        Adafruit_ILI9341_fillScreen(&tft, COLOR_BLACK);
    }
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
        if (saveCounter >= 5) {
            saveCounter = 0;
            saveDataToEEPROM();
        }
    }
}

void saveDataToEEPROM() {
    int addr = EEPROM_DATA_START;
    EEPROM_write(addr, dataBufferCount);
    addr++;
    
    for (int i = 0; i < dataBufferCount && addr < EEPROM.length() - 12; i++) {
        EEPROM_writeFloat(addr, dataBuffer[i].heartRate); addr += 4;
        EEPROM_writeFloat(addr, dataBuffer[i].spO2); addr += 4;
        EEPROM_writeFloat(addr, dataBuffer[i].batteryLevel); addr += 4;
    }
    
    EEPROM_commit();
    printf("Data saved to EEPROM\n");
}

void loadDataFromEEPROM() {
    dataBufferCount = EEPROM_read(EEPROM_DATA_START);
    if (dataBufferCount > DATA_BUFFER_SIZE) dataBufferCount = DATA_BUFFER_SIZE;
    
    int addr = EEPROM_DATA_START + 1;
    for (int i = 0; i < dataBufferCount && addr < EEPROM.length() - 12; i++) {
        dataBuffer[i].heartRate = EEPROM_readFloat(addr); addr += 4;
        dataBuffer[i].spO2 = EEPROM_readFloat(addr); addr += 4;
        dataBuffer[i].batteryLevel = EEPROM_readFloat(addr); addr += 4;
        dataBuffer[i].timestamp = millis();
    }
    
    printf("Loaded %d data entries from EEPROM\n", dataBufferCount);
}

void clearData() {
    printf("Clearing data...\n");
    dataBufferCount = 0;
    EEPROM_write(EEPROM_DATA_START, 0);
    EEPROM_commit();
    
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
    alertThresholds.heartRateMin = EEPROM_readFloat(EEPROM_HR_MIN);
    if (alertThresholds.heartRateMin < 1) alertThresholds.heartRateMin = 60.0;
    alertThresholds.heartRateMax = EEPROM_readFloat(EEPROM_HR_MAX);
    if (alertThresholds.heartRateMax < 1) alertThresholds.heartRateMax = 100.0;
    alertThresholds.spO2Min = EEPROM_readFloat(EEPROM_SPO2_MIN);
    if (alertThresholds.spO2Min < 1) alertThresholds.spO2Min = 95.0;
    alertThresholds.batteryMin = EEPROM_readFloat(EEPROM_BAT_MIN);
    if (alertThresholds.batteryMin < 1) alertThresholds.batteryMin = 20.0;
    alertThresholds.enabled = EEPROM_read(EEPROM_ALERTS_EN);
    screenBrightness = EEPROM_read(EEPROM_BRIGHTNESS);
    if (screenBrightness < 1) screenBrightness = 128;
    
    printf("Settings loaded from EEPROM\n");
}

void saveSettings() {
    EEPROM_writeFloat(EEPROM_HR_MIN, alertThresholds.heartRateMin);
    EEPROM_writeFloat(EEPROM_HR_MAX, alertThresholds.heartRateMax);
    EEPROM_writeFloat(EEPROM_SPO2_MIN, alertThresholds.spO2Min);
    EEPROM_writeFloat(EEPROM_BAT_MIN, alertThresholds.batteryMin);
    EEPROM_write(EEPROM_ALERTS_EN, alertThresholds.enabled);
    EEPROM_write(EEPROM_BRIGHTNESS, screenBrightness);
    EEPROM_commit();
    
    printf("Settings saved to EEPROM\n");
}

// ==================== UTILITY FUNCTIONS ====================
void updateDisplay() {
    if (!displayOn) return;
    
    if (currentScreen == SCREEN_TYPE_MAIN) {
        updateVitalSigns();
        drawStatusBar();
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
    printf("Free SRAM: %d bytes\n", freeMemory());
    printf("Sensor Status: %s\n", MAX30105_begin(&particleSensor) ? "Connected" : "Disconnected");
    printf("Display Status: Active\n");
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
    
    printf("Testing MAX30102 sensor... ");
    if (MAX30105_begin(&particleSensor)) {
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
            printf("data - Show current readings\n");
            printf("export - Export data to serial\n");
            printf("clear - Clear data buffer\n");
            printf("alerts - Show active alerts\n");
            printf("========================\n");
        }
        else if (strcmp(command, "info") == 0) {
            printSystemInfo();
        }
        else if (strcmp(command, "test") == 0) {
            performSelfTest();
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
            clearData();
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
        else {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }
}

void watchdogFeed() {
    yield();
}

void handleLowPowerMode() {
    if (currentVitals.batteryLevel < 10) {
        printf("Entering low power mode...\n");
        screenBrightness = 50;
        Adafruit_ILI9341_fillRect(&tft, 0, 0, 320, 20, COLOR_RED);
        Adafruit_ILI9341_setTextColor(&tft, COLOR_WHITE);
        Adafruit_ILI9341_setTextSize(&tft, 1);
        Adafruit_ILI9341_setCursor(&tft, 5, 5);
        Adafruit_ILI9341_println(&tft, "LOW POWER MODE");
    }
}

void checkMemoryUsage() {
    static unsigned long lastMemCheck = 0;
    
    if (millis() - lastMemCheck > 30000) {
        lastMemCheck = millis();
        
        int freeMem = freeMemory();
        if (freeMem < 200) {
            printf("WARNING: Low memory - %d bytes free\n", freeMem);
            if (dataBufferCount > 10) {
                memmove(dataBuffer, dataBuffer + 5, (dataBufferCount - 5) * sizeof(VitalSigns));
                dataBufferCount -= 5;
                printf("Cleaned up data buffer to free memory\n");
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
    
    loadDataFromEEPROM();
    
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

// Helper function to estimate free SRAM
int freeMemory() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/*
 * USAGE INSTRUCTIONS:
 * 
 * 1. Hardware Setup:
 *    - Connect MAX30102 sensor to I2C (SDA=A4, SCL=A5)
 *    - Connect ILI9341 display to SPI (CS=10, DC=9, MOSI=11, CLK=13, RST=8)
 *    - Connect buzzer to pin 7
 *    - Connect battery monitor to A0
 *    - Connect push buttons to pins 2 and 3 (with pull-up resistors)
 * 
 * 2. Operation:
 *    - Place finger on MAX30102 sensor
 *    - View heart rate and SpO2 on display
 *    - Use Button 1 to switch to Settings or return to Main
 *    - Use Button 2 to switch to History from Main
 * 
 * 3. Serial Commands:
 *    - Connect at 9600 baud
 *    - Type "help" for available commands
 * 
 * 4. Data Export:
 *    - Use serial command "export" to retrieve data
 * 
 * SAFETY WARNING:
 * This device is for educational purposes only.
 * Do not use for medical diagnosis or treatment.
 * Always consult healthcare professionals for medical advice.
 */