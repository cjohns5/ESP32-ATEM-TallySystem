# Basic Setup Example

A simple 2-camera tally setup perfect for getting started with the ESP32 ATEM Tally System.

## Hardware Required

### ATEM Bridge (1x)
- ESP32-WROOM-32 development board
- USB cable for tethering to PC/ATEM
- Basic breadboard and jumper wires (optional, for testing)

### Tally Lights (2x)  
- ESP32-WROOM-32 development board
- RGB LED (common cathode) or 3x individual LEDs
- 220Ω resistors (3x per tally light)
- Small enclosure or mounting solution
- Battery pack (3.7V LiPo recommended)

## Wiring Diagram

### Tally Light Wiring
```
ESP32 Pin | Component
----------|----------
GPIO 25   | Red LED + (through 220Ω resistor)
GPIO 26   | Green LED + (through 220Ω resistor)  
GPIO 27   | Blue LED + (through 220Ω resistor)
GND       | LED common cathode (-)
```

## Configuration

### Bridge Configuration
```cpp
// Network settings
#define ATEM_IP "192.168.1.100"        // Your ATEM IP address
#define MAX_TALLY_DEVICES 2             // Only 2 cameras for basic setup

// Camera assignments
#define CAM1_DEVICE_ID 1                // First tally light
#define CAM2_DEVICE_ID 2                // Second tally light

// Performance settings (basic)
#define TALLY_CHECK_INTERVAL 150        // Slightly relaxed for stability
#define HEARTBEAT_INTERVAL 5000         // Standard monitoring
```

### Tally Light Configuration
```cpp
// Device identification
#define DEVICE_ID 1                     // Set to 1 for first camera, 2 for second

// LED pins
#define RED_LED_PIN 25
#define GREEN_LED_PIN 26
#define BLUE_LED_PIN 27

// LED settings
#define LED_BRIGHTNESS 128              // Medium brightness for good battery life
```

## Setup Steps

### 1. Prepare Hardware
1. Flash bridge ESP32 with `ESP32_ATEM_Bridge_BLE_v3.cpp`
2. Flash both tally ESP32s with `ESP32_Tally_Light_BLE_v2.cpp`
3. Wire LEDs to tally lights according to diagram
4. Set `DEVICE_ID` to 1 for first tally, 2 for second

### 2. Network Setup
1. Connect ATEM switcher to network
2. Note ATEM IP address (found in ATEM Software Control)
3. Update `ATEM_IP` in bridge configuration
4. Connect bridge ESP32 to PC via USB
5. Enable USB tethering/internet sharing to bridge

### 3. Initial Testing
1. Power on ATEM switcher
2. Power on bridge ESP32 (via USB)
3. Power on tally lights (via battery)
4. Wait 30 seconds for connections to establish
5. Test tally states by switching cameras in ATEM software

## Expected Behavior

### Normal Operation
- **RED**: Camera is LIVE (on air)
- **GREEN**: Camera is in PREVIEW (ready to cut to)
- **BLUE**: Standby preview mode (when production is active)
- **OFF**: Camera is not selected

### Startup Sequence
1. Tally lights flash BLUE during connection
2. Bridge connects to ATEM (may take 10-15 seconds)
3. Tally lights show current state from ATEM

## Troubleshooting

### Tally Light Won't Connect
- Check battery charge level
- Verify `DEVICE_ID` is unique (1 or 2)
- Move closer to bridge (within 10 meters)
- Restart tally light

### Bridge Can't Reach ATEM
- Verify USB tethering is active
- Check `ATEM_IP` address is correct
- Ensure ATEM is powered on and connected to network
- Try pinging ATEM IP from PC

### LEDs Not Working
- Check wiring matches diagram exactly
- Verify resistor values (220Ω)
- Test LEDs individually with multimeter
- Check GPIO pin assignments in code

## Performance Characteristics

- **Latency**: ~200ms tally response time
- **Range**: 10-15 meters between bridge and tally lights
- **Battery Life**: 8-12 hours with 2000mAh LiPo battery
- **Startup Time**: 30-45 seconds from power-on to operational

## Scaling This Setup

To expand beyond 2 cameras:
1. Increase `MAX_TALLY_DEVICES` in bridge code
2. Add more tally lights with unique `DEVICE_ID` values (3, 4, etc.)
3. Consider switching to higher capacity bridge (more concurrent BLE connections)
4. Monitor performance - may need to adjust `TALLY_CHECK_INTERVAL`

## Next Steps

Once comfortable with basic setup, consider:
- [Multi-Camera Example](../multi-camera/) for 4-camera setup
- [Battery-Optimized Example](../battery-optimized/) for longer runtime
- [NeoPixel Version](../neopixel-version/) for brighter indicators
