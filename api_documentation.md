# ESP32 Cardiac Monitor - API Documentation
## REST API Endpoints
### Device Information
- **GET** `/api/info` - Get device information and status
- **Response**: `{"device": "ESP32", "firmware": "v1.0", "uptime": "hh:mm:ss"}`
### Vital Signs Data
- **GET** `/api/vitals` - Get current vital signs
- **Response**: `{"heartRate": 75, "spO2": 97, "battery": 85, "timestamp": "2025-07-03T16:36:00Z"}`
- **GET** `/api/vitals/history` - Get historical data
- **Response**: `[{"timestamp": "2025-07-03T16:00:00Z", "heartRate": 72, "spO2": 96}, ...]`
- **GET** `/api/vitals/export` - Export data as CSV
- **Response**: Download `vitals_export.csv`
### Configuration
- **GET** `/api/config` - Get current configuration
- **Response**: `{"heartRateThreshold": 100, "spO2Threshold": 95, "batteryThreshold": 20}`
- **POST** `/api/config` - Update configuration
- **Request**: `{"heartRateThreshold": 110, "spO2Threshold": 90}`
- **POST** `/api/config/reset` - Reset to defaults
- **Response**: `{"status": "reset"}`
### Alerts
- **GET** `/api/alerts` - Get alert configuration
- **Response**: `{"enabled": true, "thresholds": {"heartRate": 100, "spO2": 95}}`
- **POST** `/api/alerts` - Update alert settings
- **Request**: `{"enabled": false}`
- **GET** `/api/alerts/history` - Get alert history
- **Response**: `[{"timestamp": "2025-07-03T16:05:00Z", "type": "lowSpO2"}, ...]`
### System Control
- **POST** `/api/system/restart` - Restart device
- **Response**: `{"status": "restarting"}`
- **POST** `/api/system/calibrate` - Calibrate sensors
- **Response**: `{"status": "calibrating"}`
- **GET** `/api/system/logs` - Get system logs
- **Response**: `[{"timestamp": "2025-07-03T16:00:00Z", "message": "Boot successful"}, ...]`
## WebSocket Events
### Client to Server
```json
{
  "type": "subscribe",
  "data": "vitals"
}
```
### Server to Client
```json
{
  "type": "vitals_update",
  "data": {"heartRate": 75, "spO2": 97, "timestamp": "2025-07-03T16:36:00Z"}
}
```
## Security Notes
- All endpoints require HTTPS/WSS
- Authentication via API key (header: `X-API-Key`)
- Rate limiting: 100 requests/minute