#define _USE_GNU
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <signal.h>
#include <string.h>
#include <tr1/unordered_set>
#include <set>
#include <errno.h>
#include <assert.h>

/*Stupid little list application function*/
#include "Applier.h"
#include "STQueue.h"

/*Header for the plugin processing stuff*/
#include "PluginProcessing.h"

/*Headers declaring the StateMachine stuff*/
#include "StateMachines/StateMachine.h"
#include "StateMachines/StateMachineFactory.h"
#include "StateMachines/SMAction.h"

/*Header declaring the interface of this module*/
#include "IR_PThreadsRuntime.h"

/*Header declaring functions private to this module*/
#include "IR_PThreadsRuntime_private.h"

/*Header declaring struct TraceEvent*/
#include "TraceEvent.h"

/*Header declaring and defining the Backtrace class and BTLEN*/
#include "Backtrace.h"

/*Header declaring avoidance routines that call into FSMS*/
#include "Avoidance.h"

#include "ThreadData.h"

/*Implementations*/
int GetTid(){

  for(int i = 0; i < MAX_NUM_THDS; i++){
    if(tids[i] != 0){//found a valid id!
      if(CAS(&(tids[i]),tids[i],0)){
        return i;
      }//else we failed the CAS -- someone took our tid!  Continue...
    }
  }
 
  fprintf(stderr,"[AVISO] Ran out of thread IDs!\n");
  exit(1);

}

FILE *getRPBFile(){

  return RPBFile;    

}

/************************************************************************
*Signal Handler
*    This handler asks the monitor thread to dump the event history, 
*    and waits for it to join, signifying that it has done so.  Then,
*    this signal handler calls the next signal handler that the program
*    had registered, to maintain correct program termination behavior (
*    for example, mySQL does some signal time shutdown stuff).
************************************************************************/
void thdSigEnd(int signum){

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);
  if( signum == SIGUSR1 ){

    if( t->alreadyDumped ){ return; }

    //fprintf(stderr,"[AVISO] Thread %lu Received SIGUSR1 -- Dumping RPB\n",(unsigned long)pthread_self());

    t->alreadyDumped = true;
    if (pthread_mutex_trylock(&outputLock) != 0){
      /*Note: If the lock acquire can't proceed, it must be because this thread alread holds the lock*/
      //fprintf(stderr,"[AVISO] Thread %lu tried acquiring its lock again!\n",(unsigned long)pthread_self());
    }

    FILE *f = getRPBFile();
    t->myRPB->Dump( f );
    int ret = fflush( f );
    if( ret != 0 ){
      fprintf(stderr,"[AVISO] There was an error flushing the RPB\n");
    } 
    //fprintf(stderr,"Thread %lu Done Dumping RPB\n",(unsigned long)pthread_self());

    numDumped++;
    pthread_mutex_unlock(&outputLock);

  }

  return;

}


/*This is fucking hairy -- don't change it!*/
void terminationHandler(int signum){

  fprintf(stderr,"[AVISO] Process %d died with signal %d (Handler Thread: %lu)\n",getpid(),signum,(unsigned long)pthread_self());

  if( tlsKey == NULL ){
    GetThreadData();
  }

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  volatile int totalThreads = 0;

  if( signum == SIGABRT || signum == SIGSEGV ){

    if( handlingFatalSignal ){

      /*I am not the first signal handler.  I should hang out and let the other signal handling guy deal with this*/

      /*First, I need to make sure I'm not prodded by the first signal handler.  I'll dump my RPB here anyway*/
      sigset_t new_set;
      pthread_sigmask(0, NULL, &new_set );
      sigemptyset( &new_set );
      sigaddset( &new_set, SIGUSR1 );
      pthread_sigmask(SIG_BLOCK, &new_set, NULL );

      /*If SIGUSR1 handling hasn't dumped my RPB already, I will dump it here*/
      if( !t->alreadyDumped ){

        //fprintf(stderr,"Thread %lu Was Handling Second Signal %d -- Dumping RPB\n",(unsigned long)pthread_self(),signum);
        /*Concurrency with self: can't be holding lock because alreadyDumped would be true*/
        t->alreadyDumped = true;
        pthread_mutex_lock(&outputLock);
        FILE *f = getRPBFile();
        t->myRPB->Dump( f );
        int ret = fflush( f );
        if( ret != 0 ){
          fprintf(stderr,"[AVISO] There was an error flushing the RPB\n");
        } 
        //fprintf(stderr,"Thread %lu Done Dumping RPB\n",(unsigned long)pthread_self());

        pthread_mutex_unlock(&outputLock);
       
        
      }

      sigset_t empty;
      sigset_t pending;
      sigemptyset( &empty );
      //fprintf(stderr,"Thread %lu is Waiting\n",(unsigned long)pthread_self());
      while( !finishedFatalSignal ){ 

        /*Wait for the other guy handling a fatal signal to exit... (could it be me?)*/ 
        sigpending(&pending);
        if( sigismember(&pending, SIGUSR1) ){
          //fprintf(stderr,"Thread %lu was waiting and received SIGUSR1\n");
          pthread_mutex_lock(&outputLock);
          numDumped++;
          pthread_mutex_unlock(&outputLock);
        }

      }

      //fprintf(stderr,"Thread %lu is Done Waiting\n",(unsigned long)pthread_self());
      return;

    }else{

      fprintf(stderr,"[AVISO] Fatal Signal Received\n");
      handlingFatalSignal = true;

    }

  }

  for(int i = 0; i < MAX_NUM_THDS; i++ ){

    pthread_mutex_lock(&allThreadsLock);
    if( allThreads[i] != 0 ){

      if( allThreads[i] == pthread_self() ){

        //fprintf(stderr,"Thread %lu Was Handling First Signal %d -- Dumping RPB\n",(unsigned long)pthread_self(),signum);
        FILE *f = getRPBFile();
        pthread_mutex_lock(&outputLock);
        t->myRPB->Dump( f );
        int ret = fflush( f );
        if( ret != 0 ){
          fprintf(stderr,"[AVISO] There was an error flushing the RPB\n");
        } 
        //fprintf(stderr,"Thread %lu Done Dumping RPB\n",(unsigned long)pthread_self());

        numDumped++;
        pthread_mutex_unlock(&outputLock);
        
      }else{

        int curNum = numDumped;
        int ret = pthread_kill(allThreads[i],SIGUSR1);
        //fprintf(stderr,"Thread %lu Sent Dump Signal to %lu\n",(unsigned long)pthread_self(),(unsigned long)allThreads[i]);
        totalThreads++;
        if( ret != 0 ){

          switch(ret){
            case ESRCH:
              fprintf(stderr,"Thread not found!\n");
              break;
            case EINVAL:
              fprintf(stderr,"No Such Signal!\n");
              break;
            default:
              fprintf(stderr,"Unknown Signaling Error\n");
              break;
          }
          
        }else{

          //fprintf(stderr,"Thread %lu waiting for %lu\n",(unsigned long)pthread_self(),(unsigned long)allThreads[i]);
          while(1){
        
            pthread_mutex_lock(&outputLock); 
            if( numDumped > curNum ){
              pthread_mutex_unlock(&outputLock); 
              //fprintf(stderr,"Thread %lu came back from dumping!  It is done now!\n",(unsigned long)allThreads[i]);
              break;
            }
            pthread_mutex_unlock(&outputLock); 
            usleep(1000);

          }
          //fprintf(stderr,"Thread %lu done waiting for %lu\n",(unsigned long)pthread_self(),(unsigned long)allThreads[i]);

        }
 
      }

    }

    pthread_mutex_unlock(&allThreadsLock);

  }

  finishedFatalSignal = true;

  if( signum == SIGSEGV){

    if( sigSEGVSaver.sa_handler != SIG_DFL &&
        sigSEGVSaver.sa_handler != SIG_IGN){
      fprintf(stderr,"Thread %lu Calling Program Segv Handler\n",(unsigned long)pthread_self());
      (*sigSEGVSaver.sa_handler)(signum);  
    }
        

  }else if( signum == SIGTERM){
    
    if( sigTERMSaver.sa_handler != SIG_DFL &&
        sigTERMSaver.sa_handler != SIG_IGN){
      fprintf(stderr,"Thread %lu Calling Program Term Handler\n",(unsigned long)pthread_self());
      (*sigTERMSaver.sa_handler)(signum);  
    }

  }else if( signum == SIGABRT){
    if( sigABRTSaver.sa_handler != SIG_DFL &&
        sigABRTSaver.sa_handler != SIG_IGN){
      fprintf(stderr,"Thread %lu Calling Program Abort Handler\n",(unsigned long)pthread_self());
      (*sigABRTSaver.sa_handler)(signum);  
    }
  }

  //fprintf(stderr,"Thread %lu Calling Default handler for signal %d\n",(unsigned long)pthread_self(),signum);

  signal(signum, SIG_DFL);
  if( signum == SIGABRT ){
    assert(false);
  }
  kill(getpid(), signum);

}


extern "C" void IR_Constructor(){

  if( !initialized ){ initialized = true; }else{ return; }

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  maxMachinesGlobal = 0;

  t->numSynthEvents =
  t->numSynthBail =
  t->numSynthPartial =
  t->numSyncEvents =
  t->numSyncPartial =
  t->numMachineStartChecks =
  t->numMachineStarts =
  t->curActiveMachines =
  t->maxActiveMachines = 0;

  /*Register Signal handler for SEGV and TERM*/
  memset(&sigABRTSaver, 0, sizeof(struct sigaction));
  memset(&sigSEGVSaver, 0, sizeof(struct sigaction));
  memset(&sigTERMSaver, 0, sizeof(struct sigaction));

  struct sigaction segv_sa;
  segv_sa.sa_handler = terminationHandler;
  sigemptyset(&segv_sa.sa_mask);
  segv_sa.sa_flags = SA_RESTART;
  sigaction(SIGSEGV,&segv_sa,&sigSEGVSaver);
  sigaction(SIGTERM,&segv_sa,&sigTERMSaver);
  sigaction(SIGABRT,&segv_sa,&sigABRTSaver);
  
  signal(SIGUSR1,thdSigEnd); 

  /*Initialize the array of sequentialized thread IDs*/
  pthread_mutex_init(&allThreadsLock,NULL);
  for(int i = 0; i < MAX_NUM_THDS; i++){
    tids[i] = i;
    allThreads[i] = 0;
  } 

  /*Get the environment specified sequence length*/
  char *p = getenv("IR_SeqLen");
  if(p != NULL){
    sequenceLength = atoi(p);
  }

  /*Set up the main thread's thread meta-data and start the sequence monitor*/
  t->mytid = GetTid();
  t->myRPB = new STQueue();

  /*Initialize the avoidance data structures*/
  InitMachines();
  Factories = new std::set<StateMachineFactory *>(); 

  t->bt = new Backtrace();
  
  pthread_mutex_init(&outputLock,NULL);
  char outFileName[512];
  memset(outFileName,0,512);
  srand(time(NULL));
  p = NULL;
  if((p = getenv("IR_TraceDir")) != NULL){

    char buf2[1024];
    memset(buf2,0,1024);
    sprintf(outFileName,"%s%s",p,"/RPB");
    sprintf(buf2,"%s%lu",outFileName,(unsigned long)getpid());
    RPBFile = fopen(buf2,"w");
    fprintf(stderr,"[Aviso] RPB: %s\n",buf2);

  }else{

    RPBFile = NULL;
    fprintf(stderr,"[Aviso] RPB: <stderr>\n");

  }

  if( RPBFile == NULL ){
    RPBFile = stderr;
  }

  loadPlugins(Factories,"IR_Plugins");

  t->myFactories = new std::set<StateMachineFactory *>();
  std::set<StateMachineFactory *>::iterator it, et;
  for(it = Factories->begin(), et = Factories->end(); it != et; it++){

    t->myFactories->insert((*it));

  }

  t->involvedBacktraces = 
    new std::tr1::unordered_set<Backtrace *, BTHash, LooseBTEquals>();
  loadPluginConfigs(t->involvedBacktraces,"IR_PluginConfs");

}

extern "C" void IR_Destructor(){
  
  fprintf(stderr,"[AVISO] Process %d ended.\n",getpid());
  
  #if defined(PRINTSTATS)
  fprintf(stderr,"FORMATthreadid numSynthEvents, numSynthBail,numSynthPartial,numSyncEvents,numSyncPartial, numMachineStartChecks, numMachineStarts, curActiveMachines, maxActiveMachines\n");
  fprintf(stderr,"Max Machines Global = %lu\n",maxMachinesGlobal);
  #endif

  while(handlingFatalSignal && !finishedFatalSignal){ }
}

void thdDestructor(void *vt){

  ThreadData *t = (ThreadData *)vt;
  #if defined(PRINTSTATS)
  fprintf(stderr,"STATS%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", 
                 pthread_self(),
                 numSynthEvents, 
                 numSynthBail,
                 numSynthPartial,
                 numSyncEvents,
                 numSyncPartial, 
                 numMachineStartChecks, 
                 numMachineStarts, 
                 curActiveMachines, 
                 maxActiveMachines);
  #endif 

  /*Release tids[] entry and allThreads[] entry
   *so they can be reused by other threads
  */
  pthread_mutex_lock(&allThreadsLock);
  allThreads[t->mytid] = NULL;
  pthread_mutex_unlock(&allThreadsLock);

  CAS(&(tids[t->mytid]),0,tids[t->mytid]);


  FILE *f = getRPBFile();
  if( ! t->alreadyDumped ){

    //fprintf(stderr,"Thread %lu Dumping RPB\n",(unsigned long)pthread_self());
    t->alreadyDumped = true;
    pthread_mutex_lock(&outputLock);
    //fprintf(stderr,"Thread %lu Dumping RPB %p\n",(unsigned long)pthread_self(), t->myRPB);
    t->myRPB->Dump( f );
    int ret = fflush( f );
    if( ret != 0 ){
      //fprintf(stderr,"[AVISO] There was an error flushing the RPB\n");
    } 
    //fprintf(stderr,"Thread %lu Done Dumping RPB\n",(unsigned long)pthread_self());
    pthread_mutex_unlock(&outputLock);

  }

}

void GetThreadData(){

  tlsKey = (pthread_key_t *)malloc( sizeof( pthread_key_t ) );

  pthread_key_create( tlsKey, thdDestructor );

  ThreadData *t = (ThreadData *)malloc( sizeof(ThreadData) );

  pthread_setspecific(*tlsKey,(void*)t);

  t->bt = new Backtrace();

  t->mytid = GetTid(); 
  
  pthread_mutex_lock(&allThreadsLock);
  allThreads[t->mytid] = pthread_self();
  pthread_mutex_unlock(&allThreadsLock);
  
  t->myRPB = new STQueue();

  t->myFactories = new std::set<StateMachineFactory *>();
  std::set<StateMachineFactory *>::iterator it, et;
  for(it = Factories->begin(), et = Factories->end(); it != et; it++){

    t->myFactories->insert((*it));

  }

  t->involvedBacktraces = 
    new std::tr1::unordered_set<Backtrace *, BTHash, LooseBTEquals>();
  loadPluginConfigs(t->involvedBacktraces,"IR_PluginConfs");

}
