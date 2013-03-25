AVISOPOINTS=test.points 
GCC=gcc-4.7
PLUGINPATH=${PWD}/../AvisoCompiler
RUNTIMEPATH=${PWD}/../Runtime
CFLAGS=-std=c99 -finstrument-functions -fno-omit-frame-pointer -fplugin=${PLUGINPATH}/aviso.so -g
LDFLAGS=-L${RUNTIMEPATH} -lIRPTR

TARGET=./test

all:
	AVISOPOINTS=test.points ${GCC} ./test.c -o ${TARGET} ${CFLAGS} ${LDFLAGS}

run:
	DYLD_LIBRARY_PATH=${RUNTIMEPATH} ${TARGET}

clean:
	-rm -r test.dSYM
	-rm test
