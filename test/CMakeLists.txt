set(ALL_TEST_TARGET_NAME all-${PROJECT_NAME}-test)

# Locate GTest
find_package(GTest REQUIRED)

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
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ftest-coverage -fprofile-arcs")
endif ()

add_executable(${ALL_TEST_TARGET_NAME} ${${ALL_TEST_TARGET_NAME}_SRC})
target_link_libraries(${ALL_TEST_TARGET_NAME} ${GTEST_LIBRARIES} ${PROJECT_NAME}::${PROJECT_NAME})

# Add a test to the project to be run by ctest.
# See https://cmake.org/cmake/help/latest/command/add_test.html
# See https://cmake.org/cmake/help/latest/manual/ctest.1.html
# COMMAND tag specifies the test command-line. If it is an executable target
# created by add_executable(), it will automatically be replaced by the location
# of the executable created at build time.
add_test(NAME ${ALL_TEST_TARGET_NAME}
         COMMAND ${ALL_TEST_TARGET_NAME})
         
# Adding specific test targets
set(EXPR_TARGET_NAME expression-${PROJECT_NAME}-test)
set(${EXPR_TARGET_NAME}_SRC main.cpp expressionTest.cpp)
add_executable(${EXPR_TARGET_NAME} ${${EXPR_TARGET_NAME}_SRC})
target_link_libraries(${EXPR_TARGET_NAME} ${GTEST_LIBRARIES} ${PROJECT_NAME}::${PROJECT_NAME})

set(CONTAINERS_TARGET_NAME containers-${PROJECT_NAME}-test)
set(${CONTAINERS_TARGET_NAME}_SRC main.cpp containersTest.cpp)
add_executable(${CONTAINERS_TARGET_NAME} ${${CONTAINERS_TARGET_NAME}_SRC})
target_link_libraries(${CONTAINERS_TARGET_NAME} ${GTEST_LIBRARIES} ${PROJECT_NAME}::${PROJECT_NAME})

set(RPNCONVERTER_TARGET_NAME rpnconverter-${PROJECT_NAME}-test)
set(${RPNCONVERTER_TARGET_NAME}_SRC main.cpp rpnconverterTest.cpp)
add_executable(${RPNCONVERTER_TARGET_NAME} ${${RPNCONVERTER_TARGET_NAME}_SRC})
target_link_libraries(${RPNCONVERTER_TARGET_NAME} ${GTEST_LIBRARIES} ${PROJECT_NAME}::${PROJECT_NAME})

set(ALLOCATOR_TARGET_NAME allocator-${PROJECT_NAME}-test)
set(${ALLOCATOR_TARGET_NAME}_SRC main.cpp allocatorTest.cpp)
add_executable(${ALLOCATOR_TARGET_NAME} ${${ALLOCATOR_TARGET_NAME}_SRC})
target_link_libraries(${ALLOCATOR_TARGET_NAME} ${GTEST_LIBRARIES} ${PROJECT_NAME}::${PROJECT_NAME})
