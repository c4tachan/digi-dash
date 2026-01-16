# Running Tests

## Quick Start

```bash
# Build the project and tests
cd simulator/build
cmake ..
make digi-dash-tests

# Run all tests
../tests/digi-dash-tests

# Or use ctest
ctest --output-on-failure
```

## Test Suites

- **ConfigTest** (10 tests): Configuration file loading and parsing
- **UnitSystemTest** (9 tests): Unit system configuration (metric/SAE)
- **ProtocolTest** (11 tests): Protocol support (CAN, KWP2000, ISO 9141-2)
- **GaugeTest** (10 tests): Gauge features (zones, markers, alerts)
- **MockDataTest** (13 tests): Mock data generation and ranges

## Test Results

```
[==========] 54 tests from 5 test suites ran. (12 ms total)
[  PASSED  ] 54 tests.
```

## Run Specific Test Suite

```bash
../tests/digi-dash-tests --gtest_filter="ConfigTest.*"     # 10 tests
../tests/digi-dash-tests --gtest_filter="UnitSystemTest.*" # 9 tests
../tests/digi-dash-tests --gtest_filter="ProtocolTest.*"   # 11 tests
../tests/digi-dash-tests --gtest_filter="GaugeTest.*"      # 10 tests
../tests/digi-dash-tests --gtest_filter="MockDataTest.*"   # 13 tests
```

## Generate Test Report

```bash
../tests/digi-dash-tests --gtest_output="xml:test-results.xml"
```

For complete documentation, see [docs/TESTING.md](docs/TESTING.md).
