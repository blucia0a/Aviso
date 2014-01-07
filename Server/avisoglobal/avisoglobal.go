package avisoglobal

import (
        "../avisomsgs"
       )

/*Channels that the callbacks use to talk to various coroutines*/
/*The stats manager communicates on 4 different channels:*/

/*  1)With the /stats http server handler to send stats on statsOutputChan*/
var StatsOutputChan = make(chan string)
/*  statsOutputChan is by the stats manager*/

/*  2)With the /end http server handler to receive terminations on statsChan*/
var StatsChan = make(chan avisomsgs.Termination)
/*  statsChan is by the stats manager*/

/*  3)With the fsm dealing thread, to send it failure rates to help decide which fsms are doing best*/
var DealStatsChan = make(chan avisomsgs.StatsUpdate)
/*  dealStatsChan is owned by the stats manager*/


/* There are two channels that rpbManager uses to communicate:*/

/*  1)The rpb manager receives encoded event sequences from endHandler
      over rpbChan */
var RpbChan = make(chan avisomsgs.RpbUpdate)
/*rpbChan is owned by the rpb manager*/


/*dealFSMs() communicates over 2 channels:*/

/*  1)dealFSMs sends fsms out on fsmChan.  fsmChan is read by
    the server startHandler*/
var FsmChan = make(chan string)

/*  2)dealFSMs sends requests to the stats manager via dealStatsChan
    and receives stat updates from the stats manager over dealStatsChan */


/*End Channels that the callbacks use to talk to various coroutines*/


/*These global variables get set by main when it processes flags*/
var CorrectModelFile = ""
var ServerPortString = ""

