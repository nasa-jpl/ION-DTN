# High-speed CRC remainder regression

Build and install ION with `--enable-high-speed`, then run its registered test:

```bash
make buildcheck
cd tests
./runtests crc32-high-speed
```

Checks CRC32C and CRC32 against independent bitwise calculations for every
length from 0 through 1024, including 64-byte boundaries and nonzero remainders.
Offsets 0 through 3 cover unaligned inputs on platforms supporting the existing
slice implementation. Each input is also split into two incremental calls.
Known `123456789` vectors check the CRC conventions.

The test is included in `normaltests`, so `make check` also runs it. Builds
without `--enable-high-speed` compile a skip executable that returns status 2.
It does not start an ION node. `make install` and an installed `ionadmin` on
PATH are required by the standard test runner.

Minimal reproduction against an unpatched source tree, without a running node:

```bash
cc -O2 -std=c11 -DENABLE_HIGH_SPEED -Iici/include ici/library/crc.c tests/crc32-high-speed/crc32_regression.c -o /tmp/crc32-regression
/tmp/crc32-regression
```

The original slice routine fails at length 65. The fixed routine passes all
16400 whole/incremental checks plus known vectors.
