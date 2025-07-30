# Troubleshooting Guide

Comprehensive troubleshooting guide for ESP32 ATEM Tally System.

## 🔧 Quick Diagnosis

Use these serial commands first to identify the issue:

### Bridge Diagnosis
```
STATUS    - Complete system overview
ATEM      - ATEM connection status
NETWORK   - Network connectivity status
BLE       - Bluetooth status and devices
DEVICES   - Registered tally lights
```

### Tally Light Diagnosis
```
STATUS       - Device connection status
TEST_LED     - Test LED functionality
RECONNECT    - Force reconnection attempt
```

## 🚨 Common Issues

### Bridge Connection Issues

#### **Bridge Not Connecting to ATEM**

**Symptoms:**
- Serial shows "ATEM: Disconnected"
- No tally data updates
- Yellow status on tally lights

**Solutions:**
1. **Check ATEM IP Configuration**
   ```cpp
   #define ATEM_IP "192.168.1.100"  // Update this IP
   ```
   - Use ATEM Software Control to find ATEM IP
   - Try ping test: `ping 192.168.1.100`

2. **Verify USB Tethering**
   - Enable USB tethering in Windows/Mac network settings
   - Check ESP32 is getting network access via USB
   - Verify PC has Ethernet connection to ATEM network

3. **Network Connectivity Test**
   ```
   NETWORK   // Check if ESP32 can reach network
   ```

4. **ATEM Switcher Checks**
   - Ensure ATEM is powered on
   - Check Ethernet cable connections
   - Verify ATEM Software Control can connect
   - Try different Ethernet port on ATEM

#### **Bridge BLE Not Working**

**Symptoms:**
- Tally lights can't find bridge
- "No devices found" in scanning

**Solutions:**
1. **Check BLE Initialization**
   ```cpp
   BLE    // Serial command to check BLE status
   ```

2. **Reset BLE Stack**
   ```cpp
   RESET  // Restart bridge completely
   ```

3. **Check UUIDs Match**
   - Verify UUIDs in bridge and tally code are identical
   - UUIDs are case-sensitive

4. **Range and Interference**
   - Move devices within 5 meters
   - Remove WiFi routers, microwaves from area
   - Try different location

### Tally Light Issues

#### **Tally Lights Not Connecting to Bridge**

**Symptoms:**
- Orange blinking LED (searching)
- "Bridge not found" in serial
- Never connects to bridge

**Solutions:**
1. **Check Bridge is Running**
   - Bridge serial should show "BLE Advertising Started"
   - Bridge STATUS command should show BLE active

2. **Verify UUIDs Match**
   ```cpp
   // Bridge and tally must have identical UUIDs
   #define BLE_SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"
   ```

3. **Check Device Name**
   ```cpp
   // Bridge scans for this exact name
   #define BLE_SERVER_NAME "ATEM_Bridge_BLE"
   ```

4. **Range Issues**
   - Start with devices 1 meter apart
   - Move closer if connection fails
   - Check for metal obstacles

#### **Frequent Disconnections**

**Symptoms:**
- Magenta blinking (connection lost)
- Connects then immediately disconnects
- Intermittent operation

**Solutions:**
1. **Power Supply Issues**
   - Use quality USB power supply (2A minimum)
   - Check battery voltage (18650 should be >3.7V)
   - Try different power source

2. **Reduce Interference**
   - Move away from WiFi networks
   - Avoid 2.4GHz devices
   - Test in different location

3. **Adjust Timeouts**
   ```cpp
   #define HEARTBEAT_TIMEOUT 15000    // Increase if disconnecting
   #define RECONNECT_INTERVAL 5000    // Adjust reconnection speed
   ```

4. **Check Connections**
   - Verify all GPIO connections are solid
   - Check for loose wires
   - Test LED functionality: `TEST_LED`

#### **LED Not Working Correctly**

**Symptoms:**
- No LED output
- Wrong colors displayed
- Dim or flickering LEDs

**Solutions:**
1. **Check Wiring**
   ```
   GPIO 25 ──[220Ω]── Red LED ──┐
   GPIO 26 ──[220Ω]── Green LED ├── Common GND
   GPIO 27 ──[220Ω]── Blue LED ──┘
   ```

2. **Test LED Hardware**
   ```cpp
   TEST_LED  // Serial command to test RGB sequence
   ```

3. **Check Resistor Values**
   - Use 220Ω resistors for 3.3V GPIO
   - Higher resistance = dimmer LEDs
   - Lower resistance = risk of GPIO damage

4. **Verify GPIO Pins**
   ```cpp
   #define RED_LED_PIN 25      // Check these match your wiring
   #define GREEN_LED_PIN 26
   #define BLUE_LED_PIN 27
   ```

5. **Brightness Settings**
   ```cpp
   #define LED_BRIGHTNESS 255        // Reduce if too bright
   #define LED_DIM_BRIGHTNESS 64     // Adjust status brightness
   ```

### ATEM Integration Issues

#### **Tally States Not Updating**

**Symptoms:**
- LEDs stay same color
- No response to ATEM switcher changes
- Serial shows old tally states

**Solutions:**
1. **Check ATEM Library Installation**
   - Install ATEMbase and ATEMmin libraries
   - Use Library Manager in Arduino IDE
   - Verify library versions are compatible

2. **Verify Camera Indexing**
   ```cpp
   // ATEM uses 0-based indexing, our display uses 1-based
   uint8_t atemIndex = cameraId - 1;  // Convert 1-20 to 0-19
   ```

3. **Check Polling Interval**
   ```cpp
   #define TALLY_CHECK_INTERVAL 100   // Reduce for faster updates
   ```

4. **ATEM Connection Verification**
   ```cpp
   ATEM      // Check ATEM connection status
   STATUS    // Complete system status
   ```

#### **Standby Preview Mode Issues**

**Symptoms:**
- All cameras showing green when should be off
- Standby mode not working as expected

**Solutions:**
1. **Check Configuration**
   ```cpp
   #define STANDBY_AS_PREVIEW true   // Enable/disable standby mode
   ```

2. **Verify Logic**
   - Standby mode shows non-active cameras as "ready" (green)
   - Only works when at least one camera is PROGRAM
   - Cameras explicitly in PREVIEW stay green

3. **Test Different Modes**
   ```cpp
   // Disable standby mode to test basic functionality
   #define STANDBY_AS_PREVIEW false
   ```

## 🔍 Advanced Debugging

### Serial Monitoring

**Bridge Serial Output (115200 baud):**
```
[BOOT] ESP32 ATEM Bridge BLE v3.0
[BLE] Initializing BLE server...
[BLE] Advertising started: ATEM_Bridge_BLE
[NETWORK] USB tethering connected
[ATEM] Connecting to 192.168.1.100...
[ATEM] Connected to ATEM Mini Pro
[MAIN] System ready - Bridge active
```

**Tally Light Serial Output (115200 baud):**
```
[BOOT] ESP32 Tally Light BLE v2.0
[BLE] Scanning for bridge...
[BLE] Found bridge: ATEM_Bridge_BLE
[BLE] Connected to bridge
[REG] Registered as Camera 1
[TALLY] State: PREVIEW (Green)
```

### Performance Monitoring

1. **Check Loop Timing**
   - Bridge should process ~10 loops per second
   - Tally lights should process ~5 loops per second
   - Slower = potential blocking issues

2. **Memory Usage**
   - Bridge typically uses 60-70% of ESP32 memory
   - Memory leaks show as gradually increasing usage
   - Reset if memory usage >90%

3. **BLE Connection Quality**
   - Strong signal: -40 to -60 dBm
   - Weak signal: -70 to -80 dBm
   - Poor signal: < -80 dBm

### Hardware Testing

#### **ESP32 Board Tests**
1. **Basic Function Test**
   ```cpp
   // Upload blink sketch to verify ESP32 functionality
   ```

2. **GPIO Test**
   ```cpp
   // Test each GPIO pin individually
   digitalWrite(25, HIGH);  // Test Red LED
   digitalWrite(26, HIGH);  // Test Green LED
   digitalWrite(27, HIGH);  // Test Blue LED
   ```

3. **Power Supply Test**
   - Measure 3.3V on ESP32 power pins
   - Check USB cable with multimeter
   - Verify battery voltage >3.7V

#### **LED Circuit Tests**
1. **Continuity Test**
   - Check wiring with multimeter
   - Verify no short circuits
   - Test resistor values

2. **Forward Voltage Test**
   - Red LED: ~2.0V
   - Green LED: ~3.2V
   - Blue LED: ~3.2V

## 📊 Error Codes and Messages

### Bridge Error Messages

| Message | Meaning | Solution |
|---------|---------|----------|
| `ATEM connection failed` | Cannot reach ATEM switcher | Check IP, network, ATEM power |
| `BLE init failed` | Bluetooth hardware issue | Reset ESP32, check power |
| `Network timeout` | USB tethering not working | Check PC network settings |
| `Memory allocation failed` | Low memory condition | Reset ESP32, check for leaks |

### Tally Light Error Messages

| Message | Meaning | Solution |
|---------|---------|----------|
| `Bridge not found` | Cannot find bridge BLE | Check bridge running, UUIDs match |
| `Connection timeout` | BLE connection failed | Reduce range, check interference |
| `Registration failed` | Bridge rejected connection | Check unique CAMERA_ID |
| `Heartbeat timeout` | Lost connection to bridge | Check range, power, interference |

## 🔄 Reset Procedures

### **Soft Reset** (Preferred)
```cpp
RESET  // Serial command - cleanly restarts system
```

### **Hard Reset**
- Press ESP32 reset button
- Power cycle the device
- Re-upload firmware if needed

### **Factory Reset**
1. Erase ESP32 flash completely
2. Re-upload bootloader
3. Upload fresh firmware
4. Reconfigure all settings

## 💡 Prevention Tips

### **Stable Operation**
1. **Use Quality Power Supplies**
   - 2A minimum for bridge
   - High-quality 18650 batteries for tally lights
   - Avoid cheap USB chargers

2. **Proper Mounting**
   - Secure all connections
   - Protect from moisture
   - Ensure good ventilation

3. **Regular Maintenance**
   - Check battery levels weekly
   - Verify connections monthly
   - Update firmware when available

### **Performance Optimization**
1. **Network Optimization**
   - Use gigabit Ethernet when possible
   - Minimize network hops to ATEM
   - Use quality Ethernet cables

2. **BLE Optimization**
   - Keep devices within 5-10 meter range
   - Minimize WiFi interference
   - Use line-of-sight connections when possible

3. **Power Management**
   - Use external power for permanent installations
   - Implement sleep modes for battery operation
   - Monitor power consumption

## 🆘 Emergency Procedures

### **Live Production Issues**
1. **Immediate Backup**
   - Have wired tally system as backup
   - Keep spare ESP32 devices programmed and ready
   - Manual camera switching procedures

2. **Quick Recovery**
   - Power cycle bridge first
   - Reset individual tally lights if needed
   - Switch to backup ATEM if available

3. **Communication**
   - Inform director/operator immediately
   - Use handheld radios as backup communication
   - Have printed camera assignments

## 📞 Getting Help

### **Community Support**
- **GitHub Issues**: Report bugs and get help
- **Arduino Forums**: ESP32 and BLE specific questions
- **ATEM User Groups**: Switcher integration questions

### **Professional Support**
- **System Integration**: For large installations
- **Custom Development**: Feature additions and modifications
- **Training**: User training and operation procedures

### **Useful Resources**
- **ESP32 Documentation**: https://docs.espressif.com/
- **ATEM Software Control**: Blackmagic Design support
- **Arduino IDE Help**: https://www.arduino.cc/reference/
- **BLE Troubleshooting**: Nordic Semiconductor resources

---

**Remember**: Most issues are related to power supply, network connectivity, or BLE range. Start with these basics before diving into complex debugging.
