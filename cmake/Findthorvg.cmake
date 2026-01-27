# FindThorVG.cmake - Find ThorVG library
# This module defines:
#   thorvg_FOUND - True if ThorVG found
#   thorvg::thorvg - Imported target for ThorVG

message(STATUS "[FindThorVG] Starting search...")

# Try pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    message(STATUS "[FindThorVG] PkgConfig found, searching for thorvg-1...")
    pkg_check_modules(PC_THORVG QUIET thorvg-1)
    if(NOT PC_THORVG_FOUND)
        message(STATUS "[FindThorVG] thorvg-1 not found, trying thorvg...")
        pkg_check_modules(PC_THORVG QUIET thorvg)
    endif()
    message(STATUS "[FindThorVG] PC_THORVG_FOUND=${PC_THORVG_FOUND}")
else()
    message(STATUS "[FindThorVG] PkgConfig NOT found")
endif()

# Direct path search - try common locations
find_path(THORVG_INCLUDE_DIR
    NAMES thorvg.h
    HINTS ${PC_THORVG_INCLUDE_DIRS}
    PATHS /usr/local/include/thorvg-1
          /usr/local/include
          /usr/include
    NO_CMAKE_FIND_ROOT_PATH
)

find_library(THORVG_LIBRARY
    NAMES thorvg-1 thorvg
    HINTS ${PC_THORVG_LIBRARY_DIRS}
    PATHS /usr/local/lib/x86_64-linux-gnu
          /usr/local/lib
          /usr/lib
          /usr/lib/x86_64-linux-gnu
    NO_CMAKE_FIND_ROOT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(thorvg
    REQUIRED_VARS THORVG_LIBRARY THORVG_INCLUDE_DIR
)

if(thorvg_FOUND)
    if(NOT TARGET thorvg::thorvg)
        add_library(thorvg::thorvg UNKNOWN IMPORTED)
        set_target_properties(thorvg::thorvg PROPERTIES
            IMPORTED_LOCATION ${THORVG_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${THORVG_INCLUDE_DIR}
        )
    endif()
endif()

mark_as_advanced(THORVG_INCLUDE_DIR THORVG_LIBRARY)
