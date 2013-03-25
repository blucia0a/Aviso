#ifndef _STATEMACHINE_FACTORY_H_
#define _STATEMACHINE_FACTORY_H_
class StateMachineFactory{

public:
  virtual StateMachine *CreateMachine() = 0;
  virtual bool isStartState(Backtrace *bt) = 0;

};
#endif
