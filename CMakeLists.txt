cmake_minimum_required(VERSION 3.9)
project(astrengine)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#Clang is the default compiler
set(GLOBAL CMAKE_C_COMPILER clang)
#set(CMAKE_C_COMPILER_TARGET )
set(GLOBAL CMAKE_CXX_COMPILER clang++)
#set(CMAKE_CXX_COMPILER )

add_subdirectory(${PROJECT_SOURCE_DIR}/docs)
add_subdirectory(${PROJECT_SOURCE_DIR}/source)