file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_library(thirdParty_tinyRegex ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET thirdParty_tinyRegex PROPERTY FOLDER "ThirdParty")
set_property(TARGET thirdParty_tinyRegex PROPERTY C_STANDARD 99)