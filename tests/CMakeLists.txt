function (cobra_create_test NAME)
	set(SAMPLE_NAME test-${NAME})
	add_executable(${SAMPLE_NAME} Main.cpp)

	target_link_libraries(${SAMPLE_NAME} PRIVATE CobraRHI)
	set_target_properties(${SAMPLE_NAME} PROPERTIES FOLDER "CobraRHI Tests")
	set (TEST $<TARGET_FILE:${SAMPLE_NAME}>)

	add_test(NAME ${SAMPLE_NAME} COMMAND $<TARGET_FILE:${SAMPLE_NAME}> WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	add_custom_command(TARGET ${SAMPLE_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/deps/slang/bin/windows/slang.dll $<TARGET_FILE_DIR:${SAMPLE_NAME}>
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/deps/slang/bin/windows/slang-glslang.dll $<TARGET_FILE_DIR:${SAMPLE_NAME}>

		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${SAMPLE_NAME}>/D3D12
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/deps/agility/bin/x64/D3D12Core.dll $<TARGET_FILE_DIR:${SAMPLE_NAME}>/D3D12		
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/deps/agility/bin/x64/d3d12SDKLayers.dll $<TARGET_FILE_DIR:${SAMPLE_NAME}>/D3D12
	)
endfunction()

add_subdirectory(instance)
add_subdirectory(shader)
add_subdirectory(triangle)