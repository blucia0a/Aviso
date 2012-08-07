include ../Makefile.global

CC=gcc-4.7
CXX=g++-4.7

OPT = -O3
DBG = -g
CXXFLAGS += $(OPT) $(DBG) $(PROF) -Wno-deprecated -fno-omit-frame-pointer -fpermissive
LDFLAGS += $(OPT) $(DBG) $(PROF) -fno-omit-frame-pointer
SHARED = -shared -fPIC

#Targets
IRLIB=libIRPTR.so

#Source Files
SRCS = \
ClockPortability.cpp \
LogicalTimeKeeper.cpp \
CorrectRunModel.cpp \
Applier.cpp \
PluginProcessing.cpp \
Avoidance.cpp \
STQueue.cpp \
GetBacktrace.cpp \
Constructor.cpp \
IR_PThreadsRuntime.cpp

OBJS = $(SRCS:%.cpp=%.o) EventRoutines.o

all: $(IRLIB)

%.o : %.cpp
	$(CXX) -fPIC -c $(CXXFLAGS) -o $@ $<

EventRoutines.o : EventRoutines.cpp
	$(CXX) -fPIC -finstrument-functions -c $(CXXFLAGS) -o $@ $<

$(IRLIB): $(OBJS) $(TEMPLATES)
	$(CXX) -O3 -fpermissive -lcurl -ldl -shared -fPIC -o $(IRLIB) $(OBJS)

clean:
	-rm *.o $(IRLIB)
