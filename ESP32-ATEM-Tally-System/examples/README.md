# ESP32 ATEM Tally System Examples

This directory contains example configurations and use cases for the ESP32 ATEM Tally System.

## Examples Included

### Basic Configurations
- **[Basic Setup](basic-setup/)**: Simple 2-camera setup with minimal configuration
- **[Multi-Camera](multi-camera/)**: 4-camera production setup with standby preview
- **[Single-Operator](single-operator/)**: Streamlined setup for solo content creators

### Advanced Configurations  
- **[Live-Streaming](live-streaming/)**: Optimized for live streaming with OBS/ATEM integration
- **[Event-Production](event-production/)**: Large event setup with multiple bridges
- **[Studio-Production](studio-production/)**: Professional studio environment

### Custom Hardware
- **[NeoPixel-Version](neopixel-version/)**: Using WS2812 NeoPixels instead of RGB LEDs
- **[Battery-Optimized](battery-optimized/)**: Extended battery life configuration
- **[Wireless-Charging](wireless-charging/)**: Qi wireless charging integration

### Testing and Development
- **[Test-Harness](test-harness/)**: Automated testing setup for development
- **[Debug-Configuration](debug-configuration/)**: Enhanced debugging and monitoring
- **[Performance-Testing](performance-testing/)**: Latency and throughput measurement

## Using Examples

1. **Choose appropriate example** for your use case
2. **Copy configuration** from example files  
3. **Modify for your setup** (IP addresses, camera assignments, etc.)
4. **Test thoroughly** before production use

## Configuration Quick Reference

### Common Settings
```cpp
// Network Configuration
#define ATEM_IP "192.168.1.100"

// Performance Settings  
#define TALLY_CHECK_INTERVAL 100    // Latency vs CPU usage
#define HEARTBEAT_INTERVAL 5000     // Connection monitoring
#define MAX_TALLY_DEVICES 4         // Concurrent connections

// Feature Settings
#define STANDBY_AS_PREVIEW true     // Show ready cameras as green
#define LED_BRIGHTNESS 128          // Power vs visibility
```

### Camera Assignment Matrix
| Camera | Use Case | Typical Assignment |
|--------|----------|-------------------|
| CAM1 | Wide Shot | Main/Master shot |
| CAM2 | Close-up | Presenter/Talent |
| CAM3 | Side Angle | Audience/Secondary |
| CAM4 | Detail/Insert | Graphics/Products |

### Network Planning
| Setup Size | Bridge Count | Network Topology |
|------------|-------------|------------------|
| 1-4 Cameras | 1 Bridge | Single USB tether |
| 5-8 Cameras | 2 Bridges | Dual USB/Ethernet |
| 9+ Cameras | 3+ Bridges | Managed switch recommended |

## Troubleshooting Examples

Each example includes:
- **Known Issues**: Common problems and solutions
- **Performance Notes**: Expected latency and battery life
- **Scaling Options**: How to expand the setup
- **Alternative Configurations**: Different approaches to same goal

## Contributing Examples

We welcome new example configurations! Please include:
- **Complete configuration files**
- **Hardware requirements list**
- **Setup instructions**
- **Performance characteristics**
- **Photos/diagrams** (if applicable)

See [Contributing Guide](../CONTRIBUTING.md) for submission process.
