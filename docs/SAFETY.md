# Safety and Compliance Guide

**Version:** 1.0  
**Date:** January 16, 2026

## 1. Legal and Regulatory Compliance

### 1.1 OBD-II Standards
This software implements the following standards:
- **SAE J1979** - OBD-II Standard
- **ISO 15765-4** - CAN Protocol for OBD
- **ISO 14230-4** - KWP2000 Protocol
- **ISO 9141-2** - OBD Protocol

**Compliance Statement**: Digi-Dash is designed to read vehicle diagnostic data only and does not modify engine parameters or emission control systems.

### 1.2 Data Privacy
- System collects real-time vehicle telemetry data
- Data is processed locally and not transmitted externally (except via user display)
- No data is stored persistently without explicit user configuration
- Users must comply with local privacy regulations regarding vehicle data

### 1.3 Vehicle Warranty
⚠️ **WARNING**: Adding non-OEM diagnostic equipment to a vehicle may void manufacturer warranty. Users assume full responsibility for any warranty implications.

## 2. Read-Only Operation

### 2.1 Allowed OBD-II Modes
Digi-Dash **ONLY** implements read-only OBD-II modes:
- **Mode 01**: Current data (ALLOWED)
- **Mode 02**: Freeze frame data (ALLOWED)
- **Mode 09**: Vehicle information (ALLOWED)

### 2.2 Prohibited OBD-II Modes
The following modes are **EXPLICITLY DISABLED** for safety:
- **Mode 04**: Clear diagnostic trouble codes (DISABLED)
- **Mode 06**: Test results (DISABLED)
- **Mode 07**: Pending trouble codes (DISABLED)
- **Mode 08**: Enable special testing (DISABLED)
- **Mode 10**: Enable actuators (DISABLED)
- **Mode 20**: Return to normal (DISABLED)

Any attempt to execute prohibited modes will be rejected by the system.

### 2.3 Command Verification
All OBD-II commands are logged before execution:
```
[OBD-II] Executing command: 010C (Engine RPM) on CAN bus
[OBD-II] Response: 41 0C 12 34 (Valid)
```

Users can audit command history in logs.

## 3. Vehicle Safety

### 3.1 Non-Intrusive Connection
- Digi-Dash connects via OBD-II diagnostic port (read-only)
- No electrical modifications to vehicle required
- No modification of vehicle firmware or parameters
- System is completely isolated from vehicle control systems

### 3.2 OBD-II Bus Isolation
- System operates as passive listener on OBD-II bus
- Does not transmit control commands
- Gracefully disconnects without affecting vehicle operation
- Multiple diagnostic readers can operate simultaneously

### 3.3 Fail-Safe Design
- Loss of connection does not affect vehicle operation
- Display errors do not cascade to vehicle systems
- Configuration errors produce user warnings, not silent failures
- System handles malformed data gracefully

## 4. Driver Safety

### 4.1 Display Best Practices
- **DO**: Place display in vehicle headrest or center console
- **DO**: Configure display to visible but not obstructing
- **DON'T**: Obstruct driver's view of road
- **DON'T**: Require driver interaction while driving
- **DON'T**: Display overly bright/flashing content

### 4.2 Alert Safety
- High-contrast alert colors (red, yellow) for visibility
- Non-audible alerts only (visual flashing)
- Alerts can be disabled if distracting
- Driver retains full responsibility for monitoring

### 4.3 Distractions
- All configuration must be performed when vehicle is stationary
- Display provides baseline vehicle information only
- Driver should not rely solely on display for vehicle operation
- Vehicle gauges remain the primary monitoring method

## 5. Data Handling and Security

### 5.1 What Data is Collected?
Only OBD-II PIDs configured in `display.json`:
- Engine speed (RPM)
- Vehicle speed (km/h)
- Coolant temperature (°C)
- Oxygen sensor data (optional)
- Fuel pressure (optional)

### 5.2 What Data is NOT Collected?
- Vehicle identification number (VIN)
- GPS location (unless separately configured)
- Vehicle owner information
- Diagnostic trouble codes (by default)

### 5.3 Data Storage
- No persistent storage of vehicle data
- Data is displayed in real-time only
- Optional logging to local files only
- No cloud transmission or external sharing

### 5.4 User Privacy
- All data remains under user's control
- No telemetry or analytics
- No third-party integrations
- Open-source code allows independent verification

## 6. Communication Security

### 6.1 OBD-II Bus Security
- CAN bus is inherently open (no encryption)
- KWP2000 protocol does not support encryption
- Assume all data on OBD-II bus is visible to all connected devices
- Do not transmit sensitive information via OBD-II

### 6.2 Serial Connection Security
- Bluetooth connections should use PIN protection
- USB connections are physical only
- Serial protocols do not include authentication
- Secure your vehicle/device to prevent unauthorized access

## 7. Testing and Validation

### 7.1 Vehicle Testing Checklist
Before using Digi-Dash on a production vehicle:

- [ ] Test on vehicle without driving (parking lot)
- [ ] Verify all display values are realistic
- [ ] Verify no warning lights illuminate on dashboard
- [ ] Verify vehicle responds normally to controls
- [ ] Test disconnection (pull OBD-II adapter) - vehicle should operate normally
- [ ] Check for error codes after testing
- [ ] Drive at low speed before highway driving
- [ ] Verify no thermal issues after 30min operation

### 7.2 Protocol Testing
Each protocol implementation must be tested on actual target vehicles:
- **CAN** (ISO 15765-4): 2008+ vehicles
- **KWP2000** (ISO 14230-4): 1996-2010 vehicles  
- **ISO 9141-2**: 1994-2000 vehicles

### 7.3 Compatibility Matrix
| Vehicle | Year | Make/Model | Protocol | Status | Notes |
|---------|------|-----------|----------|--------|-------|
| Test 1 | 2012 | Mazda 2 | CAN | Tested | Working |
| Test 2 | 2002 | Toyota MR2 Spyder | KWP2000 | Pending | Planned test |

## 8. Troubleshooting Guide

### 8.1 Connection Issues
**Problem**: OBD-II adapter not detected

**Solutions**:
1. Check device path: `ls -la /dev/tty*`
2. Verify permissions: `sudo ls -la /dev/ttyUSB0`
3. Check adapter is powered and blinking
4. Restart Bluetooth pairing if wireless

### 8.2 Protocol Errors
**Problem**: "No response from vehicle"

**Solutions**:
1. Verify correct protocol in `display.json`
2. Check adapter is OBD-II compliant (ELM327 compatible)
3. Ensure vehicle is running and OBD-II port is powered
4. Try different baudrate in configuration

### 8.3 Data Validation Errors
**Problem**: Values are unrealistic (RPM 30000, Speed 999)

**Solutions**:
1. Check PID formula in configuration
2. Verify OBD-II response is valid
3. Try different vehicle systems (may have multi-protocol support)
4. Enable verbose logging: `LOGLEVEL=DEBUG ./digi-dash-simulator`

## 9. Incident Reporting

If you experience any of the following:
- Vehicle behaves abnormally during diagnostic session
- Warnings lights illuminate unexpectedly
- OBD-II bus becomes unresponsive
- Data appears corrupted or invalid

**Please report to**: [Create GitHub issue with details]

Include:
- Vehicle make, model, year
- OBD-II protocol used
- Exact commands that triggered issue
- Error messages from logs
- Steps to reproduce

## 10. Liability and Disclaimers

**DISCLAIMER**: This software is provided "AS-IS" without warranty. Users assume full responsibility for:
- Vehicle safety and operation
- Compliance with local laws and regulations
- Any damage resulting from misuse or misconfiguration
- Any warranty implications from adding non-OEM equipment
- Data privacy and security

The authors and contributors assume no liability for:
- Vehicle damage or malfunction
- Personal injury or property damage
- Loss of vehicle warranty
- Regulatory violations or fines
- Data breaches or privacy violations

---

**Document Control**: Maintained in `/docs/SAFETY.md`

**Last Updated**: January 16, 2026  
**Next Review**: Q2 2026 (or after each vehicle test)
