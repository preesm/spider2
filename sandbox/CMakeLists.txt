set(SANDBOX_TARGET_NAME ${PROJECT_NAME}-sandbox)

# Add the test files
file(
	GLOB_RECURSE
	${SANDBOX_TARGET_NAME}_SRC
	*.cpp
	*.h
)

add_executable(${SANDBOX_TARGET_NAME} ${${SANDBOX_TARGET_NAME}_SRC})
target_link_libraries(${SANDBOX_TARGET_NAME} ${PROJECT_NAME}::${PROJECT_NAME})
