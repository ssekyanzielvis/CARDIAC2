import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.patches import FancyBboxPatch, ConnectionPatch
import numpy as np

class CardiacMonitorArchitecture:
    def __init__(self):
        self.fig, self.ax = plt.subplots(1, 1, figsize=(20, 14))
        self.ax.set_xlim(0, 20)
        self.ax.set_ylim(0, 14)
        self.ax.set_aspect('equal')
        self.ax.axis('off')
        
        # Color scheme
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
        """Create a styled component box"""
        box = FancyBboxPatch(
            (x, y), width, height,
            boxstyle="round,pad=0.1",
            facecolor=color,
            edgecolor='black',
            linewidth=1.5,
            alpha=0.8
        )
        self.ax.add_patch(box)
        
        # Add text
        self.ax.text(x + width/2, y + height/2, text, 
                    ha='center', va='center', fontsize=text_size, 
                    fontweight='bold', wrap=True)
        
        return box
    
    def create_connection(self, start_pos, end_pos, style='solid', color='black', width=2):
        """Create connection lines between components"""
        line = ConnectionPatch(start_pos, end_pos, "data", "data",
                             arrowstyle="->", shrinkA=5, shrinkB=5,
                             mutation_scale=20, fc=color, ec=color,
                             linewidth=width, linestyle=style)
        self.ax.add_patch(line)
        
    def draw_hardware_layer(self):
        """Draw the hardware components layer"""
        # Title
        self.ax.text(10, 13.5, 'ESP32 Cardiac Monitor - System Architecture', 
                    ha='center', va='center', fontsize=18, fontweight='bold')
        
        # Hardware Layer
        self.ax.text(1, 12.5, 'HARDWARE LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['hardware'])
        
        # ESP32 Core
        self.create_component_box(1, 11, 3, 1, 'ESP32\nMicrocontroller\n(Dual Core)', 
                                self.colors['hardware'], 9)
        
        # Sensors
        self.create_component_box(5, 11.5, 2.5, 0.8, 'MAX30102\nHeart Rate &\nSpO2 Sensor', 
                                self.colors['hardware'], 8)
        
        self.create_component_box(8, 11.5, 2, 0.8, 'Battery\nMonitor\n(ADC)', 
                                self.colors['hardware'], 8)
        
        # Display and Input
        self.create_component_box(11, 11.5, 2.5, 0.8, 'ILI9341\n2.8" TFT\nTouch Display', 
                                self.colors['hardware'], 8)
        
        # Audio Output
        self.create_component_box(14.5, 11.5, 1.5, 0.8, 'Buzzer\nAlerts', 
                                self.colors['hardware'], 8)
        
        # Communication
        self.create_component_box(16.5, 11.5, 2.5, 0.8, 'WiFi\nModule\n(Built-in)', 
                                self.colors['hardware'], 8)
        
    def draw_firmware_layer(self):
        """Draw the firmware/software layer"""
        # Firmware Layer
        self.ax.text(1, 10.2, 'FIRMWARE LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['firmware'])
        
        # Core System
        self.create_component_box(1, 9, 3, 1, 'Main Application\n(cardiac_monitor.ino)\nTask Scheduler', 
                                self.colors['firmware'], 9)
        
        # Sensor Libraries
        self.create_component_box(5, 9.5, 2, 0.8, 'Heart Rate\nCalculation\nLibrary', 
                                self.colors['firmware'], 8)
        
        self.create_component_box(7.5, 9.5, 2, 0.8, 'SpO2\nCalculation\nLibrary', 
                                self.colors['firmware'], 8)
        
        # UI System
        self.create_component_box(10, 9.5, 2.5, 0.8, 'UI Elements\nTouch Handler\nDisplay Manager', 
                                self.colors['firmware'], 8)
        
        # Configuration
        self.create_component_box(13, 9.5, 2, 0.8, 'Config\nManager\nSPIFFS', 
                                self.colors['firmware'], 8)
        
        # Alert System
        self.create_component_box(15.5, 9.5, 2, 0.8, 'Alert\nManager\nBuzzer Control', 
                                self.colors['alerts'], 8)
        
        # Web Server
        self.create_component_box(18, 9.5, 1.5, 0.8, 'Web Server\nWebSocket\nAPI', 
                                self.colors['firmware'], 8)
        
    def draw_data_layer(self):
        """Draw the data processing and storage layer"""
        # Data Layer
        self.ax.text(1, 8.2, 'DATA LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['data'])
        
        # Data Structures
        self.create_component_box(1, 7, 2.5, 1, 'Vital Signs\nData Structure\nReal-time Buffer', 
                                self.colors['data'], 9)
        
        # Data Processing
        self.create_component_box(4, 7.5, 2.5, 0.8, 'Signal Processing\nFiltering\nValidation', 
                                self.colors['data'], 8)
        
        # Storage
        self.create_component_box(7, 7.5, 2.5, 0.8, 'Data Logger\nSPIFFS Storage\nJSON Format', 
                                self.colors['data'], 8)
        
        # Analytics
        self.create_component_box(10, 7.5, 2.5, 0.8, 'Trend Analysis\nThreshold Check\nAlert Logic', 
                                self.colors['data'], 8)
        
        # Export
        self.create_component_box(13, 7.5, 2, 0.8, 'Data Export\nCSV Generator\nAPI Endpoints', 
                                self.colors['data'], 8)
        
    def draw_communication_layer(self):
        """Draw the communication layer"""
        # Communication Layer
        self.ax.text(1, 6.2, 'COMMUNICATION LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['communication'])
        
        # WiFi Management
        self.create_component_box(1, 5, 2.5, 1, 'WiFi Manager\nConnection\nReconnection', 
                                self.colors['communication'], 9)
        
        # HTTP Server
        self.create_component_box(4, 5.5, 2.5, 0.8, 'HTTP Server\nREST API\nFile Server', 
                                self.colors['communication'], 8)
        
        # WebSocket
        self.create_component_box(7, 5.5, 2.5, 0.8, 'WebSocket Server\nReal-time Data\nBidirectional', 
                                self.colors['communication'], 8)
        
        # mDNS
        self.create_component_box(10, 5.5, 2, 0.8, 'mDNS\nService\nDiscovery', 
                                self.colors['communication'], 8)
        
        # OTA Updates
        self.create_component_box(12.5, 5.5, 2, 0.8, 'OTA Updates\nFirmware\nSPIFFS', 
                                self.colors['communication'], 8)
        
    def draw_web_interface_layer(self):
        """Draw the web interface layer"""
        # Web Interface Layer
        self.ax.text(1, 4.2, 'WEB INTERFACE LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['web'])
        
        # Frontend
        self.create_component_box(1, 3, 3, 1, 'HTML5 Dashboard\nResponsive Design\nPWA Support', 
                                self.colors['web'], 9)
        
        # Styling
        self.create_component_box(4.5, 3.5, 2, 0.8, 'CSS3 Styling\nAnimations\nResponsive', 
                                self.colors['web'], 8)
        
        # JavaScript
        self.create_component_box(7, 3.5, 2.5, 0.8, 'JavaScript App\nWebSocket Client\nChart.js', 
                                self.colors['web'], 8)
        
        # Charts
        self.create_component_box(10, 3.5, 2, 0.8, 'Real-time\nCharts\nTrend Analysis', 
                                self.colors['web'], 8)
        
        # Service Worker
        self.create_component_box(12.5, 3.5, 2, 0.8, 'Service Worker\nOffline Support\nCaching', 
                                self.colors['web'], 8)
        
        # Settings
        self.create_component_box(15, 3.5, 2, 0.8, 'Settings Panel\nConfiguration\nThresholds', 
                                self.colors['web'], 8)
        
    def draw_user_interface_layer(self):
        """Draw the user interface layer"""
        # User Interface Layer
        self.ax.text(1, 2.2, 'USER INTERFACE LAYER', fontsize=14, fontweight='bold', 
                    color=self.colors['user'])
        
        # Local Display
        self.create_component_box(1, 1, 3, 1, 'Local TFT Display\nTouch Interface\nReal-time Vitals', 
                                self.colors['user'], 9)
        
        # Web Browser
        self.create_component_box(5, 1.5, 2.5, 0.8, 'Web Browser\nDesktop/Mobile\nRemote Access', 
                                self.colors['user'], 8)
        
        # Mobile App
        self.create_component_box(8, 1.5, 2, 0.8, 'Mobile PWA\nOffline Capable\nNotifications', 
                                self.colors['user'], 8)
        
        # Alerts
        self.create_component_box(10.5, 1.5, 2, 0.8, 'Visual Alerts\nAudio Alerts\nNotifications', 
                                self.colors['alerts'], 8)
        
        # Data Export
        self.create_component_box(13, 1.5, 2, 0.8, 'Data Download\nCSV Export\nReports', 
                                self.colors['user'], 8)
        
        # Settings UI
        self.create_component_box(15.5, 1.5, 2, 0.8, 'Settings UI\nConfiguration\nCalibration', 
                                self.colors['user'], 8)
        
    def draw_connections(self):
        """Draw connections between components"""
        # Hardware to Firmware connections
        self.create_connection((2.5, 11), (2.5, 10), color='red', width=2)
        self.create_connection((6.25, 11.5), (6, 10.3), color='red')
        self.create_connection((9, 11.5), (8.5, 10.3), color='red')
        self.create_connection((12.25, 11.5), (11.25, 10.3), color='red')
        self.create_connection((15.25, 11.5), (16.5, 10.3), color='red')
        self.create_connection((17.75, 11.5), (18.75, 10.3), color='red')
        
        # Firmware to Data connections
        self.create_connection((2.5, 9), (2.25, 8), color='blue', width=2)
        self.create_connection((6, 9.5), (5.25, 8.3), color='blue')
        self.create_connection((8.5, 9.5), (8.25, 8.3), color='blue')
        
        # Data to Communication connections
        self.create_connection((2.25, 7), (2.25, 6), color='green', width=2)
        self.create_connection((8.25, 7.5), (8.25, 6.3), color='green')
        self.create_connection((14, 7.5), (5.25, 6.3), color='green')
        
        # Communication to Web connections
        self.create_connection((5.25, 5.5), (2.5, 4), color='purple', width=2)
        self.create_connection((8.25, 5.5), (8.25, 4.3), color='purple')
        
        # Web to User connections
        self.create_connection((2.5, 3), (2.5, 2), color='orange', width=2)
        self.create_connection((8.25, 3.5), (6.25, 2.3), color='orange')
        
        # Local display connection
        self.create_connection((11.25, 9.5), (2.5, 2), color='brown', style='dashed')
        
    def add_data_flow_arrows(self):
        """Add data flow indicators"""
        # Sensor data flow
        self.ax.annotate('Sensor Data', xy=(6.25, 10.8), xytext=(6.25, 10.5),
                        arrowprops=dict(arrowstyle='->', color='red', lw=2),
                        fontsize=8, ha='center', color='red')
        
        # Real-time data flow
        self.ax.annotate('Real-time\nData Stream', xy=(8.25, 6.8), xytext=(8.25, 6.5),
                        arrowprops=dict(arrowstyle='->', color='green', lw=2),
                        fontsize=8, ha='center
    def add_data_flow_arrows(self):
        """Add data flow indicators"""
        # Sensor data flow
        self.ax.annotate('Sensor Data', xy=(6.25, 10.8), xytext=(6.25, 10.5),
                        arrowprops=dict(arrowstyle='->', color='red', lw=2),
                        fontsize=8, ha='center', color='red')
        
        # Real-time data flow
        self.ax.annotate('Real-time\nData Stream', xy=(8.25, 6.8), xytext=(8.25, 6.5),
                        arrowprops=dict(arrowstyle='->', color='green', lw=2),
                        fontsize=8, ha='center', color='green')
        
        # Web data flow
        self.ax.annotate('HTTP/WebSocket', xy=(5.25, 4.8), xytext=(5.25, 4.5),
                        arrowprops=dict(arrowstyle='->', color='purple', lw=2),
                        fontsize=8, ha='center', color='purple')
        
        # User interaction
        self.ax.annotate('User\nInteraction', xy=(2.5, 2.8), xytext=(2.5, 2.5),
                        arrowprops=dict(arrowstyle='<->', color='orange', lw=2),
                        fontsize=8, ha='center', color='orange')
        
    def add_legend(self):
        """Add color legend"""
        legend_x = 0.5
        legend_y = 0.5
        legend_items = [
            ('Hardware', self.colors['hardware']),
            ('Firmware', self.colors['firmware']),
            ('Data Processing', self.colors['data']),
            ('Communication', self.colors['communication']),
            ('Web Interface', self.colors['web']),
            ('User Interface', self.colors['user']),
            ('Alerts', self.colors['alerts'])
        ]
        
        for i, (label, color) in enumerate(legend_items):
            y_pos = legend_y - i * 0.15
            self.create_component_box(legend_x, y_pos, 1.5, 0.1, '', color, 8)
            self.ax.text(legend_x + 1.6, y_pos + 0.05, label, 
                        fontsize=8, va='center', fontweight='bold')
    
    def generate_architecture(self):
        """Generate the complete system architecture diagram"""
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

# Additional utility functions for detailed component analysis
def generate_component_details():
    """Generate detailed component specifications"""
    components = {
        'Hardware Components': {
            'ESP32': {
                'Type': 'Microcontroller',
                'Features': ['Dual-core Xtensa LX6', 'WiFi & Bluetooth', '520KB SRAM', 'GPIO pins'],
                'Purpose': 'Main processing unit and connectivity'
            },
            'MAX30102': {
                'Type': 'Biometric Sensor',
                'Features': ['Heart rate detection', 'SpO2 measurement', 'I2C interface', 'Low power'],
                'Purpose': 'Vital signs monitoring'
            },
            'ILI9341': {
                'Type': 'TFT Display',
                'Features': ['320x240 resolution', 'Touch interface', 'SPI communication', '2.8 inch'],
                'Purpose': 'Local user interface'
            },
            'Buzzer': {
                'Type': 'Audio Output',
                'Features': ['Piezo buzzer', 'Alert tones', 'Low power consumption'],
                'Purpose': 'Audio alerts and notifications'
            }
        },
        'Software Components': {
            'Main Application': {
                'Language': 'C++/Arduino',
                'Features': ['Task scheduling', 'Sensor management', 'UI control', 'Data processing'],
                'Purpose': 'Core system orchestration'
            },
            'Web Server': {
                'Type': 'HTTP/WebSocket Server',
                'Features': ['REST API', 'Real-time communication', 'File serving', 'CORS support'],
                'Purpose': 'Remote access and control'
            },
            'Data Logger': {
                'Type': 'Storage System',
                'Features': ['SPIFFS filesystem', 'JSON format', 'Circular buffer', 'Export capability'],
                'Purpose': 'Historical data management'
            },
            'Alert System': {
                'Type': 'Monitoring System',
                'Features': ['Threshold checking', 'Multi-level alerts', 'Audio/visual output', 'Configurable'],
                'Purpose': 'Health monitoring and warnings'
            }
        }
    }
    return components

def create_data_flow_diagram():
    """Create a detailed data flow diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(16, 10))
    ax.set_xlim(0, 16)
    ax.set_ylim(0, 10)
    ax.set_aspect('equal')
    ax.axis('off')
    
    # Title
    ax.text(8, 9.5, 'Cardiac Monitor - Data Flow Diagram', 
            ha='center', va='center', fontsize=16, fontweight='bold')
    
    # Data flow stages
    stages = [
        {'name': 'Sensor\nAcquisition', 'pos': (1, 7), 'color': '#FF6B6B'},
        {'name': 'Signal\nProcessing', 'pos': (4, 7), 'color': '#4ECDC4'},
        {'name': 'Data\nValidation', 'pos': (7, 7), 'color': '#45B7D1'},
        {'name': 'Storage &\nLogging', 'pos': (10, 7), 'color': '#96CEB4'},
        {'name': 'Display &\nTransmission', 'pos': (13, 7), 'color': '#FFEAA7'},
        {'name': 'Alert\nProcessing', 'pos': (7, 4), 'color': '#FFB347'},
        {'name': 'User\nInterface', 'pos': (4, 1), 'color': '#DDA0DD'},
        {'name': 'Web\nInterface', 'pos': (10, 1), 'color': '#87CEEB'}
    ]
    
    # Draw stages
    for stage in stages:
        box = FancyBboxPatch(
            (stage['pos'][0]-0.8, stage['pos'][1]-0.5), 1.6, 1,
            boxstyle="round,pad=0.1",
            facecolor=stage['color'],
            edgecolor='black',
            linewidth=1.5,
            alpha=0.8
        )
        ax.add_patch(box)
        ax.text(stage['pos'][0], stage['pos'][1], stage['name'], 
                ha='center', va='center', fontsize=10, fontweight='bold')
    
    # Draw connections with labels
    connections = [
        ((1.8, 7), (3.2, 7), 'Raw PPG\nSignal'),
        ((4.8, 7), (6.2, 7), 'Filtered\nData'),
        ((7.8, 7), (9.2, 7), 'Valid\nVitals'),
        ((10.8, 7), (12.2, 7), 'Stored\nData'),
        ((7, 6.5), (7, 4.5), 'Threshold\nCheck'),
        ((6.2, 4), (4.8, 1.5), 'Local\nAlerts'),
        ((7.8, 4), (9.2, 1.5), 'Remote\nAlerts'),
        ((13, 6.5), (10.8, 1.5), 'Web\nData'),
        ((13, 6.5), (4.8, 1.5), 'Display\nData')
    ]
    
    for start, end, label in connections:
        # Draw arrow
        ax.annotate('', xy=end, xytext=start,
                   arrowprops=dict(arrowstyle='->', lw=2, color='black'))
        # Add label
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.2, label, ha='center', va='center', 
                fontsize=8, bbox=dict(boxstyle="round,pad=0.2", facecolor='white', alpha=0.8))
    
    plt.tight_layout()
    plt.savefig('cardiac_monitor_dataflow.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_system_states_diagram():
    """Create system states and transitions diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(14, 10))
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 10)
    ax.set_aspect('equal')
    ax.axis('off')
    
    # Title
    ax.text(7, 9.5, 'Cardiac Monitor - System States Diagram', 
            ha='center', va='center', fontsize=16, fontweight='bold')
    
    # System states
    states = [
        {'name': 'BOOT', 'pos': (2, 8), 'color': '#FF6B6B'},
        {'name': 'INIT', 'pos': (5, 8), 'color': '#4ECDC4'},
        {'name': 'IDLE', 'pos': (8, 8), 'color': '#45B7D1'},
        {'name': 'MONITORING', 'pos': (11, 8), 'color': '#96CEB4'},
        {'name': 'ALERT', 'pos': (11, 5), 'color': '#FFB347'},
        {'name': 'CONFIG', 'pos': (8, 5), 'color': '#FFEAA7'},
        {'name': 'DATA_EXPORT', 'pos': (5, 5), 'color': '#DDA0DD'},
        {'name': 'ERROR', 'pos': (2, 5), 'color': '#FF4757'},
        {'name': 'SLEEP', 'pos': (8, 2), 'color': '#A4B0BE'}
    ]
    
    # Draw states
    for state in states:
        circle = plt.Circle(state['pos'], 0.8, facecolor=state['color'], 
                           edgecolor='black', linewidth=2, alpha=0.8)
        ax.add_patch(circle)
        ax.text(state['pos'][0], state['pos'][1], state['name'], 
                ha='center', va='center', fontsize=9, fontweight='bold')
    
    # State transitions
    transitions = [
        ((2.8, 8), (4.2, 8), 'Hardware\nInit'),
        ((5.8, 8), (7.2, 8), 'System\nReady'),
        ((8.8, 8), (10.2, 8), 'Finger\nDetected'),
        ((11, 7.2), (11, 5.8), 'Threshold\nExceeded'),
        ((10.2, 5), (8.8, 5), 'Settings\nRequest'),
        ((7.2, 5), (5.8, 5), 'Export\nRequest'),
        ((4.2, 5), (2.8, 5), 'System\nError'),
        ((8, 4.2), (8, 2.8), 'Inactivity\nTimeout'),
        ((8, 2.8), (8, 4.2), 'Wake Up\nEvent'),
        ((10.2, 8), (8.8, 8), 'Finger\nRemoved'),
        ((11, 5.8), (11, 7.2), 'Alert\nAcknowledged')
    ]
    
    for start, end, label in transitions:
        ax.annotate('', xy=end, xytext=start,
                   arrowprops=dict(arrowstyle='->', lw=1.5, color='blue'))
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.3, label, ha='center', va='center', 
                fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='lightblue', alpha=0.7))
    
    plt.tight_layout()
    plt.savefig('cardiac_monitor_states.png', dpi=300, bbox_inches='tight')
    plt.show()

def generate_performance_metrics():
    """Generate performance and specifications table"""
    import pandas as pd
    
    # System specifications
    specs = {
        'Component': [
            'Heart Rate Accuracy', 'SpO2 Accuracy', 'Response Time', 'Battery Life',
            'Display Resolution', 'Web Interface', 'Data Storage', 'WiFi Range',
            'Alert Response', 'Power Consumption', 'Operating Temperature', 'Dimensions'
        ],
        'Specification': [
            '±2 BPM (60-100 BPM)', '±2% (70-100%)', '<3 seconds', '8-12 hours',
            '320x240 pixels', 'Responsive HTML5', '4MB SPIFFS', '50-100 meters',
            '<1 second', '150-300mA', '0-40°C', '10x6x3 cm'
        ],
        'Notes': [
            'Calibrated for adults', 'Affected by motion', 'Finger detection dependent', 'Varies with usage',
            '2.8 inch TFT', 'Works on mobile/desktop', 'Circular buffer', 'Depends on environment',
            'Audio and visual', 'Display brightness dependent', 'Sensor limitations', 'Enclosure dependent'
        ]
    }
    
    df = pd.DataFrame(specs)
    
    # Save to CSV
    df.to_csv('cardiac_monitor_specifications.csv', index=False)
    print("System specifications saved to cardiac_monitor_specifications.csv")
    
    return df

def create_network_topology():
    """Create network topology diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8)
    ax.set_aspect('equal')
    ax.axis('off')
    
    # Title
    ax.text(6, 7.5, 'Cardiac Monitor - Network Topology', 
            ha='center', va
def create_network_topology():
    """Create network topology diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 8)
    ax.set_aspect('equal')
    ax.axis('off')
    
    # Title
    ax.text(6, 7.5, 'Cardiac Monitor - Network Topology', 
            ha='center', va='center', fontsize=16, fontweight='bold')
    
    # Network components
    components = [
        {'name': 'WiFi Router\n192.168.1.1', 'pos': (6, 6), 'color': '#4ECDC4', 'shape': 'rect'},
        {'name': 'ESP32 Monitor\n192.168.1.100', 'pos': (3, 4), 'color': '#FF6B6B', 'shape': 'circle'},
        {'name': 'Desktop PC\n192.168.1.50', 'pos': (9, 4), 'color': '#45B7D1', 'shape': 'rect'},
        {'name': 'Mobile Device\n192.168.1.75', 'pos': (6, 2), 'color': '#96CEB4', 'shape': 'rect'},
        {'name': 'Tablet\n192.168.1.80', 'pos': (2, 2), 'color': '#FFEAA7', 'shape': 'rect'},
        {'name': 'Cloud Storage\n(Optional)', 'pos': (10, 6), 'color': '#DDA0DD', 'shape': 'cloud'}
    ]
    
    # Draw components
    for comp in components:
        if comp['shape'] == 'circle':
            circle = plt.Circle(comp['pos'], 0.6, facecolor=comp['color'], 
                               edgecolor='black', linewidth=2, alpha=0.8)
            ax.add_patch(circle)
        elif comp['shape'] == 'rect':
            rect = FancyBboxPatch(
                (comp['pos'][0]-0.8, comp['pos'][1]-0.4), 1.6, 0.8,
                boxstyle="round,pad=0.1",
                facecolor=comp['color'],
                edgecolor='black',
                linewidth=1.5,
                alpha=0.8
            )
            ax.add_patch(rect)
        elif comp['shape'] == 'cloud':
            # Simple cloud shape using ellipse
            ellipse = patches.Ellipse(comp['pos'], 1.5, 0.8, facecolor=comp['color'], 
                                    edgecolor='black', linewidth=1.5, alpha=0.8)
            ax.add_patch(ellipse)
        
        ax.text(comp['pos'][0], comp['pos'][1], comp['name'], 
                ha='center', va='center', fontsize=8, fontweight='bold')
    
    # Network connections
    connections = [
        ((6, 5.4), (3.6, 4.6), 'WiFi\n2.4GHz'),
        ((6, 5.4), (8.4, 4.6), 'Ethernet/WiFi'),
        ((6, 5.4), (6, 2.8), 'WiFi\n5GHz'),
        ((6, 5.4), (2.8, 2.6), 'WiFi\n2.4GHz'),
        ((6.8, 6), (9.2, 6), 'Internet\n(Optional)')
    ]
    
    for start, end, label in connections:
        ax.plot([start[0], end[0]], [start[1], end[1]], 'k-', linewidth=2)
        mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
        ax.text(mid_x, mid_y + 0.2, label, ha='center', va='center', 
                fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='white', alpha=0.8))
    
    # Add protocol information
    protocols = [
        'HTTP: Port 80 (Web Interface)',
        'WebSocket: Port 81 (Real-time Data)',
        'mDNS: cardiac-monitor.local',
        'OTA: Port 3232 (Updates)'
    ]
    
    for i, protocol in enumerate(protocols):
        ax.text(0.5, 1 - i*0.2, protocol, fontsize=9, 
                bbox=dict(boxstyle="round,pad=0.3", facecolor='lightgray', alpha=0.7))
    
    plt.tight_layout()
    plt.savefig('cardiac_monitor_network.png', dpi=300, bbox_inches='tight')
    plt.show()

def create_security_architecture():
    """Create security architecture diagram"""
    fig, ax = plt.subplots(1, 1, figsize=(14, 10))
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 10)
    ax.set_aspect('equal')
    ax.axis('off')
    
    # Title
    ax.text(7, 9.5, 'Cardiac Monitor - Security Architecture', 
            ha='center', va='center', fontsize=16, fontweight='bold')
    
    # Security layers
    layers = [
        {'name': 'Physical Security', 'pos': (7, 8.5), 'width': 12, 'height': 0.8, 'color': '#FF6B6B'},
        {'name': 'Network Security', 'pos': (7, 7.5), 'width': 10, 'height': 0.8, 'color': '#4ECDC4'},
        {'name': 'Application Security', 'pos': (7, 6.5), 'width': 8, 'height': 0.8, 'color': '#45B7D1'},
        {'name': 'Data Security', 'pos': (7, 5.5), 'width': 6, 'height': 0.8, 'color': '#96CEB4'}
    ]
    
    # Draw security layers
    for layer in layers:
        rect = FancyBboxPatch(
            (layer['pos'][0] - layer['width']/2, layer['pos'][1] - layer['height']/2), 
            layer['width'], layer['height'],
            boxstyle="round,pad=0.1",
            facecolor=layer['color'],
            edgecolor='black',
            linewidth=1.5,
            alpha=0.3
        )
        ax.add_patch(rect)
        ax.text(layer['pos'][0], layer['pos'][1], layer['name'], 
                ha='center', va='center', fontsize=12, fontweight='bold')
    
    # Security components
    security_components = [
        {'name': 'Device\nEnclosure', 'pos': (2, 8.5), 'color': '#FF6B6B'},
        {'name': 'Tamper\nDetection', 'pos': (5, 8.5), 'color': '#FF6B6B'},
        {'name': 'Secure Boot', 'pos': (9, 8.5), 'color': '#FF6B6B'},
        {'name': 'Hardware\nRNG', 'pos': (12, 8.5), 'color': '#FF6B6B'},
        
        {'name': 'WPA2/WPA3\nEncryption', 'pos': (3, 7.5), 'color': '#4ECDC4'},
        {'name': 'MAC Address\nFiltering', 'pos': (7, 7.5), 'color': '#4ECDC4'},
        {'name': 'Firewall\nRules', 'pos': (11, 7.5), 'color': '#4ECDC4'},
        
        {'name': 'HTTPS/WSS\nEncryption', 'pos': (4, 6.5), 'color': '#45B7D1'},
        {'name': 'Input\nValidation', 'pos': (7, 6.5), 'color': '#45B7D1'},
        {'name': 'Session\nManagement', 'pos': (10, 6.5), 'color': '#45B7D1'},
        
        {'name': 'Data\nEncryption', 'pos': (5, 5.5), 'color': '#96CEB4'},
        {'name': 'Access\nControl', 'pos': (7, 5.5), 'color': '#96CEB4'},
        {'name': 'Audit\nLogging', 'pos': (9, 5.5), 'color': '#96CEB4'}
    ]
    
    # Draw security components
    for comp in security_components:
        box = FancyBboxPatch(
            (comp['pos'][0]-0.6, comp['pos'][1]-0.3), 1.2, 0.6,
            boxstyle="round,pad=0.05",
            facecolor=comp['color'],
            edgecolor='black',
            linewidth=1,
            alpha=0.8
        )
        ax.add_patch(box)
        ax.text(comp['pos'][0], comp['pos'][1], comp['name'], 
                ha='center', va='center', fontsize=8, fontweight='bold')
    
    # Threat vectors
    threats = [
        {'name': 'Physical\nTampering', 'pos': (1, 4), 'color': '#FF4757'},
        {'name': 'Network\nEavesdropping', 'pos': (4, 4), 'color': '#FF4757'},
        {'name': 'Web\nAttacks', 'pos': (7, 4), 'color': '#FF4757'},
        {'name': 'Data\nBreach', 'pos': (10, 4), 'color': '#FF4757'},
        {'name': 'Firmware\nModification', 'pos': (13, 4), 'color': '#FF4757'}
    ]
    
    ax.text(7, 3.5, 'Potential Threats', ha='center', va='center', 
            fontsize=12, fontweight='bold', color='red')
    
    for threat in threats:
        triangle = patches.RegularPolygon(threat['pos'], 3, radius=0.4, 
                                        facecolor=threat['color'], 
                                        edgecolor='darkred', linewidth=2, alpha=0.8)
        ax.add_patch(triangle)
        ax.text(threat['pos'][0], threat['pos'][1]-0.8, threat['name'], 
                ha='center', va='center', fontsize=8, fontweight='bold', color='red')
    
    # Mitigation arrows
    mitigations = [
        ((2, 8.2), (1, 4.4)),
        ((5, 7.2), (4, 4.4)),
        ((7, 6.2), (7, 4.4)),
        ((7, 5.2), (10, 4.4)),
        ((9, 8.2), (13, 4.4))
    ]
    
    for start, end in mitigations:
        ax.annotate('', xy=end, xytext=start,
                   arrowprops=dict(arrowstyle='->', lw=2, color='green', alpha=0.7))
    
    plt.tight_layout()
    plt.savefig('cardiac_monitor_security.png', dpi=300, bbox_inches='tight')
    plt.show()

def generate_deployment_guide():
    """Generate deployment architecture and guide"""
    deployment_steps = {
        'Hardware Setup': [
            'Connect MAX30102 sensor to ESP32 I2C pins (SDA=21, SCL=22)',
            'Connect ILI9341 display to ESP32 SPI pins',
            'Connect buzzer to GPIO pin 27',
            'Connect battery monitor to ADC pin 35',
            'Assemble components in enclosure'
        ],
        'Software Installation': [
            'Install PlatformIO IDE',
            'Clone project repository',
            'Configure WiFi credentials in data/config.json',
            'Build and upload firmware to ESP32',
            'Upload SPIFFS filesystem with web files',
            'Verify serial output for successful boot'
        ],
        'Network Configuration': [
            'Connect ESP32 to WiFi network',
            'Note assigned IP address from serial monitor',
            'Configure router for static IP (optional)',
            'Enable mDNS for cardiac-monitor.local access',
            'Test web interface connectivity'
        ],
        'Calibration & Testing': [
            'Place finger on sensor for initial readings',
            'Verify heart rate and SpO2 accuracy',
            'Test alert thresholds and buzzer',
            'Calibrate display touch sensitivity',
            'Perform data logging and export tests'
        ],
        'Production Deployment': [
            'Set secure WiFi credentials',
            'Configure appropriate alert thresholds',
            'Enable data logging and retention policies',
            'Set up backup and recovery procedures',
            'Document system configuration and contacts'
        ]
    }
    
    # Save deployment guide
    with open('deployment_guide.txt', 'w') as f:
        f.write("ESP32 Cardiac Monitor - Deployment Guide\n")
        f.write("=" * 50 + "\n\n")
        
        for section, steps in deployment_steps.items():
            f.write(f"{section}:\n")
            f.write("-" * len(section) + "\n")
            for i, step in enumerate(steps, 1):
                f.write(f"{i}. {step}\n")
            f.write("\n")
    
    print("Deployment guide saved to deployment_guide.txt")
    return deployment_steps

def main():
    """Main function to generate all architecture diagrams"""
    print("Generating ESP32 Cardiac Monitor System Architecture...")
    
    # Create main system architecture
    arch = CardiacMonitorArchitecture()
    print("1. Generating main system architecture...")
    arch.generate_architecture()
    
    # Create data flow diagram
    print("2. Generating data flow diagram...")
    create_data_flow_diagram()
    
    # Create system states diagram
    print("3. Generating system states diagram...")
    create_system_states_diagram()
    
    # Create network topology
    print("4. Generating network topology...")
    create_network_topology()
    
    # Create security architecture
    print("5. Generating security architecture...")
    create_security_architecture()
    
    # Generate component details
    print("6. Generating component specifications...")
    components = generate_component_details()
    
    # Generate performance metrics
    print("7. Generating performance specifications...")
    specs_df = generate_performance_metrics()
    print(specs_df)
    
    # Generate deployment guide
    print("8. Generating deployment guide...")
    deployment = generate_deployment_guide()
    
    print("\nAll architecture diagrams and documentation generated successfully!")
    print("\nGenerated files:")
    print("- cardiac_monitor_architecture.png")
    print("- cardiac_monitor_dataflow.png") 
    print("- cardiac_monitor_states.png")
    print("- cardiac_monitor_network.png")
    print("- cardiac_monitor_security.png")
    print("- cardiac_monitor_specifications.csv")
    print("- deployment_guide.txt")
    
    # Generate additional documentation
    generate_api_documentation()
    generate_troubleshooting_guide()
    
    print("- api_documentation.md")
    print("- troubleshooting_guide.md")
    
    print("\n" + "="*60)
    print("ESP32 Cardiac Monitor System Architecture Complete!")
    print("="*60)

def generate_api_documentation():
    """Generate API documentation"""
    api_docs = """# ESP32 Cardiac Monitor - API Documentation

## REST API Endpoints

### Device Information
- **GET** `/api/info` - Get device information and status
- **GET** `/api/status` - Get current system status

### Vital Signs Data
- **GET** `/api/vitals` - Get current vital signs
- **GET** `/api/vitals/history` - Get historical data
- **GET** `/api/vitals/export` - Export data as CSV

### Configuration
- **GET** `/api/config` - Get current configuration
- **POST** `/api/config` - Update configuration
- **POST** `/api/config/reset` - Reset to defaults

### Alerts
- **GET** `/api/alerts` - Get alert configuration
- **POST** `/api/alerts` - Update alert settings
- **GET** `/api/alerts/history` - Get alert history

### System Control
- **POST** `/api/system/restart` - Restart device
- **POST** `/api/system/calibrate` - Calibrate sensors
- **GET** `/api/system/logs` - Get system logs

## WebSocket Events

### Client to Server
```json
{
  "type": "subscribe",
  "data": "vitals"
}
