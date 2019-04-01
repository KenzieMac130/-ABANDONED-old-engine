option(BUILD_DOC "Build Doxygen Docs" ON)
find_package(Doxygen)
if(DOXYGEN_FOUND)
	set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/docs/Doxyfile.in)
	set(DOXYGEN_OUT ${PROJECT_BINARY_DIR}/Doxyfile)
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
	add_custom_target( build_docs ALL
       COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
       WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
       COMMENT "Generating API documentation with Doxygen"
       VERBATIM )
else(DOXYGEN_FOUND)
	message("Doxygen not found")
endif(DOXYGEN_FOUND)