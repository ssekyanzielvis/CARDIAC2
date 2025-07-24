import time
import numpy as np
from dataclasses import dataclass
from typing import List, Dict, Tuple
import random
import math
import sys

# ==================== CONSTANTS ====================
FIRMWARE_VERSION = "1.0.0"
DEVICE_NAME = "CardiacMonitor"
SENSOR_UPDATE_INTERVAL = 0.1  # seconds
DISPLAY_UPDATE_INTERVAL = 0.1  # seconds
DATA_LOG_INTERVAL = 1.0  # seconds

SAMPLE_RATE = 100
BUFFER_SIZE = 100  # Reduced for memory
FINGER_THRESHOLD = 50000
SPO2_BUFFER_SIZE = 50

MAX_ALERTS = 5    # Reduced for memory
MAX_HISTORY = 20  # Reduced for memory
DATA_BUFFER_SIZE = 50  # Reduced for memory
MAX_STRING = 64   # Reduced for memory

# System states
SYSTEM_STATE_INITIALIZING = 0
SYSTEM_STATE_RUNNING = 1
SYSTEM_STATE_SETTINGS = 2
SYSTEM_STATE_HISTORY = 3
SYSTEM_STATE_ERROR = 4
SYSTEM_STATE_SLEEP = 5

# Screen types
SCREEN_TYPE_MAIN = 0
SCREEN_TYPE_SETTINGS = 1
SCREEN_TYPE_HISTORY = 2

# Alert levels
ALERT_LEVEL_INFO = 0
ALERT_LEVEL_WARNING = 1
ALERT_LEVEL_CRITICAL = 2

# Colors (RGB)
COLOR_BLACK = (0, 0, 0)
COLOR_WHITE = (255, 255, 255)
COLOR_RED = (255, 0, 0)
COLOR_GREEN = (0, 255, 0)
COLOR_BLUE = (0, 0, 255)
COLOR_YELLOW = (255, 255, 0)
COLOR_ORANGE = (255, 165, 0)
COLOR_PURPLE = (128, 0, 128)
COLOR_GRAY = (128, 128, 128)
COLOR_DARKGRAY = (64, 64, 64)

# ==================== DATA CLASSES ====================
@dataclass
class AlertThresholds:
    heartRateMin: float = 60.0
    heartRateMax: float = 100.0
    spO2Min: float = 95.0
    batteryMin: float = 20.0
    enabled: bool = True

@dataclass
class VitalSigns:
    heartRate: float = 0.0
    spO2: float = 0.0
    batteryLevel: float = 0.0
    isFingerDetected: bool = False
    timestamp: float = 0.0

@dataclass
class Alert:
    level: int
    message: str
    timestamp: float
    acknowledged: bool = False

@dataclass
class Preferences:
    hr_min: float = 60.0
    hr_max: float = 100.0
    spo2_min: float = 95.0
    bat_min: float = 20.0
    alerts_en: bool = True
    brightness: int = 128

# ==================== SIMULATED HARDWARE CLASSES ====================
class SimulatedTFT:
    def __init__(self):
        self.width = 240
        self.height = 320
        self.buffer = np.zeros((self.height, self.width, 3), dtype=np.uint8)
        self.text_size = 1
        self.text_color = COLOR_WHITE
        self.cursor_x = 0
        self.cursor_y = 0
        self.rotation = 0
        
    def begin(self):
        return True
        
    def setRotation(self, rotation):
        self.rotation = rotation
        
    def fillScreen(self, color):
        self.buffer[:, :] = color
        
    def fillRect(self, x, y, w, h, color):
        self.buffer[y:y+h, x:x+w] = color
        
    def drawRect(self, x, y, w, h, color):
        self.buffer[y:y+h, x] = color
        self.buffer[y:y+h, x+w-1] = color
        self.buffer[y, x:x+w] = color
        self.buffer[y+h-1, x:x+w] = color
        
    def setTextSize(self, size):
        self.text_size = size
        
    def setTextColor(self, color):
        self.text_color = color
        
    def setCursor(self, x, y):
        self.cursor_x = x
        self.cursor_y = y
        
    def print(self, text):
        # Simulate text drawing (in a real implementation, this would use a font)
        for i, char in enumerate(text):
            if self.cursor_x + i * 6 * self.text_size < self.width and self.cursor_y < self.height:
                self.buffer[self.cursor_y, self.cursor_x + i * 6 * self.text_size] = self.text_color
                
    def println(self, text):
        self.print(text)
        self.cursor_y += 8 * self.text_size
        
    def drawPixel(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.buffer[y, x] = color
            
    def fillCircle(self, x, y, r, color):
        for i in range(max(0, x-r), min(self.width, x+r+1)):
            for j in range(max(0, y-r), min(self.height, y+r+1)):
                if (i-x)**2 + (j-y)**2 <= r**2:
                    self.buffer[j, i] = color
                    
    def fillTriangle(self, x0, y0, x1, y1, x2, y2, color):
        # Simple triangle fill (in a real implementation, use a better algorithm)
        for i in range(min(x0, x1, x2), max(x0, x1, x2)+1):
            for j in range(min(y0, y1, y2), max(y0, y1, y2)+1):
                if (i-x0)*(y1-y0) - (j-y0)*(x1-x0) >= 0 and \
                   (i-x1)*(y2-y1) - (j-y1)*(x2-x1) >= 0 and \
                   (i-x2)*(y0-y2) - (j-y2)*(x0-x2) >= 0:
                    self.buffer[j, i] = color

class SimulatedTouch:
    def __init__(self):
        self.touched_state = False
        
    def begin(self):
        return True
        
    def touched(self):
        # Simulate occasional touch
        if random.random() < 0.1:
            self.touched_state = True
        return self.touched_state
        
    def getPoint(self):
        self.touched_state = False
        return type('Point', (), {'x': random.randint(0, 240), 'y': random.randint(0, 320)})

class SimulatedMAX30102:
    def __init__(self):
        self.available_samples = 0
        self.ir_buffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        self.red_buffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        
    def begin(self):
        return True
        
    def setup(self, ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange):
        pass
        
    def setPulseAmplitudeRed(self, amplitude):
        pass
        
    def setPulseAmplitudeGreen(self, amplitude):
        pass
        
    def available(self):
        # Simulate sensor data availability
        self.available_samples = random.randint(0, 10)
        return self.available_samples > 0
        
    def getRed(self):
        return random.randint(10000, 60000)
        
    def getIR(self):
        return random.randint(10000, 60000)
        
    def nextSample(self):
        if self.available_samples > 0:
            self.available_samples -= 1

# ==================== MAIN APPLICATION CLASS ====================
class CardiacMonitor:
    def __init__(self):
        # Initialize simulated hardware
        self.tft = SimulatedTFT()
        self.ts = SimulatedTouch()
        self.particleSensor = SimulatedMAX30102()
        
        # System state
        self.currentState = SYSTEM_STATE_INITIALIZING
        self.currentScreen = SCREEN_TYPE_MAIN
        
        # Data structures
        self.currentVitals = VitalSigns()
        self.alertThresholds = AlertThresholds()
        self.preferences = Preferences()
        
        # Buffers and arrays
        self.irBuffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        self.redBuffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        self.bufferIndex = 0
        self.fingerDetected = False
        
        # Alerts
        self.activeAlerts = []
        self.alertHistory = []
        self.lastAlertTime = 0.0
        
        # Data logging
        self.dataBuffer = []
        
        # Timing
        self.lastSensorUpdate = 0.0
        self.lastDisplayUpdate = 0.0
        self.lastDataLog = 0.0
        self.lastAlertCheck = 0.0
        self.lastTouchTime = 0.0
        
        # Display settings
        self.screenBrightness = 128
        self.displayOn = True
        
        # Initialize system
        self.setup()
        
    def setup(self):
        """Initialize the system"""
        print("\n=== Cardiac Monitor v1.0 ===")
        print("Initializing system...")
        
        # Load settings
        self.loadSettings()
        
        # Initialize display
        if not self.initializeDisplay():
            print("FATAL: Display initialization failed")
            sys.exit(1)
            
        # Show splash screen
        self.showSplashScreen()
        time.sleep(2)
        
        # Initialize sensor
        if not self.initializeSensor():
            self.showError("Sensor Error", "Failed to initialize MAX30102")
            time.sleep(5)
            
        # Set initial state
        self.currentState = SYSTEM_STATE_RUNNING
        self.currentScreen = SCREEN_TYPE_MAIN
        self.showMainScreen()
        
        print("System initialization complete")
        print("=================================")
        
    def loop(self):
        """Main application loop"""
        while True:
            currentTime = time.time()
            
            # Handle touch input
            self.handleTouch()
            
            # Update sensors at regular intervals
            if currentTime - self.lastSensorUpdate >= SENSOR_UPDATE_INTERVAL:
                self.lastSensorUpdate = currentTime
                self.updateSensors()
                
            # Update display at regular intervals
            if currentTime - self.lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL:
                self.lastDisplayUpdate = currentTime
                self.updateDisplay()
                
            # Log data at regular intervals
            if currentTime - self.lastDataLog >= DATA_LOG_INTERVAL:
                self.lastDataLog = currentTime
                self.logData()
                
            # Check for alerts at regular intervals
            if currentTime - self.lastAlertCheck >= 1.0:
                self.lastAlertCheck = currentTime
                self.checkAlerts()
                
            # Handle screen timeout
            self.handleScreenTimeout()
            
            # Handle serial commands
            self.handleSerialCommands()
            
            # Check memory usage
            self.checkMemoryUsage()
            
            # Handle low power mode
            self.handleLowPowerMode()
            
            # Small delay to prevent CPU overload
            time.sleep(0.01)
            
    # ==================== DISPLAY FUNCTIONS ====================
    def initializeDisplay(self):
        """Initialize the display hardware"""
        print("Initializing display...")
        if not self.tft.begin():
            print("Display initialization failed")
            return False
            
        self.tft.setRotation(1)
        self.tft.fillScreen(COLOR_BLACK)
        
        if not self.ts.begin():
            print("Touch screen initialization failed")
            return False
            
        self.ts.setRotation(1)
        
        print("Display initialized successfully")
        return True
        
    def showSplashScreen(self):
        """Show the splash screen"""
        self.tft.fillScreen(COLOR_BLACK)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(3)
        
        # Center the title
        title = "Cardiac Monitor"
        self.tft.setCursor((self.tft.width - len(title) * 6 * 3) // 2, 60)
        self.tft.println(title)
        
        # Version
        version = "v1.0"
        self.tft.setTextSize(2)
        self.tft.setCursor((self.tft.width - len(version) * 6 * 2) // 2, 100)
        self.tft.println(version)
        
        # Draw heart icon
        self.drawHeart(self.tft.width // 2, 140, COLOR_RED)
        
        # Disclaimer
        self.tft.setTextSize(1)
        self.tft.setTextColor(COLOR_GRAY)
        self.tft.setCursor(10, 200)
        self.tft.println("For Professional use only")
        self.tft.setCursor(30, 215)
        self.tft.println("Used for medical diagnosis")
        
    def showMainScreen(self):
        """Show the main screen"""
        self.currentScreen = SCREEN_TYPE_MAIN
        self.tft.fillScreen(COLOR_BLACK)
        
        # Header
        self.tft.fillRect(0, 0, self.tft.width, 30, COLOR_BLUE)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(2)
        self.tft.setCursor(10, 8)
        self.tft.println("Cardiac Monitor")
        
        # Draw UI elements
        self.drawStatusBar()
        self.drawVitalSignsLayout()
        self.drawMainButtons()
        
    def drawStatusBar(self):
        """Draw the status bar"""
        self.tft.setTextSize(1)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setCursor(180, 10)
        
        # Battery level
        batteryColor = COLOR_GREEN if self.currentVitals.batteryLevel > 20 else COLOR_RED
        self.tft.setTextColor(batteryColor)
        self.tft.setCursor(200, 20)
        self.tft.println(f"{int(self.currentVitals.batteryLevel)}%")
        
    def drawVitalSignsLayout(self):
        """Draw the vital signs layout"""
        # Heart rate box
        self.tft.drawRect(10, 40, 100, 80, COLOR_WHITE)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(1)
        self.tft.setCursor(15, 45)
        self.tft.println("Heart Rate")
        self.tft.setCursor(30, 55)
        self.tft.println("(BPM)")
        
        # SpO2 box
        self.tft.drawRect(130, 40, 100, 80, COLOR_WHITE)
        self.tft.setCursor(150, 45)
        self.tft.println("SpO2 (%)")
        
        # Waveform box
        self.tft.drawRect(10, 130, 220, 60, COLOR_WHITE)
        self.tft.setCursor(15, 135)
        self.tft.println("Waveform")
        
    def drawMainButtons(self):
        """Draw the main screen buttons"""
        # Settings button
        self.tft.fillRect(10, 200, 60, 30, COLOR_GRAY)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(1)
        self.tft.setCursor(20, 212)
        self.tft.println("Settings")
        
        # History button
        self.tft.fillRect(90, 200, 60, 30, COLOR_GRAY)
        self.tft.setCursor(100, 212)
        self.tft.println("History")
        
        # Info button
        self.tft.fillRect(170, 200, 60, 30, COLOR_GRAY)
        self.tft.setCursor(180, 212)
        self.tft.println("Info")
        
    def updateVitalSigns(self):
        """Update the displayed vital signs"""
        # Heart rate
        self.tft.fillRect(15, 60, 90, 50, COLOR_BLACK)
        self.tft.setTextColor(COLOR_RED)
        self.tft.setTextSize(3)
        self.tft.setCursor(20, 70)
        
        if self.currentVitals.isFingerDetected and self.currentVitals.heartRate > 0:
            self.tft.print(f"{int(self.currentVitals.heartRate)}")
        else:
            self.tft.print("--")
            
        # SpO2
        self.tft.fillRect(135, 60, 90, 50, COLOR_BLACK)
        self.tft.setTextColor(COLOR_BLUE)
        self.tft.setCursor(140, 70)
        
        if self.currentVitals.isFingerDetected and self.currentVitals.spO2 > 0:
            self.tft.print(f"{int(self.currentVitals.spO2)}")
        else:
            self.tft.print("--")
            
        # Finger detection status
        self.tft.setTextSize(1)
        fingerColor = COLOR_GREEN if self.currentVitals.isFingerDetected else COLOR_RED
        self.tft.setTextColor(fingerColor)
        self.tft.setCursor(15, 105)
        self.tft.fillRect(15, 105, 100, 10, COLOR_BLACK)
        status = "Finger detected" if self.currentVitals.isFingerDetected else "Place finger"
        self.tft.println(status)
        
    def drawWaveform(self):
        """Draw the waveform display"""
        if not hasattr(self, 'waveformX'):
            self.waveformX = 15
            self.lastY = 160
            
        # Clear previous point
        self.tft.drawPixel(self.waveformX, self.lastY, COLOR_BLACK)
        
        if self.currentVitals.isFingerDetected:
            # Simulate waveform
            waveY = 160 + int(math.sin(time.time() * 0.01) * 20)
            self.tft.drawPixel(self.waveformX, waveY, COLOR_GREEN)
            self.lastY = waveY
            
        self.waveformX += 1
        if self.waveformX > 225:
            self.waveformX = 15
            self.tft.fillRect(15, 140, 210, 45, COLOR_BLACK)
            
    def showError(self, title, message):
        """Show an error screen"""
        self.tft.fillScreen(COLOR_BLACK)
        self.tft.setTextColor(COLOR_RED)
        self.tft.setTextSize(2)
        
        # Center the title
        self.tft.setCursor((self.tft.width - len(title) * 6 * 2) // 2, 60)
        self.tft.println(title)
        
        # Show message
        self.tft.setTextSize(1)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setCursor(10, 100)
        self.tft.println(message)
        
    def showSettingsScreen(self):
        """Show the settings screen"""
        self.currentScreen = SCREEN_TYPE_SETTINGS
        self.tft.fillScreen(COLOR_BLACK)
        
        # Header
        self.tft.fillRect(0, 0, self.tft.width, 30, COLOR_BLUE)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(2)
        self.tft.setCursor(10, 8)
        self.tft.println("Settings")
        
        # Back button
        self.tft.fillRect(200, 5, 40, 20, COLOR_GRAY)
        self.tft.setTextSize(1)
        self.tft.setCursor(210, 10)
        self.tft.println("Back")
        
        # Alert thresholds
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(1)
        self.tft.setCursor(10, 50)
        self.tft.println("Alert Thresholds:")
        
        self.tft.setCursor(20, 70)
        self.tft.println(f"HR: {int(self.alertThresholds.heartRateMin)} - {int(self.alertThresholds.heartRateMax)} BPM")
        
        self.tft.setCursor(20, 90)
        self.tft.println(f"SpO2 Min: {int(self.alertThresholds.spO2Min)}%")
        
        self.tft.setCursor(20, 110)
        self.tft.println(f"Battery Min: {int(self.alertThresholds.batteryMin)}%")
        
        # Brightness
        self.tft.setCursor(10, 140)
        self.tft.println(f"Brightness: {self.screenBrightness}")
        
        # Buttons
        self.tft.fillRect(10, 170, 100, 30, COLOR_GREEN)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setCursor(20, 182)
        self.tft.println("Export")
        
        self.tft.fillRect(130, 170, 100, 30, COLOR_RED)
        self.tft.setCursor(150, 182)
        self.tft.println("Clear")
        
    def showHistoryScreen(self):
        """Show the history screen"""
        self.currentScreen = SCREEN_TYPE_HISTORY
        self.tft.fillScreen(COLOR_BLACK)
        
        # Header
        self.tft.fillRect(0, 0, self.tft.width, 30, COLOR_BLUE)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(2)
        self.tft.setCursor(10, 8)
        self.tft.println("Data History")
        
        # Back button
        self.tft.fillRect(200, 5, 40, 20, COLOR_GRAY)
        self.tft.setTextSize(1)
        self.tft.setCursor(210, 10)
        self.tft.println("Back")
        
        # Data entries
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(1)
        self.tft.setCursor(10, 40)
        self.tft.println("Recent Readings:")
        
        y = 60
        count = min(6, len(self.dataBuffer))
        
        for i in range(len(self.dataBuffer) - count, len(self.dataBuffer)):
            if i >= 0:
                data = self.dataBuffer[i]
                timeStr = self.formatTime(data.timestamp)
                
                self.tft.setCursor(10, y)
                self.tft.println(f"{timeStr} HR:{int(data.heartRate)}")
                y += 15
                
                self.tft.setCursor(10, y)
                self.tft.println(f"SpO2:{int(data.spO2)} Bat:{int(data.batteryLevel)}%")
                y += 15
                
        if not self.dataBuffer:
            self.tft.setCursor(10, 60)
            self.tft.println("No data available")
            
    def drawHeart(self, x, y, color):
        """Draw a heart icon"""
        self.tft.fillCircle(x - 8, y - 5, 8, color)
        self.tft.fillCircle(x + 8, y - 5, 8, color)
        self.tft.fillTriangle(x - 15, y, x + 15, y, x, y + 15, color)
        
    # ==================== SENSOR FUNCTIONS ====================
    def initializeSensor(self):
        """Initialize the MAX30102 sensor"""
        print("Initializing MAX30102 sensor...")
        
        if not self.particleSensor.begin():
            print("MAX30102 not found")
            return False
            
        # Setup with default settings
        self.particleSensor.setup(
            ledBrightness=0x1F,
            sampleAverage=4,
            ledMode=2,
            sampleRate=100,
            pulseWidth=411,
            adcRange=4096
        )
        self.particleSensor.setPulseAmplitudeRed(0x0A)
        self.particleSensor.setPulseAmplitudeGreen(0)
        
        print("MAX30102 initialized successfully")
        return True
        
    def updateSensors(self):
        """Update sensor readings"""
        self.currentVitals.batteryLevel = self.readBatteryLevel()
        self.currentVitals.timestamp = time.time()
        
        if self.particleSensor.available():
            self.redBuffer[self.bufferIndex] = self.particleSensor.getRed()
            self.irBuffer[self.bufferIndex] = self.particleSensor.getIR()
            
            self.fingerDetected = self.irBuffer[self.bufferIndex] > FINGER_THRESHOLD
            self.currentVitals.isFingerDetected = self.fingerDetected
            
            self.bufferIndex += 1
            
            if self.bufferIndex >= BUFFER_SIZE:
                self.bufferIndex = 0
                
                if self.fingerDetected:
                    # Simulate heart rate and SpO2 calculation
                    if random.random() > 0.2:  # 80% chance of valid reading
                        self.currentVitals.heartRate = random.uniform(60.0, 100.0)
                        self.currentVitals.spO2 = random.uniform(95.0, 100.0)
                    else:
                        self.currentVitals.heartRate = 0.0
                        self.currentVitals.spO2 = 0.0
                else:
                    self.currentVitals.heartRate = 0.0
                    self.currentVitals.spO2 = 0.0
                    
            self.particleSensor.nextSample()
            
    def readBatteryLevel(self):
        """Read and calculate battery level"""
        # Simulate battery level (3.0V to 4.2V range)
        voltage = random.uniform(3.0, 4.2)
        percentage = ((voltage - 3.0) / 1.2) * 100.0
        return max(0.0, min(100.0, percentage))
        
    # ==================== TOUCH HANDLING ====================
    def handleTouch(self):
        """Handle touch input"""
        if self.ts.touched():
            p = self.ts.getPoint()
            
            # Map touch coordinates to screen coordinates
            x = int((p.x / 3700) * self.tft.width)
            y = int((p.y / 3800) * self.tft.height)
            
            self.lastTouchTime = time.time()
            
            if not self.displayOn:
                self.displayOn = True
                return
                
            self.handleTouchEvent(x, y)
            
    def handleTouchEvent(self, x, y):
        """Handle touch events based on current screen"""
        if self.currentScreen == SCREEN_TYPE_MAIN:
            self.handleMainScreenTouch(x, y)
        elif self.currentScreen == SCREEN_TYPE_SETTINGS:
            self.handleSettingsScreenTouch(x, y)
        elif self.currentScreen == SCREEN_TYPE_HISTORY:
            self.handleHistoryScreenTouch(x, y)
            
    def handleMainScreenTouch(self, x, y):
        """Handle touch on main screen"""
        if 10 <= x <= 70 and 200 <= y <= 230:
            self.showSettingsScreen()
        elif 90 <= x <= 150 and 200 <= y <= 230:
            self.showHistoryScreen()
        elif 170 <= x <= 230 and 200 <= y <= 230:
            self.printSystemInfo()
            
    def handleSettingsScreenTouch(self, x, y):
        """Handle touch on settings screen"""
        if 200 <= x <= 240 and 5 <= y <= 25:
            self.showMainScreen()
        elif 10 <= x <= 110 and 170 <= y <= 200:
            self.exportData()
        elif 130 <= x <= 230 and 170 <= y <= 200:
            self.clearData()
            
    def handleHistoryScreenTouch(self, x, y):
        """Handle touch on history screen"""
        if 200 <= x <= 240 and 5 <= y <= 25:
            self.showMainScreen()
            
    def handleScreenTimeout(self):
        """Handle screen timeout"""
        if self.displayOn and (time.time() - self.lastTouchTime > SCREEN_TIMEOUT): # type: ignore
            self.displayOn = False
            self.tft.fillScreen(COLOR_BLACK)
            
    # ==================== DATA LOGGING ====================
    def logData(self):
        """Log current vital signs data"""
        if self.currentVitals.isFingerDetected and self.currentVitals.heartRate > 0:
            if len(self.dataBuffer) < DATA_BUFFER_SIZE:
                self.dataBuffer.append(self.currentVitals)
            else:
                self.dataBuffer.pop(0)
                self.dataBuffer.append(self.currentVitals)
                
    # ==================== ALERT SYSTEM ====================
    def checkAlerts(self):
        """Check for alert conditions"""
        if not self.alertThresholds.enabled:
            return
            
        if (self.currentVitals.isFingerDetected and 
            self.currentVitals.heartRate > 0 and
            (self.currentVitals.heartRate < self.alertThresholds.heartRateMin or
             self.currentVitals.heartRate > self.alertThresholds.heartRateMax)):
            
            level = (ALERT_LEVEL_CRITICAL if self.currentVitals.heartRate < 50 or 
                     self.currentVitals.heartRate > 120 else ALERT_LEVEL_WARNING)
            self.triggerAlert(level, f"HR: {int(self.currentVitals.heartRate)} BPM")
            
        if (self.currentVitals.isFingerDetected and 
            self.currentVitals.spO2 > 0 and
            self.currentVitals.spO2 < self.alertThresholds.spO2Min):
            
            level = ALERT_LEVEL_CRITICAL if self.currentVitals.spO2 < 90 else ALERT_LEVEL_WARNING
            self.triggerAlert(level, f"Low SpO2: {int(self.currentVitals.spO2)}%")
            
        if self.currentVitals.batteryLevel < self.alertThresholds.batteryMin:
            level = ALERT_LEVEL_CRITICAL if self.currentVitals.batteryLevel < 10 else ALERT_LEVEL_WARNING
            self.triggerAlert(level, f"Low battery: {int(self.currentVitals.batteryLevel)}%")
            
        self.removeOldAlerts()
        
    def triggerAlert(self, level, message):
        """Trigger a new alert"""
        currentTime = time.time()
        if currentTime - self.lastAlertTime < ALERT_COOLDOWN: # type: ignore
            return
            
        if len(self.activeAlerts) < MAX_ALERTS:
            self.activeAlerts.append(Alert(
                level=level,
                message=message,
                timestamp=currentTime
            ))
            
        if len(self.alertHistory) < MAX_HISTORY:
            self.alertHistory.append(Alert(
                level=level,
                message=message,
                timestamp=currentTime
            ))
        else:
            self.alertHistory.pop(0)
            self.alertHistory.append(Alert(
                level=level,
                message=message,
                timestamp=currentTime
            ))
            
        self.lastAlertTime = currentTime
        self.playAlertSound(level)
        self.showAlert(message, level)
        
        print(f"ALERT [{'CRITICAL' if level == ALERT_LEVEL_CRITICAL else 'WARNING'}]: {message}")
        
    def playAlertSound(self, level):
        """Play alert sound based on level"""
        beepCount = 1
        beepDuration = 0.2
        
        if level == ALERT_LEVEL_CRITICAL:
            beepCount = 3
            beepDuration = 0.5
        elif level == ALERT_LEVEL_WARNING:
            beepCount = 2
            beepDuration = 0.3
            
        # In a real implementation, this would control a buzzer
        for i in range(beepCount):
            print("\a", end='', flush=True)  # System bell
            time.sleep(beepDuration)
            if i < beepCount - 1:
                time.sleep(0.2)
                
    def showAlert(self, message, level):
        """Show alert on display"""
        alertColor = COLOR_RED if level == ALERT_LEVEL_CRITICAL else \
                     COLOR_ORANGE if level == ALERT_LEVEL_WARNING else COLOR_YELLOW
                     
        self.tft.fillRect(0, 30, self.tft.width, 25, alertColor)
        self.tft.setTextColor(COLOR_BLACK)
        self.tft.setTextSize(1)
        self.tft.setCursor(5, 38)
        self.tft.println(message)
        
        # Schedule alert to disappear after 5 seconds
        if hasattr(self, 'alertDisplayTime'):
            self.alertDisplayTime = time.time()
            
    def removeOldAlerts(self):
        """Remove old alerts"""
        currentTime = time.time()
        self.activeAlerts = [alert for alert in self.activeAlerts 
                           if not alert.acknowledged and 
                           (currentTime - alert.timestamp <= 30)]
                           
    # ==================== SETTINGS MANAGEMENT ====================
    def loadSettings(self):
        """Load settings from storage"""
        # In a real implementation, this would load from persistent storage
        self.alertThresholds.heartRateMin = self.preferences.hr_min
        self.alertThresholds.heartRateMax = self.preferences.hr_max
        self.alertThresholds.spO2Min = self.preferences.spo2_min
        self.alertThresholds.batteryMin = self.preferences.bat_min
        self.alertThresholds.enabled = self.preferences.alerts_en
        self.screenBrightness = self.preferences.brightness
        
        print("Loaded default settings")
        
    def saveSettings(self):
        """Save settings to storage"""
        # In a real implementation, this would save to persistent storage
        self.preferences.hr_min = self.alertThresholds.heartRateMin
        self.preferences.hr_max = self.alertThresholds.heartRateMax
        self.preferences.spo2_min = self.alertThresholds.spO2Min
        self.preferences.bat_min = self.alertThresholds.batteryMin
        self.preferences.alerts_en = self.alertThresholds.enabled
        self.preferences.brightness = self.screenBrightness
        
        print("Settings saved")
        
    # ==================== UTILITY FUNCTIONS ====================
    def updateDisplay(self):
        """Update the display based on current screen"""
        if not self.displayOn:
            return
            
        if self.currentScreen == SCREEN_TYPE_MAIN:
            self.updateVitalSigns()
            self.drawWaveform()
            self.drawStatusBar()
            
    def formatTime(self, timestamp):
        """Format timestamp as HH:MM:SS"""
        seconds = int(timestamp)
        hours = seconds // 3600
        minutes = (seconds % 3600) // 60
        seconds = seconds % 60
        return f"{hours:02d}:{minutes:02d}:{seconds:02d}"
        
    def printSystemInfo(self):
        """Print system information to console"""
        print("\n=== System Information ===")
        print(f"Firmware Version: {FIRMWARE_VERSION}")
        print(f"Device Name: {DEVICE_NAME}")
        print(f"Free Memory: {sys.getsizeof(self)} bytes (simulated)")
        
        print(f"Sensor Status: {'Connected' if self.particleSensor.begin() else 'Disconnected'}")
        print(f"Display Status: Active")
        print(f"Touch Status: {'Active' if self.ts.begin() else 'Inactive'}")
        print(f"Data Buffer: {len(self.dataBuffer)}/{DATA_BUFFER_SIZE} entries")
        print(f"Active Alerts: {len(self.activeAlerts)}")
        print("========================")
        
    def performSelfTest(self):
        """Perform system self-test"""
        print("Performing system self-test...")
        
        testsPassed = True
        
        # Test display
        print("Testing display... ", end='')
        self.tft.fillScreen(COLOR_RED)
        time.sleep(0.5)
        self.tft.fillScreen(COLOR_GREEN)
        time.sleep(0.5)
        self.tft.fillScreen(COLOR_BLUE)
        time.sleep(0.5)
        self.tft.fillScreen(COLOR_BLACK)
        print("OK")
        
        # Test touch controller
        print("Testing touch controller... ", end='')
        if self.ts.begin():
            print("OK")
        else:
            print("FAILED")
            testsPassed = False
            
        # Test sensor
        print("Testing MAX30102 sensor... ", end='')
        if self.particleSensor.begin():
            print("OK")
        else:
            print("FAILED")
            testsPassed = False
            
        # Test buzzer (simulated)
        print("Testing buzzer... ", end='')
        print("\a", end='', flush=True)  # System bell
        time.sleep(0.2)
        print("OK")
        
        # Test battery monitor
        print("Testing battery monitor... ", end='')
        batteryLevel = self.readBatteryLevel()
        if 0 <= batteryLevel <= 100:
            print(f"OK ({batteryLevel:.1f}%)")
        else:
            print("WARNING - Unusual reading")
            
        print(f"Self-test {'PASSED' if testsPassed else 'FAILED'}")
        
        # Play test sound
        for i in range(3 if testsPassed else 5):
            print("\a", end='', flush=True)
            time.sleep(0.1 if testsPassed else 0.2)
            if i < (2 if testsPassed else 4):
                time.sleep(0.1)
                
    def handleSerialCommands(self):
        """Handle serial commands from console"""
        # In a real implementation, this would read from serial port
        pass
        
    def exportData(self):
        """Export data to console"""
        print("Exporting data...")
        print("Timestamp,HeartRate,SpO2,BatteryLevel")
        for data in self.dataBuffer:
            print(f"{data.timestamp:.1f},{data.heartRate:.1f},{data.spO2:.1f},{data.batteryLevel:.1f}")
            
        # Show confirmation on screen
        self.tft.fillRect(50, 100, 140, 60, COLOR_GREEN)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(2)
        self.tft.setCursor(60, 120)
        self.tft.println("Data Exported!")
        
        time.sleep(2)
        self.showSettingsScreen()
        
    def clearData(self):
        """Clear collected data"""
        print("Clearing data...")
        self.dataBuffer = []
        
        # Show confirmation on screen
        self.tft.fillRect(50, 100, 140, 60, COLOR_RED)
        self.tft.setTextColor(COLOR_WHITE)
        self.tft.setTextSize(2)
        self.tft.setCursor(70, 120)
        self.tft.println("Data Cleared!")
        
        time.sleep(2)
        self.showSettingsScreen()
        
    def handleLowPowerMode(self):
        """Handle low power mode"""
        if self.currentVitals.batteryLevel < 10:
            print("Entering low power mode...")
            self.screenBrightness = 50
            
            self.tft.fillRect(0, 0, self.tft.width, 20, COLOR_RED)
            self.tft.setTextColor(COLOR_WHITE)
            self.tft.setTextSize(1)
            self.tft.setCursor(5, 5)
            self.tft.println("LOW POWER MODE")
            
    def checkMemoryUsage(self):
        """Check and manage memory usage"""
        currentTime = time.time()
        if not hasattr(self, 'lastMemCheck'):
            self.lastMemCheck = 0.0
            
        if currentTime - self.lastMemCheck > 30:
            self.lastMemCheck = currentTime
            
            # Simulate memory check
            if len(self.dataBuffer) > 25:
                self.dataBuffer = self.dataBuffer[-25:]
                print("Cleaned up data buffer to free memory")
                
            if len(self.alertHistory) > 10:
                self.alertHistory = self.alertHistory[-10:]
                print("Cleaned up alert history to free memory")
                
    def initializeSystem(self):
        """Reinitialize the system"""
        print("Reinitializing system...")
        
        self.currentState = SYSTEM_STATE_INITIALIZING
        self.currentScreen = SCREEN_TYPE_MAIN
        
        # Reset buffers
        self.irBuffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        self.redBuffer = np.zeros(BUFFER_SIZE, dtype=np.uint32)
        self.bufferIndex = 0
        
        # Reset timers
        self.lastSensorUpdate = 0.0
        self.lastDisplayUpdate = 0.0
        self.lastDataLog = 0.0
        self.lastAlertCheck = 0.0
        
        # Set running state
        self.currentState = SYSTEM_STATE_RUNNING
        self.showMainScreen()
        
        print("System reinitialization complete")
        
    def handleSystemError(self, errorMessage):
        """Handle system errors"""
        print(f"SYSTEM ERROR: {errorMessage}")
        self.currentState = SYSTEM_STATE_ERROR
        
        self.showError("System Error", errorMessage)
        
        # Play error sound
        for _ in range(5):
            print("\a", end='', flush=True)
            time.sleep(0.1)
            
        time.sleep(5)
        print("Attempting system recovery...")
        self.initializeSystem()

# ==================== MAIN EXECUTION ====================
if __name__ == "__main__":
    monitor = CardiacMonitor()
    monitor.loop()