cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sandbox)

# Set flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pedantic-errors -Wall -Wextra -coverage -ftest-coverage -fprofile-arcs")


# Link runTests with what we want to test and the GTest and pthread library
set(SPIDER_LIB_DIR "../../libspider")

include_directories(
        ${GTEST_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ${SPIDER_LIB_DIR}/
)


file(GLOB_RECURSE
        source_files
        ${SPIDER_LIB_DIR}/archi/*.cpp
        ${SPIDER_LIB_DIR}/common/*.cpp
        ${SPIDER_LIB_DIR}/containers/*.cpp
        ${SPIDER_LIB_DIR}/memory/*.cpp
        ${SPIDER_LIB_DIR}/memory/dynamic-allocators/*.cpp
        ${SPIDER_LIB_DIR}/memory/static-allocators/*.cpp
        ${SPIDER_LIB_DIR}/graphs/pisdf/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/brv/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/exporter/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/*.cpp
        ${SPIDER_LIB_DIR}/graphs-tools/numerical/*.cpp
        ${SPIDER_LIB_DIR}/runtime/distributed/*.cpp
        ${SPIDER_LIB_DIR}/runtime/master-slave/*.cpp
        ${SPIDER_LIB_DIR}/spider-api/*.cpp
        ${SPIDER_LIB_DIR}/scheduling/*.cpp
        ${SPIDER_LIB_DIR}/scenario/*.cpp
        )

file(GLOB_RECURSE
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
        ${SPIDER_LIB_DIR}/graphs-tools/exporter/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/expression-parser/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/transformation/optims/*.h
        ${SPIDER_LIB_DIR}/graphs-tools/numerical/*.h
        ${SPIDER_LIB_DIR}/runtime/*.h
        ${SPIDER_LIB_DIR}/runtime/distributed/*.h
        ${SPIDER_LIB_DIR}/runtime/master-slave/*.h
        ${SPIDER_LIB_DIR}/spider-api/*.h
        ${SPIDER_LIB_DIR}/scheduling/*.h
        ${SPIDER_LIB_DIR}/scenario/*.h
        ${SPIDER_LIB_DIR}/spider.h
        )

add_executable(sandbox main.cpp Test.h ${source_files} ${header_files})
target_link_libraries(sandbox ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
