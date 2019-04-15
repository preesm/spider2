cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -Wall -Wextra -coverage")


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
	 sources/*.cpp
)

file(GLOB_RECURSE
     header_files
	 ${SPIDER_LIB_DIR}/common/*.h
	 ${SPIDER_LIB_DIR}/common/containers/*.h
	 ${SPIDER_LIB_DIR}/common/memory/*.h
	 ${SPIDER_LIB_DIR}/common/memory/abstract/*.h
	 sources/*.h
)

add_executable(test-containers main.cpp ${source_files} ${header_files})
target_link_libraries(test-containers ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
