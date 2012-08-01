#include "Backtrace.h"
#include "Applier.h"
#include "StateMachines/StateMachine.h"
#include "StateMachines/StateMachineFactory.h"
#include <tr1/unordered_set>
#include <set>
#include <dlfcn.h>

struct pcbt {
  bool skipName;
  std::tr1::unordered_set< Backtrace *, BTHash, LooseBTEquals> *btSet;
  Backtrace *b;
  int i;
};


void processPluginConfigBT(char *btAddr, void *v);
void processPluginConfigLine(char *fullbt,void *v);
void processPluginConfig(char *conf,void *v);
void processPlugin(char *pi, void *v);

void processPluginConfigBT(char *btAddr, void *v){

  unsigned long a;
  struct pcbt *pcb = (struct pcbt *)v;
  sscanf(btAddr,"%lx",&a);
  pcb->b->bt[pcb->i++] = (void*)a;

}

void processPluginConfigLine(char *fullbt,void *v){

  struct pcbt *pcb = (struct pcbt *)v;
  bool skipName = pcb->skipName;
  if(skipName){
     
    pcb->skipName = false;

  }else{

    pcb->b = new Backtrace();
    pcb->i = 0;

    applyToTokens(fullbt,":",processPluginConfigBT,v);

    pcb->btSet->insert(pcb->b);

  }

}

void processPluginConfig(char *conf,void *v){

  FILE *f = fopen(conf,"r");
  if(f == NULL){
    return;
  }

  struct pcbt pcb;
  pcb.skipName = true;
  pcb.btSet = 
    (std::tr1::unordered_set< Backtrace *, BTHash, LooseBTEquals> *) v;
  char buf[1024];
  memset(buf,0,1024);
  fgets(buf,1024,f);

  applyToTokens(buf," ",processPluginConfigLine,(void*)&pcb);

  fclose(f);

}

void loadPluginConfigs(std::tr1::unordered_set< Backtrace *, BTHash, LooseBTEquals> *btSet, const char *va){
  
  char *plugins = getenv(va);
  if(plugins != NULL){

    applyToTokens(plugins,", \n",processPluginConfig,(void*)btSet);

  }else{

    //fprintf(stderr,"Couldn't process the config list in\n"); 

  }
  
}

void processPlugin(char *pi, void *v){

  fprintf(stderr,"Registering plugin: %s\n",pi);
  std::set<StateMachineFactory *> *smfs =
    (std::set<StateMachineFactory *> *)v;
  StateMachineFactory *(*fptr)();

  void *pluginObject = dlopen(pi, RTLD_LOCAL | RTLD_LAZY);
  if(pluginObject == NULL){
    //fprintf(stderr,"Couldn't dlopen %s\n",pi);
    fprintf(stderr,"%s\n",dlerror());
  }
  *(void **)(&fptr) = dlsym(pluginObject, "getFactory");

  if(fptr != NULL){

    smfs->insert((*fptr)());

  }else{

    fprintf(stderr,"Goofy plugin.  Can't load function\n");

  }

}

void loadPlugins(std::set<StateMachineFactory *> *smfs, const char *va){

  char *plugins = getenv(va);
  if(plugins != NULL){

    applyToTokens(plugins,", ",processPlugin,(void*)smfs);

  }else{

    //fprintf(stderr,"Couldn't open plugins\n");

  }

}
