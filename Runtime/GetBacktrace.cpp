#include <stdio.h>
#include <stdlib.h>
#include "STQueue.h"
#include "Backtrace.h"

#define MAX_STACK_DEPTH 100
void _get_backtrace(void **baktrace,int addrs);
__thread void **btbuff = NULL;
__thread void **btbuffend = NULL;
__thread int btbuff_init = 0;

/*extern __thread unsigned long synthEvTime;
extern __thread STQueue *myRPB;
extern __thread Backtrace *bt;
extern __thread unsigned long mytid;*/
extern __thread pthread_key_t *tlsKey;
void GetThreadData();


extern "C" void IR_SyntheticEvent();
//__thread unsigned int rseed;

extern "C"{
//void profile_func_enter(void *this_fn, void *call_site){
void __cyg_profile_func_enter (void *this_fn, void *call_site){

  //fprintf(stderr,"initializing in func_enter %p %p\n",this_fn,call_site);
  if( btbuff == NULL ){  

    btbuff = (void**)calloc(MAX_STACK_DEPTH,sizeof(void*));
    btbuffend = btbuff;
    btbuff_init = 1;

  }

  *btbuffend = __builtin_return_address(1);
  btbuffend++;

/*
  void **biter = btbuffend;
  fprintf(stderr,"Enter Backtrace (");
  do{
    fprintf(stderr,"%p ",*biter);
  }while(biter-- != btbuff && *biter != NULL);
  fprintf(stderr,")\n");
*/
   
}
}

extern "C"{
void __cyg_profile_func_exit  (void *this_fn, void *call_site){
//void profile_func_exit  (void *this_fn, void *call_site){
  
/*  if( btbuff_init == 0){
    fprintf(stderr,"initializing in func_exit\n");
    btbuff = (void**)malloc(100*sizeof(void*));
    btbuffend = btbuff;
    btbuff_init = 1;
  }*/
  *btbuffend = NULL;
  btbuffend--;

/*
  void **biter =  btbuffend;
  fprintf(stderr,"Exit Backtrace (");
  do{ 
    fprintf(stderr,"%p ",*biter);
  }while(biter-- != btbuff && *biter != NULL);
  fprintf(stderr,")\n");
*/ 

}
}

void _get_backtrace(void **baktrace,int addrs){

/*  if( btbuff_init == 0 ){
    fprintf(stderr,"initializing in get_backtrace\n");
    btbuff = (void**)malloc(100*sizeof(void*));
    btbuffend = btbuff;
    btbuff_init = 1;
  }*/


  int a = 0;
  void **biter = btbuffend;
  biter--;
  //fprintf(stderr,"BT: ");
  while(a < addrs && biter != btbuff){
    //fprintf(stderr,"%p ",*biter);
    baktrace[a++] = *biter;
    biter--;
  } 
  //fprintf(stderr,"\n");

}
