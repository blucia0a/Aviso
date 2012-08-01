package fsm

import ( "fmt"
         "strings"
         "strconv"
         "bytes" )

type Fsm struct{
  name string
  first string
  second string
  rank float64
}


func NewFsm( fsmstr string ) Fsm{

  parts := strings.Split(fsmstr, " ")
  if( len(parts) != 4 ){
    fmt.Printf("WEIRD: %v\n",parts)
  }

  rank,err := strconv.ParseFloat(parts[3],64)
  if( err != nil ){ return Fsm{} }

  return Fsm{ name: parts[0], first: parts[1], second: parts[2], rank: rank }

}

func NewFsmEventsOnly(ev1 string, ev2 string) Fsm{

  return Fsm{name: "", first: ev1, second: ev2, rank: 0}

}

func (s *Fsm) ToString() string{
  b := bytes.NewBufferString("")
  fmt.Fprintf(b, "%s %s %s", s.name, s.first, s.second)
  return string(b.Bytes())
}

func (s *Fsm) EventString() string{
  b := bytes.NewBufferString("")
  fmt.Fprintf(b, "%s %s", s.first, s.second)
  return string(b.Bytes())
}

func (s *Fsm) SetRank(r float64) {
  s.rank = r
}

func (s *Fsm) GetRank() float64{
  return s.rank
}

func (s *Fsm) GetFirst() string{
  return s.first
}

func (s *Fsm) GetSecond() string{
  return s.second
}

type Fsmset map[Fsm]bool

func (s *Fsmset) ToList() *Fsmlist{

  var fsmlist Fsmlist = make( Fsmlist, len(*s) )
  i := 0
  for f,_ := range *s{
    fsmlist[i] = f
  }
  return &fsmlist

}

type Fsmlist []Fsm

type FsmTag struct{
  class uint
  fsm uint
}

func FsmTagCtr(c uint, r uint) FsmTag{
  return FsmTag{ class: c, fsm: r }
}

func (t FsmTag) Class() uint{
  return t.class
}

func (t FsmTag) Fsm() uint{
  return t.fsm
}

func (t FsmTag) ToName() string{
  b := bytes.NewBufferString("fsm")
  fmt.Fprintf(b,"%v.%v",t.class,t.fsm)
  return string(b.Bytes())
}

func NewFsmTag(name string) FsmTag{

  sp := strings.Split( strings.Replace( name, "fsm", "", 1 ), "." )
  class,_ := strconv.Atoi( sp[0] )
  fsm,_ := strconv.Atoi( sp[1] )
  return FsmTag{uint(class),uint(fsm)}
}

