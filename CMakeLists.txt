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

# Build options
# =============

# Compilation options
OPTION(XROB_DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)
OPTION(XROB_DISABLE_TUNE_GENERIC "disable -mtune=generic flag" OFF)
OPTION(XROB_ENABLE_PYPI_WARNING "Enable warning on PyPI wheels" OFF)

OPTION(XROB_BUILD_XROBOT_EXECUTABLE "Build the xrobot executable" ON)
OPTION(XROB_BUILD_XROBOT_EXTENSION "Build the xrobot extension module" OFF)

OPTION(XROB_USE_SHARED_XEUS_PYTHON "Link xrobot and xrobot_extension with the xeus-python shared library (instead of the static library)" ON)

# Test options
OPTION(XROB_BUILD_TESTS "xeus-robot test suite" OFF)
OPTION(XROB_DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)

# Dependencies
# ============

set(xeus_python_REQUIRED_VERSION 0.11.1)
set(pybind11_REQUIRED_VERSION 2.6.0)
set(pybind11_json_REQUIRED_VERSION 0.2.8)

if (NOT TARGET xeus-python AND NOT TARGET xeus-python-static)
    find_package(xeus-python ${xeus_python_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET pybind11::headers)
    find_package(pybind11 ${pybind11_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET pybind11_json)
    find_package(pybind11_json ${pybind11_json_REQUIRED_VERSION} REQUIRED)
endif ()

# Flags
# =====

include(CheckCXXCompilerFlag)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    if (XROB_DISABLE_ARCH_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    elseif (XROB_DISABLE_TUNE_GENERIC)
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
    src/xinternal_utils.hpp
    src/xinternal_utils.cpp
    src/xinterpreter.hpp
    src/xinterpreter.cpp
    src/xeus_robot_config.hpp
    src/xdebugger.hpp
    src/xdebugger.cpp
    src/xrobodebug_client.hpp
    src/xrobodebug_client.cpp
)

set(XROBOT_EXTENSION_SRC
    src/xrobot_extension.cpp
    src/xinternal_utils.hpp
    src/xinternal_utils.cpp
    src/xinterpreter.hpp
    src/xinterpreter.cpp
    src/xeus_robot_config.hpp
    src/xdebugger.hpp
    src/xdebugger.cpp
    src/xrobodebug_client.hpp
    src/xrobodebug_client.cpp
)

# Targets and link - Macros
# =========================

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

include(CheckCXXCompilerFlag)

macro(xrob_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # Mtune generic/native
        if (XROB_DISABLE_ARCH_NATIVE AND NOT XROB_DISABLE_TUNE_GENERIC)
            target_compile_options(${target_name} PUBLIC -mtune=generic)
        elseif (XROB_DISABLE_TUNE_GENERIC)
        else ()
            target_compile_options(${target_name} PUBLIC -march=native)
        endif ()

        # C++14 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
        if (HAS_CPP14_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_14)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-robot requires C++14 support!")
        endif ()
    endif ()

    if (APPLE)
        set_target_properties(${target_name} PROPERTIES
            MACOSX_RPATH ON
        )
    else ()
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH 1
            SKIP_BUILD_RPATH FALSE
        )
    endif ()

    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endmacro()

# Common macro kernels (xpython and xpython_extension)
macro(xrob_set_kernel_options target_name)
    if(XROB_ENABLE_PYPI_WARNING)
        message(STATUS "Enabling PyPI warning for target: " ${target_name})
        target_compile_definitions(${target_name} PRIVATE XEUS_ROBOT_PYPI_WARNING)
    endif()

    if (XROB_USE_SHARED_XEUS_PYTHON)
        target_link_libraries(${target_name} PRIVATE xeus-python)

        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_link_libraries(${target_name} PRIVATE xeus-python-static)
    endif()

    if (XEUS_PYTHONHOME_RELPATH)
        target_compile_definitions(${target_name} PRIVATE XEUS_PYTHONHOME_RELPATH=${XEUS_PYTHONHOME_RELPATH})
    endif()

    target_link_libraries(${target_name} PRIVATE pybind11::pybind11 pybind11_json)

    find_package(Threads) # TODO: add Threads as a dependence of xeus or xeus-static?
    target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
endmacro()

# xrobot
# ======

if (XROB_BUILD_XROBOT_EXECUTABLE)
    add_executable(xrobot ${XROBOT_SRC})
    target_link_libraries(xrobot PRIVATE pybind11::embed)

    xrob_set_common_options(xrobot)
    xrob_set_kernel_options(xrobot)
endif()

# xpython_extension
# =================

if (XROB_BUILD_XROBOT_EXTENSION)
    pybind11_add_module(xrobot_extension ${XROBOT_EXTENSION_SRC})

    xrob_set_common_options(xrobot_extension)
    xrob_set_kernel_options(xrobot_extension)
endif()

# Tests
# =====

if(XROB_DOWNLOAD_GTEST OR XROB_GTEST_SRC_DIR)
    set(XROB_BUILD_TESTS ON)
endif()

if(XROB_BUILD_TESTS)
    add_subdirectory(test)
endif()

# Installation
# ============

# Install xrobot
if (XROB_BUILD_XROBOT_EXECUTABLE)
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
endif()
