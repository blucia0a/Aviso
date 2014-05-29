#include "StateMachines/StateMachine.h"
#include "StateMachines/StateMachineFactory.h"
#include "Backtrace.h"
void loadPlugins(std::set<StateMachineFactory *> *smfs,const char *va);
void loadPluginConfigs(std::unordered_set< Backtrace *, BTHash, LooseBTEquals> *btSet,const char *va);
