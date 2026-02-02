cmake_minimum_required(VERSION 3.14)
project(digi_dash_unit_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
	catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

add_executable(unit_tests test_color_utils.cpp test_pid_binding_system.cpp test_binary_gauge_loader.cpp test_tile_height_renderer.cpp)

# Ensure engine headers are available to tests
target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/../../engine/include)

# Add engine sources required by tests (compile into the test binary)
target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/esp_stubs)
target_include_directories(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/../../firmware/main)

# Engine sources used by tests
target_sources(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/../../engine/src/pid_binding_system.cpp
									${PROJECT_SOURCE_DIR}/../../engine/src/binary_gauge_loader.cpp)

# Tile renderer (firmware) used by renderer tests
target_sources(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/../../firmware/main/subsystems/rendering/tile_height_renderer.cpp)
target_sources(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/gauge_scene_stub.cpp)
target_sources(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/engine_stubs.cpp)

# Firmware/platform sources and test stubs
target_sources(unit_tests PRIVATE ${PROJECT_SOURCE_DIR}/../../firmware/main/platform/display/display_driver.cpp
									${PROJECT_SOURCE_DIR}/esp_stubs/esp_stubs.cpp)

# Link Catch2 (header-only) and enable test discovery
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)

# tests are self-contained for now
install(TARGETS unit_tests DESTINATION bin)
