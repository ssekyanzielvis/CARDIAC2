#!/bin/bash
# ESP32 Cardiac Monitor - Installation Script

echo "ESP32 Cardiac Monitor Installation Script"
echo "========================================"

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo "Python3 is required but not installed. Please install Python3 first."
    exit 1
fi

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    echo "Installing PlatformIO..."
    pip3 install platformio
fi

# Create project directory
PROJECT_DIR="esp32-cardiac-monitor"
if [ ! -d "$PROJECT_DIR" ]; then
    echo "Creating project directory..."
    mkdir -p $PROJECT_DIR
    cd $PROJECT_DIR
    
    # Initialize PlatformIO project
    pio project init --board esp32dev
    
    echo "Project structure created successfully!"
else
    echo "Project directory already exists."
    cd $PROJECT_DIR
fi

# Install required libraries
echo "Installing required libraries..."
pio lib install "MAX30105 library"
pio lib install "Adafruit ILI9341"
pio lib install "ArduinoJson"
pio lib install "ESPAsyncWebServer"
pio lib install "AsyncTCP"

# Create basic project structure
mkdir -p src data include lib test

echo "Installation completed successfully!"
echo "Next steps:"
echo "1. Copy your source code to src/ directory"
echo "2. Copy web files to data/ directory"
echo "3. Configure WiFi settings"
echo "4. Build and upload: pio run --target upload"
echo "5. Upload filesystem: pio run --target uploadfs"
