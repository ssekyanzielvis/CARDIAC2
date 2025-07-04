# ESP32 Cardiac Monitor - Troubleshooting Guide
## Common Issues and Solutions
### 1. Device Does Not Boot
- **Symptom**: No display output, no serial log
- **Cause**: Power supply issue or hardware failure
- **Solution**: Check power connections, ensure 3.3V/5V supply, verify ESP32 is seated correctly
### 2. Sensor Readings Inaccurate
- **Symptom**: Erratic heart rate or SpO2 values
- **Cause**: Poor finger contact, sensor misalignment, or noise
- **Solution**: Ensure proper finger placement, clean sensor, recalibrate via `/api/system/calibrate`
### 3. WiFi Connection Fails
- **Symptom**: No IP address in serial log
- **Cause**: Incorrect credentials or network issues
- **Solution**: Verify WiFi SSID/password in `config.json`, ensure router is in range
### 4. Web Interface Unreachable
- **Symptom**: Cannot access `cardiac-monitor.local`
- **Cause**: mDNS not enabled, firewall blocking
- **Solution**: Enable mDNS, check router firewall settings, use IP address directly
### 5. Alerts Not Triggering
- **Symptom**: No audio/visual alerts despite threshold breach
- **Cause**: Alert thresholds not configured, buzzer disconnected
- **Solution**: Check `/api/alerts`, verify buzzer wiring, test with manual trigger
### 6. Data Logging Fails
- **Symptom**: No data in `/logs/vitals.json`
- **Cause**: SPIFFS full or file corruption
- **Solution**: Format SPIFFS via PlatformIO, ensure sufficient storage (min 1MB)
## Advanced Diagnostics
- **Serial Monitor**: Enable for real-time logs (115200 baud)
- **Log File**: Review `/logs/system.log` for errors
- **Firmware Update**: Use OTA or reflash via PlatformIO if issues persist
## Contact Support
- Email: support@xai.com
- Last Updated: 04:36 PM EAT, Jul 03, 2025