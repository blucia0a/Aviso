package avisoevent

import (
         "bytes"
         "fmt"
         "encoding/json"
         "sort"
         "errors"
         "strings"
         "strconv"
       )


/*Event structures and sorting code (move to module?)*/
type Event struct{
  Time float64
  Thread float64
  Backtrace []string
}

type Events []*Event;
type EventPairFreqs map[string]map[string][]uint


type ByTime struct{ Events }

func (s ByTime) Less(i, j int) bool { return s.Events[i].Time < s.Events[j].Time }
func (s Events) Len() int { return len(s) }
func (s Events) Swap(i, j int) { s[i], s[j] = s[j], s[i] }

func RpbStringToEvents(s string) (Events,error){


  buf := bytes.NewBufferString("")
  fmt.Fprintf(buf,"[ %s \n {\"time\" : 0,\"thread\" : 0, \"backtrace\" : [\"(nil)\",\"(nil)\",\"(nil)\",\"(nil)\",\"(nil)\"]} ]",s)
  var events Events

  err := json.Unmarshal(buf.Bytes(),&events);
  if err != nil {
    fmt.Printf("PARSE JSON FAILED",string(buf.Bytes()))
    events = Events{}
    e := errors.New("JSON FAILURE")
    return nil,e
  }

  sort.Sort(ByTime{events})

  retEv := events[1:len(events)]

  return retEv,nil


}


func normalizeAddr(adStr string) string{

  var ad uint64
  if strings.Contains(adStr, "(nil)"){

    return "0x0"

  }else{

    adi,_ := strconv.ParseUint(adStr,0,64)
    ad = uint64(adi)
    if ad > 0x700000000000 {

      return "0xffffffffffffffff"

    }else{

      return fmt.Sprintf("0x%x",ad)

    }

  }
  return "0x0"

}

func NormalizeAddrs(e *Event) {

  for i := range((*e).Backtrace){
    (*e).Backtrace[i] = normalizeAddr((*e).Backtrace[i])
  }

}

func EventToString(e *Event) string{
  buf := bytes.NewBufferString("")
  fmt.Fprintf(buf,"%v",e.Thread)
  for i := range e.Backtrace{
    fmt.Fprintf(buf,":%v",e.Backtrace[i])
  }
  return string(buf.Bytes())
}



func EventBacktraceString(e *Event) string{
  buf := bytes.NewBufferString("")
  for i := range e.Backtrace{
    if i == 0{
      fmt.Fprintf(buf,"%v",e.Backtrace[i])
    }else{
      fmt.Fprintf(buf,":%v",e.Backtrace[i])
    }
  }
  return strings.Replace(string(buf.Bytes()), "(nil)", "0x0", -1)
}

/*The input is a *Events -- a list of events*/
/*The output should be a list like
  event1 event2 freq1 freq2 freq3 freq4 ... freq<thresh>
*/
func (events *Events) EventsToPairFreqs() EventPairFreqs{

  var THRESHOLD int = 10

  for i := range(*events) {
    NormalizeAddrs((*events)[i])
  }

  histo := make(map[string]map[string][]uint)

  for i := 0; i < len(*events); i++ {

    for j := i; j < i + THRESHOLD && j < len(*events); j++ {

      var fst *Event = (*events)[i]
      var snd *Event = (*events)[j]

      if fst.Thread != snd.Thread{

        var fString string = EventBacktraceString(fst)
        var sString string = EventBacktraceString(snd)
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

func (histo EventPairFreqs) PairStrings() []string{

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


/*Normalizes addresses -- (nil) becomes 0x0
  TODO: find library addresses more reliably
  >0x700000000000  becomes 0xffffffffffffffff;
*/
/*End Event structures and sorting code */
