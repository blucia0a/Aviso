#include "STQueue.h"
#include <stdlib.h>

/* 
 * Cur starts at 0.  each add frees cur if it isn't null, and increments it %
 * RECENCY_WINDOW.  Cur is the tail. q[(cur + 1) % RECENCY_WINDOW] is the head / OLDEST ENTRY
 */
STQueue::STQueue(){

  q = new TraceEvent [RECENCY_WINDOW];
  memset(q,0,sizeof(TraceEvent) * RECENCY_WINDOW);
  cur = 0; 

}

TraceEvent *STQueue::GetNextEvent(){
  TraceEvent *ret = &(q[cur]);
  cur = (cur + 1) % RECENCY_WINDOW;
  return ret; 
}

void STQueue::Add(TraceEvent *t){
 
 
}

void STQueue::Dump(FILE *out){

  char *json = getenv("AVISO_JSON");
  if( json == NULL ){

    /*Dumps the queue's contents oldest-first*/
    for(int i = ((cur + 1) % RECENCY_WINDOW);
        i != cur;
        i = (i + 1) % RECENCY_WINDOW){
      if(q[i].time_ns != 0){
        fprintf(out,"%lu:",q[i].time_ns);
        fprintf(out,"%lu:",q[i].actor);
        for(int bti = 0; bti < BTLEN-1; bti++){
          fprintf(out,"%p->",q[i].bt[bti]);
        }
        fprintf(out,"%p\n",q[i].bt[BTLEN-1]);
      }
    }
  }else{

    for(int i = ((cur + 1) % RECENCY_WINDOW);
        i != cur;
        i = (i + 1) % RECENCY_WINDOW){
      if(q[i].time_ns != 0){
        fprintf(out,"{ ");
        fprintf(out,"\"time\" : %lu,",q[i].time_ns);
        fprintf(out,"\"thread\" : %lu,",q[i].actor);
        fprintf(out,"\"backtrace\" : [",q[i].actor);
        for(int bti = 0; bti < BTLEN-1; bti++){
          fprintf(out,"\"%p\",",q[i].bt[bti]);
        }
        fprintf(out,"\"%p\"]",q[i].bt[BTLEN-1]);
        fprintf(out,"},\n");
      }
    }
  }
}
