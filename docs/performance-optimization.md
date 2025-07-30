# Performance Optimization Guide

Comprehensive guide for optimizing ESP32 ATEM Tally System performance, latency, and reliability.

## 🎯 Performance Targets

### **System Latency Goals**
- **Total System Latency**: <100ms (ATEM → Tally Light)
- **ATEM Polling**: 25-100ms
- **BLE Communication**: 10-30ms
- **LED Update**: <10ms

### **Reliability Targets**
- **Connection Uptime**: >99% during production
- **Battery Life**: 8-12 hours continuous operation
- **Range**: 10+ meters line-of-sight
- **Simultaneous Devices**: 4+ tally lights per bridge

## ⚡ Bridge Performance Optimization

### **Network Optimization**

#### **USB Tethering Settings**
```cpp
// Optimal network configuration
#define NETWORK_CHECK_INTERVAL 30000    // Don't check too frequently
#define USB_TETHER_TIMEOUT 30000        // Allow time for network setup
```

**PC Network Optimization:**
- Use gigabit Ethernet connection
- Disable WiFi when using tethering
- Set network adapter to full duplex
- Disable power management on USB ports

#### **ATEM Connection Optimization**
```cpp
// Aggressive ATEM polling for low latency
#define TALLY_CHECK_INTERVAL 25         // Minimum safe interval
#define ATEM_RECONNECT_INTERVAL 5000    // Quick reconnection

// Conservative polling for stability
#define TALLY_CHECK_INTERVAL 100        // Standard interval
#define ATEM_RECONNECT_INTERVAL 10000   // Stable reconnection
```

**Network Quality Tips:**
- Use Cat6 Ethernet cables
- Minimize network switches between PC and ATEM
- Avoid network traffic during production
- Consider dedicated ATEM network

### **BLE Performance Optimization**

#### **Connection Parameters**
```cpp
// Optimized BLE settings
#define MAX_TALLY_DEVICES 4             // Don't exceed ESP32 limits
#define HEARTBEAT_INTERVAL 5000         // Balance between detection and overhead
```

#### **Message Optimization**
```cpp
// Efficient message structure (already optimized)
typedef struct {
    uint8_t cameraId;        // 1 byte
    char state[12];          // 12 bytes (could reduce to 8)
    uint32_t timestamp;      // 4 bytes
    uint8_t bridgeId;        // 1 byte
    uint8_t bridgeStatus;    // 1 byte
    uint8_t checksum;        // 1 byte
} __attribute__((packed)) TallyMessage; // Total: 20 bytes
```

**Advanced BLE Optimization:**
```cpp
// Reduce message size for higher throughput
typedef struct {
    uint8_t cameraId;        // 1 byte
    uint8_t state;           // 1 byte (enum instead of string)
    uint16_t timestamp;      // 2 bytes (reduced precision)
    uint8_t bridgeStatus;    // 1 byte
    uint8_t checksum;        // 1 byte
} __attribute__((packed)) OptimizedTallyMessage; // Total: 6 bytes
```

### **Memory Management**

#### **Heap Optimization**
```cpp
void setup() {
    // Check available memory at startup
    Serial.printf("Free heap: %d bytes\\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %d bytes\\n", ESP.getMaxAllocHeap());
}

void loop() {
    // Monitor memory usage (remove in production)
    static unsigned long lastMemCheck = 0;
    if (millis() - lastMemCheck > 30000) {
        Serial.printf("Free heap: %d bytes\\n", ESP.getFreeHeap());
        lastMemCheck = millis();
    }
}
```

#### **String Optimization**
```cpp
// Avoid String objects - use char arrays
char atemIP[16] = "192.168.1.100";  // Fixed size
char deviceName[32];                 // Pre-allocated

// Use F() macro for constant strings
Serial.println(F("ATEM Connected"));
```

### **CPU Performance**

#### **Task Prioritization**
```cpp
// Main loop optimization
void loop() {
    // High priority: ATEM communication
    if (millis() - lastATEMCheck > TALLY_CHECK_INTERVAL) {
        checkATEMTallyStates();
        lastATEMCheck = millis();
    }
    
    // Medium priority: BLE heartbeat
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
        sendHeartbeatSignal();
        lastHeartbeat = millis();
    }
    
    // Low priority: Network monitoring
    if (millis() - lastNetworkCheck > NETWORK_CHECK_INTERVAL) {
        checkNetworkConnectivity();
        lastNetworkCheck = millis();
    }
    
    // Handle serial commands (lowest priority)
    handleSerialCommands();
    
    // Don't block the loop
    yield();
}
```

## 🔋 Tally Light Power Optimization

### **Battery Life Optimization**

#### **LED Power Management**
```cpp
// Adaptive brightness based on battery level
uint8_t getBatteryOptimizedBrightness() {
    float voltage = analogRead(A0) * 3.3 / 4096 * 2; // Voltage divider
    
    if (voltage > 4.0) return 255;        // Full brightness
    else if (voltage > 3.7) return 200;   // 78% brightness
    else if (voltage > 3.5) return 150;   // 59% brightness
    else if (voltage > 3.2) return 100;   // 39% brightness
    else return 50;                       // Low battery mode
}

void updateLEDStatus() {
    uint8_t brightness = getBatteryOptimizedBrightness();
    
    if (currentTallyState == "PROGRAM") {
        setLED(255, 0, 0, brightness);  // Red with adaptive brightness
    }
    // ... other states
}
```

#### **Sleep Mode Implementation**
```cpp
#include "esp_sleep.h"

// Light sleep during idle periods
void enterLightSleep(uint32_t sleepTimeMs) {
    esp_sleep_enable_timer_wakeup(sleepTimeMs * 1000);
    esp_light_sleep_start();
}

// Use during inactive periods
void loop() {
    // Normal operation
    handleConnection();
    updateLEDStatus();
    
    // Sleep when no activity detected
    static unsigned long lastActivity = millis();
    if (millis() - lastActivity > 5000 && currentTallyState == "OFF") {
        enterLightSleep(1000); // Sleep 1 second
        lastActivity = millis();
    }
}
```

### **Connection Optimization**

#### **Intelligent Reconnection**
```cpp
// Exponential backoff with maximum limit
class ConnectionManager {
private:
    uint32_t baseInterval = 1000;
    uint32_t currentInterval = 1000;
    uint32_t maxInterval = 30000;
    uint32_t lastAttempt = 0;
    uint8_t attempts = 0;

public:
    bool shouldAttemptReconnection() {
        if (millis() - lastAttempt > currentInterval) {
            lastAttempt = millis();
            attempts++;
            
            // Exponential backoff
            currentInterval = min(baseInterval * pow(2, attempts), maxInterval);
            return true;
        }
        return false;
    }
    
    void resetOnSuccess() {
        currentInterval = baseInterval;
        attempts = 0;
    }
};
```

#### **Signal Strength Optimization**
```cpp
// Monitor BLE signal strength
void checkConnectionQuality() {
    if (pRemoteCharacteristic != nullptr) {
        // Get RSSI (signal strength)
        int rssi = pBLEClient->getRssi();
        
        if (rssi < -80) {
            Serial.println("Weak signal - consider moving closer");
            // Reduce update frequency to conserve power
            connectionState = WEAK_SIGNAL;
        } else if (rssi > -60) {
            connectionState = STRONG_SIGNAL;
        }
    }
}
```

## 📡 BLE Range and Reliability

### **Antenna Optimization**

#### **Physical Positioning**
- **Bridge Placement**: Elevated, central location
- **Tally Placement**: Avoid metal camera bodies blocking signal
- **Orientation**: ESP32 antenna perpendicular to metal surfaces

#### **RF Environment**
```cpp
// Channel scanning for less congested frequencies
void optimizeBLEChannel() {
    // BLE automatically handles this, but minimize 2.4GHz interference:
    // - Move away from WiFi routers
    // - Avoid microwave ovens
    // - Separate from wireless cameras
}
```

### **Multi-Path and Interference**

#### **Environmental Considerations**
- **Line of Sight**: Best performance within 10 meters
- **Obstacles**: Each wall reduces range by ~30%
- **Interference**: WiFi, Bluetooth, wireless cameras on 2.4GHz

#### **Reliability Improvements**
```cpp
// Message retry mechanism
class ReliableMessaging {
private:
    uint8_t retryCount = 0;
    const uint8_t maxRetries = 3;
    TallyMessage lastMessage;

public:
    bool sendWithRetry(TallyMessage msg) {
        for (retryCount = 0; retryCount < maxRetries; retryCount++) {
            if (sendMessage(msg)) {
                return true;  // Success
            }
            delay(100 * (retryCount + 1));  // Progressive delay
        }
        return false;  // Failed after retries
    }
};
```

## 🚀 Advanced Performance Features

### **Predictive Tally States**

#### **State Change Prediction**
```cpp
// Predict likely state changes based on ATEM patterns
class TallyPredictor {
private:
    uint8_t lastProgram = 0;
    uint8_t lastPreview = 0;
    unsigned long lastChange = 0;

public:
    void updatePattern(uint8_t program, uint8_t preview) {
        if (program != lastProgram || preview != lastPreview) {
            // Pattern detected - could pre-load next likely state
            if (millis() - lastChange < 5000) {
                // Rapid switching detected
                increasePollingRate();
            }
            
            lastProgram = program;
            lastPreview = preview;
            lastChange = millis();
        }
    }
    
    void increasePollingRate() {
        // Temporarily increase ATEM polling during active switching
        // Reset to normal after period of stability
    }
};
```

### **Load Balancing Multiple Bridges**

#### **Bridge Selection Algorithm**
```cpp
// For large installations with multiple bridges
class BridgeLoadBalancer {
private:
    struct BridgeInfo {
        String bridgeId;
        uint8_t connectedDevices;
        int8_t signalStrength;
        bool isActive;
    };
    
    std::vector<BridgeInfo> bridges;

public:
    String selectOptimalBridge() {
        String bestBridge = "";
        int bestScore = -1000;
        
        for (auto& bridge : bridges) {
            if (!bridge.isActive) continue;
            
            // Score based on load and signal strength
            int score = bridge.signalStrength - (bridge.connectedDevices * 10);
            
            if (score > bestScore) {
                bestScore = score;
                bestBridge = bridge.bridgeId;
            }
        }
        
        return bestBridge;
    }
};
```

### **Quality of Service (QoS)**

#### **Priority-Based Messaging**
```cpp
enum MessagePriority {
    CRITICAL = 0,    // PROGRAM state changes
    HIGH = 1,        // PREVIEW state changes
    NORMAL = 2,      // Status updates
    LOW = 3          // Heartbeats, diagnostics
};

class QoSMessaging {
private:
    std::queue<TallyMessage> priorityQueues[4];

public:
    void queueMessage(TallyMessage msg, MessagePriority priority) {
        priorityQueues[priority].push(msg);
    }
    
    bool sendNextMessage() {
        // Send highest priority message first
        for (int i = 0; i < 4; i++) {
            if (!priorityQueues[i].empty()) {
                TallyMessage msg = priorityQueues[i].front();
                priorityQueues[i].pop();
                return sendMessage(msg);
            }
        }
        return false;
    }
};
```

## 📊 Performance Monitoring

### **Real-Time Metrics**

#### **Latency Measurement**
```cpp
class PerformanceMonitor {
private:
    unsigned long atemPollTime = 0;
    unsigned long atemResponseTime = 0;
    unsigned long bleSendTime = 0;
    unsigned long bleAckTime = 0;

public:
    void startATEMPoll() { atemPollTime = micros(); }
    void endATEMPoll() { atemResponseTime = micros() - atemPollTime; }
    
    void startBLESend() { bleSendTime = micros(); }
    void endBLESend() { bleAckTime = micros() - bleSendTime; }
    
    void printMetrics() {
        Serial.printf("ATEM latency: %lu μs\\n", atemResponseTime);
        Serial.printf("BLE latency: %lu μs\\n", bleAckTime);
        Serial.printf("Total latency: %lu μs\\n", atemResponseTime + bleAckTime);
    }
};
```

#### **Connection Quality Metrics**
```cpp
class ConnectionQualityMonitor {
private:
    uint32_t messagesTotal = 0;
    uint32_t messagesLost = 0;
    uint32_t reconnections = 0;
    unsigned long uptimeStart = 0;

public:
    void messageSuccess() { messagesTotal++; }
    void messageLost() { messagesLost++; messagesTotal++; }
    void reconnected() { reconnections++; }
    
    float getReliability() {
        if (messagesTotal == 0) return 100.0;
        return ((float)(messagesTotal - messagesLost) / messagesTotal) * 100.0;
    }
    
    float getUptime() {
        return (millis() - uptimeStart) / 1000.0 / 3600.0; // Hours
    }
    
    void printQualityReport() {
        Serial.printf("Reliability: %.2f%%\\n", getReliability());
        Serial.printf("Uptime: %.2f hours\\n", getUptime());
        Serial.printf("Reconnections: %u\\n", reconnections);
    }
};
```

## 🔧 Configuration Profiles

### **Production Profile** (Maximum Reliability)
```cpp
// Conservative settings for live production
#define TALLY_CHECK_INTERVAL 100
#define HEARTBEAT_INTERVAL 5000
#define HEARTBEAT_TIMEOUT 15000
#define RECONNECT_INTERVAL 10000
#define LED_BRIGHTNESS 200
#define MAX_TALLY_DEVICES 4
```

### **Low Latency Profile** (Minimum Delay)
```cpp
// Aggressive settings for low latency
#define TALLY_CHECK_INTERVAL 25
#define HEARTBEAT_INTERVAL 2000
#define HEARTBEAT_TIMEOUT 8000
#define RECONNECT_INTERVAL 3000
#define LED_BRIGHTNESS 255
#define MAX_TALLY_DEVICES 2
```

### **Battery Saver Profile** (Extended Operation)
```cpp
// Power-conscious settings
#define TALLY_CHECK_INTERVAL 200
#define HEARTBEAT_INTERVAL 10000
#define HEARTBEAT_TIMEOUT 30000
#define RECONNECT_INTERVAL 20000
#define LED_BRIGHTNESS 128
#define MAX_TALLY_DEVICES 3
```

### **High Capacity Profile** (Many Devices)
```cpp
// Support more devices with reasonable performance
#define TALLY_CHECK_INTERVAL 150
#define HEARTBEAT_INTERVAL 7000
#define HEARTBEAT_TIMEOUT 20000
#define RECONNECT_INTERVAL 15000
#define LED_BRIGHTNESS 180
#define MAX_TALLY_DEVICES 8  // Requires testing
```

## 📈 Scaling Considerations

### **Large Installations**

#### **Multiple Bridge Setup**
```cpp
// Bridge coordination for >4 devices
class BridgeCoordinator {
    struct BridgeAssignment {
        String bridgeId;
        std::vector<uint8_t> assignedCameras;
        IPAddress bridgeIP;
    };
    
    std::vector<BridgeAssignment> bridges;
    
    void distributeCameras() {
        // Assign cameras 1-4 to Bridge A
        // Assign cameras 5-8 to Bridge B
        // etc.
    }
};
```

#### **Performance at Scale**
- **1-4 Cameras**: Single bridge, optimal performance
- **5-12 Cameras**: 2-3 bridges, good performance
- **13-20 Cameras**: 4-5 bridges, requires coordination
- **>20 Cameras**: Consider wired backup system

### **Network Considerations**

#### **Bandwidth Requirements**
- **Single Device**: ~100 bytes/second
- **4 Devices**: ~400 bytes/second
- **Network Overhead**: ~50% additional
- **Total Per Bridge**: <1 KB/second

#### **Latency Budget**
```
Network Propagation:     1-5ms    (LAN)
USB Tethering:          5-15ms    (PC processing)
ATEM Processing:        10-20ms   (switcher response)
ESP32 Processing:       5-15ms    (bridge processing)
BLE Transmission:       10-30ms   (wireless)
Tally Processing:       5-10ms    (LED update)
----------------------------------------
Total System Latency:  36-95ms   (typical)
```

## 🎯 Performance Benchmarks

### **Target Performance Metrics**

| Metric | Excellent | Good | Acceptable | Poor |
|--------|-----------|------|------------|------|
| **Total Latency** | <50ms | 50-100ms | 100-200ms | >200ms |
| **Reliability** | >99.5% | 99-99.5% | 95-99% | <95% |
| **Battery Life** | >12hrs | 8-12hrs | 4-8hrs | <4hrs |
| **Range** | >15m | 10-15m | 5-10m | <5m |
| **Uptime** | >24hrs | 12-24hrs | 4-12hrs | <4hrs |

### **Testing Procedures**

#### **Latency Testing**
1. Connect bridge to ATEM
2. Set up tally light with known camera ID
3. Switch camera to PROGRAM in ATEM
4. Measure time until LED changes to red
5. Repeat 50 times for average

#### **Reliability Testing**
1. Run system continuously for 24 hours
2. Count state change messages vs. successful LED updates
3. Monitor for disconnections and reconnection time
4. Calculate uptime percentage

#### **Range Testing**
1. Place bridge in fixed location
2. Move tally light progressively farther away
3. Test at 1m, 5m, 10m, 15m, 20m distances
4. Note connection quality and latency at each distance
5. Identify maximum reliable range

---

**Remember**: Performance optimization often involves trade-offs between latency, reliability, and power consumption. Choose the profile that best matches your specific use case and environment.
