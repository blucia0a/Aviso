package avisomodel

import ( "strings"
         "strconv"
         "fmt"
         "os"
         "log"
         "bytes"
         "bufio"
         "../avisofailure"
         "../avisoevent"
         "../fsm" 
       )

var thresh uint = 10
type Freq uint

type FreqVec []uint

type Elem struct{

  fsm fsm.Fsm
  freqs FreqVec

}

type Model struct{

  totalEvents uint

  pairs map[string]map[string][]uint

  /*Array of maps from strings to maps from strings to arrays of uints*/
  /*Entry per failure class.  Map from event 1 to map from event 2 to array of
 * frequencies*/
  /*failurePairs[ class ][ event1 ][ event2 ] = frequency*/
  failurePairs *[]map[string]map[string]uint

  numFailures uint

  /*TODO: Get file from config*/
  file string
  OutFile *os.File

}

func (model *Model)SetFile( fname string ){

  model.file = fname

  file, err := os.Open(fname) // For read access.

  if err != nil {

    log.Fatal(err)

  }

  model.DeserializeFromFile(file)

  file.Close()



}

func (model *Model)DeserializeFromFile( file *os.File ) {

  section := 0

  scanner := bufio.NewScanner(file)

  for scanner.Scan() {

    line := scanner.Text()
    if line == ""{ continue }

    if strings.Contains(line,"TotalEvents"){

      var junk string
      var te uint
      _,err := fmt.Sscanln(line, &junk, &te)
      if err == nil{
        fmt.Printf("[Aviso] Model contains %d events\n",te)
      }else{
        log.Fatal(err)
      }
      model.totalEvents = te


      if section != 0{

        log.Fatal("[AVISO] Error Deserializing Model - Saw TotalEvents when section was not 0")

      }else{

        section = 1
        continue

      }

    }

    if strings.Contains(line,"NumFailures"){

      var junk string
      _,err := fmt.Sscanln(line, &junk, &(model.numFailures))
      if err == nil{
        fmt.Printf("[AVISO] Model contained %d Failures\n",model.numFailures)
      }else{
        log.Fatal(err)
      }

      if section != 1{

        log.Fatal("[AVISO] Error Deserializing Model - Saw NumFailurs when section was not 1")

      }else{

        section = 2
        continue

      }

    }

    if strings.Contains(line,"failurePairs"){

      if section != 2{

        log.Fatal("[AVISO] Error Deserializing Model - Saw failurePairs when section was not 2")

      }else{

        section = 3
        continue

      }

    }

    if strings.Contains(line,"endOfModel"){

      if section != 3{

        log.Fatal("[AVISO] Error Deserializing Model - Saw endOfModel when section was not 3")

      }else{

        file.Close()
        return

      }


    }


    /*Not a section header.  We're in a section*/
    if section == 2{

      var first, second string
      var freqs []uint
      var strs []string = strings.Split(line," ")
      first = strs[0]
      second = strs[1]
      for i := range(strs){

        if i == 0 || i == 1{

          continue

        }

        s,_ := strconv.Atoi(strs[i])
        freqs = append(freqs,uint(s))

      }

      /*section 2 is the correct pairs section*/
      model.addCorrectPairWithFreqs(first,second,freqs)

    }

    /*Not a section header.  We're in a section*/
    if section == 3{

      var first, second string
      var class, freq uint
      /*section 2 is the correct pairs section*/
      ret,err := fmt.Sscanln(line, &class, &first, &second, &freq)
      if err != nil { log.Fatal(err) }
      if ret == 4{
        model.addPairToModel(class,first,second,freq)
      }

    }

  }

}

func (model *Model)SerializeToFile(  ) {

  outfile, err := os.Create(model.file) // For read access.

  if err != nil {

    log.Fatal(err)

  }

  if model == nil{

    return

  }

  f := bytes.NewBufferString("")
  fmt.Fprintf(f,"TotalEvents %d\n",model.totalEvents)
  fmt.Fprintf(f,"NumFailures %d\n",model.numFailures)

  if model.pairs != nil{

    for fst := range model.pairs{

      for snd := range model.pairs[fst]{

        fmt.Fprintf(f,"%s ",fst)
        fmt.Fprintf(f,"%s ",snd)
        for i := range model.pairs[fst][snd]{

          fmt.Fprintf(f,"%v ",model.pairs[fst][snd][i])

        }
        fmt.Fprintf(f,"\n")

      }

    }

  }

  fmt.Fprintf(f,"\nfailurePairs\n")
  if (*model.failurePairs) != nil{
    for i := range (*model.failurePairs){

      if len((*model.failurePairs)[i]) > 0{

        for fst := range (*model.failurePairs)[i]{


          for snd := range (*model.failurePairs)[i][fst]{

            /*class*/
            fmt.Fprintf(f,"%d ",i)

            /*pair first*/
            fmt.Fprintf(f,"%s ",fst)

            /*pair second and freq*/
            fmt.Fprintf(f,"%s %v",snd,(*model.failurePairs)[i][fst][snd])

            fmt.Fprintf(f,"\n")

          }

        }

      }

    }

  }

  fmt.Fprintf(f,"\nendOfModel\n")
  outfile.WriteString( string(f.Bytes()) )

  outfile.Close()

}

func (model *Model)addCorrectPairWithFreqs(first string, second string, freqs []uint){

      if _,ok := model.pairs[ first ]; !ok{

        model.pairs[ first ] = make(map[string][]uint)

      }

      if _,ok := model.pairs[ first ][ second ]; !ok{

        model.pairs[ first ][ second ] = make([]uint,len(freqs))

      }


      //TODO: iterate over range of frequencies, adding them from the update pairs to the model
      for f := range freqs {

        model.pairs[ first ][ second ][f] += freqs[f]
        model.totalEvents += freqs[f]

      }

}

func (model *Model)AddCorrectRunToModel( modelUpdate *avisoevent.Events ) {

  if model.pairs == nil{
    model.pairs = make(map[string]map[string][]uint)
  }

  var howmany uint = 0
  var updatePairs avisoevent.EventPairFreqs = (*modelUpdate).EventsToPairFreqs()

  for first := range updatePairs{

    for second := range updatePairs[ first ]{

      model.addCorrectPairWithFreqs(first,second,updatePairs[first][second])
      howmany++

    }

  }

  fmt.Printf("Correct Run updated %u pairs\n",howmany)

}




func (m *Model) addPairToModel( class uint, e1 string, e2 string, f uint ){

  //log.Printf( "PUTTING: ((%d)) ((%s)) ((%s)) ((%d)\n", class, e1, e2, f )
  if (*m.failurePairs) == nil{
    (*m.failurePairs) = make([]map[string]map[string]uint,10)
  }
  if (*m.failurePairs)[class] == nil{
    (*m.failurePairs)[class] = make(map[string]map[string]uint)
  }
  if (*m.failurePairs)[class][e1] == nil{
    (*m.failurePairs)[class][e1] = make(map[string]uint)
    (*m.failurePairs)[class][e1][e2] = 0
  }

  (*m.failurePairs)[ class ][e1][e2] += f

}

func (m *Model) AddFailureToModel( class uint, failure avisofailure.Failure ){

  /*m.failurePairs is has a map for each failure class
   *A failure class's map maps from a first event to a second event
   *and then to a count of the number of failures in which that 
   *pair of events has occurred.
   *
   *The top portion of the code in this section manages setting
   *that data structure up so that there are no nil map accesses, etc
   */
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

  /*The idea with the localmap is that we only want to
   *count each pair once per failure.  If we see a pair,
   *we add it to the set <localmap>.  Then, when we see
   *a pair that is already in the localmap, don't double
   *count it.*/
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

  model.pairs = make(map[string]map[string][]uint)
  mo := make([]map[string]map[string]uint,10)
  model.failurePairs = &mo

  if oldModel != nil{

    model.failurePairs = oldModel.failurePairs
    model.numFailures = oldModel.numFailures

  }


  lines := strings.Split( *data, "\n" )
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

      return float64( (*m.failurePairs)[class][fsm.GetFirst()][fsm.GetSecond()] ) / float64(m.numFailures)

    }

  }

  return 0.01

}

func (model *Model) RankFsm( fsm fsm.Fsm, class uint ) float64{

  newval := model.computeOrderScore(fsm) * model.computeCoOccurrenceScore(fsm) * model.computeFailureCorrelate( fsm, class )

  return newval

}
