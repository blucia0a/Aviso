package avisoevent

import (
         "bytes"
         "fmt"
         "encoding/json"
         "sort"
         "errors"
         "strings"
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
  //var b []byte = []byte(buf.Bytes())
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


/*End Event structures and sorting code */
