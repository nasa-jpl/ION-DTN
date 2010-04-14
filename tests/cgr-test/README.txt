This test exercises CGR in a fairly large and complex contact plan, to
verify the effectiveness of optimization strategies.  The topology is as
follows.

There are 9 nodes in the notional network (of which we only instantiate one,
node #9, for the purposes of this test):

- Two Mars rovers: ExoMars (node 8) and MAX-C (node 9).

- Three Mars orbiters: MAVEN (node 5), MRO (node 6), TGO (node 7).

- Three DSN stations: Goldstone (node 2), Madrid (node 3), Canberra (node 4).

- One Mission Control Center, node 1.

All rover/orbiter and orbiter/DSN contacts are intermittent and periodic; all
DSN/MCC contacts are continuous.  Each cycle of network operations comprises
14 contact intervals and lasts two hours; the entire contact plan covers
48 hours of operations, 336 contact intervals.  See global.ionrc for details.

One-way light time from Mars to Earth is assumed to be 324 seconds.

The test consists of using bptrace to source a single small bundle at node 9,
destined for the MCC at node 1.  The bundle's lifetime is 200000 seconds, so
CGR needs to consider all contacts in the contact graph in order to select
the best next-hop node to forward the bundle to.  The bundle never actually
goes anywhere; all we're exercising is the route computation logic.
