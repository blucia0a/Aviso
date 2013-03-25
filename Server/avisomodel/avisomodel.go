package avisomodel

import ( "strings"
         "strconv"
         "fmt"
         "os"
         "log"
         "os/exec"
         "../avisoglobal"
         "../avisofailure"
         "../avisoevent"
         "../fsm" )

var thresh uint = 10
type Freq uint

type FreqVec []uint

type Elem struct{

  fsm fsm.Fsm
  freqs FreqVec

}

type Model struct{

  elems []Elem

  evts map[string]bool

  totalEvents uint

  pairs map[string]map[string][]uint

  /*Array of maps from strings to maps from strings to arrays of uints*/
  /*Entry per failure class.  Map from event 1 to map from event 2 to array of
 * frequencies*/
  /*failurePairs[ class ][ event1 ][ event2 ] = frequency*/
  failurePairs *[]map[string]map[string]uint
  numFailures uint

  file string

}

func GenerateCorrectRunModel( newModelFile []string, oldmodel *Model ) *Model{

  fmt.Println("Running", avisoglobal.CorrGen, avisoglobal.CorrectModel, strings.Join(newModelFile," "))

  arg := ""+" "+avisoglobal.CorrectModel+" "+strings.Join(newModelFile," ")+""
  arg = strings.Replace( arg, "\n", "", -1 )

  out,err := exec.Command(avisoglobal.CorrGen, arg ).Output()
  if( err != nil ){
    log.Println(err)
  }

  s := string(out)

  m := NewModel( &s, oldmodel )

  os.Remove( avisoglobal.CorrectModel )
  f,_ := os.Create( avisoglobal.CorrectModel )
  fmt.Fprintf(f, "%v", string(out) )
  f.Close()

  return &m

}


func (m *Model) addPairToModel( class uint, e1 string, e2 string, f uint ){

  //log.Printf( "PUTTING: ((%s)) ((%s))\n", e1, e2 )
  if _,ok := (*m.failurePairs)[ class ][e1]; !ok{
    (*m.failurePairs)[ class ][e1] = make(map[string]uint)
  }

  (*m.failurePairs)[ class ][e1][e2] += f

}

func (m *Model) AddFailureToModel( class uint, failure avisofailure.Failure ){

  if m.failurePairs == nil {

    mo := make([]map[string]map[string]uint,10)
    m.failurePairs = &mo
    for i := range *m.failurePairs {
      if (*m.failurePairs)[i] == nil{
        (*m.failurePairs)[i] = make(map[string]map[string]uint)
      }
    }

  }

  if len(*m.failurePairs) < int(class + 1) {

    tmp := make( []map[string]map[string]uint, int(class) - len(*m.failurePairs) )

    *m.failurePairs = append(*m.failurePairs, tmp...)
    for i := range *m.failurePairs {
      if (*m.failurePairs)[i] == nil{
        (*m.failurePairs)[i] = make(map[string]map[string]uint)
      }
    }

  }

  localmap := make(map[string]map[string]bool)

  for i := range (*failure.History){

    e1e := (*failure.History)[i]
    e1 := avisoevent.EventBacktraceString(e1e)

    for j := i+1; j < len(*failure.History); j++ {

      e2e := (*failure.History)[j]
      e2 := avisoevent.EventBacktraceString(e2e)

      if _,ok := localmap[e1]; !ok{

        localmap[e1] = make(map[string]bool)

      }

      if !localmap[e1][e2] {

        localmap[e1][e2] = true
        m.addPairToModel( class, e1, e2, 1 )

      }

    }

  }
  m.numFailures++

}

func NewModel( data *string, oldModel *Model ) Model{

  model := Model{}

  if oldModel != nil{

    model.failurePairs = oldModel.failurePairs
    model.numFailures = oldModel.numFailures

  }

  lines := strings.Split( *data, "\n" )
  model.pairs = make(map[string]map[string][]uint)
  model.evts = make(map[string]bool)
  for i := range lines{

    e := Elem{}
    parts := strings.Split( lines[i], " " )
    if len(parts) < 2 { continue }
    e.fsm = fsm.NewFsmEventsOnly( parts[0], parts[1] )

    if _,ok := model.pairs[ parts[0] ]; !ok{

      model.pairs[ parts[0] ] = make(map[string][]uint)

    }

    for j := 2; j < len(parts); j++ {

      f,_ := strconv.Atoi(parts[j])
      e.freqs = append(e.freqs, uint(f))
      model.pairs[ parts[0] ][ parts[1] ] = append( model.pairs[ parts[0] ][ parts[1] ], uint(f) )
      model.totalEvents += uint(f)

    }

    model.elems = append(model.elems,e)
    model.evts[ parts[0] ] = true
    model.evts[ parts[1] ] = true

  }

  return model

}

func sumOf( ar *[]uint ) uint{

  s := uint(0)
  for i := range *ar{
    s += (*ar)[i]
  }
  return s

}

func (model *Model) computeOrderScore( fsm fsm.Fsm ) float64{

  e1 := fsm.GetFirst()
  e2 := fsm.GetSecond()

  fwdFreq := uint(0)
  revFreq := uint(0)

  if _,ok1 := model.pairs[e1]; ok1{

    if _,ok2 := model.pairs[e1][e2]; ok2{

      for i := range model.pairs[e1][e2]{
        fwdFreq += model.pairs[e1][e2][i]
      }

    }

  }

  if _,ok2 := model.pairs[e2]; ok2{

    if _,ok1 := model.pairs[e2][e1]; ok1{

      for i := range model.pairs[e2][e1]{
        revFreq += model.pairs[e2][e1][i]
      }

    }

  }

  if fwdFreq + revFreq > 0{

    return float64(revFreq) / float64(fwdFreq + revFreq)

  }else{

    return 0.5

  }
  return 0.5

}

func (model *Model) computeCoOccurrenceScore( fsm fsm.Fsm ) float64{

  e1 := fsm.GetFirst()
  e2 := fsm.GetSecond()

  sum := uint(0)

  /*Sum is the sum of all pairs that occur with k1 
    in the first positions
  */
  if _,ok := model.pairs[e1]; ok{

    for k2,_ := range model.pairs[e1]{

      for v := range model.pairs[e1][k2]{

        sum += model.pairs[e1][k2][v]

      }

    }

  }


  /*num is the number of pairs with e1 as the first
    and e2 as the second
  */
  num := uint(0)

  if _,ok := model.pairs[e1]; ok{

    if _,ok2 := model.pairs[e1][e2]; ok2{

      for i := range model.pairs[e1][e2]{

        num += model.pairs[e1][e2][i]

      }

    }

  }

  if sum > 0{

    fracOther := (  1.0 - (float64(num)/float64(sum)) )
    fracTotal := float64(sum) / float64( model.totalEvents )
    return  fracOther * fracTotal

  }else{

    if num == 0{

      return 0.5

    }else{

      return 0

    }

  }

  return 0

}

func (m *Model) computeFailureCorrelate( fsm fsm.Fsm, class uint ) float64{

  if _,ok := (*m.failurePairs)[class][fsm.GetFirst()]; ok{

    if _,ok2 := (*m.failurePairs)[class][fsm.GetFirst()][fsm.GetSecond()]; ok2{
      return float64((*m.failurePairs)[class][fsm.GetFirst()][fsm.GetSecond()]) / float64(m.numFailures)

    }

  }

  return 0.01

}

func (model *Model) RankFsm( fsm fsm.Fsm, class uint ) float64{

  newval := model.computeOrderScore(fsm) * model.computeCoOccurrenceScore(fsm) * model.computeFailureCorrelate( fsm, class )
  //newval :=  model.computeFailureCorrelate( fsm, class )
  //newval := 1.0
  //fmt.Println("Rank: ",newval)
  return newval

}
