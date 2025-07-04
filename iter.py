# Title
ax.text(8, 11.5, 'Component Interaction Diagram', 
        ha='center', va='center', fontsize=16, fontweight='bold')

# Components with their interactions
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

# Draw components
for comp in components:
    circle = plt.Circle(comp['pos'], 0.8, facecolor=comp['color'], 
                       edgecolor='black', linewidth=2, alpha=0.8)
    ax.add_patch(circle)
    ax.text(comp['pos'][0], comp['pos'][1], comp['name'], 
            ha='center', va='center', fontsize=9, fontweight='bold')

# Interactions with protocols
interactions = [
    ((2.8, 9), (7.2, 9), 'I2C\n400kHz'),
    ((8.8, 9), (13.2, 9), 'SPI\n40MHz'),
    ((8, 8.2), (5, 7.8), 'Raw PPG\nData'),
    ((8, 8.2), (11, 7.8), 'Raw PPG\nData'),
    ((5, 6.2), (8, 5.8), 'HR Data\nJSON'),
    ((11, 6.2), (8, 5.8), 'SpO2 Data\nJSON'),
    ((8, 4.2), (14, 5.8), 'HTTP/WS\nData'),
    ((2, 4.2), (2, 1.8), 'PWM\nSignal'),
    ((5, 2.2), (8, 4.2), 'Config\nData'),
    ((11, 3.8), (14, 4.2), 'Network\nStatus'),
    ((14, 1.8), (8, 4.2), 'Battery\nLevel')
]

# Draw interactions
for start, end, label in interactions:
    ax.annotate('', xy=end, xytext=start,
               arrowprops=dict(arrowstyle='->', lw=1.5, color='blue'))
    mid_x, mid_y = (start[0] + end[0]) / 2, (start[1] + end[1]) / 2
    ax.text(mid_x, mid_y + 0.3, label, ha='center', va='center', 
            fontsize=7, bbox=dict(boxstyle="round,pad=0.2", facecolor='lightblue', alpha=0.7))

plt.tight_layout()
plt.savefig('component_interactions.png', dpi=300, bbox_inches='tight')
plt.show()
