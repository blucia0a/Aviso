#ifndef _STATEMACHINE_H_
#define _STATEMACHINE_H_
#include <string.h>
#include "SMAction.h"
#include "../Backtrace.h"

/*Interface superclass for all StateMachines*/
class StateMachine{

protected:
  char *IDString;

public:
  virtual enum SMAction run(Backtrace *bt, int tid) = 0;  
  void dumpBTs(FILE *f){ fprintf(f,IDString); }

};
#endif
