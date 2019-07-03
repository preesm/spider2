cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate GTest
find_package(GTest REQUIRED)

# Set flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -Wextra -coverage -ftest-coverage -fprofile-arcs")


# Link runTests with what we want to test and the GTest and pthread library
set(SPIDER_LIB_DIR "../../../libspider")

include_directories(
        ${GTEST_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ${SPIDER_LIB_DIR}/
        sources
)


file(GLOB_RECURSE
        source_files
        ${SPIDER_LIB_DIR}/common/*.cpp
        ${SPIDER_LIB_DIR}/common/containers/*.cpp
        ${SPIDER_LIB_DIR}/common/memory/*.cpp
        ${SPIDER_LIB_DIR}/common/memory/dynamic-allocators/*.cpp
        ${SPIDER_LIB_DIR}/common/memory/static-allocators/*.cpp
        sources/*.cpp
        )

file(GLOB_RECURSE
        header_files
        ${SPIDER_LIB_DIR}/common/*.h
        ${SPIDER_LIB_DIR}/common/containers/*.h
        ${SPIDER_LIB_DIR}/common/memory/*.h
        ${SPIDER_LIB_DIR}/common/memory/abstract-allocators/*.h
        ${SPIDER_LIB_DIR}/common/memory/dynamic-allocators/*.h
        ${SPIDER_LIB_DIR}/common/memory/static-allocators/*.h
        sources/*.h
        )

add_executable(test-containers main.cpp ${source_files} ${header_files})
target_link_libraries(test-containers ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
