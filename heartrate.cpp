#include "heartRate.h"

HeartRateCalculator heartRateCalc;

HeartRateCalculator::HeartRateCalculator() {
    rateSpot = 0;
    lastBeat = 0;
    threshold = 512;
    beatDetected = false;
    lastBeatTime = 0;
    
    // Initialize rate array
    for (int i = 0; i < RATE_ARRAY_SIZE; i++) {
        rateArray[i] = 0;
    }
}

bool HeartRateCalculator::checkForBeat(long sample) {
    // Improved peak detection algorithm
    if (sample > threshold && !beatDetected && (millis() - lastBeatTime) > 300) {
        beatDetected = true;
        lastBeatTime = millis();
        
        // Calculate time between beats
        long delta = millis() - lastBeat;
        lastBeat = millis();
        
        // Store valid beat intervals (300ms to 3000ms = 20-200 BPM)
        if (delta > 300 && delta < 3000) {
            rateArray[rateSpot++] = (byte)(60000 / delta);
            rateSpot %= RATE_ARRAY_SIZE;
        }
        
        return true;
    } else if (sample < threshold - 100) {
        beatDetected = false;
    }
    
    // Adaptive threshold with smoothing
    threshold = (threshold * 31 + sample) / 32;
    
    return false;
}

int HeartRateCalculator::getBeatsPerMinute() {
    long total = 0;
    int validReadings = 0;
    
    for (byte i = 0; i < RATE_ARRAY_SIZE; i++) {
        if (rateArray[i] != 0) {
            total += rateArray[i];
            validReadings++;
        }
    }
    
    if (validReadings == 0) return 0;
    
    return total / validReadings;
}

void HeartRateCalculator::reset() {
    for (int i = 0; i < RATE_ARRAY_SIZE; i++) {
        rateArray[i] = 0;
    }
    rateSpot = 0;
    lastBeat = 0;
    beatDetected = false;
}

void HeartRateCalculator::setThreshold(long newThreshold) {
    threshold = newThreshold;
}
