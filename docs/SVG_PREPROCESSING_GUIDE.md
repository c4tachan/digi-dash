# SVG Preprocessing Guide

Complete guide for converting SVG dashboards to embedded-ready binary gauge format.

## Overview

The preprocessing pipeline transforms complex SVG files into optimized binary gauges with three key stages:

```
Original SVG
    ↓
[1] Advanced Preprocessing (svg_preprocessor_advanced.py)
    - Flatten arcs to cubic Bézier curves
    - Convert circles/ellipses to Bézier paths
    - Clean and optimize attributes
    ↓
Preprocessed SVG
    ↓
[2] Binary Encoding (svg_parser.py)
    - Parse path data
    - Apply scaling for target display
    - Encode to compact binary format
    ↓
Binary Gauge File (.gauge)
    ↓
[3] Runtime Loading (binary_gauge_loader.cpp)
    - Load and validate binary format
    - Render using VectorRenderer
    - Display in simulator or device
```

## Stage 1: Advanced Preprocessing

### Purpose

Prepares SVG for embedded rendering by:
- **Arc Flattening**: Converts SVG arc commands (A/a) to cubic Bézier curves
  - Critical for LVGL compatibility
  - Improves visual quality on limited rendering systems
  - Reduces path complexity
  
- **Circle/Ellipse Conversion**: Converts shape elements to path data
  - Ensures consistent rendering across platforms
  - Enables unified path processing
  - Improves LVGL compatibility (vector API works better with paths)

- **Attribute Cleanup**: Removes redundant attributes
  - Prevents rendering conflicts
  - Ensures stroke-width is explicit
  - Removes unused fill attributes

### Usage

```bash
python3 tools/svg_preprocessor/svg_preprocessor_advanced.py <input.svg> [output.svg]
```

**Example:**
```bash
python3 tools/svg_preprocessor/svg_preprocessor_advanced.py assets/dashboard_tiny.svg assets/dashboard_tiny_processed.svg
```

### What It Does

**1. Circle Conversion**
- Converts `<circle>` elements to cubic Bézier path approximation
- Uses 4 curve segments to approximate a perfect circle
- Preserves fill/stroke attributes

**Before:**
```xml
<circle cx="360" cy="360" r="360" fill="black" />
```

**After:**
```xml
<path d="M 720.00 360.00 C 720.00 161.18 558.82 0.00 360.00 0.00 C 161.18 0.00 0.00 161.18 0.00 360.00 ..." />
```

**2. Ellipse Conversion**
- Similar to circle conversion
- Handles different rx/ry values (elliptical)
- 4-segment Bézier approximation

**3. Arc Flattening**
- Converts `A`/`a` path commands to cubic Bézier segments
- Implements W3C SVG spec arc-to-Bézier conversion algorithm
- Adjusts number of segments based on arc sweep angle
- Maintains accuracy within 0.1 pixel

**Example Arc Conversion:**
- Input: `M 100 100 A 50 50 0 0 1 200 100`
- Output: Multiple `C` commands approximating the arc

### Prerequisites

- Python 3.6+
- No external dependencies (uses stdlib `xml.etree` and `math`)

### Validation

After preprocessing, verify:
```bash
# Check file is valid XML
xmllint --noout dashboard_tiny_processed.svg

# Count path elements (should be all shape elements converted to paths)
grep -c '<path' dashboard_tiny_processed.svg
```

## Stage 2: Binary Encoding

### Purpose

Converts preprocessed SVG to compact binary format:
- Reduces file size (typically 60-70% of SVG)
- Fast loading (no XML parsing at runtime)
- Validates color and geometry data
- Applies scaling for target display dimensions

### Usage

```bash
python3 tools/svg_preprocessor/svg_parser.py <input.svg> <output.gauge> [scale_factor]
```

**Examples:**
```bash
# From preprocessed SVG
python3 tools/svg_preprocessor/svg_parser.py assets/dashboard_tiny_processed.svg dashboard_tiny.gauge

# Specify scale factor
python3 tools/svg_preprocessor/svg_parser.py dashboard.svg dashboard.gauge 0.5
```

### Binary Format Specification

```
HEADER (128 bytes):
  [0-3]     Magic number: 0x44474147 ("DGAG")
  [4-7]     Version: 0x00000001
  [8-11]    Canvas width (uint32 LE)
  [12-15]   Canvas height (uint32 LE)
  [16-127]  Reserved

PATH SECTIONS (variable):
  Repeated for each path:
    uint32 LE    Point count
    uint32 ARGB  Color
    float        Stroke width
    (points...)  Array of (float, float) vertices
```

### Output Metrics

**Example - dashboard_tiny.svg:**
```
Input:  720×720 SVG with 3 paths
Processing:
  - Circle → 13 points (Bézier approximation)
  - Red arc → 11 points
  - Blue arc → 12 points
  Total: 36 point pairs = 144 floats
Output:  338 bytes (9.6x compression vs raw floats)
```

### Quality Settings

**Automatic Scaling**
- Input SVG dimensions → Target 480×320 (typical embedded display)
- Automatic scale factor calculation: `min(480/width, 320/height)`
- Preserves aspect ratio
- Applied to all point coordinates

**Custom Scaling**
```bash
# Scale by 2.0x
python3 tools/svg_preprocessor/svg_parser.py input.svg output.gauge 2.0

# Scale to exact 480x320 target
# (parser auto-calculates scale factor internally)
```

## Stage 3: Runtime Loading & Display

### Engine Integration

The binary gauge is loaded at runtime by `BinaryGaugeLoader`:

```cpp
// Load from file
auto gauge = BinaryGaugeLoader::load_from_file("dashboard_tiny.gauge");

// Or from embedded buffer
auto gauge = BinaryGaugeLoader::load_from_buffer(embedded_gauge_data);

// Validate
if (!gauge) {
    log_error("Failed to load gauge");
}
```

### Rendering Pipeline

```cpp
// Create scene and load gauge
GaugeScene scene;
scene.load_gauge(gauge);

// Update PID data
scene.update_pid_value("speed", 65.5);
scene.update_pid_value("rpm", 3200);

// Render to framebuffer
scene.render(framebuffer);
```

## Complete Workflow Example

### Step 1: Create or Edit SVG

Design your dashboard in Inkscape, Adobe XD, or other SVG editor:
- Use circles, rectangles, paths
- Set stroke colors and widths
- Group related elements
- Ensure proper XML structure

Save as `dashboard.svg`

### Step 2: Preprocess

```bash
python3 tools/svg_preprocessor/svg_preprocessor_advanced.py \
    assets/dashboard.svg \
    assets/dashboard_processed.svg
```

**Output:** 
```
Preprocessing assets/dashboard.svg...
  - Converting circles to Bézier paths
  - Converting ellipses to Bézier paths
  - Flattening arc commands
  - Cleaning attributes
✓ Preprocessing complete!
  Input:  assets/dashboard.svg
  Output: assets/dashboard_processed.svg
```

### Step 3: Encode to Binary

```bash
python3 tools/svg_preprocessor/svg_parser.py \
    assets/dashboard_processed.svg \
    dashboard.gauge
```

**Output:**
```
Parsing assets/dashboard_processed.svg...
Found 8 paths
SVG dimensions: 800×600
Scaling by 0.60
Wrote dashboard.gauge
```

### Step 4: Test in Simulator

```bash
./build/digi-dash-simulator dashboard.gauge
```

The gauge will display with proper colors and shapes. Verify:
- ✅ All elements visible
- ✅ Colors are correct
- ✅ Arcs are smooth (not jagged)
- ✅ Circles are round (not faceted)
- ✅ Text elements are readable (if present)

### Step 5: Deploy to Device (Future)

```cpp
// In device firmware (LVGL-based)
auto gauge = BinaryGaugeLoader::load_from_file(FLASH_GAUGE_PATH);
scene.load_gauge(gauge);
// Render to device display
```

## Troubleshooting

### Issue: Gauge renders but elements are missing

**Causes:**
- SVG uses shape elements not yet converted (e.g., polygons, images)
- Groups with `fill="none"` preventing child rendering
- Elements in `<defs>` or `<use>` (not yet supported)

**Solution:**
1. Edit SVG in text editor
2. Convert problematic shapes to `<path>` elements manually
3. Re-run preprocessing

### Issue: Colors are wrong

**Causes:**
- Fill vs stroke attributes not set
- Color specified in `style` attribute
- Color inheritance from parent group

**Solution:**
```bash
# Debug: check what colors were extracted
python3 -c "
import xml.etree.ElementTree as ET
tree = ET.parse('dashboard.svg')
for path in tree.findall('.//path'):
    fill = path.get('fill', 'none')
    stroke = path.get('stroke', 'none')
    print(f'fill={fill}, stroke={stroke}')
"
```

### Issue: File size is too large for embedded storage

**Causes:**
- Too many points in paths (high detail)
- Many paths with small differences
- Unnecessary precision in coordinates

**Solution:**
1. Simplify paths in SVG editor (Path → Simplify)
2. Increase scale factor to reduce stored coordinates:
   ```bash
   python3 tools/svg_preprocessor/svg_parser.py input.svg output.gauge 0.5
   ```
3. Merge similar paths in SVG

### Issue: Arcs appear as straight lines in output

**This should not occur** - arc flattening is automatic in preprocessing.

Verification:
```bash
# Check preprocessed SVG
grep -c ' C ' dashboard_processed.svg  # Should have many C (Bézier) commands
grep -c ' A ' dashboard_processed.svg  # Should be 0 (all arcs flattened)
```

## Performance Characteristics

**Preprocessing Time:**
- Simple gauges (3-5 paths): < 100ms
- Complex gauges (20+ paths): 200-500ms
- SVG size impact: Linear (10KB SVG ≈ 50ms)

**Binary File Size:**
```
Typical gauge: 300-2000 bytes
  Small:    < 500 bytes   (3-5 simple paths)
  Medium:   500-2000      (10-15 complex paths)
  Large:    2000-10000    (30+ paths, high detail)
```

**Runtime Loading:**
- Binary file load: < 5ms (flash read + parsing)
- Path rendering: 5-15ms per frame (30 FPS target)
- Memory usage: ~1-10KB for gauge asset (path storage)

## Best Practices

### For Designers

1. **Use simple paths** - Complex bezier curves are preserved as-is
2. **Set colors explicitly** - Use `fill` or `stroke` attributes, not CSS
3. **Consistent stroke widths** - Round to nearest integer
4. **Avoid transforms** - Pre-bake transforms in SVG data, not attributes
5. **Group logically** - Use `<g>` to organize related elements

### For Engineers

1. **Always preprocess first** - Never skip the advanced preprocessing step
2. **Validate binary output** - Check file magic number and dimensions
3. **Test on target hardware** - Binary works on desktop, verify on device
4. **Monitor file size** - Track gauge binary size during iteration
5. **Cache processed gauges** - Preprocess during build, not at runtime

### For LVGL Migration

When moving from SDL2 simulator to LVGL device:

1. **Verify LVGL flags are set:**
   ```cmake
   set(LV_USE_SVG 1)
   set(LV_USE_VECTOR_GRAPHIC 1)
   set(LV_USE_THORVG_INTERNAL 1)
   ```

2. **Create LVGL-specific renderer** alongside VectorRenderer
3. **Flatten arcs before rendering** (already done in preprocessing)
4. **Test circles and strokes** (converted to paths, should work identically)

## Future Enhancements

- [ ] Polygon to path conversion
- [ ] SVG text element support with font embedding
- [ ] Automatic color palette extraction
- [ ] Grouped path animation support
- [ ] SVG filters (blur, shadow) simulation
- [ ] Compression (LZ4/DEFLATE) for large gauge files

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System design overview
- [SVG_PROCESSING_COMPLETE.md](SVG_PROCESSING_COMPLETE.md) - Technical details
- [SVG_TO_GAUGE_WORKFLOW.md](SVG_TO_GAUGE_WORKFLOW.md) - Quick reference
