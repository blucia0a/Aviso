package avisofailure

import (
        "bytes"
        "fmt"
        //"os"
        "io/ioutil"
        "../fsm"
        "../avisoevent"
        "../avisoexternal"
        "../distribution"
       )


type Failure struct{

  Fsms fsm.Fsmlist /*a set of fsm strings*/
  History *avisoevent.Events /*the history that generated these fsms*/
  EventSet *map[string]bool

}

func NewFailure(events *avisoevent.Events) Failure{

  var f Failure = Failure{}

  f.History = events
  f.EventSet = new(map[string]bool)
  *f.EventSet = make(map[string]bool)

  /*To generate FSMs, record the event list in a temp
  file prefixed with "aviso"*/
  buffer := bytes.NewBufferString("");
  for v := range *events{

    s := avisoevent.EventToString((*events)[v])
    fmt.Fprintf(buffer,"%v\n",s)

    (*f.EventSet)[ avisoevent.EventBacktraceString( (*events)[v] ) ] = true

  }

  tmpFile,_ := ioutil.TempFile(".","aviso")
  fmt.Fprintf(tmpFile,"%v",string(buffer.Bytes()))
  tmpFile.Close()
  fmt.Println("generateFSMs's RPB is in ",tmpFile.Name())

  /*Generate a set of FSM descriptors from the temp file*/
  f.Fsms = avisoexternal.GenerateFSMs(tmpFile.Name())

  //os.Remove(tmpFile.Name())

  return f

}


func FailureSimilarity( other *Failure, thisFailure *Failure ) float64{

  /*Size of the symmetric difference as a fraction of all elements in either set*/
  total := make( map[string]bool )

  otherNotThis := 0
  thisNotOther := 0

  for o,_ := range (* (*other).EventSet ){

    if !(*(*thisFailure).EventSet)[ o ]{

      otherNotThis++

    }
    total[ o ] = true

  }

  for o,_ := range (* (*thisFailure).EventSet ){

    if !(*(*other).EventSet)[ o ]{
      thisNotOther++
    }
    total[ o ] = true

  }

  return (1.0 - (  (float64(otherNotThis) + float64(thisNotOther)) / float64( len(total) )  ))


}


type Failurelist []Failure

func (f *Failurelist) UniqueFsmList() *fsm.Fsmlist{

  /*Returns a list of unique fsms from all fsmlists in this failure list*/
  var retArr fsm.Fsmlist
  var tmpMap map[string]bool = make(map[string]bool)
  for i := range *f{
    for j := range (*f)[i].Fsms{
      ft := (*f)[i].Fsms[j]
      t := ft.EventString()
      if !tmpMap[ t ] {
        retArr = append(retArr, ft)
      }
      tmpMap[ t ] = true
    }
  }

  return &retArr

}


type FailureClass struct{

  Dist *distribution.Distribution /*The distribution describing how to draw fsms*/
  DrawChan chan uint
  Fsms *fsm.Fsmlist /*The fsms*/
  FsmRanks []float64 /*The model built from correct runs*/
  StatsSnapshot []float64 /*the failure rate of each fsm*/
  EffectiveFsms map[uint]float64 /*map from fsm index to X^2 value - only set if the fsm is significant*/

}


type FailureClassStats struct{

  FsmFailures map[uint]uint
  FsmSuccesses map[uint]uint

}

func FailureClassSimilarity( other *Failurelist, thisFailure *Failure ) float64{

  simSum := 0.0
  for o := range *other{

    simSum += FailureSimilarity( &((*other)[o]), thisFailure )

  }

  return simSum / float64(len( *other ))

}

func FindFailureClass( classes *[]Failurelist, thisFailure *Failure ) uint{

  winner := 0
  winnerScore := 0.0
  for c := range *classes{
    if score := FailureClassSimilarity( &((*classes)[c]), thisFailure ); score > winnerScore{
      winnerScore = score
      winner = c
    }
  }
  return uint(winner)

}
