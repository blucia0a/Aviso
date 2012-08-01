#ifndef _TRACE_EVENT_H_
#define _TRACE_EVENT_H_
#include <pthread.h>
#include "Misc.h"
typedef struct _TraceEvent{
  char type;
  void *bt[BTLEN];
  unsigned long actor;
  unsigned long time_ns;
  bool trunc;
} TraceEvent;
#endif
