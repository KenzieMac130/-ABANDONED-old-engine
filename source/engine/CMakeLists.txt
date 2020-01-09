add_definitions(-DUNICODE -D_UNICODE)

set (ASTRENGINE_VERSION_MAJOR 1)
set (ASTRENGINE_VERSION_MINOR 0)
set (ASTRENGINE_VERSION_PATCH 0)
#option (ASTRENGINE_VULKAN ON)
#if(ASTRENGINE_VULKAN)
#	set (ASTRENGINE_VK 1)
#else()
#	set (ASTRENGINE_VK 0)
#endif()
set (ASTRENGINE_VK 1)
set (ASTRENGINE_NUKLEAR 1)

configure_file (
	${PROJECT_SOURCE_DIR}/source/engine/astrengineConfig.h.in
	${PROJECT_BINARY_DIR}/astrengineConfig.h )
include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/source/engine/*.c)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/source/engine/*.h)
add_library(astrengine SHARED ${SRC_FILES} ${HEADER_FILES})
target_link_libraries(astrengine ${SDL2_LIBRARIES})
target_link_libraries(astrengine ${Vulkan_LIBRARIES})

set_property(TARGET astrengine PROPERTY C_STANDARD 99)

install (FILES ${PROJECT_BINARY_DIR}/astrengineConfig.h DESTINATION ${PROJECT_BINARY_DIR}/include/engine)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/source/engine DESTINATION ${PROJECT_BINARY_DIR}/include FILES_MATCHING PATTERN "*.h")

install (FILES ${SDL2_SHARED} DESTINATION ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})