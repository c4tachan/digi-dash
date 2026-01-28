# ESP32 Unit Testing Guide

## Overview

The project includes ESP-IDF Unity tests for validating ESP32 core functionality. Tests can run on real hardware or in QEMU.

## Test Structure

```
components/digi_dash_core/      # Component under test
├── digi_dash_core.c           # Implementation
├── include/digi_dash_core.h   # Public API
└── test/                       # Unit tests
    ├── CMakeLists.txt
    └── test_digi_dash_core.c  # Test cases

test_app/                       # Test runner application
├── main/test_main.c           # Unity test runner
└── CMakeLists.txt
```

## Running Tests

### Quick Start - QEMU

```bash
cd test_app
. ~/esp/esp-idf/export.sh
idf.py build
idf.py qemu monitor
```

When prompted:
1. Press ENTER to see test list
2. Type `all` to run all tests
3. Or enter specific test name/pattern

### On Hardware

```bash
cd test_app
. ~/esp/esp-idf/export.sh
idf.py -p /dev/ttyUSB0 flash monitor
```

## Test Categories

### Display Tests
- Initialization with valid parameters
- Parameter validation (zero dimensions)
- Status checking

### Data Validation Tests
- Requires initialization check
- Negative value rejection
- Out-of-range value rejection (RPM, temperature)

### Unit Conversion Tests
- `km/h ↔ mph` conversion accuracy
- `°C ↔ °F` conversion accuracy
- Edge cases (0, negatives)

### Range Validation Tests
- In-range value checking
- Boundary conditions
- Out-of-range detection

### Integration Tests
- Typical driving scenarios
- Real-world data sequences

## Writing New Tests

Add tests to `components/digi_dash_core/test/test_digi_dash_core.c`:

```c
TEST_CASE("your test description", "[tag]")
{
    // Setup
    digi_dash_display_init(320, 240);
    
    // Test
    esp_err_t ret = digi_dash_update_data(100.0f, 3000, 90.0f);
    
    // Verify
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}
```

### Test Tags

Use tags to group related tests:
- `[digi_dash]` - Core functionality
- `[conversion]` - Unit conversions
- `[validation]` - Input validation
- `[integration]` - Multi-step scenarios

## Running Specific Tests

```bash
# Interactive menu
idf.py qemu monitor

# Examples:
> all                    # Run all tests
> digi_dash             # Run tests with [digi_dash] tag
> conversion            # Run conversion tests
> "display init*"       # Run tests matching pattern
```

## Expected Output

```
═══════════════════════════════════════════════════════
     Digi-Dash ESP32-S3 Unit Tests
═══════════════════════════════════════════════════════

Press ENTER to see the list of tests.

Running all tests...
display initialization with valid parameters [digi_dash] ... PASS
display initialization with zero width fails [digi_dash] ... PASS
kmh to mph conversion [digi_dash][conversion] ... PASS
...

-----------------------
12 Tests 0 Failures 0 Ignored 
OK
```

## Continuous Integration

For CI/CD, use non-interactive mode:

```bash
# Build tests
cd test_app
idf.py build

# Run in QEMU (requires automation wrapper)
idf.py qemu &
QEMU_PID=$!
echo "all" | idf.py monitor
kill $QEMU_PID
```

## Troubleshooting

### Tests don't appear in menu
- Ensure test file is in `components/<component>/test/`
- Check CMakeLists.txt includes test directory
- Rebuild: `idf.py fullclean && idf.py build`

### Component not found
- Verify symlink: `ls -la test_app/components`
- Should point to `../components`

### Build errors
```bash
# Clean and retry
cd test_app
idf.py fullclean
idf.py build
```

## Adding New Components

To test a new component:

1. Create component structure:
```
components/my_component/
├── my_component.c
├── include/my_component.h
├── CMakeLists.txt
└── test/
    ├── CMakeLists.txt
    └── test_my_component.c
```

2. Test CMakeLists.txt:
```cmake
idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES "unity" "my_component"
)
```

3. Rebuild test_app

## References

- [ESP-IDF Unity Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/unit-tests.html)
- [Unity Test Framework](http://www.throwtheswitch.org/unity)
