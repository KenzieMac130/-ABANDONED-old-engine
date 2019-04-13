add_definitions(-DUNICODE -D_UNICODE)

include_directories ("${PROJECT_BINARY_DIR}/include")

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/source/tests/*.c")
file(GLOB_RECURSE HEADER_FILES "${PROJECT_SOURCE_DIR}/source/tests/*.h")
add_executable(astrengine_Testbed ${SRC_FILES} ${HEADER_FILES})

set_property(TARGET astrengine_Testbed PROPERTY C_STANDARD 99)
target_link_libraries (astrengine_Testbed astrengine)

#tests
add_test (RUNS astrengine_Testbed)

include (CTest)
set (CTEST_PROJECT_NAME "astrengine_Testbed")