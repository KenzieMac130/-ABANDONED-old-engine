file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_library(thirdParty_xxHash ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET thirdParty_xxHash PROPERTY FOLDER "ThirdParty")
set_property(TARGET thirdParty_xxHash PROPERTY C_STANDARD 99)