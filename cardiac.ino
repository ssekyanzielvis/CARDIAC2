#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <time.h>

// Display pins
#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   2
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_MISO  19

// Touch pins
#define TOUCH_CS  15
#define TOUCH_IRQ 32

// Sensor pins
#define MAX30102_INT 33
#define BUZZER_PIN   27
#define ECG_PIN      34

// Battery monitoring
#define BATTERY_PIN  35

// Display and touch objects
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Sensor objects
MAX30105 particleSensor;

// System variables
struct VitalSigns {
  float heartRate;
  float spO2;
  float ecgValue;
  float batteryLevel;
  bool isFingerDetected;
  unsigned long timestamp;
};

VitalSigns currentVitals;
VitalSigns alertThresholds = {100, 95, 0, 20, false, 0}; // Default thresholds

// UI State Management
enum UIState {
  MAIN_SCREEN,
  SETTINGS_SCREEN,
  HISTORY_SCREEN,
  ALERT_SCREEN
};

UIState currentState = MAIN_SCREEN;

// Data buffers for real-time plotting
#define BUFFER_SIZE 320
float heartRateBuffer[BUFFER_SIZE];
float ecgBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// Touch calibration
#define TOUCH_CALIBRATION_X1 200
#define TOUCH_CALIBRATION_X2 3700
#define TOUCH_CALIBRATION_Y1 200
#define TOUCH_CALIBRATION_Y2 3700

// Colors
#define COLOR_BG        0x0000
#define COLOR_TEXT      0xFFFF
#define COLOR_ACCENT    0x07E0
#define COLOR_WARNING   0xFFE0
#define COLOR_DANGER    0xF800
#define COLOR_BUTTON    0x4208
#define COLOR_BUTTON_PRESSED 0x2104

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }
  
  // Initialize display
  tft.begin();
  tft.setRotation(1); // Landscape
  tft.fillScreen(COLOR_BG);
  
  // Initialize touch
  touch.begin();
  touch.setRotation(1);
  
  // Initialize sensors
  initializeSensors();
  
  // Initialize WiFi for time sync
  initializeWiFi();
  
  // Configure time
  configTime(0, 0, "pool.ntp.org");
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ECG_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  
  // Load settings
  loadSettings();
  
  // Show splash screen
  showSplashScreen();
  delay(2000);
  
  // Initialize main screen
  drawMainScreen();
  
  Serial.println("Cardiac Monitor Initialized");
}

void loop() {
  // Read sensors
  readSensors();
  
  // Handle touch input
  handleTouch();
  
  // Update display based on current state
  updateDisplay();
  
  // Check for alerts
  checkAlerts();
  
  // Log data periodically
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) { // Log every 5 seconds
    logVitalSigns();
    lastLog = millis();
  }
  
  delay(50); // Main loop delay
}

void initializeSensors() {
  // Initialize MAX30102
  if (!particleSensor.begin()) {
    Serial.println("MAX30102 not found");
    showError("Sensor Error", "MAX30102 not detected");
    return;
  }
  
  // Configure MAX30102
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  particleSensor.setPulseAmplitudeIR(0x0A);
  
  Serial.println("MAX30102 initialized");
}

void initializeWiFi() {
  // Add your WiFi credentials here
  const char* ssid = "YOUR_WIFI_SSID";
  const char* password = "YOUR_WIFI_PASSWORD";
  
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void readSensors() {
  static unsigned long lastReading = 0;
  if (millis() - lastReading < 100) return; // Read every 100ms
  
  // Read MAX30102
  if (particleSensor.available()) {
    uint32_t irValue = particleSensor.getIR();
    
    if (checkForBeat(irValue)) {
      long delta = millis() - lastReading;
      lastReading = millis();
      
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        // Store valid heart rate
        currentVitals.heartRate = beatsPerMinute;
        currentVitals.isFingerDetected = true;
        
        // Add to buffer for plotting
        heartRateBuffer[bufferIndex] = beatsPerMinute;
      }
    }
    
    // Calculate SpO2 (simplified)
    uint32_t redValue = particleSensor.getRed();
    if (irValue > 50000 && redValue > 50000) {
      float ratio = (float)redValue / (float)irValue;
      currentVitals.spO2 = 110 - 25 * ratio; // Simplified calculation
      if (currentVitals.spO2 > 100) currentVitals.spO2 = 100;
      if (currentVitals.spO2 < 70) currentVitals.spO2 = 70;
    }
    
    particleSensor.nextSample();
  } else {
    currentVitals.isFingerDetected = false;
  }
  
  // Read ECG (simulated with analog noise for demo)
  int ecgRaw = analogRead(ECG_PIN);
  currentVitals.ecgValue = (ecgRaw / 4095.0) * 3.3; // Convert to voltage
  ecgBuffer[bufferIndex] = currentVitals.ecgValue;
  
  // Read battery level
  int batteryRaw = analogRead(BATTERY_PIN);
  currentVitals.batteryLevel = map(batteryRaw, 0, 4095, 0, 100);
  
  // Update buffer index
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  
  // Update timestamp
  currentVitals.timestamp = millis();
}

void handleTouch() {
  if (!touch.touched()) return;
  
  TS_Point p = touch.getPoint();
  
  // Map touch coordinates to screen coordinates
  int x = map(p.x, TOUCH_CALIBRATION_X1, TOUCH_CALIBRATION_X2, 0, 320);
  int y = map(p.y, TOUCH_CALIBRATION_Y1, TOUCH_CALIBRATION_Y2, 0, 240);
  
  // Constrain to screen bounds
  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);
  
  handleTouchInput(x, y);
  
  // Debounce
  delay(200);
}

void handleTouchInput(int x, int y) {
  switch (currentState) {
    case MAIN_SCREEN:
      handleMainScreenTouch(x, y);
      break;
    case SETTINGS_SCREEN:
      handleSettingsScreenTouch(x, y);
      break;
    case HISTORY_SCREEN:
      handleHistoryScreenTouch(x, y);
      break;
    case ALERT_SCREEN:
      handleAlertScreenTouch(x, y);
      break;
  }
}

void handleMainScreenTouch(int x, int y) {
  // Settings button (top right)
  if (x > 270 && x < 310 && y > 10 && y < 40) {
    currentState = SETTINGS_SCREEN;
    drawSettingsScreen();
    return;
  }
  
  // History button (bottom left)
  if (x > 10 && x < 80 && y > 200 && y < 230) {
    currentState = HISTORY_SCREEN;
    drawHistoryScreen();
    return;
  }
  
  // Alert acknowledge button (if alert is active)
  if (isAlertActive() && x > 120 && x < 200 && y > 100 && y < 140) {
    acknowledgeAlert();
  }
}

void handleSettingsScreenTouch(int x, int y) {
  // Back button
  if (x > 10 && x < 60 && y > 10 && y < 40) {
    currentState = MAIN_SCREEN;
    drawMainScreen();
    return;
  }
  
  // Heart rate threshold adjustment
  if (y > 60 && y < 90) {
    if (x > 200 && x < 230) { // Increase
      alertThresholds.heartRate += 5;
      if (alertThresholds.heartRate > 200) alertThresholds.heartRate = 200;
    } else if (x > 170 && x < 200) { // Decrease
      alertThresholds.heartRate -= 5;
      if (alertThresholds.heartRate < 40) alertThresholds.heartRate = 40;
    }
    saveSettings();
    drawSettingsScreen();
  }
  
  // SpO2 threshold adjustment
  if (y > 100 && y < 130) {
    if (x > 200 && x < 230) { // Increase
      alertThresholds.spO2 += 1;
      if (alertThresholds.spO2 > 100) alertThresholds.spO2 = 100;
    } else if (x > 170 && x < 200) { // Decrease
      alertThresholds.spO2 -= 1;
      if (alertThresholds.spO2 < 80) alertThresholds.spO2 = 80;
    }
    saveSettings();
    drawSettingsScreen();
  }
}

void handleHistoryScreenTouch(int x, int y) {
  // Back button
  if (x > 10 && x < 60 && y > 10 && y < 40) {
    currentState = MAIN_SCREEN;
    drawMainScreen();
  }
}

void handleAlertScreenTouch(int x, int y) {
  // Acknowledge button
  if (x > 120 && x < 200 && y > 150 && y < 180) {
    acknowledgeAlert();
    currentState = MAIN_SCREEN;
    drawMainScreen();
  }
}

void updateDisplay() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return; // Update every second
  
  switch (currentState) {
    case MAIN_SCREEN:
      updateMainScreen();
      break;
    case SETTINGS_SCREEN:
      // Settings screen is static, no updates needed
      break;
    case HISTORY_SCREEN:
      updateHistoryScreen();
      break;
    case ALERT_SCREEN:
      updateAlertScreen();
      break;
  }
  
  lastUpdate = millis();
}

void checkAlerts() {
  static bool alertActive = false;
  static unsigned long lastAlertCheck = 0;
  
  if (millis() - lastAlertCheck < 2000) return; // Check every 2 seconds
  
  bool shouldAlert = false;
  String alertMessage = "";
  
  if (currentVitals.isFingerDetected) {
    if (currentVitals.heartRate > alertThresholds.heartRate + 20 || 
        currentVitals.heartRate < alertThresholds.heartRate - 20) {
      shouldAlert = true;
      alertMessage = "Heart Rate Alert";
    }
    
    if (currentVitals.spO2 < alertThresholds.spO2) {
      shouldAlert = true;
      alertMessage = "Low SpO2 Alert";
    }
  }
  
  if (currentVitals.batteryLevel < alertThresholds.batteryLevel) {
    shouldAlert = true;
    alertMessage = "Low Battery";
  }
  
  if (shouldAlert && !alertActive) {
    triggerAlert(alertMessage);
    alertActive = true;
  } else if (!shouldAlert) {
    alertActive = false;
  }
  
  lastAlertCheck = millis();
}

void triggerAlert(String message) {
  // Sound buzzer
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
  
  // Show alert screen
  currentState = ALERT_SCREEN;
  drawAlertScreen(message);
}

bool isAlertActive() {
  return currentState == ALERT_SCREEN;
}

void acknowledgeAlert() {
  digitalWrite(BUZZER_PIN, LOW);
  // Alert acknowledged - could log this event
}

void logVitalSigns() {
  if (!SPIFFS.exists("/logs")) {
    return;
  }
  
  File logFile = SPIFFS.open("/logs/vitals.json", "a");
  if (!logFile) {
    Serial.println("Failed to open log file");
    return;
  }
  // Create JSON object for logging
  DynamicJsonDocument doc(200);
  doc["timestamp"] = currentVitals.timestamp;
  doc["heartRate"] = currentVitals.heartRate;
  doc["spO2"] = currentVitals.spO2;
  doc["ecgValue"] = currentVitals.ecgValue;
  doc["batteryLevel"] = currentVitals.batteryLevel;
  doc["fingerDetected"] = currentVitals.isFingerDetected;
  
  String jsonString;
  serializeJson(doc, jsonString);
  logFile.println(jsonString);
  logFile.close();
  
  Serial.println("Vitals logged: " + jsonString);
}

void loadSettings() {
  if (!SPIFFS.exists("/config.json")) {
    saveSettings(); // Create default settings
    return;
  }
  
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }
  
  alertThresholds.heartRate = doc["heartRateThreshold"] | 100;
  alertThresholds.spO2 = doc["spO2Threshold"] | 95;
  alertThresholds.batteryLevel = doc["batteryThreshold"] | 20;
  
  Serial.println("Settings loaded");
}

void saveSettings() {
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to create config file");
    return;
  }
  
  DynamicJsonDocument doc(512);
  doc["heartRateThreshold"] = alertThresholds.heartRate;
  doc["spO2Threshold"] = alertThresholds.spO2;
  doc["batteryThreshold"] = alertThresholds.batteryLevel;
  
  serializeJson(doc, configFile);
  configFile.close();
  
  Serial.println("Settings saved");
}

// ==================== UI DRAWING FUNCTIONS ====================

void showSplashScreen() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextSize(3);
  tft.setCursor(50, 80);
  tft.println("CARDIAC");
  tft.setCursor(60, 120);
  tft.println("MONITOR");
  
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(100, 180);
  tft.println("Initializing...");
  
  // Draw heart icon
  drawHeartIcon(280, 200, COLOR_DANGER);
}

void drawMainScreen() {
  tft.fillScreen(COLOR_BG);
  
  // Title bar
  tft.fillRect(0, 0, 320, 30, COLOR_ACCENT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.println("Cardiac Monitor");
  
  // Settings button
  drawButton(270, 5, 40, 20, "SET", COLOR_BUTTON, COLOR_TEXT);
  
  // Main vitals display area
  drawVitalsDisplay();
  
  // Waveform area
  drawWaveformArea();
  
  // Bottom buttons
  drawButton(10, 200, 70, 30, "HISTORY", COLOR_BUTTON, COLOR_TEXT);
  drawButton(240, 200, 70, 30, "ALERTS", COLOR_BUTTON, COLOR_TEXT);
  
  // Battery indicator
  drawBatteryIndicator();
  
  // Connection status
  drawConnectionStatus();
}

void drawVitalsDisplay() {
  // Heart Rate
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("Heart Rate (BPM)");
  
  tft.setTextSize(3);
  tft.setCursor(10, 55);
  if (currentVitals.isFingerDetected) {
    tft.setTextColor(COLOR_ACCENT);
    tft.printf("%.0f", currentVitals.heartRate);
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("---");
  }
  
  // SpO2
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(160, 40);
  tft.println("SpO2 (%)");
  
  tft.setTextSize(3);
  tft.setCursor(160, 55);
  if (currentVitals.isFingerDetected) {
    if (currentVitals.spO2 >= 95) {
      tft.setTextColor(COLOR_ACCENT);
    } else if (currentVitals.spO2 >= 90) {
      tft.setTextColor(COLOR_WARNING);
    } else {
      tft.setTextColor(COLOR_DANGER);
    }
    tft.printf("%.0f", currentVitals.spO2);
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("--");
  }
  
  // Finger detection indicator
  tft.setTextSize(1);
  tft.setCursor(10, 85);
  if (currentVitals.isFingerDetected) {
    tft.setTextColor(COLOR_ACCENT);
    tft.println("Finger Detected");
    drawHeartIcon(150, 85, COLOR_DANGER);
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("Place finger on sensor");
  }
}

void drawWaveformArea() {
  // ECG Waveform
  tft.drawRect(10, 100, 300, 80, COLOR_TEXT);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(15, 105);
  tft.println("ECG Waveform");
  
  // Draw ECG waveform
  drawECGWaveform(15, 115, 290, 60);
  
  // Heart rate trend (mini graph)
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(15, 185);
  tft.println("HR Trend:");
  drawHeartRateTrend(80, 185, 150, 10);
}

void drawECGWaveform(int x, int y, int width, int height) {
  // Clear waveform area
  tft.fillRect(x, y, width, height, COLOR_BG);
  
  // Draw grid
  tft.setTextColor(0x2104); // Dark gray
  for (int i = 0; i < width; i += 20) {
    tft.drawFastVLine(x + i, y, height, 0x2104);
  }
  for (int i = 0; i < height; i += 10) {
    tft.drawFastHLine(x, y + i, width, 0x2104);
  }
  
  // Draw ECG waveform
  tft.setTextColor(COLOR_ACCENT);
  int centerY = y + height / 2;
  
  for (int i = 1; i < width && i < BUFFER_SIZE; i++) {
    int prevIndex = (bufferIndex - width + i - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    int currIndex = (bufferIndex - width + i + BUFFER_SIZE) % BUFFER_SIZE;
    
    int y1 = centerY - (int)(ecgBuffer[prevIndex] * height / 6.6); // Scale for 3.3V range
    int y2 = centerY - (int)(ecgBuffer[currIndex] * height / 6.6);
    
    y1 = constrain(y1, y, y + height - 1);
    y2 = constrain(y2, y, y + height - 1);
    
    tft.drawLine(x + i - 1, y1, x + i, y2, COLOR_ACCENT);
  }
}

void drawHeartRateTrend(int x, int y, int width, int height) {
  // Draw mini heart rate trend
  tft.setTextColor(COLOR_DANGER);
  
  for (int i = 1; i < width && i < BUFFER_SIZE; i++) {
    int prevIndex = (bufferIndex - width + i - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    int currIndex = (bufferIndex - width + i + BUFFER_SIZE) % BUFFER_SIZE;
    
    if (heartRateBuffer[prevIndex] > 0 && heartRateBuffer[currIndex] > 0) {
      int y1 = y + height - (int)(heartRateBuffer[prevIndex] * height / 200); // Scale for 0-200 BPM
      int y2 = y + height - (int)(heartRateBuffer[currIndex] * height / 200);
      
      y1 = constrain(y1, y, y + height);
      y2 = constrain(y2, y, y + height);
      
      tft.drawLine(x + i - 1, y1, x + i, y2, COLOR_DANGER);
    }
  }
}

void drawSettingsScreen() {
  tft.fillScreen(COLOR_BG);
  
  // Title bar
  tft.fillRect(0, 0, 320, 30, COLOR_ACCENT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.println("Settings");
  
  // Back button
  drawButton(270, 5, 40, 20, "BACK", COLOR_BUTTON, COLOR_TEXT);
  
  // Heart Rate Threshold
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.println("Heart Rate Alert Threshold:");
  tft.setTextSize(2);
  tft.setCursor(10, 65);
  tft.printf("%.0f BPM", alertThresholds.heartRate);
  
  drawButton(170, 60, 25, 25, "-", COLOR_BUTTON, COLOR_TEXT);
  drawButton(200, 60, 25, 25, "+", COLOR_BUTTON, COLOR_TEXT);
  
  // SpO2 Threshold
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 100);
  tft.println("SpO2 Alert Threshold:");
  tft.setTextSize(2);
  tft.setCursor(10, 115);
  tft.printf("%.0f%%", alertThresholds.spO2);
  
  drawButton(170, 110, 25, 25, "-", COLOR_BUTTON, COLOR_TEXT);
  drawButton(200, 110, 25, 25, "+", COLOR_BUTTON, COLOR_TEXT);
  
  // Battery Threshold
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 150);
  tft.println("Battery Alert Threshold:");
  tft.setTextSize(2);
  tft.setCursor(10, 165);
  tft.printf("%.0f%%", alertThresholds.batteryLevel);
  
  drawButton(170, 160, 25, 25, "-", COLOR_BUTTON, COLOR_TEXT);
  drawButton(200, 160, 25, 25, "+", COLOR_BUTTON, COLOR_TEXT);
  
  // WiFi Status
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(10, 200);
  tft.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(COLOR_ACCENT);
    tft.println("Connected");
  } else {
    tft.setTextColor(COLOR_DANGER);
    tft.println("Disconnected");
  }
}

void drawHistoryScreen() {
  tft.fillScreen(COLOR_BG);
  
  // Title bar
  tft.fillRect(0, 0, 320, 30, COLOR_ACCENT);
  tft.setTextColor(COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  tft.println("History");
  
  // Back button
  drawButton(270, 5, 40, 20, "BACK", COLOR_BUTTON, COLOR_TEXT);
  
  // Display recent readings
  displayRecentReadings();
}

void drawAlertScreen(String message) {
  tft.fillScreen(COLOR_DANGER);
  
  // Alert title
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(3);
  tft.setCursor(80, 50);
  tft.println("ALERT!");
  
  // Alert message
  tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.println(message);
  
  // Current values
  tft.setTextSize(1);
  tft.setCursor(20, 130);
  tft.printf("HR: %.0f BPM", currentVitals.heartRate);
  tft.setCursor(20, 145);
  tft.printf("SpO2: %.0f%%", currentVitals.spO2);
  tft.setCursor(20, 160);
  tft.printf("Battery: %.0f%%", currentVitals.batteryLevel);
  
  // Acknowledge button
  drawButton(120, 180, 80, 30, "ACK", COLOR_TEXT, COLOR_DANGER);
  
  // Flashing effect
  static bool flashState = false;
  static unsigned long lastFlash = 0;
  if (millis() - lastFlash > 500) {
    flashState = !flashState;
    lastFlash = millis();
    
    if (flashState) {
      tft.fillRect(0, 0, 320, 10, COLOR_TEXT);
      tft.fillRect(0, 230, 320, 10, COLOR_TEXT);
    } else {
      tft.fillRect(0, 0, 320, 10, COLOR_DANGER);
      tft.fillRect(0, 230, 320, 10, COLOR_DANGER);
    }
  }
}

void updateMainScreen() {
  // Update only the vital signs values to avoid flickering
  
  // Clear and redraw heart rate
  tft.fillRect(10, 55, 140, 25, COLOR_BG);
  tft.setTextSize(3);
  tft.setCursor(10, 55);
  if (currentVitals.isFingerDetected) {
    tft.setTextColor(COLOR_ACCENT);
    tft.printf("%.0f", currentVitals.heartRate);
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("---");
  }
  
  // Clear and redraw SpO2
  tft.fillRect(160, 55, 140, 25, COLOR_BG);
  tft.setTextSize(3);
  tft.setCursor(160, 55);
  if (currentVitals.isFingerDetected) {
    if (currentVitals.spO2 >= 95) {
      tft.setTextColor(COLOR_ACCENT);
    } else if (currentVitals.spO2 >= 90) {
      tft.setTextColor(COLOR_WARNING);
    } else {
      tft.setTextColor(COLOR_DANGER);
    }
    tft.printf("%.0f", currentVitals.spO2);
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("--");
  }
  
  // Update finger detection status
  tft.fillRect(10, 85, 200, 10, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(10, 85);
  if (currentVitals.isFingerDetected) {
    tft.setTextColor(COLOR_ACCENT);
    tft.println("Finger Detected");
  } else {
    tft.setTextColor(COLOR_WARNING);
    tft.println("Place finger on sensor");
  }
  
  // Update waveforms
  drawECGWaveform(15, 115, 290, 60);
  drawHeartRateTrend(80, 185, 150, 10);
  
  // Update battery indicator
  drawBatteryIndicator();
  
  // Update connection status
  drawConnectionStatus();
}

void updateHistoryScreen() {
  // Refresh history data
  displayRecentReadings();
}

void updateAlertScreen() {
  // Keep alert screen flashing and updated
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    // Update current values on alert screen
    tft.fillRect(20, 130, 200, 45, COLOR_DANGER);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(20, 130);
    tft.printf("HR: %.0f BPM", currentVitals.heartRate);
    tft.setCursor(20, 145);
    tft.printf("SpO2: %.0f%%", currentVitals.spO2);
    tft.setCursor(20, 160);
    tft.printf("Battery: %.0f%%", currentVitals.batteryLevel);
    lastUpdate = millis();
  }
}

void drawButton(int x, int y, int w, int h, String text, uint16_t bgColor, uint16_t textColor) {
  tft.fillRoundRect(x, y, w, h, 3, bgColor);
  tft.drawRoundRect(x, y, w, h, 3, textColor);
  
  // Center text in button
  int textWidth = text.length() * 6; // Approximate width for size 1 text
  int textHeight = 8;
  
  tft.setTextColor(textColor);
  tft.setTextSize(1);
  tft.setCursor(x + (w - textWidth) / 2, y + (h - textHeight) / 2);
  tft.println(text);
}

void drawBatteryIndicator() {
  int batteryX = 250;
  int batteryY = 35;
  int batteryW = 30;
  int batteryH = 15;
  
  // Battery outline
  tft.drawRect(batteryX, batteryY, batteryW, batteryH, COLOR_TEXT);
  tft.fillRect(batteryX + batteryW, batteryY + 3, 3, batteryH - 6, COLOR_TEXT);
  
  // Battery fill based on level
  int fillWidth = (int)((currentVitals.batteryLevel / 100.0) * (batteryW - 2));
  uint16_t fillColor;
  
  if (currentVitals.batteryLevel > 50) {
    fillColor = COLOR_ACCENT;
  } else if (currentVitals.batteryLevel > 20) {
    fillColor = COLOR_WARNING;
  } else {
    fillColor = COLOR_DANGER;
  }
  
  tft.fillRect(batteryX + 1, batteryY + 1, fillWidth, batteryH - 2, fillColor);
  
  // Battery percentage text
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(1);
  tft.setCursor(batteryX - 20, batteryY + 3);
  tft.printf("%.0f%%", currentVitals.batteryLevel);
}

void drawConnectionStatus() {
  int statusX = 290;
  int statusY = 35;
  
  if (WiFi.status() == WL_CONNECTED) {
    // Draw WiFi icon (simplified)
    tft.fillCircle(statusX, statusY, 3, COLOR_ACCENT);
    tft.drawCircle(statusX, statusY, 6, COLOR_ACCENT);
    tft.drawCircle(statusX, statusY, 9, COLOR_ACCENT);
  } else {
    // Draw disconnected icon
    tft.drawLine(statusX - 5, statusY - 5, statusX + 5, statusY + 5, COLOR_DANGER);
    tft.drawLine(statusX - 5, statusY + 5, statusX + 5, statusY - 5, COLOR_DANGER);
  }
}

void drawHeartIcon(int x, int y, uint16_t color) {
  // Simple heart shape using filled circles and triangle
  tft.fillCircle(x - 3, y - 2, 3, color);
  tft.fillCircle(x + 3, y - 2, 3, color);
  tft.fillTriangle(x - 6, y, x + 6, y, x, y + 6, color);
}

void displayRecentReadings() {
  // Clear content area
  tft.fillRect(0, 35, 320, 200, COLOR_BG);
  
  // Read recent log entries
  if (!SPIFFS.exists("/logs/vitals.json")) {
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(10, 50);
    tft.println("No history data available");
    return;
  }
  
  File logFile = SPIFFS.open("/logs/vitals.json", "r");
  if (!logFile) {
    tft.setTextColor(COLOR_DANGER);
    tft.setTextSize(1);
    tft.setCursor(10, 50);
    tft.println("Error reading history");
    return;
  }
  
  // Display headers
  tft.setTextColor(COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("Time    HR   SpO2  Battery");
  tft.drawLine(10, 50, 310, 50, COLOR_ACCENT);
  
  // Read and display last few entries
  String lines[10];
  int lineCount = 0;
  
  // Read file into array (simplified - in production, you'd want to read from end)
  while (logFile.available() && lineCount < 10) {
    lines[lineCount] = logFile.readStringUntil('\n');
    lineCount++;
  }
  logFile.close();
  
  // Display entries
  tft.setTextColor(COLOR_TEXT);
  for (int i = max(0, lineCount - 8); i < lineCount; i++) {
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, lines[i]);
    
    if (!error) {
      int yPos = 60 + (i - max(0, lineCount - 8)) * 15;
      
      // Format timestamp (simplified)
      unsigned long timestamp = doc["timestamp"];
      int seconds = (timestamp / 1000) % 60;
      int minutes = (timestamp / 60000) % 60;
      int hours = (timestamp / 3600000) % 24;
      
      tft.setCursor(10, yPos);
      tft.printf("%02d:%02d:%02d", hours, minutes, seconds);
      
      tft.setCursor(70, yPos);
      tft.printf("%.0f", (float)doc["heartRate"]);
      
      tft.setCursor(100, yPos);
      tft.printf("%.0f", (float)doc["spO2"]);
      
      tft.setCursor(130, yPos);
      tft.printf("%.0f%%", (float)doc["batteryLevel"]);
      
      // Status indicator
      bool fingerDetected = doc["fingerDetected"];
      tft.fillCircle(170, yPos + 4, 3, fingerDetected ? COLOR_ACCENT : COLOR_WARNING);
    }
  }
}

void showError(String title, String message) {
  tft.fillScreen(COLOR_DANGER);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(20, 80);
  tft.println(title);
  
  tft.setTextSize(1);
  tft.setCursor(20, 120);
  tft.println(message);
  
  tft.setCursor(20, 180);
  tft.println("Please restart the device");
}

// ==================== HELPER FUNCTIONS ====================

// Heart rate detection variables
const byte RATE_ARRAY_SIZE = 4;
byte rateArray[RATE_ARRAY_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
int beatsPerMinute;

bool checkForBeat(long sample) {
  static long lastSample = 0;
  static long threshold = 512;
  static bool beatDetected = false;
  static unsigned long lastBeatTime = 0;
  
  // Simple peak detection algorithm
  if (sample > threshold && !beatDetected && (millis() - lastBeatTime) > 300) {
    beatDetected = true;
    lastBeatTime = millis();
    
    // Calculate time between beats
    long delta = millis() - lastBeat;
    lastBeat = millis();
    
    // Store valid beat intervals
    if (delta > 300 && delta < 3000) { // Valid heart rate range
      rateArray[rateSpot++] = (byte)(60000 / delta);
      rateSpot %= RATE_ARRAY_SIZE;
      
      // Calculate average
      long total = 0;
      for (byte i = 0; i < RATE_ARRAY_SIZE; i++) {
        total += rateArray[i];
      }
      beatsPerMinute = total / RATE_ARRAY_SIZE;
    }
    
    return true;
  } else if (sample < threshold - 50) {
    beatDetected = false;
  }
  
  // Adaptive threshold
  threshold = (threshold * 15 + sample) / 16;
  
  lastSample = sample;
  return false;
}

// ==================== ADDITIONAL UTILITY FUNCTIONS ====================

void performSystemCheck() {
  Serial.println("Performing system check...");
  
  // Check SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed");
    showError("Storage Error", "Failed to initialize storage");
    return;
  }
  
  // Create logs directory if it doesn't exist
  if (!SPIFFS.exists("/logs")) {
    File dir = SPIFFS.open("/logs", "w");
    dir.close();
  }
  
  // Check sensor
  if (!particleSensor.begin()) {
    Serial.println("MAX30102 sensor check failed");
    showError("Sensor Error", "Heart rate sensor not detected");
    return;
  }
  
  // Check display
  // Display check is implicit - if we can draw, it's working
  
  // Check touch
  if (!touch.begin()) {
    Serial.println("Touch screen check failed");
    showError("Touch Error", "Touch screen not responding");
    return;
  }
  
  Serial.println("System check completed successfully");
}

void enterSleepMode() {
  // Save current state
  saveSettings();
  
  // Turn off display backlight
  // Note: This would require additional hardware control
  
  // Configure wake-up sources
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 0); // Touch interrupt
  esp_sleep_enable_timer_wakeup(30 * 1000000); // Wake every 30 seconds
  
  Serial.println("Entering sleep mode");
  esp_deep_sleep_start();
}

void handleLowBattery() {
  if (currentVitals.batteryLevel < 5) {
    // Critical battery - save data and shutdown
    saveSettings();
    
    tft.fillScreen(COLOR_DANGER);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(50, 100);
    tft.println("CRITICAL");
    tft.setCursor(60, 130);
    tft.println("BATTERY");
    
    delay(3000);
    enterSleepMode();
  } else if (currentVitals.batteryLevel < 15) {
    // Low battery warning
    static unsigned long lastWarning = 0;
    if (millis() - lastWarning > 60000) { // Warn every minute
      triggerAlert("Low Battery Warning");
      lastWarning = millis();
    }
  }
}

void calibrateTouch() {
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(50, 100);
  tft.println("Touch Calibration");
  
  // This would implement a proper touch calibration routine
  // For now, we use predefined values
  
  delay(2000);
}

// ==================== MAIN LOOP ENHANCEMENTS ====================

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      printSystemStatus();
    } else if (command == "reset") {
      ESP.restart();
    } else if (command == "calibrate") {
      calibrateTouch();
    } else if (command == "sleep") {
      enterSleepMode();
    } else if (command == "logs") {
      printRecentLogs();
    } else {
      Serial.println("Unknown command. Available: status, reset, calibrate, sleep, logs");
    }
  }
}

void printSystemStatus() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.printf("Heart Rate: %.1f BPM\n", currentVitals.heartRate);
  Serial.printf("SpO2: %.1f%%\n", currentVitals.spO2);
  Serial.printf("Battery:
  Serial.printf("Battery: %.1f%%\n", currentVitals.batteryLevel);
  Serial.printf("Finger Detected: %s\n", currentVitals.isFingerDetected ? "Yes" : "No");
  Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
  Serial.println("=====================");
}

void printRecentLogs() {
  Serial.println("=== RECENT LOGS ===");
  
  if (!SPIFFS.exists("/logs/vitals.json")) {
    Serial.println("No log file found");
    return;
  }
  
  File logFile = SPIFFS.open("/logs/vitals.json", "r");
  if (!logFile) {
    Serial.println("Error opening log file");
    return;
  }
  
  // Print last 10 entries
  String lines[10];
  int lineCount = 0;
  
  while (logFile.available() && lineCount < 10) {
    lines[lineCount] = logFile.readStringUntil('\n');
    lineCount++;
  }
  logFile.close();
  
  for (int i = max(0, lineCount - 10); i < lineCount; i++) {
    Serial.println(lines[i]);
  }
  Serial.println("==================");
}

// Enhanced loop function with additional features
void loop() {
  // Handle serial commands for debugging
  handleSerialCommands();
  
  // Read sensors
  readSensors();
  
  // Handle touch input
  handleTouch();
  
  // Update display based on current state
  updateDisplay();
  
  // Check for alerts
  checkAlerts();
  
  // Handle low battery conditions
  handleLowBattery();
  
  // Log data periodically
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) { // Log every 5 seconds
    logVitalSigns();
    lastLog = millis();
  }
  
  // Watchdog timer reset (if implemented)
  // esp_task_wdt_reset();
  
  delay(50); // Main loop delay
}
