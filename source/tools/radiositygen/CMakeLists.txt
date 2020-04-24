add_definitions(-DUNICODE -D_UNICODE)

include_directories (${PROJECT_BINARY_DIR}/include)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_executable(asRadiosityGen ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET asRadiosityGen PROPERTY FOLDER "Tools")
target_link_libraries (asRadiosityGen astrengine)
set_property(TARGET asRadiosityGen PROPERTY C_STANDARD 99)