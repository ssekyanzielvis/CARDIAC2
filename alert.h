#ifndef ALERTS_H
#define ALERTS_H

#include <Arduino.h>

enum AlertType {
    ALERT_NONE,
    ALERT_HIGH_HEART_RATE,
    ALERT_LOW_HEART_RATE,
    ALERT_LOW_SPO2,
    ALERT_LOW_BATTERY,
    ALERT_SENSOR_ERROR,
    ALERT_NO_FINGER
};

struct Alert {
    AlertType type;
    String message;
    unsigned long timestamp;
    bool acknowledged;
};

class AlertManager {
private:
    static const int MAX_ALERTS = 10;
    Alert alerts[MAX_ALERTS];
    int alertCount;
    unsigned long lastBuzzerTime;
    bool buzzerEnabled;
    
public:
    AlertManager();
    void addAlert(AlertType type, String message);
    void acknowledgeAlert(int index);
    void clearAllAlerts();
    bool hasActiveAlerts();
    Alert* getAlerts();
    int getAlertCount();
    void setBuzzerEnabled(bool enabled);
    void handleBuzzer();
    
private:
    void shiftAlerts();
    void playAlertTone(AlertType type);
};

extern AlertManager alertManager;

#endif
