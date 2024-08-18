function (cobra_create_test NAME)
	set(SAMPLE_NAME test-${NAME})
	add_executable(${SAMPLE_NAME} Main.cpp)

	target_link_libraries(${SAMPLE_NAME} PRIVATE CobraRHI)
	set_target_properties(${SAMPLE_NAME} PROPERTIES FOLDER "CobraRHI Tests")
	set (TEST $<TARGET_FILE:${SAMPLE_NAME}>)

	add_test(NAME ${SAMPLE_NAME} COMMAND $<TARGET_FILE:${SAMPLE_NAME}>)
	add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/deps/slang/bin/windows/slang.dll $<TARGET_FILE_DIR:${SAMPLE_NAME}>
	)
endfunction()

add_subdirectory(instance)