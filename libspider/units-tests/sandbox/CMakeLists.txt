cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sandbox)

# Set flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pedantic-errors -Wall -Wextra -coverage -ftest-coverage -fprofile-arcs")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -pedantic-errors -Wall -Wextra")


# Link runTests with what we want to test and the GTest and pthread library
set(SPIDER_INCLUDE_DIR "../../libspider/")
set(SPIDER_LIB_DIR "../../cmake-build-debug/")

include_directories(
        ${GTEST_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ${SPIDER_INCLUDE_DIR}/
)

Find_Package(Threads REQUIRED)

file(GLOB_RECURSE
        header_files
        ${SPIDER_INCLUDE_DIR}/*.h
        )

find_library(SPIDER_LIBRARY
        NAMES Spider2
        HINTS
        ${SPIDER_LIB_DIR}
        PATH_SUFFIXES lib
        PATHS ${SPIDER_SEARCH_PATHS}
        )
message("SPIDER_LIBRARY : ${SPIDER_LIBRARY}")

add_executable(sandbox main.cpp ${header_files})
target_link_libraries(sandbox ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SPIDER_LIBRARY})
