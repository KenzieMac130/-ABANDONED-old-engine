file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(asCommon ${SRC_FILES} ${HEADER_FILES})

set_property(TARGET asCommon PROPERTY FOLDER "astrengine/Modules")
set_property(TARGET asCommon PROPERTY C_STANDARD 99)
target_link_libraries(asCommon thirdParty_tinyRegex)
target_link_libraries(asCommon thirdParty_xxHash)