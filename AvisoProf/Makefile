PLATFORM := $(shell uname)
CXX=g++
CC=gcc
PIN_HOME = $(HOME)/pin

OBJS = ap.o

DBG= -g -DVERBOSE
OPT= -O3

CXXFLAGS= -Wall -Wno-unknown-pragmas -Wno-deprecated $(DBG) $(OPT) -MMD

PIN_KIT=$(PIN_HOME)
KIT=1
include $(PIN_HOME)/source/tools/makefile.gnu.config
TARGET = ap


all: $(TARGET)
%.o : %.cpp
	$(CXX) -c  $(CXXFLAGS) $(PIN_CXXFLAGS) $(OPT) $(DBG) -o $@ $<

ap: $(OBJS)
	${PIN_LD} $(PIN_LDFLAGS) $(LINK_DEBUG) -o $@ $< ${PIN_LPATHS} $(PIN_LIBS) $(EXTRA_LIBS) $(DBG) $(OPT) 

clean:
	-rm -f *.o ap *.out *.tested *.failed *.d

-include *.d
