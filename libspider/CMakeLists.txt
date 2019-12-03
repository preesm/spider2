cmake_minimum_required(VERSION 3.12)
project(Spider2)

# Setting C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setting extra modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/extras/cmake_modules/")

# Setting build options
if (NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set(SPIDER_WARNING_FLAGS "-std=gnu++11 -Wall -Wextra -pedantic-errors -Wnon-virtual-dtor -Wconversion -Wformat")
    if (${CMAKE_BUILD_TYPE} MATCHES Debug)
        message("Generating Debug project")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -O0 ${SPIDER_WARNING_FLAGS} --coverage")
    else ()
        message("Generating Release project")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 ${SPIDER_WARNING_FLAGS}")
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            message("Using Link Time Optimization on GNU platforms. ")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        endif ()
    endif ()
else ()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()


# *******************************************
# ************* CMake Content ***************
# *******************************************
# This CMake create a workspace containing the following projects
# 
# Programs
#  - spider

# CMake options
option(CMAKE_TYPE_SAFE "CMAKE_TYPE_SAFE" OFF)
option(CMAKE_ENABLE_PAPI "CMAKE_ENABLE_PAPI" OFF)
option(CMAKE_CHECK_ALLOCATOR "CMAKE_CHECK_ALLOCATOR" OFF)

# CMake cross compilation stuff
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

# IDE dependent config
if (${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    message("Add definition for ${CMAKE_GENERATOR}")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (${CMAKE_GENERATOR} MATCHES "MinGW Makefiles")
    message("Add definition for ${CMAKE_GENERATOR}")
    #To prevent a redefinition conflict
    add_definitions(-D_TIMESPEC_DEFINED)
endif ()

# Add definition for relative path into project
add_definitions(-DPROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

if (${CMAKE_TYPE_SAFE} MATCHES OFF)
    message("Using standard library printf without type checking.")
    add_definitions(-D_SPIDER_NO_TYPESAFETY_PRINT)
else ()
    message("Using type safe printf. (this will increase library binary size)")
endif ()

if (${CMAKE_ENABLE_PAPI} MATCHES ON)
    message("Using PAPI library (if available).")
    set(SKIP_PAPI 0)
else ()
    message("Discarding PAPI library.")
    set(SKIP_PAPI 1)
endif ()

if (${CMAKE_CHECK_ALLOCATOR} MATCHES ON)
    message("Enabling allocator check. This may slow down execution")
    add_definitions(-D_SPIDER_CHECK_ALLOCATOR)
endif()

# Build specific options
if (WIN32)
    set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extras/)
endif ()

# Searching external libraries

# *******************************************
# ************ Pthread LIBRARY **************
# *******************************************
if (WIN32 AND NOT MINGW)
    set(THREADS_USE_PTHREADS_WIN32 true)
    # pthread included AFTER Sdl to avoid unnecessary warnings
    file(GLOB PTHREADDIR "${LIBS_DIR}/pthread-[\\.|0-9]*")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PTHREADDIR})
    Find_Package(Threads REQUIRED)
else ()
    Find_Package(Threads REQUIRED)
endif ()

if (NOT THREADS_FOUND)
    message(FATAL_ERROR "Pthread not found !")
endif ()

if (WIN32)
    file(GLOB
            PTHREADS_DLL
            ${CMAKE_PREFIX_PATH}/lib/*.dll
            )

    message("Copy Pthreads DLLs into ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    if (NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
        file(COPY ${PTHREADS_DLL} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    else ()
        file(COPY ${PTHREADS_DLL} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
        file(COPY ${PTHREADS_DLL} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
    endif ()
endif ()

# *******************************************
# ************ PAPI LIBRARY *****************
# *******************************************
if (${SKIP_PAPI} MATCHES 0)
    find_package(PAPI)
endif ()

if (PAPI_FOUND)
    add_definitions(-D_SPIDER_PAPI_AVAILABLE)
    set(papify_sources
            libspider/papify/PapifyAction.cpp
            libspider/papify/PapifyAction.h
            libspider/papify/PapifyEventLib.cpp
            libspider/papify/PapifyEventLib.h)
else ()
    set(PAPI_LIBRARIES "")
    set(PAPI_INCLUDE_DIRS "")
endif ()


# *******************************************
# *************  spider.dll/a  **************
# *******************************************
set(SPIDER_LIB_DIR "./libspider/")

include_directories(
        ${PAPI_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ${SPIDER_LIB_DIR}/
)

file(
        GLOB_RECURSE
        source_files
        ${SPIDER_LIB_DIR}/*.cpp
)

file(
        GLOB_RECURSE
        header_files
        ${SPIDER_LIB_DIR}/*.h
)

set(ALL_MY_SOURCE_FILES ${source_files} ${header_files})
add_custom_target(
        clang-tidy-8
        COMMAND /usr/bin/clang-tidy-8
        ${ALL_MY_SOURCE_FILES}
        -config=''
        -fix-errors
        --
        -std=c++11
        -I${MY_SOURCE_INCLUDE_DIR}
        -I${MY_MPI_INCLUDE_DIR1}
        -I${MY_MPI_INCLUDE_DIR2}
        -I${MY_MPI_INCLUDE_DIR3}
        -I${MY_BOOST_INCLUDE_DIR}
        -I${MY_VTK_INCLUDE_DIR}
)

add_library(Spider2 SHARED ${source_files} ${header_files})
target_link_libraries(Spider2 ${CMAKE_THREAD_LIBS_INIT} ${PAPI_LIBRARIES})

if (NOT MINGW AND NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set(CMAKE_CXX_FLAGS "-fPIC")
    if (${32BITS})
        set_target_properties(Spider2 PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
    else ()
        if (${64BITS})
            set_target_properties(Spider2 PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
        endif ()
    endif ()
endif ()
