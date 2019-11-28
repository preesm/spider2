cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(spider2.0-tests)

# Macro to get short file name (vs full path) at compile time
# The additional / is important to remove the last character from the path.
# Note that it does not matter if the OS uses / or \, because we are only
# saving the path size.
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")
add_definitions("-D__FILENAME__=(__FILE__ + ${SOURCE_PATH_SIZE})")

# Locate GTest
find_package(GTest REQUIRED)

# Set flags
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -pg -Wall -Wextra -coverage -ftest-coverage -fprofile-arcs")


# Link runTests with what we want to test and the GTest and pthread library
set(SPIDER_INCLUDE_DIR "../../libspider")
set(SPIDER_LIB_DIR "../../bin")

include_directories(
        ${GTEST_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ${SPIDER_INCLUDE_DIR}/
        source
)

file(GLOB_RECURSE
        header_files
        ${SPIDER_INCLUDE_DIR}/*.h
        )

file(GLOB_RECURSE
        source_files
        ${CMAKE_SOURCE_DIR}/source/*.cpp
        )

find_library(SPIDER_LIBRARY
        NAMES Spider2
        HINTS
        ${SPIDER_LIB_DIR}
        PATH_SUFFIXES lib
        PATHS ${SPIDER_SEARCH_PATHS}
        )
message("SPIDER_LIBRARY : ${SPIDER_LIBRARY}")

add_executable(all-spider2.0-tests source/main.cpp ${source_files} ${header_files})
target_link_libraries(all-spider2.0-tests ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SPIDER_LIBRARY} gcov)

add_executable(containers-spider2.0-tests source/main.cpp source/containersTest.cpp ${header_files})
target_link_libraries(containers-spider2.0-tests ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SPIDER_LIBRARY} gcov)

add_executable(allocator-spider2.0-tests source/main.cpp source/allocatorTest.cpp ${header_files})
target_link_libraries(allocator-spider2.0-tests ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SPIDER_LIBRARY} gcov)

add_executable(expression-spider2.0-tests source/main.cpp source/expressionTest.cpp ${header_files})
target_link_libraries(expression-spider2.0-tests ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${SPIDER_LIBRARY} gcov)
