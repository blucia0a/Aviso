#include "pin.H"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <list>
#include <tr1/unordered_set>
#include <set>
#include <map>
#include <sys/time.h>

using std::tr1::unordered_set;

PIN_LOCK globalLock;

class CodePointInfo{
/*Going to track features of each code point:*/

  unsigned long frequency;
/* 
 * The frequency is the number of times 
 * the code point is executed during the
 * execution.
 */

  unsigned long cofreq;
/* The co-occurrence frequency is 
 * the frequency with which a code 
 * point occurs within the threshold
 * number of instructions of another
 * sharing instruction in the same thread.
 */

  unsigned long avgtts;
  unsigned long numtts;
/*
 * The average time-to-self for a code
 * point is the rolling average of the 
 * amount of time between consecutive
 * instances of a code point.
 * numtts is the number of instances
 * of the code point up to this point
 * in the execution.
 *
 * We may also be interested in these
 * statistics.
 *   a)Min
 *   b)Max
 *   c)Var
 *
 */
  unsigned long avgtto;
  unsigned long numtto;
/*
 * The average time-to-other for a code
 * point is the rolling average of the
 * amount of time between a code point's
 * execution by a thread, and the same
 * thread's execution of another code
 * point.
 *
 * We may also want to compute these
 * statistics for the tto like the tts.
 *   a)Min
 *   b)Max
 *   c)Var
 */

};

#define THRESH 100 /*100 microsecond window?*/

map< ADDRINT, unsigned long > sharingPCs;
map< ADDRINT, unsigned long > PCFreq;

map< ADDRINT, map<ADDRINT, unsigned long> *> globalFollowGraph;

class infoPair{
public:
  ADDRINT a;
  unsigned long time;
  infoPair(){ }

};

class History{

  /*Contains the history*/
  list<infoPair *> history;

  /*maps the pairs of address that followed, and their following distance*/

  unsigned long oldest;
  unsigned long newest;

public:
  map< ADDRINT, map<ADDRINT, unsigned long> *> followGraph;
  /*This should do several things:
    1)Add the thing to the history
    2)check the extents of the history to ensure the timespan of the history is less than THRESH; if it is, pop elements from the history until that condition is false
    3)iterate over the history and add/update the entry in followGraph as necessary*/
  History();
  void add(ADDRINT a);

};

History::History(){

  this->history = list<infoPair *>();  
  this->followGraph = map<ADDRINT, map<ADDRINT, unsigned long> *>();
  struct timeval t;
  gettimeofday(&t,NULL);
  oldest = t.tv_usec;
  newest = t.tv_usec;

}

void History::add(ADDRINT a){




  /*Populate this new infoPair with information about this access*/
  struct timeval t;
  gettimeofday(&t,NULL);

  infoPair *ip = new infoPair();
  ip->a = a;
  ip->time = t.tv_usec;

  /*Remove stuff that is outside the 1 us window from the history*/
  while( !this->history.empty() && 
         ip->time - this->history.front()->time > THRESH ){

    this->history.pop_front();

  }
 
  /*Push this new infoPair into the history -- the window changes size here*/ 
  this->history.push_back(ip);


  /*
   *Next, we add an edge to the follower graph for everything still in the 
   *history window
  */

  /*If a hasn't been accessed yet*/
  if( this->followGraph.find(a) == this->followGraph.end() ){

    this->followGraph[a] = new map<ADDRINT, unsigned long>();  

  }


  /*This loop iterates over the history and for anything that recently*/
  std::set<unsigned long> predSet = std::set<unsigned long>();
  list<infoPair *>::iterator i,e;
  for( i = this->history.begin(), e = this->history.end(); i != e; i++){

    if(predSet.find((*i)->a) == predSet.end()){ 
      (*this->followGraph[a])[(*i)->a]++;
      predSet.insert((*i)->a);
    }

  }

}



unordered_set< ADDRINT >::iterator sharingPCs_it;

map<unsigned int, unordered_set< THREADID > > memAddrThreadSet;
map<unsigned int, unordered_set< THREADID > >::iterator memAddrThreadSet_it;

PIN_LOCK FiniLock;

FILE* output;

static TLS_KEY tls_key;

enum MemOpType { MemRead = 0, MemWrite = 1 };

class thread_t
{

public:
  map<unsigned long, unordered_set< ADDRINT > > memAddrPCSet;
  unsigned long lastICount;
  History *the_histo;
  map< ADDRINT, unsigned long > *PCFreq;
  thread_t(){
    the_histo = new History();
    this->PCFreq = new map<ADDRINT, unsigned long>();
  }

};

thread_t* get_tls(THREADID thread_id)
{
    thread_t *tdata =
	  static_cast<thread_t*>(PIN_GetThreadData(tls_key, thread_id));

    return tdata;
}

void insertHash(THREADID thread_id, ADDRINT pc)
{
  thread_t *cur = get_tls(thread_id);

  PIN_GetLock(&globalLock, 1);
  PIN_LockClient();

  /*Look through my thread local addresses*/
  map<unsigned long, unordered_set< ADDRINT > >::iterator b,e;
  for(b = cur->memAddrPCSet.begin(), e = cur->memAddrPCSet.end(); b!=e; b++){
  
    /*If I find that address in the global thread set...*/
    ADDRINT ad = b->first;
    memAddrThreadSet_it = memAddrThreadSet.find(ad);
    if(memAddrThreadSet_it != memAddrThreadSet.end()){

      /*If it was found and a different thread put it in there...*/
      bool found = false;
      unordered_set<THREADID>::iterator thit, thet;
      for(thit = memAddrThreadSet_it->second.begin(), 
          thet = memAddrThreadSet_it->second.end(); 
          thit!=thet; 
          thit++){

        if( *thit != thread_id){
          found = true;
        } 

      }

      /*Put my all of my PCs that have accessed that addr into the sharing PCs set*/
      if(found){
        unordered_set<ADDRINT>::iterator it, et;
        for(it = b->second.begin(), 
            et = b->second.end(); 
            it!=et; 
            it++){

          if(sharingPCs.find(*it) == sharingPCs.end()){
            sharingPCs[ *it ] = 1;
          }else{
            sharingPCs[ *it ]++;
          }

        }
      }
 
      /*After i'm done detecting sharing, put my tid in that addr's set*/
      memAddrThreadSet_it->second.insert(thread_id);

    }else{

      /*Address wasn't found...*/
      memAddrThreadSet[ad] = unordered_set< THREADID >();
      memAddrThreadSet[ad].insert(thread_id);

    }
  }

  PIN_UnlockClient();
  PIN_ReleaseLock(&globalLock);
  cur->memAddrPCSet.clear(); 

}

VOID addMemOp(ADDRINT addr, ADDRINT pc, MemOpType t, THREADID tid)
{

  thread_t* current_thread = get_tls(tid);
  current_thread->memAddrPCSet[addr].insert(pc);

  PIN_GetLock(&globalLock,1);
  if( sharingPCs.find(pc) == sharingPCs.end() ){
    /*A isn't a sharing PC -- that is computed at function return time,
     *so return from this function immediately.*/
    PIN_ReleaseLock(&globalLock);
    return;
  }
  PIN_ReleaseLock(&globalLock);

  if( current_thread->PCFreq->find(pc) == 
      current_thread->PCFreq->end() ){
    (*current_thread->PCFreq)[pc] = 1;
  }else{
    (*current_thread->PCFreq)[pc]++;
  }
  current_thread->the_histo->add(pc);

}

void Routine(RTN rtn, VOID *v){
  
  if(!RTN_Valid(rtn)){
    return;
  }
  RTN_Open(rtn);
  if(!IMG_IsMainExecutable(IMG_FindByAddress(RTN_Address(rtn)))){
    RTN_Close(rtn);
    return;
  }


  for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)){	
    if(INS_IsRet(ins)){
      RTN_InsertCall(rtn, 
                     IPOINT_BEFORE,
                     (AFUNPTR)insertHash,
                     IARG_THREAD_ID,
                     IARG_INST_PTR,
                     IARG_END);
    }

    if(INS_IsMemoryRead(ins)){
      if(!INS_IsStackRead(ins)) {
      INS_InsertCall(ins,
                     IPOINT_BEFORE, 
                     (AFUNPTR)addMemOp,
                     IARG_MEMORYREAD_EA,
                     IARG_INST_PTR,
                     IARG_UINT32, MemRead,
                     IARG_THREAD_ID,
                     IARG_END);
      }
    } else if (INS_IsMemoryWrite(ins)) {

      if(!INS_IsStackWrite(ins)) {
      INS_InsertCall(ins,
                     IPOINT_BEFORE, 
                     (AFUNPTR)addMemOp,
                     IARG_MEMORYWRITE_EA,
                     IARG_INST_PTR,
                     IARG_UINT32, MemWrite,
                     IARG_THREAD_ID,
                     IARG_END);
      }
    }
  }
  RTN_Close(rtn);
}



VOID ThreadStart(THREADID thread_id, CONTEXT *ctxt, INT32 flags, VOID *v)
{

  thread_t *tdata = new thread_t();
  PIN_SetThreadData(tls_key, tdata, thread_id);

}

VOID ThreadEnd(THREADID thread_id, const CONTEXT *ctxt, INT32 code, VOID *v)
{

  insertHash(thread_id,0x0); 

  map< ADDRINT, map<ADDRINT, unsigned long> *> followGraph;

  thread_t *cur = get_tls(thread_id);
  PIN_GetLock(&FiniLock,1);

  map< ADDRINT, map<ADDRINT, unsigned long> *>::iterator i,e;
  for(i = cur->the_histo->followGraph.begin(), e = cur->the_histo->followGraph.end();
      i != e;
      i++){

    if( globalFollowGraph.find(i->first) == globalFollowGraph.end() ){
      globalFollowGraph[i->first] = new map<ADDRINT, unsigned long>();  
    }

    map<ADDRINT, unsigned long>::iterator ii,ie;
    for(ii = i->second->begin(), ie = i->second->end(); ii != ie; ii++){

      if(globalFollowGraph[i->first]->find(ii->first) == 
         globalFollowGraph[i->first]->end()){

        (*globalFollowGraph[i->first])[ii->first] = ii->second;

      }else{

        (*globalFollowGraph[i->first])[ii->first] += ii->second;

      }
      //fprintf(stderr,"0x%016lx 0x%016lx %lu\n",i->first,ii->first,ii->second);
    }
  
  }
  map<ADDRINT, unsigned long>::iterator ii,ie;
  for(ii = cur->PCFreq->begin(), ie = cur->PCFreq->end(); ii != ie; ii++){
    if( PCFreq.find(ii->first) == PCFreq.end() ){
      PCFreq[ii->first] = ii->second;
    }else{
      PCFreq[ii->first] += ii->second;
    }
  }
  
  PIN_ReleaseLock(&FiniLock);

}


// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{

  //char *pairs = getenv("APPAIRS");
  char *freqs = getenv("APFREQS");
  //FILE *pairsf = NULL;
  FILE *freqsf = NULL;
  /*if( !pairs ){
    pairsf = stderr;
  }else{
    pairsf = fopen(pairs,"w");
  }*/
  
  if( !freqs ){
    freqsf = stderr;
  }else{
    freqsf = fopen(freqs,"w");
  }

  PIN_GetLock(&FiniLock,1);
  /*map< ADDRINT, map<ADDRINT, unsigned long> *>::iterator i,e;
  for(i = globalFollowGraph.begin(), e = globalFollowGraph.end();
      i != e;
      i++){

    map<ADDRINT, unsigned long>::iterator ii,ie;
    for(ii = i->second->begin(), ie = i->second->end(); ii != ie; ii++){
      if(i->first != ii->first){
        fprintf(pairsf,"0x%016lx 0x%016lx %lu\n",i->first,ii->first,ii->second);
      }
    }
  
  }*/
  
  for(map< ADDRINT, unsigned long>::iterator it = PCFreq.begin(); it != PCFreq.end(); it++){	
    fprintf(freqsf, "0x%016lx\n", it->first );
  }
  PIN_ReleaseLock(&FiniLock);

}


int main(int argc, char *argv[])
{

    globalFollowGraph = map<ADDRINT, map<ADDRINT, unsigned long> *>();
    PCFreq = map<ADDRINT, unsigned long>();

    // Initialize pin
    PIN_InitSymbols();
    PIN_Init(argc, argv);

    // Initialize the lock
    PIN_InitLock(&globalLock);
    PIN_InitLock(&FiniLock);

    // Register Instruction to be called to instrument instructions.
    RTN_AddInstrumentFunction(Routine, 0);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadEnd, 0);

    // Register Fini to be called when the application exits.
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

