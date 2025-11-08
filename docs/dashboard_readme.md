# Environmental Monitoring Dashboard

A Python-based GUI application for Raspberry Pi that collects and visualizes data from multiple BLE sensor nodes in real-time.

![Dashboard Status](https://img.shields.io/badge/version-3.0-blue)
![Python](https://img.shields.io/badge/python-3.7+-green)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red)

## 📋 Overview

This dashboard connects to multiple environmental sensors via Bluetooth Low Energy (BLE) and provides:
- Real-time data visualization with live graphs
- Connection health monitoring and auto-reconnect
- Automatic data logging to CSV
- Statistical analysis (min/max/average)
- Detailed connection logging for troubleshooting

## 🎯 Supported Sensors

### 1. **SPL Meter** (XIAO MG24)
- **Function**: A-weighted sound pressure level monitoring
- **Device Name**: `SPL_Meter`
- **Measurement**: dBA (decibels A-weighted)
- **Update Rate**: 2 Hz (500ms intervals)

### 2. **AI Vision Node** (ESP32-C3)
- **Function**: Person detection and occupancy tracking
- **Device Name**: `AIVisionNode`
- **Measurement**: People count (integer)
- **Update Rate**: 1 Hz (1 second intervals)

## 🔧 Hardware Requirements

- **Raspberry Pi** (3B+, 4, or 5 recommended)
  - Built-in Bluetooth or USB Bluetooth adapter
  - Running Raspberry Pi OS (Bullseye or newer)
- **Seeed Studio XIAO MG24** with SPL meter firmware
- **Seeed Studio XIAO ESP32-C3** with AI vision firmware
- All devices within 10 meters range

## 📦 Software Requirements

### System Dependencies

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install python3-pip python3-tk bluetooth bluez

# Ensure Bluetooth is enabled
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

### Python Dependencies

```bash
# Install Python packages
pip3 install bleak matplotlib pandas

# Or use requirements.txt
pip3 install -r requirements.txt
```

#### `requirements.txt`
```
bleak>=0.21.0
matplotlib>=3.5.0
pandas>=1.3.0
```

## 🚀 Installation

### 1. Clone or Download

```bash
# Create project directory
mkdir ~/environmental_monitor
cd ~/environmental_monitor

# Download the dashboard script
# Place environmental_dashboard.py in this directory
```

### 2. Verify Bluetooth

```bash
# Check Bluetooth status
sudo systemctl status bluetooth

# Scan for BLE devices (should see your sensors)
sudo hcitool lescan

# Press Ctrl+C to stop scanning
```

### 3. Test Connection

```bash
# Make script executable
chmod +x environmental_dashboard.py

# Run the dashboard
python3 environmental_dashboard.py
```

## 🎮 Usage

### Starting the Dashboard

```bash
cd ~/environmental_monitor
python3 environmental_dashboard.py
```

### Dashboard Interface

The dashboard is divided into several sections:

#### **Left Panel - Sensor Readings**
- **SPL Meter Section**
  - Large display showing current dBA level
  - Min/Max/Average statistics
  - Connection status indicator
  
- **Vision Node Section**
  - Large display showing people count
  - Connection status indicator

- **Connection Log**
  - Real-time logging of BLE events
  - Scan results and connection attempts
  - Error messages and diagnostics

#### **Right Panel - Graphs**
- **Top Graph**: Sound pressure level over time (last 100 samples)
- **Bottom Graph**: People count over time (last 100 samples)

#### **Top Bar**
- Application title
- Overall connection status (X/2 devices connected)

### Understanding Status Indicators

| Status | Meaning |
|--------|---------|
| **Connected ✓** (Green) | Device connected and receiving data |
| **Disconnected ✗** (Red) | Device not connected |
| **Status: 2/2 devices connected** | Both sensors active |
| **Status: 1/2 devices connected** | One sensor active |
| **Status: No devices connected** | Searching for sensors |

## 📊 Data Logging

### Automatic CSV Logging

All data is automatically logged to CSV files with timestamps:

**Filename Format**: `sensor_data_YYYYMMDD_HHMMSS.csv`

**Example**: `sensor_data_20251108_143025.csv`

### CSV Structure

```csv
Timestamp,SPL_dBA,People_Count
2025-11-08 14:30:25.123,65.2,2
2025-11-08 14:30:25.623,64.8,2
2025-11-08 14:30:26.123,66.1,3
```

### Analyzing Logged Data

```python
import pandas as pd

# Load data
df = pd.read_csv('sensor_data_20251108_143025.csv')

# Calculate statistics
print(df['SPL_dBA'].describe())
print(df['People_Count'].value_counts())

# Plot data
df.plot(x='Timestamp', y=['SPL_dBA', 'People_Count'], subplots=True)
```

## 🔍 Troubleshooting

### Device Not Found

**Problem**: Sensors don't appear in scan

**Solutions**:
1. Verify sensors are powered on and running
2. Check sensor firmware is programmed correctly
3. Ensure sensors are advertising (check Arduino Serial Monitor)
4. Verify device names match exactly:
   - SPL Meter: `SPL_Meter`
   - Vision Node: `AIVisionNode`

```bash
# Manual BLE scan
sudo hcitool lescan

# Look for your device names in the output
```

### Connection Fails

**Problem**: Device found but won't connect

**Solutions**:
1. Check Bluetooth service is running:
   ```bash
   sudo systemctl restart bluetooth
   ```

2. Clear Bluetooth cache:
   ```bash
   sudo systemctl stop bluetooth
   sudo rm -rf /var/lib/bluetooth/*
   sudo systemctl start bluetooth
   ```

3. Check BLE permissions:
   ```bash
   # Add user to bluetooth group
   sudo usermod -a -G bluetooth $USER
   # Logout and login again
   ```

4. Increase connection timeout in code (line 31):
   ```python
   CONNECTION_TIMEOUT = 30.0  # Increase from 20.0
   ```

### Frequent Disconnections

**Problem**: Sensors keep disconnecting

**Solutions**:
1. **Reduce distance** - Keep sensors within 5 meters
2. **Check power supply** - Use stable power source (wall adapter, not USB)
3. **Minimize interference**:
   - Move away from WiFi routers
   - Avoid metal obstacles
   - Turn off other BLE devices
4. **Reduce update rate** on Arduino side:
   ```cpp
   // In SPL Meter code
   #define BLE_UPDATE_INTERVAL 1000  // Change from 500 to 1000ms
   ```

### One Sensor Works, Other Doesn't

**Problem**: Only one sensor connects

**Solutions**:
1. Test each sensor individually (turn off one at a time)
2. Check the connection log for specific error messages
3. Verify UUIDs match between Arduino and Python code:
   ```python
   # Check these match your Arduino configuration
   SPL_CHAR_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214"
   VISION_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
   ```
4. Power cycle the non-working sensor

### GUI Freezes or Crashes

**Problem**: Dashboard becomes unresponsive

**Solutions**:
1. Check system resources:
   ```bash
   htop  # Monitor CPU and RAM usage
   ```
2. Reduce graph update rate (line 598):
   ```python
   interval=2000  # Change from 1000 to 2000ms
   ```
3. Reduce data buffer size (line 37):
   ```python
   MAX_DATA_POINTS = 50  # Change from 100
   ```

## 🛠️ Configuration

### Customizing Device Names

If your sensors have different names, edit these lines (17-23):

```python
# SPL Meter Configuration
SPL_DEVICE_NAME = "YOUR_SPL_NAME"

# Vision Node Configuration  
VISION_DEVICE_NAME = "YOUR_VISION_NAME"
```

### Customizing UUIDs

If you changed UUIDs in Arduino firmware, update these (18-24):

```python
SPL_SERVICE_UUID = "your-service-uuid"
SPL_CHAR_UUID = "your-characteristic-uuid"

VISION_SERVICE_UUID = "your-service-uuid"
VISION_CHAR_UUID = "your-characteristic-uuid"
```

### Adjusting Connection Parameters

```python
# Line 27-30
SCAN_TIMEOUT = 15.0          # How long to scan for devices
RECONNECT_DELAY = 5.0        # Wait between reconnection attempts
CONNECTION_TIMEOUT = 20.0    # Timeout for connection attempts
```

### Changing Data Buffer Size

```python
# Line 33
MAX_DATA_POINTS = 100  # Number of points shown in graphs
```

## 📱 Running at Startup

### Create Systemd Service

1. Create service file:
```bash
sudo nano /etc/systemd/system/envmonitor.service
```

2. Add this content:
```ini
[Unit]
Description=Environmental Monitoring Dashboard
After=bluetooth.target graphical.target

[Service]
Type=simple
User=pi
Environment=DISPLAY=:0
WorkingDirectory=/home/pi/environmental_monitor
ExecStart=/usr/bin/python3 /home/pi/environmental_monitor/environmental_dashboard.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=graphical.target
```

3. Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable envmonitor.service
sudo systemctl start envmonitor.service
```

4. Check status:
```bash
sudo systemctl status envmonitor.service
```

## 📈 Advanced Usage

### Multiple Dashboard Instances

You can run multiple dashboards for different sensor sets:

```python
# dashboard_room1.py
SPL_DEVICE_NAME = "SPL_Room1"
VISION_DEVICE_NAME = "Vision_Room1"

# dashboard_room2.py
SPL_DEVICE_NAME = "SPL_Room2"
VISION_DEVICE_NAME = "Vision_Room2"
```

### Remote Monitoring

Access dashboard remotely using VNC:

```bash
# Enable VNC on Raspberry Pi
sudo raspi-config
# Interface Options → VNC → Enable

# Connect from another computer using VNC Viewer
# Address: raspberrypi.local:5900
```

### Data Export and Analysis

```python
# Export script example
import pandas as pd
import glob

# Combine all CSV files
all_files = glob.glob("sensor_data_*.csv")
df_list = [pd.read_csv(f) for f in all_files]
combined_df = pd.concat(df_list, ignore_index=True)

# Save combined data
combined_df.to_csv('all_sensor_data.csv', index=False)

# Generate statistics
summary = combined_df.describe()
print(summary)
```

## 🐛 Debug Mode

To enable verbose logging, add this at the start of `main()`:

```python
import logging
logging.basicConfig(level=logging.DEBUG)
```

## 📄 License

This project is provided as-is for educational and research purposes.

## 🤝 Contributing

Improvements and bug fixes are welcome! Key areas for contribution:
- Better error recovery mechanisms
- Additional sensor support
- Data export formats
- Alert/notification system
- Mobile app companion

## 📞 Support

For issues and questions:
1. Check the connection log in the dashboard
2. Review the troubleshooting section
3. Test each sensor individually
4. Verify Bluetooth functionality with `hcitool`

## 🔄 Version History

### Version 3.0 (Current)
- Simplified BLE connection manager
- Sequential connection handling
- Real-time connection logging
- Improved error recovery
- Better status indicators

### Version 2.0
- Multi-device connection watchdog
- Health monitoring
- Auto-reconnection logic

### Version 1.0
- Initial release
- Basic BLE connectivity
- Real-time graphing
- CSV logging

---

**Last Updated**: November 2025  
**Tested On**: Raspberry Pi 5, Raspberry Pi OS Bookworm  
**Python Version**: 3.11+
