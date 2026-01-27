cmake_minimum_required(VERSION 3.16)
project(svg_preprocessor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Look for local ThorVG installation first
set(THORVG_ROOT "/home/catachan/projects/thorvg" CACHE PATH "ThorVG installation directory")

# Try to find ThorVG
if(EXISTS "${THORVG_ROOT}/inc/thorvg.h")
    message(STATUS "Found local ThorVG at ${THORVG_ROOT}")
    set(THORVG_INCLUDE_DIR "${THORVG_ROOT}/inc")
    set(THORVG_LIB_DIR "${THORVG_ROOT}/builddir/src")
    
    # Create an imported target for ThorVG
    add_library(ThorVG::ThorVG SHARED IMPORTED)
    set_target_properties(ThorVG::ThorVG PROPERTIES
        IMPORTED_LOCATION "${THORVG_LIB_DIR}/libthorvg.so.0.15.16"
        INTERFACE_INCLUDE_DIRECTORIES "${THORVG_INCLUDE_DIR}"
    )
    set(THORVG_FOUND TRUE)
else()
    # Try system-wide installation
    find_package(ThorVG QUIET)
    if(NOT ThorVG_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(THORVG thorvg QUIET)
            if(NOT THORVG_FOUND)
                message(FATAL_ERROR "ThorVG library not found. Install it or set THORVG_ROOT to the installation directory.")
            endif()
        else()
            message(FATAL_ERROR "ThorVG library not found. Install it or set THORVG_ROOT to the installation directory.")
        endif()
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)

add_library(digidash_preproc
    src/svg_loader.cpp
    src/svg_normalizer.cpp
    src/path_flattener.cpp
    src/gauge_serializer.cpp
)

# Link against ThorVG
if(THORVG_FOUND AND TARGET ThorVG::ThorVG)
    target_link_libraries(digidash_preproc PRIVATE ThorVG::ThorVG)
elseif(THORVG_LIBRARIES)
    target_include_directories(digidash_preproc PRIVATE ${THORVG_INCLUDE_DIRS})
    target_link_libraries(digidash_preproc PRIVATE ${THORVG_LIBRARIES})
endif()

add_executable(svg_preprocessor
    src/main.cpp
)

target_link_libraries(svg_preprocessor PRIVATE digidash_preproc)