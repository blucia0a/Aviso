#Options
#CXX=${HOME}/cvsandbox/Apps/STLFilt/gfilt
CC=gcc-4.7
CXX=g++-4.7
#AR = gcc-4.7-ar
OPT = -O3
DBG = -g
PROF = 
CXXFLAGS += $(OPT) $(DBG) $(PROF) -Wno-deprecated -fno-omit-frame-pointer -fpermissive
LDFLAGS += $(OPT) $(DBG) $(PROF) -fno-omit-frame-pointer
SHARED = -shared -fPIC


#Targets
IRLIB=libIRPTR.so
STATIC_IRLIB=libIRPTR.a
STATICQUEUE=

#Source Files
SRCS = ClockPortability.cpp LogicalTimeKeeper.cpp CorrectRunModel.cpp Applier.cpp PluginProcessing.cpp Avoidance.cpp STQueue.cpp  GetBacktrace.cpp Constructor.cpp IR_PThreadsRuntime.cpp

OBJS = $(SRCS:%.cpp=%.o) EventRoutines.o

all: $(IRLIB)

## build rules

%.o : %.cpp
	$(CXX) -fPIC -c $(CXXFLAGS) -o $@ $<

EventRoutines.o : EventRoutines.cpp
	$(CXX) -fPIC -finstrument-functions -c $(CXXFLAGS) -o $@ $<

$(IRLIB): $(OBJS) $(TEMPLATES)
	$(CXX) -O3 -fpermissive -lcurl -ldl -shared -fPIC -o $(IRLIB) $(OBJS)

$(STATIC_IRLIB): $(OBJS) $(TEMPLATES)
	$(AR) rcs $(STATIC_IRLIB) $(OBJS)

test:
	$(CXX) -c $(CXXFLAGS) -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -o tester2.o tester2.cpp
	$(CXX) $(LDFLAGS) -lpthread -Lfifo -llf -L. -lIRPTR tester2.o -o tester2

clean:
	-rm *.o $(IRLIB)
