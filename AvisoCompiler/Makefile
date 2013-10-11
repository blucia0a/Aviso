GXX=g++-4.7
GCC=gcc-4.7
GCCPLUGINS_DIR:= $(shell $(GCC) -print-file-name=plugin)
CFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2 -g

all: aviso.so

myexpand.o: myexpand.cpp
	$(GXX) $(CFLAGS) -c $^ -o $@

aviso.o: aviso.c
	$(GCC) $(CFLAGS) -c $^ -o $@

aviso.so: aviso.o myexpand.o
	#$(GCC) $(CFLAGS) -std=c++11 -lstdc++ -shared -undefined dynamic_lookup $^ -o $@
	$(GXX) $(CFLAGS) -std=c++11 -shared $^ -o $@

clean:
	-rm aviso.o myexpand.o aviso.so
