message(STATUS "----Configuring the Aviso Compiler----")
function(get_gcc_prop prop OUTVAR)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} "-${prop}"
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE gcc_prop
    )
  set(${OUTVAR} ${gcc_prop} PARENT_SCOPE)
endfunction()

function(check_gcc_version)
  get_gcc_prop(dumpversion VER)
  set(VER__ "${VER}.0.0")

  string(REGEX MATCHALL "[0-9]+" VERS ${VER__})

  list(GET VERS 0 V0)
  set(GCC_MAJOR ${V0} PARENT_SCOPE)
  list(GET VERS 1 V1)
  set(GCC_MINOR ${V1} PARENT_SCOPE)
  list(GET VERS 2 V2)
  set(GCC_MICRO ${V2} PARENT_SCOPE)

  message(STATUS "GCC ${VER} detected")
  if(${VER} VERSION_LESS "4.7.0")
    message(FATAL_ERROR "GCC 4.7.0 or greater is required to build AvisoCompiler.")
  endif(${VER} VERSION_LESS "4.7.0")

endfunction()

if(NOT ${CMAKE_COMPILER_IS_GNUC})
message(FATAL_ERROR "AvisoCompiler requires the GNU C Compiler")
endif(NOT ${CMAKE_COMPILER_IS_GNUC})

if(NOT ${CMAKE_COMPILER_IS_GNUCXX})
message(FATAL_ERROR "AvisoCompiler requires the GNU CXX Compiler")
endif(NOT ${CMAKE_COMPILER_IS_GNUCXX})

check_gcc_version()

get_gcc_prop("-print-file-name=plugin" GCC_PLUGIN_DIR)
message(STATUS "GCC's plugin directory is ${GCC_PLUGIN_DIR}")
include_directories("${GCC_PLUGIN_DIR}/include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -shared")

#building just a library. 
add_library(aviso SHARED aviso.c myexpand.cpp)
set_target_properties(aviso PROPERTIES PREFIX "")
message(STATUS "----Done Configuring the Aviso Compiler----")
