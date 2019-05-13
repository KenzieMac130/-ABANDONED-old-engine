add_definitions(-DUNICODE -D_UNICODE)

include_directories (${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/source/tools/shadercompiler/*.c")
file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/source/tools/shadercompiler/*.cc")
file(GLOB_RECURSE HEADER_FILES "${PROJECT_SOURCE_DIR}/source/tools/shadercompiler/*.h")
add_executable(asShaderCompiler ${SRC_FILES} ${HEADER_FILES})

#target_link_libraries(asShaderCompiler ${SDL2_LIBRARIES})
target_link_libraries (asShaderCompiler astrengine)
set_property(TARGET astrengine PROPERTY CXX_STANDARD 11)

#tests
add_test (RUNS asShaderCompiler)

include (CTest)
set (CTEST_PROJECT_NAME "asShaderCompiler")