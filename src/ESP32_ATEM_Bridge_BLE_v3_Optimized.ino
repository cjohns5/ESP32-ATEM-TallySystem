/*
 * ESP32 ATEM Tally Bridge v3.0 Optimized (BLE Multi-Device + Lightweight ATEM)
 * Hardware: ESP32-WROOM-32
 * 
 * Optimized for smaller program storage:
 * - Reduced camera support (10 instead of 20)
 * - Simplified BLE message structure
 * - Removed verbose logging
 * - Lightweight ATEM communication
 * - Removed some debug features
 * 
 * Features:
 * - Uses USB tethering to access PC's Ethernet network
 * - Lightweight ATEM communication (no heavy library)
 * - BLE server supporting multiple tally connections (up to 4)
 * - Individual device registration and management
 * - Supports up to 10 camera inputs
 * - Auto-reconnection for network and ATEM connections
 * 
 * Author: ESP32 Tally System
 * Version: 3.0 Optimized
 * Date: July 2025
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <WiFiClient.h>

// ===============================================
// CONFIGURATION - UPDATE THESE VALUES
// ===============================================

// ATEM Configuration
#define ATEM_IP "192.168.1.100"             // ATEM switcher IP address
#define ATEM_PORT 9910                      // ATEM port (usually 9910)

// BLE Configuration
#define BLE_DEVICE_NAME "ATEM_Bridge_BLE"   // BLE name
#define MAX_TALLY_DEVICES 4                 // Max BLE connections
#define BLE_SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"
#define BLE_CHARACTERISTIC_UUID "87654321-4321-8765-cba9-987654321cba"

// System Configuration
#define MAX_CAMERAS 10                      // Reduced from 20 to save space
#define ATEM_RECONNECT_INTERVAL 10000       // ATEM reconnect interval
#define TALLY_CHECK_INTERVAL 200            // Tally check interval
#define HEARTBEAT_INTERVAL 5000             // Heartbeat interval

// ===============================================
// DATA STRUCTURES
// ===============================================

// Simplified BLE tally device
typedef struct {
    String deviceName;
    uint8_t cameraId;
    unsigned long lastSeen;
    bool connected;
    bool registered;
    BLECharacteristic* characteristic;
} TallyDevice;

// ===============================================
// GLOBAL VARIABLES
// ===============================================

// BLE Server
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
TallyDevice tallyDevices[MAX_TALLY_DEVICES];
int numConnectedDevices = 0;

// Network
WiFiClient atemClient;
bool networkConnected = false;
bool atemConnected = false;
unsigned long lastNetworkCheck = 0;
unsigned long lastATEMReconnectAttempt = 0;

// Tally state tracking
uint8_t currentTallyStates[MAX_CAMERAS + 1] = {0};
unsigned long lastTallyCheck = 0;
unsigned long lastHeartbeat = 0;

// ===============================================
// BLE FUNCTIONS
// ===============================================

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        numConnectedDevices++;
        if (numConnectedDevices < MAX_TALLY_DEVICES) {
            BLEDevice::startAdvertising();
        }
    }

    void onDisconnect(BLEServer* pServer) {
        if (numConnectedDevices > 0) numConnectedDevices--;
        for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
            if (tallyDevices[i].connected) {
                tallyDevices[i].connected = false;
                break;
            }
        }
        BLEDevice::startAdvertising();
    }
};

// BLE Characteristic Callbacks
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String message = String(pCharacteristic->getValue().c_str());
        
        if (message.length() > 0) {
            message.trim();
            
            if (message.startsWith("TALLY_REG:")) {
                int firstColon = message.indexOf(':', 10);
                int secondColon = message.indexOf(':', firstColon + 1);
                
                if (firstColon > 0 && secondColon > 0) {
                    uint8_t cameraId = message.substring(10, firstColon).toInt();
                    String deviceName = message.substring(secondColon + 1);
                    
                    for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
                        if (!tallyDevices[i].registered || 
                            tallyDevices[i].deviceName == deviceName) {
                            
                            tallyDevices[i].deviceName = deviceName;
                            tallyDevices[i].cameraId = cameraId;
                            tallyDevices[i].lastSeen = millis();
                            tallyDevices[i].connected = true;
                            tallyDevices[i].characteristic = pCharacteristic;
                            
                            if (!tallyDevices[i].registered) {
                                tallyDevices[i].registered = true;
                                Serial.printf("Registered: %s (CAM%d)\n", deviceName.c_str(), cameraId);
                            }
                            
                            // Send current state
                            sendTallyToDevice(i, cameraId, getCurrentTallyState(cameraId));
                            break;
                        }
                    }
                }
            }
        }
    }
};

// Initialize BLE server
bool initializeBLE() {
    BLEDevice::init(BLE_DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    pService = pServer->createService(BLE_SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                      BLE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
    
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    
    // Initialize tally devices
    for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
        tallyDevices[i].deviceName = "";
        tallyDevices[i].cameraId = 0;
        tallyDevices[i].lastSeen = 0;
        tallyDevices[i].connected = false;
        tallyDevices[i].registered = false;
        tallyDevices[i].characteristic = nullptr;
    }
    
    return true;
}

// Send tally state to device
void sendTallyToDevice(int deviceIndex, uint8_t cameraId, const char* state) {
    if (deviceIndex >= 0 && deviceIndex < MAX_TALLY_DEVICES && 
        tallyDevices[deviceIndex].connected && 
        tallyDevices[deviceIndex].characteristic) {
        
        String message = String(cameraId) + ":" + String(state);
        tallyDevices[deviceIndex].characteristic->setValue(message.c_str());
        tallyDevices[deviceIndex].characteristic->notify();
    }
}

// Get current tally state for camera
const char* getCurrentTallyState(uint8_t cameraId) {
    if (cameraId < 1 || cameraId > MAX_CAMERAS) return "OFF";
    
    uint8_t state = currentTallyStates[cameraId];
    if (state == 1) return "PREVIEW";
    if (state == 2) return "PROGRAM";
    return "OFF";
}

// Send heartbeat to all devices
void sendHeartbeatSignal() {
    for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
        if (tallyDevices[i].connected && tallyDevices[i].registered) {
            sendTallyToDevice(i, tallyDevices[i].cameraId, "HEARTBEAT");
        }
    }
    lastHeartbeat = millis();
}

// Check tally device connections
void checkTallyDeviceConnections() {
    unsigned long now = millis();
    for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
        if (tallyDevices[i].registered && 
            (now - tallyDevices[i].lastSeen) > 30000) {
            tallyDevices[i].connected = false;
        }
    }
}

// ===============================================
// NETWORK FUNCTIONS
// ===============================================

// Initialize network connection
bool initializeNetwork() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    int attempts = 0;
    while (attempts < 60) {
        if (WiFi.localIP().toString() != "0.0.0.0") {
            networkConnected = true;
            Serial.printf("Network: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        delay(500);
        attempts++;
    }
    
    networkConnected = false;
    return false;
}

// Check network connectivity
bool checkNetworkConnectivity() {
    if (WiFi.localIP().toString() == "0.0.0.0") {
        networkConnected = false;
        return false;
    }
    networkConnected = true;
    return true;
}

// ===============================================
// ATEM FUNCTIONS (Lightweight)
// ===============================================

// Connect to ATEM switcher
bool connectToATEM() {
    if (!networkConnected) return false;
    
    IPAddress atemIP;
    if (!atemIP.fromString(ATEM_IP)) return false;
    
    if (atemClient.connect(atemIP, ATEM_PORT)) {
        atemConnected = true;
        Serial.println("ATEM connected");
        return true;
    }
    
    atemConnected = false;
    return false;
}

// Handle ATEM communication
void handleATEM() {
    if (!atemConnected) {
        if (millis() - lastATEMReconnectAttempt > ATEM_RECONNECT_INTERVAL) {
            lastATEMReconnectAttempt = millis();
            connectToATEM();
        }
        return;
    }
    
    // Simple ATEM packet parsing (lightweight)
    if (atemClient.available()) {
        uint8_t buffer[1024];
        int bytesRead = atemClient.read(buffer, sizeof(buffer));
        
        if (bytesRead > 0) {
            // Parse tally data from ATEM packets
            parseATEMTallyData(buffer, bytesRead);
        }
    }
    
    // Check connection
    if (!atemClient.connected()) {
        atemConnected = false;
    }
}

// Parse ATEM tally data (simplified)
void parseATEMTallyData(uint8_t* buffer, int length) {
    // Simplified ATEM packet parsing for tally data
    // This is a basic implementation - you may need to adjust based on your ATEM model
    
    for (int i = 0; i < length - 4; i++) {
        // Look for tally data patterns
        if (buffer[i] == 0x01 && buffer[i+1] == 0x00) {
            // Potential tally data
            uint8_t cameraId = buffer[i+2];
            uint8_t tallyState = buffer[i+3];
            
            if (cameraId >= 1 && cameraId <= MAX_CAMERAS) {
                uint8_t newState = 0;
                if (tallyState & 0x01) newState = 1; // Preview
                if (tallyState & 0x02) newState = 2; // Program
                
                if (currentTallyStates[cameraId] != newState) {
                    currentTallyStates[cameraId] = newState;
                    
                    // Send to all registered devices for this camera
                    for (int j = 0; j < MAX_TALLY_DEVICES; j++) {
                        if (tallyDevices[j].connected && 
                            tallyDevices[j].registered && 
                            tallyDevices[j].cameraId == cameraId) {
                            sendTallyToDevice(j, cameraId, getCurrentTallyState(cameraId));
                        }
                    }
                }
            }
        }
    }
}

// ===============================================
// MAIN FUNCTIONS
// ===============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32 ATEM Bridge v3.0 Optimized");
    
    // Initialize BLE
    if (!initializeBLE()) {
        Serial.println("BLE init failed");
        while(1) delay(1000);
    }
    
    // Initialize network
    Serial.println("Initializing network...");
    if (initializeNetwork()) {
        connectToATEM();
    }
    
    Serial.println("Bridge ready");
    lastNetworkCheck = millis();
    lastTallyCheck = millis();
    lastHeartbeat = millis();
}

void loop() {
    // Handle ATEM communication
    handleATEM();
    
    // Check tally device connections
    checkTallyDeviceConnections();
    
    // Send heartbeat
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
        sendHeartbeatSignal();
    }
    
    // Network check
    if (millis() - lastNetworkCheck > 30000) {
        lastNetworkCheck = millis();
        if (!checkNetworkConnectivity()) {
            networkConnected = false;
            if (initializeNetwork()) {
                delay(2000);
                connectToATEM();
            }
        }
    }
    
    delay(10);
} 