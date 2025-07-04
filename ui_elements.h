#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class UIElements {
private:
    Adafruit_ILI9341* display;
    
public:
    UIElements(Adafruit_ILI9341* tft);
    
    // Button functions
    void drawButton(int x, int y, int w, int h, String text, uint16_t bgColor, uint16_t textColor);
    void drawIconButton(int x, int y, int w, int h, const uint8_t* icon, uint16_t bgColor);
    bool isButtonPressed(int x, int y, int w, int h, int touchX, int touchY);
    
    // Progress bars and indicators
    void drawProgressBar(int x, int y, int w, int h, float percentage, uint16_t color);
    void drawBatteryIcon(int x, int y, float percentage);
    void drawWiFiIcon(int x, int y, bool connected);
    void drawHeartIcon(int x, int y, uint16_t color);
    
    // Charts and graphs
    void drawLineChart(int x, int y, int w, int h, float* data, int dataSize, uint16_t color);
    void drawBarChart(int x, int y, int w, int h, float* data, int dataSize, uint16_t color);
    
    // Text utilities
    void drawCenteredText(int x, int y, int w, int h, String text, uint16_t color, int textSize);
    void drawScrollingText(int x, int y, int w, String text, uint16_t color, int& scrollPos);
    
    // Animations
    void drawPulsingHeart(int x, int y, uint16_t color, float intensity);
    void drawLoadingSpinner(int x, int y, int radius, uint16_t color);
};

#endif
