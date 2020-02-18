file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(asVulkanBackend ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asVulkanBackend PROPERTY FOLDER "astrengine/Modules/Renderer/Vulkan")
set_property(TARGET asVulkanBackend PROPERTY C_STANDARD 99)

target_link_libraries (asVulkanBackend asResource)