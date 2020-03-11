find_library(LIBSHADERC_LIBRARIES REQUIRED)
find_path(LIBSHADERC_INCLUDE_DIRS REQUIRED)
find_file(LIBSHADERC_DLL REQUIRED)

add_definitions(-DUNICODE -D_UNICODE)

include_directories (${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)
include_directories (${LIBSHADERC_INCLUDE_DIRS})

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/source/tools/shadercompiler/*.c)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/source/tools/shadercompiler/*.h)
add_executable(asShaderCompiler ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asShaderCompiler PROPERTY FOLDER "Tools")

#target_link_libraries(asShaderCompiler ${SDL2_LIBRARIES})
target_link_libraries (asShaderCompiler astrengine)
target_link_libraries(asShaderCompiler ${LIBSHADERC_LIBRARIES})
set_property(TARGET astrengine PROPERTY C_STANDARD 99)
#set_property(TARGET astrengine PROPERTY CXX_STANDARD 11)

#tests
add_test (RUNS asShaderCompiler)

include (CTest)
set (CTEST_PROJECT_NAME "asShaderCompiler")