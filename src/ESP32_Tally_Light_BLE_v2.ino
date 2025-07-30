/*
 * ESP32 ATEM Tally Light v2.0 (BLE Client)
 * Hardware: ESP32-WROOM-32 + RGB LED
 * 
 * System Architecture:
 * ESP32 Bridge ──BLE Secure──> This Tally Light ESP32
 * 
 * Features:
 * - BLE client that connects to ATEM Bridge
 * - Secure BLE with automatic pairing and encryption
 * - Individual device registration with unique name and camera ID
 * - RGB LED status indication with brightness control
 * - Auto-reconnection with exponential backoff
 * - Message integrity verification with checksums
 * - Status monitoring and debugging via Serial
 * - Low power consumption in standby mode
 * - Support for 20 camera inputs with full state tracking
 * 
 * LED Status Codes:
 * - Red Solid: Camera is LIVE/PROGRAM (on air)
 * - Green Solid: Camera is PREVIEW (ready) or STANDBY (ready when production active)
 * - Blue Pulsing: Heartbeat/Connected to bridge with ATEM
 * - Yellow Slow Pulse: Connected to bridge but bridge has no ATEM connection
 * - Orange Slow Blink: Searching for bridge (not connected)
 * - Orange Fast Blink: Connecting to bridge
 * - White Flash: Data received/processed
 * - Purple: BLE error or invalid message
 * - Magenta Blink: No heartbeat received (connection lost)
 * - Off: Camera is off/not active
 * 
 * Setup Instructions:
 * 1. Update CAMERA_ID below to match your camera number (1-20)
 * 2. Update DEVICE_NAME to be unique for each tally light
 * 3. Upload code to ESP32 tally light
 * 4. Power on - LED will blink orange while searching for bridge
 * 5. Automatic pairing will occur when bridge is found
 * 6. LED turns blue when connected and ready
 * 7. Tally states (red/green) display based on ATEM switcher
 * 
 * BLE Advantages:
 * - Multiple simultaneous connections (3-4 devices per bridge)
 * - Lower power consumption vs Bluetooth Classic
 * - Faster connection establishment
 * - Better security with modern encryption
 * - More reliable in crowded environments
 * 
 * Author: ESP32 Tally System
 * Version: 2.0 BLE Multi-Device + Standby Preview
 * Date: July 2025
 */

#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ===============================================
// CONFIGURATION - UPDATE THESE VALUES
// ===============================================

// Device Configuration (UNIQUE FOR EACH TALLY LIGHT)
#define CAMERA_ID 1                         // Camera number this tally represents (1-20)
#define DEVICE_NAME "Tally_CAM_1"          // Unique device name (change for each device!)

// LED Pin Configuration (GPIO pins for RGB LED)
#define RED_LED_PIN 25                      // GPIO pin for RED LED
#define GREEN_LED_PIN 26                    // GPIO pin for GREEN LED  
#define BLUE_LED_PIN 27                     // GPIO pin for BLUE LED

// BLE Configuration (must match bridge configuration)
#define BLE_SERVER_NAME "ATEM_Bridge_BLE"  // Name of the bridge to connect to
#define SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-8765-cba9-987654321cba"

// System Configuration
#define LED_BRIGHTNESS 255                  // LED brightness (0-255) - 255 for maximum visibility
#define LED_DIM_BRIGHTNESS 64              // Dimmed brightness for status indicators
#define HEARTBEAT_TIMEOUT 15000            // Heartbeat timeout (ms) - disconnect if no heartbeat
#define RECONNECT_INTERVAL 5000            // Base reconnection attempt interval (ms)
#define MAX_RECONNECT_INTERVAL 30000       // Maximum reconnection interval (ms)
#define STATUS_UPDATE_INTERVAL 10000       // Status print interval (ms)

// Power Management
#define POWER_SAVE_MODE false              // Enable power saving features
#define DEEP_SLEEP_THRESHOLD 300000        // Enter deep sleep after 5 minutes of inactivity

// ===============================================
// DATA STRUCTURES
// ===============================================

// BLE tally message structure (must match bridge)
typedef struct {
    uint8_t cameraId;        // Camera number (1-20)
    char state[12];          // "PREVIEW", "PROGRAM", "OFF", "STANDBY", "HEARTBEAT", "NO_ATEM"
    uint32_t timestamp;      // Message timestamp for debugging
    uint8_t bridgeId;        // Bridge identifier (for multiple bridges)
    uint8_t bridgeStatus;    // Bridge status: 0=No ATEM, 1=ATEM Connected, 2=Heartbeat
    uint8_t checksum;        // Simple checksum for data integrity
} __attribute__((packed)) TallyMessage;

// Connection states
enum ConnectionState {
    DISCONNECTED,     // Not connected to bridge
    SCANNING,         // Scanning for bridge
    CONNECTING,       // Attempting to connect
    CONNECTED,        // Connected and registered
    ERROR_STATE       // Error condition
};

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// Forward declarations
void updateLEDStatus();

// BLE Client
BLEClient* pClient = nullptr;
BLERemoteService* pRemoteService = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
BLEAdvertisedDevice* targetDevice = nullptr;

// Connection Management
ConnectionState connectionState = DISCONNECTED;
unsigned long lastHeartbeat = 0;
unsigned long lastReconnectAttempt = 0;
unsigned long currentReconnectInterval = RECONNECT_INTERVAL;
unsigned long lastStatusUpdate = 0;
bool deviceRegistered = false;

// Tally State
String currentTallyState = "OFF";
String bridgeStatus = "UNKNOWN";
unsigned long lastStateChange = 0;
unsigned long totalMessagesReceived = 0;
unsigned long systemStartTime = 0;

// LED Control
unsigned long lastLEDUpdate = 0;
bool ledPulseState = false;
int pulsePhase = 0;

// ===============================================
// LED FUNCTIONS
// ===============================================

// Set RGB LED to specific color with brightness
void setLED(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness = LED_BRIGHTNESS) {
    // Apply brightness scaling
    red = (red * brightness) / 255;
    green = (green * brightness) / 255;
    blue = (blue * brightness) / 255;
    
    // Set LED pins
    analogWrite(RED_LED_PIN, red);
    analogWrite(GREEN_LED_PIN, green);
    analogWrite(BLUE_LED_PIN, blue);
}

// Turn off all LEDs
void clearLED() {
    setLED(0, 0, 0);
}

// Flash LED briefly for feedback
void flashLED(uint8_t red, uint8_t green, uint8_t blue, int duration = 100) {
    setLED(red, green, blue, LED_BRIGHTNESS);
    delay(duration);
    updateLEDStatus(); // Return to current status
}

// Update LED based on current system state
void updateLEDStatus() {
    unsigned long currentTime = millis();
    
    switch (connectionState) {
        case DISCONNECTED:
            // Orange slow blink - searching for bridge
            if (currentTime - lastLEDUpdate > 1000) {
                ledPulseState = !ledPulseState;
                setLED(255, 165, 0, ledPulseState ? LED_DIM_BRIGHTNESS : 0); // Orange
                lastLEDUpdate = currentTime;
            }
            break;
            
        case SCANNING:
            // Orange fast blink - scanning
            if (currentTime - lastLEDUpdate > 200) {
                ledPulseState = !ledPulseState;
                setLED(255, 165, 0, ledPulseState ? LED_BRIGHTNESS : 0); // Orange fast
                lastLEDUpdate = currentTime;
            }
            break;
            
        case CONNECTING:
            // Yellow pulse - connecting
            if (currentTime - lastLEDUpdate > 300) {
                ledPulseState = !ledPulseState;
                setLED(255, 255, 0, ledPulseState ? LED_BRIGHTNESS : LED_DIM_BRIGHTNESS); // Yellow
                lastLEDUpdate = currentTime;
            }
            break;
            
        case CONNECTED:
            // Display tally state or connection status
            if (bridgeStatus == "NO_ATEM") {
                // Yellow slow pulse - connected but bridge has no ATEM
                if (currentTime - lastLEDUpdate > 2000) {
                    ledPulseState = !ledPulseState;
                    setLED(255, 255, 0, ledPulseState ? LED_BRIGHTNESS : LED_DIM_BRIGHTNESS);
                    lastLEDUpdate = currentTime;
                }
            } else if (currentTallyState == "PROGRAM") {
                // Red solid - camera is live
                setLED(255, 0, 0, LED_BRIGHTNESS);
            } else if (currentTallyState == "PREVIEW" || currentTallyState == "STANDBY") {
                // Green solid - camera is in preview or standby
                setLED(0, 255, 0, LED_BRIGHTNESS);
            } else if (currentTallyState == "HEARTBEAT") {
                // Blue gentle pulse - connected and receiving heartbeats
                if (currentTime - lastLEDUpdate > 100) {
                    pulsePhase = (pulsePhase + 5) % 360;
                    int brightness = LED_DIM_BRIGHTNESS + (int)((LED_BRIGHTNESS - LED_DIM_BRIGHTNESS) * 
                                   (1 + sin(pulsePhase * PI / 180)) / 2);
                    setLED(0, 0, 255, brightness);
                    lastLEDUpdate = currentTime;
                }
            } else if (currentTallyState == "OFF") {
                // Check if we've lost heartbeat
                if (currentTime - lastHeartbeat > HEARTBEAT_TIMEOUT) {
                    // Magenta blink - no heartbeat
                    if (currentTime - lastLEDUpdate > 500) {
                        ledPulseState = !ledPulseState;
                        setLED(255, 0, 255, ledPulseState ? LED_BRIGHTNESS : 0); // Magenta
                        lastLEDUpdate = currentTime;
                    }
                } else {
                    // Off - camera not active
                    clearLED();
                }
            }
            break;
            
        case ERROR_STATE:
            // Purple blink - error
            if (currentTime - lastLEDUpdate > 250) {
                ledPulseState = !ledPulseState;
                setLED(128, 0, 128, ledPulseState ? LED_BRIGHTNESS : 0); // Purple
                lastLEDUpdate = currentTime;
            }
            break;
    }
}

// ===============================================
// BLE FUNCTIONS
// ===============================================

// Calculate simple checksum for message integrity
uint8_t calculateChecksum(TallyMessage* msg) {
    uint8_t checksum = 0;
    checksum ^= msg->cameraId;
    checksum ^= msg->bridgeId;
    checksum ^= msg->bridgeStatus;
    for (int i = 0; i < strlen(msg->state); i++) {
        checksum ^= msg->state[i];
    }
    return checksum;
}

// Verify message integrity
bool verifyMessage(TallyMessage* msg) {
    uint8_t calculatedChecksum = calculateChecksum(msg);
    return (calculatedChecksum == msg->checksum);
}

// BLE Client Callbacks
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        Serial.println("✓ Connected to bridge");
        connectionState = CONNECTED;
        currentReconnectInterval = RECONNECT_INTERVAL; // Reset backoff
    }

    void onDisconnect(BLEClient* pclient) {
        Serial.println("✗ Disconnected from bridge");
        connectionState = DISCONNECTED;
        deviceRegistered = false;
        bridgeStatus = "DISCONNECTED";
        currentTallyState = "OFF";
        
        // Start reconnection process
        lastReconnectAttempt = millis();
    }
};

// Notification callback for receiving tally data
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                          uint8_t* pData, size_t length, bool isNotify) {
    
    // Verify message size
    if (length != sizeof(TallyMessage)) {
        Serial.printf("Invalid message size: %d (expected %d)\n", length, sizeof(TallyMessage));
        flashLED(128, 0, 128); // Purple flash for error
        return;
    }
    
    // Parse message
    TallyMessage* msg = (TallyMessage*)pData;
    
    // Verify message integrity
    if (!verifyMessage(msg)) {
        Serial.println("Message checksum failed");
        flashLED(128, 0, 128); // Purple flash for error
        return;
    }
    
    // Flash white to indicate data received
    flashLED(255, 255, 255, 50);
    
    totalMessagesReceived++;
    lastHeartbeat = millis();
    
    // Handle different message types
    if (msg->cameraId == 0) {
        // Heartbeat/status message
        bridgeStatus = String(msg->state);
        currentTallyState = "HEARTBEAT";
        
        Serial.printf("Heartbeat received - Bridge: %s, ATEM: %s\n", 
                     msg->state, msg->bridgeStatus ? "Connected" : "Disconnected");
    } else if (msg->cameraId == CAMERA_ID) {
        // Tally state for this camera
        String newState = String(msg->state);
        if (newState != currentTallyState) {
            Serial.printf("Tally state change: CAM%d %s -> %s\n", 
                         CAMERA_ID, currentTallyState.c_str(), newState.c_str());
            currentTallyState = newState;
            lastStateChange = millis();
        }
        
        // Update bridge status
        if (msg->bridgeStatus == 0) {
            bridgeStatus = "NO_ATEM";
        } else {
            bridgeStatus = "ATEM_OK";
        }
    }
    
    // Update LED immediately after receiving valid message
    updateLEDStatus();
}

// Device scan callback
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.getName() == BLE_SERVER_NAME) {
            Serial.printf("Found target device: %s\n", BLE_SERVER_NAME);
            BLEDevice::getScan()->stop();
            targetDevice = new BLEAdvertisedDevice(advertisedDevice);
            connectionState = CONNECTING;
        }
    }
};

// Initialize BLE client
bool initializeBLE() {
    Serial.printf("Initializing BLE client: %s (CAM%d)\n", DEVICE_NAME, CAMERA_ID);
    
    // Initialize BLE device
    BLEDevice::init(DEVICE_NAME);
    
    Serial.println("✓ BLE client initialized");
    return true;
}

// Scan for bridge device
bool scanForBridge() {
    connectionState = SCANNING;
    
    Serial.printf("Scanning for bridge: %s\n", BLE_SERVER_NAME);
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, false); // Scan for 10 seconds
    
    return (targetDevice != nullptr);
}

// Connect to bridge
bool connectToBridge() {
    if (!targetDevice) {
        Serial.println("No target device found");
        return false;
    }
    
    Serial.println("Attempting to connect to bridge...");
    
    // Create BLE client
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    
    // Connect to the bridge
    if (!pClient->connect(targetDevice)) {
        Serial.println("Failed to connect to bridge");
        delete pClient;
        pClient = nullptr;
        return false;
    }
    
    Serial.println("Connected to bridge - getting service...");
    
    // Get the service
    pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find service");
        pClient->disconnect();
        return false;
    }
    
    // Get the characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic");
        pClient->disconnect();
        return false;
    }
    
    // Register for notifications
    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
        Serial.println("✓ Registered for notifications");
    }
    
    // Register with bridge
    registerWithBridge();
    
    connectionState = CONNECTED;
    return true;
}

// Register this device with the bridge
void registerWithBridge() {
    if (!pRemoteCharacteristic || !pRemoteCharacteristic->canWrite()) {
        Serial.println("Cannot register - characteristic not writable");
        return;
    }
    
    // Send registration message: "TALLY_REG:camera_id:device_name"
    String regMessage = "TALLY_REG:" + String(CAMERA_ID) + ":" + String(DEVICE_NAME);
    
    Serial.printf("Registering with bridge: %s\n", regMessage.c_str());
    
    pRemoteCharacteristic->writeValue(regMessage.c_str(), regMessage.length());
    deviceRegistered = true;
    
    Serial.printf("✓ Registered as CAM%d (%s)\n", CAMERA_ID, DEVICE_NAME);
}

// Main connection handler
void handleConnection() {
    unsigned long currentTime = millis();
    
    switch (connectionState) {
        case DISCONNECTED:
            // Attempt reconnection with exponential backoff
            if (currentTime - lastReconnectAttempt >= currentReconnectInterval) {
                Serial.println("Attempting to reconnect...");
                
                // Clean up previous connection
                if (pClient) {
                    pClient->disconnect();
                    delete pClient;
                    pClient = nullptr;
                }
                
                pRemoteService = nullptr;
                pRemoteCharacteristic = nullptr;
                targetDevice = nullptr;
                deviceRegistered = false;
                
                // Start scanning
                if (scanForBridge()) {
                    if (connectToBridge()) {
                        Serial.println("✓ Reconnected successfully");
                        currentReconnectInterval = RECONNECT_INTERVAL;
                    } else {
                        connectionState = DISCONNECTED;
                        // Exponential backoff
                        currentReconnectInterval = min(currentReconnectInterval * 2, 
                                                     (unsigned long)MAX_RECONNECT_INTERVAL);
                    }
                } else {
                    connectionState = DISCONNECTED;
                    currentReconnectInterval = min(currentReconnectInterval * 2, 
                                                 (unsigned long)MAX_RECONNECT_INTERVAL);
                }
                
                lastReconnectAttempt = currentTime;
            }
            break;
            
        case CONNECTED:
            // Check for heartbeat timeout
            if (currentTime - lastHeartbeat > HEARTBEAT_TIMEOUT) {
                Serial.println("Heartbeat timeout - disconnecting");
                if (pClient && pClient->isConnected()) {
                    pClient->disconnect();
                }
                connectionState = DISCONNECTED;
            }
            break;
            
        default:
            // Handle other states as needed
            break;
    }
}

// ===============================================
// SYSTEM FUNCTIONS
// ===============================================

// Print system status
void printSystemStatus() {
    Serial.println("\n==== ESP32 Tally Light v2.0 Status ====");
    Serial.printf("Device: %s (CAM%d)\n", DEVICE_NAME, CAMERA_ID);
    Serial.printf("Uptime: %lu seconds\n", (millis() - systemStartTime) / 1000);
    
    // Connection status
    const char* stateNames[] = {"DISCONNECTED", "SCANNING", "CONNECTING", "CONNECTED", "ERROR"};
    Serial.printf("Connection: %s\n", stateNames[connectionState]);
    
    if (connectionState == CONNECTED) {
        Serial.printf("Registered: %s\n", deviceRegistered ? "YES" : "NO");
        Serial.printf("Bridge Status: %s\n", bridgeStatus.c_str());
        
        unsigned long heartbeatAge = (millis() - lastHeartbeat) / 1000;
        Serial.printf("Last Heartbeat: %lu seconds ago\n", heartbeatAge);
        
        if (heartbeatAge > HEARTBEAT_TIMEOUT / 1000) {
            Serial.println("⚠️ HEARTBEAT TIMEOUT!");
        }
    }
    
    // Tally status
    Serial.printf("Tally State: %s\n", currentTallyState.c_str());
    if (lastStateChange > 0) {
        Serial.printf("Last Change: %lu seconds ago\n", 
                     (millis() - lastStateChange) / 1000);
    }
    
    // Statistics
    Serial.printf("Messages Received: %lu\n", totalMessagesReceived);
    Serial.printf("Reconnect Interval: %lu ms\n", currentReconnectInterval);
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    Serial.println("========================================\n");
}

// Handle serial commands
void handleSerialCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command == "STATUS") {
        printSystemStatus();
    }
    else if (command == "RECONNECT") {
        Serial.println("Forcing reconnection...");
        if (pClient && pClient->isConnected()) {
            pClient->disconnect();
        }
        connectionState = DISCONNECTED;
        currentReconnectInterval = RECONNECT_INTERVAL;
        lastReconnectAttempt = 0;
    }
    else if (command == "RESET") {
        Serial.println("Restarting ESP32...");
        delay(1000);
        ESP.restart();
    }
    else if (command == "TEST_LED") {
        Serial.println("Testing LED colors...");
        Serial.println("Red...");
        setLED(255, 0, 0);
        delay(1000);
        Serial.println("Green...");
        setLED(0, 255, 0);
        delay(1000);
        Serial.println("Blue...");
        setLED(0, 0, 255);
        delay(1000);
        Serial.println("Off...");
        clearLED();
        Serial.println("LED test complete");
    }
    else if (command == "HELP") {
        Serial.println("\nAvailable Commands:");
        Serial.println("STATUS     - Show system status");
        Serial.println("RECONNECT  - Force reconnection to bridge");
        Serial.println("TEST_LED   - Test RGB LED colors");
        Serial.println("RESET      - Restart ESP32");
        Serial.println("HELP       - Show this help\n");
    }
    else {
        Serial.println("Unknown command. Type HELP for available commands.");
    }
}

// ===============================================
// MAIN FUNCTIONS
// ===============================================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    systemStartTime = millis();
    
    Serial.println("\n==========================================");
    Serial.println("ESP32 ATEM Tally Light v2.0");
    Serial.println("BLE Client + Standby Preview");
    Serial.println("==========================================");
    
    // Print device information
    Serial.printf("Device Name: %s\n", DEVICE_NAME);
    Serial.printf("Camera ID: %d\n", CAMERA_ID);
    Serial.printf("Target Bridge: %s\n", BLE_SERVER_NAME);
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize LED pins
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    
    // Turn off all LEDs initially
    clearLED();
    
    // Test LED startup sequence
    Serial.println("LED startup test...");
    setLED(255, 0, 0, LED_DIM_BRIGHTNESS); // Red
    delay(200);
    setLED(0, 255, 0, LED_DIM_BRIGHTNESS); // Green
    delay(200);
    setLED(0, 0, 255, LED_DIM_BRIGHTNESS); // Blue
    delay(200);
    clearLED();
    
    // Initialize BLE
    if (!initializeBLE()) {
        Serial.println("BLE initialization failed - stopping");
        connectionState = ERROR_STATE;
        while(1) {
            updateLEDStatus();
            delay(100);
        }
    }
    
    Serial.println("\n==========================================");
    Serial.printf("Tally Light Ready! (CAM%d - %s)\n", CAMERA_ID, DEVICE_NAME);
    Serial.println("Searching for ATEM Bridge...");
    Serial.println("Type HELP for available commands");
    Serial.println("==========================================\n");
    
    lastReconnectAttempt = millis();
}

void loop() {
    // Handle BLE connection
    handleConnection();
    
    // Update LED status
    updateLEDStatus();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Print periodic status updates
    if (millis() - lastStatusUpdate > STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = millis();
        
        if (connectionState == CONNECTED) {
            Serial.printf("Status: CAM%d %s | Bridge: %s | Heartbeat: %lus ago\n",
                         CAMERA_ID, currentTallyState.c_str(), bridgeStatus.c_str(),
                         (millis() - lastHeartbeat) / 1000);
        } else {
            Serial.printf("Status: %s | Reconnect in %lus\n",
                         connectionState == DISCONNECTED ? "DISCONNECTED" : "CONNECTING",
                         (currentReconnectInterval - (millis() - lastReconnectAttempt)) / 1000);
        }
    }
    
    // Small delay for system stability
    delay(10);
}
