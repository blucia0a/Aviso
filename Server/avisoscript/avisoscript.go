package avisoscript

import(
        "strings"
        "../avisoevent"
      )

var files []string
var done bool

var THRESHOLD int = 10


/*The input is a *avisoevent.Events -- a list of events*/
/*The output should be a list like
  event1 event2 freq1 freq2 freq3 freq4 ... freq<thresh>
*/
func ProcessEvents(events *avisoevent.Events) map[string]map[string][]uint{

  for i := range(*events) {
    avisoevent.NormalizeAddrs((*events)[i])
  }

  histo := make(map[string]map[string][]uint)

  for i := 0; i < len(*events); i++ {

    for j := i; j < i + THRESHOLD && j < len(*events); j++ {

      var fst *avisoevent.Event = (*events)[i]
      var snd *avisoevent.Event = (*events)[j]

      if fst.Thread != snd.Thread{

        var fString string = avisoevent.EventBacktraceString(fst)
        var sString string = avisoevent.EventBacktraceString(snd)
        var found bool = false

        if _,ok := histo[fString]; ok{

          if _,ok2 := histo[fString][sString]; ok2{

            found = true
            if j - i < THRESHOLD {
              histo[fString][sString][j - i]++
            }

          }

        }

        if !found {

          if _,ok := histo[fString]; !ok{

            histo[fString] = make(map[string][]uint)

          }

          if _,ok := histo[fString][sString]; !ok{

            histo[fString][sString] = make([]uint,THRESHOLD)

          }

          if j - i < THRESHOLD {

            histo[fString][sString][j - i]++

          }

        }

      }

    }

  }

  /*histo now contains frequencies of each pair's occurrence within each of the
   * <THRESHOLD> ranges evaluated*/
  return histo

}

func PrintPairs (histo map[string]map[string][]uint) []string{

  uniq := make(map[string]bool)
  for fst,_ := range histo {

    for snd,_ := range(histo[fst]) {

      s := []string{fst,snd}
      uniq[ strings.Join(s, " ") ] = true

    }

  }

  var pairs []string
  for k := range(uniq){
    pairs = append(pairs, k)
  }
  return pairs

}
