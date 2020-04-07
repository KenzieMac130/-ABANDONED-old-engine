#ADD PROJECT
file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_library(astrengine SHARED ${SRC_FILES} ${HEADER_FILES})
set_property(TARGET astrengine PROPERTY C_STANDARD 99)
set_property(TARGET astrengine PROPERTY FOLDER "astrengine")

#LINK SUBLIBRARIES
target_link_libraries(astrengine asResource)
target_link_libraries(astrengine asRenderer)
#target_link_libraries(astrengine asNuklear)
target_link_libraries(astrengine asDearImgui)
target_link_libraries(astrengine asFlecs)

#LINK GUI TOOLS
target_link_libraries(astrengine asCmdConsole)