#include "spo2_algorithm.h"

SpO2Calculator spO2Calc;

SpO2Calculator::SpO2Calculator() {
    bufferIndex = 0;
    bufferFull = false;
    reset();
}

void SpO2Calculator::addSample(uint32_t irValue, uint32_t redValue) {
    irBuffer[bufferIndex] = irValue;
    redBuffer[bufferIndex] = redValue;
    
    bufferIndex++;
    if (bufferIndex >= BUFFER_SIZE) {
        bufferIndex = 0;
        bufferFull = true;
    }
}

float SpO2Calculator::calculateSpO2() {
    if (!bufferFull) return 0;
    
    float ratio = calculateRatio();
    if (ratio == 0) return 0;
    
    // Empirical formula for SpO2 calculation
    // This is a simplified version - actual implementation would use
    // more sophisticated algorithms
    float spO2 = 110.0 - 25.0 * ratio;
    
    // Constrain to reasonable values
    if (spO2 > 100) spO2 = 100;
    if (spO2 < 70) spO2 = 70;
    
    return spO2;
}

float SpO2Calculator::calculateRatio() {
    if (!bufferFull) return 0;
    
    // Find AC and DC components for both IR and Red
    uint32_t irMax = 0, irMin = 0xFFFFFFFF;
    uint32_t redMax = 0, redMin = 0xFFFFFFFF;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (irBuffer[i] > irMax) irMax = irBuffer[i];
        if (irBuffer[i] < irMin) irMin = irBuffer[i];
        if (redBuffer[i] > redMax) redMax = redBuffer[i];
        if (redBuffer[i] < redMin) redMin = redBuffer[i];
    }
    
    // Calculate AC/DC ratios
    float irAC = irMax - irMin;
    float irDC = (irMax + irMin) / 2.0;
    float redAC = redMax - redMin;
    float redDC = (redMax + redMin) / 2.0;
    
    if (irDC == 0 || redDC == 0) return 0;
    
    float irRatio = irAC / irDC;
    float redRatio = redAC / redDC;
    
    if (irRatio == 0) return 0;
    
    return redRatio / irRatio;
}

bool SpO2Calculator::isValidReading() {
    return bufferFull && isFingerPresent();
}

bool SpO2Calculator::isFingerPresent() {
    if (!bufferFull) return false;
    
    // Check if recent samples indicate finger presence
    uint32_t recentSum = 0;
    int recentSamples = min(10, BUFFER_SIZE);
    
    for (int i = 0; i < recentSamples; i++) {
        int index = (bufferIndex - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
        recentSum += irBuffer[index];
    }
    
    uint32_t average = recentSum / recentSamples;
    return average > 50000; // Threshold for finger detection
}

void SpO2Calculator::reset() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        irBuffer[i] = 0;
        redBuffer[i] = 0;
    }
    bufferIndex = 0;
    bufferFull = false;
}
