find_library(LIBSHADERC_LIBRARIES 
NAMES shaderc_shared.lib
HINTS ${PROJECT_SOURCE_DIR}/../libs/shaderc/lib/ REQUIRED)
find_path(LIBSHADERC_INCLUDE_DIRS 
NAMES shaderc
HINTS ${PROJECT_SOURCE_DIR}/../libs/shaderc/include REQUIRED)
find_file(LIBSHADERC_DLL 
NAMES shaderc_shared.dll
HINTS ${PROJECT_SOURCE_DIR}/../libs/shaderc/bin/ REQUIRED)


add_definitions(-DUNICODE -D_UNICODE)

include_directories (${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)
include_directories (${LIBSHADERC_INCLUDE_DIRS})

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_executable(asShaderCompiler ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asShaderCompiler PROPERTY FOLDER "Tools")
set_property(TARGET asShaderCompiler PROPERTY C_STANDARD 99)

target_link_libraries(asShaderCompiler ${SDL2_LIBRARIES})
target_link_libraries (asShaderCompiler astrengine)
target_link_libraries(asShaderCompiler ${LIBSHADERC_LIBRARIES})