# QEMU Simulator Setup

This document explains how to build and run the digi-dash firmware in QEMU for testing on the host computer.

## Prerequisites

1. ESP-IDF 5.5.2 or later installed
2. QEMU with ESP32-S3 support
3. cmake 3.16+

## Setup

### 1. Install and configure ESP-IDF

```bash
# Install ESP-IDF v5.5.2
mkdir -p ~/esp
cd ~/esp

# Download and extract ESP-IDF 5.5.2
wget https://github.com/espressif/esp-idf/releases/download/v5.5.2/esp-idf-v5.5.2.zip
python3 -m zipfile -e esp-idf-v5.5.2.zip .
ln -s esp-idf-v5.5.2 esp-idf

# Install tools for ESP32-S3
cd esp-idf
chmod +x tools/*.py tools/*.sh
bash install.sh esp32s3

# Source the environment (add to ~/.bashrc for permanent setup)
source ~/esp/esp-idf/export.sh
```

### 2. Generate gauge file

The firmware loads gauge files from SPIFFS. Generate the gauge file:

```bash
cd /home/catachan/projects/digi-dash
./build/tools/svg_preprocessor/svg_preprocessor assets/dashboard_tiny.svg assets/dashboard_tiny.gauge
```

This creates `assets/dashboard_tiny.gauge` which contains the preprocessed SVG rendering data.

### 3. Build firmware

From the project root:

```bash
# Option 1: Using idf.py (recommended)
idf.py build

# Option 2: Using the script
bash scripts/build_firmware.sh
```

### 4. Run in QEMU

```bash
idf.py qemu monitor
```

The firmware will:
1. Initialize SPIFFS
2. Load the gauge file from `/spiffs/dashboard_tiny.gauge`
3. Parse and render the gauge to a framebuffer
4. Log rendering updates

### Adding gauge files to QEMU

For QEMU to access gauge files, you need to mount them in SPIFFS. There are two approaches:

#### Approach A: Flash image with gauge file (Recommended)

Create a flash image with the gauge file included:

```bash
# Generate SPIFFS image with gauge file
python $IDF_PATH/components/spiffs/spiffsgen.py \
    147456 assets spiffs_image.img

# Erase SPIFFS partition in QEMU image
esptool.py -p /dev/ttyUSB0 erase_region 0x370000 0x90000

# Write SPIFFS image
esptool.py -p /dev/ttyUSB0 write_flash 0x370000 spiffs_image.img
```

#### Approach B: Dynamically mount host directory

When using QEMU, you can use ESP-IDF's host filesystem support to mount the assets directory:

```bash
idf.py qemu monitor --qemu-extra-args "-fsdev local,security_model=none,id=fsdev0,path=$(pwd)/assets -device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag=spiffs"
```

## Display Output

Currently, the firmware renders to an internal framebuffer. Future integration points:

- **LVGL**: For a GUI framework
- **SPI Display Driver**: For real hardware with displays like ILI9341
- **Framebuffer Console**: For QEMU virtual display

## Testing in QEMU

The QEMU simulator will:
1. Boot the ESP32-S3 with SPIFFS mounted
2. Load and parse the gauge file
3. Render frames at ~30 FPS
4. Log periodic updates

Monitor output will show:
```
I (timestamp) digi-dash: Gauge file loaded: 352 bytes
I (timestamp) digi-dash: Gauge parsed: 720x720 with 3 paths
I (timestamp) digi-dash: Rendering... frame 30
I (timestamp) digi-dash: Rendering... frame 60
...
```

## Troubleshooting

### SPIFFS not mounting
- Ensure `CONFIG_SPIFFS_USE_MTIME=y` in sdkconfig
- Check partition table: `idf.py partition_table`

### Gauge file not found
- Verify gauge file exists: `ls assets/dashboard_tiny.gauge`
- Check SPIFFS image creation
- Ensure file is mounted at `/spiffs/`

### Build errors
- Run `idf.py fullclean` then `idf.py build`
- Verify ESP-IDF is properly sourced: `echo $IDF_PATH`
- Check CMake configuration: `idf.py reconfigure`

## Next Steps

1. Implement actual display driver (LVGL + SPI)
2. Add OBD2 data simulation for testing
3. Implement gauge animations
4. Add touch input handling for future interactive elements
