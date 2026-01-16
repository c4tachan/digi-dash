# Unit tests for Digi-Dash
# Requires: Google Test (gtest)

enable_testing()

# Find or download Google Test
include(FetchContent)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(digi-dash-tests
    test_config.cpp
    test_units.cpp
    test_protocol.cpp
    test_gauge.cpp
    test_mock_data.cpp
)

# Link against test library and main code
target_link_libraries(digi-dash-tests
    gtest
    gtest_main
    nlohmann_json::nlohmann_json
)

# Include paths
target_include_directories(digi-dash-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/build/_deps/json-src/include
)

# Add config source files (without main.cpp)
target_sources(digi-dash-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/src/config.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../simulator/src/mock_data.cpp
)

# Register tests
add_test(NAME AllTests COMMAND digi-dash-tests)
