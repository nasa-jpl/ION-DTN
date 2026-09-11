[SPDX-License-Identifier: BSD-3-Clause
SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
]::

# Contributing guide

This article describes how to report issues and contribute code.

## Reporting issues

Before reporting issues on the [Issues page](https://github.com/nasa-jpl/ION-DTN/issues),
please check that:

*   The issue applies to the most recent version on the `integration` branch
    and on a clean installation.

*   The issue is not a duplicate report.

Provide a complete description
and as many details as you find relevant.
This could include:

*   The date (`date -u`)

*   The operating system, architecture, and/or libc

*   The commit or version of ION or other tools
    (compiler, third-party libraries, etc.)

*   Core dumps, backtraces, and/or logs

*   ION configuration files

*   The expected behavior of a bugged feature

*   Detailed steps to reproduce a bug

Provide screenshots, links, files, etc. if you find they are needed.

## Code contributions

### Workflow

Contributing requires
a [GitHub](https://github.com) account
and a basic understanding of [Git](https://git-scm.com/).

1.  [Fork](https://github.com/nasa-jpl/ION-DTN/fork) the ION-DTN repository on GitHub.

1.  `git clone` your fork locally.

1.  Create a branch with `integration` as the start-point branch.

    ```console
    $ git switch -c my-branch integration
    ```

    Ensure your `integration` branch is up to date before branching.
    Do not work directly on the `integration` branch.
    This will make it easier to sync changes made in the original repository
    with your fork.

1.  Make your changes.
    Follow the guidelines in the [development guide](./development-guide.md).

1.  Separate each logical change into a separate, atomic commit.
    Each commit should make an easily understood change.
    Ideally, ION-DTN should compile without warnings or errors
    and all regression tests should pass at each commit.
    This makes it easier for tools like `git bisect` to track down bugs.

    *   The project uses a fast-forward merging style,
        so your commits will be directly copied and applied to `integration`.

    *   Be mindful of how a commit affects `git blame`.

    *   Order your commits cleanly and linearly
        to make your pull request (PR) easy to review.
        Do not be afraid to rewrite your history.

        *   Your history should not show your scratch work.
            An earlier commit of your branch should not introduce code
            that a later commit removes.

1.  Open a [pull request](https://github.com/nasa-jpl/ION-DTN/pulls) on GitHub.

### Rebasing onto `integration`

If changes are made to the `integration` branch after you created your branch,
then you must resolve any conflicts with `integration`
by rebasing it onto the most recent commit on the upstream `integration` branch.

#### Configuration

Configuring a remote repository for a fork
can be done immediately after cloning.
Assuming that your fork's remote is named `origin`
(this is the default name of the remote you cloned from),
add the nasa-jpl/ION-DTN repository as `upstream`:

```console
$ git remote add upstream https://github.com/nasa-jpl/ION-DTN.git
```

The project avoids merge commits to have a linear history.
Specify that only fast-forward merges are allowed:

```console
$ git config merge.ff only
$ git config pull.ff only
```

!!! tip

    If you're new to Git,
    pretend that the `merge` subcommand does not exist
    when working on ION.

#### Updating your branch

Pull new commits to your fork's `integration` branch from `upstream`:

```console
$ git switch integration
$ git pull upstream integration
```

Assuming that your branch's name is `my-branch`,
rebase the branch onto `integration`:

```console
$ git switch my-branch
$ git rebase integration
```

Push your updated branch:

```console
$ git push -u origin my-branch
```

If your updated branch changes the history in the remote,
use the `--force-with-lease` option with `git push`
to replace existing commits.
Note that this is potentially destructive, hence "force".

### Commit message guidelines

Commit messages are just as important as the changes themselves.
The primary goal of a commit message
is to explain __why__ a change was done.
This helps future developers (possibly including yourself!)
understand whether some code can be fixed,
improved, used elsewhere, or removed.

Begin with a single short line summarizing the changes.
Ideally limit the subject line to 50 characters.
A subject line generally should not end with punctuation.

The subject may be prefixed with an area of the codebase.

!!! example

    A commit that discards zero-length green data segments
    to prevent assertion failures
    could have the subject line:

    `ltp: reject a zero-length green data segment`

Be careful to use `doc:` for changes in the `doc/` directory,
and `docs:` for changes in the `site-docs/` directory.
Do not use either `doc:` or `docs:`
when the only files affected
are man pages and Markdown files (outside of `site-docs/`).

An asterisk (`*`) can be used to match multiple things.

!!! example

    A commit that updates section 3 man pages
    to have accurate function signatures and typedefs
    could have the subject line:

    `**/pod3/*.pod: Fix function signatures and typedefs`

If you're unsure about what prefix to use,
use `git log` on the affected files to see what previous commits used.

Use the present tense, imperative mood.
Say `foo: Fix xyz` instead of `foo: Fixed xyz`
or `This change fixed xyz in foo`
or `I fixed xyz`.

Bear in mind that the subject line should make a change discoverable
and that subject line is also used through Git
(such as in `git blame` and `git revert`).

Some changes are simple enough that a subject is sufficient,
such as simple typo fix.
But most changes to the codebase should follow the subject line
with a blank line and a more thorough description.

Guidelines for the message body:

*   Wrap the body of the message at 72 columns
    unless it breaks a reference like a URL.

*   Explain what the change accomplishes and why it is being done.
    Focus on the __why__, not the __how__.

    *   Explain the underlying problem that motivated the change.

    *   Describe alternative solutions considered
        or whether a change could be done in other areas,
        and explain why those actions were not taken.

*   Like the subject line,
    use the present tense, imperative mood.

    *   Make it sound like you are giving orders to the codebase
        instead of saying "I did so-and-so".

    *   Problems are described in the present sense
        as if the change has not been applied.
        Write "The code does so-and-so"
        instead of "The code used to do so-and-so".

        *   The word "currently" is redundant
            when describing problems
            since the status quo is without your change.

*   Never reference a plain issue or PR number.

    *   ION has and continues to use multiple issue trackers.
        A number by itself can be ambiguous;
        provide a URL to the issue or PR itself.

    *   Avoid linking to private issue trackers
        since most readers won't be able to access them.

*   A message may contain references,
    but the message should contain all relevant information
    in case those references become unavailable in the future.

    *   Referencing a previous commit may be relevant
        for noting when a bug was introduced,
        when a feature was introduced,
        or when a suggestion was made.

        *   Only reference commits that already exist in `integration`.
            Commits outside of `integration` are inaccessible
            to most readers.

        *   When referring to a previous commit,
            include the commit hash and the subject line of the commit ---
            preferably in the output of
            <code>git show -s \-\-pretty=reference <i>commit</i></code>.

## See also

The following resources inspired parts of this document:

*   [FreeBSD Committer's Guide](https://docs.freebsd.org/en/articles/committers-guide/)
*   [Git project, Submitting Patches](https://git-scm.com/docs/SubmittingPatches)
*   [Linux kernel, Submitting patches](https://docs.kernel.org/process/submitting-patches.html)
*   [Radare2's CONTRIBUTING.md](https://github.com/radareorg/radare2/blob/84e28a449edc97b72ef25eed18cafc7efa31fa35/CONTRIBUTING.md)
