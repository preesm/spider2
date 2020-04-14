# Set all-test name
set(ALL_TEST_TARGET_NAME all-${PROJECT_NAME}-test)

# Add all sub-test directories
add_subdirectory(allocator-test)
add_subdirectory(containers-test)
add_subdirectory(expression-test)
add_subdirectory(pisdf-test)
add_subdirectory(math-test)
add_subdirectory(srdag-test)
add_subdirectory(runtime-test)

# Add the test files
file(
        GLOB_RECURSE
        ${ALL_TEST_TARGET_NAME}_SRC
        *.cpp
        *.h
)

# Set the include directories to use <> instead of ""
include_directories(
        ${PAPI_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ../libspider/
)

# On GNU add compile flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftest-coverage -fprofile-arcs")
endif ()

add_executable(${ALL_TEST_TARGET_NAME} ${${ALL_TEST_TARGET_NAME}_SRC})
target_link_libraries(${ALL_TEST_TARGET_NAME} ${PROJECT_NAME}::${PROJECT_NAME} gtest_main gmock_main)
