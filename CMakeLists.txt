cmake_minimum_required(VERSION 2.6)
project(AVISO)

INCLUDE(ExternalProject)

set(CMAKE_CXX_FLAGS "-g -O2")
set(CMAKE_C_FLAGS "-g -O2")


add_subdirectory(Runtime)
add_subdirectory(AvisoCompiler)

ExternalProject_Add(avisoprof
PREFIX ${CMAKE_SOURCE_DIR}/AvisoProf
SOURCE_DIR ${CMAKE_SOURCE_DIR}/AvisoProf
TMP_DIR ${CMAKE_SOURCE_DIR}/AvisoProf/.tmp
STAMP_DIR ${CMAKE_SOURCE_DIR}/AvisoProf/.cmake_stamp
BUILD_IN_SOURCE 1
CONFIGURE_COMMAND "echo" "Building AvisoProf against $(PIN_ROOT)"
BUILD_COMMAND "/bin/bash" "make.sh"
INSTALL_COMMAND "echo" "Installed AvisoProf"
)
