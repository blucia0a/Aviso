Aviso
=====

=Overview=
A distributed system implementation of the Aviso algorithm for dynamically avoiding failures due to concurrency bugs

Aviso is a system that avoids failures in shared memory multithreaded programs.  Aviso avoids failures that are
the result of concurrency bugs.  Aviso works by strategically perturbing the order of events in different threads
of a multithreaded program while it executes.  Aviso works because some bugs only show up as failures when
operations in different threads occur in a particular order.  Aviso automatically identifies operation orders likely
to be responsible for a failure observed in some execution.  In subsequent executions, Aviso perturbs the execution schedule
to reorder those operations.  When the operations are reordered, the bug does not cause a failure.

Aviso programs are compiled with an instrumenting compiler pass.  That compiler pass inserts calls into the Aviso runtime 
at some points in the program, called events.  Events are synchronization (pthreads) operations and some shared memory operations.
By way of these event calls, the runtime keeps a history of the events that have executed during an execution.  A 
history maintained by the runtime is called a Recent Past Buffer, or RPB.  A program compiled with the Aviso compiler and
linked to the Aviso runtime is called "Aviso Enabled".  

A user runs an Aviso Enabled program using the Aviso "runner", an execution harness.  The purpose of the execution harness
is to track when the program being run crashes and it otherwise does not alter the program's behavior.  When the program
crashes, the runtime preserves the RPB that contains a list of events that executed right before the crash occurred.  The
execution harness picks up the preserved RPB and sends it off to the Aviso Server.

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




To summarize the mechanism:

Runtime runs program.  Crash happens.  Runtime sends RPBs to server.  Server collects RPBs.  Server builds FSMs
from RPBs.  Server builds statistical model from RPBs.  Server uses model to decide which FSMs runtime should load.
Runtime loads RPBs recommended by Server.  Runtime reports to server which FSMs are loaded when failures occur.


=Further Reading=
The algorithms implemented in this system were first described in a "Cooperative Empirical Failure Avoidance for 
Multithreaded Programs" (http://dl.acm.org/citation.cfm?id=2451121) and later in Chapter 6 of Brandon Lucia's PhD
dissertation (https://digital.lib.washington.edu/dspace/bitstream/handle/1773/23483/Lucia_washington_0250E_11953.pdf?sequence=1)
