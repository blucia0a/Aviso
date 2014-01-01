package main

import (
	"./avisoevent"
	"./avisofailure"
	"./avisoglobal"
	"./avisomodel"
	"./avisomsgs"
	"./avisooutput"
	"./avisostats"
	"./distribution"
	"./fsm"
	"bytes"
	"flag"
	"fmt"
	"html"
	"io/ioutil"
	"log"
	"math"
	"math/rand"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

func doDistributionUpdate(which uint, rank float64, failureRate float64, dist *distribution.Distribution) {

	scale := float64(0)

	if failureRate < 0 {

		scale = 1.0

	} else {

		/*The failure rate is in [0,1]    */
		successRate := float64(1.0 - failureRate)
		fmt.Printf("[AVISO] Success rate was %f\n", successRate)

		if successRate < 0 {

			log.Printf("[AVISO] Success rate %f out of range in doDistributionUpdate\n", successRate)
			return

		}

		if successRate == 1.0 {

			scale = 50

		} else {

			scale = math.Exp(8*(successRate-0.7)) + 0.001

		}

		if scale <= 0 {

			log.Printf("[AVISO] Scale value %f out of range in doDistributionUpdate\n", scale)
			scale = 0.001

		}

	}

	fmt.Printf("[AVISO] Scale value was %f\n", scale)

	var newVali int = 0
	newVali = int(rank * 1000 * scale)

	newVal := uint(newVali)

	fmt.Printf("[AVISO] Updating fsm %d to %d (rank=%f, rate=%f scale=%f)\n", which, newVal, rank, failureRate, scale)
	if newVal == 0 {

		newVal = 1
		fmt.Printf("[AVISO] Updating fsm %d, but got 0 (rank=%f, rate=%f scale=%f)\n", which, rank, failureRate, scale)

	}

	dist.Update(which, newVal)

}

/*dealFSMs takes a list of fsms and doles them out to instances
  as they start.

  dealFSMs is started by rpbManager when rpbManager receives its
  first RPB.

  dealFSMs communicates over 2 channels:

  1)dealFSMs sends fsms out on fsmChan.  fsmChan is read by
    the server startHandler

  2)dealFSMs sends requests to the stats manager via dealStatsChan
    and receives stat updates from the stats manager over dealStatsChan 
*/

func dealFSMs(rpbDealChan chan avisomsgs.FailureClassUpdate) {

	var mostRuns uint = 0
	var failureClasses []avisofailure.FailureClass = make([]avisofailure.FailureClass, 1000)

	var baselineSuccesses uint
	var baselineFailures uint

	var E float64 = 0.05 //5% maximum error from the real baseline rate
	var Z float64 = 2    //95% likelihood we got it within our error rate

	//E = Z * Sp

	/*Get the first failure class update packet*/
	classUpdate := <-rpbDealChan

	ind := classUpdate.ClassIndex

	failureClasses[ind].FsmRanks = make([]float64, len(*classUpdate.Fsms))

	/*Allocation Note: classUpdate.fsms was allocated by dealFSMs*/
	failureClasses[ind].Fsms = classUpdate.Fsms

	//fmt.Printf("Ranks from FailureClassUpdate were (before distribution update)")
	for i := range *classUpdate.Fsms {

		failureClasses[ind].FsmRanks[i] = (*classUpdate.Fsms)[i].GetRank()
		//fmt.Printf(" %v", failureClasses[ ind ].FsmRanks[ i ])

	}
	//fmt.Printf("\n")

	failureClasses[ind].Dist = distribution.NewDistribution(&(failureClasses[ind].FsmRanks))

	failureClasses[ind].DrawChan = make(chan uint)
	go (*failureClasses[ind].Dist).Draw(failureClasses[ind].DrawChan)

	/*statsSnapshot is a record of the last known failure
	  rate for each FSM, as reported by the stats manager*/
	failureClasses[ind].StatsSnapshot = make([]float64, len(*failureClasses[ind].Fsms))

	/*Before doing anything, dealFSMs records the FSMs it has 
	  received for debugging and forensics purposes -- mine
	  and the devs of the software aviso is working for.*/
	f, _ := os.Create(".avisoFSMRecord")
	for i := range *failureClasses[ind].Fsms {
		fmt.Fprintf(f, "%+v\n", (*failureClasses[ind].Fsms)[i].ToString())
	}
	f.Close()

	/*The main loop is to ask for classUpdates, or requests for FSMs*/
	useFSM := 0
	var numFSMTrials int

	for {

		/*This string holds the FSM command string we'll return over HTTP*/
		curStrB := bytes.NewBufferString("")

		baseRate := float64(baselineFailures) / (float64(baselineFailures) + float64(baselineSuccesses))
		baseDev := math.Sqrt((baseRate * (1.0 - baseRate)) / float64(baselineFailures+baselineSuccesses))

		currentError := Z * baseDev

		if baselineFailures+baselineSuccesses <= 0 || currentError > E {

			fmt.Println("Not confident about baseline failure rate.  Experimenting more.  Error=", currentError)
			numFSMTrials = 5

		} else {

			fmt.Println("Confident about baseline failure rate.  Experimenting less.  Error=", currentError)
			numFSMTrials = 100

		}

		val := rand.Intn(numFSMTrials)
		if val != 1 && !avisoglobal.BaselineOnly {

			//useFSM++
			//if( useFSM >= numFSMTrials ){ useFSM = 0 }

			/*We should return an FSM for each failure class*/
			for classInd := 0; failureClasses[classInd].Dist != nil; classInd++ {

				/*r represents the FSM that is currently "on deck"*/
				var r uint = 0

				/*Once we have contenders, use them 90% of the time and explore 10%*/
				haveSig := (len(failureClasses[classInd].EffectiveFsms) > 0)
				useSig := rand.Intn(10)
				if haveSig && useSig != 1 {

					/*Contender Mode: use one from the EffectiveFsms list*/
					/*We may want to change this to choose the one we're most confident in*/
					best := 1.0
					bestInd := -1
					for cur, _ := range failureClasses[classInd].EffectiveFsms {

						if failureClasses[classInd].StatsSnapshot[cur] <= best {

							best = failureClasses[classInd].StatsSnapshot[cur]
							bestInd = int(cur)

						}

					}
					r = uint(bestInd)
					fmt.Printf("[AVISO] Drew effective %d.%d\n", classInd, r)

				} else {

					/*Exploration Mode: Draw at random!*/
					failureClasses[classInd].DrawChan <- 0
					r = <-failureClasses[classInd].DrawChan

				}

				req := fsm.FsmTagCtr(uint(classInd), r)

				/*construct the name of the fsm that was chosen*/
				fmt.Fprintf(curStrB, "%s,%s;", req.ToName(), (*failureClasses[classInd].Fsms)[r].ToString())

			}

		} else {

			useFSM++
			fmt.Fprintf(curStrB, "baseline")

		}

		/*Send the fsm string to the startHandler*/
		select {

		case statsUpdate := <-avisoglobal.DealStatsChan:
			/*Ask the stats manager for an update on this fsms failure
			  rate while we're at it*/
			c := statsUpdate.Fsm.Class()
			f := statsUpdate.Fsm.Fsm()
			baselineSuccesses = statsUpdate.BaselineSucc
			baselineFailures = statsUpdate.BaselineFail

			numRunsSeen := (uint(statsUpdate.Successes) + uint(statsUpdate.Failures))
			if numRunsSeen > mostRuns {
				mostRuns = numRunsSeen
			}

			now := time.Now()
			fmt.Printf("[AVISO] Stats update for %d.%d at %v (%f %%)\n", c, f, now, float64(statsUpdate.Failures)/(float64(statsUpdate.Successes)+float64(statsUpdate.Failures)))
			failureClasses[c].StatsSnapshot[f] = float64(statsUpdate.Failures) / (float64(statsUpdate.Successes) + float64(statsUpdate.Failures))

			//if statsUpdate.Failures + statsUpdate.Successes > 0{
			//fmt.Printf("Updating distribution from status update in dealFSMs\n")

			v := rand.Intn(5)
			if v == 0 {
				go doDistributionUpdate(uint(f), (*failureClasses[c].Fsms)[f].GetRank(),
					failureClasses[c].StatsSnapshot[f],
					failureClasses[c].Dist)
			}
			//}

			if failureClasses[c].EffectiveFsms == nil {
				failureClasses[c].EffectiveFsms = make(map[uint]float64)
			}

			if statsUpdate.ChiSquare > 0 {

				fmt.Printf("[AVISO] Got an effective fsm %d.%d\n", c, f)
				failureClasses[c].EffectiveFsms[f] = statsUpdate.ChiSquare

			} else {

				if _, ok := failureClasses[c].EffectiveFsms[f]; ok {
					delete(failureClasses[c].EffectiveFsms, f)
				}

			}

			now = time.Now()
			fmt.Printf("[AVISO] Done with stats update for %d.%d at %v (%f %%)\n", c, f, now, float64(statsUpdate.Failures)/(float64(statsUpdate.Successes)+float64(statsUpdate.Failures)))

		case avisoglobal.FsmChan <- string(curStrB.Bytes()):
			now := time.Now()
			fmt.Printf("[AVISO] Sent %s at %v\n", string(curStrB.Bytes()), now)

		case classUpdate := <-rpbDealChan:

			now := time.Now()
			fmt.Printf("[AVISO] Got a class update for class %d at %v\n", classUpdate.ClassIndex, now)

			ind := classUpdate.ClassIndex
			failureClasses[ind].FsmRanks = make([]float64, len(*classUpdate.Fsms))

			/*statsSnapshot is a record of the last known failure
			  rate for each FSM, as reported by the stats manager*/

			if len(*classUpdate.Fsms) > len(*failureClasses[ind].Fsms) {

				tmp := make([]float64, len(*classUpdate.Fsms)-len(*failureClasses[ind].Fsms))

				for i := range tmp {

					tmp[i] = float64(-1.0)

				}

				failureClasses[ind].StatsSnapshot = append(failureClasses[ind].StatsSnapshot, tmp...)

			}

			/*Allocation Note: classUpdate.fsms was allocated by dealFSMs*/
			//fmt.Printf("Ranks from FailureClassUpdate were (before distribution update)")
			failureClasses[ind].Fsms = classUpdate.Fsms

			for i := range *classUpdate.Fsms {

				failureClasses[ind].FsmRanks[i] = (*classUpdate.Fsms)[i].GetRank()
				//fmt.Printf(" %v", failureClasses[ ind ].FsmRanks[ i ])

			}
			//fmt.Printf("\n")

			now = time.Now()
			//fmt.Printf("Class update appending to distribution at %v\n", now)
			failureClasses[ind].Dist.AppendIfNotPresent(&(failureClasses[ind].FsmRanks))
			now = time.Now()
			//fmt.Printf("Class update done appending to distribution at %v\n", now)

			/*
			   for i := range failureClasses[ ind ].FsmRanks{

			     if failureClasses[ ind ].StatsSnapshot[ i ] > 0{

			       go doDistributionUpdate( uint(i), (*failureClasses[ ind ].Fsms)[ i ].GetRank(),
			                             failureClasses[ ind ].StatsSnapshot[ i ],
			                             failureClasses[ ind ].Dist )

			     }

			   }*/

			/*Before doing anything, dealFSMs records the FSMs it has 
			  received for debugging and forensics purposes -- mine
			  and the devs of the software aviso is working for.*/
			f, _ := os.Create(".avisoFSMRecord")
			for i := range *failureClasses[ind].Fsms {
				fmt.Fprintf(f, "%+v\n", (*failureClasses[ind].Fsms)[i].ToString())
			}
			f.Close()

			now = time.Now()
			fmt.Printf("[AVISO] Done with class update for class %d at %v\n", classUpdate.ClassIndex, now)

		}

	}

}

/*The rpb Manager handles incoming messages from the endHandler server function.
  The rpb manager receives encoded event sequences representing RPBs from
  failing runs.  

  When the rpb manager receives a sequence, it decodes it 
  to an Events type, and stores it.  If it is the first event list received,
  the rpb manager uses it to generate FSMs.  FSMs are generated through an
  external call ("exec") to the FSM compiler.

  The rpb manager starts dealFSMs(), the go routine that facilitates doling
  out fsms to starting program instances.

  There are two channels that rpbManager uses to communicate:

  1)The rpb manager receives encoded event sequences from endHandler
    over rpbChan 
  2)The rpb manager receives shutdown messages over the done channel (created by main)

*/

func updateCorrectRunModel(modelUpdate *[]*avisoevent.Events, oldmodel *avisomodel.Model) *avisomodel.Model {

	var fnames []string
	for i := range *modelUpdate {

		if (*modelUpdate)[i] != nil {
			tmpFile, _ := ioutil.TempFile(".", "correct")

			for j := range *(*modelUpdate)[i] {

				fmt.Fprintf(tmpFile, "%s\n", avisoevent.EventToString((*(*modelUpdate)[i])[j]))

			}

			tmpFile.Close()
			fnames = append(fnames, tmpFile.Name())
		}

	}

	model := avisomodel.GenerateCorrectRunModel(fnames, oldmodel)

	//for i := range fnames {
		//os.Remove(fnames[i])
	//}

	return model

}

func updateRanksByCorrectModel(fsmlist *fsm.Fsmlist, model *avisomodel.Model, class uint) {

	for i := range *fsmlist {

		(*fsmlist)[i].SetRank((*model).RankFsm((*fsmlist)[i], class))

	}

}

func rpbManager() {

	/*A channel to communicate with dealFSMs*/
	var rpbDealChan chan avisomsgs.FailureClassUpdate = make(chan avisomsgs.FailureClassUpdate)

	/*Track the first 100 different failures*/
	maxNumFailures := 100
	var failureClasses []avisofailure.Failurelist = make([]avisofailure.Failurelist, maxNumFailures)

	/*Track 10 correct rpbs at a time before sending them to the file system as the
	  new correct run model*/
	maxCorrectRuns := 10
	curCorrectRun := 0
	var correctRuns []*avisoevent.Events = make([]*avisoevent.Events, maxCorrectRuns)
	var model *avisomodel.Model = new(avisomodel.Model)

	model = updateCorrectRunModel(&correctRuns, model)

	/*Go off an asynchronously dole out FSMs to starting instances*/
	go dealFSMs(rpbDealChan)

	for {

		/*Channel 1: got an rpb from endHandler on rpbChan*/
		rpbMsg := <-avisoglobal.RpbChan

		if rpbMsg.IsFailure {

			if maxNumFailures > 0 {

				/*If this is one of the first maxNumFailures failures...*/
				thisFailure := avisofailure.NewFailure(rpbMsg.Rpb)

				failureClass := avisofailure.FindFailureClass(&failureClasses, &thisFailure)

				failureClasses[failureClass] = append(failureClasses[failureClass], thisFailure)

				newf := failureClasses[failureClass].UniqueFsmList()

				model.AddFailureToModel(failureClass, thisFailure)

				updateRanksByCorrectModel(newf, model, failureClass)

				fcu := avisomsgs.FailureClassUpdate{ClassIndex: failureClass, Fsms: newf}

				rpbDealChan <- fcu

				maxNumFailures--

			} else {

				fmt.Printf("[AVISO] Got an RPB, but not collecting any more RPBs\n")

			}

		} else {

			/*rpbMsg.isFailure == false*/
			/*This is a correct run's RPB!*/
			fmt.Println("[AVISO] Updating the model with a correct run RPB")
			correctRuns[curCorrectRun] = rpbMsg.Rpb
			curCorrectRun++

			if curCorrectRun >= maxCorrectRuns {

				model = updateCorrectRunModel(&correctRuns, model)
				curCorrectRun = 0

			}
			fmt.Println("[AVISO] Done updating the model with a correct run RPB")

		}

	}

}

/*The stats manager arbitrates access to the number of failures
and successes, and the per-fsm number of failures and successes.

  The stats manager communicates on 4 different channels:
  1)With the main thread, via doneChan to shutdown and dump stats
  2)With the /stats http server handler to send stats
  3)With the /end http server handler to receive terminations
  4)With the fsm dealing thread, to send it failure rates to help
    decide which fsms are doing best

*/

func statsOutputString(baselineFailures uint, baselineSuccesses uint, statsByFailureClass *[]avisofailure.FailureClassStats) string {

	outstring := bytes.NewBufferString("")
	h := avisooutput.StatsHeaderString(baselineFailures, baselineSuccesses)
	fmt.Fprintf(outstring, "%s", h)
	for c := range *statsByFailureClass {
		if len((*statsByFailureClass)[c].FsmFailures) != 0 || len((*statsByFailureClass)[c].FsmSuccesses) != 0 {
			fmt.Fprintf(outstring, avisooutput.FailureClassStatsTableString(&((*statsByFailureClass)[c].FsmFailures), &((*statsByFailureClass)[c].FsmSuccesses), baselineFailures, baselineSuccesses))
		}
	}
	f := avisooutput.StatsFooterString()
	fmt.Fprintf(outstring, "%s", f)

	return string(outstring.Bytes())

}

func checkpointStatsManager(baselineFailures uint, baselineSuccesses uint, statsByFailureClass *[]avisofailure.FailureClassStats) {

	fmt.Println("Checkpointing stats manager\n")
	sf, err := os.Create("STATS")
	if err == nil {

		outstring := statsOutputString(baselineFailures, baselineSuccesses, statsByFailureClass)
		fmt.Fprintf(sf, "%s\n", outstring)
		sf.Close()

	}

}

func rateFromNums(fail uint, succ uint) float64 {
	return float64(fail) / float64(fail+succ)
}

func statsManager(doneChan chan bool) {

	/*TODO: statsManager needs to figure out which failure happened
	  based on the RPB in the Termination record.*/
	var totalruns uint = 0
	var anyFsmFailures uint = 0
	var anyFsmSuccesses uint = 0
	var baselineFailures uint = 0
	var baselineSuccesses uint = 0

	plot, _ := os.Create(avisoglobal.PlotFile)

	var chk int = 0

	var statsByFailureClass []avisofailure.FailureClassStats = make([]avisofailure.FailureClassStats, 100)

	for {

		select {

		/*Channel 2: Is there a stats request coming in? Send the stats*/
		case <-avisoglobal.StatsOutputChan:

			outstring := statsOutputString(baselineFailures, baselineSuccesses, &statsByFailureClass)
			avisoglobal.StatsOutputChan <- outstring

		/*Channel 3: Is there a termination coming in? Update the stats*/
		case t := <-avisoglobal.StatsChan:

			now := time.Now()
			fmt.Printf("[AVISO] Received a Termination for PID %d at %v\n", t.Pid, now)

			totalruns++

			/*Get a Termination*/
			if t.Fsm == "baseline" || t.Fsm == "" {
				if t.Fail {
					baselineFailures++
				} else {
					baselineSuccesses++
				}

				fmt.Fprintf(plot, "%v %v %v %v %v\n", totalruns,
                                                                      baselineFailures,
                                                                      baselineSuccesses,
                                                                      anyFsmFailures,
                                                                      anyFsmSuccesses)
				continue

			}

			if t.Fail {
				anyFsmFailures++
			} else {
				anyFsmSuccesses++
			}
			fmt.Fprintf(plot, "%v %v %v %v %v\n", totalruns,
                                                              baselineFailures,
                                                              baselineSuccesses,
                                                              anyFsmFailures,
                                                              anyFsmSuccesses)

			activeFsms := strings.Split(t.Fsm, ";")

			if t.Fsm != "" {

				for fs := range activeFsms {

					if activeFsms[fs] == "" {
						continue
					}
					thisfsm := fsm.NewFsmTag(activeFsms[fs])
					if statsByFailureClass[thisfsm.Class()].FsmFailures == nil {
						statsByFailureClass[thisfsm.Class()].FsmFailures = make(map[uint]uint)
						statsByFailureClass[thisfsm.Class()].FsmSuccesses = make(map[uint]uint)

					}

					/*The termination record included a fsm name*/
					if _, ok := statsByFailureClass[thisfsm.Class()].FsmFailures[thisfsm.Fsm()]; !ok {
						statsByFailureClass[thisfsm.Class()].FsmFailures[thisfsm.Fsm()] = 0
						statsByFailureClass[thisfsm.Class()].FsmSuccesses[thisfsm.Fsm()] = 0
					}

					if t.Fail {
						statsByFailureClass[thisfsm.Class()].FsmFailures[thisfsm.Fsm()]++
					} else {
						statsByFailureClass[thisfsm.Class()].FsmSuccesses[thisfsm.Fsm()]++
					}

					var statsUpdate avisomsgs.StatsUpdate = avisomsgs.StatsUpdate{}
					statsUpdate.Fsm = fsm.FsmTagCtr(thisfsm.Class(), thisfsm.Fsm())
					statsUpdate.Successes = statsByFailureClass[thisfsm.Class()].FsmSuccesses[thisfsm.Fsm()]
					statsUpdate.Failures = statsByFailureClass[thisfsm.Class()].FsmFailures[thisfsm.Fsm()]

					sig := avisostats.ComputeChiSquare(float64(statsUpdate.Failures),
                                                                           float64(statsUpdate.Successes),
                                                                           float64(baselineFailures),
                                                                           float64(baselineSuccesses))

					if sig > 3.85 && rateFromNums(statsUpdate.Failures, statsUpdate.Successes) <
                                                         rateFromNums(baselineFailures, baselineSuccesses) {

						statsUpdate.ChiSquare = sig

					} else {

						statsUpdate.ChiSquare = -1

					}

					statsUpdate.BaselineFail = baselineFailures
					statsUpdate.BaselineSucc = baselineSuccesses

					avisoglobal.DealStatsChan <- statsUpdate

				}

			}

			/*Be paranoid about server crashes and checkpoint every 3 terminations*/
			chk++
			if chk > 3 {

				checkpointStatsManager(baselineFailures, baselineSuccesses, &statsByFailureClass)
				chk = 0

			}

		} /*End Select*/

	}

}

/*------------HTTP Event Handling Routines---------------*/
func statsHandler(w http.ResponseWriter, r *http.Request) {

	defer r.Body.Close()
	//fmt.Println("Dumping Stats!\n")
	avisoglobal.StatsOutputChan <- ""
	s := <-avisoglobal.StatsOutputChan
	fmt.Fprintf(w, "%s\n", s)

}

func endHandler(w http.ResponseWriter, r *http.Request) {

	defer r.Body.Close()
	if err := r.ParseForm(); err != nil {
		fmt.Fprintf(w, "PARSE FORM FAILED", html.EscapeString(r.URL.Path))
	}

	var s = r.Form.Get("fsm")
	var fs = r.Form.Get("fail")
	var pid = r.Form.Get("pid")
	var rpb = r.Form.Get("rpb")

	var t avisomsgs.Termination = *(new(avisomsgs.Termination))

	events, err := avisoevent.RpbStringToEvents(rpb)

	if fs == "1" {

		t.Fail = true
		if err == nil {
			avisoglobal.RpbChan <- avisomsgs.RpbUpdate{Rpb: &events, IsFailure: true}
		} /*else events didn't parse from json correctly...*/

	} else {

		t.Fail = false
		if err == nil {
			avisoglobal.RpbChan <- avisomsgs.RpbUpdate{Rpb: &events, IsFailure: false}
		} /*else events didn't parse from json correctly...*/

	}

	t.Fsm = strings.Replace(s, "\n", "", -1)
	ipid, _ := strconv.Atoi(pid)
	t.Pid = uint(ipid)

	avisoglobal.StatsChan <- t
	fmt.Printf("[AVISO] Registered End Event for pid %v (%v)\n", t.Pid, t.Fail)
	/*[AVISO] Registered End Event for pid 11407 (false)
	[AVISO] Updating the model with a correct run RPB
	[AVISO] Done updating the model with a correct run RPB
	[AVISO] Received a Termination for PID 11407 at 2012-07-03 00:18:20.032458 -0700 PDT
	Couldn't read from fsm dealer!  Continuing fsmless

	[AVISO] Updating the model with a correct run RPB
	[AVISO] Done updating the model with a correct run RPB
	*/

}

func correctHandler(w http.ResponseWriter, r *http.Request) {

	if err := r.ParseForm(); err != nil {
		fmt.Fprintf(w, "PARSE FORM FAILED", html.EscapeString(r.URL.Path))
	}

	defer r.Body.Close()

	rpb, _ := ioutil.ReadAll(r.Body)

	events, err := avisoevent.RpbStringToEvents(string(rpb))

	if err == nil {

		avisoglobal.RpbChan <- avisomsgs.RpbUpdate{Rpb: &events, IsFailure: false}

	} else {

		log.Printf("Couldn't Parse JSON: %v\n", events)

	}

}

func tickHandler(w http.ResponseWriter, r *http.Request) {

	if err := r.ParseForm(); err != nil {
		fmt.Fprintf(w, "PARSE FORM FAILED", html.EscapeString(r.URL.Path))
	}
	defer r.Body.Close()

	var t avisomsgs.Termination = avisomsgs.Termination{}
	var s = r.Form.Get("fsm")
	var pid = r.Form.Get("pid")
	ipid, _ := strconv.Atoi(pid)

	t.Pid = uint(ipid)
	t.Fail = false
	t.Rpb = nil
	t.Fsm = strings.Replace(s, "\n", "", -1)
	fmt.Println("Logical time tick for fsm=", t.Fsm, " pid=", t.Pid)

	avisoglobal.StatsChan <- t

}

func channelTimeout(c chan bool) {

	time.Sleep(time.Second)
	c <- true

}

func startHandler(w http.ResponseWriter, r *http.Request) {

	defer r.Body.Close()
	var c chan bool = make(chan bool)
	go channelTimeout(c)
	select {
	case <-c:
		fmt.Println("Couldn't read from fsm dealer!  Continuing fsmless\n")
		return
	case fsm := <-avisoglobal.FsmChan:
		fmt.Fprintf(w, "%s\n", fsm)
	}
}

/*------------END HTTP Event Handling Routines---------------*/

func init() {

	http.HandleFunc("/start", startHandler)
	http.HandleFunc("/end", endHandler)
	http.HandleFunc("/stats", statsHandler)
	http.HandleFunc("/tick", tickHandler)
	http.HandleFunc("/correct", correctHandler)

}

func main() {

	var correctModelFlag = flag.String("model", "correct.histo", "Specify the path to the correct run model")
	var fsmGenFlag = flag.String("fsmgen", "../Scripts/generate_fsms.sh", "Specify the path to the fsm generator")
	var corrGenFlag = flag.String("corrgen", "../Scripts/generate_correct_model.sh", "Specify the path to the correct model generator")
	var ratePlotFlag = flag.String("rateplot", "current.plots", "Specify where Aviso should dump the plots of failures vs. runs")
	var baselineOnlyFlag = flag.String("baselineonly", "0", "True if Aviso should never send any FSM (for experiments)")

	flag.Parse()
	avisoglobal.FsmGen = *fsmGenFlag
	avisoglobal.CorrGen = *corrGenFlag
	avisoglobal.CorrectModel = *correctModelFlag
	avisoglobal.PlotFile = *ratePlotFlag
	avisoglobal.BaselineOnly = (*baselineOnlyFlag) == "1"

	var done chan bool = make(chan bool)

	go rpbManager()
	go statsManager(done)

	fmt.Println("[AVISOSERVER Version 0 - Brandon Lucia - 2012]")

	log.Fatal(http.ListenAndServe(":22221", nil))

	fmt.Println("[AVISOSERVER Shutting Down!]")

}
