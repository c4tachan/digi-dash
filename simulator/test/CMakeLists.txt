cmake_minimum_required(VERSION 3.10)
project(digi_dash_unit_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(unit_tests test_dummy.cpp)

# tests are self-contained for now
install(TARGETS unit_tests DESTINATION bin)
