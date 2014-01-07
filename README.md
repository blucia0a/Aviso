Aviso
=====

Overview
--------
A distributed system implementation of the Aviso algorithm for dynamically avoiding failures due to concurrency bugs

Aviso is a system that avoids failures in shared memory multithreaded programs.  Aviso avoids failures that are
the result of concurrency bugs.  Aviso works by strategically perturbing the order of events in different threads
of a multithreaded program while it executes.  Aviso works because some bugs only show up as failures when
operations in different threads occur in a particular order.  Aviso automatically identifies operation orders likely
to be responsible for a failure observed in some execution.  In subsequent executions, Aviso perturbs the execution schedule
to reorder those operations.  When the operations are reordered, the bug does not cause a failure.

Aviso Profiler, Compiler, and Runtime
--------------------------
Aviso programs are compiled with an instrumenting compiler pass.  That compiler
pass inserts calls into the Aviso runtime at some points in the program, called
events.  Events are synchronization (pthreads) operations and some shared
memory operations that are discovered by the Aviso Profiler.  By way of these
inserted event calls, the runtime keeps a history of the events that have executed
during an execution.  A history maintained by the runtime is called a Recent
Past Buffer, or RPB.  A program compiled with the Aviso compiler and linked to
the Aviso runtime is called "Aviso Enabled".  

Aviso Runner
------------
A user runs an Aviso Enabled program using the Aviso "runner", an execution harness.  The purpose of the execution harness
is to track when the program being run crashes and it otherwise does not alter the program's behavior.  When the program
crashes, the runtime preserves the RPB that contains a list of events that executed right before the crash occurred.  The
execution harness picks up the preserved RPB and sends it off to the Aviso Server.

Aviso Server
------------
The Aviso server is a program that can run on any computer accessible via http.  When the server receives an RPB from a
failed execution from the execution harness, it analyzes it for several reasons.  The first thing the server does is
to generate small program plugins called Failure-avoidance State Machines (FSMs).  The server generates an FSM for each
unique ordered pair of events in the RPB.  An FSM is a dynamically loadable shared library that can be loaded by the
runtime.  An FSM contains code that tries to reorder the pair of events from which it was generated.   If the runtime
loads an FSM that reorders a pair of events that, if not reordered, would cause a failure, then that FSM successfully
avoids that failure.  

The second thing the server does is to build a statistical model that helps decide which pair of events in the RPB
is likely to have caused the failure.  If the server can decide that, then it recommends to the runtime that the FSM
corresponding to that pair of events be loaded.  The server maintains a statistical model of pairs of events that
helps determine which pairs' FSMs are worth loading.  The statistical model tracks properties of RPBs from failed 
executions as well as properties of RPBs from non-failing portions of executions.  RPBs from non-failing portions
of executions are periodically sampled by the runtime and sent via http to the server.  The details of the model
are omitted here.  See the Further Reading section below for two references that describe the model in detail (with
pictures).  A key part of the model is that the server tries to decide which FSMs were observed to have been loaded
in failing and non-failing executions.  If an FSM is loaded in a failing run, its pair isn't likely to have caused the
failure -- if it had, the FSM would've prevented it.  IF an FSM is loaded in a non-failing run and its pair caused
the failure, then the FSM prevented it.  The server also considers analytical properties of RPBs from failing and
non-failing executions.  Combining that with direct observation of FSM failure rates helps decide which FSMs to use.


Summary of Mechanism
---------------------

Compiler instruments program with runtime calls.  Runtime calls run while program executes.  Crash happens.  
Runtime sends RPBs to server.  Server collects RPBs.  Server builds FSMs
from RPBs.  Server builds statistical model from RPBs.  Server uses model to decide which FSMs runtime should load.
Runtime loads RPBs recommended by Server.  Runtime reports to server which FSMs are loaded when failures occur.


Building
--------
This project uses cmake.  Install cmake first.  Then, in the Aviso root
directory, run "cmake .".  That will generate makefiles in the AvisoCompiler,
Runtime, and AvisoProf directories.  Then run make.  That will build the
project.  

The event profiler requires a modern version of Intel's Pin binary
instrumentation framework.  You can get one at http://pintool.org.  You need to
be sure "PIN\_ROOT" is set in your environment and pointing to the root of the
Pin installation before you run cmake.

The Aviso Server and Runner require Go (http://golang.org).  The server uses
some modern features of Go that require version 1.1 or greater.  On Ubuntu,
that sometimes means adding the PPA that points to the newest release version.
Googling that produces results that will help you get that set up.


Things the Programmer Does
==========================
There are some steps during the use of Aviso that the programmer does to
profile, compile, and link their program.  These steps are "one time" things
that the users of the software being Aviso-enabled do not need to think about.

Running the Event Profiler
--------------------------
The event profiler is a little klugy, but this overview will help you to run it
and massage the results into a form usable with the Aviso compiler.  When you
run 'make' in the top level, you will produce AvisoProf/ap.so.  ap.so is a
"pintool" that implements the Aviso event profiler.  You will run your program
inside the profiler using a representative test case.  After you have done
that, the profiler will report a list of points in the code that are events. To
run the profiler, you should run:

'pin -t path/to/ap.so -- path/to/your/program your programs args'

If everything went correctly during the build, this should produce a bunch of 
output on stderr.  You can put that in a file.  Each line shows a path 
to a source file and a line number.  Here's the klugy part that I am planning
to fix when I have time...  You need to run 'basename' on each line of that 
output and replace the colons that separate the filenames from the line numbers
with spaces, instead.  If you write a script to do this, feel free to submit it
and I'll definitely add it to the project.  That file that looks like


foo.c 94
frampton.cpp 915
cornbluth.c 875

is your AVISOPOINTS file.  When you compile your program, you should specify
AVISOPOINTS=/that/file in your environment.  The compiler pass will be looking
for it.

Using the Compiler and Linking
------------------------------
The compiler pass reads in the AVISOPOINTS file and treats the file/line pairs
in it, as well as pthread functions, as events.  At each event the compiler
inserts a call to a function in the Aviso Runtime.  There are a bunch of
compiler flags that need to be set to be sure that nothing breaks.  These are
specified for you in configure.aviso, which lists an autoconf "configure"
command line that you can use with Aviso.

First, you need to specify -fno-omit-frame-pointer or backtrace collection
breaks and Aviso is useless.  Second, you need to keep debug symbols around.
Third, you need to compile with -finstrument-functions.  That will add a call
to a special Aviso runtime function that correctly tracks backtraces (unlike
the builtins...).  Fourth, you need to specify
-fplugin=/path/to/AvisoCompiler/libaviso.so so that gcc finds the compiler
plugin.  Fifth, you need to redefine a few functions that should be treated as
events by Aviso -- these are listed in the configure.aviso file (e.g.,
-Dpthread\_mutex\_lock=IR\_Lock).  The reason for these redefinitions is that
the runtime defines its own version of those functions and they need to be
treated as Aviso events.  You can add other functions by simply preceding calls
to those functions with a call to AVISO\_Synthetic\_Event(). Sixth, you need to 
link to the Aviso runtime by specifying -L/path/to/Aviso/Runtime -lIRPTR on
the commandline.  And with that, you will build an Aviso-enabled version of 
your program.  

Things the User Does
====================


Running the Server
------------------
The next step after producing an Aviso-enabled version of the program is to run
the Server.  The Server runs on a machine that the users of the program will
have access to.  That can be a developer machine, reachable over the internet,
or it can be the same host that the program runs on.  You will run a server per
program -- not per instance, but rather per program.  So if you're running
Apache httpd 2.4.2 in lots of places, you can run just one server and all
instances Apache httpd 2.4.2 can point to that server.  You configure the 
server via the command line when you run it.  There are two main things to
configure.  The first thing to configure is the "model" of your program
that is maintained by the server.  For now, this is a database stored in
a plaintext file and you can specify its path to the server on the command
line.  The second thing to configure is the port that the server should
run on.  The server defaults to using 22221 and you can change that via
the command line.  If the defaults are OK for you, you can run

'go run AvisoServer.go'

The server will run in the foreground by default.  If you want it to run
in the background, use 

'nohup go run AvisoServer.go > server.out 2> server.err < /dev/null &'


Running Programs with the Runner
---------------------------------
Now: the exciting part! With the server running, the very last step is to run
the program using the Aviso Runner.  The Aviso Runner does not molest your
program's execution at all.  It simply monitors for failures, collects RPBs,
and handles most interactions with the server [note: the runtime also handles
some interactions with the server].  

To run the Runner, you should use

'go run Runner.go -prog=/path/to/your/program -args="your programs args"'

There are a few other important options to the Runner that, if you do not set
correctly, will prevent Aviso from working.  First there is the RPB Path.  This
is the path to a scratch directory used by the Aviso runner to temporarily
store RPBs before sending them to the server.  It is OK if this is on /tmp.
Second There is the FSM Compiler.  This is the path to a script in the Aviso
tree.  You should not need to set this unless you have changed the layout of
the Aviso source tree.  Lastly there is the FSM directory.  This is the path to
a directory that will store compiled Aviso Runtime Plugins that manipulate the
execution schedule (these are the FSMs described above).  The default directory
is the ".fsms", which is a hidden directory in the working directory when the
Runner is run.

Now Everything Works! (...)
---------------------------
If you set all those things and run your program, it should send a "Start"
message to the server.  Then, it should run.  The Aviso runtime may send 1 or
more "Correct Run RPBs" to the Aviso Server.  Don't be surprised by that.  If
your program terminates uneventfully (i.e., correctly) Aviso will send the
Server a Correct Termination event.  If your program crashes, the Runner should
detect that if it is a "signal 6", "signal 11", "segmentation fault", "abort",
or "assertion failure".  When that happens, it will send the RPB that the
runtime stored in the RPB path of the Runner and send it to the server as a
Failure Termination message.  After some failures, the Server may start sending
the Runner some FSM Templates when you start the program.  The Runner will use the
FSM Compiler to build FSMs and then load them into the Runtime.  FSMs built before
are cached in the FSM Dir.  At startup the server instructs the Runner to use an 
FSM by name.  If it is already built, it will use the built version.  If not, it
will build and cache the FSM.

Watching Executions Unfold
--------------------------
If you're using Aviso, you might be intersted in whether or not you're seeing
failures, whether or not your program is using any FSMs, and how effective each
is.  You can monitor Aviso through a web interface that the Server runs.  Go to
http://your server URL:port/stats to see how things are coming along.  You'll
see the baseline failure rate (i.e., the rate of failures with no FSMs), a
failure rate for each used FSM, and a confidence measure that shows how likely
it is that the FSM has a different failure rate than the baseline.  When Aviso
finds an effective FSM, you'll see the "Contenders" section populated with FSMs
that are statistically significantly effective at reducing the failure rate
relative to the baseline rate.

Further Reading
===============
The algorithms implemented in this system were first described in a "Cooperative Empirical Failure Avoidance for 
Multithreaded Programs" (http://dl.acm.org/citation.cfm?id=2451121) and later in Chapter 7 of Brandon Lucia's PhD
dissertation (https://digital.lib.washington.edu/dspace/bitstream/handle/1773/23483/Lucia_washington_0250E_11953.pdf?sequence=1)
