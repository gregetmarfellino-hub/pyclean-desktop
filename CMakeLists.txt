cmake_minimum_required(VERSION 3.16)

project(
    PyClean_CPP
    VERSION 1.0
    LANGUAGES CXX
)

# ============================================================
# C++ LANGUAGE
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Useful for VS Code / clangd / IntelliSense tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# ============================================================
# QT
# ============================================================

find_package(
    Qt6
    REQUIRED
    COMPONENTS
        Widgets
        Sql
        Network
        HttpServer
)

qt_standard_project_setup()


# ============================================================
# TARGET
# ============================================================

qt_add_executable(
    PyClean_CPP

    src/main.cpp

    src/MainWindow.cpp
    src/MainWindow.h

    src/CategoryRules.cpp
    src/CategoryRules.h

    src/FileOrganizer.cpp
    src/FileOrganizer.h

    src/Database.cpp
    src/Database.h

    src/MobileServer.cpp
    src/MobileServer.h

    src/QrWindow.cpp
    src/QrWindow.h

    src/qrcodegen/qrcodegen.cpp
    src/qrcodegen/qrcodegen.hpp
)


target_link_libraries(
    PyClean_CPP
    PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qt6::HttpServer
)


# ============================================================
# WINDOWS GUI TARGET
# ============================================================

set_target_properties(
    PyClean_CPP
    PROPERTIES
        WIN32_EXECUTABLE ON
)


# ============================================================
# RELEASE OPTIMIZATION
# ============================================================

include(CheckIPOSupported)

check_ipo_supported(
    RESULT PYCLEAN_IPO_SUPPORTED
    OUTPUT PYCLEAN_IPO_ERROR
)

if(PYCLEAN_IPO_SUPPORTED)
    set_property(
        TARGET PyClean_CPP
        PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
else()
    message(
        STATUS
        "IPO/LTO unavailable: ${PYCLEAN_IPO_ERROR}"
    )
endif()


# MinGW / GCC-specific Release optimizations.
#
# CMake already supplies the normal Release optimization level
# (typically -O3 -DNDEBUG for GCC), so do not duplicate it here.
#
# These flags place functions/data in separate sections and let
# the linker discard unused ones from the final executable.

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

    target_compile_options(
        PyClean_CPP
        PRIVATE
            $<$<CONFIG:Release>:-ffunction-sections>
            $<$<CONFIG:Release>:-fdata-sections>
    )

    target_link_options(
        PyClean_CPP
        PRIVATE
            $<$<CONFIG:Release>:-Wl,--gc-sections>
            $<$<CONFIG:Release>:-s>
    )

endif()


# ============================================================
# RELEASE DEFINITIONS
# ============================================================

target_compile_definitions(
    PyClean_CPP
    PRIVATE
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)
