CC=gcc-4.7
CFLAGS+=-O0 -g
LDFLAGS+=-lpthread -g

CFLAGS += -Wno-strict-aliasing -Dassert=IR_Assert -DAVISO_Synthetic_Event=IR_SyntheticEvent -Dpthread_exit=IR_ThreadExit -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -Dpthread_mutex_init=IR_LockInit -g -O3 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer -finstrument-functions
LDFLAGS= -lpthread -Dassert=IR_Assert -L${HOME}/cvsandbox/Aviso/Runtime -lIRPTR 

all:
	$(CC) -finstrument-functions -c $(CFLAGS) -o Crasher.o ./Crasher.c
	$(CC) $(LDFLAGS) -lpthread -g -o Crasher ./Crasher.o

clean:
	-rm *.o
	-rm Crasher
