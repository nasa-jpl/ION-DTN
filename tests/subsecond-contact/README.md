# Subsecond contact regression test

This test validates millisecond contact parsing, persistence, export,
revise/delete, live `rfxclock` dispatch, CGR boundary selection, fractional
`clockerr`, signed `utcdelta`, BP DTN time, and legacy whole-second behavior.

Run it through the normal test harness:

```sh
cd tests
./runtests subsecond-contact
```

For direct execution against an in-tree build:

```sh
ION_TEST_BUILD_DIR=/absolute/path/to/ion-build \
  tests/subsecond-contact/dotest
```

The live subsecond cases explicitly select a 10 ms timeline poll. The legacy
case uses the default 1000 ms poll. Timing results are functional regression
observations, not latency guarantees. All cases use fresh temporary SDR/node
databases because the subsecond persistent layouts are not binary compatible
with stock databases.
