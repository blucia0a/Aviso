#include <stdio.h>

#include "SequenceMonitor.h"
#include "Misc.h"
#include "SeqHist.h"
#include "Backtrace.h"
#include "fifo/mmu.h"
#include "fifo/queue.h"

#include "Apache1StateMachine.h"

#define QSHR 0
#define QDST 1

struct BTEqualsAllLibrariesAreEqual{

  bool operator()(const Backtrace *v1,const Backtrace *v2) const{
  
    if(  !((unsigned long)v1) ^ !((unsigned long)v2) ){
      /*xor is true if they're not both non-null or both null*/
      return false;
    }
    if( v1 == NULL ){

      /*They are both null*/
      return true;

    }
    
    if( v1->bt[0] != v2->bt[0] ){

      if( (((unsigned long)v1->bt[0]) & 0x8000000000000000) ^
          (((unsigned long)v2->bt[0]) & 0x8000000000000000) ){
        /*if xor of handoff bits is true, they are different, so return false.*/
        //fprintf(stderr,"0 - HANDOFF MISMATCH: %p %p\n",v1->bt[0],v2->bt[0]);

        return false;

      }

      unsigned long t1 = ((unsigned long)v1->bt[0]) & 0x7fffffffffffffff;
      unsigned long t2 = ((unsigned long)v2->bt[0]) & 0x7fffffffffffffff;

      if( t1 != t2 ){
        
        if( !(t1 > 0x700000000000 && t2 > 0x700000000000) ){

          //fprintf(stderr,"0 - LIBRARY MISMATCH: %p %p\n",v1->bt[0],v2->bt[0]);
          /*both were not in shared library range!*/
          return false;

        }

      }

    }

    for(int i = 1; i < BTLEN; i++){
     
      /* 
      TODO: Need a more platform independent way of doing this.
     
      In previously collected backtraces:
      0x8000000000000000 indicates a handoff
      0xffffffffffffffff indicates a library

      In current-run backtraces:
      0x8000000000000000 indicates a handoff
      e.g., 0x7f94eee0e5ec indicates a library

      */
      unsigned long t1 = ((unsigned long)v1->bt[i]);
      unsigned long t2 = ((unsigned long)v2->bt[i]);

      if( t1 != t2 ){
        
        if( (t1 > 0x700000000000 && t2 > 0x700000000000) ){
          /*both were in shared library range!*/
          continue;
        }        

        //fprintf(stderr,"%d - MISMATCH: %p %p\n",i,v1->bt[i],v2->bt[i]);
        return false; 

      }else{
          
        //fprintf(stderr,"%d - MATCH: %p %p\n",i,t1,t2);

      }

    }

    //fprintf(stderr,"all - FULL MATCH:\n");
    //((Backtrace*)v1)->print(stderr);fprintf(stderr,"\n");
    //((Backtrace*)v2)->print(stderr);fprintf(stderr,"\n");
    return true;
  }

};

typedef struct _avoidanceModeRecord{

  bool enabled;
  int currentTid;
  pthread_t currentThread;
  unsigned long long currThdStartTime;
  unsigned long numAvoidanceActions;
  unsigned long numAvoidanceUpdates;
  unsigned long numAvoidanceTimeouts;
  unsigned long numAvoidanceExits;

} AvoidanceModeRecord;

class AvoidanceManager{

private:

  /*Queues to get events from the app*/
  mmu_t *app_in_mmus[MAX_NUM_THDS];
  queue_t *app_in_queues[MAX_NUM_THDS];

  TraceEvent *app_in_buffer;

  /*Queues to send events to the app*/
  mmu_t *app_out_mmus[MAX_NUM_THDS];
  queue_t *app_out_queues[MAX_NUM_THDS];
  
  /*Queues to send events to the sequence collector*/
  mmu_t *collector_out_mmu;
  queue_t *collector_out_queue;

  int tid;
  int queueMode;
  int seqLen;
  pthread_t lastHeld;

  unsigned int *seqTids;
  unsigned long maskBits;
  
  Backtrace **avoidSets[MAX_NUM_THDS];
  pthread_t threads[MAX_NUM_THDS];

  Apache1StateMachine *a1sm;

  FILE *badSequenceFile;
  Backtrace **currentSequence;
  SeqHist<Backtrace,BTEqualsAllLibrariesAreEqual> *badDB;

  AvoidanceModeRecord avMode;

public:

  bool done;
  pthread_barrier_t *MakeSeqMonWait;
  AvoidanceManager(int t, int seqLen, FILE *badSeqFile, SequenceMonitor *seqMon);
  queue_t *getIncomingQueue(int tid);
  TraceEvent **getIncomingEvt();
  void setDone();
  void Shutdown();
  void initializeDatabase(FILE *bad);
  void setAvoidSet(int appTid, Backtrace **aset);
  void mapThreadToTid(int tid, pthread_t thd);

  void avModeEnter();
  void avModeUpdate();
  bool avModeEnabled(){ return avMode.enabled; }
  void avModeExit();

  int getTid(){ return this->tid; } 
  queue_t *getCollectorOutQueue(){ return this->collector_out_queue; } 

  static void *Run(void *arg);


};
