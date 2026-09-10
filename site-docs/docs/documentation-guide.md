[SPDX-License-Identifier: BSD-3-Clause
SPDX-FileCopyrightText: 2026, California Institute of Technology. ALL RIGHTS RESERVED. U.S. Government sponsorship acknowledged.
]::

# Documentation guide

This article provides guidelines for writing documentation
under the `site-docs/` directory.

## Getting started

The documentation uses [Zensical](https://zensical.org/docs/get-started/)
as its static site generator.

`site-docs/` holds a [uv](https://docs.astral.sh/uv/) project.
Run `uv sync` in the `site-docs/` directory to set up the project
and sync dependencies:

```console
$ uv sync
```

After syncing,
dependencies like `zensical` are available
in the project virtual environment.

Run `zensical serve` in the virtual environment
to build and serve the docs at <http://localhost:8000>:

```console
$ uv run zensical serve
```

!!! note

    If TCP port 8000 is already used,
    a different port can be set with the `-a` option
    of `zensical serve`.
    Run `zensical serve --help` for more information.

### Editing environment

Set up your editor to use [EditorConfig](./development-guide.md#editorconfig).

Do not use rendered documentation
apart from what Zensical provides.
Evaluate the output based on what your browser renders
since this is the format that readers will use.

## Writing style

*   Be clear, complete, and concise.

*   Use serial commas.

*   Use American English.

*   Prefer third person and passive voice.

    *   Avoid first or second person pronouns,
        but these are acceptable for "meta" contexts
        like in contribution related-documentation
        (such as this file).

*   Use lead-in sentences before code blocks.

*   Do not provide untested examples or bad examples.

    *   Even when warned against using them,
        readers may end up using bad examples.

*   Do not repeat material, cross-reference it.

    *   Be aware that every line of documentation adds maintenance burden.

## Formatting

### Markdown flavor

Zensical uses Python Markdown with additional extensions.
Use Markdown as much as possible;
limit usage of HTML to cases that can't be handled with Markdown.

???+ example

    When rendering command syntax,
    it's desirable to indicate which parts are arguments.

    For example, to render bprc(5)'s `a induct` command like:

    <code>
        a induct
        <i>protocol_name</i>
        <i>duct_name</i>
        <i>cli_command</i>
    </code>

    Write:

    ```html
    <code>
        a induct
        <i>protocol_name</i>
        <i>duct_name</i>
        <i>cli_command</i>
    </code>
    ```

That is, use `**word**` or `__word__` to place strong importance
on "word"
instead of using the `<strong>` HTML element.

### Semantic Line Breaks

Documentation files should follow [Semantic Line Breaks](https://sembr.org/).
Respect the recommended maximum line length of 80 characters;
exceeding this limit is acceptable
for links
and when necessary to achieve a particular rendered output
(tables, headings, code blocks, etc.).

### Special characters

Do not use raw em dashes, raw en dashes, or raw ellipsis.

The following table shows how these symbols should be written
in files:

Name     | Syntax | Rendered
-------- | ------ | --------
Em dash  | `---`  | ---
En dash  | `--`   | --
Ellipsis | `...`  | ...

Do not use curved quotation marks or curved apostrophes.

### Use of newlines

There should be an empty line above and below section headings
and around paragraph-breaking elements.

### Headings

Never use a level 1 section heading (`#`) more than once.
This must be the title of the document.

Use sentence case
(usually only the first word and proper names are capitalized)
for page titles and section headings.

### Lists

#### Unordered list markers

Asterisks, pluses, and hyphens are all acceptable as list markers.
If unsure, prefer asterisks.

#### Long ordered lists

For long ordered lists that may change,
use lazy list numbering:

```md
1.  Lorem ipsum dolor sit amet
1.  Consectetur adipiscing elit
    1.  Sed do eiusmod tempor incididunt ut labore
    1.  et dolore magna aliqua
1.  Ut enim ad minim veniam
```

For small ordered lists that are unlikely to change,
prefer using the correct ordinal numbers:

```md
1.  Foo
2.  Bar
3.  Baz
```

#### Spacing

Most lists should use a 4-space indent:

```md
1.  Ordered list starting with "1." has 2 spaces.
    Multiple lines for an item are indented with 4 spaces.
2.  Next list item.

*   Unordered list has 3 spaces.
    *   Nested lists are indented to the next level.
```

A newline between list items is allowed
if the writer finds it more readable.

???+ danger

    Indent nested lists with 4 spaces, not 2.
    Python Markdown requires 4 spaces for nested content,
    but many other flavors support 2 spaces for nested content.
    Yet another reason to [evaluate the rendered output
    in a browser](#editing-environment).

    === "2 spaces"

        <div class="grid" markdown>

        ```md
        * Lorem ipsum dolor sit amet
          * Consectetur adipiscing elit
        * Sed do eiusmod tempor incididunt ut labore
          et dolore magna aliqua
        ```

        * Lorem ipsum dolor sit amet
          * Consectetur adipiscing elit
        * Sed do eiusmod tempor incididunt ut labore
          et dolore magna aliqua

        </div>

    === "4 spaces"

        <div class="grid" markdown>

        ```md
        *   Lorem ipsum dolor sit amet
            *   Consectetur adipiscing elit
        *   Sed do eiusmod tempor incididunt ut labore
            et dolore magna aliqua
        ```

        *   Lorem ipsum dolor sit amet
            *   Consectetur adipiscing elit
        *   Sed do eiusmod tempor incididunt ut labore
            et dolore magna aliqua

        </div>

Small, unnested lists which are unlikely to grow
do not need to be indented with 4 spaces,
and can be indented with just 1 space:

```md
* Foo
* Bar
* Baz
```

### Links

The following section about links
is reproduced from [Google's Markdown style guide](https://google.github.io/styleguide/docguide/style.html#links)
(licensed under [CC-BY-3.0](https://creativecommons.org/licenses/by/3.0/)).

<!--
SPDX-SnippetBegin
SPDX-License-Identifier: CC-BY-3.0
This is taken from
https://github.com/google/styleguide/blob/1809c769de31ba388c755ad15dd057a9ba8531fd/docguide/style.md
with the following modifications:
* Increase heading levels to fit our document.
* Remove the U+FEFF workaround
  since reference links
  do not render within Zensical's code blocks.
* Rewrite Markdown examples to use SemBr.
* Reword link-shortening guideline to forbid URL shorteners.
* Omit "Use explicit paths for links within Markdown"
  and "Avoid relative paths unless within the same directory" sections
  since that conflicts with how Zensical handles links.
* Rename "Reference" section to "Reference links"
  since that seems to be the intended title.
* Fix apparent typo in "Use reference links for long links"
  which uses a raw URL instead of an inline link in a code block.
  This section also uses a shorter example link
  to respect our 80 character limit.
-->

Long links make source Markdown difficult to read and break the 80 character
wrapping.
Wherever possible, keep link URLs concise,
but do not use URL shorteners.

#### Use informative Markdown link titles

Markdown link syntax allows you to set a link title. Use it wisely. Users often
do not read documents; they scan them.

Links catch the eye. But titling your links "here," "link," or simply
duplicating the target URL tells the hasty reader precisely nothing and is a
waste of space:

```markdown
DO NOT DO THIS.

See the Markdown guide for more info: [link](markdown.md),
or check out the style guide [here](style.md).

Check out a typical test result:
[https://example.com/foo/bar](https://example.com/foo/bar).
```

Instead, write the sentence naturally, then go back and wrap the most
appropriate phrase with the link:

```markdown
See the [Markdown guide](markdown.md) for more info,
or check out the [style guide](style.md).

Check out a
[typical test result](https://example.com/foo/bar).
```

#### Reference links

For long links or image URLs, you may want to split the link use from the link
definition, like this:

```markdown
See the [Markdown style guide][style],
which has suggestions for making docs more readable.

[style]: http://Markdown/corp/Markdown/docs/reference/style.md
```

##### Use reference links for long links

Use reference links where the length of the link would detract from the
readability of the surrounding text if it were inlined. Reference links make it
harder to see the destination of a link in source text, and add additional
syntax.

In this example, reference link usage is not appropriate, because the link is
not long enough to disrupt the flow of the text:

```markdown
DO NOT DO THIS.

The [style guide][style_guide] says not to use reference links
unless you have to.

[style_guide]: https://google.com/md-style
```

Just inline it instead:

```markdown
The [style guide](https://google.com/md-style) says not to use reference links
unless you have to.
```

In this example, the link destination is long enough that it makes sense to use
a reference link:

```markdown
The [style guide] says not to use reference links
unless you have to.

[style guide]: https://docs.google.com/document/d/13HQBxfhCwx8lVRuN2Wf6poqvAfVeEXmFVcawP5I6B3c/edit
```

Use reference links more often in tables. It is particularly important to keep
table content short, since Markdown does not provide a facility to break text in
cell tables across multiple lines, and smaller tables are more readable.

For example, this table's readability is worsened by inline links:

```markdown
DO NOT DO THIS.

Site                                                             | Description
---------------------------------------------------------------- | -----------------------
[site 1](http://google.com/excessively/long/path/example_site_1) | This is example site 1.
[site 2](http://google.com/excessively/long/path/example_site_2) | This is example site 2.
```

Instead, use reference links to keep the line length manageable:

```markdown
Site     | Description
-------- | -----------------------
[site 1] | This is example site 1.
[site 2] | This is example site 2.

[site 1]: http://google.com/excessively/long/path/example_site_1
[site 2]: http://google.com/excessively/long/path/example_site_2
```

##### Use reference links to reduce duplication

Consider using reference links when referencing the same link destination
multiple times in a document, to reduce duplication.

##### Define reference links after their first use

We recommend putting reference link definitions just before the next heading, at
the end of the section in which they're first used. If your editor has its own
opinion about where they should go, don't fight it; the tools always win.

We define a "section" as all text between two headings. Think of reference links
like footnotes, and the current section like the current page.

This arrangement makes it easy to find the link destination in source view,
while keeping the flow of text free from clutter. In long documents with lots of
reference links, it also prevents "footnote overload" at the bottom of the file,
which makes it difficult to pick out the relevant link destination.

There is one exception to this rule: reference link definitions that are used in
multiple sections should go at the end of the document. This avoids dangling
links when a section is updated or moved.

In the following example, the reference definition is far from its initial use,
which makes the document harder to read:

```markdown
# Header FOR A BAD DOCUMENT

Some text with a [link][link_def].

Some more text with the same [link][link_def].

## Header 2

... lots of text ...

## Header 3

Some more text using a [different_link][different_link_def].

[link_def]: http://reallyreallyreallylonglink.com
[different_link_def]: http://differentreallyreallylonglink.com
```

Instead, put it just before the header following its first use:

```markdown
# Header

Some text with a [link][link_def].

Some more text with the same [link][link_def].

[link_def]: http://reallyreallyreallylonglink.com

## Header 2

... lots of text ...

## Header 3

Some more text using a [different_link][different_link_def].

[different_link_def]: http://differentreallyreallylonglink.com
```

<!-- SPDX-SnippetEnd -->

### Table of contents

Do not manually write a table of contents.
And although the Table of Contents extension is enabled,
do not add a `[TOC]` marker in the document.
The table of contents that Zensical renders in the navigation sidebar
is sufficient for our use case.

### Code

Syntax higlighting for code blocks comes from [Pygments](https://pygments.org).
Declare the language based on Pygments' [languages](https://pygments.org/languages/).

Code should abide by ION's [developer guidelines](./development-guide.md).

!!! note

    The restriction against over-commenting is more lax in documentation,
    though consider using [code annotations](https://zensical.org/docs/authoring/code-blocks/#code-annotations).

When representing the name of a path or directory,
end it with a `/` or explicitly add an appositive like "directory".

!!! example

    "Use the `runtests` script in `tests/` for performing tests."

    or

    "Use the `runtests` script in the `tests` directory for performing tests."

    **NOT**

    "Use the `runtests` script in `tests` for performing tests."

#### Shell

Shell session code blocks use `console` or `shell-session` as the language,
not `sh`.
Using the correct language is required
for the copy-to-clipboard button to work properly.

???+ example

    The copy-to-clipboard button
    will not copy the shell prompt and command output for `console`,
    but all contents of the code block are copied for `sh`.

    === "`console`"

        ```console
        $ foo -a -b b_arg -m m_arg req1 req2 \
          opt1 opt2
        Lorem ipsum dolor sit amet, consectetur adipiscing elit,
        sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
        ```

    === "`sh`"

        ```sh
        $ foo -a -b b_arg -m m_arg req1 req2 \
          opt1 opt2
        Lorem ipsum dolor sit amet, consectetur adipiscing elit,
        sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
        ```

Use `$` as the shell prompt
for commands that a non-root user can execute.

Use `#` as the shell prompt
for commands that require elevated privileges.

Prefer using:

```console
# command
```

instead of writing:

```console
$ sudo command
```

Not all systems that ION targets
have `sudo` installed by default;
avoid using `sudo` in documentation
to improve the generalizability of the docs.

To show running a command as another user,
prefix the shell prompt with the username in square brackets:

```console
[alice]$ command
```

The [shell guidelines from the development guide](./development-guide.md#shell-guidelines)
apply to shell sessions in documentation as well.
Try to be as shell-neutral as possible (do not assume Bash)
unless writing shell-specific explanations,
and do not assume features exclusive to GNU coreutils are always available.

### Comments

Prefer the following format for comments
that should not appear in HTML:

```md
[Lorem ipsum dolor sit amet, consectetur adipiscing elit,
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
]::
```

Most comments should _not_ appear in HTML.
But for those that should, use regular HTML comments:

```md
<!--
Lorem ipsum dolor sit amet, consectetur adipiscing elit,
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
-->
```

## See also

The following resources inspired parts of this document:

*   [Gentoo Wiki:Guidelines](https://wiki.gentoo.org/wiki/Gentoo_Wiki:Guidelines)
*   [ArchWiki Help:Style](https://wiki.archlinux.org/title/Help:Style)
*   [FreeBSD Documentation Project Primer](https://docs.freebsd.org/en/books/fdp-primer/)
*   [Wikipedia:Manual of Style](https://en.wikipedia.org/wiki/Wikipedia:Manual_of_Style)
*   [Google Markdown style guide](https://google.github.io/styleguide/docguide/style.html)
