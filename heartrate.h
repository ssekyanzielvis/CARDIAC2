#ifndef HEARTRATE_H
#define HEARTRATE_H

#include <Arduino.h>

class HeartRateCalculator {
private:
    static const int RATE_ARRAY_SIZE = 4;
    byte rateArray[RATE_ARRAY_SIZE];
    byte rateSpot;
    long lastBeat;
    long threshold;
    bool beatDetected;
    unsigned long lastBeatTime;
    
public:
    HeartRateCalculator();
    bool checkForBeat(long sample);
    int getBeatsPerMinute();
    void reset();
    void setThreshold(long newThreshold);
};

// Global instance
extern HeartRateCalculator heartRateCalc;

#endif
