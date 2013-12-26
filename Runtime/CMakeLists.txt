set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_LD_FLAGS "-shared")

include_directories(${CMAKEDEMO_SOURCE_DIR}/Runtime)
link_directories(${CMAKEDEMO_BINARY_DIR}/Runtime)

#these are all compiled the same way
set(IRPTROBJS Applier.cpp Avoidance.cpp ClockPortability.cpp Constructor.cpp CorrectRunModel.cpp EventRoutines.cpp GetBacktrace.cpp IR_PThreadsRuntime.cpp LogicalTimeKeeper.cpp PluginProcessing.cpp STQueue.cpp)

#building just a library. 
add_library(IRPTR SHARED ${IRPTROBJS})
