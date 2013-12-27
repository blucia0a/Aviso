CC=gcc-4.7
CFLAGS+=-O2 -g 
LDFLAGS+=-pthread -g 

CFLAGS += -Wno-strict-aliasing -Dassert=IR_Assert -DAVISO_Synthetic_Event=IR_SyntheticEvent -Dpthread_exit=IR_ThreadExit -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -Dpthread_mutex_init=IR_LockInit -g -O3 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer -finstrument-functions
LDFLAGS= -pthread -Dassert=IR_Assert -L${HOME}/cvsandbox/Aviso/Runtime -lIRPTR 
PLAINCFLAGS += -Wno-strict-aliasing -g -O3 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer
PLAINLDFLAGS= -pthread -g 

all:
	AVISOPOINTS=c.list $(CC) $(CFLAGS) -finstrument-functions -fplugin=${HOME}/cvsandbox/Aviso/AvisoCompiler/aviso.so -c $(CFLAGS) -o Crasher.o Crasher.c
	$(CC) ./Crasher.o $(LDFLAGS) -pthread -g -o Crasher 

plain:
	$(CC) $(PLAINCFLAGS) -c -o Crasher.o Crasher.c
	$(CC) ./Crasher.o $(PLAINLDFLAGS) -o Crasher 

ctraps:
	$(CC) -finstrument-functions -fplugin=$(CTRAPS) -c $(CFLAGS) -o Crasher.o Crasher.c
	$(CC) ./Crasher.o $(LDFLAGS) -pthread -ldl -lcurl -g -o Crasher 


clean:
	-rm *.o
	-rm Crasher
