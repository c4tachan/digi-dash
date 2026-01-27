# Binary Gauge Format Specification

## Overview

Binary gauge files contain all data needed to render a complete gauge on either the ESP32 or Linux simulator. Files are size-optimized and designed for fast loading from flash/storage.

## File Structure

```
[HEADER]
[PATH DATA]
[ANIMATION DATA]
[PID BINDING DATA]
[FONT REFERENCES]
[METADATA]
```

## Header (128 bytes)

```c
struct GaugeHeader {
    uint32_t magic;           // 0x44474147 ("DGAG")
    uint32_t version;         // Currently 1
    uint32_t width;           // Display width in pixels
    uint32_t height;          // Display height in pixels
    uint32_t path_data_size;  // Bytes of path data
    uint32_t animation_count; // Number of animations
    uint32_t binding_count;   // Number of PID bindings
    uint32_t reserved[18];    // For future use
};
```

## Path Data Section

Vector paths encoded as compact cubic Bézier curves:

```c
struct BezierPath {
    uint16_t point_count;    // Number of control points
    uint32_t color;          // ARGB color
    float stroke_width;      // Width in pixels (0 = filled)
    float control_points[point_count * 2];  // X, Y pairs
};
```

## Animation Data Section

Keyframe animation definitions:

```c
struct Animation {
    uint32_t id;
    uint16_t target_element;  // Index of path/text to animate
    uint16_t property;        // ROTATION, POSITION, OPACITY, etc.
    uint32_t duration_ms;
    uint8_t easing_type;      // LINEAR, EASE_IN, etc.
    uint8_t loop;
};
```

## PID Binding Data Section

Maps OBD2 PIDs to gauge elements:

```c
struct PIDBinding {
    uint32_t pid_id;           // OBD2 PID code
    uint16_t target_element;   // Index of element to update
    uint8_t binding_type;      // ROTATION, TEXT, COLOR, etc.
    float min_value;
    float max_value;
    float scale;
    float offset;
};
```

## Font References

List of required bitmap fonts with variants:

```c
struct FontReference {
    uint8_t font_id;
    char font_name[64];        // "NotoSans-24pt"
    uint8_t is_monospace;
};
```

## Format Constraints

- All multi-byte integers are little-endian
- Floating point values use IEEE 754 single precision
- Color values are ARGB (A in high byte)
- String lengths are Pascal-style (length prefix)

## Validation

Tools should verify:
- Magic number matches
- Version is supported
- Data sections don't overlap
- All referenced elements exist
- PID IDs are valid OBD2 codes
