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

/*Normalizes addresses -- (nil) becomes 0x0
  TODO: find library addresses more reliably
  >0x700000000000  becomes 0xffffffffffffffff;
*/
/*End Event structures and sorting code */
