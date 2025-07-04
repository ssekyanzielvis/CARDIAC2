#ifndef CONFIG_H
#define CONFIG_H

// Hardware Pin Definitions
#define TFT_CS    5
#define TFT_DC    4
#define TFT_RST   2
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_MISO  19

#define TOUCH_CS  15
#define TOUCH_IRQ 32

#define MAX30102_INT 33
#define BUZZER_PIN   27
#define ECG_PIN      34
#define BATTERY_PIN  35

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Display Settings
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// Sensor Settings
#define MAX_HEART_RATE 200
#define MIN_HEART_RATE 40
#define MAX_SPO2 100
#define MIN_SPO2 70

// System Settings
#define MAIN_LOOP_DELAY 50
#define SENSOR_READ_INTERVAL 100
#define DISPLAY_UPDATE_INTERVAL 1000
#define LOG_INTERVAL 5000
#define ALERT_CHECK_INTERVAL 2000

// Buffer Sizes
#define WAVEFORM_BUFFER_SIZE 320
#define LOG_BUFFER_SIZE 100

// Colors (RGB565)
#define COLOR_BG        0x0000  // Black
#define COLOR_TEXT      0xFFFF  // White
#define COLOR_ACCENT    0x07E0  // Green
#define COLOR_WARNING   0xFFE0  // Yellow
#define COLOR_DANGER    0xF800  // Red
#define COLOR_BUTTON    0x4208  // Dark Gray
#define COLOR_BUTTON_PRESSED 0x2104  // Darker Gray

#endif
