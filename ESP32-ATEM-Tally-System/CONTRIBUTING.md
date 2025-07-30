# Contributing to ESP32 ATEM Tally System

Thank you for your interest in contributing to the ESP32 ATEM Tally System! This document provides guidelines and information for contributors.

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Process](#development-process)
- [Contribution Types](#contribution-types)
- [Code Standards](#code-standards)
- [Testing Guidelines](#testing-guidelines)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)

## Code of Conduct

This project adheres to a code of conduct that promotes a welcoming and inclusive environment:

- **Be respectful** and constructive in discussions
- **Be patient** with newcomers and different skill levels
- **Be collaborative** and open to feedback
- **Focus on the technology** and avoid personal attacks
- **Help maintain a professional** environment

## Getting Started

### Prerequisites
- Basic knowledge of C/C++ programming
- Familiarity with Arduino IDE or PlatformIO
- Understanding of ESP32 microcontrollers
- Basic knowledge of BLE (Bluetooth Low Energy)
- Experience with ATEM switchers (helpful but not required)

### Development Environment Setup
1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/ESP32-ATEM-Tally-System.git
   cd ESP32-ATEM-Tally-System
   ```
3. **Set up upstream** remote:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/ESP32-ATEM-Tally-System.git
   ```
4. **Install development tools** (see [Software Installation Guide](docs/software-installation.md))

### Hardware for Testing
- ESP32 development boards (minimum 2 for bridge + tally testing)
- RGB LEDs or NeoPixels
- ATEM switcher or ATEM Software Control for testing
- USB cables and basic prototyping supplies

## Development Process

### Branching Strategy
- **main**: Stable releases only
- **develop**: Integration branch for new features
- **feature/**: New features (`feature/battery-monitoring`)
- **bugfix/**: Bug fixes (`bugfix/connection-timeout`)
- **hotfix/**: Critical fixes for main branch

### Workflow
1. **Create feature branch** from `develop`:
   ```bash
   git checkout develop
   git pull upstream develop
   git checkout -b feature/your-feature-name
   ```
2. **Make your changes** with regular commits
3. **Test thoroughly** (see Testing Guidelines)
4. **Update documentation** as needed
5. **Submit pull request** to `develop` branch

## Contribution Types

### Code Contributions
- **New Features**: Standby modes, additional protocols, UI improvements
- **Bug Fixes**: Connection issues, memory leaks, timing problems
- **Performance**: Latency reduction, power optimization, memory usage
- **Compatibility**: New ESP32 variants, ATEM models, IDE versions

### Documentation Contributions
- **Setup Guides**: Installation, configuration, troubleshooting
- **Code Documentation**: Comments, API documentation, examples
- **User Guides**: Usage scenarios, best practices, tutorials
- **Technical Docs**: Architecture, protocols, performance analysis

### Testing Contributions
- **Hardware Testing**: Different ESP32 boards, LED types, ATEM models
- **Environment Testing**: Various network setups, interference scenarios
- **Performance Testing**: Latency measurement, battery life, range testing
- **Automated Testing**: Unit tests, integration tests, CI/CD

### Community Contributions
- **Issue Reporting**: Bug reports, feature requests, questions
- **Support**: Helping other users, answering questions
- **Examples**: Sample projects, use cases, tutorials
- **Translations**: Documentation in other languages

## Code Standards

### C/C++ Style Guidelines
```cpp
// Use descriptive names
void sendTallyStateToDevice(int deviceIndex, uint8_t cameraId, const char* state);

// Constants in ALL_CAPS
#define MAX_TALLY_DEVICES 4
#define HEARTBEAT_INTERVAL 5000

// Variables in camelCase
unsigned long lastHeartbeat = 0;
bool isConnected = false;

// Structs in PascalCase
typedef struct {
    String deviceName;
    uint8_t cameraId;
    bool connected;
} TallyDevice;
```

### Commenting Standards
```cpp
/**
 * Send tally data to a specific BLE device
 * @param deviceIndex Index in tallyDevices array (0-3)
 * @param cameraId Camera number (1-20)
 * @param state Tally state ("PREVIEW", "PROGRAM", "OFF", etc.)
 */
void sendTallyToDevice(int deviceIndex, uint8_t cameraId, const char* state) {
    // Validate parameters
    if (deviceIndex < 0 || deviceIndex >= MAX_TALLY_DEVICES) return;
    
    // Create message structure
    TallyMessage msg;
    // ... implementation
}
```

### Error Handling
```cpp
// Always check return values
if (!AtemSwitcher.isConnected()) {
    Serial.println("ERROR: ATEM not connected");
    return false;
}

// Use descriptive error messages
if (deviceIndex >= MAX_TALLY_DEVICES) {
    Serial.printf("ERROR: Device index %d exceeds maximum %d\n", 
                  deviceIndex, MAX_TALLY_DEVICES);
    return;
}

// Implement graceful degradation
if (!networkConnected) {
    Serial.println("WARNING: Network disconnected, entering offline mode");
    enterOfflineMode();
}
```

### Configuration Management
```cpp
// Use #define for compile-time constants
#define ATEM_IP "192.168.1.100"
#define TALLY_CHECK_INTERVAL 100

// Group related configurations
// ===============================================
// BLE Configuration
// ===============================================
#define BLE_DEVICE_NAME "ATEM_Bridge_BLE"
#define MAX_TALLY_DEVICES 4
#define BLE_SERVICE_UUID "12345678-1234-5678-9abc-123456789abc"

// ===============================================
// Performance Configuration  
// ===============================================
#define TALLY_CHECK_INTERVAL 100
#define HEARTBEAT_INTERVAL 5000
```

## Testing Guidelines

### Unit Testing
```cpp
// Test individual functions
void testCalculateChecksum() {
    TallyMessage msg;
    msg.cameraId = 1;
    strcpy(msg.state, "PROGRAM");
    msg.bridgeId = 1;
    msg.bridgeStatus = 1;
    
    uint8_t checksum = calculateChecksum(&msg);
    assert(checksum != 0); // Basic validation
}
```

### Integration Testing
- **Bridge + Single Tally**: Basic functionality
- **Bridge + Multiple Tallies**: Multi-device scenarios
- **Network Disconnect/Reconnect**: Recovery testing
- **ATEM Disconnect/Reconnect**: ATEM reliability
- **Extended Runtime**: Memory leak detection

### Hardware Testing
- **Different ESP32 Boards**: DevKit, WROVER, custom boards
- **Different LED Types**: RGB, NeoPixel, different brightness
- **Power Sources**: USB, battery, different voltages
- **Environmental**: Temperature, humidity, RF interference

### Performance Testing
```cpp
// Measure latency
unsigned long startTime = micros();
sendTallyToDevice(0, 1, "PROGRAM");
unsigned long latency = micros() - startTime;
Serial.printf("Tally send latency: %lu microseconds\n", latency);

// Memory usage monitoring
void checkMemoryUsage() {
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("Free heap: %d bytes\n", freeHeap);
    
    if (freeHeap < 10000) {
        Serial.println("WARNING: Low memory detected");
    }
}
```

## Documentation

### Code Documentation
- **Function Comments**: Purpose, parameters, return values
- **Complex Logic**: Explain algorithms and state machines
- **Configuration**: Document all configurable parameters
- **API Changes**: Update documentation for any interface changes

### User Documentation
- **Setup Guides**: Step-by-step installation and configuration
- **Troubleshooting**: Common issues and solutions
- **Examples**: Working code examples for common scenarios
- **Performance**: Latency, range, battery life specifications

### Technical Documentation
- **Architecture**: System design and component interaction
- **Protocols**: BLE message formats, ATEM communication
- **Hardware**: Wiring diagrams, component specifications
- **Testing**: Test procedures and validation criteria

## Pull Request Process

### Before Submitting
1. **Test thoroughly** on actual hardware
2. **Update documentation** for any changes
3. **Check code style** follows project conventions
4. **Verify compatibility** with existing features
5. **Update CHANGELOG.md** with your changes

### Pull Request Template
```markdown
## Description
Brief description of changes made.

## Type of Change
- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Tested on ESP32 hardware
- [ ] Tested with ATEM switcher
- [ ] Tested multiple device scenarios
- [ ] Updated/added unit tests
- [ ] Documentation updated

## Hardware Tested
- ESP32 Board: (e.g., ESP32-DevKitC)
- ATEM Model: (e.g., ATEM Mini Pro)
- LED Type: (e.g., RGB LED, NeoPixel)

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review of code completed
- [ ] Documentation updated
- [ ] No new compiler warnings
- [ ] Changes tested on hardware
```

### Review Process
1. **Automated Checks**: Code formatting, compilation
2. **Technical Review**: Code quality, architecture fit
3. **Testing Review**: Adequate test coverage
4. **Documentation Review**: Clear and complete documentation
5. **Final Approval**: Maintainer approval and merge

### Feedback and Iteration
- **Be responsive** to review feedback
- **Ask questions** if feedback is unclear
- **Make requested changes** promptly
- **Update** pull request description if scope changes

## Issue Reporting

### Bug Reports
Use the bug report template:
```markdown
**Hardware Setup**
- ESP32 Board: 
- ATEM Model:
- LED Type:
- Power Source:

**Software Versions**
- Arduino IDE:
- ESP32 Package:
- Project Version:

**Description**
Clear description of the bug.

**Steps to Reproduce**
1. Step 1
2. Step 2
3. Step 3

**Expected Behavior**
What should happen.

**Actual Behavior**
What actually happens.

**Serial Output**
Include relevant serial monitor output.

**Additional Context**
Any other relevant information.
```

### Feature Requests
- **Clear description** of desired functionality
- **Use cases** where this would be valuable
- **Proposed implementation** (if you have ideas)
- **Compatibility considerations** with existing features

## Community

### Communication Channels
- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: General questions, ideas
- **Pull Requests**: Code reviews, technical discussion

### Getting Help
- **Documentation**: Check existing docs first
- **Search Issues**: Look for similar problems/questions
- **Ask Questions**: Use GitHub Discussions for help
- **Be Specific**: Include hardware details, code versions, error messages

### Helping Others
- **Answer Questions**: Share your knowledge and experience
- **Test Pull Requests**: Help validate changes on different hardware
- **Improve Documentation**: Fix typos, add examples, clarify instructions
- **Share Examples**: Show how you're using the system

## Recognition

Contributors are recognized in:
- **README.md**: Major contributors
- **CHANGELOG.md**: Specific contributions
- **Release Notes**: Feature credits
- **Documentation**: Tutorial and guide authors

Thank you for contributing to the ESP32 ATEM Tally System! Your contributions help make professional broadcast tools accessible to everyone.
