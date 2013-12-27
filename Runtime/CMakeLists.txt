set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")
set(INSTRUMENT_CXX_FLAGS "-fno-omit-frame-pointer -finstrument-functions -pthread -fPIC")
set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -pthread -shared")

include_directories(${CMAKEDEMO_SOURCE_DIR}/Runtime)
link_directories(${CMAKEDEMO_BINARY_DIR}/Runtime)

#these are all compiled the same way
set(IRPTROBJS Applier.cpp Avoidance.cpp ClockPortability.cpp Constructor.cpp CorrectRunModel.cpp EventRoutines.cpp GetBacktrace.cpp IR_PThreadsRuntime.cpp LogicalTimeKeeper.cpp PluginProcessing.cpp STQueue.cpp ConfigurationManager.cpp)

set_source_files_properties(EventRoutines.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${INSTRUMENT_CXX_FLAGS}")

#building just a library. 
add_library(IRPTR SHARED ${IRPTROBJS})

target_link_libraries(IRPTR dl)
target_link_libraries(IRPTR curl)
