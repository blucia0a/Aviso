#ifndef _AVOIDANCE_H_
#define _AVOIDANCE_H_

#include "Misc.h"
#include "Backtrace.h"
#include "StateMachines/StateMachine.h"
#include "StateMachines/StateMachineFactory.h"
#include <pthread.h>
#include <set>
#include <tr1/unordered_set>

bool Avoidance();
void Probe();
void InitMachines();

#endif
