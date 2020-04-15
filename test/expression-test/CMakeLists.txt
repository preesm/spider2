set(EXPR_TARGET_NAME expression-${PROJECT_NAME}-test)

# Add the test files
set(${EXPR_TARGET_NAME}_SRC ../main.cpp expressionTest.cpp)

# Set the include directories to use <> instead of ""
include_directories(
        ${PAPI_INCLUDE_DIRS}
        ${PTHREADDIR}/include
        ../../libspider/include
)

# On GNU add compile flags
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftest-coverage -fprofile-arcs")
endif ()

add_executable(${EXPR_TARGET_NAME} ${${EXPR_TARGET_NAME}_SRC})
target_link_libraries(${EXPR_TARGET_NAME} gtest_main gmock_main ${PROJECT_NAME}::${PROJECT_NAME})

# Add a test to the project to be run by ctest.
# See https://cmake.org/cmake/help/latest/command/add_test.html
# See https://cmake.org/cmake/help/latest/manual/ctest.1.html
# COMMAND tag specifies the test command-line. If it is an executable target
# created by add_executable(), it will automatically be replaced by the location
# of the executable created at build time.
add_test(NAME ${EXPR_TARGET_NAME}
        COMMAND ${EXPR_TARGET_NAME})
