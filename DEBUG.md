# VS Code Debugging Setup

## PNG Decoding Issue Debugging

The simulator is now set up to debug PNG decoding issues in VS Code.

### Quick Start

1. **Open VS Code** in the workspace root
2. **Install the C/C++ Extension** (if not already installed):
   - Install "C/C++" by Microsoft
   
3. **Start Debugging**:
   - Press `Ctrl+Shift+D` to open the Debug view
   - Select "Debug Simulator (PNG Decode)" from the dropdown
   - Click the green play button or press `F5`

### Debug Configurations

#### Option 1: Debug Simulator (PNG Decode)
- **Auto-builds** the simulator before debugging
- **Best for**: Fresh builds after code changes
- Command: `Ctrl+Shift+D` → Select → `F5`

#### Option 2: Debug Simulator (No Pre-build)
- **Skips the build step** if already built
- **Best for**: Quick debugging without rebuilding
- Faster startup
- Command: `Ctrl+Shift+D` → Select → `F5`

### Debugging Workflow

1. **Set Breakpoints**:
   - Click in the gutter (left of line numbers) in `simulator/src/main.cpp`
   - Red dot appears = breakpoint set
   - Focus on these areas:
     - `lv_image_set_src()` call (line ~145)
     - Image descriptor check (line ~147)
     - PNG decoder path in LVGL

2. **Step Through Code**:
   - `F10` = Step over
   - `F11` = Step into (dive into function calls)
   - `Shift+F11` = Step out
   - `F5` = Continue execution

3. **Inspect Variables**:
   - Hover over variables to see values
   - Use Debug Console at bottom to run expressions
   - Watch tab: Right-click variable → "Add to Watch"

4. **Debug Console**:
   - Type gdb commands directly:
     - `p img_src` = print variable
     - `info locals` = show all local variables
     - `bt` = backtrace (call stack)

### Key Areas to Investigate

The PNG decoder is failing. These are the key callstacks to trace:

1. **LVGL Image Module**: `simulator/lib/lvgl/src/widgets/image/lv_image.c`
   - `lv_image_set_src()` - where the filename is passed
   - Check if decoder is being called

2. **Image Decoder**: `simulator/lib/lvgl/src/core/lv_image_decoder.c`
   - `image_decoder_get_info()` - where decode info is retrieved
   - Check decoder callbacks

3. **PNG Decoder**: `simulator/lib/lvgl/src/core/lv_image_decoder_*.c`
   - Find PNG-specific decoder
   - Check if it's registered and enabled
   - Verify ThorVG/libpng is initialized

4. **POSIX FS Driver**: `simulator/lib/lvgl/src/libs/fsdrv/lv_fs_posix.c`
   - File open succeeds (we know this works)
   - But decoder might fail with the file handle

### Expected Behavior

**Current (Broken)**:
```
✓ File exists on disk: ../../assets/scca_400.png (242403 bytes)
✓ Successfully opened PNG via LVGL FS
✗ Image decode failed - descriptor is NULL
```

**Should be (Fixed)**:
```
✓ File exists on disk: ../../assets/scca_400.png (242403 bytes)
✓ Successfully opened PNG via LVGL FS
✓ Image decoded successfully!
   Width: 400, Height: 400
```

### Build Tasks (Ctrl+Shift+B)

- **Configure & Build Simulator** (default): Full cmake + make
- **Build Simulator**: Incremental make only
- **Build ESP32**: For ESP32 firmware

### Console Output

When debugging, you'll see LVGL's debug logs:
- `[digi-dash-sim]` = Our simulator code
- `[Warn]` = LVGL warnings (PNG decoder issues will appear here)
- `[Error]` = LVGL errors

### Troubleshooting

**"gdb not found"**:
```bash
# Install gdb
sudo apt-get install gdb

# Or via your package manager
```

**"Program not found"**:
- Make sure "Build Simulator" task ran successfully
- Check: `ls -la simulator/build/digi-dash-simulator`

**"Debugger won't attach"**:
- Kill any running simulator: `pkill digi-dash-simulator`
- Try "Debug Simulator (No Pre-build)" instead

**Can't hit breakpoints**:
- Ensure binary was built with `-g` (it is, we use `-DCMAKE_BUILD_TYPE=Debug`)
- Check breakpoint is set before launching

### Next Steps

1. Set a breakpoint at the `lv_image_set_src()` call
2. Run the debugger
3. Step into the function to trace where it fails
4. Check the PNG decoder initialization
5. Verify ThorVG/PNG support is properly enabled in lv_conf.h
