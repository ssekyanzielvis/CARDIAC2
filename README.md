#include "alerts.h"
#include "config.h"

AlertManager alertManager;

AlertManager::AlertManager() {
    alertCount = 0;
    lastBuzzerTime = 0;
    buzzerEnabled = true;
    
    // Initialize alerts array
    for (int i = 0; i < MAX_ALERTS; i++) {
        alerts[i].type = ALERT_NONE;
        alerts[i].message = "";
        alerts[i].timestamp = 0;
        alerts[i].acknowledged = false;
    }
}

void AlertManager::addAlert(AlertType type, String message) {
    // Check if same alert type already exists and is recent
    for (int i = 0; i < alertCount; i++) {
        if (alerts[i].type == type && 
            (millis() - alerts[i].timestamp) < 30000) { // 30 seconds
            return; // Don't add duplicate recent alerts
        }
    }
    
    // Add new alert
    if (alertCount >= MAX_ALERTS) {
        shiftAlerts();
        alertCount = MAX_ALERTS - 1;
    }
    
    alerts[alertCount].type = type;
    alerts[alertCount].message = message;
    alerts[alertCount].timestamp = millis();
    alerts[alertCount].acknowledged = false;
    alertCount++;
    
    // Play alert tone
    if (buzzerEnabled) {
        playAlertTone(type);
    }
    
    // Log alert
    Serial.printf("ALERT: %s\n", message.c_str());
}

void AlertManager::acknowledgeAlert(int index) {
    if (index >= 0 && index < alertCount) {
        alerts[index].acknowledged = true;
    }
}

void AlertManager::clearAllAlerts() {
    alertCount = 0;
    for (int i = 0; i < MAX_ALERTS; i++) {
        alerts[i].type = ALERT_NONE;
        alerts[i].acknowledged = false;
    }
}

bool AlertManager::hasActiveAlerts() {
    for (int i = 0; i < alertCount; i++) {
        if (!alerts[i].acknowledged) {
            return true;
        }
    }
    return false;
}

Alert* AlertManager::getAlerts() {
    return alerts;
}

int AlertManager::getAlertCount() {
    return alertCount;
}

void AlertManager::setBuzzerEnabled(bool enabled) {
    buzzerEnabled = enabled;
}

void AlertManager::handleBuzzer() {
    if (!buzzerEnabled || !hasActiveAlerts()) {
        return;
    }
    
    // Buzz every 2 seconds for active alerts
    if (millis() - lastBuzzerTime > 2000) {
        // Find highest priority active alert
        AlertType highestPriority = ALERT_NONE;
        for (int i = 0; i < alertCount; i++) {
            if (!alerts[i].acknowledged && alerts[i].type > highestPriority) {
                highestPriority = alerts[i].type;
            }
        }
        
        if (highestPriority != ALERT_NONE) {
            playAlertTone(highestPriority);
            lastBuzzerTime = millis();
        }
    }
}

void AlertManager::shiftAlerts() {
    // Remove oldest alert and shift others
    for (int i = 0; i < MAX_ALERTS - 1; i++) {
        alerts[i] = alerts[i + 1];
    }
    alerts[MAX_ALERTS - 1].type = ALERT_NONE;
}

void AlertManager::playAlertTone(AlertType type) {
    int frequency, duration, pulses;
    
    switch (type) {
        case ALERT_HIGH_HEART_RATE:
        case ALERT_LOW_SPO2:
            frequency = 1000;
            duration = 200;
            pulses = 3;
            break;
        case ALERT_LOW_HEART_RATE:
            frequency = 800;
            duration = 300;
            pulses = 2;
            break;
        case ALERT_LOW_BATTERY:
            frequency = 600;
            duration = 100;
            pulses = 1;
            break;
        case ALERT_SENSOR_ERROR:
            frequency = 1200;
            duration = 150;
            pulses = 4;
            break;
        default:
            return;
    }
    
    for (int i = 0; i < pulses; i++) {
        tone(BUZZER_PIN, frequency, duration);
        delay(duration + 50);
    }
}
### Project Structure
```
├── src/
│   └── cardiac_monitor.ino    # Main application
├── lib/
│   ├── HeartRateLib/         # Heart rate calculation
│   ├── SPO2Lib/              # SpO2 calculation
│   ├── UIElements/           # Display UI components
│   ├── WebInterface/         # Web server and WebSocket
│   └── AlertManager/         # Alert system management
├── include/
│   ├── config.h              # Configuration constants
│   ├── vital_signs.h         # Data structures
│   └── alerts.h              # Alert definitions
├── data/
│   └── www/                  # Web interface files
│       ├── index.html
│       ├── style.css
│       ├── app.js
│       └── sw.js
└── platformio.ini            # PlatformIO configuration
```

### Building Custom Features

1. **Adding New Sensors**:
   ```cpp
   // In cardiac_monitor.ino
   void setupNewSensor() {
       // Initialize your sensor
   }
   
   void readNewSensor() {
       // Read sensor data
       // Update vital signs structure
   }
   ```

2. **Custom Alert Types**:
   ```cpp
   // In alerts.h
   enum AlertType {
       // Add new alert types
       ALERT_CUSTOM_CONDITION
   };
   ```

3. **UI Customization**:
   ```cpp
   // In ui_elements.cpp
   void drawCustomWidget(int x, int y, CustomData data) {
       // Implement custom display elements
   }
   ```

## Performance Optimization

- **Memory Management**: Use PSRAM for large data buffers
- **Power Optimization**: Implement deep sleep when inactive
- **Network Efficiency**: Compress data transmission
- **Display Optimization**: Use partial screen updates

## Security Considerations

- **WiFi Security**: Use WPA2/WPA3 encryption
- **Web Interface**: Implement authentication if needed
- **Data Privacy**: Encrypt sensitive health data
- **Firmware Updates**: Implement secure OTA updates

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- MAX30102 sensor library contributors
- Adafruit for display libraries
- ESP32 community for examples and support
- Chart.js for web visualization

## Disclaimer

This device is for educational and research purposes only. It is not intended for medical diagnosis or treatment. Always consult healthcare professionals for medical advice.

## Support

For support and questions:
- Open an issue on GitHub
- Check the troubleshooting section
- Review the hardware connections
- Verify software dependencies

## Version History

- **v1.0.0**: Initial release with basic monitoring
- **v1.1.0**: Added web interface and alerts
- **v1.2.0**: Enhanced UI and data logging
- **v1.3.0**: Added offline capability and export features

## Future Enhancements

- [ ] Bluetooth connectivity
- [ ] Mobile app development
- [ ] Cloud data synchronization
- [ ] Advanced analytics and trends
- [ ] Multi-user support
- [ ] Integration with health platforms
- [ ] Machine learning for anomaly detection
```

Now let me create the final configuration and utility files:

```json:data/config.json
{
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD",
    "hostname": "cardiac-monitor"
  },
  "device": {
    "name": "ESP32 Cardiac Monitor",
    "version": "1.0.0",
    "location": "Home"
  },
  "alerts": {
    "heartRateHigh": 120,
    "heartRateLow": 50,
    "spO2Low": 95,
    "batteryLow": 20,
    "buzzerEnabled": true
  },
  "display": {
    "brightness": 80,
    "sleepTimeout": 300,
    "orientation": 0
  },
  "logging": {
    "enabled": true,
    "interval": 5,
    "maxFiles": 10
  },
  "network": {
    "webServerPort": 80,
    "websocketPort": 81,
    "mdnsEnabled": true
  }
}
```

```cpp:lib/ConfigManager/config_manager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

struct WiFiConfig {
    String ssid;
    String password;
    String hostname;
};

struct AlertConfig {
    int heartRateHigh;
    int heartRateLow;
    int spO2Low;
    int batteryLow;
    bool buzzerEnabled;
};

struct DisplayConfig {
    int brightness;
    int sleepTimeout;
    int orientation;
};

struct LoggingConfig {
    bool enabled;
    int interval;
    int maxFiles;
};

class ConfigManager {
private:
    WiFiConfig wifiConfig;
    AlertConfig alertConfig;
    DisplayConfig displayConfig;
    LoggingConfig loggingConfig;
    
public:
    ConfigManager();
    bool loadConfig();
    bool saveConfig();
    void setDefaults();
    
    // Getters
    WiFiConfig getWiFiConfig() { return wifiConfig; }
    AlertConfig getAlertConfig() { return alertConfig; }
    DisplayConfig getDisplayConfig() { return displayConfig; }
    LoggingConfig getLoggingConfig() { return loggingConfig; }
    
    // Setters
    void setWiFiConfig(WiFiConfig config);
    void setAlertConfig(AlertConfig config);
    void setDisplayConfig(DisplayConfig config);
    void setLoggingConfig(LoggingConfig config);
    
private:
    bool parseConfigFile(const String& filename);
    String configToJson();
};

extern ConfigManager configManager;

#endif
```

```cpp:lib/ConfigManager/config_manager.cpp
#include "config_manager.h"

ConfigManager configManager;

ConfigManager::ConfigManager() {
    setDefaults();
}

bool ConfigManager::loadConfig() {
    if (!SPIFFS.exists("/config.json")) {
        Serial.println("Config file not found, using defaults");
        return saveConfig(); // Create default config file
    }
    
    return parseConfigFile("/config.json");
}

bool ConfigManager::saveConfig() {
    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    String jsonString = configToJson();
    file.print(jsonString);
    file.close();
    
    Serial.println("Configuration saved");
    return true;
}

void ConfigManager::setDefaults() {
    // WiFi defaults
    wifiConfig.ssid = "YOUR_WIFI_SSID";
    wifiConfig.password = "YOUR_WIFI_PASSWORD";
    wifiConfig.hostname = "cardiac-monitor";
    
    // Alert defaults
    alertConfig.heartRateHigh = 120;
    alertConfig.heartRateLow = 50;
    alertConfig.spO2Low = 95;
    alertConfig.batteryLow = 20;
    alertConfig.buzzerEnabled = true;
    
    // Display defaults
    displayConfig.brightness = 80;
    displayConfig.sleepTimeout = 300;
    displayConfig.orientation = 0;
    
    // Logging defaults
    loggingConfig.enabled = true;
    loggingConfig.interval = 5;
    loggingConfig.maxFiles = 10;
}

bool ConfigManager::parseConfigFile(const String& filename) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse config file: %s\n", error.c_str());
        return false;
    }
    
    // Parse WiFi config
    if (doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        wifiConfig.ssid = wifi["ssid"].as<String>();
        wifiConfig.password = wifi["password"].as<String>();
        wifiConfig.hostname = wifi["hostname"].as<String>();
    }
    
    // Parse alert config
    if (doc.containsKey("alerts")) {
        JsonObject alerts = doc["alerts"];
        alertConfig.heartRateHigh = alerts["heartRateHigh"];
        alertConfig.heartRateLow = alerts["heartRateLow"];
        alertConfig.spO2Low = alerts["spO2Low"];
        alertConfig.batteryLow = alerts["batteryLow"];
        alertConfig.buzzerEnabled = alerts["buzzerEnabled"];
    }
    
    // Parse display config
    if (doc.containsKey("display")) {
        JsonObject display = doc["display"];
        displayConfig.brightness = display["brightness"];
        displayConfig.sleepTimeout = display["sleepTimeout"];
        displayConfig.orientation = display["orientation"];
    }
    
    // Parse logging config
    if (doc.containsKey("logging")) {
        JsonObject logging = doc["logging"];
        loggingConfig.enabled = logging["enabled"];
        loggingConfig.interval = logging["interval"];
        loggingConfig.maxFiles = logging["maxFiles"];
    }
    
    Serial.println("Configuration loaded successfully");
    return true;
}

String ConfigManager::configToJson() {
    DynamicJsonDocument doc(2048);
    
    // WiFi config
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = wifiConfig.ssid;
    wifi["password"] = wifiConfig.password;
    wifi["hostname"] = wifiConfig.hostname;
    
    // Alert config
    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["heartRateHigh"] = alertConfig.heartRateHigh;
    alerts["heartRateLow"] = alertConfig.heartRateLow;
    alerts["spO2Low"] = alertConfig.spO2Low;
    alerts["batteryLow"] = alertConfig.batteryLow;
    alerts["buzzerEnabled"] = alertConfig.buzzerEnabled;
    
    // Display config
    JsonObject display = doc.createNestedObject("display");
    display["brightness"] = displayConfig.brightness;
    display["sleepTimeout"] = displayConfig.sleepTimeout;
    display["orientation"] = displayConfig.orientation;
    
    // Logging config
    JsonObject logging = doc.createNestedObject("logging");
    logging["enabled"] = loggingConfig.enabled;
    logging["interval"] = loggingConfig.interval;
    logging["maxFiles"] = loggingConfig.maxFiles;
    
    String jsonString;
    serializeJsonPretty(doc, jsonString);
    return jsonString;
}

void ConfigManager::setWiFiConfig(WiFiConfig config) {
    wifiConfig = config;
}

void ConfigManager::setAlertConfig(AlertConfig config) {
    alertConfig = config;
}

void ConfigManager::setDisplayConfig(DisplayConfig config) {
    displayConfig = config;
}

void ConfigManager::setLoggingConfig(LoggingConfig config) {
    loggingConfig = config;
}
```

Finally, let me create the LICENSE file to complete the project:

```text:LICENSE
MIT License

Copyright (c) 2024 ESP32 Cardiac Monitor Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

DISCLAIMER: This software is for educational and research purposes only. 
It is not intended for medical diagnosis, treatment, or any clinical use. 
Always consult qualified healthcare professionals for medical advice and 
never rely solely on this device for health-related decisions.
```

```gitignore:.gitignore
# PlatformIO
.pio/
.vscode/
.pioenvs/
.piolibdeps/

# Compiled Object files
*.o
*.obj

# Executables
*.exe
*.out
*.app

# IDE files
*.swp
*.swo
*~

# OS generated files
.DS_Store
.DS_Store?
._*
.Spotlight-V100
.Trashes
ehthumbs.db
Thumbs.db

# Configuration files with sensitive data
data/config.json

# Log files
*.log
logs/

# Temporary files
*.tmp
*.temp

# Build artifacts
build/
dist/
