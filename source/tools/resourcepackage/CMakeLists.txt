add_definitions(-DUNICODE -D_UNICODE)

include_directories (${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/source/tools/resourcepackage/*.c")
#file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/source/tools/resourcepackage/*.cc)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/source/tools/resourcepackage/*.h)
add_executable(asResourcePackager ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asResourcePackager PROPERTY FOLDER "Tools")

#target_link_libraries(asResourcePackager ${SDL2_LIBRARIES})
target_link_libraries (asResourcePackager astrengine)
set_property(TARGET asResourcePackager PROPERTY C_STANDARD 99)
#set_property(TARGET astrengine PROPERTY CXX_STANDARD 11)

#tests
add_test (RUNS asResourcePackager)

include (CTest)
set (CTEST_PROJECT_NAME "asResourcePackager")