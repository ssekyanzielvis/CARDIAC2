#ifndef SPO2_ALGORITHM_H
#define SPO2_ALGORITHM_H

#include <Arduino.h>

class SpO2Calculator {
private:
    static const int BUFFER_SIZE = 100;
    uint32_t irBuffer[BUFFER_SIZE];
    uint32_t redBuffer[BUFFER_SIZE];
    int bufferIndex;
    bool bufferFull;
    
public:
    SpO2Calculator();
    void addSample(uint32_t irValue, uint32_t redValue);
    float calculateSpO2();
    bool isValidReading();
    void reset();
    
private:
    float calculateRatio();
    bool isFingerPresent();
};

extern SpO2Calculator spO2Calc;

#endif
