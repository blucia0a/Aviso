#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "STQueue.h"
#include "TraceEvent.h"
#include "Avoidance.h"
#include "Misc.h"
#include "Backtrace.h"
#include "ClockPortability.h"
#include "ThreadData.h"

extern "C" void dumpIfRequired( unsigned long now );

#define BIL 1000000000
//extern __thread std::tr1::unordered_set<Backtrace *, BTHash, LooseBTEquals> *involvedBacktraces;

/*__thread Backtrace *bt;
__thread unsigned t->synthEvTime;
__thread unsigned t->syncEvTime;
__thread STQueue *myRPB = NULL;
__thread unsigned long mytid = 0;*/

extern __thread void **btbuffend;
extern __thread void **btbuff;

extern __thread pthread_key_t *tlsKey;

void GetThreadData();
extern void _get_backtrace(void **baktrace,int addrs);

extern "C"{
void IR_SyntheticEvent(){

  if(tlsKey == NULL){
    GetThreadData();
  }

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  struct timespec ts;
  clocktime(&ts);

  unsigned dif = ts.tv_nsec - t->synthEvTime;
  t->synthEvTime = ts.tv_nsec;
  
  dumpIfRequired( ts.tv_sec * BIL + ts.tv_nsec );
  
  TraceEvent * e;
 
  if(dif < SE_INT_MIN && dif > 0){

    return;

  }else{

    e = t->myRPB->GetNextEvent();
    int a = 0;
    void **biter = btbuffend;
    biter--;
    while(a < BTLEN && biter != btbuff){
      e->bt[a++] = *biter;
      biter--;
    } 

  }

  e->actor = t->mytid;
  e->time_ns = ts.tv_nsec + BIL*ts.tv_sec;

  t->bt->bt = e->bt;

  if((t->involvedBacktraces->find(t->bt) != t->involvedBacktraces->end())){
    Avoidance();
  }
  
  t->myRPB->Add(e);

  return;

}
}

extern "C"{
int IR_Lock(pthread_mutex_t *lock){

  if(tlsKey == NULL){
    GetThreadData();
  }
  
  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  struct timespec ts;
  clocktime(&ts);

  unsigned dif = ts.tv_nsec - t->syncEvTime;
  t->syncEvTime = ts.tv_nsec;
  dumpIfRequired( ts.tv_sec * BIL + ts.tv_nsec );
  
  TraceEvent * e = t->myRPB->GetNextEvent();

  if(dif < SE_INT_MIN && dif > 0){

    return pthread_mutex_lock(lock);

  }else {

    int a = 0;
    void **biter = btbuffend;
    biter--;
    while(a < BTLEN && biter != btbuff){
      e->bt[a++] = *biter;
      biter--;
    } 

  }

  e->actor = t->mytid;
  e->time_ns = ts.tv_nsec + BIL*ts.tv_sec;

  t->bt->bt = e->bt; 

  if( (t->involvedBacktraces->find(t->bt) != t->involvedBacktraces->end()) ){
    Avoidance();
  }
  
  t->myRPB->Add(e);

  return pthread_mutex_lock(lock);

}
}

extern "C"{
int IR_Unlock(pthread_mutex_t *lock){


  if(tlsKey == NULL){
    GetThreadData();
  }

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  struct timespec ts;
  clocktime(&ts);

  unsigned dif = ts.tv_nsec - t->syncEvTime;
  t->syncEvTime = ts.tv_nsec;
  
  dumpIfRequired( ts.tv_sec * BIL + ts.tv_nsec );
  
  TraceEvent * e; 

  if(dif < SE_INT_MIN && dif > 0 ){

    return pthread_mutex_unlock(lock);

  }else{

    e = t->myRPB->GetNextEvent();
    int a = 0;
    void **biter = btbuffend;
    biter--;
    //fprintf(stderr,"BT: ");
    while(a < BTLEN && biter != btbuff){
      //fprintf(stderr,"%p ",*biter);
      e->bt[a++] = *biter;
      biter--;
    } 

  }

  e->actor = t->mytid;
  e->time_ns = ts.tv_nsec + BIL*ts.tv_sec;

  t->bt->bt = e->bt; 
  
  if((t->involvedBacktraces->find(t->bt) != t->involvedBacktraces->end())){
    Avoidance();
  }

  t->myRPB->Add(e);

  return pthread_mutex_unlock(lock);

}
}

extern "C"{
int IR_Join(pthread_t thd,void **value){


  if(tlsKey == NULL){
    GetThreadData();
  }

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  struct timespec ts;
  clocktime(&ts);

  unsigned dif = ts.tv_nsec - t->syncEvTime;
  t->syncEvTime = ts.tv_nsec;
  
  dumpIfRequired( ts.tv_sec * BIL + ts.tv_nsec );

  TraceEvent * e; 

  if(dif < SE_INT_MIN && dif > 0 ){

    return pthread_join(thd,value);

  }else{

    e = t->myRPB->GetNextEvent();
    int a = 0;
    void **biter = btbuffend;
    biter--;
    //fprintf(stderr,"BT: ");
    while(a < BTLEN && biter != btbuff){
      //fprintf(stderr,"%p ",*biter);
      e->bt[a++] = *biter;
      biter--;
    } 

  }

  e->actor = t->mytid;
  e->time_ns = ts.tv_nsec + BIL*ts.tv_sec;

  t->bt->bt = e->bt; 
  
  if((t->involvedBacktraces->find(t->bt) != t->involvedBacktraces->end())){
    Avoidance();
  }

  t->myRPB->Add(e);

  return pthread_join(thd,value);

}
}


extern "C"{
int IR_Mutex_Destroy(pthread_mutex_t *l){

  if(tlsKey == NULL){
    GetThreadData();
  }
  
  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  struct timespec ts;
  clocktime(&ts);

  unsigned dif = ts.tv_nsec - t->syncEvTime;
  t->syncEvTime = ts.tv_nsec;
  
  dumpIfRequired( ts.tv_sec * BIL + ts.tv_nsec );
  
  TraceEvent * e; 

  e = t->myRPB->GetNextEvent();
  int a = 0;
  void **biter = btbuffend;
  biter--;
  while(a < BTLEN && biter != btbuff){
    e->bt[a++] = *biter;
    biter--;
  } 


  e->actor = t->mytid;
  e->time_ns = ts.tv_nsec + BIL*ts.tv_sec;

  t->bt->bt = e->bt; 
  
  if((t->involvedBacktraces->find(t->bt) != t->involvedBacktraces->end())){
    Avoidance();
  }

  t->myRPB->Add(e);

  return pthread_mutex_destroy(l);

}
}



extern "C"{
int IR_ThreadExit(void *value_ptr){
  
  pthread_exit(value_ptr);

}
}
