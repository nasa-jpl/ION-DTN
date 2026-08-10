# ION Coding Guide

- [ION Coding Guide](#ion-coding-guide)
  - [Preface](#preface)
  - [C Language Standard](#c-language-standard)
    - [C11/C18 Features in Use](#c11c18-features-in-use)
    - [C99 Features Used Throughout](#c99-features-used-throughout)
    - [Per-Component Overrides](#per-component-overrides)
    - [Guidelines for Contributors](#guidelines-for-contributors)
    - [Operating System Support Matrix for Space Processors](#operating-system-support-matrix-for-space-processors)
  - [Application Behavior](#application-behavior)
  - [Function Design Guidelines](#function-design-guidelines)
  - [Error Checking](#error-checking)
    - [CHK Macro Behavior and Fail-Fast Mode](#chk-macro-behavior-and-fail-fast-mode)
  - [Error and Status Reporting](#error-and-status-reporting)
  - [‘C’ Coding Style](#c-coding-style)
    - [Naming Conventions](#naming-conventions)
    - [Indentation, Bracketing, Whitespace](#indentation-bracketing-whitespace)
    - [Comment Formatting](#comment-formatting)
    - [Miscellaneous Rules](#miscellaneous-rules)
  - [BP Service Access Point (SAP) Ownership](#bp-service-access-point-sap-ownership)

## Preface

The following coding guidelines apply to all software
delivered as part of the Interplanetary Overlay Network (ION) distribution,
except:

- Where the delivered software is legacy code
  rather than code developed specifically for ION.
- Where conformance to some other standard is clearly appropriate.
  For example, when using a framework library like Motif
  it may be appropriate to modify these guidelines
  so as to be consistent with the practices of the framework.
- Where, in the judgment of the programmer,
  deviating from the guidelines in a particular case results
  in manifestly clearer code.
  This is not a license to ignore the guidelines;
  it is intended to cover special circumstances.

Adherence to these guidelines is the responsibility of the individual programmer
but will be considered during peer reviews of new ION code.

## C Language Standard

ION targets **C18 (ISO 9899:2018)** as its primary standard,
with an automatic fallback to **C99 (ISO 9899:1999)** for older toolchains.
C17/C18 is a bugfix revision of C11 with no new language features,
so in practice ION is a **C11/C18 codebase**.

The build system in `configure.ac` establishes a preference hierarchy:

1. **C18** (`-std=iso9899:2018`) —
   validated by checking if `<stdatomic.h>` compiles.
2. **C99** (`-std=c99`) — validated by checking if `<stdint.h>` compiles.
3. The build fails if neither is supported.

The `-pedantic` flag is enabled to enforce strict standards compliance.

### C11/C18 Features in Use

- **Atomic operations** — used for lock-free reference counting
  (semaphore management),
  lock-free statistics counters (BP and LTP tally deltas),
  daemon shutdown flags,
  and inter-process semaphore table state in shared memory.
  See the **Atomic Operations** section below for the portable abstraction,
  the dual-zone architecture, and the three-tier fallback chain.

### C99 Features Used Throughout

- `<stdint.h>`, `<stdbool.h>`
- Designated initializers
- `inline` functions
- Mixed declarations and code
- `//` single-line comments

### Per-Component Overrides

Some subdirectories pin to a specific standard in their own Makefiles:

- **QCBOR, Unity, libbloom** — pinned to `-std=c99`
- **contrib/bptap** — uses `-std=gnu99` (GCC extension of C99)

### Guidelines for Contributors

- Avoid GNU extensions and non-standard constructs in core ION code.
- Use standards-compliant macro helper names.
- When adding new code that needs atomics,
  use the ION-specific opaque types `ion_atomic_t` (process-local)
  or `ion_ipc_atomic_t` (shared memory)
  together with the `ion_atomic_*` / `ion_ipc_atomic_*` accessor macros.
  Do not include `<stdatomic.h>` directly; include `ion_atomic.h` instead.
  See the **Atomic Operations** section below for selection rules.
- Prefer C99-compatible constructs for all other code;
  this maximizes portability to the C99 fallback path.
- A basic [`AGENTS.md`](AGENTS.md) is provided for use with LLMs.
  Copy it into the main folder to use.
  PRs and issues will not be accepted for this.
  To use it with Claude Code, create a symlink to it named `CLAUDE.md`.

## Application Behavior

Every process should return an exit code on termination.

- On normal termination, the exit code should be 0.
- On abnormal or error termination,
  the exit code should be a non-zero number in the range 1-255.
  - In this case the code should be 1
    unless specific codes are used
    to distinguish between different kinds of errors.

## Function Design Guidelines

All file I/O should be performed
using POSIX functions
rather than the buffered I/O functions
`fopen`, `fread`, `fseek`, etc.
This is because buffered I/O entails
the dynamic allocation of system memory,
which some missions may prohibit in flight software.

The `iputs` function provided in `platform.c`
should be used in place of `fputs`,
and the `igets` function should be used in place of `fgets`.
Rather than `fscanf`, use `igets` and `sscanf`;
rather than `fprintf`, use `isprintf` and `iputs`.

All varargs-based string composition
should be performed using `isprintf` rather than `sprintf`,
to minimize the chance of overrunning string composition buffers.
(`isprintf` is similar to `snprintf`.
Since VxWorks 5.4 does not support `snprintf`,
`isnprintf` is included in `platform.c`.)

Similarly, all string copying
should be performed using `istrcpy`
rather than `strcpy`, `strncpy`, and `strcat`.

The `isignal` function should be used
instead of `signal`;
it ensures that reception of a signal
will always interrupt system calls
in SVR4 fashion even when running on a FreeBSD platform.

The `iblock` function provides a simple,
portable means of preventing reception
of the indicated signal by the calling thread.

Data objects larger than 1024 bytes
should not be declared in stack space.
This is to

- Minimize complaints by Coverity, and
- Minimize the chance of overrunning
  allocated stack space
  when running on a VxWorks platform.

**Static variables that must be made globally accessible
should be declared within external functions,
rather than declared as external variables.**
This is per the JPL C Coding Standard,
but it also has the useful property
of providing an easy way to track all access
to a global static variable in `gdb`:
you just set a breakpoint at the start
of the function in which the variable is declared.

## Error Checking

In the implementation of any ION library function
or any ION task’s top-level driver function,
any condition that prevents the function
from continuing execution
toward producing the effect it is designed to produce
is considered an “error”.

Detection of an error should result
in the printing of an error message and, normally,
the immediate return of whatever return value
is used to indicate the failure of the function
in which the error was detected.

By convention this value is usually -1,
but both zero and NULL are appropriate failure indications
under some circumstances such as object creation.

The `CHKERR`, `CHKZERO`, `CHKNULL`, and `CHKVOID` macros
are used to implement this behavior
in a standard and lexically terse manner.

## Error and Status Reporting

To write a simple status message, use `writeMemo`.
To write a status message
and annotate that message with some other context-dependent string,
use `writeMemoNote`.
(The `itoa` and `utoa` functions may be used
to express signed and unsigned integer values,
respectively, as strings for this purpose.)
Note that adhering to ION’s conventions for tagging status messages
will simplify any automated status message processing
that the messages might be delivered to, i.e.,
the first four characters of the status message should be as follows:

- [i] – informational
- [?] – warning
- [s] – reserved for bundle status reports
- [x] – reserved for communication statistics

To write a simple diagnostic message, use `putErrmsg`;
the source file name and line number
will automatically be inserted into the message text,
and a context-dependent string may be provided.
(Again the `itoa` and `utoa` functions may be helpful here.)
The diagnostic message should normally begin with a capital letter
and end with a period.

To write a diagnostic message
in response to the failure of a system call
or some other non-ION function that sets errno,
use `putSysErrmsg` instead.
In this case, the diagnostic message
should normally begin with a capital letter
and not end with a period.

## ‘C’ Coding Style

This page contains guidelines for programming in the C language.

### Naming Conventions

Names of global variables, local variables, structure fields,
and function arguments are in mixed upper and lower case,
without embedded underscores,
and beginning with a lowercase letter.

```c
int numItems;
```

Private function names are in mixed upper and lower case,
without embedded underscores,
and beginning with a lowercase letter.

```c
void computeSomething(int firstArg, int secondArg);
```

Public function names are in lower case with tokens separated by underscores.
The first token of each public function name
is the name of the package
whose “include” directory contains the .h file
in which the function prototype is defined.

```c
 extern int ltp_open(unsigned long clientId);
```

Macro names are written in upper case with tokens separated by underscores.

```c
#define SYMBOLIC_CONSTANT 5
```

Unions are not used.

Typedef names are in mixed upper and lower case,
with the first token capitalized.
Type names are never the same
as the structure or enum tags
for the structures and enums that they name.

```c
typedef struct gloplist_str
{
int thing1;
int thing2;
} GlopList;
```

### Indentation, Bracketing, Whitespace

No line of source text is ever more than 80 characters long.
When the length of a line of code exceeds 80 characters,
the line of code is wrapped across two or more lines of text.
Whenever the point at which the text must be wrapped is within a literal,
a newline character (\) is inserted at the wrap point
and the continuation of the literal begins in the first column
of the next text line.
Otherwise, each continuation line is normally indented two tab stops
from the first text line of the long line of code;
when indenting just one tab stop (rather than two)
seems to make the code more readable,
indenting one tab stop is okay.
When a single meaningful clause of a source code line
must be wrapped across multiple lines of text,
each text line after the first line in that clause
is normally indented one additional tab stop.

In the declaration of a function or variable,
the type name and function/variable name are normally separated
by a single tab.
They may be separated by multiple tabs
when this is necessary in order to have the variable names
in multiple consecutive variable declarations line up,
which is always preferred.

Functions are written with the return type, function name, and arguments
as a single line of code,
subject to the code line wrapping guidelines given above.
The opening brace of the function definition
appears in the first column of the next line.

The opening brace of a structure definition
likewise appears in the first column of the next line
after the structure name.

A control statement (starting with `if`, `else`, `while`, or `switch`)
begins a new line of code.
The opening brace for the control statement always appears on the next line,
at the same indentation as the control statement keyword.

The first line of code appearing after an opening brace
(whether for a structure definition, for a function definition,
or in the scope of a control statement)
always appears on the next line, indented one tab stop.
From that point on, every subsequent line of code
is indented the same number of tabs as the preceding line of code,
subject to the code line wrapping guidelines given above.

Every closing brace always appears in the same column
as the corresponding opening brace.

Every closing brace is always followed by a single blank line,
except when it is immediately followed
either by another closing brace
(which will be indented one less tab stop)
or by an else
(which will be indented by the same number of tab stops
as the closing brace and, therefore, the corresponding if).

```c
static void  computeSomething(int numItems, Item *items)
{
 unsigned int x;
 int  i;

 while (x > 0)
 {
  x--;
 }

 for (i = 0; i < numItems; i++)
 {
  x += items[i].field1;
 }

 if (numItems == 0)
 {
  doThis();
 }
 else
 {
  doThat();
 }
}
```

The case labels in switch statements line up with the braces.
Every case (or default) label in the switch, after the first case,
is preceded by a blank line.
Cases which do not include a break or return statement
either contain no code at all
or else end with a comment along the lines of:

```c
/* Intentional fall-through to next case. */
For example:
switch (ch)
{
case 'A':
.
.
.
break;

case 'B':
case 'C':
.
.
.
break;

case 'D':
.
.
.
/* Intentionally falls through. */

case 'E':
.
.
.
break;

default:
.
.
.
break;
}
```

### Comment Formatting

Comments are so rare and valuable
that we hesitate to risk discouraging them by overly constraining their format.
In general, comments should be inserted
in such a way as to be as easy as possible to read
in relevant context.
The multi-line comment formatting performed automatically by vim
is particularly acceptable.

```c
/* Here is the beginning of an extremely long comment, so long
 *  that it has to wrap over two lines of source code text. */
```

### Miscellaneous Rules

Use – and write – thread-safe library functions where possible.
E.g., normally prefer `strtok_r()` to `strtok()`.

Avoid writing non-portable code,
e.g., prefer POSIX library calls to OS-specific library calls.

Template for ".c" files

```c
 1 2 3 4 5 6 7
123456789012345678901234567890123456789012345678901234567890123456789012
/*
 platform_sm.c: platform-dependent implementation of common
   functions, to simplify porting.

 Author:  Alan Schlutsmeyer, JPL

 Copyright 1997, California Institute of Technology.
 ALL RIGHTS RESERVED.  U.S. Government sponsorship
 acknowledged.
                                         */
```

Each file should have a header comment like the one shown above.

```c
#include <stdio.h>
#include <locallib.h>
#include "appheader.h"
  .
  .
  .
```

.h files are included just after the header.
System-provided headers should be specified with angle brackets;
ION-provided headers should be specified with double-quotes.

```c
#define SYMBOLIC_CONSTANT 5
```

Next, symbolic constants and macros (if any) are defined.
They normally go first,
because they might be used in the definitions of data types
and static variables.
However, symbolic constants and macros may be inserted later in the source text
if that will improve the readability of the file.

```c
typedef struct fb_str
{
 int field1;
 in field2;
} Foobar;
```

Data types are defined next because they might be used by static variables.

```c
static int numFoobars = 0;  /* Number of foobars in the program. */
  .
  .
  .
```

Global functions used only within the program should be declared static.
Public function prototypes should be in a header file;
the definitions of those functions, with their headers,
are included in the corresponding .c file.
Low-level functions, such as commonly-used utility functions,
appear first in the .c file.
They are followed by the functions that call those functions directly,
followed by higher-level-functions that call those functions, and so on.

Template for ".h" Files

```c
 1 2 3 4 5 6 7
123456789012345678901234567890123456789012345678901234567890123456789012
/*
 platform_sm.h: portable definitions of types and functions.

 Author:  Alan Schlutsmeyer, JPL

 Copyright 1997, California Institute of Technology.
 ALL RIGHTS RESERVED.  U.S. Government sponsorship
 acknowledged.
                                         */
```

Each header file begins with a standard header comment like the one shown above.

```c
#ifndef _PLATFORM_SM_H_
#define _PLATFORM_SM_H_
```

Each header file must have an "include" guard.

```c
#include "platform.h"
  .
  .
  .
```

Next come any includes required by the declarations in the header.

```c
#ifdef __cplusplus
extern "C" {
#endif
```

Next comes the beginning of the C++ guard.
This allows the header to be included in a C++ program
without error.

Next come declarations of various sorts,
followed by the ends of the C++ and “include” guards.

```c
#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_SM_H */
```

Nothing should go after the "#endif" of the include guard.

## BP Service Access Point (SAP) Ownership

A Bundle Protocol Service Access Point (`BpSAP`)
opened for reception via `bp_open()`
is **owned exclusively by the single thread that opened it**.
This is a hard invariant of the ION BP API, not a recommendation,
and it has two consequences that callers must respect.

**Only the owning thread may receive on the SAP.**
The volatile endpoint object (`VEndpoint`)
tracks ownership with a `<appPid, appCookie>` tuple,
set on `bp_open()` and cleared on `bp_close()`.
`appPid` is `sm_TaskIdSelf()`
(the OS PID;
on Linux this is the thread-group ID via `getpid()`,
so every thread in a process reports the same value).
`appCookie` is `sm_ProcessCookie()`,
a per-process-instance value
that lets ION distinguish two distinct processes
that happen to share a recycled PID.
Ownership checks in `bp_receive()` and `bp_close()`
are at *process* granularity, not thread granularity:
ION cannot detect a second thread in the owning process
calling `bp_receive()` on a SAP it did not open.
Such cross-thread reception is undefined behaviour
at the API level
and will race against the owner
inside the delivery semaphore and SDR transactions.
Applications that need delivery from multiple threads
must funnel reception through the single owning thread.

**A second `bp_open()` on the same endpoint always fails.**
ION does not permit a process or thread to "reopen" an endpoint
it already holds.
If `vpoint->appPid` is set to a live task,
every subsequent `bp_open()` returns `-1`
with `putErrmsg("Endpoint is already open.", "<pid>")`,
regardless of whether the second caller is:

* the same thread that already opened it,
* a different thread in the same process, or
* a thread in a different process.

The collapse of these three cases into one error path is deliberate:
ION has no per-thread ownership token,
so it cannot meaningfully distinguish them,
and any attempt to be "lenient" for the same-PID case
produced an `rc == 0` return with `*bpsapPtr == NULL`
that propagated as a delayed null-pointer dereference in `bp_receive()`.
Callers should test `bp_open()`'s return with `if (rc != 0)`
(or `if (rc < 0 || sap == NULL)` to also handle the `dtn:none` null-EID case)
and bail out rather than dereferencing the SAP.
If a previous owner died without calling `bp_close()`,
`createBpSAP()` self-heals by clearing `appPid`
and allowing the new open to proceed; the caller does not need to retry.
The same self-heal fires for PID recycling:
when a brand-new process
happens to receive the same PID as the dead original owner,
its `appCookie` mismatches,
so `createBpSAP()` treats the stored ownership as stale
and reclaims the endpoint
instead of misidentifying the new process as the original owner.

**Source-only SAPs are exempt.**
`bp_open_source()` (used for send-only SAPs) does not set `appPid`
and imposes no exclusivity.
Multiple threads or processes may open source SAPs
on the same endpoint concurrently without conflict.

**Lifetime invariant — close from the owning thread, on every exit path.**
A reception SAP must be closed by its owning thread
before that thread exits,
*including* abnormal exit paths
(cancellation, longjmp out of the receive loop,
an error return out of the worker function).
If the owning thread terminates without calling `bp_close()`,
the endpoint remains locked for the lifetime of the **process**:
ION's self-heal logic in `createBpSAP()` only fires
when `sm_TaskExists(appPid)` reports the whole process is gone,
not the individual thread.
Subsequent `bp_open()` calls on that endpoint —
from any thread in the same process —
will keep returning `-1` with `"Endpoint is already open."`
until the process exits.

The recommended idiom is to bracket the receive loop
with a thread-cancellation cleanup handler
so `bp_close()` runs regardless of how the thread leaves the loop:

```c
static void closeSap(void *arg)
{
    bp_close((BpSAP) arg);
}

void *worker(void *arg)
{
    BpSAP sap;

    if (bp_open(eid, &sap) != 0)
    {
        return NULL;       /* bp_open already logged via putErrmsg */
    }
    pthread_cleanup_push(closeSap, sap);
    while (running)
    {
        if (bp_receive(sap, &dlv, BP_BLOCKING) < 0) break;
        /* ... process delivery ... */
    }
    pthread_cleanup_pop(1);     /* runs closeSap on every exit path */
    return NULL;
}
```

`pthread_cleanup_pop(3)` invokes the handler
on normal return, on `pthread_exit()`, and on `pthread_cancel()`.
It does **not** run if the thread is killed by an asynchronous signal
or if the whole process crashes —
for those, only process-exit cleanup applies,
and `createBpSAP()`'s self-heal will release the endpoint
on the next ION-aware start.
