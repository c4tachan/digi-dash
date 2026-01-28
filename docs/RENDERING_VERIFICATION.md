# Rendering Verification

## Summary

The vector rendering engine is working correctly on both the simulator and ESP32-S3 firmware. Initial concerns about "missing alpha channel" were due to sampling pixels outside the filled region.

## Verification Results

### Test Case: Circle Rendering
- **Gauge**: dashboard_tiny.gauge (720x720, black filled circle centered at 360,360 r=360)
- **Render mode**: Tile-based rendering (720x60 tiles)
- **First tile**: y=0 to y=59

### Simulator Test (SDL Display)
```
Loaded gauge: 720x720
Pixel write [0]: offset=1440 y=0 x=360 RGBA(0,0,0,255) ✅
Pixel write [1]: offset=4212 y=1 x=333 RGBA(0,0,0,255) ✅
```

### ESP32-S3 QEMU Test
```
I (833) gauge_scene: Path 0: FILLED RGBA(0,0,0,255) ✅
draw_filled_path: bbox y=[0.0,720.0] clipped=[0,59] RGBA(0,0,0,255) tile_h=60 ✅
I (1203) digi-dash: Frame 1: 16071 pixels (37.2%) ✅
```

## Key Findings

### Alpha Channel is Correct
- ✅ Colors loaded from binary gauge: RGBA(0,0,0,**255**)
- ✅ Colors passed to renderer: RGBA(0,0,0,**255**)
- ✅ Pixels written with alpha=255 at correct locations

### Rendering Coverage
- **Expected**: Top slice of circle (y=0 to y=59) from a 720px diameter circle
- **Actual**: 16,071 pixels (37.2% of 43,200-pixel tile)
- **Analysis**: Correct! The top edge of a circle has less coverage than the middle

### Pixel Locations
- Pixel[0] at coordinates (0,0): **Outside circle** → RGBA(0,0,0,0) ✅ Correct
- Pixel at coordinates (360,0): **Top of circle** → RGBA(0,0,0,255) ✅ Correct
- First pixel written: offset=1440 = y=0, x=360 ✅ Matches circle geometry

## Rendering Pipeline Validation

### 1. Binary Gauge Loading
```cpp
// gauge_scene.cpp line ~85
bezier_path.color = (path.fill.color.a << 24) |
                   (path.fill.color.r << 16) |
                   (path.fill.color.g << 8) |
                   path.fill.color.b;
```
✅ Alpha stored in bits [31:24]

### 2. Color Unpacking
```cpp
// vector_renderer.cpp line ~36
uint8_t r = (color >> 16) & 0xFF;
uint8_t g = (color >> 8) & 0xFF;
uint8_t b = (color >> 0) & 0xFF;
uint8_t a = (color >> 24) & 0xFF;  // Alpha extracted correctly
```
✅ Alpha extracted from correct bit position

### 3. Pixel Writing
```cpp
// vector_renderer.cpp line ~115
buffer[pixel_offset + 0] = r;
buffer[pixel_offset + 1] = g;
buffer[pixel_offset + 2] = b;
buffer[pixel_offset + 3] = a;  // Alpha written to buffer
```
✅ All 4 channels written in RGBA order

## Tile Rendering Math

### Circle Geometry
- Center: (360, 360)
- Radius: 360
- Equation: $(x-360)^2 + (y-360)^2 = 360^2$

### First Tile Coverage (y=0 to y=59)
At y=0: $x = 360$ (single point at top)
At y=30: $(x-360)^2 = 360^2 - 330^2 = 20,700$ → $x \in [216, 504]$ (288 pixels)
At y=59: $(x-360)^2 = 360^2 - 301^2 = 39,099$ → $x \in [162, 558]$ (396 pixels)

**Measured coverage**: 16,071 pixels across 60 rows
**Average per row**: 268 pixels
**Expected**: Increasing from 0 to ~400 pixels per row ✅ Matches

## Conclusion

**No bugs found**. The rendering engine correctly:
1. Loads colors with alpha=255 from binary gauge
2. Unpacks ARGB uint32_t to separate RGBA channels
3. Writes all 4 channels to the framebuffer
4. Renders the expected pixel coverage for the geometry

The initial investigation was triggered by sampling pixel[0] which happens to be outside the filled circle region. Sampling pixels at the actual filled locations (e.g., x=360, y=0) confirms alpha=255 is present.

## Testing Notes

### To Run Simulator
```bash
cd /home/catachan/projects/digi-dash
./build/simulator/digi-dash-simulator
```

### To Run QEMU Firmware
```bash
cd /home/catachan/projects/digi-dash  
bash scripts/rebuild_gauge_and_run_qemu.sh
tail -f /tmp/qemu_render.log
```

### Expected Output
- Simulator: Visual circle on SDL window
- QEMU: Serial log showing "16071 pixels (37.2%)" and center pixel RGBA(0,0,0,255)

## Next Steps

- Implement display driver for actual LCD hardware
- Add remaining gauge paths (stroked paths for red/blue circles)
- Implement animation system for dynamic gauges
- Test with real OBD2 data binding

---
*Verified: January 27, 2026*
*Platforms: x86_64 simulator (SDL2), ESP32-S3 (QEMU)*
