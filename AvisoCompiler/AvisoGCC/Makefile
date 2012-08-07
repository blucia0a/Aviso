GXX=g++-4.7
GCC=gcc-4.7
PLUGIN_SOURCE_FILES= aviso.c
PLUGIN_OBJECT_FILES= $(patsubst %.c,%.o,$(PLUGIN_SOURCE_FILES))
GCCPLUGINS_DIR:= $(shell $(GCC) -print-file-name=plugin)
CFLAGS+= -I$(GCCPLUGINS_DIR)/include -fPIC -O2 -g

all: aviso.so

myexpand.o: myexpand.c
	$(GXX) $(CFLAGS) -c $^ -o $@

aviso.o: aviso.c
	$(GCC) $(CFLAGS) -c $^ -o $@

aviso.so: aviso.o myexpand.o
	$(GCC) $(CFLAGS) -shared -undefined dynamic_lookup $^ -o $@

clean:
	-rm aviso.o myexpand.o
