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
        ${SPIDER_LIB_DIR}/graphs/pisdf/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.cpp
        ${SPIDER_LIB_DIR}/common/*.cpp
        ${SPIDER_LIB_DIR}/memory/*.cpp
        ${SPIDER_LIB_DIR}/memory/static-allocators/*.cpp
        ${SPIDER_LIB_DIR}/memory/dynamic-allocators/*.cpp
        ${SPIDER_LIB_DIR}/spider-api/pisdf.cpp
        ${SPIDER_LIB_DIR}/spider-api/config.cpp
        sources/*.cpp
    )

file(GLOB_RECURSE
        header_files
        ${SPIDER_LIB_DIR}/graphs/pisdf/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.h
        ${SPIDER_LIB_DIR}/common/*.h
        ${SPIDER_LIB_DIR}/memory/*.h
        ${SPIDER_LIB_DIR}/memory/abstract-allocators/*.h
        ${SPIDER_LIB_DIR}/memory/static-allocators/*.h
        ${SPIDER_LIB_DIR}/memory/dynamic-allocators/*.h
        ${SPIDER_LIB_DIR}/spider-api/pisdf.h
        ${SPIDER_LIB_DIR}/spider-api/config.h
        sources/*.h
        )

add_executable(test-expression-parser main.cpp ${source_files} ${header_files})
target_link_libraries(test-expression-parser ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
