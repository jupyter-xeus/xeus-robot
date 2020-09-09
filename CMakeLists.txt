############################################################################
# Copyright (c) 2016, Martin Renou, Johan Mabille, Sylvain Corlay, and     #
# Wolf Vollprecht                                                          #
# Copyright (c) 2016, QuantStack                                           #
#                                                                          #
# Distributed under the terms of the BSD 3-Clause License.                 #
#                                                                          #
# The full license is in the file LICENSE, distributed with this software. #
############################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-robot)

set(XEUS_ROBOT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Versionning
# ===========

file(STRINGS "${XEUS_ROBOT_SRC_DIR}/xeus_robot_config.hpp" xrob_version_defines
     REGEX "#define XROB_VERSION_(MAJOR|MINOR|PATCH)")
foreach (ver ${xrob_version_defines})
    if (ver MATCHES "#define XROB_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XROB_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()
set(${PROJECT_NAME}_VERSION
    ${XROB_VERSION_MAJOR}.${XROB_VERSION_MINOR}.${XROB_VERSION_PATCH})
message(STATUS "Building xeus-robot v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XROBOT_KERNELSPEC_PATH)
    set(XROBOT_KERNELSPEC_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
endif ()

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xrobot/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xrobot/kernel.json"
)

# Dependencies
# ============

set(xeus_REQUIRED_VERSION 0.24.0)
set(xeus_python_REQUIRED_VERSION 0.8.4)

find_package(xeus ${xeus_python_REQUIRED_VERSION} REQUIRED)
find_package(xeus-python ${xeus_python_REQUIRED_VERSION} REQUIRED)

# Flags
# =====

include(CheckCXXCompilerFlag)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    if (XPYT_DISABLE_ARCH_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    elseif (XPYT_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif ()

    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)

    if (HAS_CPP14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++14 support!")
    endif ()
endif ()

# Source files
# ============

set(XROBOT_SRC
    src/main.cpp
    src/xinterpreter.hpp
    src/xinterpreter.cpp
    src/xeus_robot_config.hpp
)

# Targets and link
# ================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

# xrobot
# ======
add_executable(xrobot ${XROBOT_SRC})
target_link_libraries(xrobot PRIVATE pybind11::embed)

if (MSVC)
    target_compile_options(xrobot PUBLIC /wd4251 /wd4141)
    target_compile_options(xrobot PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
    CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
    CMAKE_CXX_COMPILER_ID MATCHES "Intel")

    target_compile_options(xrobot PUBLIC -Wunused-parameter -Wextra -Wreorder)

    # C++14 flag
    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
    if (HAS_CPP14_FLAG)
        target_compile_features(xrobot PRIVATE cxx_std_14)
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus-robot requires C++14 support!")
    endif ()

    # Enable link time optimization and set the default symbol
    # visibility to hidden (very important to obtain small binaries)
    if (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
        # Check for Link Time Optimization support
        # (GCC/Clang)
        CHECK_CXX_COMPILER_FLAG("-flto" HAS_LTO_FLAG)
        if (HAS_LTO_FLAG)
            target_compile_options(xrobot PUBLIC -flto)
        endif ()

        # Avoids removing symbols from the static library
        CHECK_CXX_COMPILER_FLAG("-ffat-lto-objects" HAS_FATLTO_FLAG)
        if (${linkage_upper} MATCHES "STATIC" AND  HAS_FATLTO_FLAG)
            message(STATUS "ENABLE FAT LTO OBJECTS")
            target_compile_options(xrobot PUBLIC -ffat-lto-objects)
        endif ()

        # Intel equivalent to LTO is called IPO
        if (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
            CHECK_CXX_COMPILER_FLAG("-ipo" HAS_IPO_FLAG)
            if (HAS_IPO_FLAG)
                target_compile_options(xrobot PUBLIC -ipo)
            endif ()
        endif ()
    endif ()

endif ()

if (APPLE)
    set_target_properties(xrobot PROPERTIES
        MACOSX_RPATH ON
    )
else ()
    set_target_properties(xrobot PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        SKIP_BUILD_RPATH FALSE
    )
endif ()

set_target_properties(xrobot PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

target_link_libraries(xrobot PRIVATE xeus xeus-python)

if(CMAKE_DL_LIBS)
    target_link_libraries(xrobot PRIVATE ${CMAKE_DL_LIBS} util)
endif()

# Installation
# ============

# Install xrobot
install(TARGETS xrobot
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Configuration and data directories for jupyter and xeus-python
set(XJUPYTER_DATA_DIR "share/jupyter"    CACHE STRING "Jupyter data directory")

# Install xrobot Jupyter kernelspec
set(XROB_KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
install(DIRECTORY ${XROB_KERNELSPEC_DIR}
        DESTINATION ${XJUPYTER_DATA_DIR}
        PATTERN "*.in" EXCLUDE)

# Extra path for installing Jupyter kernelspec
if (XEXTRA_JUPYTER_DATA_DIR)
    install(DIRECTORY ${XROB_KERNELSPEC_DIR}
            DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)
endif ()
