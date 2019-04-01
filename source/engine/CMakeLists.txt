add_definitions(-DUNICODE -D_UNICODE)

set (ASTRENGINE_VERSION_MAJOR 1)
set (ASTRENGINE_VERSION_MINOR 0)

configure_file (
	"${PROJECT_SOURCE_DIR}/source/engine/astrengineConfig.h.in"
	"${PROJECT_BINARY_DIR}/astrengineConfig.h" )
include_directories("${PROJECT_BINARY_DIR}")

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/source/engine/*.c")
file(GLOB_RECURSE HEADER_FILES "${PROJECT_SOURCE_DIR}/source/engine/*.h")
add_library(astrengine ${SRC_FILES} ${HEADER_FILES})

set_property(TARGET astrengine PROPERTY C_STANDARD 99)

install (TARGETS astrengine DESTINATION ${PROJECT_BINARY_DIR}/bin)
install (FILES "${PROJECT_BINARY_DIR}/astrengineConfig.h" DESTINATION ${PROJECT_BINARY_DIR}/include/engine)
file(GLOB_RECURSE INCLUDEHEADER_FILES "${PROJECT_SOURCE_DIR}/source/engine/include/*.h")
install (FILES ${INCLUDEHEADER_FILES} DESTINATION ${PROJECT_BINARY_DIR}/include/engine)