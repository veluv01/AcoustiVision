# AcoustiVision

### *Distributed Acoustic and Visual Environmental Monitoring*

![AcoustiVision](https://img.shields.io/badge/AcoustiVision-v3.0-blue)
![Python](https://img.shields.io/badge/python-3.7+-green)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red)
![License](https://img.shields.io/badge/license-MIT-yellow)

---

## 🎯 Overview

**AcoustiVision** is a distributed environmental monitoring system that combines acoustic analysis and AI-powered vision to provide comprehensive real-time insights into indoor spaces. The system uses multiple BLE sensor nodes and a centralized Raspberry Pi dashboard for data collection, visualization, and analysis.

### What Makes AcoustiVision Unique?

- **Multi-Modal Sensing**: Combines sound pressure level (SPL) monitoring with AI person detection
- **Distributed Architecture**: Multiple independent sensor nodes communicate via BLE
- **Real-Time Visualization**: Live graphs and statistics on an intuitive dashboard
- **Automatic Data Logging**: All sensor data timestamped and saved to CSV
- **Smart Reconnection**: Robust connection management with automatic recovery

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi Dashboard                    │
│              (Central Data Collection & Display)             │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Live SPL   │  │  Occupancy   │  │ Connection   │     │
│  │   Display    │  │   Counter    │  │     Log      │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │        Real-Time Graphs & Statistics               │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                           │
                    BLE Connection
                           │
          ┌────────────────┴────────────────┐
          │                                 │
    ┌─────▼─────┐                    ┌─────▼─────┐
    │  SPL Node │                    │Vision Node│
    │(MG24 + Mic)                    │(ESP32-C3) │
    │           │                    │  + Grove  │
    │  Acoustic │                    │  AI V2    │
    │ Monitoring│                    │  Camera   │
    └───────────┘                    └───────────┘
```

---

## 📊 Sensor Nodes

### 🔊 Acoustic Node - SPL Meter
- **Hardware**: Seeed Studio XIAO MG24 + Analog Microphone
- **Function**: A-weighted sound pressure level measurement
- **Range**: 30-120 dBA
- **Update Rate**: 2 Hz (500ms)
- **BLE Service**: Custom SPL measurement service
- **Output**: Float (dBA value)

### 👁️ Vision Node - AI Person Detection
- **Hardware**: Seeed Studio XIAO ESP32-C3 + Grove Vision AI V2
- **Function**: Real-time person detection and counting
- **AI Model**: SSCMA PeopleNet
- **Update Rate**: 1 Hz (1 second)
- **BLE Service**: Custom occupancy service
- **Output**: Integer (person count)

---

## 🎨 Dashboard Features

### Real-Time Monitoring
- **Large Displays**: Easy-to-read current values for both sensors
- **Live Graphs**: Scrolling time-series plots (last 100 samples)
- **Statistics**: Min/Max/Average calculations for SPL data
- **Status Indicators**: Clear connection health visualization

### Data Logging
- **Automatic CSV Export**: All data timestamped and logged
- **Synchronized Data**: SPL and occupancy data aligned by timestamp
- **Easy Analysis**: CSV format compatible with Excel, Python, R

### Connection Management
- **Real-Time Log**: See exactly what's happening with connections
- **Auto-Reconnect**: Automatic recovery from disconnections
- **Health Monitoring**: Visual indicators for connection quality
- **Detailed Diagnostics**: Error messages and troubleshooting info

---

## 🔧 Hardware Requirements

### Central Hub
- **Raspberry Pi** (Model 3B+, 4, or 5)
  - Built-in Bluetooth or USB BLE adapter
  - Running Raspberry Pi OS (Bullseye or newer)
  - Display (HDMI monitor or touchscreen)
  - Minimum 2GB RAM recommended

### Sensor Nodes

#### SPL Meter Node
- Seeed Studio XIAO MG24 (Sense)
- Analog microphone (built-in or external)
- USB-C cable for power/programming

#### Vision Node
- Seeed Studio XIAO ESP32-C3
- Grove Vision AI V2 module
- Grove cable for I2C connection
- USB-C cable for power/programming

### Optional
- USB power adapters (stable 5V supply recommended)
- Enclosures for sensor nodes
- Tripods or mounting hardware

---

## 📦 Software Requirements

### Raspberry Pi Dashboard

#### System Dependencies
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install python3-pip python3-tk bluetooth bluez

# Enable Bluetooth
sudo systemctl enable bluetooth
sudo systemctl start bluetooth
```

#### Python Dependencies
```bash
pip3 install bleak matplotlib pandas
```

**requirements.txt**:
```
bleak>=0.21.0
matplotlib>=3.5.0
pandas>=1.3.0
```

### Sensor Node Firmware

#### SPL Meter (XIAO MG24)
- Arduino IDE with Seeed XIAO MG24 board support
- Libraries:
  - `SilabsMicrophoneAnalog`
  - `ArduinoBLE`
  - Custom `SPL_Meter` class

#### Vision Node (ESP32-C3)
- Arduino IDE with ESP32 board support
- Libraries:
  - `Seeed_Arduino_SSCMA`
  - ESP32 BLE (native)
  - `Wire` (I2C)

---

## 🚀 Quick Start Guide

### 1. Program Sensor Nodes

#### Flash SPL Meter
```bash
# Open Arduino IDE
# Load: acousticNode.ino along with SPL_Meter.h and SPL_Meter.cpp
# Select Board: "Seeed XIAO MG24 (Sense)"
# Upload
```

#### Flash Vision Node
```bash
# Open Arduino IDE
# Load: aiVisionNode.ino
# Select Board: "XIAO_ESP32C3"
# Upload
```

### 2. Set Up Raspberry Pi

```bash
# Create project directory
mkdir ~/acoustivision
cd ~/acoustivision/dashboard

# Install dependencies
pip3 install -r requirements.txt
```

### 3. Run Dashboard

```bash
cd ~/acoustivision
python3 environmental_dashboard.py
```

### 4. Verify Connections

Watch the **Connection Log** panel:
- ✓ Devices should appear in scan
- ✓ Connections should succeed
- ✓ Data should start flowing

---

## 🎮 Using AcoustiVision

### Dashboard Layout

```
┌─────────────────────────────────────────────────────────┐
│  AcoustiVision Dashboard          Status: 2/2 Connected │
├──────────────┬──────────────────────────────────────────┤
│              │                                          │
│  SPL METER   │                                          │
│              │         SOUND LEVEL GRAPH                │
│   65.2 dBA   │                                          │
│  Connected ✓ │                                          │
│              ├──────────────────────────────────────────┤
│  Min: 45.1   │                                          │
│  Max: 78.3   │         OCCUPANCY GRAPH                  │
│  Avg: 62.7   │                                          │
│              │                                          │
├──────────────┤                                          │
│              │                                          │
│ VISION NODE  │                                          │
│              │                                          │
│   3 people   │                                          │
│  Connected ✓ │                                          │
│              │                                          │
├──────────────┤                                          │
│ CONNECTION   │                                          │
│     LOG      │                                          │
│              │                                          │
│ [12:34:56]   │                                          │
│ SPL: 65.2 dBA│                                          │
│ [12:34:57]   │                                          │
│ Vision: 3    │                                          │
└──────────────┴──────────────────────────────────────────┘
```

### Reading Status Indicators

| Indicator | Meaning |
|-----------|---------|
| **Connected ✓** (Green) | Device active, receiving data |
| **Disconnected ✗** (Red) | Device not connected |
| **Status: 2/2 devices connected** | All sensors online |
| **Status: 1/2 devices connected** | Partial connectivity |
| **Status: No devices connected** | Searching for sensors |

### Data Files

AcoustiVision automatically creates CSV log files:

**Format**: `sensor_data_YYYYMMDD_HHMMSS.csv`

**Example**: `sensor_data_20251108_143530.csv`

**Contents**:
```csv
Timestamp,SPL_dBA,People_Count
2025-11-08 14:35:30.123,65.2,3
2025-11-08 14:35:30.623,64.8,3
2025-11-08 14:35:31.123,66.1,2
```

---

## 🔍 Troubleshooting

### Sensors Not Found

**Check scan results in Connection Log**

1. Verify sensors are powered and running
2. Check LED indicators on boards
3. Confirm device names match:
   - SPL Meter: `SPL_Meter`
   - Vision Node: `AIVisionNode`

```bash
# Manual BLE scan
sudo hcitool lescan
# Look for your device names
```

### Connection Failures

**"Connection timeout" errors**

1. Restart Bluetooth service:
   ```bash
   sudo systemctl restart bluetooth
   ```

2. Check Bluetooth permissions:
   ```bash
   sudo usermod -a -G bluetooth $USER
   # Logout and login
   ```

3. Reduce distance between devices (< 5 meters)

4. Power cycle sensor nodes

### Frequent Disconnections

**Devices keep dropping**

1. **Power Issues**: Use wall adapters, not laptop USB
2. **Interference**: Move away from WiFi routers
3. **Distance**: Keep sensors within 5-10 meters
4. **Reduce Update Rate**: 
   ```cpp
   // In Arduino code
   #define BLE_UPDATE_INTERVAL 1000  // Slower = more stable
   ```

### One Sensor Works, Other Doesn't

1. Test each sensor individually (power off one)
2. Check UUIDs match between firmware and dashboard
3. Verify characteristic formats:
   - SPL: Float (4 bytes)
   - Vision: Uint8 (1 byte)

---

## ⚙️ Configuration

### Customizing Device Names

Edit `acoustivision_dashboard.py` (lines 17-23):

```python
# SPL Meter Configuration
SPL_DEVICE_NAME = "YOUR_SPL_NAME_HERE"

# Vision Node Configuration
VISION_DEVICE_NAME = "YOUR_VISION_NAME_HERE"
```

### Adjusting Connection Parameters

```python
# Line 27-30
SCAN_TIMEOUT = 15.0          # Scan duration
RECONNECT_DELAY = 5.0        # Wait between reconnects
CONNECTION_TIMEOUT = 20.0    # Connection attempt timeout
```

### Changing Graph Settings

```python
# Line 33
MAX_DATA_POINTS = 100  # Points shown in graphs (50-200 recommended)
```

---

## 📈 Use Cases

### Smart Buildings
- Correlate noise levels with occupancy
- Optimize HVAC based on actual room usage
- Energy efficiency monitoring

### Research & Academia
- Environmental psychology studies
- Acoustic comfort analysis
- Occupancy pattern research
- Building performance evaluation

### Workplace Analytics
- Meeting room utilization
- Noise pollution monitoring
- Workspace optimization
- Productivity environment studies

### Security & Safety
- Unusual occupancy detection
- Emergency evacuation monitoring
- After-hours activity tracking
- Intrusion detection via noise

### Healthcare
- Patient room monitoring
- Hospital noise management
- Occupancy tracking for infection control
- Staff activity analysis

---

## 🔬 Data Analysis Examples

### Basic Analysis

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('sensor_data_20251108_143530.csv')
df['Timestamp'] = pd.to_datetime(df['Timestamp'])

# Calculate statistics
print("SPL Statistics:")
print(df['SPL_dBA'].describe())

print("\nOccupancy Statistics:")
print(df['People_Count'].value_counts())

# Plot correlation
plt.figure(figsize=(10, 5))
plt.scatter(df['People_Count'], df['SPL_dBA'], alpha=0.5)
plt.xlabel('People Count')
plt.ylabel('SPL (dBA)')
plt.title('Noise Level vs Occupancy')
plt.show()
```

### Advanced Analysis

```python
# Hourly averages
df['Hour'] = df['Timestamp'].dt.hour
hourly_avg = df.groupby('Hour').agg({
    'SPL_dBA': 'mean',
    'People_Count': 'mean'
})

# Peak times
peak_occupancy = df.loc[df['People_Count'].idxmax()]
print(f"Peak occupancy: {peak_occupancy['People_Count']} at {peak_occupancy['Timestamp']}")

# Quiet periods (< 50 dBA and 0 people)
quiet_periods = df[(df['SPL_dBA'] < 50) & (df['People_Count'] == 0)]
print(f"Quiet time: {len(quiet_periods)} samples ({len(quiet_periods)/len(df)*100:.1f}%)")
```

---

## 🚀 Advanced Features

### Running at Startup

Create systemd service:

```bash
sudo nano /etc/systemd/system/acoustivision.service
```

```ini
[Unit]
Description=AcoustiVision Environmental Monitor
After=bluetooth.target graphical.target

[Service]
Type=simple
User=pi
Environment=DISPLAY=:0
WorkingDirectory=/home/pi/acoustivision
ExecStart=/usr/bin/python3 /home/pi/acoustivision/acoustivision_dashboard.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=graphical.target
```

Enable:
```bash
sudo systemctl daemon-reload
sudo systemctl enable acoustivision.service
sudo systemctl start acoustivision.service
```

### Remote Access

```bash
# Enable VNC
sudo raspi-config
# Interface Options → VNC → Enable

# Access from another computer
# VNC Viewer: raspberrypi.local:5900
```

### Multiple Rooms

Deploy multiple sensor sets:

```python
# Room 1
SPL_DEVICE_NAME = "SPL_Room1"
VISION_DEVICE_NAME = "Vision_Room1"

# Room 2
SPL_DEVICE_NAME = "SPL_Room2"
VISION_DEVICE_NAME = "Vision_Room2"
```

---

## 🐛 Debug Mode

Enable verbose logging:

```python
# At start of main()
import logging
logging.basicConfig(level=logging.DEBUG)
```

---

## 🤝 Contributing

We welcome contributions! Areas for improvement:

- [ ] Additional sensor types (temperature, humidity, CO2)
- [ ] Mobile app companion
- [ ] Cloud data sync
- [ ] Alert/notification system
- [ ] Machine learning analysis
- [ ] Web-based dashboard
- [ ] Multi-location aggregation

---

## 📄 License

MIT License - See LICENSE file for details

---

## 🙏 Acknowledgments

- Seeed Studio for XIAO and Grove Vision AI v2 hardware platforms
- Bleak Python BLE library
- SSCMA AI framework
- Arduino, Silicon Labs and ESP32 communities

---


## 📊 System Requirements Summary

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| **Raspberry Pi** | 3B+ | 4B (4GB) |
| **OS Version** | Bullseye | Bookworm |
| **Python** | 3.7 | 3.11+ |
| **RAM** | 1GB | 2GB+ |
| **Storage** | 8GB | 16GB+ |
| **Display** | 1024x768 | 1920x1080 |
| **BLE Range** | 5m | 10m |

---

<div align="center">

**AcoustiVision** - *Where Sound Meets Vision*

![Acoustic Icon](https://img.shields.io/badge/🔊-Acoustic-blue) ![Vision Icon](https://img.shields.io/badge/👁️-Vision-green)

*Built with ❤️ for environmental monitoring*

</div>
