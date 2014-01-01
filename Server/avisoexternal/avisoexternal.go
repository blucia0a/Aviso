package avisoexternal

import (
        "fmt"
        "log"
        "strings"
        "os/exec"
        "../fsm"
        "../avisoglobal"
       )

func GenerateFSMs(rpbFileName string ) fsm.Fsmlist{

  var fsms fsm.Fsmlist

  var fsmstrs []string

  fmt.Println("Running",avisoglobal.FsmGen,rpbFileName,avisoglobal.CorrectModel)

  out,err := exec.Command(avisoglobal.FsmGen, rpbFileName, avisoglobal.CorrectModel).Output()

  if( err == nil ){

    fsmstrs = strings.Split(string(out), "\n")

  }else{

    log.Println(err)

  }

  fsms = make( fsm.Fsmlist, len(fsmstrs) )
  for i := range fsmstrs{

    if fsmstrs[i] != "" && fsmstrs[i] != " "{

      fsms[i] = fsm.NewFsm( fsmstrs[i] )

    }

  }

  return fsms

}
