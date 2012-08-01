package avisooutput

import( "bytes"
        "fmt"
        "io"
        "sort"
        "../avisostats"
      )


// A data structure to hold a key/value pair. 
type Pair struct {

  Key uint
  Value float64

}

// A slice of Pairs that implements sort.Interface to sort by Value. 
type PairList []Pair
func (p PairList) Swap(i, j int) { p[i], p[j] = p[j], p[i] }
func (p PairList) Len() int { return len(p) }
func (p PairList) Less(i, j int) bool { return p[i].Value < p[j].Value }
// A function to turn a map into a PairList, then sort and return it. 
func sortMapByValue(m map[uint]float64) PairList {

   p := make(PairList, len(m))
   i := 0
   for k, v := range m {
      p[i] = Pair{k, v}
      i++
   }
   sort.Sort(p)

   return p
}

/*------------Statistics Output Code---------------*/

func printFailureRateTR(f io.Writer, fsm uint, fRate float64, total uint, chi float64){
  redCol := (uint(float64(255) * fRate))
  greenCol := (uint(255 - redCol))
  blueCol := 0

  fmt.Fprintf(f,"<tr><td>fsm%d</td><td bgcolor=#%02x%02x%02x>%.4f</td> <td bgcolor=#%02x%02x%02x>(%v runs)</td><td bgcolor=#%02x%02x%02x>(X^2 = %v)</td></tr>\n",
              fsm,
              redCol,greenCol,blueCol, fRate,
              redCol,greenCol,blueCol, total,
              redCol,greenCol,blueCol, chi)

}

func StatsHeaderString(baseFail uint, baseSucc uint) string{

  f := bytes.NewBufferString("<html><head><meta http-equiv=\"refresh\" content=\"5\"></head><body>")
  fmt.Fprintf(f,"<h3>[AVISOSERVER]</h3>\n<h4>Baseline Failure Rate: %v (%v/%v)</h4>\n\n",
             float64(baseFail)/(float64(baseFail)+float64(baseSucc)),
             float64(baseFail),float64(baseFail)+float64(baseSucc))
  return string(f.Bytes())
}

func FailureClassStatsTableString( fsmFailures *map[uint] uint,
                                   fsmSuccesses *map[uint] uint,
                                   baseFail uint,
                                   baseSucc uint) string{

  f := bytes.NewBufferString("")

  var matureFailureRates map[uint ]float64 = make(map[uint]float64)
  var newbFailureRates map[uint]float64 = make(map[uint]float64)
  var runTotals map[uint]uint = make(map[uint]uint)
  var chiSquaredValues map[uint]float64 = make(map[uint]float64)

  baselineFailureRate := float64(baseFail) / (float64(baseFail) + float64(baseSucc))



  for k,v := range *fsmSuccesses{

    if k == 99999 {  continue }

    runTotals[k] = v

    /*Get the ones that have so far only succeeded*/
    if _,ok := (*fsmFailures)[k]; !ok{

      sig := avisostats.ComputeChiSquare(0.0,float64(v),float64(baseFail),float64(baseSucc))
      if sig > 3.84 {

        matureFailureRates[k] = 0.0

      }else{

        newbFailureRates[k] = 0.0

      }
      chiSquaredValues[k] = sig

    }


  }

  for k,v := range *fsmFailures{

    if k == 99999 { continue }


    var numSuc uint = 0
    if _,ok := (*fsmSuccesses)[k]; ok{

      numSuc = (*fsmSuccesses)[k]

    }

    fRate := float64(v)/float64(v + numSuc)

    /*Now compute the chi-square statistic for this
      failure rate and the baseline to determine if
      the failure rate is dependent on this fsm being
      used; i.e., does this fsm work?*/
    sig := avisostats.ComputeChiSquare(float64(v),float64(numSuc),float64(baseFail),float64(baseSucc))

    if sig > 3.84 && fRate < baselineFailureRate {

      matureFailureRates[k] = fRate

    }else{

      newbFailureRates[k] = fRate

    }
    runTotals[k] = v + numSuc
    chiSquaredValues[k] = sig

  }

  contenders := sortMapByValue( matureFailureRates )

  fmt.Fprintf(f,"<h4>Contenders</h4>\n")
  fmt.Fprintf(f,"<table>\n")

  for i := range contenders {

    printFailureRateTR(f, contenders[i].Key, contenders[i].Value, runTotals[contenders[i].Key], chiSquaredValues[ contenders[i].Key ] )



  }

  fmt.Fprintf(f,"</table>")

  newbs := sortMapByValue( newbFailureRates )

  fmt.Fprintf(f,"<h4>Insufficient Data</h4>\n")
  fmt.Fprintf(f,"<table>\n")

  for i := range newbs{

    printFailureRateTR(f, newbs[i].Key, newbs[i].Value, runTotals[newbs[i].Key], chiSquaredValues[ newbs[i].Key ]  )

  }

  fmt.Fprintf(f,"</table>")

  return string(f.Bytes())

}

func StatsFooterString() string{

  return "</body></html>"
}

func dumpStats(f io.Writer,
               fsmFailures *map[uint] uint,
               fsmSuccesses *map[uint] uint,
               baselineFailures uint, baselineSuccesses uint) {

  h := StatsHeaderString(baselineFailures,baselineSuccesses)
  s := FailureClassStatsTableString( fsmFailures, fsmSuccesses, baselineFailures, baselineSuccesses )
  ft := StatsFooterString()
  fmt.Fprintf(f,"%s%s%s",h,s,ft)

}
/*------------END Statistics Output Code---------------*/
