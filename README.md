# ESP32 ATEM Tally System

A wireless tally light system for Blackmagic ATEM switchers using ESP32 microcontrollers with BLE (Bluetooth Low Energy) communication.

![ESP32 ATEM Tally System](docs/images/system-overview.png)

## 🎯 Features

### **Bridge (Server)**
- **USB Tethering Network**: No WiFi required - uses PC's Ethernet connection
- **ATEMmin Library**: Proven SKAARHOJ library for robust ATEM communication
- **BLE Multi-Device**: Supports up to 4 simultaneous tally light connections
- **Secure Communication**: BLE pairing and encryption
- **Auto-Reconnection**: Automatic recovery from network/ATEM connection loss
- **Standby Preview Mode**: Shows inactive cameras as "ready" when production is active

### **Tally Lights (Clients)**
- **Individual Registration**: Each tally light registers with unique camera ID
- **RGB LED Indicators**: Clear visual status with brightness control
- **Connection Monitoring**: Heartbeat detection with visual connection status
- **Low Power Design**: Optimized for battery operation
- **Multiple States**: Program (Red), Preview/Standby (Green), Off (Blue), Status indicators

## 🏗️ System Architecture

```
ATEM Switcher ──Ethernet Network──┐
                                  │
PC ──Ethernet Cable──────────────┘
│ (USB Tethering - shares Ethernet connection)
│
ESP32 Bridge ──BLE Secure──> Multiple Tally ESP32s
                            (Camera 1, 2, 3, 4...)
```

## 📋 Hardware Requirements

### **Bridge Device**
- ESP32-WROOM-32 development board
- USB cable for PC connection and power
- PC with Ethernet connection to ATEM switcher

### **Tally Light (per camera)**
- ESP32-WROOM-32 development board
- RGB LED (common cathode) or WS2812 NeoPixel
- 3x 220Ω resistors (for RGB LED)
- Battery pack (18650 Li-ion recommended)
- Enclosure/housing

## 🚀 Quick Start

### **1. Hardware Setup**

#### Bridge Wiring
```
ESP32 Bridge:
- Connect to PC via USB cable
- No additional wiring required
```

#### Tally Light Wiring
```
ESP32 Tally Light:
GPIO 25 ──[220Ω]── Red LED ──┐
GPIO 26 ──[220Ω]── Green LED ├── Common GND
GPIO 27 ──[220Ω]── Blue LED ──┘
```

### **2. Software Installation**

1. **Install Arduino IDE** with ESP32 board support
2. **Install Required Libraries**:
   - ATEMbase (SKAARHOJ)
   - ATEMmin (SKAARHOJ)
   - ESP32 BLE Arduino

3. **Configure Network**:
   - Update `ATEM_IP` in bridge code
   - Enable USB tethering on PC
   - Connect PC to same network as ATEM

4. **Upload Code**:
   - Upload `ESP32_ATEM_Bridge_BLE_v3.ino` to bridge ESP32
   - Upload `ESP32_Tally_Light_BLE_v2.ino` to each tally ESP32
   - Set unique `CAMERA_ID` for each tally light

### **3. Operation**

1. Power on bridge (connects to ATEM automatically)
2. Power on tally lights (auto-pair with bridge)
3. Tally lights show current camera states:
   - **Red**: Camera is LIVE/PROGRAM
   - **Green**: Camera is PREVIEW or STANDBY (ready)
   - **Blue**: Camera is OFF but connected
   - **Orange**: Connecting to bridge
   - **Yellow**: Connected but ATEM offline
   - **Magenta**: Connection lost

## 🎛️ Configuration

### **Bridge Configuration** (`ESP32_ATEM_Bridge_BLE_v3.ino`)
```cpp
#define ATEM_IP "192.168.1.100"        // ATEM switcher IP
#define MAX_TALLY_DEVICES 4            // Max simultaneous connections
#define STANDBY_AS_PREVIEW true        // Show inactive cameras as ready
#define TALLY_CHECK_INTERVAL 100       // Polling interval (ms)
```

### **Tally Light Configuration** (`ESP32_Tally_Light_BLE_v2.ino`)
```cpp
#define CAMERA_ID 1                    // Camera number (1-20)
#define DEVICE_NAME "Tally_CAM_1"      // Unique device name
#define LED_BRIGHTNESS 128             // LED brightness (0-255)
```

## 📊 LED Status Indicators

| Color | Pattern | Status |
|-------|---------|--------|
| 🔴 Red | Solid | Camera LIVE/PROGRAM (On Air) |
| 🟢 Green | Solid | Camera PREVIEW or STANDBY (Ready) |
| 🔵 Blue | Pulsing | Camera OFF but connected |
| 🟡 Yellow | Slow Pulse | Bridge connected, ATEM offline |
| 🟠 Orange | Fast Blink | Connecting to bridge |
| 🟣 Magenta | Blink | Connection lost |
| ⚪ White | Flash | Data received/processed |

## 🛠️ Advanced Features

### **Serial Commands** (Bridge)
Connect to bridge via Serial Monitor (115200 baud):

```
STATUS      - Show system status
ATEM        - Show ATEM connection status  
BLE         - Show BLE devices status
DEVICES     - List registered tally devices
NETWORK     - Show network status
STANDBY     - Show standby preview status
CAM1:PROGRAM - Manual test commands
RESET       - Restart bridge
HELP        - Show all commands
```

### **Standby Preview Mode**
When enabled, cameras not in PROGRAM or PREVIEW will show as GREEN (ready/standby) when any camera is actively broadcasting. This improves operator awareness of which cameras are "ready for action" vs truly inactive.

### **Connection Monitoring**
- 5-second heartbeat signals ensure connection health
- 15-second timeout detection with visual indicators
- Automatic reconnection with exponential backoff
- Bridge status reporting (ATEM connected/disconnected)

## 🔧 Troubleshooting

### **Common Issues**

**Bridge not connecting to ATEM:**
- Verify ATEM IP address in configuration
- Check USB tethering is enabled and working
- Ensure PC and ATEM are on same network
- Check ATEM is powered on and accessible

**Tally lights not connecting:**
- Verify BLE is working (check Serial Monitor)
- Ensure devices are within 10-meter range
- Check for RF interference
- Verify unique CAMERA_ID for each device

**High latency/delayed responses:**
- Reduce `TALLY_CHECK_INTERVAL` (minimum 25ms)
- Check network latency to ATEM
- Verify stable power supply
- Check for wireless interference

**Battery life issues:**
- Use high-capacity 18650 batteries
- Implement sleep modes during idle
- Reduce LED brightness
- Consider external power for permanent installations

## 📈 Performance

- **System Latency**: 65-160ms typical
- **Battery Life**: 8-12 hours (depending on usage)
- **Range**: Up to 10 meters (BLE)
- **Capacity**: 4 simultaneous tally lights per bridge
- **Scalability**: Multiple bridges for larger setups

## 🔒 Security

- BLE encryption and secure pairing
- Message integrity checking with checksums
- Device authentication and registration
- No WiFi passwords or network credentials required

## 📚 Documentation

- [Hardware Setup Guide](docs/hardware-setup.md)
- [Software Installation](docs/software-installation.md)
- [Troubleshooting Guide](docs/troubleshooting.md)
- [API Reference](docs/api-reference.md)
- [Performance Optimization](docs/performance-optimization.md)

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **SKAARHOJ** for the ATEMmin library
- **Blackmagic Design** for ATEM switchers
- **Espressif** for ESP32 platform
- Community contributors and testers

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/username/ESP32-ATEM-Tally-System/issues)
- **Discussions**: [GitHub Discussions](https://github.com/username/ESP32-ATEM-Tally-System/discussions)
- **Wiki**: [Project Wiki](https://github.com/username/ESP32-ATEM-Tally-System/wiki)

---

⭐ **Star this repository if it helped you!**
