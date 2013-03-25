#ifndef __STQUEUE_H_
#define __STQUEUE_H_

#include "TraceEvent.h"
#include <stdio.h>
#include <string.h>

#define RECENCY_WINDOW 100
class STQueue{
  
  TraceEvent *q; 
  unsigned cur;

public:
  STQueue();
  void Add(TraceEvent *t);
  TraceEvent *GetNextEvent();
  void Dump(FILE *f);

};

#endif
