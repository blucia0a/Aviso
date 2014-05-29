CLANGDIR=/Users/blucia/cvsandbox/clang/inst/bin
CC=$(CLANGDIR)/clang

RUNTIMEPATH=/Users/blucia/cvsandbox/symbiosis/Aviso/Runtime
LDFLAGS=-L${RUNTIMEPATH} -lIRPTR

CFLAGS=-std=c99 -finstrument-functions -fno-omit-frame-pointer -g

LOADPASS= -Xclang -load -Xclang ../Aviso/Aviso.dylib

TARGET=./test

all:
	$(CC) ./test.c -o ${TARGET} $(LOADPASS) ${CFLAGS} ${LDFLAGS}

run:
	DYLD_LIBRARY_PATH=${RUNTIMEPATH} ${TARGET}

clean:
	-rm -r test.dSYM
	-rm test
