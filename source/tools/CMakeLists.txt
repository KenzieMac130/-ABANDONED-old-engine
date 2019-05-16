option(BUILD_TOOL_SHADERCOMPILER "Build the shader compiler" ON)

if(BUILD_TOOL_SHADERCOMPILER)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/shadercompiler)
endif()