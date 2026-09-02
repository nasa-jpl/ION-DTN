[SPDX-License-Identifier: BSD-3-Clause
SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
]::

# Development guide

This article explains how to set up a development environment
and the coding guidelines of the Interplanetary Overlay Network (ION).

The following coding guidelines apply to all software
delivered as part of the ION distribution,
except:

-   Where the delivered software is legacy code
    rather than code developed specifically for ION.
-   Where conformance to some other standard is clearly appropriate.
    For example, when using a external library
    it may be appropriate to modify these guidelines
    to be consistent with the practices of the library.
-   Where, in the judgment of the programmer,
    deviating from the guidelines in a particular case results
    in manifestly clearer code.
    This is not a license to ignore the guidelines;
    it is intended to cover special circumstances.

Adherence to these guidelines is the responsibility of the individual programmer
but will be considered during peer reviews of new ION code.

## Development environment

### Dependencies

The default build of ION requires:

* GNU Autotools (Autoconf, Automake, Libtool)
* pkg-config
* GNU Make or BSD Make
* GCC or Clang with C17 support
* C standard library with functionality from POSIX.1-2008

Testing the default build requires Bash.
Additional tests require Python 3.

Some configuration options require additional dependencies.

[TODO: make an article that explains each option and their dependencies.
Then reference it here.
]::

### EditorConfig

The codebase has several `.editorconfig` files to help the project
maintain a consistent style.
Editors will be automatically configured to follow ION's conventions
if they have [EditorConfig](https://editorconfig.org/) enabled.

### Language server

[Bear](https://github.com/rizsotto/Bear)
generates a [compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html)
in the build directory with `bear -- make`.
The generated `compile_commands.json` is used by language servers
(e.g., clangd and ccls) to interpret source code.

!!! note

    When storing generated files in the same directory as the Git repository,
    set up an [exclude file](https://git-scm.com/docs/gitignore/2.55.0),
    e.g., at `.git/info/exclude`, to ignore them.

### AGENTS.md

A basic [`AGENTS.md`](AGENTS.md) is provided for use with LLMs.
Copy it into the main folder to use.
PRs and issues will not be accepted for this.
To use it with Claude Code, create a symlink to it named `CLAUDE.md`.

## C guidelines

Abide by all _shall_ rules of the _JPL Institutional Coding Standard
for the C Programming Language_ up to LOC-4.
_Should_ rules up to LOC-4 are encouraged,
but several of these _should_ rules are difficult to apply to ION
since core components were created prior to the coding standard.
The following _should_ rules are not followed:

* 6: Use IPC messages for task communication.
* 8: Explicitly transfer write-permission (ownership) for shared data objects.
* 25: Use short functions with a limited number of parameters.

The following list summarizes rules from the coding standard that are followed.
_Should_ rules indicated with a "*" are less strictly enforced.
For additional guidance, see the coding standard.

???+ abstract "JPL Coding Standard rule summary"

    *   LOC-1 Language Compliance
        *   1: Do not stray outside the language definition.
        *   2: Compile with all warnings enabled;
            use static source code analyzers.
    *   LOC-2 Predictable Execution
        *   3: Use verifiable loop bounds for all loops
            meant to be terminating.
        *   4: Do not use direct or indirect recursion.
        *   5: Do not use dynamic memory allocation after task initialization.
        *   7: Do not use task delays for task synchronization.
        *   9: Place restrictions on the use of semaphores and locks.
        *   10: Use memory protection, safety margins, barrier patterns.
        *   11: Do not use `goto`, `setjmp`, or `longjmp`.
        *   12: Do not use selective value assignments
            to elements of an enum list.
    *   LOC-3 Defensive Coding
        *   13: Declare data objects at smallest possible level of scope.
        *   14: Check the return value of non-void functions,
            or explicitly cast to `(void)`.
        *   15: Check the validity of values passed to functions.
        *   16: Use static and dynamic assertions as sanity checks.
        *   *17: Use typedefs that indicate size and signedness
            instead of predefined C data types such as `int`, `short`, etc.
        *   18: Make the order of evaluation in compound expressions explicit.
        *   19: Do not use Boolean expressions with side effects.
    *   LOC-4 Code Clarity
        *   20: Make only very limited use of the C preprocessor.
        *   21: Do not define macros within a function or a block.
        *   22: Do not undefine or redefine macros.
        *   23: Place `#else`, `#elif`, and `#endif` in the same file
            as the matching `#if` or `#ifdef`.
        *   *24: Place no more than one statement or declaration
            per line of text.
        *   *26: Use no more than two levels of indirection per declaration.
        *   *27: Use no more than two levels of dereferencing
            per object reference.
        *   *28: Do not hide dereference operations
            inside macros or typedefs.
        *   *29: Do not use non-constant function pointers.
        *   30: Do not cast function pointers into other types.
        *   31: Do not place code or declarations
            before an `#include` directive.

### Language standard

ION primarily targets C17,
but maintains support for C99.

When using C17 features, ensure that it is still possible to build with C99
during configuration time.
Maintain compatibility with conditional compilation or fallback implementations.

!!! warning

    Do not use C11 atomics directly,
    use the abstraction in `ion_atomic.h`.
    See [Atomic Operations](./atomic-operations.md) for more information.

It is acceptable for a default configuration build
to rely on C17 features --- for example,
a build with the following configuration may fail:

```console
$ CFLAGS="-std=c99" ./configure
```

However, it must be possible to pass an appropriate set of flags
to enable a successful C99 build:

```console
$ CFLAGS="-std=c99" ./configure --some-flags
```

The necessary configuration to build with C99 and C99 compatible options
must be thoroughly documented in the [quick start guide](./quick-start-guide.md).

[TODO: As of a8fc35d6f (Bump runner dependency version, 2026-08-31),
we only talk about building in quick start guide and design operations guide,
both of which in an incomplete manner.
Once we make a separate page talking about building and go in-depth about
each option, update the line above with a hyperlink to it.
]::

Do not stray outside the language definition;
do not rely on compiler or platform specific behavior,
and do not rely on undefined or unspecified behavior.
Non-standard extensions are tentatively allowed
if their use is opt-in at configuration time.

### Application behavior

Every process should return an exit code on termination.

-   On normal termination, the exit code should be 0.
-   On abnormal or error termination,
    the exit code should be a non-zero number in the range 1--255.
    -   In this case the code should be 1
        unless specific codes are used
        to distinguish between different kinds of errors.

### Function design

All file I/O should be performed
using unbuffered functions taking file descriptors
(`open`, `read`, `lseek`, etc.)
rather than the buffered I/O functions
(`fopen`, `fread`, `fseek`, etc.).
This is because buffered I/O entails
the dynamic allocation of system memory,
which some missions may prohibit in flight software.

The `iputs` function provided in `platform.c`
should be used in place of `fputs`,
and the `igets` function should be used in place of `fgets`.
Rather than `fscanf`, use `igets` and `sscanf`;
rather than `fprintf`, use `snprintf` and `iputs`.

All varargs-based string composition
should be performed using `snprintf` rather than `sprintf`,
to minimize the chance of overrunning string composition buffers.

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

-   Minimize complaints by Coverity, and
-   Minimize the chance of overrunning
    allocated stack space when running on a VxWorks platform.

**Static variables that must be made globally accessible
should be declared within external functions,
rather than declared as external variables.**
This is per the JPL C Coding Standard,
but it also has the useful property
of providing an easy way to track all access
to a global static variable in `gdb`:
you just set a breakpoint at the start
of the function in which the variable is declared.

### Error checking

In the implementation of any ION library function
or any ION task's top-level driver function,
any condition that prevents the function
from continuing execution
toward producing the effect it is designed to produce
is considered an "error".

Detection of an error should result
in the printing of an error message and, normally,
the immediate return of whatever return value
is used to indicate the failure of the function
in which the error was detected.

By convention this value is usually -1,
but both zero and NULL are appropriate failure indications
under some circumstances such as object creation.

!!! danger

    The `CHK*` macros were historically used
    to implement this behavior.
    Ensure that a `CHK*` macro **never** appears in the execution path
    of any public function.
    Use explicit error checking and logging instead.

    The `CHK*` macros [affect control flow differently](./chk-macro-behavior.md)
    depending on how the build is configured,
    so any library function that uses the macros
    (and applications that use the library function)
    will behave in surprising ways across systems
    with different build configurations.

### Error and status reporting

To write a simple status message, use `writeMemo`.
To write a status message
and annotate that message with some other context-dependent string,
use `writeMemoNote`.
(The `itoa` and `utoa` functions may be used
to express signed and unsigned integer values,
respectively, as strings for this purpose.)
Note that adhering to ION's conventions for tagging status messages
will simplify any automated status message processing
that the messages might be delivered to, i.e.,
the first four characters of the status message should be as follows:

- <code>[i]&nbsp;</code> -- informational
- <code>[?]&nbsp;</code> -- warning
- <code>[s]&nbsp;</code> -- reserved for bundle status reports
- <code>[x]&nbsp;</code> -- reserved for communication statistics

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

### Include what you use

Every `.c` file and `.h` file must include the exact headers
for every symbol (type, function, variable, macro)
that the file uses.
Do not rely on transitive includes.

The [`include-what-you-use`](https://include-what-you-use.org/)
utility can help with this,
but the utility's use is not enforced.

!!! note

    When using `include-what-you-use`,
    omit the "why" comments,
    either with `--comment_style=none`
    or `--no_comments`.

### Conditional compilation

Rather than using preprocessor conditionals directly in `.c` files
for conditional compilation,
prefer using conditionals in a header file defining functions
with conditionally different definitions,
and then call those functions unconditionally from `.c` files.

Prefer to make an entire function conditional
rather than parts of a function.
If parts of a function need to behave differently,
factor out that logic into a separate helper function,
and define the helper function conditionally.

Never guess the presence of features
based on system-specific or common predefined macros.
System-specific predefined macros
such as `__unix__` and `linux`
should never be used.
Always check specifically for the features you need
(usually with Autoconf) ---
test for features, not for platforms or compilers.

Avoid conditional compilation in publicly installed header files,
as doing so would require leaking ION's build configuration
to downstream users.

### Public interfaces

Be very mindful about what is exposed in an installed header.
Once a function is public,
moving the function, changing its name, parameters or return type
will break the API.
If a function is intended to only be used internally by ION,
do not expose it in a public header.

A function's contract must be consistent across build configurations.
Avoid altering function parameters or return types
based on configuration flags.

Similarly, design structure definitions to prevent ABI breakages.
Keep the size and layout consistent across build configurations.

Ensure the [names](#naming) of public identifiers are properly scoped.
Do not use generic names that could conflict with other libraries
or with user code.

Be mindful of the user experience of a CLI.
Once an interface is public,
users will have expectations
and [changes](#changes-to-public-interfaces) may be non-trivial.

### Man pages

Document all public interfaces in man pages.
Man pages are the primary means of documenting API contracts.
Do not rely on comments in header files.

The man pages are first-class citizens.
Code should not be admitted to the public interface
if the interface is not documented.
The man pages must be accurate and kept up to date.

Ensure man pages are complete.
In particular,
for section 3 man pages,
ensure the documentation explains how to safely use the API.
For example,
does a function need to be used within an SDR transaction?
Does the caller need to hold a lock?
If locks need to be held, what is the intended locking order?
What are the side-effects from the function?
What happens if an error happens so the function has to return early?
Are the effects rolled back?
When we pass something, is the function going to modify its state?
Is a function re-entrant?
Is it thread-safe?
What arguments are valid?
What happens to pointer arguments on error?

!!! note

    When an external man page is cross-referenced,
    you may need to add it to the `EXTERNAL_MANPAGES` set
    in `.github/scripts/check_doc_consistency.py`
    to prevent CI failures.

### Changes to public interfaces

Once something is a part of our public interface
(CLIs and libraries),
it cannot be significantly changed without warning
since users may depend on parts of it.
(This is why you don't expose stuff unless you need to.)

Unless something is an outright bug or vulnerability,
a transition period must be provided
where an interface retains its existing behavior
while an improved / intended behavior is also available.
This could start with a configuration flag to opt-in to a new behavior,
later an opt-out period to stick to the old behavior,
and then eventually removing the old behavior altogether.

Be complete in documenting significant changes.
Provide notices in man pages and in header files (if applicable).
Update documentation under `site-docs/`
to show examples users _should_ be following.

!!! warning

    Be mindful of API and ABI stability.
    Rather than modifying an existing interface,
    it may be better to create a new one.

### Miscellaneous rules

*   Use --- and write --- thread-safe library functions where possible.
    E.g., normally prefer `strtok_r()` to `strtok()`.

*   Avoid writing non-portable code,
    e.g., prefer POSIX library calls to OS-specific library calls.

*   If a function returns `void *`,
    do not explicitly cast its return value to any pointer types.

### C code style

Most styling rules are handled by the `.clang-format` file
in the root of the source tree.
If [clang-format]'s styling is particularly poor,
use `// clang-format off` and `// clang-format on`
to disable formatting pieces of code.
If the side-effects are minor,
consider updating the `.clang-format` file itself.

The remainder of this section documents styling rules
that aren't handled by clang-format.

[clang-format]: https://clang.llvm.org/docs/ClangFormat.html

#### Naming

!!! warning

    Do not use identifiers reserved by the C standard or by POSIX.
    See the [Reserved Names] section of the glibc manual,
    and [Section 2.2.2 The Name Space][posix-namespace]
    of the System Interfaces volume from POSIX.1
    for more information.

[Reserved Names]: https://sourceware.org/glibc/manual/2.44/html_node/Reserved-Names.html
[posix-namespace]: https://pubs.opengroup.org/onlinepubs/9799919799/functions/V2_chap02.html#tag_16_02_02

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
whose "include" directory contains the `.h` file
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

!!! warning

    Be cautious when creating new typedefs.
    A typedef should generally only be used to intentionally hide something.
    Do not use typedefs merely to avoid writing `struct`.

#### Line length

The preferred line width is 80 characters,
though output from `clang-format` may exceed this for clarity.
Anything frequently grepped for,
such as error or log messages,
should never be broken up over multiple lines.
Similarly, references like URLs in comments
should remain on a single line even if they exceed 80 characters.

Whenever the point at which the text must be wrapped is within a literal,
a backslash (`\`) is inserted at the wrap point
and the continuation of the literal begins in the first column
of the next text line.

For example, a long string literal can be broken as follows:

```c
char x[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do \
eiusmod tempor incididunt ut labore et dolore magna aliqua.";
```

Alternatively, make use of adjacent string literals being concatenated:

```c
char x[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
	   "eiusmod tempor incididunt ut labore et dolore magna aliqua.";
```

#### Comments

Block comments (`/* ... */`) and line comments (`//`) are both acceptable.
Within a file, consistently use either block or line comments
for single-line notes.
Multiline comments should also be consistent,
but may differ from single-line comment choices.
Copyright statements should be treated separately from this guideline;
follow the guidance in the [copyright statements](#copyright-statements) section,
regardless of what comment style a file uses.

<!--
SPDX-SnippetBegin
SPDX-License-Identifier: BSD-2-Clause
SPDX-SnippetCopyrightText: Copyright (c) 1995-2025 The FreeBSD Project
SPDX-SnippetName: style(9) comment style
Modified from
https://cgit.freebsd.org/src/tree/share/man/man9/style.9?id=1876f629b97608679f1bd71b9aa88a57b55c4574
with a multiline comment using `//`
-->

```c
/*
 * VERY important single-line comments look like this.
 */

/* Most single-line comments look like this. */

// Although they may look like this.

/*
 * Multiline comment using block style. Make these real sentences. Fill them so
 * they look like real paragraphs.
 */

// Multiline comment starting with //. Make these real sentences. Fill them so
// they look like real paragraphs.
```

<!-- SPDX-SnippetEnd -->

Take care to not over-comment:

*   Explaining why a change was made belongs in commit messages, not comments.

    *   Do not reference issue or PR numbers in comments.
        Referencing a public issue or PR with a piece of code
        belongs in the commit message.

*   Do not over-reference other parts of the codebase
    such that changes to the referenced parts
    would require updating comments to make them correct.

*   Avoid obvious comments like:
    ```c
    /* Local variables */
    int one;
    size_t two;

    /* Check if foo() works */
    if (!foo())	  /* If foo() fails */
    {
    	return 1; /* Return 1 on error */
    }
    ```

Avoid stylistic constructs (borders, tables, boxes, dividers, etc.)
to lessen maintenance burden.

When code is conditionally compiled with `#if` or `#ifdef`,
add a comment to the matching `#else`, `#elif`, or `#endif`
if it will help readers discern where conditional regions end.
Whether or not a comment makes code less confusing
is based on your subjective judgment.
Generally, comments are only used for large conditional regions;
avoid commenting for single-line conditional blocks.

The comment for `#endif` should match the expression used in `#if` or `#ifdef`.
The comment for `#else` should match the inversion expression(s)
used in preceding `#if` and/or `#elif` statements.
In comments, abbreviate `defined(FOO)` as `FOO`.

```c
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef FOO
/* Large region here, or other conditional code. */
#else /* !FOO */
/* Or here. */
#endif /* FOO */

#ifndef BAR
/* Another large region here, or other conditional code. */
#elif BAZ /* BAR */
/* And here. */
#else /* BAR && !BAZ */
/* Or here. */
#endif /* !BAR */
```

#### `.c` file layout

A `.c` file should start with a [copyright statement](#copyright-statements):

```c
         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
```

Leave a blank line before the `#include` directives.

System-provided headers should be specified with angle brackets;
ION-provided headers should be specified with double-quotes.
The grouping and ordering of `#include`s is handled by clang-format.

```c
#include <stdio.h>
#include <locallib.h>

#include "appheader.h"
```

!!! note

    ION's headers historically did not follow the IWYU principle
    and also defined feature test macros inside headers
    instead of the build system.
    If clang-format's sorting of `#include`s results in build errors,
    disable formatting around the `#include`s with `// clang-format off`,
    and sort them manually.

Next, symbolic constants and macros (if any) are defined.
They normally go first,
because they might be used in the definitions of data types
and static variables.
However, symbolic constants and macros may be inserted later in the source text
if that will improve the readability of the file.

```c
#define SYMBOLIC_CONSTANT 5
```

Data types are defined next because they might be used or implied
by static variables.

```c
struct foobar_str
{
	int field1;
	int field2;
};

static int numFoobars = 0; /* Number of foobars in the program. */
```

Global functions used only within the program should be declared static.
Public function prototypes should be in a header file;
the definitions of those functions, with their headers,
are included in the corresponding `.c` file.
Low-level functions, such as commonly-used utility functions,
appear first in the `.c` file.
They are followed by the functions that call those functions directly,
followed by higher-level-functions that call those functions, and so on.

#### `.h` file layout

Each header file begins with a [copyright statement](#copyright-statements):

```c
         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
```

Each header file must have an `#include` guard.

```c
#ifndef MY_HEADER_H
#define MY_HEADER_H
```

Bear in mind the [naming](#naming) guidelines;
avoid a name like `_MY_HEADER_H`.

Next comes any `#include`s required by the declarations in the header.
This follows the same styling used in `.c` files.

```c
#include "platform.h"
```

Next comes the beginning of the C++ guard.
This allows the header to be included in a C++ program
without error.

```c
#ifdef __cplusplus
extern "C" {
#endif
```

Next come declarations of various sorts
(constants, types, function prototypes),
followed by the ends of the C++ and `#include` guards.

```c
#ifdef __cplusplus
}
#endif

#endif /* MY_HEADER_H */
```

Nothing should go after the `#endif` of the include guard.

## Shell guidelines

*   Maximum line length is 80 characters.

*   Prefer `#!/bin/sh` as the shebang line.

    *   Scripts should work on the default `/bin/sh` on ION's targets
        (Bash, DASH, Solaris' `ksh`, and FreeBSD's `ash`).

*   If Bash must be used, use `#!/usr/bin/env bash` as the shebang line.

*   Address warnings from [ShellCheck](https://www.shellcheck.net/).
    Disable specific warnings if they are not applicable.

*   Do not use features exclusive to GNU coreutils,
    or at least guard their use.
    Scripts should work with Solaris' and FreeBSD's core utilities.

*   If unsure about formatting, use [`shfmt`](https://github.com/mvdan/sh),
    but this is not enforced in CI.

*   Follow the guidelines
    about over-commenting and avoiding stylistic constructs
    from the [C comments section](#comments).

## Python guidelines

*   Maximum line length is 88 characters.

*   Use [Ruff](https://docs.astral.sh/ruff/)
    to format code and organize imports.

*   Follow [PEP 8](https://peps.python.org/pep-0008/)
    for style guidelines not covered by Ruff.

*   Follow the guidelines
    about over-commenting and avoiding stylistic constructs
    from the [C comments section](#comments).

## Copyright statements

For new files, a `SPDX-License-Identifier` tag followed by copyright text
should be placed at the first possible lines in a file which can contain
a comment.

=== "C"

    For new files, prefer a format like:

    ```c
    // SPDX-License-Identifier: BSD-3-Clause
    // SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
    ```

    A style found in older files is also acceptable:

    ```c
    /*
     * some_file.c: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do
     *              eiusmod tempor incididunt ut labore et dolore magna aliqua.
     *
     * Copyright (c) 1997-2026, California Institute of Technology.
     * ALL RIGHTS RESERVED.  U.S. Government sponsorship acknowledged.
     *
     * Author: Scott Burleigh, Jet Propulsion Laboratory
     */
    ```

    If a file already has a copyright statement in an older style,
    do not bother changing it.
    But a `SPDX-License-Identifier` comment may be added to a file
    with the older style
    (either in the form of `//` comment preceding the `/* */` comment,
    or in the `/* */` comment itself before or after the "Copyright" lines.)

=== "Shell"

    ```sh
    #!/bin/sh
    # SPDX-License-Identifier: BSD-3-Clause
    # SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
    ```

=== "Python"

    ```py
    #!/usr/bin/env python3
    # SPDX-License-Identifier: BSD-3-Clause
    # SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
    ```

=== "Markdown"

    ```md
    [SPDX-License-Identifier: BSD-3-Clause
    SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
    ]::
    ```

Most files are licensed under BSD-3-Clause.
Use a different [SPDX License Expression](https://spdx.github.io/spdx-spec/v3.0.1/annexes/spdx-license-expressions/)
if appropriate.

The copyright year should start with the year of file creation.
When copyrightable changes are made to a file,
the year range should be incremented.
For example, a file created in 2011 and with copyrightable changes in 2026
would have

```
SPDX-FileCopyrightText: 2011-2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
```

Handle snippets of copyrightable code
with `SPDX-SnippetBegin` and `SPDX-SnippetEnd` tags.
The snippet should have at least `SPDX-License-Identifier`
and `SPDX-SnippetCopyrightText` tags.
Use additional tags at your discretion.
See [Annex H.3 Snippet tags format](https://spdx.github.io/spdx-spec/v2.3/file-tags/#h3-snippet-tags-format)
of version 2.3 of the SPDX Specification for more information.

Keep the value of SPDX file tags and snippet tags in a single line,
regardless of line length limits.

### License compatibility

ION's codebase (including third-party snippets)
must be licensed under a permissive license compatible with BSD-3-Clause.

Do not submit code licensed under more restrictive licenses
that impose additional restrictions on ION.

## See also

The following resources inspired parts of this document:

*   [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html)
*   [FreeBSD `style(9)`](https://man.freebsd.org/cgi/man.cgi?query=style%289%29&manpath=FreeBSD+15.1-RELEASE)
