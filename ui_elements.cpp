#include "ui_elements.h"

UIElements::UIElements(Adafruit_ILI9341* tft) {
    display = tft;
}

void UIElements::drawButton(int x, int y, int w, int h, String text, uint16_t bgColor, uint16_t textColor) {
    // Draw button background with rounded corners
    display->fillRoundRect(x, y, w, h, 4, bgColor);
    display->drawRoundRect(x, y, w, h, 4, textColor);
    
    // Add subtle shadow effect
    display->drawRoundRect(x + 1, y + 1, w, h, 4, 0x2104);
    
    // Calculate text position for centering
    int16_t x1, y1;
    uint16_t textW, textH;
    display->setTextSize(1);
    display->getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);
    
    int textX = x + (w - textW) / 2;
    int textY = y + (h - textH) / 2;
    
    display->setTextColor(textColor);
    display->setCursor(textX, textY);
    display->println(text);
}

void UIElements::drawIconButton(int x, int y, int w, int h, const uint8_t* icon, uint16_t bgColor) {
    display->fillRoundRect(x, y, w, h, 4, bgColor);
    display->drawRoundRect(x, y, w, h, 4, COLOR_TEXT);
    
    // Draw icon (simplified - would need proper bitmap handling)
    display->fillRect(x + w/4, y + h/4, w/2, h/2, COLOR_TEXT);
}

bool UIElements::isButtonPressed(int x, int y, int w, int h, int touchX, int touchY) {
    return (touchX >= x && touchX <= x + w && touchY >= y && touchY <= y + h);
}

void UIElements::drawProgressBar(int x, int y, int w, int h, float percentage, uint16_t color) {
    // Background
    display->fillRect(x, y, w, h, COLOR_BG);
    display->drawRect(x, y, w, h, COLOR_TEXT);
    
    // Fill based on percentage
    int fillWidth = (int)((percentage / 100.0) * (w - 2));
    if (fillWidth > 0) {
        display->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
    }
    
    // Percentage text
    display->setTextColor(COLOR_TEXT);
    display->setTextSize(1);
    display->setCursor(x + w + 5, y + (h - 8) / 2);
    display->printf("%.0f%%", percentage);
}

void UIElements::drawBatteryIcon(int x, int y, float percentage) {
    int w = 20, h = 12;
    
    // Battery outline
    display->drawRect(x, y, w, h, COLOR_TEXT);
    display->fillRect(x + w, y + 2, 2, h - 4, COLOR_TEXT);
    
    // Battery fill
    uint16_t fillColor;
    if (percentage > 50) fillColor = COLOR_ACCENT;
    else if (percentage > 20) fillColor = COLOR_WARNING;
    else fillColor = COLOR_DANGER;
    
    int fillWidth = (int)((percentage / 100.0) * (w - 2));
    if (fillWidth > 0) {
        display->fillRect(x + 1, y + 1, fillWidth, h - 2, fillColor);
    }
}

void UIElements::drawWiFiIcon(int x, int y, bool connected) {
    if (connected) {
        // Draw WiFi signal bars
        display->fillRect(x, y + 6, 2, 2, COLOR_ACCENT);
        display->fillRect(x + 3, y + 4, 2, 4, COLOR_ACCENT);
        display->fillRect(x + 6, y + 2, 2, 6, COLOR_ACCENT);
        display->fillRect(x + 9, y, 2, 8, COLOR_ACCENT);
    } else {
        // Draw disconnected icon
        display->drawLine(x, y, x + 10, y + 8, COLOR_DANGER);
        display->drawLine(x, y + 8, x + 10, y, COLOR_DANGER);
    }
}

void UIElements::drawHeartIcon(int x, int y, uint16_t color) {
    // Draw heart shape using filled circles and triangle
    display->fillCircle(x - 3, y - 2, 3, color);
    display->fillCircle(x + 3, y - 2, 3, color);
    display->fillTriangle(x - 6, y, x + 6, y, x, y + 8, color);
}

void UIElements::drawLineChart(int x, int y, int w, int h, float* data, int dataSize, uint16_t color) {
    if (dataSize < 2) return;
    
    // Draw chart background
    display->fillRect(x, y, w, h, COLOR_BG);
    display->drawRect(x, y, w, h, COLOR_TEXT);
    
    // Find min/max values for scaling
    float minVal = data[0], maxVal = data[0];
    for (int i = 1; i < dataSize; i++) {
        if (data[i] < minVal) minVal = data[i];
        if (data[i] > maxVal) maxVal = data[i];
    }
    
    if (maxVal == minVal) return; // Avoid division by zero
    
    // Draw grid lines
    for (int i = 1; i < 4; i++) {
        int gridY = y + (h * i) / 4;
        display->drawFastHLine(x, gridY, w, 0x2104);
    }
    
    // Draw data points
    for (int i = 1; i < dataSize && i < w; i++) {
        int x1 = x + i - 1;
        int x2 = x + i;
        
        int y1 = y + h - (int)((data[i-1] - minVal) / (maxVal - minVal) * h);
        int y2 = y + h - (int)((data[i] - minVal) / (maxVal - minVal) * h);
        
        display->drawLine(x1, y1, x2, y2, color);
    }
}

void UIElements::drawBarChart(int x, int y, int w, int h, float* data, int dataSize, uint16_t color) {
    if (dataSize == 0) return;
    
    // Draw chart background
    display->fillRect(x, y, w, h, COLOR_BG);
    display->drawRect(x, y, w, h, COLOR_TEXT);
    
    // Find max value for scaling
    float maxVal = data[0];
    for (int i = 1; i < dataSize; i++) {
        if (data[i] > maxVal) maxVal = data[i];
    }
    
    if (maxVal == 0) return;
    
    // Draw bars
    int barWidth = w / dataSize;
    for (int i = 0; i < dataSize; i++) {
        int barHeight = (int)((data[i] / maxVal) * h);
        int barX = x + i * barWidth;
        int barY = y + h - barHeight;
        
        display->fillRect(barX + 1, barY, barWidth - 2, barHeight, color);
    }
}

void UIElements::drawCenteredText(int x, int y, int w, int h, String text, uint16_t color, int textSize) {
    display->setTextSize(textSize);
    display->setTextColor(color);
    
    int16_t x1, y1;
    uint16_t textW, textH;
    display->getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);
    
    int textX = x + (w - textW) / 2;
    int textY = y + (h - textH) / 2;
    
    display->setCursor(textX, textY);
    display->println(text);
}

void UIElements::drawScrollingText(int x, int y, int w, String text, uint16_t color, int& scrollPos) {
    display->setTextColor(color);
    display->setTextSize(1);
    
    // Clear text area
    display->fillRect(x, y, w, 8, COLOR_BG);
    
    // Calculate text width
    int16_t x1, y1;
    uint16_t textW, textH;
    display->getTextBounds(text, 0, 0, &x1, &y1, &textW, &textH);
    
    if (textW <= w) {
        // Text fits, no scrolling needed
        display->setCursor(x, y);
        display->println(text);
        scrollPos = 0;
    } else {
        // Scroll text
        display->setCursor(x - scrollPos, y);
        display->println(text);
        
        scrollPos += 2;
        if (scrollPos > textW + w) {
            scrollPos = 0;
        }
    }
}

void UIElements::drawPulsingHeart(int x, int y, uint16_t color, float intensity) {
    // Scale heart based on intensity (0.5 to 1.5)
    float scale = 0.8 + (intensity * 0.4);
    
    int size = (int)(6 * scale);
    
    // Draw scaled heart
    display->fillCircle(x - size/2, y - size/3, size/2, color);
    display->fillCircle(x + size/2, y - size/3, size/2, color);
    display->fillTriangle(x - size, y, x + size, y, x, y + size, color);
}

void UIElements::drawLoadingSpinner(int x, int y, int radius, uint16_t color) {
    static int angle = 0;
    
    // Clear previous spinner
    display->fillCircle(x, y, radius + 2, COLOR_BG);
    
    // Draw spinner segments
    for (int i = 0; i < 8; i++) {
        int segmentAngle = (angle + i * 45) % 360;
        float radians = segmentAngle * PI / 180.0;
        
        int x1 = x + (int)(radius * 0.6 * cos(radians));
        int y1 = y + (int)(radius * 0.6 * sin(radians));
        int x2 = x + (int)(radius * cos(radians));
        int y2 = y + (int)(radius * sin(radians));
        
        // Fade effect
        uint16_t segmentColor = color;
        if (i > 4) {
            segmentColor = 0x2104; // Dimmed
        }
        
        display->drawLine(x1, y1, x2, y2, segmentColor);
    }
    
    angle = (angle + 45) % 360;
}
