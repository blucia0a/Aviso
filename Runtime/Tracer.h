#ifndef _TRACER_H_
#define _TRACER_H_

#include <stdio.h>
#define EVENT_BUFFER_SIZE 1

class Tracer{

  typedef struct _Event{
    char type;
    unsigned long lockAddr;
    unsigned long timestamp;
    void *evt; 
  } Event;

  Event events[EVENT_BUFFER_SIZE];
  unsigned cur;

public:
  FILE *traceFile;
  Tracer();
  Tracer(unsigned long ind);

  virtual void AddEvent(char type,
                unsigned long lockAddr, 
                void *evt, 
                unsigned long long timestamp);
  virtual void FlushTrace();

};
#endif
