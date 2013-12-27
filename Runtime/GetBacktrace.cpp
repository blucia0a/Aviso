#include <stdio.h>
#include <stdlib.h>
#include "STQueue.h"
#include "Backtrace.h"

#define MAX_STACK_DEPTH 100
void _get_backtrace(void **baktrace,int addrs);
__thread void **btbuff = NULL;
__thread void **btbuffend = NULL;
__thread int btbuff_init = 0;

extern __thread pthread_key_t *tlsKey;
void GetThreadData();


extern "C" void IR_SyntheticEvent();

extern "C"{
void __cyg_profile_func_enter(void *this_fn, void *call_site){

  if( btbuff == NULL ){  

    btbuff = (void**)calloc(MAX_STACK_DEPTH,sizeof(void*));
    btbuffend = btbuff;
    btbuff_init = 1;

  }

  *btbuffend = __builtin_return_address(1);

  btbuffend++;
   
}
}

extern "C"{
void __cyg_profile_func_exit  (void *this_fn, void *call_site){

  *btbuffend = NULL;
  btbuffend--;

}
}

void _get_backtrace(void **baktrace,int addrs){

  int a = 0;
  void **biter = btbuffend;
  biter--;
  while(a < addrs && biter != btbuff){
    baktrace[a++] = *biter;
    biter--;
  } 

}
