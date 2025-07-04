#include "alert.h"
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
