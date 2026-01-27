# Rendering Pipeline

## Overview

The rendering pipeline converts gauge data → screen pixels in real-time on both simulator and ESP32.

## Stage 1: Data Binding

1. **OBD2 Input** → PIDBindingSystem
2. **Value Scaling** (raw value → gauge range)
3. **Animation Updates** (smooth transitions)
4. **Scene State** is now current

## Stage 2: Vector Rendering

1. **Path Enumeration**: Iterate all paths in gauge asset
2. **Bézier Rasterization**: Convert cubic Bézier to scanlines
3. **Antialiasing**: 4x supersampling for smooth curves
4. **Color Application**: Apply colors and strokes
5. **Composition**: Blend layers to framebuffer

## Stage 3: Text Rendering

1. **Dynamic Text** values (RPM, speed, etc.)
2. **Font Selection** based on asset definition
3. **Glyph Rasterization** using LVGL bitmap fonts
4. **Composition** over vector elements

## Stage 4: Display Output

- **SDL2 (Simulator)**: Copy framebuffer to SDL_Texture and present
- **LVGL (ESP32)**: Direct framebuffer write to display driver
- **Target FPS**: 30-60 FPS depending on platform

## Performance Considerations

### Memory Usage

- Framebuffer: 480×320×4 = 614 KB (ESP32 PSRAM)
- Asset Cache: ~100 KB per gauge (ESP32 Flash)
- Runtime State: ~10 KB (animations, bindings)

### CPU Budget (ESP32)

- 30 FPS target = ~33 ms per frame
- Vector rasterization: ~15 ms
- Animation updates: ~2 ms
- Bluetooth/OBD2: ~5 ms (interrupt-driven)
- Headroom: ~11 ms

### Optimization Strategies

1. **Dirty Region Tracking**: Only re-render changed areas
2. **Path Simplification**: Pre-process SVG to reduce curves
3. **Glyph Caching**: Cache frequently updated text
4. **Double Buffering**: Avoid visual tearing
5. **SIMD Rasterization**: Use CPU vector instructions

## Pipeline Pseudocode

```c++
void render_frame(GaugeScene& scene, PlatformDisplay& display) {
    uint8_t* fb = display.lock_framebuffer();
    
    // Clear background
    display.clear(BACKGROUND_COLOR);
    
    // Render all paths
    for (const auto& path : scene.get_paths()) {
        renderer.rasterize_bezier(path, fb);
    }
    
    // Render dynamic text
    for (const auto& text : scene.get_dynamic_text()) {
        font_mgr.render_text(text, fb);
    }
    
    // Update display
    display.unlock_and_update();
}
```
