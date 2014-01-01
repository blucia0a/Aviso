package avisomsgs

/*
AvisoMsgs - this is a set of message data structures that are passed
            between the main components of the system.  
*/

import (
        "../avisoevent"
        "../fsm"
       )

type FailureClassUpdate struct{

  Fsms *fsm.Fsmlist
  ClassIndex uint

}

type StatsUpdate struct{

  Fsm fsm.FsmTag
  Successes uint
  Failures uint
  ChiSquare float64
  BaselineSucc uint
  BaselineFail uint

}

type Termination struct{

  Fsm string
  Fail bool
  Pid uint
  Rpb avisoevent.Events

}

type RpbUpdate struct{

  Rpb *avisoevent.Events
  IsFailure bool

}

