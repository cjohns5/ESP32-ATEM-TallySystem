# Arduino IDE Setup Files

This directory contains the proper `.ino` files for Arduino IDE compatibility:

## ✅ Ready-to-Upload Files

### Bridge Device
- **ESP32_ATEM_Bridge_BLE_v3.ino** - Main bridge firmware (upload to bridge ESP32)

### Tally Lights  
- **ESP32_Tally_Light_BLE_v2.ino** - Tally light firmware (upload to each tally ESP32)

## 🔧 Arduino IDE Instructions

### 1. Setup Arduino IDE
1. Install [Arduino IDE 2.0+](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to **File > Preferences**
   - Add to **Additional Board Manager URLs**: 
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools > Board > Board Manager**
   - Search for "ESP32" and install "ESP32 by Espressif Systems"

### 2. Install Required Libraries
In Arduino IDE, go to **Tools > Manage Libraries** and install:
- **ATEMmin** by SKAARHOJ (for bridge only)
- ESP32 BLE libraries (usually included with ESP32 board package)

### 3. Upload Firmware

#### Bridge ESP32:
1. Open `ESP32_ATEM_Bridge_BLE_v3.ino` in Arduino IDE
2. **Configure settings** (lines 46-48):
   ```cpp
   #define ATEM_IP "192.168.1.100"     // Your ATEM IP address
   #define BLE_DEVICE_NAME "ATEM_Bridge_BLE"
   #define MAX_TALLY_DEVICES 4         // Number of tally lights
   ```
3. Select **Board**: "ESP32 Dev Module" 
4. Select correct **Port** for your ESP32
5. Click **Upload**

#### Tally Light ESP32s:
1. Open `ESP32_Tally_Light_BLE_v2.ino` in Arduino IDE
2. **Configure for each device** (lines 46-48):
   ```cpp
   #define CAMERA_ID 1                    // Change to 2, 3, 4 for other cameras
   #define DEVICE_NAME "Tally_CAM_1"     // Change to "Tally_CAM_2", etc.
   ```
3. **Configure LED pins** if different (lines 51-53):
   ```cpp
   #define RED_LED_PIN 25
   #define GREEN_LED_PIN 26  
   #define BLUE_LED_PIN 27
   ```
4. Select **Board**: "ESP32 Dev Module"
5. Select correct **Port** for your ESP32
6. Click **Upload**
7. **Repeat for each tally light** with unique CAMERA_ID and DEVICE_NAME

## 📋 File Compatibility

### .ino vs .cpp Files
- **`.ino` files** = Arduino IDE native format (recommended)
- **`.cpp` files** = C++ source files (for reference/other IDEs)

Both contain identical code, but Arduino IDE expects `.ino` extensions for proper syntax highlighting and compilation.

### Legacy Files (Reference Only)
The `.cpp` files are maintained for:
- PlatformIO users
- Other development environments  
- Code reference and version control

## 🚀 Quick Start Checklist

1. ✅ Install Arduino IDE with ESP32 support
2. ✅ Install ATEMmin library  
3. ✅ Configure ATEM IP in bridge code
4. ✅ Upload bridge firmware to bridge ESP32
5. ✅ Configure unique CAMERA_ID for each tally
6. ✅ Upload tally firmware to each tally ESP32
7. ✅ Connect bridge to PC via USB with tethering enabled
8. ✅ Test tally operation with ATEM switcher

## 🔍 Need Help?

- **Hardware Setup**: See [../docs/hardware-setup.md](../docs/hardware-setup.md)
- **Software Installation**: See [../docs/software-installation.md](../docs/software-installation.md)  
- **Examples**: See [../examples/](../examples/) for complete configurations
- **Troubleshooting**: Check serial monitor output for connection status

Your ESP32 ATEM Tally System is ready to upload and use! 🎉
