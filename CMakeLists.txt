# cmake minimum version
cmake_minimum_required(VERSION 3.8)

# This CMakeLists was freely adapted by Florian Arrestier from 
# https://github.com/robotology/how-to-export-cpp-library and
# https://github.com/gegelati/gegelati/tree/develop
# Which is available under the following license :
#
# The MIT License (MIT)
#
# Copyright (c) 2014 Silvio Traversaro, Claudio Fantacci
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files (the "Software"), to 
# deal in the Software without restriction, including without limitation the 
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
# sell copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
# THE SOFTWARE.


# This sets the PROJECT_NAME, PROJECT_VERSION as well as other variable
# listed here: https://cmake.org/cmake/help/latest/command/project.html.
# We use this name to export all the files such that is then possible to use
# find_package(LibTemplateCMake) in third party projects.
# LANGUAGES specifies which languages your project supports. To have a broad
# support with external libraries, it is good practice to list both CXX and C.
# Infact, some legacy Find<package>.cmake files that require the C language to
# be enabled and thus using only CXX may cause issues. Note that by default when
# LANGUAGES is not specified CMake enables both CXX and C.

project(spider2
        LANGUAGES CXX
        VERSION 0.0.0)

# Defines the CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR and many other useful macros.
# See https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
include(GNUInstallDirs)

# We need thread support
include(FindThreads)

# Control where libraries and executables are placed during the build.
# With the following settings executables are placed in <the top level of the
# build tree>/bin and libraries/archives in <top level of the build tree>/lib.
# This is particularly useful to run ctests on libraries built on Windows
# machines: tests, which are executables, are placed in the same folders of
# dlls, which are treated as executables as well, so that they can properly
# find the libraries to run. This is a because of missing RPATH on Windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

# To build shared libraries in Windows, we set CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS to TRUE.
# See https://cmake.org/cmake/help/v3.4/variable/CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS.html
# See https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Set warnings for all compilers
include(cmake/CompilerWarnings.cmake)

# Compiler specific settings
if (MSVC)
    # Under MSVC, we set CMAKE_DEBUG_POSTFIX to "d" to add a trailing "d" to library
    # built in debug mode. In this Windows user can compile, build and install the
    # library in both Release and Debug configuration avoiding naming clashes in the
    # installation directories.
    set(CMAKE_DEBUG_POSTFIX "d")
    message("Add definition _CRT_SECURE_NO_WARNINGS flag for MSVC compiler.")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> CONFIGURATIONS "Debug" "RelWithDebInfo" DESTINATION "${CMAKE_INSTALL_BINDIR}")
else ()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -O0 --coverage -ftest-coverage -fprofile-arcs")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}  -O2 -DNDEBUG -fno-rtti -flto")
    message(STATUS "Using Link Time Optimization (-flto) on GNU platforms.")
    if (${CMAKE_GENERATOR} MATCHES "MinGW Makefiles")
        # To prevent a redefinition conflict
        message(STATUS "Add definition _TIMESPEC_DEFINED")
        add_definitions(-D_TIMESPEC_DEFINED)
    endif ()
endif ()

# CMake cross compilation to mingw
if (${CROSS_COMPILE_MINGW})
    set(CMAKE_SYSTEM_NAME Windows)
    set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

    # cross compilers to use for C and C++
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    set(CMAKE_Fortran_COMPILER ${TOOLCHAIN_PREFIX}-gfortran)
    set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

    # target environment on the build host system
    #   set 1st to dir with the cross compiler's C/C++ headers/libs
    set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

    # modify default behavior of FIND_XXX() commands to
    # search for headers/libs in the target environment and
    # search for programs in the build host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif ()


# Build position independent code.
# Position Independent Code (PIC) is commonly used for shared libraries so that
# the same shared library code can be loaded in each program address space in a
# location where it will not overlap with any other uses of such memory.
# In particular, this option avoids problems occurring when a process wants to
# load more than one shared library at the same virtual address.
# Since shared libraries cannot predict where other shared libraries could be
# loaded, this is an unavoidable problem with the traditional shared library
# concept.
# Generating position-independent code is often the default behavior for most
# modern compilers.
# Moreover linking a static library that is not built with PIC from a shared
# library will fail on some compiler/architecture combinations.
# Further details on PIC can be found here:
# https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable C and C++ compiler extensions.
# C/CXX_EXTENSIONS are ON by default to allow the compilers to use extended
# variants of the C/CXX language.
# However, this could expose cross-platform bugs in user code or in the headers
# of third-party dependencies and thus it is strongly suggested to turn
# extensions off.
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setting C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# We use
# - InstallBasicPackageFiles (http://robotology.github.io/ycm/gh-pages/v0.8/module/InstallBasicPackageFiles.html)
# - AddUninstallTarget (http://robotology.github.io/ycm/gh-pages/v0.8/module/AddUninstallTarget.html)
# - AddInstallRPATHSupport (http://robotology.github.io/ycm/gh-pages/v0.8/module/AddInstallRPATHSupport.html)
# from YCM. Files are under the cmake subdirectory.
# See https://github.com/robotology/ycm/
# If you don't want to ship these files with your code (recommended), you can
# instead depend on YCM.
# In this case replace the following line with
# find_package(YCM REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

### Options

# Build test related commands?
option(BUILD_TESTING "Create tests using CMake" ON)
if (BUILD_TESTING MATCHES "ON")
    enable_testing()
    # Download and unpack googletest at configure time
    # from https://github.com/google/googletest/tree/master/googletest#incorporating-into-an-existing-cmake-project
    configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if (result)
        message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif ()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if (result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif ()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)
endif ()

# Build legacy runtime (default is yes) ?
option(BUILD_LEGACY_RUNTIME "Build the code related to the legacy runtime of SPIDER." ON)
if (NOT BUILD_LEGACY_RUNTIME)
    add_definitions(-D_NO_BUILD_LEGACY_RT)
    message(STATUS "Legacy runtime will not be built.")
endif ()

# Build gantt exporter(s) (default is yes) ?
option(BUILD_GANTT_EXPORTER "Build the code related to the gantt exporter(s)." ON)
if (NOT BUILD_GANTT_EXPORTER)
    add_definitions(-D_NO_BUILD_GANTT_EXPORTER)
    message(STATUS "Gantt exporter(s) will not be built.")
endif ()

# Build graph exporter(s) (default is yes) ?
option(BUILD_GRAPH_EXPORTER "Build the code related to the graph exporter(s)." ON)
if (NOT BUILD_GRAPH_EXPORTER)
    add_definitions(-D_NO_BUILD_GRAPH_EXPORTER)
    message(STATUS "Graph exporter(s) will not be built.")
endif ()

# Enable RPATH support for installed binaries and libraries
include(AddInstallRPATHSupport)
add_install_rpath_support(BIN_DIRS "${CMAKE_INSTALL_FULL_BINDIR}"
        LIB_DIRS "${CMAKE_INSTALL_FULL_LIBDIR}"
        INSTALL_NAME_DIR "${CMAKE_INSTALL_FULL_LIBDIR}"
        USE_LINK_PATH)

# Use JIT-Compiled expression (default is yes) ?
option(USE_JIT_EXPRESSION "Enable the JIT-Compiled expression parser (only on Linux)." ON)
if (USE_JIT_EXPRESSION)
    if (NOT MSVC AND NOT MINGW)
        add_definitions(-D_SPIDER_JIT_EXPRESSION)
        message(STATUS "Using just-in-time compiled expression parser.")
    else ()
        message(STATUS "Defaulting to runtime expression parser on Windows.")
    endif ()
endif ()
unset(USE_JIT_EXPRESSION CACHE)

# Use PAPI library?
option(USE_PAPI "Enable PAPI library support." OFF)
if (USE_PAPI)
    find_package(PAPI)
    if (PAPI_FOUND)
        add_definitions(-D_SPIDER_PAPI_AVAILABLE)
    else ()
        message(STATUS "PAPI library NOT FOUND")
        set(PAPI_LIB_DIR "")
        set(PAPI_INCLUDE_DIR "")
    endif ()
endif ()
unset(USE_PAPI CACHE)

# Encourage user to specify a build type (e.g. Release, Debug, etc.), otherwise set it to Release.
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        message(STATUS "Setting build type to 'Release' as none was specified.")
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Release")
    endif ()
endif ()

### Compile- and install-related commands.
add_subdirectory(libspider)

# Create and install CMake configuration files for your project that are
# necessary to for other projects to call find_package(LibTemplateCMake).
#
# Note that it is extremely important to use exactly the project name while
# installing configuration files (you can use PROJECT_NAME variable to avoid
# any possible error). This is required to allow find_package() to properly
# look for the installed library in system path, in particular in Windows when
# the installation is performed in the default path.
#
# install_basic_package_files() comes with many input parameters to customize
# the configuration files. The parameters used in the following call provide
# basic versions of CMake configuration files.
# See install_basic_package_files() documentation found in ./cmake folder.
#
# Note that if your library depends from other libraries, you are probably
# required to used the install_basic_package_files() DEPENDENCIES option.
include(InstallBasicPackageFiles)
install_basic_package_files(${PROJECT_NAME}
        VERSION ${${PROJECT_NAME}_VERSION}
        COMPATIBILITY AnyNewerVersion
        VARS_PREFIX ${PROJECT_NAME}
        NO_CHECK_REQUIRED_COMPONENTS_MACRO)

# Add the uninstall target
include(AddUninstallTarget)

# Add integration tests (unit tests for each library should be in each sublibrary directory).
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

# Add targets related to doxygen documention generation
add_subdirectory(doc)

# Add sandbox directory
add_subdirectory(sandbox)
