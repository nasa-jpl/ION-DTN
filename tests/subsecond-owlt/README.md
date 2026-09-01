# Subsecond OWLT regression fixture

This fixture validates the first compatibility milestone of the subsecond
OWLT fork.  `ionadmin` must accept `0.006` seconds, store it as a whole-second
and millisecond pair, print it as `0.006 seconds`, and export the same value to
`ranges.ionrc`.

This fork changes persistent ION range structures.  Run it only with a newly
initialized SDR database; an SDR database created by stock ION 4.2.0-b is not
binary compatible.

Run both the storage/export and CGR route-selection cases through the normal
test harness:

```sh
cd tests
./runtests subsecond-owlt
```

For direct execution against an in-tree build:

```sh
ION_TEST_BUILD_DIR=/absolute/path/to/ion-build tests/subsecond-owlt/dotest
```
