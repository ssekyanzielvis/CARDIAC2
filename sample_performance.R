# Sample performance data
time_hours = np.arange(0, 24, 0.5)
cpu_usage = 20 + 10 * np.sin(time_hours * np.pi / 12) + np.random.normal(0, 2, len(time_hours))
memory_usage = 60 + 15 * np.sin(time_hours * np.pi / 8) + np.random.normal(0, 3, len(time_hours))
network_traffic = 50 + 30 * np.sin(time_hours * np.pi / 6) + np.random.normal(0, 5, len(time_hours))
battery_level = 100 - time_hours * 3.5 + np.random.normal(0, 1, len(time_hours))

fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
fig.suptitle('ESP32 Cardiac Monitor - System Performance Metrics', fontsize=16, fontweight='bold')

# CPU Usage
ax1.plot(time_hours, cpu_usage, 'b-', linewidth=2, label='CPU Usage')
ax1.fill_between(time_hours, cpu_usage, alpha=0.3)
ax1.set_title('CPU Usage (%)')
ax1.set_xlabel('Time (hours)')
ax1.set_ylabel('Usage (%)')
ax1.grid(True, alpha=0.3)
ax1.set_ylim(0, 100)

# Memory Usage
ax2.plot(time_hours, memory_usage, 'g-', linewidth=2, label='Memory Usage')
ax2.fill_between(time_hours, memory_usage, alpha=0.3, color='green')
ax2.set_title('Memory Usage (%)')
ax2.set_xlabel('Time (hours)')
ax2.set_ylabel('Usage (%)')
ax2.grid(True, alpha=0.3)
ax2.set_ylim(0, 100)

# Network Traffic
ax3.plot(time_hours, network_traffic, 'r-', linewidth=2, label='Network Traffic')
ax3.fill_between(time_hours, network_traffic, alpha=0.3, color='red')
ax3.set_title('Network Traffic (KB/s)')
ax3.set_xlabel('Time (hours)')
ax3.set_ylabel('Traffic (KB/s)')
ax3.grid(True, alpha=0.3)
ax3.set_ylim(0, 150)

# Battery Level
ax4.plot(time_hours, battery_level, 'orange', linewidth=2, label='Battery Level')
ax4.fill_between(time_hours, battery_level, alpha=0.3, color='orange')
ax4.set_title('Battery Level (%)')
ax4.set_xlabel('Time (hours)')
ax4.set_ylabel('Level (%)')
ax4.grid(True, alpha=0.3)
ax4.set_ylim(0, 100)

plt.tight_layout()
plt.savefig('system_performance_metrics.png', dpi=300, bbox_inches='tight')
plt.show()
