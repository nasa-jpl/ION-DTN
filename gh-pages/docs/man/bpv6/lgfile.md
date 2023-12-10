# NAME

lgfile - ION Load/Go source file

# DESCRIPTION

The ION Load/Go system enables the execution of ION administrative programs
at remote nodes:

> The **lgsend** program reads a Load/Go source file from a local file system,
> encapsulates the text of that source file in a bundle, and sends the bundle
> to a designated DTN endpoint on the remote node.
>
> An **lgagent** task running on the remote node, which has opened that DTN
> endpoint for bundle reception, receives the extracted payload of the bundle
> \-- the text of the Load/Go source file -- and processes it.

Load/Go source file content is limited to newline-terminated lines of ASCII
characters.  More specifically, the text of any Load/Go source file is a
sequence of _line sets_ of two types: _file capsules_ and _directives_.
Any Load/Go source file may contain any number of file capsules and any
number of directives, freely intermingled in any order, but the typical
structure of a Load/Go source file is simply a single file capsule
followed by a single directive.

Each _file capsule_ is structured as a single start-of-capsule line, followed
by zero or more capsule text lines, followed by a single end-of-capsule
line.  Each start-of-capsule line is of this form:

> \[_file\_name_

Each capsule text line can be any line of ASCII text that does not begin
with an opening (\[) or closing (\]) bracket character.

A text line that begins with a closing bracket character (\]) is interpreted
as an end-of-capsule line.

A _directive_ is any line of text that is not one of the lines of a file
capsule and that is of this form:

> !_directive\_text_

When **lgagent** identifies a file capsule, it copies all of the capsule's
text lines to a new file named _file\_name_ that it creates in the current
working directory.  When **lgagent** identifies a directive, it executes
the directive by passing _directive\_text_ to the pseudoshell() function
(see platform(3)).  **lgagent** processes the line sets of a Load/Go source
file in the order in which they appear in the file, so the _directive\_text_
of a directive may reference a file that was created as the result of
processing a prior file capsule line set in the same source file.

Note that lgfile directives are passed to pseudoshell(), which on a VxWorks
platform will always spawn a new task; the first argument in _directive\_text_
must be a symbol that VxWorks can resolve to a function, not a shell
command.  Also note that the arguments in _directive\_text_ will be actual
task arguments, not shell command-line arguments, so they should never be
enclosed in double-quote characters (").  However, any argument that
contains embedded whitespace must be enclosed in single-quote characters (')
so that pseudoshell() can parse it correctly.

# EXAMPLES

Presenting the following lines of source file text to **lgsend**:

> \[cmd33.bprc
>
> x protocol ltp
>
> \]
>
> !bpadmin cmd33.bprc

should cause the receiving node to halt the operation of the LTP
convergence-layer protocol.

# SEE ALSO

lgsend(1), lgagent(1), platform(3)
