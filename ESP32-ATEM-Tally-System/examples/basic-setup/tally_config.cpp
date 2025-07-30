/*
 * ESP32 ATEM Tally System - Basic Setup Example  
 * Tally Light Configuration for 2-Camera Setup
 * 
 * This configuration is optimized for:
 * - Simple RGB LED indicators
 * - Good battery life
 * - Clear visual feedback
 * - Easy troubleshooting
 */

#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Device Configuration - CHANGE THIS FOR EACH TALLY LIGHT
#define DEVICE_ID 1                     // Set to 1 for first camera, 2 for second camera

// LED Pin Configuration
#define RED_LED_PIN 25
#define GREEN_LED_PIN 26  
#define BLUE_LED_PIN 27

// Basic Setup Configuration
#define LED_BRIGHTNESS 128              // Medium brightness for good battery life
#define CONNECTION_RETRY_DELAY 5000     // Wait 5 seconds between connection attempts
#define HEARTBEAT_TIMEOUT 15000         // Consider disconnected after 15 seconds
#define STATUS_BLINK_INTERVAL 2000      // Status indicator timing

// BLE Configuration  
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-cba987654321"

// Global variables
BLEClient* pClient = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
bool connected = false;
String currentState = "DISCONNECTED";
unsigned long lastHeartbeat = 0;
unsigned long lastConnectionAttempt = 0;
unsigned long lastStatusBlink = 0;
bool statusBlinkState = false;

// LED state tracking
struct LEDState {
  bool red;
  bool green; 
  bool blue;
  int brightness;
};

LEDState currentLED = {false, false, false, LED_BRIGHTNESS};

void setup() {
  Serial.begin(115200);
  Serial.printf("ESP32 Tally Light - Basic Setup (Device ID: %d)\n", DEVICE_ID);
  
  // Initialize LED pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  
  // Turn off all LEDs initially
  setLED(false, false, false);
  
  Serial.println("Initializing BLE...");
  BLEDevice::init("ATEM-Tally-" + String(DEVICE_ID));
  
  // Start connection sequence
  Serial.println("Searching for ATEM Bridge...");
  connectToBridge();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle connection status
  if (!connected) {
    // Show connection status with blinking blue
    if (currentTime - lastStatusBlink >= STATUS_BLINK_INTERVAL) {
      statusBlinkState = !statusBlinkState;
      setLED(false, false, statusBlinkState);
      lastStatusBlink = currentTime;
    }
    
    // Attempt reconnection
    if (currentTime - lastConnectionAttempt >= CONNECTION_RETRY_DELAY) {
      connectToBridge();
      lastConnectionAttempt = currentTime;
    }
  } else {
    // Check for heartbeat timeout
    if (currentTime - lastHeartbeat > HEARTBEAT_TIMEOUT) {
      Serial.println("Heartbeat timeout - connection lost");
      connected = false;
      pClient->disconnect();
      return;
    }
    
    // Update LED based on current state
    updateLEDFromState();
  }
  
  delay(100); // Small delay for stability
}

void connectToBridge() {
  Serial.println("Attempting to connect to bridge...");
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  BLEScanResults* scanResults = pBLEScan->start(5, false);
  
  for (int i = 0; i < scanResults->getCount(); i++) {
    BLEAdvertisedDevice device = scanResults->getDevice(i);
    
    // Look for our bridge device
    if (device.getName() == "ATEM-Bridge-Basic") {
      Serial.println("Found ATEM Bridge! Connecting...");
      
      pClient = BLEDevice::createClient();
      
      if (pClient->connect(&device)) {
        Serial.println("Connected to bridge");
        
        // Get the service
        BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
        if (pRemoteService == nullptr) {
          Serial.println("Failed to find service");
          pClient->disconnect();
          return;
        }
        
        // Get the characteristic
        pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
        if (pRemoteCharacteristic == nullptr) {
          Serial.println("Failed to find characteristic");
          pClient->disconnect();
          return;
        }
        
        // Register for notifications
        if(pRemoteCharacteristic->canNotify()) {
          pRemoteCharacteristic->registerForNotify(notifyCallback);
          Serial.println("Registered for notifications");
        }
        
        connected = true;
        lastHeartbeat = millis();
        currentState = "CONNECTED";
        
        // Send device registration
        String regMessage = "REGISTER:" + String(DEVICE_ID);
        pRemoteCharacteristic->writeValue(regMessage.c_str());
        
        Serial.printf("Tally light %d ready!\n", DEVICE_ID);
        break;
      }
    }
  }
  
  pBLEScan->clearResults();
}

// Callback for receiving data from bridge
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                          uint8_t* pData, size_t length, bool isNotify) {
  String message = "";
  for (size_t i = 0; i < length; i++) {
    message += (char)pData[i];
  }
  
  // Parse message
  if (message.startsWith("HEARTBEAT:")) {
    lastHeartbeat = millis();
    return;
  }
  
  // Parse tally state message (format: "deviceId:state")
  int colonIndex = message.indexOf(':');
  if (colonIndex > 0) {
    int deviceId = message.substring(0, colonIndex).toInt();
    String state = message.substring(colonIndex + 1);
    
    // Only process messages for this device
    if (deviceId == DEVICE_ID) {
      currentState = state;
      Serial.printf("Received state: %s\n", state.c_str());
    }
  }
}

void updateLEDFromState() {
  if (currentState == "LIVE") {
    // Red for live
    setLED(true, false, false);
  }
  else if (currentState == "PREVIEW") {
    // Green for preview  
    setLED(false, true, false);
  }
  else if (currentState == "STANDBY") {
    // Green for standby preview (same as preview for basic setup)
    setLED(false, true, false);
  }
  else if (currentState == "OFF") {
    // All LEDs off
    setLED(false, false, false);
  }
  else if (currentState == "DISCONNECTED") {
    // Blue blinking handled in main loop
    return;
  }
  else {
    // Unknown state - dim white
    setLED(true, true, true, 64);
  }
}

void setLED(bool red, bool green, bool blue, int brightness = LED_BRIGHTNESS) {
  // Update LED state
  currentLED.red = red;
  currentLED.green = green; 
  currentLED.blue = blue;
  currentLED.brightness = brightness;
  
  // Apply to hardware pins
  analogWrite(RED_LED_PIN, red ? brightness : 0);
  analogWrite(GREEN_LED_PIN, green ? brightness : 0);
  analogWrite(BLUE_LED_PIN, blue ? brightness : 0);
}

void printStatus() {
  Serial.println("=== Tally Status ===");
  Serial.printf("Device ID: %d\n", DEVICE_ID);
  Serial.printf("Connected: %s\n", connected ? "YES" : "NO");
  Serial.printf("Current State: %s\n", currentState.c_str());
  Serial.printf("LED: R:%d G:%d B:%d\n", 
                currentLED.red ? currentLED.brightness : 0,
                currentLED.green ? currentLED.brightness : 0, 
                currentLED.blue ? currentLED.brightness : 0);
  Serial.println("==================");
}
