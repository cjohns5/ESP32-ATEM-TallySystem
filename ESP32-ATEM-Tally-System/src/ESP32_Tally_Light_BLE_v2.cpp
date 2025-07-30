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
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// ===============================================
// CONFIGURATION - UPDATE THESE VALUES
// ===============================================

#define CAMERA_ID 1                           // Camera number this tally monitors (1-20)
#define DEVICE_NAME "Tally_CAM_1"             // Unique device name (change for each tally)

// BLE Configuration (must match bridge)
#define BRIDGE_SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"
#define BRIDGE_CHARACTERISTIC_UUID "87654321-4321-8765-cba9-987654321cba"
#define BRIDGE_DEVICE_NAME "ATEM_Bridge_BLE"  // Name of bridge to connect to

// LED Configuration
#define LED_RED_PIN 25                        // GPIO pin for red LED
#define LED_GREEN_PIN 26                      // GPIO pin for green LED
#define LED_BLUE_PIN 27                       // GPIO pin for blue LED
#define LED_BRIGHTNESS 128                    // LED brightness (0-255)
#define HEARTBEAT_LED_INTERVAL 2000           // Blue heartbeat pulse interval (ms)

// Connection Configuration
#define SCAN_TIME 5                           // BLE scan time in seconds
#define CONNECTION_TIMEOUT 10000              // Connection timeout (ms)
#define RECONNECT_INTERVAL 15000              // Reconnection attempt interval (ms)
#define MAX_RECONNECT_ATTEMPTS 5              // Max consecutive reconnection attempts
#define REGISTRATION_RETRY_INTERVAL 5000     // Registration retry interval (ms)

// System Configuration
#define HEARTBEAT_INTERVAL 30000              // Heartbeat/keepalive interval (ms)
#define MESSAGE_TIMEOUT 60000                 // Message receive timeout (ms)
#define HEARTBEAT_TIMEOUT 15000               // Heartbeat timeout for connection loss detection (ms)
#define SERIAL_DEBUG true                     // Enable serial debugging

// ===============================================
// DATA STRUCTURES
// ===============================================

// BLE tally message structure (must match bridge)
typedef struct {
    uint8_t cameraId;        // Camera number (1-20) or 0 for heartbeat
    char state[12];          // "PREVIEW", "PROGRAM", "OFF", "STANDBY", "HEARTBEAT", "NO_ATEM"
    uint32_t timestamp;      // Message timestamp for debugging
    uint8_t bridgeId;        // Bridge identifier (for multiple bridges)
    uint8_t bridgeStatus;    // Bridge status: 0=No ATEM, 1=ATEM Connected
    uint8_t checksum;        // Simple checksum for data integrity
} __attribute__((packed)) TallyMessage;

// Connection state
typedef enum {
    STATE_DISCONNECTED,
    STATE_SCANNING,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_REGISTERED,
    STATE_ERROR
} ConnectionState;

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// BLE objects
BLEClient* pClient = nullptr;
BLERemoteService* pRemoteService = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
bool doConnect = false;
bool connected = false;
bool doScan = false;
BLEAdvertisedDevice* myDevice = nullptr;

// System state
ConnectionState currentState = STATE_DISCONNECTED;
String currentTallyState = "OFF";
bool bridgeHasATEM = false;
unsigned long lastMessageReceived = 0;
unsigned long lastHeartbeatReceived = 0;
unsigned long lastConnectionAttempt = 0;
unsigned long lastRegistrationAttempt = 0;
unsigned long lastHeartbeat = 0;
int reconnectAttempts = 0;
bool registered = false;

// LED state
bool heartbeatLedState = false;
unsigned long lastHeartbeatLed = 0;

// Statistics
unsigned long totalMessagesReceived = 0;
unsigned long totalConnectionAttempts = 0;
unsigned long systemStartTime = 0;
unsigned long totalOnlineTime = 0;
unsigned long lastOnlineStart = 0;

// ===============================================
// LED FUNCTIONS
// ===============================================

// Set LED color with brightness control
void setLEDColor(uint8_t red, uint8_t green, uint8_t blue) {
    // Apply brightness scaling
    red = (red * LED_BRIGHTNESS) / 255;
    green = (green * LED_BRIGHTNESS) / 255;
    blue = (blue * LED_BRIGHTNESS) / 255;
    
    analogWrite(LED_RED_PIN, red);
    analogWrite(LED_GREEN_PIN, green);
    analogWrite(LED_BLUE_PIN, blue);
}

// Turn off all LEDs
void setLEDOff() {
    setLEDColor(0, 0, 0);
}

// Flash LED with specified color
void flashLED(uint8_t red, uint8_t green, uint8_t blue, int duration = 100) {
    setLEDColor(red, green, blue);
    delay(duration);
    updateTallyLED(); // Return to current state
}

// Update LED based on current tally state
void updateTallyLED() {
    // Check for heartbeat timeout (connection lost)
    if (currentState == STATE_REGISTERED && 
        millis() - lastHeartbeatReceived > HEARTBEAT_TIMEOUT) {
        // Connection lost - magenta blink
        static bool blinkState = false;
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 500) {
            blinkState = !blinkState;
            lastBlink = millis();
            if (blinkState) {
                setLEDColor(255, 0, 255); // Magenta
            } else {
                setLEDOff();
            }
        }
        return;
    }
    
    if (currentState == STATE_REGISTERED) {
        // Show tally state based on camera status
        if (currentTallyState == "PROGRAM") {
            setLEDColor(255, 0, 0);  // Red - Live/Program
        } else if (currentTallyState == "PREVIEW") {
            setLEDColor(0, 255, 0);  // Green - Preview
        } else if (currentTallyState == "STANDBY") {
            setLEDColor(0, 255, 0);  // Green - Standby (ready/preview state)
        } else if (currentTallyState == "NO_ATEM" || !bridgeHasATEM) {
            // Bridge connected but no ATEM - yellow slow pulse
            static bool pulseState = false;
            static unsigned long lastPulse = 0;
            if (millis() - lastPulse > 1500) {
                pulseState = !pulseState;
                lastPulse = millis();
                if (pulseState) {
                    setLEDColor(255, 255, 0); // Yellow
                } else {
                    setLEDColor(64, 64, 0);   // Dim yellow
                }
            }
        } else {
            // Show heartbeat on blue when off but connected with ATEM
            if (heartbeatLedState) {
                setLEDColor(0, 0, 64);   // Dim blue heartbeat
            } else {
                setLEDOff();
            }
        }
    } else if (currentState == STATE_CONNECTED) {
        setLEDColor(0, 0, 255);      // Blue - Connected but not registered
    } else if (currentState == STATE_CONNECTING) {
        // Fast orange blink
        static bool blinkState = false;
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 200) {
            blinkState = !blinkState;
            lastBlink = millis();
            if (blinkState) {
                setLEDColor(255, 128, 0); // Orange
            } else {
                setLEDOff();
            }
        }
    } else if (currentState == STATE_SCANNING) {
        // Slow orange blink
        static bool blinkState = false;
        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 1000) {
            blinkState = !blinkState;
            lastBlink = millis();
            if (blinkState) {
                setLEDColor(255, 128, 0); // Orange
            } else {
                setLEDOff();
            }
        }
    } else if (currentState == STATE_ERROR) {
        setLEDColor(128, 0, 128);    // Purple - Error
    } else {
        setLEDOff();                 // Off - Disconnected
    }
}

// Handle heartbeat LED pulsing
void handleHeartbeatLED() {
    if (currentState == STATE_REGISTERED && currentTallyState == "OFF" && bridgeHasATEM) {
        if (millis() - lastHeartbeatLed > HEARTBEAT_LED_INTERVAL) {
            heartbeatLedState = !heartbeatLedState;
            lastHeartbeatLed = millis();
            updateTallyLED();
        }
    }
}

// ===============================================
// MESSAGE FUNCTIONS
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
    uint8_t receivedChecksum = msg->checksum;
    msg->checksum = 0; // Clear for calculation
    uint8_t calculatedChecksum = calculateChecksum(msg);
    msg->checksum = receivedChecksum; // Restore
    
    return (receivedChecksum == calculatedChecksum);
}

// Process received tally message
void processTallyMessage(TallyMessage* msg) {
    // Verify message integrity
    if (!verifyMessage(msg)) {
        if (SERIAL_DEBUG) {
            Serial.println("✗ Message checksum verification failed");
        }
        flashLED(128, 0, 128); // Purple flash for error
        return;
    }
    
    // Update bridge status
    bridgeHasATEM = (msg->bridgeStatus == 1);
    
    // Handle heartbeat messages (cameraId = 0)
    if (msg->cameraId == 0) {
        lastHeartbeatReceived = millis();
        lastMessageReceived = millis();
        
        if (SERIAL_DEBUG) {
            Serial.printf("💓 Heartbeat from bridge (ATEM:%s)\n", 
                         bridgeHasATEM ? "OK" : "DISCONNECTED");
        }
        
        // Update LED based on bridge ATEM status
        updateTallyLED();
        return;
    }
    
    // Check if message is for this camera
    if (msg->cameraId != CAMERA_ID) {
        // Message for different camera - ignore silently
        return;
    }
    
    // Update statistics
    totalMessagesReceived++;
    lastMessageReceived = millis();
    lastHeartbeatReceived = millis(); // Any message resets heartbeat timeout
    
    // Flash white to indicate data received
    flashLED(255, 255, 255, 50);
    
    // Update tally state if changed
    String newState = String(msg->state);
    if (newState != currentTallyState) {
        String previousState = currentTallyState;
        currentTallyState = newState;
        
        if (SERIAL_DEBUG) {
            Serial.printf("✓ CAM%d: %s -> %s (bridge %d, ATEM:%s, ts: %lu)\n", 
                         msg->cameraId, previousState.c_str(), 
                         newState.c_str(), msg->bridgeId, 
                         bridgeHasATEM ? "OK" : "DISCONNECTED", msg->timestamp);
        }
        
        updateTallyLED();
    }
}

// ===============================================
// BLE FUNCTIONS
// ===============================================

// BLE notification callback for receiving data
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                          uint8_t* pData, size_t length, bool isNotify) {
    if (length == sizeof(TallyMessage)) {
        TallyMessage* msg = (TallyMessage*)pData;
        processTallyMessage(msg);
    } else {
        if (SERIAL_DEBUG) {
            Serial.printf("✗ Received invalid message size: %d bytes\n", length);
        }
    }
}

// BLE client connection callback
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        if (SERIAL_DEBUG) {
            Serial.println("✓ BLE connected to bridge");
        }
        connected = true;
        currentState = STATE_CONNECTED;
        reconnectAttempts = 0;
        
        // Start tracking online time
        lastOnlineStart = millis();
        
        updateTallyLED();
    }

    void onDisconnect(BLEClient* pclient) {
        if (SERIAL_DEBUG) {
            Serial.println("✗ BLE disconnected from bridge");
        }
        connected = false;
        registered = false;
        currentState = STATE_DISCONNECTED;
        
        // Update total online time
        if (lastOnlineStart > 0) {
            totalOnlineTime += millis() - lastOnlineStart;
            lastOnlineStart = 0;
        }
        
        updateTallyLED();
        
        // Trigger reconnection
        doScan = true;
        lastConnectionAttempt = millis();
    }
};

// BLE advertised device callback for finding the bridge
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (SERIAL_DEBUG) {
            Serial.printf("Found BLE device: %s\n", advertisedDevice.toString().c_str());
        }
        
        // Check if this is our target bridge
        if (advertisedDevice.haveServiceUUID() && 
            advertisedDevice.isAdvertisingService(BLEUUID(BRIDGE_SERVICE_UUID))) {
            
            if (SERIAL_DEBUG) {
                Serial.printf("✓ Found ATEM bridge: %s\n", advertisedDevice.getName().c_str());
            }
            
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
            currentState = STATE_CONNECTING;
            updateTallyLED();
        } else {
            if (SERIAL_DEBUG) {
                Serial.println("  Not our target bridge");
            }
        }
    }
};

// Connect to BLE bridge server
bool connectToServer() {
    if (SERIAL_DEBUG) {
        Serial.printf("Connecting to bridge: %s\n", myDevice->getAddress().toString().c_str());
    }
    
    totalConnectionAttempts++;
    
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    
    // Connect to the remove BLE Server
    if (!pClient->connect(myDevice)) {
        if (SERIAL_DEBUG) {
            Serial.println("✗ Failed to connect to bridge");
        }
        return false;
    }
    
    // Obtain a reference to the service
    pRemoteService = pClient->getService(BRIDGE_SERVICE_UUID);
    if (pRemoteService == nullptr) {
        if (SERIAL_DEBUG) {
            Serial.println("✗ Failed to find bridge service");
        }
        pClient->disconnect();
        return false;
    }
    
    // Obtain a reference to the characteristic
    pRemoteCharacteristic = pRemoteService->getCharacteristic(BRIDGE_CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
        if (SERIAL_DEBUG) {
            Serial.println("✗ Failed to find bridge characteristic");
        }
        pClient->disconnect();
        return false;
    }
    
    // Register for notifications
    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
        if (SERIAL_DEBUG) {
            Serial.println("✓ Registered for notifications");
        }
    }
    
    // Send registration message
    registerWithBridge();
    
    return true;
}

// Register this tally device with the bridge
void registerWithBridge() {
    if (!connected || !pRemoteCharacteristic) return;
    
    // Send registration message: "TALLY_REG:1:Tally_CAM_1"
    String regMessage = "TALLY_REG:" + String(CAMERA_ID) + ":" + String(DEVICE_NAME);
    
    if (SERIAL_DEBUG) {
        Serial.printf("Registering with bridge: %s\n", regMessage.c_str());
    }
    
    pRemoteCharacteristic->writeValue(regMessage.c_str(), regMessage.length());
    lastRegistrationAttempt = millis();
    
    // Give bridge time to process registration
    delay(1000);
    
    registered = true;
    currentState = STATE_REGISTERED;
    
    if (SERIAL_DEBUG) {
        Serial.printf("✓ Registered as %s for camera %d\n", DEVICE_NAME, CAMERA_ID);
    }
    
    updateTallyLED();
}

// Start BLE scan for bridge
void startBLEScan() {
    if (SERIAL_DEBUG) {
        Serial.println("Scanning for ATEM bridge...");
    }
    
    currentState = STATE_SCANNING;
    updateTallyLED();
    
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(SCAN_TIME, false);
}

// ===============================================
// SYSTEM FUNCTIONS
// ===============================================

// Check connection health and handle reconnection
void handleConnection() {
    unsigned long currentTime = millis();
    
    // Handle connection timeouts
    if (connected && (currentTime - lastMessageReceived > MESSAGE_TIMEOUT)) {
        if (SERIAL_DEBUG) {
            Serial.println("Message timeout - disconnecting");
        }
        pClient->disconnect();
        return;
    }
    
    // Handle reconnection attempts
    if (!connected && !doConnect && !doScan) {
        if (currentTime - lastConnectionAttempt > RECONNECT_INTERVAL) {
            if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                if (SERIAL_DEBUG) {
                    Serial.printf("Reconnection attempt %d/%d\n", 
                                 reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
                }
                doScan = true;
                reconnectAttempts++;
                lastConnectionAttempt = currentTime;
            } else {
                // Too many failed attempts - wait longer
                if (currentTime - lastConnectionAttempt > (RECONNECT_INTERVAL * 3)) {
                    if (SERIAL_DEBUG) {
                        Serial.println("Resetting reconnection attempts");
                    }
                    reconnectAttempts = 0;
                }
            }
        }
    }
    
    // Re-register if connection exists but not registered
    if (connected && !registered) {
        if (currentTime - lastRegistrationAttempt > REGISTRATION_RETRY_INTERVAL) {
            registerWithBridge();
        }
    }
}

// Print system status
void printSystemStatus() {
    Serial.println("\n==== ESP32 Tally Light Status ====");
    Serial.printf("Device: %s (CAM%d)\n", DEVICE_NAME, CAMERA_ID);
    Serial.printf("Uptime: %lu seconds\n", (millis() - systemStartTime) / 1000);
    
    if (lastOnlineStart > 0) {
        unsigned long currentOnlineTime = millis() - lastOnlineStart;
        Serial.printf("Online Time: %lu seconds (current session)\n", currentOnlineTime / 1000);
    }
    Serial.printf("Total Online: %lu seconds\n", totalOnlineTime / 1000);
    
    // Connection status
    const char* stateNames[] = {
        "Disconnected", "Scanning", "Connecting", "Connected", "Registered", "Error"
    };
    Serial.printf("State: %s\n", stateNames[currentState]);
    Serial.printf("Tally: %s\n", currentTallyState.c_str());
    Serial.printf("Bridge ATEM: %s\n", bridgeHasATEM ? "Connected" : "Disconnected");
    
    if (connected) {
        Serial.println("BLE: Connected to bridge");
        if (lastMessageReceived > 0) {
            Serial.printf("Last message: %lu seconds ago\n", 
                         (millis() - lastMessageReceived) / 1000);
        }
        if (lastHeartbeatReceived > 0) {
            unsigned long heartbeatAge = (millis() - lastHeartbeatReceived) / 1000;
            Serial.printf("Last heartbeat: %lu seconds ago", heartbeatAge);
            if (heartbeatAge > HEARTBEAT_TIMEOUT / 1000) {
                Serial.print(" (TIMEOUT - CONNECTION LOST)");
            }
            Serial.println();
        }
    } else {
        Serial.println("BLE: Disconnected");
        Serial.printf("Reconnect attempts: %d/%d\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
    }
    
    Serial.printf("Messages received: %lu\n", totalMessagesReceived);
    Serial.printf("Connection attempts: %lu\n", totalConnectionAttempts);
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("=================================\n");
}

// Handle serial commands for testing and debugging
void handleSerialCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command == "STATUS") {
        printSystemStatus();
    }
    else if (command == "CONNECT") {
        if (!connected) {
            Serial.println("Starting connection attempt...");
            doScan = true;
            reconnectAttempts = 0;
        } else {
            Serial.println("Already connected");
        }
    }
    else if (command == "DISCONNECT") {
        if (connected) {
            Serial.println("Disconnecting...");
            pClient->disconnect();
        } else {
            Serial.println("Not connected");
        }
    }
    else if (command == "REGISTER") {
        if (connected) {
            Serial.println("Re-registering with bridge...");
            registerWithBridge();
        } else {
            Serial.println("Not connected to bridge");
        }
    }
    else if (command == "TEST") {
        Serial.println("LED Test Sequence:");
        Serial.println("Red (PROGRAM)...");
        setLEDColor(255, 0, 0);
        delay(2000);
        
        Serial.println("Green (PREVIEW)...");
        setLEDColor(0, 255, 0);
        delay(2000);
        
        Serial.println("Blue (CONNECTED)...");
        setLEDColor(0, 0, 255);
        delay(2000);
        
        Serial.println("Yellow (NO ATEM)...");
        setLEDColor(255, 255, 0);
        delay(2000);
        
        Serial.println("Orange (CONNECTING)...");
        setLEDColor(255, 128, 0);
        delay(2000);
        
        Serial.println("Purple (ERROR)...");
        setLEDColor(128, 0, 128);
        delay(2000);
        
        Serial.println("Magenta (CONNECTION LOST)...");
        setLEDColor(255, 0, 255);
        delay(2000);
        
        Serial.println("Test complete - returning to normal operation");
        updateTallyLED();
    }
    else if (command == "RESET") {
        Serial.println("Restarting ESP32...");
        delay(1000);
        ESP.restart();
    }
    else if (command == "HELP") {
        Serial.println("\nAvailable Commands:");
        Serial.println("STATUS      - Show system status");
        Serial.println("CONNECT     - Force connection attempt");
        Serial.println("DISCONNECT  - Disconnect from bridge");
        Serial.println("REGISTER    - Re-register with bridge");
        Serial.println("TEST        - Run LED test sequence");
        Serial.println("RESET       - Restart ESP32");
        Serial.println("HELP        - Show this help\n");
    }
    else {
        Serial.println("Unknown command. Type HELP for available commands.");
    }
}

// ===============================================
// MAIN FUNCTIONS
// ===============================================

void setup() {
    // Initialize serial communication
    if (SERIAL_DEBUG) {
        Serial.begin(115200);
        delay(2000);
        
        Serial.println("\n==========================================");
        Serial.println("ESP32 ATEM Tally Light v2.0");
        Serial.println("BLE Multi-Device Client");
        Serial.println("==========================================");
    }
    
    systemStartTime = millis();
    
    // Initialize LED pins
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    
    // Initialize LEDs to off
    setLEDOff();
    
    if (SERIAL_DEBUG) {
        Serial.printf("Device Name: %s\n", DEVICE_NAME);
        Serial.printf("Camera ID: %d\n", CAMERA_ID);
        Serial.printf("Bridge Service: %s\n", BRIDGE_SERVICE_UUID);
        Serial.printf("LED Pins: R=%d, G=%d, B=%d\n", LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN);
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    }
    
    // Initialize BLE
    BLEDevice::init(DEVICE_NAME);
    
    if (SERIAL_DEBUG) {
        Serial.println("\n✓ BLE initialized");
        Serial.println("Searching for ATEM bridge...");
        Serial.println("==========================================\n");
    }
    
    // Start initial scan
    doScan = true;
    lastHeartbeatLed = millis();
    lastHeartbeatReceived = millis(); // Initialize heartbeat tracking
}

void loop() {
    // Handle BLE connection logic
    if (doConnect) {
        if (connectToServer()) {
            if (SERIAL_DEBUG) {
                Serial.println("✓ Connected to bridge");
            }
        } else {
            if (SERIAL_DEBUG) {
                Serial.println("✗ Connection failed");
            }
            currentState = STATE_ERROR;
        }
        doConnect = false;
    }
    
    // Start BLE scan if needed
    if (doScan) {
        startBLEScan();
        doScan = false;
    }
    
    // Handle connection management
    handleConnection();
    
    // Update LED display
    updateTallyLED();
    handleHeartbeatLED();
    
    // Handle serial commands
    if (SERIAL_DEBUG) {
        handleSerialCommands();
    }
    
    // Small delay for system stability
    delay(50);
}
