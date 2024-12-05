# Running the ION test set

## Directory layout

The `tests` directory under ION's root folder contains the test suite. Each test lives in its own subdirectory of this directory. Each test is conducted by a script `$TESTNAME/dotest`. Another directory that contains ION tests is the `demos` directory, which includes examples of ION configurations using different convergence layers. For this document, we focus on the usage of the `tests` directory.

## Exclude files

Exclude files are hidden files that allow for tests to be disabled based on certain conditions that may cause the test not to run correctly. If an exclude file exists, it should have a short message about why the test has been excluded.

Exclude files can exist in any of the following formats:

- `.exclude_OS-TYPE`: Disables a test for an operating system that it does not run successfully on. Acceptable values to fill in for OS-TYPE are "windows", "linux", "mac", and "solaris".

- `.exclude_BP-VERSION`: Disables a test for a version of the bundle protocol that it does not run correctly or does not make sense with. As of ION 4.0.0, the acceptable values to fill in for `BP-VERSION` are "bpv6" and "bpv7".

- `.exclude_all`: Disables a test for all platforms.

- `.exclude_expert`: Disables a test because of additional utilities that are required for the test. To work around this exclusion if you want to run an expert test, you can set `ION_RUN_EXPERT="yes"` in your shell environment to enable all ION tests classified as expert.

- `.exclude_cbased`: Disables a test that relies on compiling a C program to generate the dotest executable script. To exclude C-based tests, you need to define the environment variable `ION_EXCLUDE_CBASED`.

## Running the tests

The tests are run by running `make test-all` in the top-level directory, or by running `runtests` in this directory.

An individual test can also be run: `./runtests <test_name>`

A file defining a set of tests can be run with `runtestset`.  The arguments to `runtestset` are files that contain globs of tests to run, for example: `./runtestset quicktests`.

## Writing new tests

A test directory must contain an executable file named `dotest`.  If a directory does not contain this, the test will be ignored. The `dotest` program should execute the test, possibly reporting runtime information on stdout and stderr, and indicate by its return value the result of the test as follows:

    0: Success
    1: Failure
    2: Skip this test

The test program starts without the ION stack running.  The test program is responsible for starting ION in the ways that is appropriate for the test.

The test program *must* stop the ION protocol stack before returning.

## The test environment

The `dotest` scripts are run in their test directory. The following environment variables are set as part of the test environment:

- `IONDIR` is the root of the local ION source directory.

- `PATH` begins with `IONDIR` (this is where the local executables are found.)

## For 4.1.3 and later

The `runtests` script maintains a file called `tests/progress` that gives the start time, finish time, and final result for each test.

If the environment variable `RUNTESTS_OUTPUTDIR` is set, as in, `export RUNTESTS_OUTPUTDIR="/tmp"`, then the output from each test will be stored in `/tmp/results`, which makes it much easier to find particular text or results when debugging.