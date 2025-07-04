import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.patches import FancyBboxPatch, ConnectionPatch
import numpy as np
import pandas as pd
import os
import stat

# CardiacMonitorArchitecture Class (unchanged from previous)
class CardiacMonitorArchitecture:
    def __init__(self):
        self.fig, self.ax = plt.subplots(1, 1, figsize=(20, 14))
        self.ax.set_xlim(0, 20)
        self.ax.set_ylim(0, 14)
        self.ax.set_aspect('equal')
        self.ax.axis('off')
        
        self.colors = {
            'hardware': '#FF6B6B',
            'firmware': '#4ECDC4',
            'web': '#45B7D1',
            'data': '#96CEB4',
            'communication': '#FFEAA7',
            'user': '#DDA0DD',
            'alerts': '#FFB347'
        }
        
    def create_component_box(self, x, y, width, height, text, color, text_size=10):
        box = FancyBboxPatch((x, y), width, height, boxstyle="round,pad=0.1", facecolor=color,
                             edgecolor='black', linewidth=1.5, alpha=0.8)
        self.ax.add_patch(box)
        self.ax.text(x + width/2, y + height/2, text, ha='center', va='center', fontsize=text_size,
                     fontweight='bold', wrap=True)
        return box
    
    def create_connection(self, start_pos, end_pos, style='solid', color='black', width=2):
        line = ConnectionPatch(start_pos, end_pos, "data", "data", arrowstyle="->", shrinkA=5, shrinkB=5,
                              mutation_scale=20, fc=color, ec=color, linewidth=width, linestyle=style)
        self.ax.add_patch(line)
        
    def draw_hardware_layer(self):
        self.ax.text(10, 13.5, 'ESP32 Cardiac Monitor - System Architecture (Updated: 04:36 PM EAT, Jul 03, 2025)',
                     ha='center', va='center', fontsize=18, fontweight='bold')
        self.ax.text(1, 12.5, 'HARDWARE LAYER', fontsize=14, fontweight='bold', color=self.colors['hardware'])
        self.create_component_box(1, 11, 3, 1, 'ESP32\nMicrocontroller\n(Dual Core)', self.colors['hardware'], 9)
        self.create_component_box(5, 11.5, 2.5, 0.8, 'MAX30102\nHeart Rate &\nSpO2 Sensor', self.colors['hardware'], 8)
        self.create_component_box(8, 11.5, 2, 0.8, 'Battery\nMonitor\n(ADC)', self.colors['hardware'], 8)
        self.create_component_box(11, 11.5, 2.5, 0.8, 'ILI9341\n2.8" TFT\nTouch Display', self.colors['hardware'], 8)
        self.create_component_box(14.5, 11.5, 1.5, 0.8, 'Buzzer\nAlerts', self.colors['hardware'], 8)
        self.create_component_box(16.5, 11.5, 2.5, 0.8, 'WiFi\nModule\n(Built-in)', self.colors['hardware'], 8)
        
    def draw_firmware_layer(self):
        self.ax.text(1, 10.2, 'FIRMWARE LAYER', fontsize=14, fontweight='bold', color=self.colors['firmware'])
        self.create_component_box(1, 9, 3, 1, 'Main Application\n(cardiac_monitor.ino)\nTask Scheduler', self.colors['firmware'], 9)
        self.create_component_box(5, 9.5, 2, 0.8, 'Heart Rate\nCalculation\nLibrary', self.colors['firmware'], 8)
        self.create_component_box(7.5, 9.5, 2, 0.8, 'SpO2\nCalculation\nLibrary', self.colors['firmware'], 8)
        self.create_component_box(10, 9.5, 2.5, 0.8, 'UI Elements\nTouch Handler\nDisplay Manager', self.colors['firmware'], 8)
        self.create_component_box(13, 9.5, 2, 0.8, 'Config\nManager\nSPIFFS', self.colors['firmware'], 8)
        self.create_component_box(15.5, 9.5, 2, 0.8, 'Alert\nManager\nBuzzer Control', self.colors['alerts'], 8)
        self.create_component_box(18, 9.5, 1.5, 0.8, 'Web Server\nWebSocket\nAPI', self.colors['firmware'], 8)
        
    def draw_data_layer(self):
        self.ax.text(1, 8.2, 'DATA LAYER', fontsize=14, fontweight='bold', color=self.colors['data'])
        self.create_component_box(1, 7, 2.5, 1, 'Vital Signs\nData Structure\nReal-time Buffer', self.colors['data'], 9)
        self.create_component_box(4, 7.5, 2.5, 0.8, 'Signal Processing\nFiltering\nValidation', self.colors['data'], 8)
        self.create_component_box(7, 7.5, 2.5, 0.8, 'Data Logger\nSPIFFS Storage\nJSON Format', self.colors['data'], 8)
        self.create_component_box(10, 7.5, 2.5, 0.8, 'Trend Analysis\nThreshold Check\nAlert Logic', self.colors['data'], 8)
        self.create_component_box(13, 7.5, 2, 0.8, 'Data Export\nCSV Generator\nAPI Endpoints', self.colors['data'], 8)
        
    def draw_communication_layer(self):
        self.ax.text(1, 6.2, 'COMMUNICATION LAYER', fontsize=14, fontweight='bold', color=self.colors['communication'])
        self.create_component_box(1, 5, 2.5, 1, 'WiFi Manager\nConnection\nReconnection', self.colors['communication'], 9)
        self.create_component_box(4, 5.5, 2.5, 0.8, 'HTTP Server\nREST API\nFile Server', self.colors['communication'], 8)
        self.create_component_box(7, 5.5, 2.5, 0.8, 'WebSocket Server\nReal-time Data\nBidirectional', self.colors['communication'], 8)
        self.create_component_box(10, 5.5, 2, 0.8, 'mDNS\nService\nDiscovery', self.colors['communication'], 8)
        self.create_component_box(12.5, 5.5, 2, 0.8, 'OTA Updates\nFirmware\nSPIFFS', self.colors['communication'], 8)
        
    def draw_web_interface_layer(self):
        self.ax.text(1, 4.2, 'WEB INTERFACE LAYER', fontsize=14, fontweight='bold', color=self.colors['web'])
        self.create_component_box(1, 3, 3, 1, 'HTML5 Dashboard\nResponsive Design\nPWA Support', self.colors['web'], 9)
        self.create_component_box(4.5, 3.5, 2, 0.8, 'CSS3 Styling\nAnimations\nResponsive', self.colors['web'], 8)
        self.create_component_box(7, 3.5, 2.5, 0.8, 'JavaScript App\nWebSocket Client\nChart.js', self.colors['web'], 8)
        self.create_component_box(10, 3.5, 2, 0.8, 'Real-time\nCharts\nTrend Analysis', self.colors['web'], 8)
        self.create_component_box(12.5, 3.5, 2, 0.8, 'Service Worker\nOffline Support\nCaching', self.colors['web'], 8)
        self.create_component_box(15, 3.5, 2, 0.8, 'Settings Panel\nConfiguration\nThresholds', self.colors['web'], 8)
        
    def draw_user_interface_layer(self):
        self.ax.text(1, 2.2, 'USER INTERFACE LAYER', fontsize=14, fontweight='bold', color=self.colors['user'])
        self.create_component_box(1, 1, 3, 1, 'Local TFT Display\nTouch Interface\nReal-time Vitals', self.colors['user'], 9)
        self.create_component_box(5, 1.5, 2.5, 0.8, 'Web Browser\nDesktop/Mobile\nRemote Access', self.colors['user'], 8)
        self.create_component_box(8, 1.5, 2, 0.8, 'Mobile PWA\nOffline Capable\nNotifications', self.colors['user'], 8)
        self.create_component_box(10.5, 1.5, 2, 0.8, 'Visual Alerts\nAudio Alerts\nNotifications', self.colors['alerts'], 8)
        self.create_component_box(13, 1.5, 2, 0.8, 'Data Download\nCSV Export\nReports', self.colors['user'], 8)
        self.create_component_box(15.5, 1.5, 2, 0.8, 'Settings UI\nConfiguration\nCalibration', self.colors['user'], 8)
        
    def draw_connections(self):
        self.create_connection((2.5, 11), (2.5, 10), color='red', width=2)
        self.create_connection((6.25, 11.5), (6, 10.3), color='red')
        self.create_connection((9, 11.5), (8.5, 10.3), color='red')
        self.create_connection((12.25, 11.5), (11.25, 10.3), color='red')
        self.create_connection((15.25, 11.5), (16.5, 10.3), color='red')
        self.create_connection((17.75, 11.5), (18.75, 10.3), color='red')
        self.create_connection((2.5, 9), (2.25, 8), color='blue', width=2)
        self.create_connection((6, 9.5), (5.25, 8.3), color='blue')
        self.create_connection((8.5, 9.5), (8.25, 8.3), color='blue')
        self.create_connection((2.25, 7), (2.25, 6), color='green', width=2)
        self.create_connection((8.25, 7.5), (8.25, 6.3), color='green')
        self.create_connection((14, 7.5), (5.25, 6.3), color='green')
        self.create_connection((5.25, 5.5), (2.5, 4), color='purple', width=2)
        self.create_connection((8.25, 5.5), (8.25, 4.3), color='purple')
        self.create_connection((2.5, 3), (2.5, 2), color='orange', width=2)
        self.create_connection((8.25, 3.5), (6.25, 2.3), color='orange')
        self.create_connection((11.25, 9.5), (2.5, 2), color='brown', style='dashed')
        
    def add_data_flow_arrows(self):
        self.ax.annotate('Sensor Data', xy=(6.25, 10.8), xytext=(6.25, 10.5),
                        arrowprops=dict(arrowstyle='->', color='red', lw=2),
                        fontsize=8, ha='center', color='red')
        self.ax.annotate('Real-time\nData Stream', xy=(8.25, 6.8), xytext=(8.25, 6.5),
                        arrowprops=dict(arrowstyle='->', color='green', lw=2),
                        fontsize=8, ha='center', color='green')
        self.ax.annotate('HTTP/WebSocket', xy=(5.25, 4.8), xytext=(5.25, 4.5),
                        arrowprops=dict(arrowstyle='->', color='purple', lw=2),
                        fontsize=8, ha='center', color='purple')
        self.ax.annotate('User\nInteraction', xy=(2.5, 2.8), xytext=(2.5, 2.5),
                        arrowprops=dict(arrowstyle='<->', color='orange', lw=2),
                        fontsize=8, ha='center', color='orange')
        
    def add_legend(self):
        legend_x, legend_y = 0.5, 0.5
        legend_items = [('Hardware', self.colors['hardware']), ('Firmware', self.colors['firmware']),
                        ('Data Processing', self.colors['data']), ('Communication', self.colors['communication']),
                        ('Web Interface', self.colors['web']), ('User Interface', self.colors['user']),
                        ('Alerts', self.colors['alerts'])]
        for i, (label, color) in enumerate(legend_items):
            y_pos = legend_y - i * 0.15
            self.create_component_box(legend_x, y_pos, 1.5, 0.1, '', color, 8)
            self.ax.text(legend_x + 1.6, y_pos + 0.05, label, fontsize=8, va='center', fontweight='bold')
    
    def generate_architecture(self):
        self.draw_hardware_layer()
        self.draw_firmware_layer()
        self.draw_data_layer()
        self.draw_communication_layer()
        self.draw_web_interface_layer()
        self.draw_user_interface_layer()
        self.draw_connections()
        self.add_data_flow_arrows()
        self.add_legend()
        plt.tight_layout()
        plt.savefig('cardiac_monitor_architecture.png', dpi=300, bbox_inches='tight')
        plt.show()

# Additional Utility Functions (unchanged from previous)
def generate_component_details():
    components = {
        'Hardware Components': {
            'ESP32': {'Type': 'Microcontroller', 'Features': ['Dual-core Xtensa LX6', 'WiFi & Bluetooth', '520KB SRAM', 'GPIO pins'], 'Purpose': 'Main processing unit and connectivity'},
            'MAX30102': {'Type': 'Biometric Sensor', 'Features': ['Heart rate detection', 'SpO2 measurement', 'I2C interface', 'Low power'], 'Purpose': 'Vital signs monitoring'},
            'ILI9341': {'Type': 'TFT Display', 'Features': ['320x240 resolution', 'Touch interface', 'SPI communication', '2.8 inch'], 'Purpose': 'Local user interface'},
            'Buzzer': {'Type': 'Audio Output', 'Features': ['Piezo buzzer', 'Alert tones', 'Low power consumption'], 'Purpose': 'Audio alerts and notifications'}
        },
        'Software Components': {
            'Main Application': {'Language': 'C++/Arduino', 'Features': ['Task scheduling', 'Sensor management', 'UI control', 'Data processing'], 'Purpose': 'Core system orchestration'},
            'Web Server': {'Type': 'HTTP/WebSocket Server', 'Features': ['REST API', 'Real-time communication', 'File serving', 'CORS support'], 'Purpose': 'Remote access and control'},
            'Data Logger': {'Type': 'Storage System', 'Features': ['SPIFFS filesystem', 'JSON format', 'Circular buffer', 'Export capability'], 'Purpose': 'Historical data management'},
            'Alert System': {'Type': 'Monitoring System', 'Features': ['Threshold checking', 'Multi-level alerts', 'Audio/visual output', 'Configurable'], 'Purpose': 'Health monitoring and warnings'}
        }
    }
    return components

def create_data_flow_diagram():
    fig, ax = plt.subplots(1, 1, figsize=(16, 10))
    ax.set_xlim(0, 16); ax.set_ylim(0, 10); ax.set_aspect('equal'); ax.axis('off')
    ax.text(8, 9.5, 'Cardiac Monitor - Data Flow Diagram', ha='center', va='center', fontsize=16, fontweight='bold')
    stages = [{'name': 'Sensor\nAcquisition', 'pos': (1, 7), 'color': '#FF6B6B'}, {'name': 'Signal\nProcessing', 'pos': (4, 7), 'color': '#4ECDC4'},
              {'name': 'Data\nValidation', 'pos': (7, 7), 'color': '#45B7D1'}, {'name': 'Storage &\nLogging', 'pos': (10, 7), 'color': '#96CEB4'},
              {'name': 'Display &\nTransmission', 'pos': (13, 7), 'color': '#FFEAA7'}, {'name': 'Alert\nProcessing', 'pos': (7, 4), 'color': '#FFB347'},
              {'name': 'User\nInterface', 'pos': (4, 1), 'color': '#DDA0DD'}, {'name': 'Web\nInterface', 'pos': (10, 1), 'color': '#87CEEB'}]
    for stage in stages:
        box = FancyBboxPatch((stage['pos'][0]-0.8, stage['pos'][1]-0.5), 1.6, 1, boxstyle="round,pad=0.1", facecolor=stage['color'],
                             edgecolor='black', linewidth=1.5, alpha=0.8)
        ax.add_patch(box); ax.text(stage['pos'][0], stage['pos'][1], stage['name'], ha='center', va='center', fontsize=10, fontweight='bold')
    connections = [((1.8, 7), (3.2, 7), 'Raw PPG\nSignal'), ((4.8, 7), (6.2, 7), 'Filtered\nData'), ((7.8, 7), (9.2, 7), 'Valid\nVitals'),
                   ((10.8, 7), (12.2, 7), 'Stored\nData'), ((7, 6.5), (7, 4.5), 'Threshold\nCheck'), ((6.2, 4), (4.8, 1.5), 'Local\nAlerts'),
                   ((7.8, 4), (9.2, 1.5), 'Remote\nAlerts'), ((13, 6.5), (10.8, 1.5), 'Web\nData'), ((13, 6.5), (4.8, 1.5), 'Display\nData')]
    for start, end, label in connections:
        ax.annotate('', xy=end, xytext=start, arrowprops=dict(arrowstyle='->', lw=2, color='black'))
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.2, label, ha='center', va='center', fontsize=8, bbox=dict(boxstyle="round,pad=0.2", facecolor='white', alpha=0.8))
    plt.tight_layout(); plt.savefig('cardiac_monitor_dataflow.png', dpi=300, bbox_inches='tight'); plt.show()

def create_system_states_diagram():
    fig, ax = plt.subplots(1, 1, figsize=(14, 10))
    ax.set_xlim(0, 14); ax.set_ylim(0, 10); ax.set_aspect('equal'); ax.axis('off')
    ax.text(7, 9.5, 'Cardiac Monitor - System States Diagram', ha='center', va='center', fontsize=16, fontweight='bold')
    states = [{'name': 'BOOT', 'pos': (2, 8), 'color': '#FF6B6B'}, {'name': 'INIT', 'pos': (5, 8), 'color': '#4ECDC4'},
              {'name': 'IDLE', 'pos': (8, 8), 'color': '#45B7D1'}, {'name': 'MONITORING', 'pos': (11, 8), 'color': '#96CEB4'},
              {'name': 'ALERT', 'pos': (11, 5), 'color': '#FFB347'}, {'name': 'CONFIG', 'pos': (8, 5), 'color': '#FFEAA7'},
              {'name': 'DATA_EXPORT', 'pos': (5, 5), 'color': '#DDA0DD'}, {'name': 'ERROR', 'pos': (2, 5), 'color': '#FF4757'},
              {'name': 'SLEEP', 'pos': (8, 2), 'color': '#A4B0BE'}]
    for state in states:
        circle = plt.Circle(state['pos'], 0.8, facecolor=state['color'], edgecolor='black', linewidth=2, alpha=0.8)
        ax.add_patch(circle); ax.text(state['pos'][0], state['pos'][1], state['name'], ha='center', va='center', fontsize=9, fontweight='bold')
    transitions = [((2.8, 8), (4.2, 8), 'Hardware\nInit'), ((5.8, 8), (7.2, 8), 'System\nReady'), ((8.8, 8), (10.2, 8), 'Finger\nDetected'),
                   ((11, 7.2), (11, 5.8), 'Threshold\nExceeded'), ((10.2, 5), (8.8, 5), 'Settings\nRequest'), ((7.2, 5), (5.8, 5), 'Export\nRequest'),
                   ((4.2, 5), (2.8, 5), 'System\nError'), ((8, 4.2), (8, 2.8), 'Inactivity\nTimeout'), ((8, 2.8), (8, 4.2), 'Wake Up\nEvent'),
                   ((10.2, 8), (8.8, 8), 'Finger\nRemoved'), ((11, 5.8), (11, 7.2), 'Alert\nAcknowledged')]
    for start, end, label in transitions:
        ax.annotate('', xy=end, xytext=start, arrowprops=dict(arrowstyle='->', lw=1.5, color='blue'))
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.3, label, ha='center', va='center', fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='lightblue', alpha=0.7))
    plt.tight_layout(); plt.savefig('cardiac_monitor_states.png', dpi=300, bbox_inches='tight'); plt.show()

def create_network_topology():
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    ax.set_xlim(0, 12); ax.set_ylim(0, 8); ax.set_aspect('equal'); ax.axis('off')
    ax.text(6, 7.5, 'Cardiac Monitor - Network Topology', ha='center', va='center', fontsize=16, fontweight='bold')
    components = [{'name': 'WiFi Router\n192.168.1.1', 'pos': (6, 6), 'color': '#4ECDC4', 'shape': 'rect'},
                  {'name': 'ESP32 Monitor\n192.168.1.100', 'pos': (3, 4), 'color': '#FF6B6B', 'shape': 'circle'},
                  {'name': 'Desktop PC\n192.168.1.50', 'pos': (9, 4), 'color': '#45B7D1', 'shape': 'rect'},
                  {'name': 'Mobile Device\n192.168.1.75', 'pos': (6, 2), 'color': '#96CEB4', 'shape': 'rect'},
                  {'name': 'Tablet\n192.168.1.80', 'pos': (2, 2), 'color': '#FFEAA7', 'shape': 'rect'},
                  {'name': 'Cloud Storage\n(Optional)', 'pos': (10, 6), 'color': '#DDA0DD', 'shape': 'cloud'}]
    for comp in components:
        if comp['shape'] == 'circle':
            circle = plt.Circle(comp['pos'], 0.6, facecolor=comp['color'], edgecolor='black', linewidth=2, alpha=0.8)
            ax.add_patch(circle)
        elif comp['shape'] == 'rect':
            rect = FancyBboxPatch((comp['pos'][0]-0.8, comp['pos'][1]-0.4), 1.6, 0.8, boxstyle="round,pad=0.1",
                                  facecolor=comp['color'], edgecolor='black', linewidth=1.5, alpha=0.8)
            ax.add_patch(rect)
        elif comp['shape'] == 'cloud':
            ellipse = patches.Ellipse(comp['pos'], 1.5, 0.8, facecolor=comp['color'], edgecolor='black', linewidth=1.5, alpha=0.8)
            ax.add_patch(ellipse)
        ax.text(comp['pos'][0], comp['pos'][1], comp['name'], ha='center', va='center', fontsize=8, fontweight='bold')
    connections = [((6, 5.4), (3.6, 4.6), 'WiFi\n2.4GHz'), ((6, 5.4), (8.4, 4.6), 'Ethernet/WiFi'),
                   ((6, 5.4), (6, 2.8), 'WiFi\n5GHz'), ((6, 5.4), (2.8, 2.6), 'WiFi\n2.4GHz'), ((6.8, 6), (9.2, 6), 'Internet\n(Optional)')]
    for start, end, label in connections:
        ax.plot([start[0], end[0]], [start[1], end[1]], 'k-', linewidth=2)
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.2, label, ha='center', va='center', fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='white', alpha=0.8))
    protocols = ['HTTP: Port 80 (Web Interface)', 'WebSocket: Port 81 (Real-time Data)', 'mDNS: cardiac-monitor.local', 'OTA: Port 3232 (Updates)']
    for i, protocol in enumerate(protocols):
        ax.text(0.5, 1 - i*0.2, protocol, fontsize=9, bbox=dict(boxstyle="round,pad=0.3", facecolor='lightgray', alpha=0.7))
    plt.tight_layout(); plt.savefig('cardiac_monitor_network.png', dpi=300, bbox_inches='tight'); plt.show()

def create_security_architecture():
    fig, ax = plt.subplots(1, 1, figsize=(14, 10))
    ax.set_xlim(0, 14); ax.set_ylim(0, 10); ax.set_aspect('equal'); ax.axis('off')
    ax.text(7, 9.5, 'Cardiac Monitor - Security Architecture', ha='center', va='center', fontsize=16, fontweight='bold')
    layers = [{'name': 'Physical Security', 'pos': (7, 8.5), 'width': 12, 'height': 0.8, 'color': '#FF6B6B'},
              {'name': 'Network Security', 'pos': (7, 7.5), 'width': 10, 'height': 0.8, 'color': '#4ECDC4'},
              {'name': 'Application Security', 'pos': (7, 6.5), 'width': 8, 'height': 0.8, 'color': '#45B7D1'},
              {'name': 'Data Security', 'pos': (7, 5.5), 'width': 6, 'height': 0.8, 'color': '#96CEB4'}]
    for layer in layers:
        rect = FancyBboxPatch((layer['pos'][0] - layer['width']/2, layer['pos'][1] - layer['height']/2), layer['width'], layer['height'],
                              boxstyle="round,pad=0.1", facecolor=layer['color'], edgecolor='black', linewidth=1.5, alpha=0.3)
        ax.add_patch(rect); ax.text(layer['pos'][0], layer['pos'][1], layer['name'], ha='center', va='center', fontsize=12, fontweight='bold')
    security_components = [{'name': 'Device\nEnclosure', 'pos': (2, 8.5), 'color': '#FF6B6B'}, {'name': 'Tamper\nDetection', 'pos': (5, 8.5), 'color': '#FF6B6B'},
                          {'name': 'Secure Boot', 'pos': (9, 8.5), 'color': '#FF6B6B'}, {'name': 'Hardware\nRNG', 'pos': (12, 8.5), 'color': '#FF6B6B'},
                          {'name': 'WPA2/WPA3\nEncryption', 'pos': (3, 7.5), 'color': '#4ECDC4'}, {'name': 'MAC Address\nFiltering', 'pos': (7, 7.5), 'color': '#4ECDC4'},
                          {'name': 'Firewall\nRules', 'pos': (11, 7.5), 'color': '#4ECDC4'}, {'name': 'HTTPS/WSS\nEncryption', 'pos': (4, 6.5), 'color': '#45B7D1'},
                          {'name': 'Input\nValidation', 'pos': (7, 6.5), 'color': '#45B7D1'}, {'name': 'Session\nManagement', 'pos': (10, 6.5), 'color': '#45B7D1'},
                          {'name': 'Data\nEncryption', 'pos': (5, 5.5), 'color': '#96CEB4'}, {'name': 'Access\nControl', 'pos': (7, 5.5), 'color': '#96CEB4'},
                          {'name': 'Audit\nLogging', 'pos': (9, 5.5), 'color': '#96CEB4'}]
    for comp in security_components:
        box = FancyBboxPatch((comp['pos'][0]-0.6, comp['pos'][1]-0.3), 1.2, 0.6, boxstyle="round,pad=0.05", facecolor=comp['color'],
                             edgecolor='black', linewidth=1, alpha=0.8)
        ax.add_patch(box); ax.text(comp['pos'][0], comp['pos'][1], comp['name'], ha='center', va='center', fontsize=8, fontweight='bold')
    threats = [{'name': 'Physical\nTampering', 'pos': (1, 4), 'color': '#FF4757'}, {'name': 'Network\nEavesdropping', 'pos': (4, 4), 'color': '#FF4757'},
               {'name': 'Web\nAttacks', 'pos': (7, 4), 'color': '#FF4757'}, {'name': 'Data\nBreach', 'pos': (10, 4), 'color': '#FF4757'},
               {'name': 'Firmware\nModification', 'pos': (13, 4), 'color': '#FF4757'}]
    ax.text(7, 3.5, 'Potential Threats', ha='center', va='center', fontsize=12, fontweight='bold', color='red')
    for threat in threats:
        triangle = patches.RegularPolygon(threat['pos'], 3, radius=0.4, facecolor=threat['color'], edgecolor='darkred', linewidth=2, alpha=0.8)
        ax.add_patch(triangle); ax.text(threat['pos'][0], threat['pos'][1]-0.8, threat['name'], ha='center', va='center', fontsize=8, fontweight='bold', color='red')
    mitigations = [((2, 8.2), (1, 4.4)), ((5, 7.2), (4, 4.4)), ((7, 6.2), (7, 4.4)), ((7, 5.2), (10, 4.4)), ((9, 8.2), (13, 4.4))]
    for start, end in mitigations:
        ax.annotate('', xy=end, xytext=start, arrowprops=dict(arrowstyle='->', lw=2, color='green', alpha=0.7))
    plt.tight_layout(); plt.savefig('cardiac_monitor_security.png', dpi=300, bbox_inches='tight'); plt.show()

def generate_deployment_guide():
    deployment_steps = {'Hardware Setup': ['Connect MAX30102 sensor to ESP32 I2C pins (SDA=21, SCL=22)', 'Connect ILI9341 display to ESP32 SPI pins',
                                           'Connect buzzer to GPIO pin 27', 'Connect battery monitor to ADC pin 35', 'Assemble components in enclosure'],
                        'Software Installation': ['Install PlatformIO IDE', 'Clone project repository', 'Configure WiFi credentials in data/config.json',
                                                 'Build and upload firmware to ESP32', 'Upload SPIFFS filesystem with web files', 'Verify serial output for successful boot'],
                        'Network Configuration': ['Connect ESP32 to WiFi network', 'Note assigned IP address from serial monitor', 'Configure router for static IP (optional)',
                                                 'Enable mDNS for cardiac-monitor.local access', 'Test web interface connectivity'],
                        'Calibration & Testing': ['Place finger on sensor for initial readings', 'Verify heart rate and SpO2 accuracy', 'Test alert thresholds and buzzer',
                                                 'Calibrate display touch sensitivity', 'Perform data logging and export tests'],
                        'Production Deployment': ['Set secure WiFi credentials', 'Configure appropriate alert thresholds', 'Enable data logging and retention policies',
                                                 'Set up backup and recovery procedures', 'Document system configuration and contacts']}
    with open('deployment_guide.txt', 'w', encoding='utf-8') as f:
        f.write("ESP32 Cardiac Monitor - Deployment Guide\n"); f.write("=" * 50 + "\n\n")
        f.write(f"Last Updated: 04:36 PM EAT, Jul 03, 2025\n\n")
        for section, steps in deployment_steps.items():
            f.write(f"{section}:\n"); f.write("-" * len(section) + "\n")
            for i, step in enumerate(steps, 1): f.write(f"{i}. {step}\n"); f.write("\n")
    print("Deployment guide saved to deployment_guide.txt"); return deployment_steps

def generate_api_documentation():
    api_docs = """# ESP32 Cardiac Monitor - API Documentation\n## REST API Endpoints\n### Device Information\n- **GET** `/api/info` - Get device information and status\n- **Response**: `{"device": "ESP32", "firmware": "v1.0", "uptime": "hh:mm:ss"}`\n### Vital Signs Data\n- **GET** `/api/vitals` - Get current vital signs\n- **Response**: `{"heartRate": 75, "spO2": 97, "battery": 85, "timestamp": "2025-07-03T16:36:00Z"}`\n- **GET** `/api/vitals/history` - Get historical data\n- **Response**: `[{"timestamp": "2025-07-03T16:00:00Z", "heartRate": 72, "spO2": 96}, ...]`\n- **GET** `/api/vitals/export` - Export data as CSV\n- **Response**: Download `vitals_export.csv`\n### Configuration\n- **GET** `/api/config` - Get current configuration\n- **Response**: `{"heartRateThreshold": 100, "spO2Threshold": 95, "batteryThreshold": 20}`\n- **POST** `/api/config` - Update configuration\n- **Request**: `{"heartRateThreshold": 110, "spO2Threshold": 90}`\n- **POST** `/api/config/reset` - Reset to defaults\n- **Response**: `{"status": "reset"}`\n### Alerts\n- **GET** `/api/alerts` - Get alert configuration\n- **Response**: `{"enabled": true, "thresholds": {"heartRate": 100, "spO2": 95}}`\n- **POST** `/api/alerts` - Update alert settings\n- **Request**: `{"enabled": false}`\n- **GET** `/api/alerts/history` - Get alert history\n- **Response**: `[{"timestamp": "2025-07-03T16:05:00Z", "type": "lowSpO2"}, ...]`\n### System Control\n- **POST** `/api/system/restart` - Restart device\n- **Response**: `{"status": "restarting"}`\n- **POST** `/api/system/calibrate` - Calibrate sensors\n- **Response**: `{"status": "calibrating"}`\n- **GET** `/api/system/logs` - Get system logs\n- **Response**: `[{"timestamp": "2025-07-03T16:00:00Z", "message": "Boot successful"}, ...]`\n## WebSocket Events\n### Client to Server\n```json\n{\n  "type": "subscribe",\n  "data": "vitals"\n}\n```\n### Server to Client\n```json\n{\n  "type": "vitals_update",\n  "data": {"heartRate": 75, "spO2": 97, "timestamp": "2025-07-03T16:36:00Z"}\n}\n```\n## Security Notes\n- All endpoints require HTTPS/WSS\n- Authentication via API key (header: `X-API-Key`)\n- Rate limiting: 100 requests/minute"""
    with open('api_documentation.md', 'w', encoding='utf-8') as f: f.write(api_docs); print("API documentation saved to api_documentation.md")

def generate_troubleshooting_guide():
    troubleshooting = """# ESP32 Cardiac Monitor - Troubleshooting Guide\n## Common Issues and Solutions\n### 1. Device Does Not Boot\n- **Symptom**: No display output, no serial log\n- **Cause**: Power supply issue or hardware failure\n- **Solution**: Check power connections, ensure 3.3V/5V supply, verify ESP32 is seated correctly\n### 2. Sensor Readings Inaccurate\n- **Symptom**: Erratic heart rate or SpO2 values\n- **Cause**: Poor finger contact, sensor misalignment, or noise\n- **Solution**: Ensure proper finger placement, clean sensor, recalibrate via `/api/system/calibrate`\n### 3. WiFi Connection Fails\n- **Symptom**: No IP address in serial log\n- **Cause**: Incorrect credentials or network issues\n- **Solution**: Verify WiFi SSID/password in `config.json`, ensure router is in range\n### 4. Web Interface Unreachable\n- **Symptom**: Cannot access `cardiac-monitor.local`\n- **Cause**: mDNS not enabled, firewall blocking\n- **Solution**: Enable mDNS, check router firewall settings, use IP address directly\n### 5. Alerts Not Triggering\n- **Symptom**: No audio/visual alerts despite threshold breach\n- **Cause**: Alert thresholds not configured, buzzer disconnected\n- **Solution**: Check `/api/alerts`, verify buzzer wiring, test with manual trigger\n### 6. Data Logging Fails\n- **Symptom**: No data in `/logs/vitals.json`\n- **Cause**: SPIFFS full or file corruption\n- **Solution**: Format SPIFFS via PlatformIO, ensure sufficient storage (min 1MB)\n## Advanced Diagnostics\n- **Serial Monitor**: Enable for real-time logs (115200 baud)\n- **Log File**: Review `/logs/system.log` for errors\n- **Firmware Update**: Use OTA or reflash via PlatformIO if issues persist\n## Contact Support\n- Email: support@xai.com\n- Last Updated: 04:36 PM EAT, Jul 03, 2025"""
    with open('troubleshooting_guide.md', 'w', encoding='utf-8') as f: f.write(troubleshooting); print("Troubleshooting guide saved to troubleshooting_guide.md")

def generate_performance_metrics():
    """Generate performance and specifications table"""
    specs = {'Component': ['Heart Rate Accuracy', 'SpO2 Accuracy', 'Response Time', 'Battery Life', 'Display Resolution', 'Web Interface', 'Data Storage', 'WiFi Range', 'Alert Response', 'Power Consumption', 'Operating Temperature', 'Dimensions'],
             'Specification': ['±2 BPM (60-100 BPM)', '±2% (70-100%)', '<3 seconds', '8-12 hours', '320x240 pixels', 'Responsive HTML5', '4MB SPIFFS', '50-100 meters', '<1 second', '150-300mA', '0-40°C', '10x6x3 cm'],
             'Notes': ['Calibrated for adults', 'Affected by motion', 'Finger detection dependent', 'Varies with usage', '2.8 inch TFT', 'Works on mobile/desktop', 'Circular buffer', 'Depends on environment', 'Audio and visual', 'Display brightness dependent', 'Sensor limitations', 'Enclosure dependent']}
    df = pd.DataFrame(specs); df.to_csv('cardiac_monitor_specifications.csv', index=False)
    print("System specifications saved to cardiac_monitor_specifications.csv"); return df

# New Function: Component Interaction Diagram
def create_component_interaction_diagram():
    """Create component interaction diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(16, 10))
    ax.set_xlim(0, 16); ax.set_ylim(0, 12); ax.set_aspect('equal'); ax.axis('off')
    ax.text(8, 11.5, 'Component Interaction Diagram (Updated: 04:36 PM EAT, Jul 03, 2025)',
            ha='center', va='center', fontsize=16, fontweight='bold')
    components = [
        {'name': 'MAX30102\nSensor', 'pos': (2, 9), 'color': '#FF6B6B'},
        {'name': 'ESP32\nCore', 'pos': (8, 9), 'color': '#4ECDC4'},
        {'name': 'ILI9341\nDisplay', 'pos': (14, 9), 'color': '#45B7D1'},
        {'name': 'Heart Rate\nProcessor', 'pos': (5, 7), 'color': '#96CEB4'},
        {'name': 'SpO2\nProcessor', 'pos': (11, 7), 'color': '#96CEB4'},
        {'name': 'Data\nLogger', 'pos': (8, 5), 'color': '#FFEAA7'},
        {'name': 'Alert\nManager', 'pos': (2, 5), 'color': '#FFB347'},
        {'name': 'Web\nServer', 'pos': (14, 5), 'color': '#DDA0DD'},
        {'name': 'Config\nManager', 'pos': (5, 3), 'color': '#87CEEB'},
        {'name': 'WiFi\nManager', 'pos': (11, 3), 'color': '#F0E68C'},
        {'name': 'Buzzer\nController', 'pos': (2, 1), 'color': '#FFB347'},
        {'name': 'Battery\nMonitor', 'pos': (14, 1), 'color': '#DEB887'}
    ]
    for comp in components:
        circle = plt.Circle(comp['pos'], 0.8, facecolor=comp['color'], edgecolor='black', linewidth=2, alpha=0.8)
        ax.add_patch(circle); ax.text(comp['pos'][0], comp['pos'][1], comp['name'], ha='center', va='center', fontsize=9, fontweight='bold')
    interactions = [((2.8, 9), (7.2, 9), 'I2C\n400kHz'), ((8.8, 9), (13.2, 9), 'SPI\n40MHz'), ((8, 8.2), (5, 7.8), 'Raw PPG\nData'),
                    ((8, 8.2), (11, 7.8), 'Raw PPG\nData'), ((5, 6.2), (8, 5.8), 'HR Data\nJSON'), ((11, 6.2), (8, 5.8), 'SpO2 Data\nJSON'),
                    ((8, 4.2), (14, 5.8), 'HTTP/WS\nData'), ((2, 4.2), (2, 1.8), 'PWM\nSignal'), ((5, 2.2), (8, 4.2), 'Config\nData'),
                    ((11, 3.8), (14, 4.2), 'Network\nStatus'), ((14, 1.8), (8, 4.2), 'Battery\nLevel')]
    for start, end, label in interactions:
        ax.annotate('', xy=end, xytext=start, arrowprops=dict(arrowstyle='->', lw=1.5, color='blue'))
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.3, label, ha='center', va='center', fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='lightblue', alpha=0.7))
    plt.tight_layout(); plt.savefig('component_interactions.png', dpi=300, bbox_inches='tight'); plt.show()
    print("Component interaction diagram saved to component_interactions.png")

# New Function: System Performance Metrics
def generate_system_metrics():
    """Generate system performance metrics chart"""
    fig, ax = plt.subplots(1, 1, figsize=(10, 6))
    ax.set_xlim(0, 12); ax.set_ylim(0, 100); ax.set_aspect('auto'); ax.axis('on')
    ax.set_title('System Performance Metrics (Updated: 04:36 PM EAT, Jul 03, 2025)', fontsize=12, fontweight='bold')
    ax.set_xlabel('Component'); ax.set_ylabel('Performance (%)')
    components = ['Heart Rate', 'SpO2', 'Battery', 'WiFi', 'Display', 'Storage', 'Alerts']
    performance = [95, 92, 88, 90, 93, 87, 91]  # Example values
    bars = ax.bar(components, performance, color=['#FF6B6B', '#4ECDC4', '#45B7D1', '#96CEB4', '#FFEAA7', '#DDA0DD', '#FFB347'])
    for bar in bars: ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1, f'{bar.get_height()}%', ha='center', va='bottom')
    plt.tight_layout(); plt.savefig('system_performance_metrics.png', dpi=300, bbox_inches='tight'); plt.show()
    print("System performance metrics saved to system_performance_metrics.png")

# Main Function
def main():
    print("Generating ESP32 Cardiac Monitor System Architecture...")
    arch = CardiacMonitorArchitecture()
    print("1. Generating main system architecture..."); arch.generate_architecture()
    print("2. Generating data flow diagram..."); create_data_flow_diagram()
    print("3. Generating system states diagram..."); create_system_states_diagram()
    print("4. Generating network topology..."); create_network_topology()
    print("5. Generating security architecture..."); create_security_architecture()
    print("6. Generating component specifications..."); components = generate_component_details()
    print("7. Generating performance specifications..."); specs_df = generate_performance_metrics()
    print("8. Generating deployment guide..."); deployment = generate_deployment_guide()
    print("9. Generating API documentation..."); generate_api_documentation()
    print("10. Generating troubleshooting guide..."); generate_troubleshooting_guide()

# Installation Script
def create_installation_script():
    install_script = """#!/bin/bash
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
"""
    with open('install.sh', 'w', encoding='utf-8') as f: f.write(install_script)
    try: os.chmod('install.sh', stat.S_IRWXU | stat.S_IRGRP | stat.S_IROTH); print("Installation script created: install.sh")
    except: print("Installation script created: install.sh (make executable manually)")

# Project Structure (corrected with UTF-8 encoding)
def create_project_structure():
    structure = """
ESP32 Cardiac Monitor - Project Structure
========================================

esp32-cardiac-monitor/
├── platformio.ini              # PlatformIO configuration
├── README.md                   # Project documentation
├── LICENSE                     # License file
├── .gitignore                  # Git ignore rules
│
├── src/                        # Source code
│   ├── main.cpp               # Main application entry point
│   ├── cardiac_monitor.h      # Main header file
│   ├── sensor_manager.cpp     # Sensor handling
│   ├── display_manager.cpp    # Display control
│   ├── web_server.cpp         # Web server implementation
│   ├── data_logger.cpp        # Data logging functionality
│   ├── alert_manager.cpp      # Alert system
│   ├── config_manager.cpp     # Configuration management
│   └── utils.cpp              # Utility functions
│
├── include/                    # Header files
│   ├── sensor_manager.h
│   ├── display_manager.h
│   ├── web_server.h
│   ├── data_logger.h
│   ├── alert_manager.h
│   ├── config_manager.h
│   └── utils.h
│
├── lib/                        # Custom libraries
│   └── HeartRateLib/          # Custom heart rate library
│       ├── HeartRateLib.h
│       └── HeartRateLib.cpp
│
├── data/                       # SPIFFS filesystem data
│   ├── index.html             # Main web interface
│   ├── style.css              # Stylesheet
│   ├── script.js              # JavaScript application
│   ├── config.json            # Default configuration
│   └── manifest.json          # PWA manifest
│
├── test/                       # Unit tests
│   ├── test_sensor.cpp
│   ├── test_display.cpp
│   └── test_web_server.cpp
│
├── docs/                       # Documentation
│   ├── architecture/          # Architecture diagrams
│   ├── api/                   # API documentation
│   ├── hardware/              # Hardware specifications
│   └── user_guide/            # User documentation
│
├── tools/                      # Development tools
│   ├── generate_architecture.py
│   ├── data_analyzer.py
│   └── test_simulator.py
│
└── examples/                   # Example configurations
    ├── basic_config.json
    ├── hospital_config.json
    └── home_config.json

File Descriptions:
==================

Core Application Files:
- main.cpp: Application entry point, setup and main loop
- cardiac_monitor.h: Main application header with constants
- sensor_manager.cpp: MAX30102 sensor interface and data processing
- display_manager.cpp: ILI9341 display control and UI rendering
- web_server.cpp: HTTP/WebSocket server implementation
- data_logger.cpp: Data storage and retrieval using SPIFFS
- alert_manager.cpp: Alert logic and buzzer control
- config_manager.cpp: Configuration loading and saving

Web Interface Files:
- index.html: Responsive web dashboard
- style.css: Modern CSS styling with animations
- script.js: Real-time data visualization with Chart.js
- manifest.json: Progressive Web App configuration

Configuration Files:
- platformio.ini: Build configuration and library dependencies
- config.json: Runtime configuration (WiFi, thresholds, etc.)

Documentation:
- README.md: Project overview and quick start guide
- API documentation: REST and WebSocket API reference
- User guide: End-user operation manual
- Hardware guide: Wiring and assembly instructions

Development Tools:
- generate_architecture.py: This architecture generator
- data_analyzer.py: Historical data analysis tools
- test_simulator.py: Sensor data simulation for testing
"""
    with open('project_structure.txt', 'w', encoding='utf-8') as f: f.write(structure); print("Project structure documentation created: project_structure.txt")

# Extended Main Function
def extended_main():
    main()
    print("\n9. Generating component interaction diagram..."); create_component_interaction_diagram()
    print("10. Generating system performance metrics..."); generate_system_metrics()
    print("11. Creating installation script..."); create_installation_script()
    print("12. Creating project structure documentation..."); create_project_structure()
    print("\nFinal file list:")
    files_generated = ["cardiac_monitor_architecture.png", "cardiac_monitor_dataflow.png", "cardiac_monitor_states.png",
                      "cardiac_monitor_network.png", "cardiac_monitor_security.png", "component_interactions.png",
                      "system_performance_metrics.png", "cardiac_monitor_specifications.csv", "api_documentation.md",
                      "troubleshooting_guide.md", "deployment_guide.txt", "install.sh", "project_structure.txt"]
    for i, file in enumerate(files_generated, 1): print(f"{i:2d}. {file}")
    print(f"\nTotal files generated: {len(files_generated)}")
    print("\n" + "="*80)
    print("ESP32 CARDIAC MONITOR SYSTEM ARCHITECTURE GENERATION COMPLETE!")
    print("="*80)
    print("\nArchitecture Overview:")
    print("• Hardware Layer: ESP32, MAX30102, ILI9341, Buzzer")
    print("• Firmware Layer: Arduino C++, Real-time processing")
    print("• Data Layer: JSON storage, SPIFFS filesystem")
    print("• Communication Layer: WiFi, HTTP, WebSocket")
    print("• Web Interface: HTML5, CSS3, JavaScript")
    print("• User Interface: Touch display, Web dashboard")
    print("• Security: Multi-layer protection")
    print("• Network: WiFi topology with multiple clients")
    print("\nKey Features:")
    print("• Real-time vital signs monitoring")
    print("• Web-based remote access")
    print("• Configurable alert system")
    print("• Data logging and export")
    print("• Over-the-air updates")
    print("• Responsive web interface")
    print("• Multi-device support")
    print("\nNext Steps:")
    print("1. Review generated architecture diagrams")
    print("2. Follow deployment guide for setup")
    print("3. Refer to API documentation for integration")
    print("4. Use troubleshooting guide for issues")
    print("5. Customize configuration as needed")

# Main Execution
if __name__ == "__main__":
    try:
        import matplotlib.pyplot as plt
        import matplotlib.patches as patches
        from matplotlib.patches import FancyBboxPatch, ConnectionPatch
        import numpy as np
        import pandas as pd
        print("All required libraries imported successfully!")
    except ImportError as e:
        print(f"Error importing libraries: {e}")
        print("Please install required packages:")
        print("pip install matplotlib numpy pandas")
        exit(1)
    extended_main()