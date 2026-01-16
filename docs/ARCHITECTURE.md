# Architecture and Design Document

**Version:** 1.0  
**Date:** January 16, 2026

## 1. System Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│                   DIGI-DASH SYSTEM                         │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  User Interface (LVGL + SDL2)                       │  │
│  │  - Arc Gauge Display                                │  │
│  │  - Speed/Temperature Display                        │  │
│  │  - Zone Indicators & Alerts                         │  │
│  └─────────────────────────────────────────────────────┘  │
│                          △                                  │
│                          │ Display Updates                 │
│                          │ (20 FPS)                        │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Dashboard Manager                                  │  │
│  │  - Layout & Element Management                      │  │
│  │  - Gauge Updates                                    │  │
│  │  - Event Handling                                   │  │
│  └─────────────────────────────────────────────────────┘  │
│                          △                                  │
│                          │ PID Data                        │
│                          │                                  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Data Source Abstraction                            │  │
│  │  ┌──────────────────┬──────────────────┐           │  │
│  │  │ Mock Data        │ OBD-II Data      │           │  │
│  │  │ Generator        │ Source           │           │  │
│  │  └──────────────────┴──────────────────┘           │  │
│  └─────────────────────────────────────────────────────┘  │
│                          △                                  │
│                          │                                  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  OBD-II Communication Layer                         │  │
│  │  - CAN Protocol                                     │  │
│  │  - KWP2000 Protocol                                 │  │
│  │  - ISO 9141-2 Protocol                              │  │
│  │  - Response Parsing & Validation                    │  │
│  └─────────────────────────────────────────────────────┘  │
│                          △                                  │
│                          │ Serial Communication          │  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  Hardware Layer                                     │  │
│  │  ┌──────────────────┬──────────────────┐           │  │
│  │  │ Vehicle OBD-II   │ ELM327 Adapter   │           │  │
│  │  │ Bus              │ (USB/Bluetooth)  │           │  │
│  │  └──────────────────┴──────────────────┘           │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 2. Component Descriptions

### 2.1 User Interface Layer
**Responsible for**: Display rendering, user interactions

**Components**:
- **Dashboard** (`dashboard.h/cpp`)
  - Manages single TFT display (480x320)
  - Creates and manages SDL2 window
  - Integrates LVGL graphics library
  - Handles input events (mouse, keyboard)

- **Gauge** (`gauge.h/cpp`)
  - Generic gauge widget supporting multiple types
  - Arc gauges with zones, zones boundaries, tick marks
  - Digital gauges for numeric display
  - Bar gauges for progress/temperature

### 2.2 Configuration Layer
**Responsible for**: Loading and parsing configuration

**Components**:
- **Config** (`config.h/cpp`)
  - Loads JSON configuration from `display.json`
  - Parses OBD-II protocol settings
  - Parses gauge definitions and zones
  - Parses alert thresholds
  - Validates all configuration data

### 2.3 Data Source Abstraction
**Responsible for**: Providing vehicle data regardless of source

**Base Class**: `DataSource` (abstract)
- Pure virtual `update()` method
- Pure virtual `getValue(pid_name)` method
- Pure virtual `getName()` method

**Implementations**:
- **MockData**: Generates sinusoidal test data
- **OBD2DataSource**: Reads from real vehicle via serial port

### 2.4 OBD-II Communication Layer
**Responsible for**: Protocol-specific vehicle communication

**Components**:
- **OBD2DataSource** (`obd2_data_source.h/cpp`)
  - Manages serial port connection
  - Sends OBD-II commands
  - Parses responses
  - Caches values and handles timeouts

**Protocol Support**:
- CAN (ISO 15765-4) @ 500 kbaud
- KWP2000 (ISO 14230-4) @ 10.4 kbaud
- ISO 9141-2 @ 10.4 kbaud

### 2.5 Gauge Factory
**Responsible for**: Creating correct gauge type from configuration

**Component**: `GaugeFactory` (`gauge_factory.h/cpp`)
- Reads gauge definition
- Creates appropriate gauge type (arc, digital, bar)
- Initializes zones, markers, alerts
- Returns fully configured Gauge instance

## 3. Data Flow

### 3.1 Startup Sequence

```
1. main.cpp
   ├─ Initialize SDL2
   ├─ Initialize LVGL
   ├─ Load display.json
   │  └─ Parse protocol, PIDs, gauges, display config
   ├─ Create Dashboard
   │  └─ Create SDL window
   │  └─ Initialize LVGL display
   │  └─ Create gauge elements via GaugeFactory
   ├─ Create DataSource (MockData or OBD2DataSource)
   │  └─ For OBD2: Open serial port, handshake with adapter
   └─ Enter main loop
```

### 3.2 Update Cycle (20 FPS)

```
1. DataSource::update()
   ├─ Query vehicle (OBD-II mode 01)
   └─ Parse response and store values

2. Dashboard::update()
   ├─ Get data from DataSource (RPM, Speed, Temp)
   ├─ Update each gauge element
   │  ├─ Gauge::updateArcGauge()
   │  │  ├─ Determine color based on zones
   │  │  ├─ Update arc position
   │  │  └─ Check alerts
   │  └─ Update labels
   └─ Build display dirty regions

3. LVGL::lv_timer_handler()
   ├─ Render dirty regions to LVGL buffer
   └─ Call displayFlushCallback()

4. Dashboard::displayFlushCallback()
   ├─ Lock SDL texture
   ├─ Copy LVGL buffer to SDL texture
   └─ Unlock texture

5. Dashboard::render()
   ├─ Clear SDL renderer
   ├─ Copy texture to renderer
   └─ Present to display
```

### 3.3 Configuration Data Flow

```
display.json
    ↓
Config::loadFromFile()
    ├─ Parse protocol settings
    ├─ Parse PID definitions
    ├─ Parse gauge definitions (with zones, markers, alerts)
    └─ Parse display layout
         ↓
    DashboardConfig object
         ↓
    main.cpp passes to:
    ├─ Dashboard(display_config, gauge_definitions)
    ├─ OBD2DataSource(pids)
```

## 4. Class Hierarchy

```
DataSource (abstract)
├── MockData
└── OBD2DataSource

Gauge (concrete, but polymorphic methods for gauge types)
├── createArcGauge()
├── createDigitalGauge()
└── createBarGauge()

Configuration Structures:
├── ProtocolConfig
├── PidConfig
├── Zone
├── Marker
├── Alert
├── GaugeDefinition
├── DashboardElement
├── DisplayConfig
└── DashboardConfig
```

## 5. Key Design Decisions

### 5.1 Abstraction via DataSource
**Decision**: Use abstract DataSource interface for both mock and real data
**Rationale**:
- Single update loop for testing and production
- Easy to switch between data sources without recompilation
- Enables testing with realistic mock data
- No hardcoded dependencies on specific data sources

### 5.2 Configuration-Driven Design
**Decision**: All gauge definitions and display layout in JSON config
**Rationale**:
- Zero code changes to modify gauges, zones, or alerts
- Non-programmers can customize display
- Portable across different vehicles with different PIDs
- Easy to version control different configurations

### 5.3 Single Display Architecture
**Decision**: Support only single TFT display (not multi-window)
**Rationale**:
- Simplified architecture for ESP32-S3 constraints
- Better performance (no window management overhead)
- Matches real-world use case (single dash display)
- Easier to optimize for small devices

### 5.4 Factory Pattern for Gauges
**Decision**: Use GaugeFactory to create gauges from definitions
**Rationale**:
- Decouples gauge creation from Dashboard
- Consistent initialization of all gauges
- Easy to add new gauge types
- Supports complex zone/alert setup

## 6. Memory Layout

### 6.1 Typical Memory Usage (ESP32-S3)
```
LVGL buffers:           ~3.7 MB (480x320 @ 4bpp partial)
Display/Dashboard:      ~500 KB
Gauge objects:          ~100 KB
Configuration:          ~50 KB
System/Other:           ~1 MB
────────────────────────────
Total:                  ~6 MB / 8 MB available
```

### 6.2 LVGL Buffer Strategy
- **Render mode**: Partial (LV_DISPLAY_RENDER_MODE_PARTIAL)
- **Buffer size**: (480 × 320 / 10) × 4 bytes = 614 KB each
- **Dual buffering**: Yes (buffer1 + buffer2)
- **Benefits**: Reduced memory footprint, efficient updates

## 7. Performance Characteristics

### 7.1 Display Refresh Rate
- **Target**: 20 FPS (50ms per frame)
- **Actual**: Typically 24 FPS in testing
- **Bottleneck**: OBD-II query time (100-500ms per PID)
- **Mitigation**: Asynchronous updates, multiple gauge displays

### 7.2 OBD-II Latency
```
Query Latency:
├─ Serial transmission: ~1ms
├─ Adapter processing: ~50ms
├─ Vehicle ECU processing: ~50ms
├─ Response transmission: ~1ms
└─ Total: 100-200ms typical (can vary)
```

### 7.3 Gauge Update Time
```
Per Update Cycle:
├─ LVGL rendering: ~5ms (partial update)
├─ SDL buffer copy: ~2ms
├─ SDL render: ~3ms
└─ Total display overhead: ~10ms
```

## 8. Extensibility Points

### 8.1 Adding New Gauge Types
1. Add gauge type to GaugeType enum in config.h
2. Implement `createXyzGauge()` in Gauge class
3. Update GaugeFactory to instantiate new type
4. Add zone/alert handling to `updateXyzGauge()`

### 8.2 Adding New OBD-II Protocols
1. Extend OBD2DataSource to handle new protocol
2. Add protocol type to ProtocolConfig
3. Implement protocol-specific handshake and parsing
4. Test with target vehicle

### 8.3 Adding New Data Sources
1. Create new class inheriting from DataSource
2. Implement `update()`, `getValue()`, `getName()`
3. Add factory logic in main.cpp
4. Update command-line argument handling

## 9. Testing Strategy

### 9.1 Unit Tests
- Configuration parsing (valid/invalid JSON)
- PID formula evaluation
- OBD-II response parsing
- Data validation and range checking

### 9.2 Integration Tests
- Dashboard creation and rendering
- Gauge updates with various data values
- Alert triggering and threshold logic
- Configuration loading and gauge creation

### 9.3 Vehicle Tests
- Real OBD-II communication
- CAN protocol on 2012 Mazda 2
- KWP2000 protocol on 2002 Toyota MR2
- Multiple adapter types (USB, Bluetooth)

## 10. Build System

**Build Tool**: CMake 3.10+
**Compiler**: GCC/Clang with C++17 support
**Dependencies**:
- LVGL v9.4.0 (graphics library)
- SDL2 (window and event handling)
- nlohmann/json (configuration parsing)
- Optional: Boost, GoogleTest

---

**Document Control**: Maintained in `/docs/ARCHITECTURE.md`  
**Last Updated**: January 16, 2026
