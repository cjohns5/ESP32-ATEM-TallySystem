# ESP32 ATEM Tally System - API Reference

Complete technical documentation for developers and advanced users.

## Bridge API (ESP32_ATEM_Bridge_BLE_v3.ino)

### Configuration Constants

#### Network Configuration
```cpp
#define ATEM_IP "192.168.1.100"         // ATEM switcher IP address
#define USB_TETHER_TIMEOUT 30000        // USB connection timeout (ms)
#define NETWORK_CHECK_INTERVAL 30000    // Network connectivity check interval (ms)
```

#### BLE Configuration
```cpp
#define BLE_DEVICE_NAME "ATEM_Bridge_BLE"    // Bridge BLE name
#define MAX_TALLY_DEVICES 4                  // Maximum simultaneous connections
#define BLE_SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"
#define BLE_CHARACTERISTIC_UUID "87654321-4321-8765-cba9-987654321cba"
```

#### System Configuration
```cpp
#define MAX_CAMERAS 20                   // Maximum cameras supported
#define ATEM_RECONNECT_INTERVAL 10000    // ATEM reconnection interval (ms)
#define TALLY_CHECK_INTERVAL 100         // Tally state check interval (ms)
#define HEARTBEAT_INTERVAL 5000          // Heartbeat broadcast interval (ms)
#define STANDBY_AS_PREVIEW true          // Enable standby preview mode
```

### Data Structures

#### TallyMessage
Message structure for BLE communication:
```cpp
typedef struct {
    uint8_t cameraId;        // Camera number (1-20, 0=heartbeat)
    char state[12];          // "PREVIEW", "PROGRAM", "OFF", "STANDBY", "HEARTBEAT", "NO_ATEM"
    uint32_t timestamp;      // Message timestamp
    uint8_t bridgeId;        // Bridge identifier
    uint8_t bridgeStatus;    // Bridge status: 0=No ATEM, 1=ATEM Connected
    uint8_t checksum;        // Message integrity checksum
} __attribute__((packed)) TallyMessage;
```

#### TallyDevice
Device tracking structure:
```cpp
typedef struct {
    String deviceName;       // Unique device name
    uint8_t cameraId;        // Assigned camera ID
    unsigned long lastSeen;  // Last communication timestamp
    bool connected;          // BLE connection status
    bool registered;         // Device registration status
    BLECharacteristic* characteristic; // BLE communication handle
} TallyDevice;
```

### Core Functions

#### BLE Functions
- `bool initializeBLE()` - Initialize BLE server and advertising
- `void sendTallyToDevice(int deviceIndex, uint8_t cameraId, const char* state)` - Send tally data to specific device
- `const char* getCurrentTallyState(uint8_t cameraId)` - Get current tally state with standby logic
- `void broadcastTallyData(uint8_t cameraId, const char* state)` - Broadcast to all connected devices
- `void sendHeartbeatSignal()` - Send periodic heartbeat to all devices

#### ATEM Functions
- `bool connectToATEM()` - Connect to ATEM switcher using ATEMmin library
- `void checkATEMTallyStates()` - Monitor ATEM for tally state changes
- `void handleATEM()` - Main ATEM communication handler

#### Network Functions
- `bool initializeNetwork()` - Initialize USB tethering network connection
- `bool checkNetworkConnectivity()` - Verify network connectivity status

### Serial Commands

Available via Serial Monitor (115200 baud):

| Command | Description |
|---------|-------------|
| `STATUS` | Show complete system status |
| `NETWORK` | Show network connection details |
| `ATEM` | Show ATEM connection status and library info |
| `BLE` | Show BLE server status and connected devices |
| `DEVICES` | List all registered tally devices |
| `STANDBY` | Show standby preview mode status |
| `CAMx:STATE` | Manual tally test (e.g., `CAM1:PREVIEW`) |
| `RESET` | Restart ESP32 |
| `HELP` | Show command list |

### Tally State Logic

#### Standard States
- **PROGRAM**: Camera is live/on air (red LED)
- **PREVIEW**: Camera is selected in preview (green LED)
- **OFF**: Camera is not selected (LED off)

#### Standby Preview Mode
When `STANDBY_AS_PREVIEW` is enabled:
- Non-active cameras show as PREVIEW (green) when any camera is PROGRAM
- Provides visual indication that cameras are ready/standing by

## Tally Light API (ESP32_Tally_Light_BLE_v2.ino)

### Configuration Constants

#### Device Configuration
```cpp
#define CAMERA_ID 1                      // Camera number (1-20, unique per device)
#define DEVICE_NAME "Tally_CAM_1"       // Unique device name
```

#### LED Configuration
```cpp
#define RED_LED_PIN 25                   // GPIO pin for RED LED
#define GREEN_LED_PIN 26                 // GPIO pin for GREEN LED
#define BLUE_LED_PIN 27                  // GPIO pin for BLUE LED
#define LED_BRIGHTNESS 255               // Maximum LED brightness (0-255)
#define LED_DIM_BRIGHTNESS 64            // Dimmed brightness for status
```

#### System Configuration
```cpp
#define HEARTBEAT_TIMEOUT 15000          // Heartbeat timeout (ms)
#define RECONNECT_INTERVAL 5000          // Base reconnection interval (ms)
#define MAX_RECONNECT_INTERVAL 30000     // Maximum reconnection interval (ms)
```

### Connection States

```cpp
enum ConnectionState {
    DISCONNECTED,     // Not connected to bridge
    SCANNING,         // Scanning for bridge
    CONNECTING,       // Attempting to connect
    CONNECTED,        // Connected and registered
    ERROR_STATE       // Error condition
};
```

### LED Status Codes

| State | LED Color | Description |
|-------|-----------|-------------|
| **PROGRAM** | 🔴 Red Solid | Camera is live/on air |
| **PREVIEW/STANDBY** | 🟢 Green Solid | Camera ready/in preview |
| **Heartbeat** | 🔵 Blue Pulse | Connected with ATEM |
| **No ATEM** | 🟡 Yellow Pulse | Connected but bridge has no ATEM |
| **Searching** | 🟠 Orange Slow Blink | Searching for bridge |
| **Connecting** | 🟠 Orange Fast Blink | Connecting to bridge |
| **Connection Lost** | 🟣 Magenta Blink | No heartbeat received |
| **Error** | 🟣 Purple Blink | BLE error or invalid message |
| **Data Received** | ⚪ White Flash | Message received/processed |
| **OFF** | ⚫ Off | Camera not active |

### Core Functions

#### LED Functions
- `void setLED(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)` - Set RGB LED color
- `void clearLED()` - Turn off all LEDs
- `void flashLED(uint8_t red, uint8_t green, uint8_t blue, int duration)` - Flash LED briefly
- `void updateLEDStatus()` - Update LED based on current system state

#### BLE Functions
- `bool initializeBLE()` - Initialize BLE client
- `bool scanForBridge()` - Scan for bridge device
- `bool connectToBridge()` - Connect to bridge and register
- `void registerWithBridge()` - Send registration message to bridge
- `void handleConnection()` - Main connection management with auto-reconnect

#### Message Functions
- `uint8_t calculateChecksum(TallyMessage* msg)` - Calculate message checksum
- `bool verifyMessage(TallyMessage* msg)` - Verify message integrity
- `notifyCallback()` - Handle incoming tally messages from bridge

### Serial Commands

Available via Serial Monitor (115200 baud):

| Command | Description |
|---------|-------------|
| `STATUS` | Show device status and connection info |
| `RECONNECT` | Force reconnection to bridge |
| `TEST_LED` | Test RGB LED colors (red, green, blue sequence) |
| `RESET` | Restart ESP32 |
| `HELP` | Show command list |

### Registration Protocol

Tally lights register with bridge using this message format:
```
TALLY_REG:<camera_id>:<device_name>
```

Example: `TALLY_REG:1:Tally_CAM_1`

### Auto-Reconnection Logic

- **Exponential Backoff**: Reconnection interval doubles on each failure
- **Maximum Interval**: Caps at `MAX_RECONNECT_INTERVAL`
- **Reset on Success**: Interval resets to `RECONNECT_INTERVAL` on successful connection
- **Heartbeat Monitoring**: Disconnects if no heartbeat received within `HEARTBEAT_TIMEOUT`

## Performance Optimization

### Bridge Optimization
- **Tally Check Interval**: 100ms for responsive updates
- **Heartbeat Interval**: 5 seconds for connection monitoring
- **Memory Management**: Efficient message structures with packed attributes
- **Network Resilience**: Auto-reconnection with USB tethering monitoring

### Tally Light Optimization
- **Power Management**: Optimized LED brightness and update intervals
- **Connection Efficiency**: Smart reconnection with exponential backoff
- **Message Verification**: Checksum validation for data integrity
- **Status Feedback**: Visual indicators for all connection states

## Customization Examples

### Custom LED Patterns
```cpp
// Add custom tally state in updateLEDStatus()
else if (currentTallyState == "CUSTOM") {
    // Custom LED pattern
    setLED(255, 128, 0, LED_BRIGHTNESS); // Orange
}
```

### Additional Camera Support
```cpp
// Increase maximum cameras in bridge
#define MAX_CAMERAS 32  // Support up to 32 cameras

// Update device tracking arrays accordingly
```

### Custom BLE UUIDs
```cpp
// Use your own UUIDs for private networks
#define BLE_SERVICE_UUID "your-service-uuid-here"
#define BLE_CHARACTERISTIC_UUID "your-characteristic-uuid-here"
```

### Network Configuration
```cpp
// Custom ATEM reconnection timing
#define ATEM_RECONNECT_INTERVAL 5000   // Faster reconnection
#define TALLY_CHECK_INTERVAL 50        // Higher refresh rate
```

## Troubleshooting Reference

### Bridge Issues
- **No ATEM Connection**: Check `ATEM_IP`, USB tethering, network connectivity
- **BLE Devices Won't Connect**: Verify UUIDs match, check range, restart advertising
- **Memory Issues**: Reduce `MAX_TALLY_DEVICES` or `MAX_CAMERAS`

### Tally Light Issues
- **Won't Find Bridge**: Check bridge advertising, verify `BLE_SERVER_NAME`
- **Frequent Disconnections**: Reduce range, check interference, verify power supply
- **LED Not Working**: Check wiring, GPIO pins, resistor values

### Serial Debugging
Both devices provide extensive serial output at 115200 baud for debugging:
- Connection status and events
- Message transmission details
- Performance metrics and timing
- Error conditions and recovery attempts

## Version History

### v3.0 (Current)
- ATEMmin library integration
- Standby preview functionality
- Fixed ATEM indexing bug
- Enhanced error handling

### v2.0
- BLE multi-device support
- Improved LED status codes
- Auto-reconnection with backoff
- Message integrity verification

### v1.0
- Initial BLE implementation
- Basic tally functionality
- Single device support
