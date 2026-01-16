# Digi-Dash Requirements Document

**Version:** 1.0  
**Date:** January 16, 2026  
**Status:** Draft

## 1. Executive Summary

Digi-Dash is an automotive dashboard display system designed for embedded systems (ESP32-S3) that interfaces with vehicle OBD-II (On-Board Diagnostics) systems to display real-time engine and vehicle data on a TFT LCD display.

## 2. Functional Requirements

### 2.1 Data Acquisition
- **FR-2.1.1**: System shall support CAN (ISO 15765-4) protocol for modern vehicles (2008+)
- **FR-2.1.2**: System shall support KWP2000 (ISO 14230-4) protocol for legacy vehicles (1996-2010)
- **FR-2.1.3**: System shall support ISO 9141-2 protocol for older vehicles
- **FR-2.1.4**: System shall allow protocol configuration via `display.json`
- **FR-2.1.5**: System shall read OBD-II PIDs as specified in configuration
- **FR-2.1.6**: System shall support standard OBD-II commands (Mode 01, 02, 09)

### 2.2 Display and Visualization
- **FR-2.2.1**: System shall display RPM gauge with arc visualization
- **FR-2.2.2**: System shall display current speed value (3-digit precision)
- **FR-2.2.3**: System shall display coolant temperature with arc indicator
- **FR-2.2.4**: System shall support color-coded zones (green/yellow/red) for RPM ranges
- **FR-2.2.5**: System shall display zone boundary markers on gauge
- **FR-2.2.6**: System shall display tick marks at 500 RPM intervals (minor) and 1000 RPM (major)
- **FR-2.2.7**: Display shall refresh at minimum 20 FPS
- **FR-2.2.8**: System shall support 480x320 display resolution (standard for TFT LCD)

### 2.3 Alerts and Safety
- **FR-2.3.1**: System shall trigger visual alerts when RPM exceeds warning threshold
- **FR-2.3.2**: System shall trigger visual alerts when temperature exceeds threshold
- **FR-2.3.3**: Alert shall use flashing/color change for visual indication
- **FR-2.3.4**: Alert conditions shall be configurable via `display.json`
- **FR-2.3.5**: System shall log all alert events with timestamp

### 2.4 Configuration
- **FR-2.4.1**: System shall load all configuration from `display.json` file
- **FR-2.4.2**: Configuration shall specify OBD-II protocol type and baudrate
- **FR-2.4.3**: Configuration shall specify PID definitions and formulas
- **FR-2.4.4**: Configuration shall specify gauge definitions, zones, alerts
- **FR-2.4.5**: Configuration shall support comments and descriptions

### 2.5 Data Source Support
- **FR-2.5.1**: System shall support mock data generator for testing/simulation
- **FR-2.5.2**: System shall support real OBD-II data via serial port connection
- **FR-2.5.3**: Data source shall be selectable via command-line argument `--obd2 <device>`
- **FR-2.5.4**: System shall gracefully handle data source disconnection

## 3. Non-Functional Requirements

### 3.1 Performance
- **NFR-3.1.1**: Display refresh rate shall be >= 20 Hz (50ms update interval)
- **NFR-3.1.2**: OBD-II query shall not exceed 100ms per PID
- **NFR-3.1.3**: Configuration loading shall complete within 1 second
- **NFR-3.1.4**: Memory footprint shall not exceed 50MB on target hardware

### 3.2 Reliability
- **NFR-3.2.1**: System shall handle malformed OBD-II responses gracefully
- **NFR-3.2.2**: System shall timeout OBD-II requests after 2 seconds
- **NFR-3.2.3**: System shall continue operating on partial PID failures
- **NFR-3.2.4**: Display shall remain responsive during OBD-II communication

### 3.3 Compatibility
- **NFR-3.3.1**: System shall run on ESP32-S3 microcontroller
- **NFR-3.3.2**: System shall work with standard ELM327-based OBD-II adapters
- **NFR-3.3.3**: System shall support Bluetooth and USB serial connections
- **NFR-3.3.4**: System shall compile with GCC/Clang C++17 support

### 3.4 Usability
- **NFR-3.4.1**: System shall provide `--help` command-line option
- **NFR-3.4.2**: System shall log startup progress to console
- **NFR-3.4.3**: System shall indicate data source status on startup
- **NFR-3.4.4**: Configuration errors shall produce clear diagnostic messages

## 4. Safety and Compliance Requirements

### 4.1 Data Safety
- **SAF-4.1.1**: System shall not modify vehicle data or send unknown commands
- **SAF-4.1.2**: System shall only read OBD-II data (Mode 01, 02, 09 only)
- **SAF-4.1.3**: System shall not execute OBD-II Mode 04 (clear DTC) commands
- **SAF-4.1.4**: System shall not execute OBD-II Mode 10/20 (enable special testing)
- **SAF-4.1.5**: All OBD-II commands sent shall be logged for audit trail

### 4.2 Driver Safety
- **SAF-4.2.1**: Display shall not obstruct driver's view of road
- **SAF-4.2.2**: Display shall not require interaction while vehicle is in motion
- **SAF-4.2.3**: Alert notifications shall use high-contrast colors for visibility
- **SAF-4.2.4**: System shall not require configuration changes during driving

### 4.3 Vehicle Safety
- **SAF-4.3.1**: System shall not interfere with vehicle's OBD-II bus or ECU operation
- **SAF-4.3.2**: System shall use read-only OBD-II access
- **SAF-4.3.3**: System shall disconnect cleanly without leaving bus in undefined state
- **SAF-4.3.4**: System shall handle simultaneous connections from multiple OBD-II readers

### 4.4 Legal Compliance
- **SAF-4.4.1**: System shall comply with OBD-II standard (SAE J1979)
- **SAF-4.4.2**: System shall not defeat emission control systems
- **SAF-4.4.3**: System shall comply with local data privacy regulations
- **SAF-4.4.4**: Documentation shall include warnings about vehicle manufacturer warranties

## 5. Interface Requirements

### 5.1 Serial Communication
- **IF-5.1.1**: System shall support standard RS-232 serial communication
- **IF-5.1.2**: System shall support variable baudrates (9600 to 1M baud)
- **IF-5.1.3**: Serial communication shall use 8 data bits, 1 stop bit, no parity
- **IF-5.1.4**: System shall support both hardware and software serial ports

### 5.2 Configuration Interface
- **IF-5.2.1**: Configuration shall be in JSON format
- **IF-5.2.2**: System shall validate JSON syntax on load
- **IF-5.2.3**: System shall report JSON parsing errors with line numbers
- **IF-5.2.4**: Configuration shall support UTF-8 encoding

### 5.3 Command-Line Interface
- **IF-5.3.1**: `./digi-dash-simulator --help` shall display usage information
- **IF-5.3.2**: `./digi-dash-simulator --mock` shall enable mock data mode
- **IF-5.3.3**: `./digi-dash-simulator --obd2 <device>` shall enable OBD-II mode
- **IF-5.3.4**: System shall exit with appropriate exit codes (0=success, 1=error)

## 6. Data Requirements

### 6.1 PID Support
- **DATA-6.1.1**: System shall support configurable OBD-II PID definitions
- **DATA-6.1.2**: PID definitions shall include command, formula, and unit
- **DATA-6.1.3**: System shall support arithmetic formulas for PID decoding
- **DATA-6.1.4**: Minimum supported PIDs: RPM (010C), Speed (010D), Coolant Temp (0105)

### 6.2 Data Logging
- **DATA-6.2.1**: System shall log errors to stderr
- **DATA-6.2.2**: System shall log configuration loading to stdout
- **DATA-6.2.3**: System shall log OBD-II connection status
- **DATA-6.2.4**: System shall provide optional verbose logging mode

### 6.3 Data Validation
- **DATA-6.3.1**: System shall validate PID values are within expected ranges
- **DATA-6.3.2**: System shall handle out-of-range values gracefully
- **DATA-6.3.3**: System shall skip display of invalid data values
- **DATA-6.3.4**: System shall log validation failures for diagnostics

## 7. Testing Requirements

### 7.1 Unit Testing
- **TEST-7.1.1**: OBD-II response parsing shall be unit tested
- **TEST-7.1.2**: Configuration loading shall be unit tested
- **TEST-7.1.3**: PID formula evaluation shall be unit tested
- **TEST-7.1.4**: All parsing functions shall handle malformed input

### 7.2 Integration Testing
- **TEST-7.2.1**: System shall be tested with Mazda 2 (CAN protocol, 2012)
- **TEST-7.2.2**: System shall be tested with Toyota MR2 Spyder (KWP2000, 2002)
- **TEST-7.2.3**: System shall be tested with multiple OBD-II adapter types

### 7.3 Simulation Testing
- **TEST-7.3.1**: Mock data generator shall produce realistic sinusoidal patterns
- **TEST-7.3.2**: Mock data shall cycle through all gauge zones
- **TEST-7.3.3**: Mock data shall trigger all configured alerts

## 8. Documentation Requirements

### 8.1 Code Documentation
- **DOC-8.1.1**: All public functions shall have Doxygen-style comments
- **DOC-8.1.2**: Complex algorithms shall be documented with pseudocode
- **DOC-8.1.3**: Build instructions shall be provided in README.md

### 8.2 User Documentation
- **DOC-8.2.1**: Configuration guide shall be provided
- **DOC-8.2.2**: Supported vehicle list shall be maintained
- **DOC-8.2.3**: Troubleshooting guide shall be provided
- **DOC-8.2.4**: Safety warnings shall be prominently displayed

### 8.3 Technical Documentation
- **DOC-8.3.1**: System architecture shall be documented
- **DOC-8.3.2**: OBD-II protocol implementation details shall be documented
- **DOC-8.3.3**: Configuration schema shall be documented
- **DOC-8.3.4**: Data flow diagrams shall be included

## 9. Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-16 | System | Initial requirements document |

## 10. Appendix: Supported Vehicles

### Tested Vehicles
- **2012 Mazda 2** - CAN Protocol, 500 kbaud
- **2002 Toyota MR2 Spyder** - KWP2000 Protocol, 10.4 kbaud (Planned)

### Vehicle Protocol Matrix
| Year Range | Manufacturers | Protocol | Baudrate |
|------------|---------------|----------|----------|
| 2008+ | Most | CAN ISO 15765-4 | 500k |
| 1996-2010 | Toyota, Honda, Nissan | KWP2000 | 10.4k |
| 1994-2000 | Various | ISO 9141-2 | 10.4k |

## 11. Legal Disclaimers

- This software is provided for diagnostic and display purposes only
- Users are responsible for ensuring compliance with local laws regarding vehicle modification
- Modifying vehicle software or adding non-OEM equipment may void warranty
- The authors assume no liability for misuse or vehicle damage
- Users must ensure the system does not interfere with vehicle safety systems

---

**Document Control**: This document is maintained in the `/docs` folder of the Digi-Dash repository.
