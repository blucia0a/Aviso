CC=gcc-4.7
CFLAGS+=-O3 -g -march=native
LDFLAGS+=-pthread -g -march=native

CFLAGS += -Wno-strict-aliasing -Dassert=IR_Assert -DAVISO_Synthetic_Event=IR_SyntheticEvent -Dpthread_exit=IR_ThreadExit -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -Dpthread_mutex_init=IR_LockInit -g -O3 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer -finstrument-functions
LDFLAGS= -ldl -lcurl -pthread -Dassert=IR_Assert -L${HOME}/cvsandbox/Aviso/Runtime -lIRPTR 
CTRAPS=${HOME}/cvsandbox/CTraps-gcc/Compiler/RWInst.so


all:
	AVISOPOINTS=c.list $(CC) -finstrument-functions -fplugin=${HOME}/cvsandbox/Aviso/AvisoCompiler/aviso.so -c $(CFLAGS) -o Crasher.o Crasher.c
	$(CC) ./Crasher.o $(LDFLAGS) -pthread -ldl -lcurl -g -o Crasher 


ctraps:
	$(CC) -finstrument-functions -fplugin=$(CTRAPS) -c $(CFLAGS) -o Crasher.o Crasher.c
	$(CC) ./Crasher.o $(LDFLAGS) -pthread -ldl -lcurl -g -o Crasher 


clean:
	-rm *.o
	-rm Crasher
