# Software Installation Guide

## Prerequisites

### Required Software
- **Arduino IDE** (version 1.8.19 or newer) OR **PlatformIO**
- **ESP32 Board Package** for Arduino IDE
- **USB Drivers** for ESP32 (CP210x or CH340)

### Required Libraries
- **ATEMbase** (SKAARHOJ)
- **ATEMmin** (SKAARHOJ) 
- **ESP32 BLE Arduino** (included with ESP32 package)

## Step-by-Step Installation

### 1. Install Arduino IDE

#### Windows/Mac/Linux
1. Download Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Install following standard procedures
3. Launch Arduino IDE

### 2. Install ESP32 Board Support

#### Method 1: Board Manager (Recommended)
1. Open Arduino IDE
2. Go to **File** → **Preferences**
3. In "Additional Board Manager URLs" add:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. Go to **Tools** → **Board** → **Boards Manager**
5. Search for "ESP32"
6. Install **"ESP32 by Espressif Systems"**
7. Restart Arduino IDE

#### Method 2: Git Installation
```bash
cd ~/Documents/Arduino/hardware
mkdir espressif
cd espressif
git clone https://github.com/espressif/arduino-esp32.git esp32
cd esp32
git submodule update --init --recursive
cd tools
python get.py
```

### 3. Install Required Libraries

#### ATEMbase and ATEMmin Libraries

**Option A: Download from SKAARHOJ**
1. Visit [SKAARHOJ GitHub](https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering)
2. Download the Arduino Libraries
3. Extract to Arduino libraries folder:
   - Windows: `Documents\Arduino\libraries\`
   - Mac: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`

**Option B: Git Clone (Advanced)**
```bash
cd ~/Documents/Arduino/libraries/
git clone https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering.git
cp -r SKAARHOJ-Open-Engineering/ArduinoLibs/* ./
```

#### Verify Library Installation
The following libraries should appear in **Sketch** → **Include Library**:
- ATEMbase
- ATEMmin
- ESP32 BLE Arduino

### 4. Configure Arduino IDE for ESP32

1. **Select Board**: **Tools** → **Board** → **ESP32 Arduino** → **ESP32 Dev Module**
2. **Set Parameters**:
   - Upload Speed: **921600**
   - CPU Frequency: **240MHz (WiFi/BT)**
   - Flash Frequency: **80MHz**
   - Flash Mode: **QIO**
   - Flash Size: **4MB (32Mb)**
   - Partition Scheme: **Default 4MB with spiffs**
   - Core Debug Level: **None** (or **Info** for debugging)
   - PSRAM: **Disabled**

### 5. Install USB Drivers

#### Identify Your ESP32 Board
Most ESP32 boards use one of these USB chips:
- **CP2102/CP2104** (Silicon Labs)
- **CH340** (WCH)

#### Download and Install Drivers

**CP210x Drivers**
- Download from [Silicon Labs](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers)
- Install following manufacturer instructions

**CH340 Drivers**  
- Download from [WCH](http://www.wch.cn/products/CH340.html)
- Install following manufacturer instructions

#### Verify Driver Installation
1. Connect ESP32 to computer via USB
2. Check if COM port appears in **Tools** → **Port**
3. On Windows: Device Manager should show "Silicon Labs CP210x" or "USB-SERIAL CH340"

## Project Setup

### 1. Download Project Files

#### Option A: Git Clone
```bash
git clone https://github.com/username/ESP32-ATEM-Tally-System.git
cd ESP32-ATEM-Tally-System
```

#### Option B: Download ZIP
1. Download ZIP from GitHub
2. Extract to desired folder
3. Navigate to project folder

### 2. Configure Bridge Device

1. **Open Bridge Code**: `src/ESP32_ATEM_Bridge_BLE_v3.cpp`
2. **Configure Network Settings**:
   ```cpp
   #define ATEM_IP "192.168.1.100"     // Your ATEM switcher IP
   ```
3. **Optional Configuration**:
   ```cpp
   #define MAX_TALLY_DEVICES 4         // Max simultaneous connections
   #define STANDBY_AS_PREVIEW true     // Standby preview mode
   #define TALLY_CHECK_INTERVAL 100    // Polling interval (ms)
   ```

### 3. Configure Tally Light Devices

1. **Open Tally Code**: `src/ESP32_Tally_Light_BLE_v2.cpp`
2. **Set Camera ID** (unique for each device):
   ```cpp
   #define CAMERA_ID 1                 // Camera number (1-20)
   #define DEVICE_NAME "Tally_CAM_1"   // Unique device name
   ```
3. **Optional Configuration**:
   ```cpp
   #define LED_BRIGHTNESS 128          // LED brightness (0-255)
   #define HEARTBEAT_TIMEOUT 15000     // Connection timeout (ms)
   ```

### 4. Upload Code to Devices

#### Bridge Device
1. Connect bridge ESP32 to computer
2. Select correct **Port** in **Tools** menu
3. Open `ESP32_ATEM_Bridge_BLE_v3.cpp`
4. Click **Upload** (Ctrl+U)
5. Wait for "Done uploading" message

#### Tally Light Devices
1. Connect first tally ESP32 to computer
2. Open `ESP32_Tally_Light_BLE_v2.cpp`
3. Set `CAMERA_ID 1` and `DEVICE_NAME "Tally_CAM_1"`
4. Click **Upload**
5. Repeat for additional tally lights with incremented camera IDs

## Network Configuration

### 1. PC Network Setup

#### Windows 10/11
1. Connect PC to same network as ATEM via Ethernet
2. Connect ESP32 bridge to PC via USB
3. **Enable USB Tethering**:
   - Go to **Settings** → **Network & Internet** → **Mobile hotspot**
   - Turn on **Share my Internet connection with other devices**
   - **Share over**: **USB**

#### macOS
1. Connect PC to same network as ATEM via Ethernet  
2. Connect ESP32 bridge to PC via USB
3. **Enable Internet Sharing**:
   - Go to **System Preferences** → **Sharing**
   - Select **Internet Sharing**
   - **Share connection from**: **Ethernet**
   - **To computers using**: **USB**
   - Check **Internet Sharing** to enable

#### Linux
1. Connect PC to same network as ATEM via Ethernet
2. Connect ESP32 bridge to PC via USB
3. **Enable USB Tethering**:
   ```bash
   # Enable IP forwarding
   sudo sysctl net.ipv4.ip_forward=1
   
   # Configure USB interface (adjust interface names as needed)
   sudo ip addr add 192.168.42.1/24 dev usb0
   sudo ip link set usb0 up
   
   # Set up NAT
   sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
   sudo iptables -A FORWARD -i usb0 -o eth0 -j ACCEPT
   sudo iptables -A FORWARD -i eth0 -o usb0 -m state --state RELATED,ESTABLISHED -j ACCEPT
   ```

### 2. ATEM Network Configuration

1. **Find ATEM IP Address**:
   - Check ATEM LCD menu: **Network** → **Ethernet**
   - Use Blackmagic ATEM Software Control
   - Use network scanner tool

2. **Update Bridge Configuration**:
   ```cpp
   #define ATEM_IP "YOUR_ATEM_IP_HERE"
   ```

3. **Test Network Connectivity**:
   - Ping ATEM from PC: `ping YOUR_ATEM_IP`
   - Verify same subnet (e.g., 192.168.1.x)

## Testing Installation

### 1. Bridge Testing

1. **Open Serial Monitor** (Tools → Serial Monitor)
2. **Set Baud Rate**: 115200
3. **Power on Bridge** - should see:
   ```
   ESP32 ATEM Bridge v3.0
   BLE Multi-Device + ATEMmin Library
   USB Tethering Mode (No WiFi)
   ✓ BLE server initialized and advertising
   ✓ USB Tethering Network Connected!
   ✓ Connected to ATEM switcher via ATEMmin library
   ESP32 Tally Bridge Ready!
   ```

4. **Test Commands**:
   ```
   STATUS    - Should show all systems connected
   ATEM      - Should show "Connected"
   NETWORK   - Should show IP address
   BLE       - Should show "Active" advertising
   ```

### 2. Tally Light Testing

1. **Power on Tally Light**
2. **Check LED Sequence**:
   - Orange blinking: Searching for bridge
   - Blue solid/pulsing: Connected to bridge
   - Green/Red: Receiving tally data

3. **Verify Registration**:
   - Bridge Serial Monitor should show:
     ```
     ✓ Registered BLE tally: Tally_CAM_1 (CAM1) [slot 0]
     ```

### 3. End-to-End Testing

1. **Change Camera States** on ATEM switcher
2. **Verify Tally Responses**:
   - Red LED: Camera is LIVE/PROGRAM
   - Green LED: Camera is PREVIEW or STANDBY
   - Blue LED: Camera is OFF

3. **Test Multiple Cameras**:
   - Switch between different cameras
   - Verify each tally responds to its assigned camera only
   - Test standby preview mode (non-active cameras show green when production active)

## Troubleshooting Installation

### Compilation Errors

**"ATEMbase.h: No such file or directory"**
- Verify SKAARHOJ libraries are installed correctly
- Check library folder names match exactly
- Restart Arduino IDE

**"Bluetooth not supported"**
- Ensure ESP32 board package is installed
- Select correct board (ESP32 Dev Module)
- Check board package version (use latest)

**"WiFi.h: No such file or directory"**
- ESP32 board package not installed correctly
- Reinstall ESP32 board support

### Upload Errors

**"Failed to connect to ESP32"**
- Check USB cable and connection
- Install proper USB drivers  
- Try different USB port
- Hold BOOT button during upload if needed

**"Permission denied" (Linux/Mac)**
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Runtime Errors

**Bridge not connecting to ATEM**
- Verify ATEM IP address is correct
- Check USB tethering is working
- Test network connectivity from PC
- Ensure ATEM is on same network segment

**Tally lights not connecting**
- Check BLE is enabled and advertising
- Verify device names are unique
- Test at close range first
- Check Serial Monitor for error messages

**No tally state changes**
- Verify ATEM connection is stable
- Check camera assignments match
- Test with manual commands: `CAM1:PROGRAM`
- Verify ATEM switcher is sending tally data

For additional troubleshooting, see [Troubleshooting Guide](troubleshooting.md).
