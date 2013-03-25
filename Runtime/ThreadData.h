#include "Misc.h"
#include "Backtrace.h"
#include "StateMachines/StateMachine.h"
#include "StateMachines/StateMachineFactory.h"
#include "STQueue.h"
#include <pthread.h>
#include <set>
#include <tr1/unordered_set>

using std::set;
using std::tr1::unordered_set;

typedef struct _thread_data{

  /*Avoidance-related*/
  set<StateMachineFactory *> *myFactories;
  unordered_set<Backtrace *, BTHash, LooseBTEquals> *involvedBacktraces;

  /*Correct RPB Sampling*/
  unsigned long lastDump;
  unsigned long correctRunDumpInterval;
  bool correctDumped;

  /*RPB State*/
  Backtrace *bt;
  unsigned synthEvTime;
  unsigned syncEvTime;
  STQueue *myRPB;

  /*Event Allocation State*/
  TraceEvent **eventFreeList;
  TraceEvent **eventFreeListNext;
  TraceEvent **eventFreeListTail;

  /*Backtrace Collection State*/
  void **btbuff;
  void **btbuffend;
  int btbuff_init;

  /*Miscellaneous Admin*/
  unsigned long mytid;
  bool alreadyDumped;

  /*Experimental Stats*/
  unsigned long numSynthEvents;
  unsigned long numSynthBail;
  unsigned long numSynthPartial;
  unsigned long numSyncEvents;
  unsigned long numSyncPartial;
  unsigned long numMachineStartChecks;
  unsigned long numMachineStarts;
  unsigned long curActiveMachines;
  unsigned long maxActiveMachines;

} ThreadData;
