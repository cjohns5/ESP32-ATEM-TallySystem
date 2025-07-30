# Hardware Setup Guide

## Overview
This guide covers the hardware setup for the ESP32 ATEM Tally System, including wiring diagrams, component specifications, and assembly instructions.

## Components List

### Bridge Device (1 required)
- **ESP32-WROOM-32** development board
- **USB Cable** (USB-A to Micro-USB or USB-C)
- **PC/Laptop** with Ethernet connection to ATEM

### Tally Light (per camera)
- **ESP32-WROOM-32** development board  
- **RGB LED** (common cathode) OR **WS2812 NeoPixel**
- **3x 220Ω Resistors** (for RGB LED only)
- **Battery Pack** (18650 Li-ion recommended)
- **Enclosure/Housing** (optional but recommended)
- **Jumper Wires** and **Breadboard/PCB**

## Wiring Diagrams

### Bridge Wiring
The bridge device requires minimal wiring - just USB connection to PC:

```
PC USB Port ──── ESP32 Bridge (USB Port)
                     │
                 Power + Data
```

### Tally Light Wiring (RGB LED)

```
ESP32 Pin      Component
─────────────────────────────
GPIO 25    ──[220Ω]── Red LED ──┐
GPIO 26    ──[220Ω]── Green LED ├── Common GND
GPIO 27    ──[220Ω]── Blue LED ──┘
GND        ────────── LED Common Cathode
3.3V       ────────── Pull-up resistors (if needed)
```

### Tally Light Wiring (NeoPixel Alternative)

```
ESP32 Pin      NeoPixel
─────────────────────────
GPIO 25    ──── Data In
3.3V       ──── VCC
GND        ──── GND
```

## Power Requirements

### Bridge Device
- **Power Source**: USB from PC (5V, ~500mA)
- **Consumption**: ~150-200mA typical
- **USB Tethering**: Provides network connectivity

### Tally Light
- **Operating Voltage**: 3.3V (ESP32 internal regulator)
- **Input Voltage**: 3.6V - 6V (via Vin pin)
- **Consumption**: 
  - Idle: ~50mA
  - Active: ~100-150mA (depending on LED brightness)
  - Peak: ~200mA (during transmission)

### Battery Recommendations

**18650 Li-ion Battery (Recommended)**
- Capacity: 2500-3500mAh
- Voltage: 3.7V nominal
- Runtime: 8-12 hours typical
- Rechargeable: Yes

**AA Battery Pack (4x AA)**
- Capacity: 2000-2500mAh
- Voltage: 6V (1.5V x 4)
- Runtime: 6-10 hours
- Rechargeable: Optional (NiMH recommended)

## Assembly Instructions

### Bridge Assembly
1. **Connect USB cable** to ESP32 development board
2. **Connect other end to PC** USB port
3. **Enable USB tethering** on PC
4. **Mount in enclosure** (optional)

### Tally Light Assembly

#### Step 1: Prepare Components
- ESP32 development board
- RGB LED or NeoPixel
- Resistors (if using RGB LED)
- Battery pack
- Jumper wires

#### Step 2: Solder Connections (RGB LED)
```
LED Pin    →    ESP32 Pin
Red Anode  →    GPIO 25 (via 220Ω resistor)
Green Anode →   GPIO 26 (via 220Ω resistor)  
Blue Anode →    GPIO 27 (via 220Ω resistor)
Cathode    →    GND
```

#### Step 3: Power Connections
```
Battery +  →    ESP32 Vin (or 3.3V if using 3.3V battery)
Battery -  →    ESP32 GND
```

#### Step 4: Testing
1. Upload tally light code
2. Power on device
3. Verify LED colors:
   - Orange blinking = Searching for bridge
   - Blue = Connected to bridge
   - Green/Red = Receiving tally states

## Enclosure Design

### Bridge Enclosure
- **Size**: Small project box (80x60x30mm)
- **Mounting**: Desktop or rack-mountable
- **Ventilation**: Minimal heat generation
- **Access**: USB port accessible

### Tally Light Enclosure
- **Size**: Handheld (100x60x40mm recommended)
- **Mounting**: Camera mounting options
- **Visibility**: Clear LED window
- **Access**: Power switch, charging port
- **Durability**: Impact resistant for field use

### Recommended Enclosures
- **Hammond 1591XXBK** series (general purpose)
- **Pelican 1010** (protective, waterproof)
- **Custom 3D printed** (specific mounting needs)

## Cable Management

### Bridge
- **USB Cable**: High-quality, ferrite core recommended
- **Length**: 1-3 meters typical
- **Strain Relief**: Secure at both ends

### Tally Light
- **Internal Wiring**: Short, secure connections
- **Battery Leads**: Appropriate gauge for current
- **LED Connections**: Twisted pair for noise immunity

## Testing Procedures

### Pre-Assembly Testing
1. **ESP32 Programming**: Verify code upload works
2. **LED Testing**: Check all colors work correctly
3. **Power Testing**: Verify voltage levels
4. **Range Testing**: Check BLE communication distance

### Post-Assembly Testing
1. **Power-On Test**: Verify boot sequence
2. **Connection Test**: Pair with bridge
3. **Tally Test**: Verify state changes
4. **Battery Test**: Check runtime
5. **Range Test**: Verify operating distance

## Troubleshooting Hardware Issues

### LED Not Working
- Check wiring connections
- Verify resistor values
- Test LED with multimeter
- Check GPIO pin assignments in code

### Power Issues
- Verify battery voltage
- Check power connections
- Measure current consumption
- Test with known good power supply

### Connection Issues
- Check antenna placement (keep clear of metal)
- Verify BLE is not disabled in code
- Test at closer range first
- Check for RF interference

### Mechanical Issues
- Ensure secure mounting
- Check for loose connections
- Verify enclosure fit
- Test all switches/buttons

## Performance Optimization

### Signal Quality
- Keep antennas clear of metal enclosures
- Use proper impedance matching
- Minimize EMI sources
- Optimize antenna orientation

### Power Efficiency
- Use efficient LED drivers
- Implement sleep modes
- Optimize transmission intervals
- Use low-power ESP32 modes

### Reliability
- Use quality components
- Implement proper strain relief
- Add ESD protection
- Use conformal coating in harsh environments

## Safety Considerations

### Electrical Safety
- Use proper fusing
- Avoid exposed high-voltage connections
- Implement overcurrent protection
- Use appropriate wire gauges

### Battery Safety
- Use protection circuits
- Avoid overcharging
- Monitor temperature
- Use quality battery cells

### EMI/RFI Compliance
- Use shielded enclosures if required
- Filter power supplies
- Meet local regulatory requirements
- Test in target environment

## Maintenance

### Regular Checks
- Battery voltage and capacity
- Connection integrity
- LED brightness and color accuracy
- Enclosure condition

### Replacement Schedule
- Batteries: 1-2 years (depending on usage)
- LEDs: 5+ years typical
- ESP32: 10+ years with proper care

This hardware setup provides a reliable, professional-grade tally system suitable for broadcast and production environments.
