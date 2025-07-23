/*
 * Cardiac Monitor Arduino Uno - Simplified System
 * A basic cardiac monitoring system with heart rate, SpO2 monitoring,
 * simplified TFT display interface, and EEPROM data logging.
 * 
 * Hardware Requirements:
 * - Arduino Uno
 * - MAX30102 Heart Rate/SpO2 Sensor (I2C)
 * - ILI9341 2.8" TFT Display (SPI)
 * - Two push buttons for navigation
 * - Buzzer for alerts
 * - Voltage divider for battery monitoring
 * 
 * WARNING: This is for educational purposes only. Not for medical use.
 */

#include <Wire.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

// ==================== PIN DEFINITIONS ====================
// Display Pins (SPI)
#define TFT_CS    10  // Chip Select
#define TFT_DC    9   // Data/Command
#define TFT_RST   8   // Reset

// Sensor Pins (I2C)
#define SDA_PIN   A4  // I2C SDA
#define SCL_PIN   A5  // I2C SCL

// Input and Output Pins
#define BUTTON1_PIN 2  // Menu navigation
#define BUTTON2_PIN 3  // Select/Action
#define BUZZER_PIN  6  // PWM-capable for buzzer
#define BATTERY_PIN A0 // Analog input for battery

// ==================== CONFIGURATION ====================
// System Configuration
const char* FIRMWARE_VERSION = "1.0.0";
const int SENSOR_UPDATE_INTERVAL = 100;  // ms
const int DISPLAY_UPDATE_INTERVAL = 500; // ms, slower for Uno
const int DATA_LOG_INTERVAL = 5000;      // ms, less frequent

// Sensor Configuration
const int SAMPLE_RATE = 50;  // Reduced for Uno
const int BUFFER_SIZE = 50;  // Smaller buffer to save memory
const int FINGER_THRESHOLD = 50000;

// Alert Thresholds
struct AlertThresholds {
    float heartRateMin = 60.0f;
    float heartRateMax = 100.0f;
    float spO2Min = 95.0f;
    float batteryMin = 20.0f;
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
    char message[20]; // Limited size for Uno
    unsigned long timestamp;
};

enum class ScreenType {
    MAIN,
    HISTORY
};

// ==================== GLOBAL OBJECTS ====================
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
MAX30105 particleSensor;

// ==================== GLOBAL VARIABLES ====================
// System State
ScreenType currentScreen = ScreenType::MAIN;
VitalSigns currentVitals;
AlertThresholds alertThresholds;

// Timing Variables
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDataLog = 0;
unsigned long lastAlertCheck = 0;

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
bool displayOn = true;
unsigned long lastInputTime = 0;
const unsigned long SCREEN_TIMEOUT = 30000; // 30 seconds

// Alert Variables
Alert activeAlert;
bool alertActive = false;
const unsigned long ALERT_COOLDOWN = 5000; // 5 seconds

// Data Logging (EEPROM)
const int DATA_BUFFER_SIZE = 10; // Limited by EEPROM size
VitalSigns dataBuffer[DATA_BUFFER_SIZE];
int dataBufferIndex = 0;

// Colors
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_ORANGE    0xFD20

// EEPROM Configuration
#define EEPROM_SIZE 1024
#define EEPROM_DATA_START 0
#define EEPROM_SETTINGS_ADDR 500

// ==================== SETUP FUNCTION ====================
void setup() {
    Serial.begin(9600); // Lower baud rate for Uno stability
    Serial.println("\n=== Cardiac Monitor v1.0 ===");
    Serial.println("Initializing system...");
    
    // Initialize pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    pinMode(BATTERY_PIN, INPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    loadSettings();
    loadDataFromEEPROM();
    
    // Initialize display
    if (!initializeDisplay()) {
        Serial.println("FATAL: Display initialization failed");
        while (true) delay(1000);
    }
    
    showSplashScreen();
    delay(2000);
    
    // Initialize sensor
    if (!initializeSensor()) {
        showError("Sensor Error", "MAX30102 Failed");
        delay(5000);
    }
    
    // System ready
    currentScreen = ScreenType::MAIN;
    showMainScreen();
    
    Serial.println("System initialization complete");
}

// ==================== MAIN LOOP ====================
void loop() {
    unsigned long currentTime = millis();
    
    // Handle button input
    handleButtons();
    
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
    if (currentTime - lastAlertCheck >= 1000) {
        lastAlertCheck = currentTime;
        checkAlerts();
    }
    
    // Handle screen timeout
    handleScreenTimeout();
    
    delay(10);
}

// ==================== DISPLAY FUNCTIONS ====================
bool initializeDisplay() {
    Serial.println("Initializing display...");
    
    tft.begin();
    tft.setRotation(1); // Landscape
    tft.fillScreen(COLOR_BLACK);
    
    Serial.println("Display initialized successfully");
    return true;
}

void showSplashScreen() {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(2);
    
    tft.setCursor(40, 80);
    tft.println("Cardiac Monitor");
    
    tft.setTextSize(1);
    tft.setCursor(80, 120);
    tft.println("v1.0");
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_YELLOW);
    tft.setCursor(10, 200);
    tft.println("Educational Use Only");
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
    
    // Draw vital signs areas
    tft.drawRect(10, 40, 140, 60, COLOR_WHITE);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(15, 45);
    tft.println("Heart Rate (BPM)");
    
    tft.drawRect(170, 40, 140, 60, COLOR_WHITE);
    tft.setCursor(175, 45);
    tft.println("SpO2 (%)");
    
    // Draw buttons
    tft.fillRect(10, 180, 90, 30, COLOR_GRAY);
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(25, 192);
    tft.println("History");
}

void updateVitalSigns() {
    // Update heart rate display
    tft.fillRect(15, 55, 130, 40, COLOR_BLACK);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 60);
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        tft.print((int)currentVitals.heartRate);
    } else {
        tft.print("--");
    }
    
    // Update SpO2 display
    tft.fillRect(175, 55, 130, 40, COLOR_BLACK);
    tft.setTextColor(COLOR_BLUE);
    tft.setCursor(180, 60);
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        tft.print((int)currentVitals.spO2);
    } else {
        tft.print("--");
    }
    
    // Update battery level
    tft.setTextSize(1);
    tft.setTextColor(currentVitals.batteryLevel > 20 ? COLOR_GREEN : COLOR_RED);
    tft.fillRect(250, 10, 60, 10, COLOR_BLACK);
    tft.setCursor(250, 10);
    tft.print((int)currentVitals.batteryLevel);
    tft.println("%");
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
    int count = min(6, dataBufferIndex);
    
    for (int i = max(0, dataBufferIndex - count); i < dataBufferIndex; i++) {
        VitalSigns& data = dataBuffer[i];
        tft.setCursor(10, y);
        tft.print((data.timestamp / 1000) % 86400); // Simplified time
        tft.print("s HR:");
        tft.print((int)data.heartRate);
        tft.print(" SpO2:");
        tft.print((int)data.spO2);
        y += 15;
    }
    
    if (dataBufferIndex == 0) {
        tft.setCursor(10, 60);
        tft.println("No data");
    }
}

void showError(const String& title, const String& message) {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(2);
    tft.setCursor(40, 80);
    tft.println(title);
    
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10, 120);
    tft.println(message);
}

// ==================== SENSOR FUNCTIONS ====================
bool initializeSensor() {
    Serial.println("Initializing MAX30102 sensor...");
    
    Wire.begin();
    
    if (!particleSensor.begin()) {
        Serial.println("MAX30102 not found");
        return false;
    }
    
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    
    Serial.println("MAX30102 initialized successfully");
    return true;
}

void updateSensors() {
    currentVitals.batteryLevel = readBatteryLevel();
    currentVitals.timestamp = millis();
    
    if (particleSensor.available()) {
        redBuffer[bufferIndex] = particleSensor.getRed();
        irBuffer[bufferIndex] = particleSensor.getIR();
        
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
        
        particleSensor.nextSample();
    }
}

float readBatteryLevel() {
    int rawValue = analogRead(BATTERY_PIN);
    float voltage = (rawValue / 1023.0) * 5.0; // 5V reference
    float percentage = ((voltage - 3.0) / 1.2) * 100.0;
    return constrain(percentage, 0, 100);
}

// ==================== BUTTON HANDLING ====================
void handleButtons() {
    static unsigned long lastButtonTime = 0;
    if (millis() - lastButtonTime < 200) return; // Debounce
    
    if (digitalRead(BUTTON1_PIN) == LOW) {
        lastInputTime = millis();
        if (!displayOn) {
            displayOn = true;
            return;
        }
        
        if (currentScreen == ScreenType::MAIN) {
            showHistoryScreen();
        } else if (currentScreen == ScreenType::HISTORY) {
            showMainScreen();
        }
        lastButtonTime = millis();
    }
    
    if (digitalRead(BUTTON2_PIN) == LOW) {
        lastInputTime = millis();
        if (!displayOn) {
            displayOn = true;
            return;
        }
        lastButtonTime = millis();
    }
}

void handleScreenTimeout() {
    if (displayOn && (millis() - lastInputTime > SCREEN_TIMEOUT)) {
        displayOn = false;
        tft.fillScreen(COLOR_BLACK);
    }
}

// ==================== DATA LOGGING ====================
void logData() {
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        if (dataBufferIndex < DATA_BUFFER_SIZE) {
            dataBuffer[dataBufferIndex] = currentVitals;
            dataBufferIndex++;
            saveDataToEEPROM();
        }
    }
}

void saveDataToEEPROM() {
    int addr = EEPROM_DATA_START;
    for (int i = 0; i < dataBufferIndex; i++) {
        VitalSigns& data = dataBuffer[i];
        EEPROM.put(addr, data.heartRate);
        addr += sizeof(float);
        EEPROM.put(addr, data.spO2);
        addr += sizeof(float);
        EEPROM.put(addr, data.batteryLevel);
        addr += sizeof(float);
        EEPROM.put(addr, data.timestamp);
        addr += sizeof(unsigned long);
    }
    EEPROM.commit();
}

void loadDataFromEEPROM() {
    int addr = EEPROM_DATA_START;
    dataBufferIndex = 0;
    while (addr < EEPROM_SETTINGS_ADDR && dataBufferIndex < DATA_BUFFER_SIZE) {
        VitalSigns data;
        EEPROM.get(addr, data.heartRate);
        addr += sizeof(float);
        EEPROM.get(addr, data.spO2);
        addr += sizeof(float);
        EEPROM.get(addr, data.batteryLevel);
        addr += sizeof(float);
        EEPROM.get(addr, data.timestamp);
        addr += sizeof(unsigned long);
        
        if (data.heartRate > 0 && data.spO2 > 0) {
            dataBuffer[dataBufferIndex] = data;
            dataBufferIndex++;
        }
    }
}

// ==================== ALERT SYSTEM ====================
void checkAlerts() {
    if (millis() - lastAlertCheck < ALERT_COOLDOWN) return;
    
    if (currentVitals.isFingerDetected && currentVitals.heartRate > 0) {
        if (currentVitals.heartRate < alertThresholds.heartRateMin || 
            currentVitals.heartRate > alertThresholds.heartRateMax) {
            snprintf(activeAlert.message, 20, "HR: %d BPM", (int)currentVitals.heartRate);
            activeAlert.level = AlertLevel::WARNING;
            activeAlert.timestamp = millis();
            alertActive = true;
            showAlert();
            playAlertSound();
        }
    }
    
    if (currentVitals.isFingerDetected && currentVitals.spO2 > 0) {
        if (currentVitals.spO2 < alertThresholds.spO2Min) {
            snprintf(activeAlert.message, 20, "SpO2: %d%%", (int)currentVitals.spO2);
            activeAlert.level = AlertLevel::WARNING;
            activeAlert.timestamp = millis();
            alertActive = true;
            showAlert();
            playAlertSound();
        }
    }
    
    if (currentVitals.batteryLevel < alertThresholds.batteryMin) {
        snprintf(activeAlert.message, 20, "Bat: %d%%", (int)currentVitals.batteryLevel);
        activeAlert.level = AlertLevel::WARNING;
        activeAlert.timestamp = millis();
        alertActive = true;
        showAlert();
        playAlertSound();
    }
}

void showAlert() {
    uint16_t alertColor = (activeAlert.level == AlertLevel::CRITICAL) ? COLOR_RED : COLOR_YELLOW;
    tft.fillRect(0, 30, 320, 20, alertColor);
    tft.setTextColor(COLOR_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 35);
    tft.println(activeAlert.message);
    
    if (millis() - activeAlert.timestamp > 5000) {
        alertActive = false;
        tft.fillRect(0, 30, 320, 20, COLOR_BLACK);
    }
}

void playAlertSound() {
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }
}

// ==================== SETTINGS MANAGEMENT ====================
void loadSettings() {
    EEPROM.get(EEPROM_SETTINGS_ADDR, alertThresholds);
}

void saveSettings() {
    EEPROM.put(EEPROM_SETTINGS_ADDR, alertThresholds);
    EEPROM.commit();
}

// ==================== DISPLAY UPDATE ====================
void updateDisplay() {
    if (!displayOn) return;
    
    if (currentScreen == ScreenType::MAIN) {
        updateVitalSigns();
    }
    if (alertActive) {
        showAlert();
    }
}

/*
 * USAGE INSTRUCTIONS:
 * 
 * 1. Hardware Setup:
 *    - Connect MAX30102 sensor to I2C pins (A4=SDA, A5=SCL)
 *    - Connect ILI9341 display to SPI pins (10=CS, 9=DC, 8=RST)
 *    - Connect buttons to pins 2 and 3
 *    - Connect buzzer to pin 6
 *    - Connect battery voltage divider to A0
 * 
 * 2. Operation:
 *    - Place finger on MAX30102 sensor
 *    - View heart rate and SpO2 on display
 *    - Press BUTTON1 to toggle between main and history screens
 * 
 * 3. Serial Output:
 *    - Connect at 9600 baud for debug info
 * 
 * SAFETY WARNING:
 * This device is for educational purposes only.
 * Do not use for medical diagnosis or treatment.
 */