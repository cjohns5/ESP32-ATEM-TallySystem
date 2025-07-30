# Changelog

All notable changes to the ESP32 ATEM Tally System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [3.0.0] - 2025-07-30

### Added
- **Standby Preview Mode**: Non-active cameras show as PREVIEW (green) when production is active
- **ATEMmin Library Integration**: Replaced manual TCP parsing with proven SKAARHOJ library
- **Heartbeat Monitoring**: 5-second heartbeat signals with 15-second timeout detection
- **Connection Status Reporting**: Bridge reports ATEM connection status to tally lights
- **Enhanced Serial Commands**: Added STANDBY, HELP, and improved status commands
- **Message Integrity**: Checksum verification for all BLE communications
- **Auto-reconnection**: Automatic recovery from ATEM and network connection loss
- **Performance Optimization**: Reduced latency and improved responsiveness

### Changed
- **BREAKING**: Upgraded from Bluetooth Classic to BLE for multiple device support
- **BREAKING**: New message structure with additional status fields
- **BREAKING**: Updated pin assignments and configuration options
- Improved error handling and connection management
- Enhanced LED status indicators with more detailed states
- Optimized polling intervals for better responsiveness
- Updated documentation with comprehensive setup guides

### Fixed
- **Critical**: Fixed ATEM indexing bug (was using 1-based with 0-based library methods)
- Resolved memory leaks in BLE communication
- Fixed race conditions in tally state updates
- Corrected USB tethering network detection
- Improved BLE pairing reliability

### Removed
- Legacy Bluetooth Classic support
- Manual TCP/IP ATEM parsing code
- Outdated configuration options

## [2.0.0] - 2025-06-15

### Added
- BLE (Bluetooth Low Energy) support
- Multiple simultaneous device connections (up to 4)
- Secure pairing and encryption
- Individual device registration system
- Enhanced LED status indicators
- Battery level monitoring
- Connection loss detection

### Changed
- Migrated from WiFi to BLE communication
- Improved power efficiency for battery operation
- Updated hardware requirements

### Fixed
- Network stability issues
- Power consumption optimization
- Connection reliability improvements

## [1.0.0] - 2025-05-01

### Added
- Initial release
- Basic WiFi-based tally system
- Single device support
- ATEM switcher integration
- RGB LED indicators
- Serial command interface

### Features
- WiFi communication between bridge and tally lights
- Support for up to 20 camera inputs
- Basic tally states (Program, Preview, Off)
- Manual testing commands
- USB tethering support

## Version Comparison

| Feature | v1.0 | v2.0 | v3.0 |
|---------|------|------|------|
| Communication | WiFi | BLE | BLE |
| Multiple Devices | No | Yes (4) | Yes (4) |
| ATEM Library | Manual | Manual | ATEMmin |
| Standby Preview | No | No | Yes |
| Heartbeat Monitor | No | Basic | Advanced |
| Connection Recovery | Manual | Basic | Automatic |
| Security | WPA2 | BLE Pairing | BLE Encryption |
| Battery Life | N/A | 6-8 hours | 8-12 hours |
| Latency | 200-500ms | 100-200ms | 65-160ms |

## Migration Guide

### From v2.0 to v3.0
- **Code Changes**: Update both bridge and tally light code
- **Configuration**: Review and update configuration parameters
- **Libraries**: Install ATEMmin library
- **Testing**: Verify standby preview mode functionality
- **Network**: No changes required to network setup

### From v1.0 to v3.0
- **Hardware**: No hardware changes required
- **Code**: Complete code replacement required
- **Libraries**: Install BLE and ATEMmin libraries
- **Network**: USB tethering setup remains the same
- **Configuration**: New configuration file format

## Known Issues

### v3.0.0
- **Range Limitation**: BLE range limited to ~10 meters
- **Device Limit**: Maximum 4 simultaneous connections per bridge
- **iOS Compatibility**: Some iOS devices may have pairing issues

### Workarounds
- **Extended Range**: Use multiple bridges for larger setups
- **More Devices**: Deploy additional bridges for >4 tally lights
- **iOS Issues**: Use Android devices or dedicated ESP32 tally lights

## Upcoming Features

### v3.1.0 (Planned)
- Battery level monitoring and reporting
- Sleep mode for extended battery life
- Over-the-air (OTA) firmware updates
- Web-based configuration interface
- Extended range with ESP32-S3

### v4.0.0 (Roadmap)
- WiFi 6 support for improved range and reliability
- Mesh networking for unlimited scalability
- Advanced power management
- Professional mounting accessories
- Mobile app for monitoring and control

## Support and Compatibility

### ESP32 Compatibility
- **Supported**: ESP32-WROOM-32, ESP32-DevKitC, ESP32-WROVER
- **Tested**: ESP32-S2, ESP32-S3 (limited testing)
- **Not Supported**: ESP8266, ESP32-C3

### ATEM Compatibility
- **Fully Supported**: All current ATEM models with Ethernet
- **Tested**: ATEM Mini, ATEM Mini Pro, ATEM 1 M/E, ATEM 2 M/E
- **Requirements**: Firmware 8.0 or newer recommended

### IDE Compatibility
- **Arduino IDE**: 1.8.19 or newer
- **PlatformIO**: Core 6.0 or newer
- **ESP-IDF**: 4.4 or newer (advanced users)

---

For detailed technical information, see the [API Reference](api-reference.md).

For installation instructions, see the [Software Installation Guide](software-installation.md).

For hardware setup, see the [Hardware Setup Guide](hardware-setup.md).
