file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(asDearImgui ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asDearImgui PROPERTY FOLDER "astrengine/Modules")
set_property(TARGET asDearImgui PROPERTY C_STANDARD 99)

target_link_libraries (asDearImgui asResource)
target_link_libraries (asDearImgui asRenderer)
target_link_libraries(asDearImgui thirdParty_cimgui)