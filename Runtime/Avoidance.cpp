#include "Avoidance.h"
#include "ThreadData.h"

#include <unistd.h>
using std::set;
using std::unordered_set;

pthread_mutex_t MachinesLock;
set<StateMachine *> *Machines;
//__thread set<StateMachineFactory *> *myFactories;
//__thread unordered_set<Backtrace *, BTHash, LooseBTEquals> *involvedBacktraces;

/*Thread local buffer to put the current backtrace*/
/*extern __thread Backtrace *bt;
extern __thread unsigned long numMachineStarts;
extern __thread unsigned long numMachineStartChecks;
extern __thread unsigned long curActiveMachines;
extern __thread unsigned long maxActiveMachines;*/
extern __thread pthread_key_t *tlsKey;
extern unsigned long maxMachinesGlobal; 

bool Avoidance(){

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  int retries = 0;
  
 
retry: /*The label to use to break out of the inner loop when waiting*/

  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
  pthread_mutex_lock(&MachinesLock);

  int i = -1;
  set<StateMachine *>::iterator it, et, wk;
  for(it = Machines->begin(), et = Machines->end(); it != et; it++){
    
    volatile enum SMAction a = (*it)->run( t->bt, (unsigned long)pthread_self() );
    i++;
    switch(a){

      case WAIT:
        if(retries > AVOIDANCE_MAX_RETRIES){ /*Wait 5ms at most??*/
          if(t->curActiveMachines > 0){t->curActiveMachines--;}
          wk = it;
          wk--;
          delete (*it);
          Machines->erase(it);
          it = wk;
            break;
          } 
        /*if(retries == 0){
          fprintf(stderr,"Thread %d hit a wait point for machine %d!\n",(int)pthread_self(),i);
        }*/
        retries++;
        pthread_mutex_unlock(&MachinesLock);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        usleep(AVOIDANCE_WAIT);
        goto retry;


      case TERM:
        //fprintf(stderr,"Thread %d stopped machine %d!\n",(int)pthread_self(),i);
        if(t->curActiveMachines > 0){t->curActiveMachines--;}
        wk = it;
        wk--;
        delete (*it);
        Machines->erase(it);
        it = wk;
        break;

      
      case CONT:
      default:
        break;

    }/*End switch*/
 
  }/*Done with all machines in list.*/
  
  set<StateMachineFactory *>::iterator fit, fet;
  for(fit = t->myFactories->begin(), fet = t->myFactories->end(); fit != fet; fit++){

    t->numMachineStartChecks++;
    if((*fit)->isStartState(t->bt)){
      StateMachine *m = (*fit)->CreateMachine();
      m->run(t->bt, (unsigned long)pthread_self());
      t->numMachineStarts++;
      /*fprintf(stderr,"Thread %d just started a machine %d at %p!\n",(int)pthread_self(),i,bt->bt[0]);*/
      Machines->insert(m);

    }

  }
  if(Machines->size() > maxMachinesGlobal){
    maxMachinesGlobal = Machines->size();
  }

  pthread_mutex_unlock(&MachinesLock);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
  /*End Avoidance Code*/

}

