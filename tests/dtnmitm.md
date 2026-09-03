# dtnmitm

A UDP man-in-the-middle relay for ION's regression tests.
It sits between two ION link services,
dissects each datagram as an LTP segment
and, where a data segment carries one, as the BPv7 bundle inside it,
and decides what becomes of it.

It exists because a lossless loopback link
never exercises the paths that matter most:
checkpoint retransmission, report loss,
the reordering of a report against the data that supersedes it,
and a receiver's response to a duplicated or altered bundle.

This is test tooling.
It is importable by anything, but it is not part of ION's interface
and carries no compatibility promise.

## Requirements

Python 3.10 or later, and `bespokebpv7`,
which does all the parsing and serialization.
A test that imports this module should check for both
and exit 2 to skip when either is absent, rather than fail:
`ltp-superseded-report/dotest` shows the pattern,
including the check for a specific class
when the version installed may be too old to carry it.

## Looking up a call

Every public name carries a docstring,
so the module itself is the reference:

    python3 -m pydoc tests/dtnmitm.py     # the whole module
    cd tests && python3 -m pydoc dtnmitm.Mitm

Read this file for the shape of things;
read pydoc for the detail of one call.

## The model

A datagram arrives and becomes a `View`:
the raw bytes, plus whatever could be parsed from them.
Each rule is offered the view in turn,
and the first to return a `Verdict` settles the matter --
later rules do not run.
A datagram no rule claims is forwarded unchanged.
Either way it is recorded,
so a trace is a complete account of one direction,
not only of what was interfered with.

A verdict may pass, drop, delay or rewrite a datagram,
and may carry extra datagrams of its own,
which is how a test emits a segment that was never sent.

One relay carries one direction.
A test that must see reports as well as data segments needs two.

## A worked example

Hold every report that describes gaps,
letting data through untouched,
and write down everything either way:

    import dtnmitm as M

    reports = M.Mitm(
        6001, 1113,
        rules=[M.rule(M.report_gapped(),
                      M.Verdict(act=M.Act.DROP, note="gapped"))],
        trace_path="mitm-report.jsonl",
        label="report",
    )

    with reports:
        ...  # run the test; the relay works in background threads

    for record in M.read_trace("mitm-report.jsonl"):
        assert record["act"] in ("pass", "drop")

`rule()` also takes `nth=`,
which restricts it to chosen occurrences among the datagrams it matches,
counted from one.
That is how a test names "the second red data segment"
without knowing anything about the traffic around it.

## The reference

Matching a datagram:

    seg_type(*types)      segments of any of the given types
    report_gapped()       a report whose claims leave gaps in its scope
    report_complete()     a report claiming its whole scope
    field_is(name, value) a segment field equal to a value
    all_of(*matchers)     every given matcher at once

`RED_DATA` names every red data segment type,
for `seg_type(*RED_DATA)`.

Deciding what to do:

    rule(matcher, verdict, *, nth=None)   build a rule
    Verdict(act, delay_ms, payload, extra, note)
    Act.PASS / DROP / DELAY / REWRITE

`verdict` may be a `Verdict`
or a callable taking the view and returning one,
which is what a rule that rewrites bytes needs.

Reading a datagram, in a matcher or a verdict callable:

    view.raw            the bytes as received
    view.segment        the typed LTP segment, or None
    view.segment_type   its type, or None
    view.bundle         the BPv7 bundle it carries, or None
    view.attr(name)     one segment field, whatever the segment type
    view.summary()      the fields the trace records

Running the thing:

    Mitm(listen_port, forward_port, *, rules, trace_path, host, label)
    .start() / .stop()  or use it as a context manager
    .inject(payload, delay_ms=0)   emit a datagram of your own making
    .counts             tally by act name, plus injections

    read_trace(path)    the trace, one dict per datagram, in order

## The trace

One JSON object per line, one line per datagram.
Every record carries `t`, `relay` and `act`;
`delay_ms` and `note` appear when they were set,
and the fields of `View.summary()` --
`len`, `seg_type`, session and serial numbers,
`claims` and `report_complete` for a report,
`bundle` with its endpoints for a data segment that carries one --
appear as far as the datagram parsed.
A datagram the dissector rejected is recorded with `parsed` false,
and little else.
Note that parsing is not validation:
LTP carries no magic number,
so bytes that are not a segment at all
can still yield a plausible-looking one with nonsense in its fields.
Assert on the fields a test actually cares about,
not on the mere fact that something parsed.

Assert against the trace rather than scraping daemon logs.

## Notes

Give link services `127.0.0.1` addresses in the `ltprc`.
Everything here is IPv4 and explicit.
ION's `udplsi` binds the IPv6 loopback
when its endpoint is given as `localhost`,
and a relay listening on IPv4 then receives nothing at all, silently.

A datagram that will not parse is relayed, not dropped:
the dissector swallows every exception,
because a test relay that dies on one malformed datagram
takes the test with it.
The cost is that a parsing bug looks like a quiet `parsed: false`
rather than a traceback.
