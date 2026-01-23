# Digi-Dash ESP32-S3 Tests

This directory contains ESP-IDF Unity tests for the Digi-Dash core functionality.

## Running Tests

### On Real Hardware

```bash
cd test_app
. ~/esp-idf-5.5.2/export.sh

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### In QEMU

```bash
cd test_app
. ~/esp-idf-5.5.2/export.sh

# Build and run in QEMU
idf.py build
idf.py qemu monitor
```

### Interactive Menu

When the test app runs, you'll see an interactive menu:

```
Enter test for running.
Enter 'all' to run all tests.
Enter 'help' for more options.
```

Type `all` to run all tests, or enter a specific test name/pattern.

## Test Suites

- **Display Initialization** - Tests for display setup and validation
- **Data Updates** - Tests for dashboard data validation
- **Unit Conversions** - Tests for km/h ↔ mph and °C ↔ °F
- **Range Validation** - Tests for safe value range checking
- **Integration** - Tests for typical driving scenarios

## Component Under Test

Tests are located in: `components/digi_dash_core/test/`

The component being tested provides:
- Display initialization and status
- Dashboard data updates with validation
- Unit conversion utilities
- Range checking utilities

## Adding New Tests

1. Add test functions to `components/digi_dash_core/test/test_digi_dash_core.c`
2. Use `TEST_CASE("description", "[tag]")` macro
3. Rebuild and run: `cd test_app && idf.py build flash monitor`

## Example Output

```
═══════════════════════════════════════════════════════
     Digi-Dash ESP32-S3 Unit Tests
═══════════════════════════════════════════════════════

Running all tests...
display initialization with valid parameters [digi_dash] ... PASS
kmh to mph conversion [digi_dash][conversion] ... PASS
celsius to fahrenheit conversion [digi_dash][conversion] ... PASS
...
All tests passed!
```
