package main

import (
        "time"
        "fmt"
        "os/exec"
        "os"
        "strings"
        "bytes"
        "io"
        "io/ioutil"
        "net/http"
        "net/url"
        "strconv"
        "log"
        "flag"
       )

func setupFailureMonitor( fmon string, fmargs string ) (*os.Process,chan bool){

  var failureChan = make(chan bool)

  fpargs := strings.Split(fmargs," ")
  fcmd := exec.Command(fmon,fpargs...)//TODO: Failure Detector args

  fsout,_ := fcmd.StdoutPipe()
  fserr,_ := fcmd.StderrPipe()
  go io.Copy(os.Stdout, fsout)
  go io.Copy(os.Stderr, fserr)

  err := fcmd.Start()
  if( err != nil ){
    log.Fatal(err)
  }

  go waitForFailure(fcmd,failureChan)

  return fcmd.Process,failureChan

}

func waitForFailure( cmd *exec.Cmd, done chan bool){

  err := cmd.Wait()
  if( err == nil ){
    done <- false
  }else{
    done <- true
  }

}

func main(){

  var program = flag.String("prog", "", "Specify the path to the program to run")
  var args = flag.String("args", "", "Specify the args to the program")
  //var failureDetector = flag.String("fd", "", "Specify the path to the program's failure detector")
  //var failureDetectorArgs = flag.String("fdargs", "", "Specify the arguments to the program's failure detector")

  var rpbPath = flag.String("rpbpath", "", "Specify the path to the program's rpb storage location")
  var fsmCompiler = flag.String("fsmcompiler", "../Scripts/compile_fsm.sh", "Specify the path to the fsm compiler")
  var fsmDir = flag.String("fsmdir", ".fsms", "Specify the path to the fsms")

  flag.Parse()

  pargs := strings.Split(*args," ")

  //buffer := bytes.NewBufferString("IR_TraceDir=");
  //fmt.Fprintf(buffer, *rpbPath)
  //myenv := append( os.Environ(), string(buffer.Bytes()) )

  /*Assumes that AVISO_SampleInterval is defined in the environment already*/
  //buffer = bytes.NewBufferString("AVISO_SampleRPB=");
  //tmpFile,_ := ioutil.TempFile(".","aviso")
  //fmt.Fprintf(tmpFile,"%v",string(buffer.Bytes()))
  //tmpFile.Close()
  //fmt.Fprintf(buffer,tmpFile.Name())
  //tmpFile.Close()
  //myenv = append( os.Environ(), string(buffer.Bytes()) )
  myenv := os.Environ()

  fmt.Println("Running the start http request")
  fsmresp, _ := http.Post("http://localhost:22221/start", "application/x-www-form-urlencoded", strings.NewReader(""))

  fsmNameBuf := bytes.NewBufferString("")

  if( fsmresp != nil ){

    contentBytes,_ := ioutil.ReadAll(fsmresp.Body)

    contents := strings.Replace( string(contentBytes), "\n", "", -1 )

    fmt.Printf("Checked about FSMS -- Response from AvisoServer: <<%s>>\n",contents)

    if( len(contents) > 0 && string(contents) != "baseline" ){

      thefsms := strings.Split( string(contents), ";")

      p := bytes.NewBufferString("IR_Plugins=")
      pc := bytes.NewBufferString("IR_PluginConfs=")

      for fsmNum := range thefsms{

        if thefsms[ fsmNum ] == "" { break }

        parts := strings.Split(string( thefsms[ fsmNum ] ),",")

        fsmName := parts[0]

        fmt.Fprintf(fsmNameBuf,"%s;",fsmName)

        fsmPathBuf := bytes.NewBufferString(*fsmDir)

        fmt.Fprintf(fsmPathBuf,"/%s",fsmName)

        fsmPath := string(fsmPathBuf.Bytes())

        if  _,err := os.Stat(*fsmDir); os.IsNotExist(err) {
          os.Mkdir(*fsmDir, 0777)
        }

        /*State machine didn't already exist*/
        f,_ := os.Create(fsmPath)
        fmt.Fprintf(f,"%v",string(parts[1]))
        fmt.Printf("Debug: FSM was %v",string(parts[1]))
        f.Close()
        fmt.Println("Running ",*fsmCompiler, " ", fsmPath)
        exec.Command( *fsmCompiler, fsmPath).Run()
        fmt.Println("Generated ",fsmPath)

        fmt.Fprintf(p,"%s.so,",fsmPath)
        fmt.Fprintf(pc,"%s,",fsmPath)

      }

      ps := string(p.Bytes())
      pcs := string(pc.Bytes())

      pA := make([]string, 2)
      pA[0] = ps
      pA[1] = pcs

      myenv = append(os.Environ(), pA...)

    }else{

      if contents == "baseline"{
        fmt.Fprintf(fsmNameBuf,"baseline")
      }

    }

  }

  fsmNames := string(fsmNameBuf.Bytes())

  var failureChan = make(chan bool)

  cmd := exec.Command( *program, pargs... )
  cmd.Env = myenv

  sout,_ := cmd.StdoutPipe()
  serr,_ := cmd.StderrPipe()
  go io.Copy(os.Stdout, sout)
  go io.Copy(os.Stderr, serr)

  startTime := time.Now()
  cmd.Start()

  //failureP,failureChan := setupFailureMonitor(*failureDetector,*failureDetectorArgs)

  err := cmd.Wait()

  endTime := time.Now( )

  fmt.Fprintln(os.Stderr,"[AVISO] Execution time %v", endTime.Sub(startTime))

  var failed bool = false
  //fmt.Println( "<<",err,">>")
  if err != nil { fmt.Println( "<<",err.Error(),">> len == ", len(err.Error()) )}
  //fmt.Println("<<Exity: ",cmd.ProcessState.Exited()," ",cmd.ProcessState.Success())
  //if !cmd.ProcessState.Exited() || !cmd.ProcessState.Success() {

  if err != nil && err.Error() != "" && (strings.Contains(err.Error(),"signal 6") || strings.Contains(err.Error(),"signal 11")){
    failed = true
  }

  /*Assume we have to kill and there was no failure*/
  var exited bool = false
  select{

    default:
      /*!exited and !failed*/

    case tfailed := <-failureChan:
      exited = true
      if( !failed ){ failed = tfailed }

  }

  v := url.Values{}
  v.Set("fail","0")
  v.Set("fsm",fsmNames)
  v.Set("pid",strconv.Itoa(cmd.Process.Pid))


  if( failed ){


    fmt.Println("Sending FSM named ",fsmNames)
    rpbNameBuf := bytes.NewBufferString("")
    fmt.Fprintf(rpbNameBuf,"%v/RPB%v",*rpbPath,strconv.Itoa(cmd.Process.Pid))
    rpbData,err := ioutil.ReadFile(string(rpbNameBuf.Bytes()))
    if( err != nil ){
      log.Fatal(err)
    }
    v.Set("rpb",string(rpbData))

    /*e && exited and e && exited*/
    /*Both mean there could've been a failure cause the failure monitor has exited*/
    v.Set("fail","1")
    fmt.Fprintf(os.Stderr,"Possible failure: Program or Failure Monitor Exited with Non-Zero Status\n")

    if( !exited ){

      //fmt.Fprintf(os.Stderr,"Shutting Down Failure Detector\n")
      //failureP.Kill()

    }

  }else{

    fmt.Println("Sending Non-Failure Termination")
    v.Set("fail","0")
    if( !exited ){

      /*!e && !ok -- means failure monitor didn't exit*/
      //fmt.Fprintf(os.Stderr,"Shutting Down Failure Detector\n")
      //failureP.Kill()

    }else{

      //fmt.Fprintf(os.Stderr,"Failure Detector Exited with zero status (probably not a failure)\n")

    }

    rpbNameBuf := bytes.NewBufferString("")
    fmt.Fprintf(rpbNameBuf,"%v/RPB%v",*rpbPath,strconv.Itoa(cmd.Process.Pid))
    rpbData,err := ioutil.ReadFile(string(rpbNameBuf.Bytes()))
    if( err != nil ){
      log.Fatal(err)
    }
    v.Set("rpb",string(rpbData))

  }

  fmt.Println("Post...")
  resp, _ := http.Post("http://localhost:22221/end", "application/x-www-form-urlencoded", strings.NewReader(v.Encode()))
  if( resp != nil ){
    fmt.Printf("Response from AvisoServer: %v\n",resp)
  }


}
