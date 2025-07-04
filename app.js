class CardiacMonitorDashboard {
    constructor() {
        this.websocket = null;
        this.heartRateChart = null;
        this.spo2Chart = null;
        this.isMonitoring = false;
        this.heartRateData = [];
        this.spo2Data = [];
        this.maxDataPoints = 50;
        
        this.init();
    }
    
    init() {
        this.setupWebSocket();
        this.setupCharts();
        this.setupEventListeners();
        this.setupModal();
    }
    
    setupWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;
        
        this.websocket = new WebSocket(wsUrl);
        
        this.websocket.onopen = () => {
            console.log('WebSocket connected');
            this.updateConnectionStatus(true);
        };
        
        this.websocket.onclose = () => {
            console.log('WebSocket disconnected');
            this.updateConnectionStatus(false);
            // Attempt to reconnect after 3 seconds
            setTimeout(() => this.setupWebSocket(), 3000);
        };
        
        this.websocket.onerror = (error) => {
            console.error('WebSocket error:', error);
            this.updateConnectionStatus(false);
        };
        
        this.websocket.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                this.handleWebSocketMessage(data);
            } catch (error) {
                console.error('Error parsing WebSocket message:', error);
            }
        };
    }
    
    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'vitals':
                this.updateVitalSigns(data);
                break;
            case 'alert':
                this.showAlert(data.message, 'danger');
                break;
            default:
                console.log('Unknown message type:', data.type);
        }
    }
    
    updateVitalSigns(data) {
        // Update vital signs display
        document.getElementById('heart-rate').textContent = 
            data.heartRate > 0 ? Math.round(data.heartRate) : '--';
        document.getElementById('spo2').textContent = 
            data.spO2 > 0 ? Math.round(data.spO2) : '--';
        document.getElementById('battery').textContent = 
            Math.round(data.batteryLevel);
        
        // Update finger detection status
        this.updateFingerStatus(data.isFingerDetected);
        
        // Add pulse animation to heart rate if valid
        const heartRateElement = document.getElementById('heart-rate');
        if (data.heartRate > 0) {
            heartRateElement.classList.add('pulse');
            setTimeout(() => heartRateElement.classList.remove('pulse'), 1000);
        }
        
        // Update charts if monitoring
        if (this.isMonitoring && data.isFingerDetected) {
            this.addDataToCharts(data.heartRate, data.spO2);
        }
        
        // Check for alerts
        this.checkVitalAlerts(data);
    }
    
    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connection-status');
        const dot = statusElement.querySelector('.status-dot');
        const text = statusElement.querySelector('span:last-child');
        
        if (connected) {
            dot.classList.add('connected');
            text.textContent = 'Connected';
        } else {
            dot.classList.remove('connected');
            text.textContent = 'Disconnected';
        }
    }
    
    updateFingerStatus(detected) {
        const statusElement = document.getElementById('finger-status');
        const dot = statusElement.querySelector('.status-dot');
        const text = statusElement.querySelector('span:last-child');
        
        if (detected) {
            dot.classList.add('connected');
            text.textContent = 'Finger Detected';
        } else {
            dot.classList.remove('connected');
            text.textContent = 'No Finger';
        }
    }
    
    setupCharts() {
        // Heart Rate Chart
        const hrCtx = document.getElementById('heartRateChart').getContext('2d');
        this.heartRateChart = new Chart(hrCtx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Heart Rate (BPM)',
                    data: [],
                    borderColor: '#e74c3c',
                    backgroundColor: 'rgba(231, 76, 60, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: false,
                        min: 40,
                        max: 200
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    }
                }
            }
        });
        
        // SpO2 Chart
        const spo2Ctx = document.getElementById('spo2Chart').getContext('2d');
        this.spo2Chart = new Chart(spo2Ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'SpO2 (%)',
                    data: [],
                    borderColor: '#3498db',
                    backgroundColor: 'rgba(52, 152, 219, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: false,
                        min: 70,
                        max: 100
                    }
                },
                plugins: {
                    legend: {
                        display: false
                    }
                }
            }
        });
    }
    
    addDataToCharts(heartRate, spO2) {
        const now = new Date().toLocaleTimeString();
        
        // Add data to heart rate chart
        if (heartRate > 0) {
            this.heartRateChart.data.labels.push(now);
            this.heartRateChart.data.datasets[0].data.push(heartRate);
            
            // Keep only last maxDataPoints
            if (this.heartRateChart.data.labels.length > this.maxDataPoints) {
                this.heartRateChart.data.labels.shift();
                this.heartRateChart.data.datasets[0].data.shift();
            }
            
            this.heartRateChart.update('none');
        }
        
        // Add data to SpO2 chart
        if (spO2 > 0) {
            this.spo2Chart.data.labels.push(now);
            this.spo2Chart.data.datasets[0].data.push(spO2);
            
            // Keep only last maxDataPoints
            if (this.spo2Chart.data.labels.length > this.maxDataPoints) {
                this.spo2Chart.data.labels.shift();
                this.spo2Chart.data.datasets[0].data.shift();
            }
            
            this.spo2Chart.update('none');
        }
    }
    
    setupEventListeners() {
        // Start monitoring button
        document.getElementById('start-monitoring').addEventListener('click', () => {
            this.startMonitoring();
        });
        
        // Stop monitoring button
        document.getElementById('stop-monitoring').addEventListener('click', () => {
            this.stopMonitoring();
        });
        
        // Download data button
        document.getElementById('download-data').addEventListener('click', () => {
            this.downloadData();
        });
        
        // Settings button
        document.getElementById('settings').addEventListener('click', () => {
            this.showSettings();
        });
    }
    
    setupModal() {
        const modal = document.getElementById('settings-modal');
        const closeBtn = modal.querySelector('.close');
        const form = document.getElementById('settings-form');
        
        closeBtn.addEventListener('click', () => {
            modal.style.display = 'none';
        });
        
        window.addEventListener('click', (event) => {
            if (event.target === modal) {
                modal.style.display = 'none';
            }
        });
        
        form.addEventListener('submit', (event) => {
            event.preventDefault();
            this.saveSettings();
        });
    }
    
    startMonitoring() {
        this.isMonitoring = true;
        document.getElementById('start-monitoring').disabled = true;
        document.getElementById('stop-monitoring').disabled = false;
        this.showAlert('Monitoring started', 'info');
        
        // Clear existing chart data
        this.heartRateChart.data.labels = [];
        this.heartRateChart.data.datasets[0].data = [];
        this.spo2Chart.data.labels = [];
        this.spo2Chart.data.datasets[0].data = [];
        this.heartRateChart.update();
        this.spo2Chart.update();
    }
    
    stopMonitoring() {
        this.isMonitoring = false;
        document.getElementById('start-monitoring').disabled = false;
        document.getElementById('stop-monitoring').disabled = true;
        this.showAlert('Monitoring stopped', 'info');
    }
    
    downloadData() {
        // Create CSV data
        const csvData = this.generateCSVData();
        const blob = new Blob([csvData], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        
        // Create download link
        const a = document.createElement('a');
        a.href = url;
        a.download = `cardiac_data_${new Date().toISOString().split('T')[0]}.csv`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
        
        this.showAlert('Data downloaded successfully', 'info');
    }
    
    generateCSVData() {
        let csv = 'Timestamp,Heart Rate (BPM),SpO2 (%)\n';
        
        const hrData = this.heartRateChart.data.datasets[0].data;
        const spo2Data = this.spo2Chart.data.datasets[0].data;
        const labels = this.heartRateChart.data.labels;
        
        for (let i = 0; i < labels.length; i++) {
            csv += `${labels[i]},${hrData[i] || ''},${spo2Data[i] || ''}\n`;
        }
        
        return csv;
    }
    
    showSettings() {
        document.getElementById('settings-modal').style.display = 'block';
    }
    
    saveSettings() {
        const hrThreshold = document.getElementById('hr-threshold').value;
        const spo2Threshold = document.getElementById('spo2-threshold').value;
        const batteryThreshold = document.getElementById('battery-threshold').value;
        
        const settings = {
            heartRateThreshold: parseInt(hrThreshold),
            spO2Threshold: parseInt(spo2Threshold),
            batteryThreshold: parseInt(batteryThreshold)
        };
        
        // Send settings to device
        fetch('/api/settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(settings)
        })
        .then(response => response.text())
        .then(data => {
            this.showAlert('Settings saved successfully', 'info');
            document.getElementById('settings-modal').style.display = 'none';
        })
        .catch(error => {
            console.error('Error saving settings:', error);
            this.showAlert('Error saving settings', 'danger');
        });
    }
    
      checkVitalAlerts(data) {
        const hrThreshold = parseInt(document.getElementById('hr-threshold').value) || 100;
        const spo2Threshold = parseInt(document.getElementById('spo2-threshold').value) || 95;
        const batteryThreshold = parseInt(document.getElementById('battery-threshold').value) || 20;
        
        // Check heart rate alerts
        if (data.heartRate > 0) {
            if (data.heartRate > hrThreshold) {
                this.showAlert(`High heart rate detected: ${Math.round(data.heartRate)} BPM`, 'danger');
            } else if (data.heartRate < 60) {
                this.showAlert(`Low heart rate detected: ${Math.round(data.heartRate)} BPM`, 'warning');
            }
        }
        
        // Check SpO2 alerts
        if (data.spO2 > 0 && data.spO2 < spo2Threshold) {
            this.showAlert(`Low SpO2 detected: ${Math.round(data.spO2)}%`, 'danger');
        }
        
        // Check battery alerts
        if (data.batteryLevel < batteryThreshold) {
            this.showAlert(`Low battery: ${Math.round(data.batteryLevel)}%`, 'warning');
        }
    }
    
    showAlert(message, type = 'info') {
        const alertsContainer = document.getElementById('alerts-container');
        const alertElement = document.createElement('div');
        alertElement.className = `alert alert-${type}`;
        alertElement.innerHTML = `
            <strong>${new Date().toLocaleTimeString()}</strong> - ${message}
            <button class="close-alert" onclick="this.parentElement.remove()">&times;</button>
        `;
        
        // Add close button styling
        const closeBtn = alertElement.querySelector('.close-alert');
        closeBtn.style.cssText = `
            float: right;
            background: none;
            border: none;
            font-size: 18px;
            cursor: pointer;
            color: inherit;
            opacity: 0.7;
        `;
        
        alertsContainer.insertBefore(alertElement, alertsContainer.firstChild);
        
        // Auto-remove alert after 10 seconds
        setTimeout(() => {
            if (alertElement.parentNode) {
                alertElement.remove();
            }
        }, 10000);
        
        // Keep only last 10 alerts
        const alerts = alertsContainer.querySelectorAll('.alert');
        if (alerts.length > 10) {
            alerts[alerts.length - 1].remove();
        }
    }
    
    // Utility method to send WebSocket messages
    sendWebSocketMessage(message) {
        if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
            this.websocket.send(JSON.stringify(message));
        }
    }
    
    // Method to request current vitals
    requestVitals() {
        this.sendWebSocketMessage({ command: 'getVitals' });
    }
    
    // Method to request system status
    requestStatus() {
        this.sendWebSocketMessage({ command: 'getStatus' });
    }
}

// Initialize the dashboard when the page loads
document.addEventListener('DOMContentLoaded', () => {
    const dashboard = new CardiacMonitorDashboard();
    
    // Request initial data every 5 seconds
    setInterval(() => {
        dashboard.requestVitals();
    }, 5000);
    
    // Request system status every 30 seconds
    setInterval(() => {
        dashboard.requestStatus();
    }, 30000);
});

// Service Worker for offline functionality (optional)
if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
        navigator.serviceWorker.register('/sw.js')
            .then(registration => {
                console.log('SW registered: ', registration);
            })
            .catch(registrationError => {
                console.log('SW registration failed: ', registrationError);
            });
    });
}
