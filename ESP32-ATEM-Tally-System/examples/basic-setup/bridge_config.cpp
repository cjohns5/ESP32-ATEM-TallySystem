/*
 * ESP32 ATEM Tally System - Basic Setup Example
 * Bridge Configuration for 2-Camera Setup
 * 
 * This configuration is optimized for:
 * - Simple 2-camera operation
 * - USB tethering to PC/ATEM
 * - Stable, reliable operation
 * - Easy troubleshooting
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <ATEMmin.h>

// Network Configuration - CHANGE THESE FOR YOUR SETUP
#define ATEM_IP "192.168.1.100"        // Your ATEM switcher IP address
#define WIFI_SSID "USB_TETHER"         // Usually auto-created by USB tethering
#define WIFI_PASSWORD ""               // Often empty for USB tethering

// Basic Setup Configuration
#define MAX_TALLY_DEVICES 2             // Only 2 cameras for basic setup
#define TALLY_CHECK_INTERVAL 150        // Slightly relaxed timing for stability
#define HEARTBEAT_INTERVAL 5000         // Connection monitoring interval
#define CONNECTION_TIMEOUT 30000        // 30 second timeout for connections

// BLE Configuration
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// Camera Assignment
#define CAM1_DEVICE_ID 1
#define CAM2_DEVICE_ID 2

// Global objects
ATEMmin atem;
BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
unsigned long lastTallyCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastATEMConnection = 0;

// Tally device tracking
struct TallyDevice {
  uint8_t deviceId;
  bool connected;
  unsigned long lastSeen;
  String state;
};

TallyDevice tallyDevices[MAX_TALLY_DEVICES];

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client connected");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client disconnected");
      // Restart advertising
      pServer->getAdvertising()->start();
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 ATEM Bridge - Basic Setup");
  Serial.println("Initializing...");

  // Initialize tally device tracking
  for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
    tallyDevices[i].deviceId = i + 1;
    tallyDevices[i].connected = false;
    tallyDevices[i].lastSeen = 0;
    tallyDevices[i].state = "UNKNOWN";
  }

  // Initialize BLE
  Serial.println("Starting BLE server...");
  BLEDevice::init("ATEM-Bridge-Basic");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->start();
  
  Serial.println("BLE server started, advertising...");

  // Connect to WiFi (USB tethering)
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Check USB tethering and network settings");
  }

  // Connect to ATEM
  Serial.println("Connecting to ATEM...");
  atem.begin(ATEM_IP);
  Serial.println("Setup complete!");
  Serial.println("Waiting for tally devices to connect...");
}

void loop() {
  // Check ATEM connection
  atem.runLoop();
  
  unsigned long currentTime = millis();
  
  // Periodic tally updates
  if (currentTime - lastTallyCheck >= TALLY_CHECK_INTERVAL) {
    updateTallyStates();
    lastTallyCheck = currentTime;
  }
  
  // Heartbeat monitoring
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = currentTime;
  }
  
  delay(10); // Small delay for stability
}

void updateTallyStates() {
  if (!atem.isConnected()) {
    // Try to reconnect to ATEM
    unsigned long currentTime = millis();
    if (currentTime - lastATEMConnection > 5000) { // Try every 5 seconds
      Serial.println("Attempting ATEM reconnection...");
      atem.begin(ATEM_IP);
      lastATEMConnection = currentTime;
    }
    return;
  }

  // Update states for basic 2-camera setup
  for (int cam = 1; cam <= 2; cam++) {
    String newState = getTallyState(cam);
    updateDeviceState(cam, newState);
  }
}

String getTallyState(int cam) {
  if (!atem.isConnected()) {
    return "DISCONNECTED";
  }

  // Convert to 0-based index for ATEM library
  int atemIndex = cam - 1;
  
  // Get tally flags from ATEM
  uint8_t tallyFlags = atem.getTallyByIndexTallyFlags(atemIndex);
  
  // Check if camera is live (bit 0)
  if (tallyFlags & 0x01) {
    return "LIVE";
  }
  
  // Check if camera is in preview (bit 1)  
  if (tallyFlags & 0x02) {
    return "PREVIEW";
  }
  
  // Check for standby preview mode
  // If any camera is live and this camera isn't, show as preview-ready
  bool anyLive = false;
  for (int checkCam = 0; checkCam < 2; checkCam++) {
    if (atem.getTallyByIndexTallyFlags(checkCam) & 0x01) {
      anyLive = true;
      break;
    }
  }
  
  if (anyLive) {
    return "STANDBY"; // Show as preview-ready when production is active
  }
  
  return "OFF";
}

void updateDeviceState(int deviceId, String newState) {
  // Find device in tracking array
  for (int i = 0; i < MAX_TALLY_DEVICES; i++) {
    if (tallyDevices[i].deviceId == deviceId) {
      if (tallyDevices[i].state != newState) {
        tallyDevices[i].state = newState;
        sendTallyUpdate(deviceId, newState);
        Serial.printf("CAM%d: %s\n", deviceId, newState.c_str());
      }
      break;
    }
  }
}

void sendTallyUpdate(int deviceId, String state) {
  if (deviceConnected && pCharacteristic) {
    String message = String(deviceId) + ":" + state;
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();
  }
}

void sendHeartbeat() {
  if (deviceConnected && pCharacteristic) {
    String heartbeat = "HEARTBEAT:" + String(millis());
    pCharacteristic->setValue(heartbeat.c_str());
    pCharacteristic->notify();
  }
  
  // Print status summary
  Serial.println("=== Basic Setup Status ===");
  Serial.printf("ATEM Connected: %s\n", atem.isConnected() ? "YES" : "NO");
  Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.printf("BLE Clients: %s\n", deviceConnected ? "Connected" : "None");
  
  // Show camera states
  for (int i = 0; i < 2; i++) {
    Serial.printf("CAM%d: %s\n", i+1, tallyDevices[i].state.c_str());
  }
  Serial.println("========================");
}
