cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# *******************************************
# ************* CMake Content ***************
# *******************************************
# This CMake create a workspace containing the following projects
# 
# Programs
#  - spider

# IDE dependent config
if (${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    MESSAGE("Add definition for ${CMAKE_GENERATOR}")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

if (${CMAKE_GENERATOR} MATCHES "MinGW Makefiles")
    MESSAGE("Add definition for ${CMAKE_GENERATOR}")
    #To prevent a redefinition conflict
    add_definitions(-D_TIMESPEC_DEFINED)
endif ()

# Add definition for relative path into project
add_definitions(-DPROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

project(Spider2)

# Set to 1 to activate debug compilation (0 for release)

if (NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set(DEBUG 1)

    if (${DEBUG})
        MESSAGE("Generate Debug project")
        set(CMAKE_BUILD_TYPE Debug)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -Wall -Wextra -O0 -pedantic-errors")
    else ()
        MESSAGE("Generate Release project")
        set(CMAKE_BUILD_TYPE Release)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall -Wextra")
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        endif ()
    endif ()
else ()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif ()

if (WIN32)
    set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extras/)
endif ()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/extras/cmake_modules/")

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
    MESSAGE(FATAL_ERROR "Pthread not found !")
endif ()


if (WIN32)
    file(GLOB
            PTHREADS_DLL
            ${CMAKE_PREFIX_PATH}/lib/*.dll
            )

    MESSAGE("Copy Pthreads DLLs into ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
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
set(SKIP_PAPI 1)

if (${SKIP_PAPI})
    MESSAGE("Skipping Papi")
else ()
    find_package(PAPI)
endif ()

if (PAPI_FOUND)
    add_definitions(-DPAPI_AVAILABLE)
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
        ${SPIDER_LIB_DIR}/archi/*.cpp
        ${SPIDER_LIB_DIR}/common/*.cpp
        ${SPIDER_LIB_DIR}/containers/*.cpp
        ${SPIDER_LIB_DIR}/memory/*.cpp
        ${SPIDER_LIB_DIR}/memory/dynamic-allocators/*.cpp
        ${SPIDER_LIB_DIR}/memory/static-allocators/*.cpp
        ${SPIDER_LIB_DIR}/graphs/pisdf/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/brv/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/numerical/*.cpp
        ${SPIDER_LIB_DIR}/scenario/*.cpp
        ${SPIDER_LIB_DIR}/scheduling/*.cpp
        ${SPIDER_LIB_DIR}/scheduling/allocator/*.cpp
        ${SPIDER_LIB_DIR}/scheduling/schedule/*.cpp
        ${SPIDER_LIB_DIR}/scheduling/scheduler/*.cpp
        ${SPIDER_LIB_DIR}/runtime/distributed/*.cpp
        ${SPIDER_LIB_DIR}/runtime/master-slave/*.cpp
        ${SPIDER_LIB_DIR}/spider-api/*.cpp
)

file(
        GLOB_RECURSE
        header_files
        ${SPIDER_LIB_DIR}/archi/*.h
        ${SPIDER_LIB_DIR}/common/*.h
        ${SPIDER_LIB_DIR}/common/cxx11-printf/*.h
        ${SPIDER_LIB_DIR}/containers/*.h
        ${SPIDER_LIB_DIR}/memory/*.h
        ${SPIDER_LIB_DIR}/memory/abstract-allocators/*.h
        ${SPIDER_LIB_DIR}/memory/dynamic-allocators/*.h
        ${SPIDER_LIB_DIR}/memory/static-allocators/*.h
        ${SPIDER_LIB_DIR}/graphs/pisdf/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/brv/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/optims/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/numerical/*.h
        ${SPIDER_LIB_DIR}/runtime/*.h
        ${SPIDER_LIB_DIR}/runtime/distributed/*.h
        ${SPIDER_LIB_DIR}/runtime/master-slave/*.h
        ${SPIDER_LIB_DIR}/scenario/*.h
        ${SPIDER_LIB_DIR}/scheduling/*.h
        ${SPIDER_LIB_DIR}/scheduling/allocator/*.h
        ${SPIDER_LIB_DIR}/scheduling/schedule/*.h
        ${SPIDER_LIB_DIR}/scheduling/scheduler/*.h
        ${SPIDER_LIB_DIR}/spider-api/*.h
        ${SPIDER_LIB_DIR}/spider.h
)

set(ALL_MY_SOURCE_FILES ${source_files} ${header_files})
add_custom_target(
        clang-tidy
        COMMAND /usr/bin/clang-tidy
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

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
