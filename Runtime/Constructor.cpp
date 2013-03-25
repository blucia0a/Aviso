#include <stdlib.h>
#include <stdio.h>
#include "CorrectRunModel.h"
#include "ThreadData.h"
#include "IR_PThreadsRuntime.h"
extern __thread void **btbuff;
extern __thread void **btbuffend;
extern __thread int btbuff_init;

extern __thread pthread_key_t *tlsKey;
extern "C" void IR_Constructor();

void __attribute__ ((constructor)) Aviso_Constructor();
void __attribute__ ((destructor))  Aviso_Destructor();

void Aviso_Constructor(){

  tlsKey = (pthread_key_t *)malloc( sizeof( pthread_key_t ) );

  pthread_key_create( tlsKey, NULL );
  

  ThreadData *t = (ThreadData *)malloc( sizeof(ThreadData) );
  
  
  pthread_setspecific(*tlsKey,(void*)t);
    
  ThreadData *t2 = (ThreadData *)pthread_getspecific(*tlsKey);
  
  
  initializeCorrectRunDump();

  btbuff = (void**)calloc(100,sizeof(void*));
  btbuffend = btbuff;
  btbuff_init = 1;

  IR_Constructor();

}

void Aviso_Destructor(){

  IR_Destructor();

}

