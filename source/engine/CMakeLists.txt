#ENFORCE UNICODE
add_definitions(-DUNICODE -D_UNICODE)

#-----CONFIG FILE-----
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
set (ASTRENGINE_NUKLEAR 0)
set (ASTRENGINE_DEARIMGUI 1)
set (ASTRENGINE_FLECS 0)

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
	set (AS_ENDIAN 0xFF)
else()
	set (AS_ENDIAN 0x00)
endif()

configure_file (
	${PROJECT_SOURCE_DIR}/source/engine/astrengineConfig.h.in
	${PROJECT_BINARY_DIR}/astrengineConfig.h )
	
##-----GLOBAL INCLUDE DIRECTORIES#-----
include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/source/thirdparty)

#-----LINK GLOBAL LIBRARIES-----
link_libraries(${SDL2_LIBRARIES})
if(ASTRENGINE_VK)
	#Not ideal but some modules might use raw Vulkan
	link_libraries(${Vulkan_LIBRARIES})
endif()

#-----ADD SUBPROJECTS (SHOWS A DEPENDENCY HEIRARCHY-----
#asCore
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common) 
	link_libraries(asCommon) #link core to all other libraries
#asResource
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/resource)
#asInput
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/input)
#asRenderer
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/renderer) 
#Mesh API
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/model)
#asNuklear
if(ASTRENGINE_NUKLEAR)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nuklear)
endif()
#asDearImgui
if(ASTRENGINE_DEARIMGUI)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cimgui)
endif()
#Gui Tools
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/guiTools)
#asFlecs
if(ASTRENGINE_FLECS)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/flecs)
endif()

#asGameUtilities
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gameUtilities)
#astrengine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engineCore) 


#-----INSTALLATION-----
install (FILES ${PROJECT_BINARY_DIR}/astrengineConfig.h DESTINATION ${PROJECT_BINARY_DIR}/include/engine)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/source/engine DESTINATION ${PROJECT_BINARY_DIR}/include FILES_MATCHING PATTERN "*.h")
#install (FILES ${SDL2_SHARED} DESTINATION ${PROJECT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})