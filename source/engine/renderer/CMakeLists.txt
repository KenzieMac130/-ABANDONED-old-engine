if(ASTRENGINE_VK)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vulkan)
endif()

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(asRenderer ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asRenderer PROPERTY FOLDER "astrengine/Modules/Renderer")
set_property(TARGET asRenderer PROPERTY C_STANDARD 99)

if(ASTRENGINE_VK)
	target_link_libraries (asRenderer asVulkanBackend)
endif()

target_link_libraries (asRenderer asResource)
target_link_libraries (asRenderer asModelRuntime)
target_link_libraries (asRenderer thirdParty_tiny_ktx)