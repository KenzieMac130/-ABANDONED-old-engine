option(BUILD_TOOL_SHADERCOMPILER "Build the shader compiler" ON)
option(BUILD_TOOL_RESOURCEPACKAGE "Build the resource package tool" ON)

if(BUILD_TOOL_SHADERCOMPILER)
	#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/shadercompiler)
endif()

if(BUILD_TOOL_SHADERCOMPILER)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/resourcepackage)
endif()
