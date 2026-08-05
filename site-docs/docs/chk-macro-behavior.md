# CHK Macro Behavior and Fail-Fast Mode

When a CHK macro's condition evaluates to false,
the following actions occur:

1. An error message is posted
   with the file name, line number,
   and the failed assertion expression
2. All error memos are written to the log via `writeErrmsgMemos()`
3. A stack trace is printed via `printStackTrace()`
   (on Linux and Solaris platforms)
4. If fail-fast mode is enabled (`CORE_FILE_NEEDED=1`),
   `sm_Abort()` is called to terminate immediately with a core dump
5. Otherwise, the function returns
   the appropriate error value (-1, 0, NULL, or void)

**Fail-Fast Mode (CORE_FILE_NEEDED)**

The `CORE_FILE_NEEDED` parameter controls
whether assertion failures cause immediate process termination:

- **Default value is 1 (enabled)**:
  Assertion failures will cause immediate termination with a core dump,
  providing maximum debugging information
- **To disable at compile time**:
  Use `-DCORE_FILE_NEEDED=0` when compiling
- **To control at runtime**:
  Call `_coreFileNeeded(int *ctrl)`
  with a pointer to 0 (disable) or 1 (enable)

```c
/* Disable fail-fast mode at runtime */
int off = 0;
oK(_coreFileNeeded(&off));

/* Re-enable fail-fast mode */
int on = 1;
oK(_coreFileNeeded(&on));
```

**Stack Trace Support**

The `printStackTrace()` function prints a symbolic stack trace
when assertions fail.
This is supported on:

- **Linux**: Requires `HAVE_EXECINFO_H` to be defined
  and linking with `-rdynamic`
- **Solaris**: Uses `printstack()` from `<ucontext.h>`
- **FreeBSD**: Uses `backtrace()` from `<execinfo.h>`
- **macOS**: Uses `backtrace()` from `<execinfo.h>`

On other platforms,
a message indicating stack trace unavailability
will be logged instead.

**SDR Transaction Assertions (XNCHK macros)**

For assertions within SDR transactions,
use the `XNCHKERR`, `XNCHKZERO`, `XNCHKNULL`, and `XNCHKVOID` macros.
These variants additionally cancel the current SDR transaction
via `crashXn()` before the fail-fast check,
ensuring proper transaction cleanup.

**Interaction with ionrestart and SDR Reversibility**

When SDR transaction reversibility is enabled,
failed transactions trigger the `ionrestart` utility
to recover the system.
During recovery, `ionrestart` temporarily disables fail-fast mode
before restarting daemons
to prevent assertion failures from cascading
during the restart process.
Fail-fast mode is restored after all daemons have successfully restarted.

If you are writing tests
that intentionally trigger assertion failures
or crash recovery scenarios,
you should disable fail-fast mode at the start of your test:

```c
int off = 0;
oK(_coreFileNeeded(&off));
/* ... test code that may trigger assertions ... */
```

In the absence of any error,
the function returns a value that indicates nominal completion.
By convention this value is usually zero,
but under some circumstances other values
(such as pointers or addresses)
are appropriate indications of nominal completion.
Any additional information produced by the function,
such as an indication of “success”,
is usually returned as the value of a reference argument.

However, database management functions
and the SDR hash table management functions
deviate from this rule:
most return 0 to indicate nominal completion
but functional failure
(e.g., duplicate key or object not found)
and return 1 to indicate functional success.

Whenever returning a value that indicates an error:

- If the failure is due to the failure of a system call
  or some other non-ION function,
  assume that errno has already been set
  by the function at the lowest layer of the call stack;
  use `putSysErrmsg` (or `postSysErrmsg` if in a hurry)
  as described below.
- Otherwise – i.e., the failure is due to a condition
  that was detected within ION –
  use `putErrmsg` (or `postErrmsg` if pressed for time)
  as described below;
  this will aid in tracing the failure
  through the function stack in which the failure was detected.

When a failure in a called function is reported
to “driver” code in an application program,
before continuing or exiting
use `writeErrmsgMemos()` to empty the message pool
and print a simple stack trace identifying the failure.

Calling code may choose to ignore the error indication
returned by a function
(e.g., when an error returned by an sdr function
is subsumed by a future check of the error code
returned by `sdr_end_xn`).
To do so without incurring the wrath of a static analysis tool,
pass the entire function call as the sole argument to the `oK` macro;
the macro operates on the return code,
casting it to `(void)` and thus placating static analysis.
