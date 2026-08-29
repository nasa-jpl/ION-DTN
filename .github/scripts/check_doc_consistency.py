#!/usr/bin/env python3
"""check_doc_consistency.py - POD documentation vs.implementation
checker for ION-DTN.

This tool detects drift between the POD man-page sources
(doc/pod{1,3,5}/*.pod) and the C implementation. It encodes the
conventions ION uses so the comparison can run unattended (e.g. in CI).

Checks performed
----------------
1. admin_commands   (pod5 <-> admin .c)
   Top-level command-letter parity. Documented commands appear as
   `=item B<X ...>` in the *rc.pod file; implemented commands are the
   labels of the main dispatch switch (`switch (*(tokens[0]))`, or
   `switch (cmdCode[0])` in bpsecadmin). The command "letter" is the
   first character inside `B<...>`. This catches both undocumented
   commands (in code, not in doc) and documented commands that are not
   implemented (in doc, not in code - e.g. an advertised echo command
   with no handler).

2. manage_subcommands  (pod5 <-> admin .c)
   `m` sub-command parity: `=item B<m foo>` vs
   `strcmp(tokens[1], "foo")` inside the `executeManage` function.

3. pod3_api  (pod3 <-> header(s))
   API function parity. Documented functions are the identifier just
   before the first `(` on an `=item` line; implemented functions are
   the names declared by `extern` prototypes or `#define name(...)`
   macros in the configured  header(s).

4. pod1_synopsis  (pod1 structure)
   Every section-1 man page must contain a `=head1 SYNOPSIS` block.

5. cross_references  (pod <-> pod)
   Every `name(section)` man-page reference (e.g. in SEE ALSO) must
   resolve to a pod in the repo at that section, or be a known
   external/system man page (EXTERNAL_MANPAGES). Catches broken refs
   (no such page) and wrong-section refs (page exists but in a different
   section).

6. admin_rc_pairing  (pod1 admin page <-> its own rc file)
   An admin program page `<X>admin.pod` reads its own config file
   `<X>rc` (strip "admin", append "rc": ionsecadmin->ionsecrc,
   bpadmin->bprc, ...). The dos2unix DIAGNOSTICS boilerplate ("If you
   edit the <name> file ... before presenting it to <X>admin") must
   therefore name `<X>rc`; any other  name is a copy-paste slip (e.g.
   the security admins inheriting "ionrc" from ionadmin).

7. diagnostics  (pod1 DIAGNOSTICS <-> source error strings)
   Each `=item` under a pod1 page's `=head1 DIAGNOSTICS` should
   correspond to an error string emitted by the program's source
   (`<name>.c`). For every documented diagnostic we look for the source
   string literal that best covers its words (program-name prefix and
   I<...> placeholders removed); if the best coverage is below
   DIAG_MATCH_THRESHOLD the diagnostic is flagged as not found in the
   source. Catches stale/renamed messages (e.g. lgsend's "fgets failed"
   where the code emits "igets failed") and entirely fictional
   diagnostics (e.g. lgagent's "can't handle bundle delivery", which no
   code path emits). Only the pod->source direction is checked; the
   reverse (undocumented messages) is left alone because ION pods
   intentionally summarize rather than list every errmsg.

POD / source conventions encoded here
-------------------------------------
* Commands are documented as `=item B<X...>`; the command code is the
  first char. Descriptive `=item B<Capitalized Word>` headings
  (e.g. biberc's `B<Plan>`) are NOT commands and are ignored (command
  codes are a single lowercase letter, digit, or punctuation symbol
  followed by whitespace or `>`).
* "Watch" sub-characters (a/b/c/y/z/~/!/# ... under the `w` command) and
  echo states (`0`/`1` under `e`) live in NESTED switches, so
  brace-depth tracking of the main dispatch switch excludes them
  automatically - they are not top-level commands and are not flagged.
* Some public functions are exposed as macros
  (`#define sdr_malloc(...)`) and are split across several headers
  (e.g. cfdp.h + cfdpops.h, or the sdr*.h family); pod3 entries
  therefore map to a *list* of headers.
* C string/char literals and comments are skipped while scanning so
  braces and `case` labels inside them never confuse the parser.

Baseline
--------
Known, accepted discrepancies are stored in
.github/scripts/doc_consistency_baseline.json. The check fails (exit 1)
only on findings that are NOT in the baseline, so CI catches *new* drift
while tolerating the pre-existing gaps. Regenerate after an intentional
change with `--update-baseline`.

Usage
-----
    python3 check_doc_consistency.py            # check (CI mode)
    python3 check_doc_consistency.py --all      # list every finding
    python3 check_doc_consistency.py --check diag   # one check only
    python3 check_doc_consistency.py --update-baseline
    python3 check_doc_consistency.py --root /path/to/ion

The unit tests for this script are in
tests/test_check_doc_consistency.py
(`python3 tests/test_check_doc_consistency.py`, or
via unittest discovery).
"""

import argparse
import json
import os
import re
import sys

# -------------------------------------------------------------------- #
# Configuration                                                        #
# -------------------------------------------------------------------- #

# pod5 admin config-reference  <->  admin source
ADMIN_PAIRS = [
    ("ici/doc/pod5/ionrc.pod", "ici/utils/ionadmin.c"),
    ("ici/doc/pod5/ionsecrc.pod", "ici/utils/ionsecadmin.c"),
    ("bpv7/doc/pod5/bprc.pod", "bpv7/utils/bpadmin.c"),
    ("bpv7/doc/pod5/bpsecrc.pod", "bpv7/utils/bpsecadmin.c"),
    ("bpv7/doc/pod5/ipnrc.pod", "bpv7/ipn/ipnadmin.c"),
    ("bpv7/doc/pod5/dtn2rc.pod", "bpv7/dtn2/dtn2admin.c"),
    ("bpv7/doc/pod5/biberc.pod", "bpv7/bibe/bibeadmin.c"),
    ("ltp/doc/pod5/ltprc.pod", "ltp/utils/ltpadmin.c"),
    ("ltp/doc/pod5/ltpsecrc.pod", "ltp/utils/ltpsecadmin.c"),
    ("cfdp/doc/pod5/cfdprc.pod", "cfdp/utils/cfdpadmin.c"),
    ("dtpc/doc/pod5/dtpcrc.pod", "dtpc/utils/dtpcadmin.c"),
]

# pod3 API reference  ->  header(s) declaring the documented functions
POD3_API = {
    "ici/doc/pod3/ion.pod": ["ici/include/ion.h"],
    "ici/doc/pod3/zco.pod": ["ici/include/zco.h"],
    "ici/doc/pod3/psm.pod": ["ici/include/psm.h"],
    "ici/doc/pod3/lyst.pod": ["ici/include/lyst.h"],
    "ici/doc/pod3/llcv.pod": ["ici/include/llcv.h"],
    "ici/doc/pod3/smlist.pod": ["ici/include/smlist.h"],
    "ici/doc/pod3/smrbt.pod": ["ici/include/smrbt.h"],
    "ici/doc/pod3/memmgr.pod": ["ici/include/memmgr.h"],
    "ici/doc/pod3/sdr.pod": [
        "ici/include/sdr.h",
        "ici/include/sdrxn.h",
        "ici/include/sdrmgt.h",
    ],
    "ici/doc/pod3/sdrlist.pod": ["ici/include/sdrlist.h"],
    "ici/doc/pod3/sdrtable.pod": ["ici/include/sdrtable.h"],
    "ici/doc/pod3/sdrstring.pod": ["ici/include/sdrstring.h"],
    "ici/doc/pod3/sdrhash.pod": ["ici/include/sdrhash.h"],
    "ltp/doc/pod3/ltp.pod": ["ltp/include/ltp.h"],
    "ltp/doc/pod3/sda.pod": ["ltp/include/sda.h"],
    "bpv7/doc/pod3/bp.pod": ["bpv7/include/bp.h"],
    "cfdp/doc/pod3/cfdp.pod": [
        "cfdp/include/cfdp.h",
        "cfdp/include/cfdpops.h",
    ],
    "dgr/doc/pod3/dgr.pod": ["dgr/include/dgr.h"],
    "dtpc/doc/pod3/dtpc.pod": ["dtpc/include/dtpc.h"],
    "bss/doc/pod3/bss.pod": ["bss/include/bss.h"],
}

# pod1 man pages to skip in the SYNOPSIS structure check
# (handled elsewhere).
POD1_SYNOPSIS_SKIP = {"bpsh", "bpshd"}

# System / third-party man pages legitimately referenced by ION pods
# (i.e. not expected to have a pod in this repo). "name(section)" form.
# The cross-reference check flags any name(section) reference that
# neither resolves to a repo pod nor appears here.
EXTERNAL_MANPAGES = {
    # libc / POSIX
    "creat(2)",
    "open(2)",
    "socket(2)",
    "time(2)",
    "getcwd(3)",
    "getrandom(2)",
    "strcat(3)",
    "strcmp(3)",
    "strcpy(3)",
    "strlen(3)",
    "string(3)",
    "arc4random_buf(3)",
    # platform-specific RNG / crypto
    "BCryptGenRandom(3)",
    "SecRandomCopyBytes(3)",
    # common CLI tools / other sections
    "base64(1)",
    "hostname(1)",
    "time(1)",
    "dot(1)",
    "ping(8)",
    "random(4)",
}

# ION modules that exist in the source tree but ship no POD man page. A
# cross-reference to one of these is intentional, not broken, so it is
# not flagged (until/unless a man page is written for it).
INTERNAL_NO_MANPAGE = {
    "sptrace(3)",  # ici/library/sptrace.c, ici/include/sptrace.h
}

BASELINE_FILE = ".github/scripts/doc_consistency_baseline.json"

DISPATCH_SWITCH_RE = re.compile(
    r"switch\s*\(\s*\*?\(?\s*(?:tokens\[0\]|cmdCode\[0\]|line\[0\])\s*\)?\s*\)"
)


# -------------------------------------------------------------------- #
# Low-level C scanning (literal/comment aware)                         #
# -------------------------------------------------------------------- #


def _block_after(text, open_idx):
    """Given the index of a '{', return (start, end) spanning the
    balanced block, skipping braces inside strings, char literals and
    comments. `end` is the index just past the matching '}'.
    Returns None on imbalance.
    """
    i = open_idx
    n = len(text)
    depth = 0
    while i < n:
        c = text[i]
        two = text[i : i + 2]
        if two == "//":
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        if two == "/*":
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '"' or c == "'":
            i = _skip_literal(text, i)
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return (open_idx, i + 1)
        i += 1
    return None


def _skip_literal(text, i):
    """`i` points at an opening quote; return index just past the
    closing quote.
    """
    quote = text[i]
    i += 1
    n = len(text)
    while i < n:
        if text[i] == "\\":
            i += 2
            continue
        if text[i] == quote:
            return i + 1
        i += 1
    return n


def top_level_case_letters(src):
    """Return the set of `case 'X':` letters that sit directly inside a
    command dispatch switch (brace depth 1), ignoring nested switches
    (watch chars, echo states), strings and comments. Some admins
    dispatch in two stages (e.g. bpsecadmin maps the code to an enum in
    one switch and acts on it in another), so the result is the union
    over every `switch (*(tokens[0]))`-style switch.
    """
    letters = set()
    for m in DISPATCH_SWITCH_RE.finditer(src):
        open_idx = src.find("{", m.end())
        if open_idx < 0:
            continue
        block = _block_after(src, open_idx)
        if block:
            letters |= _cases_at_depth1(src, block)
    return letters


def _cases_at_depth1(src, block):
    start, end = block
    letters = set()
    i = start
    depth = 0
    last_word = ""
    cur = ""
    while i < end:
        c = src[i]
        two = src[i : i + 2]
        if two == "//":
            j = src.find("\n", i)
            i = end if j < 0 else min(j, end)
            continue
        if two == "/*":
            j = src.find("*/", i + 2)
            i = end if j < 0 else min(j + 2, end)
            continue
        if c == '"':
            i = _skip_literal(src, i)
            cur = ""
            continue
        if c == "'":
            # char literal; record as a case label if at depth 1 after
            # `case`
            nxt = _skip_literal(src, i)
            literal = src[i + 1 : nxt - 1]
            if last_word == "case" and depth == 1 and literal:
                # decode a couple of common escapes; command codes are
                # printable
                val = literal
                if val.startswith("\\") and len(val) >= 2:
                    val = {
                        "\\n": "\n",
                        "\\t": "\t",
                        "\\\\": "\\",
                        "\\'": "'",
                        "\\0": "\0",
                    }.get(val, val[1:])
                letters.add(val)
            cur = ""
            i = nxt
            continue
        if c == "{":
            depth += 1
            cur = ""
        elif c == "}":
            depth -= 1
            cur = ""
        elif c.isalnum() or c == "_":
            cur += c
        else:
            if cur:
                last_word = cur
                cur = ""
        i += 1
    return letters


def manage_subcommands_in_src(src):
    """Return the set of `m` sub-commands: strings passed as the second
    arg of strcmp(tokens[1], "...") inside the executeManage function.
    """
    m = re.search(r"\bexecuteManage\s*\([^)]*\)", src)
    if not m:
        return set()
    open_idx = src.find("{", m.end())
    if open_idx < 0:
        return set()
    block = _block_after(src, open_idx)
    if not block:
        return set()
    body = src[block[0] : block[1]]
    return set(
        re.findall(r'strcmp\(\s*tokens\[1\]\s*,\s*"([^"]+)"\s*\)', body)
    )


def _strip_c_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.DOTALL)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


# C keywords that can appear as `word(` but are not function names.
_C_KEYWORDS = {
    "if",
    "for",
    "while",
    "switch",
    "sizeof",
    "return",
    "do",
    "else",
    "case",
    "defined",
    "typedef",
}


def header_functions(text):
    """Return the set of function names declared in a header:
    function-like `#define name(...)` macros plus C prototypes (the
    identifier just before the first '(' of each ';'-terminated
    declaration). Comments are stripped first so banner text (e.g.
    "Copyright (c)") is not mistaken for a declaration. ION headers
    declare some functions with `extern` and some without, and expose
    others only as macros, so all three forms are covered.
    """
    text = _strip_c_comments(text)
    names = set()
    # function-like macros: name immediately followed by '('
    # (no space, per C)
    for m in re.finditer(
        r"^\s*#\s*define\s+([A-Za-z_]\w*)\(", text, re.MULTILINE
    ):
        names.add(m.group(1))
    # drop every preprocessor directive (including '\'-continued macro
    # bodies) so macro bodies don't bleed into the following declaration
    code = re.sub(r"(?m)^[ \t]*#(?:[^\n]*\\\n)*[^\n]*$", " ", text)
    # prototypes: per ';'-terminated chunk, the identifier before the
    # first '('
    for chunk in code.split(";"):
        if "(" not in chunk:
            continue
        fm = re.search(r"([A-Za-z_]\w*)\s*\(", chunk)
        if fm and fm.group(1) not in _C_KEYWORDS:
            names.add(fm.group(1))
    return names


def namespace_prefixes(names):
    """Derive the module namespace prefixes from a set of documented
    function names, used to filter the 'declared but undocumented'
    direction down to the pod's own namespace (e.g. bp_, sdr_list..., or
    the camelCase 'ion' run).
    """
    prefixes = set()
    for n in names:
        if "_" in n:
            prefixes.add(n.split("_", 1)[0] + "_")
        else:
            m = re.match(r"[a-z]+", n)
            if m:
                prefixes.add(m.group(0))
    return prefixes


# -------------------------------------------------------------------- #
# POD parsing                                                          #
# -------------------------------------------------------------------- #

# A command item: =item B<X...> where X is a single command code.
POD_CMD_RE = re.compile(
    r"^=item\s+B<\s*([a-z0-9?#@!^~&$])(?:\s|>|$)", re.MULTILINE
)
POD_M_SUB_RE = re.compile(
    r"^=item\s+B<m\s+([a-z][a-z0-9_-]*)>", re.MULTILINE
)


def pod_command_letters(text):
    return set(POD_CMD_RE.findall(text))


def pod_manage_subcommands(text):
    return set(POD_M_SUB_RE.findall(text))


def pod_api_functions(text):
    """Function names documented in a pod3 file: the identifier just
    before the first '(' of each `=item` that looks like a C prototype.
    A prototype's parentheses balance and the (possibly multi-line) item
    ends in ')', e.g.
    `=item int bssRun(char *bssName, ...\\n   char *buffer, ...)`.
    Prose items such as `=item Bundle Status Report (BSR) messages`
    end in text, not ')', and are ignored.
    """
    names = set()
    lines = text.splitlines()
    i, n = 0, len(lines)
    while i < n:
        line = lines[i]
        if line.startswith("=item") and "(" in line:
            buf, j = line, i
            while buf.count("(") > buf.count(")") and j + 1 < n:
                j += 1
                buf += " " + lines[j].strip()
            if buf.rstrip().endswith(")"):
                head = buf.split("(", 1)[0]
                # a prototype has no space between name and '(' (unlike
                # prose such as "... SDR application (VxWorks)")
                fm = re.search(r"([A-Za-z_]\w*)$", head)
                if fm:
                    names.add(fm.group(1))
            i = j + 1
            continue
        i += 1
    return names


def pod_has_synopsis(text):
    return "=head1 SYNOPSIS" in text


# A man-page cross-reference: name(section), e.g. "ipnadmin(1)" or
# "bp(3)". The name must abut '(' (so prose like "step (1)" or
# "(2 hours)" is not matched).
CROSSREF_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_.]*)\((\d)\)")


def pod_cross_references(text):
    """Return the set of (name, section) man-page cross-references in a
    pod, after stripping B<>/I<> formatting wrappers and POD verbatim
    (indented code) paragraphs, in which "func(2)" is a call argument,
    not a man ref.
    """
    text = re.sub(r"(?m)^[ \t]+.*$", "", text)
    text = re.sub(r"[A-Z]<([^>]*)>", r"\1", text)
    return set(CROSSREF_RE.findall(text))


# The dos2unix DIAGNOSTICS boilerplate in admin man pages: "... If you
# edit the <name> file on a Windows machine ...". The named file is the
# program's own input config file.
CONFIG_FILE_NOTE_RE = re.compile(
    r"edit\s+the\s+([\w.-]+)\s+file\s+on\s+a\s+Windows\s+machine",
    re.IGNORECASE,
)


def admin_config_file_refs(text):
    """Return the set of config-file names named in an admin page's
    dos2unix 'edit the <name> file' DIAGNOSTICS boilerplate
    (B<>/I<> wrappers stripped).
    """
    text = re.sub(r"[A-Z]<([^>]*)>", r"\1", text)
    return set(CONFIG_FILE_NOTE_RE.findall(text))


# Fraction of a documented diagnostic's words that must be covered by a
# single source string literal for it to count as "found in the source".
DIAG_MATCH_THRESHOLD = 0.6


def pod_diagnostics(text):
    """Return the raw text of each `=item` under a pod's DIAGNOSTICS
    section. Items that are wholly a single B<...> bold span are
    section/category headings (e.g. bptracker's "B<Normal operation>"),
    not message strings, and are skipped.
    """
    m = re.search(
        r"(?ms)^=head1[ \t]+DIAGNOSTICS\b(.*?)(?=^=head1[ \t]|\Z)", text
    )
    if not m:
        return []
    items = re.findall(r"(?m)^=item[ \t]+(.*\S)[ \t]*$", m.group(1))
    return [
        d for d in items if not re.fullmatch(r"B<[^>]*>", d.strip())
    ]


def source_error_strings(text):
    """Return the set of C string literals in a source file, joining
    backslash-newline continuations so multi-line errmsgs stay intact.
    """
    text = _strip_c_comments(text)
    text = text.replace("\\\n", "")  # join line continuations
    out = set()
    for s in re.findall(r'"((?:[^"\\]|\\.)*)"', text):
        out.add(
            re.sub(r"\\.", " ", s)
        )  # collapse \n, \t, \" ... to space
    return out


def _words(s):
    """Lower-cased word list; apostrophes are dropped first so
    contractions ("can't") stay a single token rather than splitting
    into "can" + "t".
    """
    s = s.lower().replace("'", "").replace("’", "")
    return re.findall(r"[a-z0-9]+", s)


def _diag_tokens(s, progname):
    """Word set of a diagnostic, with I<...> placeholders and the
    leading program-name token removed.
    """
    s = re.sub(r"I<[^>]*>", " ", s)  # drop variable placeholders
    s = re.sub(
        r"[A-Z]<([^>]*)>", r"\1", s
    )  # unwrap other B<>/C<> wrappers
    return {t for t in _words(s) if t != progname}


def diagnostic_is_documented(diag_tokens, src_token_sets):
    """True if some source string covers at least DIAG_MATCH_THRESHOLD
    of the diagnostic's words.
    """
    if not diag_tokens:
        return True  # nothing distinctive to match
    best = max(
        (len(diag_tokens & s) for s in src_token_sets), default=0
    )
    return best / len(diag_tokens) >= DIAG_MATCH_THRESHOLD


# -------------------------------------------------------------------- #
# Finding builders (pure: parsed input in, finding strings out)        #
# -------------------------------------------------------------------- #


def admin_command_findings(pod, ptext, stext):
    """Command-letter parity findings for one pod5/admin-source pair."""
    doc, code = (
        pod_command_letters(ptext),
        top_level_case_letters(stext),
    )
    return [
        f"[admin] {pod}: command '{c}' implemented but undocumented"
        for c in sorted(code - doc)
    ] + [
        f"[admin] {pod}: command '{c}' documented but not implemented"
        for c in sorted(doc - code)
    ]


def manage_subcommand_findings(pod, ptext, stext):
    """`m` sub-command parity findings for a pod5/admin-source pair."""
    doc, code = (
        pod_manage_subcommands(ptext),
        manage_subcommands_in_src(stext),
    )
    return [
        f"[manage] {pod}: 'm {s}' implemented but undocumented"
        for s in sorted(code - doc)
    ] + [
        f"[manage] {pod}: 'm {s}' documented but not implemented"
        for s in sorted(doc - code)
    ]


def api_findings(pod, documented, declared):
    """Function parity findings for one pod3 page. The 'declared but
    undocumented' direction is limited to the pod's own namespace and
    skips ALL-CAPS names (constants/macros).
    """
    findings = [
        f"[api] {pod}: '{f}' documented but not declared in header(s)"
        for f in sorted(documented - declared)
    ]
    prefixes = namespace_prefixes(documented)
    for f in sorted(declared - documented):
        if f == f.upper():
            continue
        if prefixes and not any(f.startswith(p) for p in prefixes):
            continue
        findings.append(
            f"[api] {pod}: '{f}' declared in header(s) but undocumented"
        )
    return findings


def cross_reference_findings(rel, refs, avail_ns, avail_name):
    """Findings for the (name, section) references of one pod, given the
    inventory of pods in the repo: `avail_ns` as (name, section) pairs
    and `avail_name` as bare page names.
    """
    findings = []
    for name, sec in sorted(refs):
        tag = f"{name}({sec})"
        if (
            (name, sec) in avail_ns
            or tag in EXTERNAL_MANPAGES
            or tag in INTERNAL_NO_MANPAGE
        ):
            continue
        if name in avail_name:
            findings.append(
                f"[xref] {rel}: '{tag}' refers to an existing page at the wrong section"
            )
        else:
            findings.append(
                f"[xref] {rel}: '{tag}' has no matching pod (broken cross-reference)"
            )
    return findings


def admin_rc_findings(rel, name, named_files):
    """Findings for an admin page whose dos2unix note names a config
    file other than the one the program actually reads
    (<name> minus "admin", plus "rc").
    """
    expected = name[: -len("admin")] + "rc"
    return [
        f"[adminrc] {rel}: config-file note names '{named}' but "
        f"{name} reads '{expected}'"
        for named in sorted(named_files)
        if named != expected
    ]


def diagnostics_findings(rel, name, diags, src_token_sets):
    """Findings for documented diagnostics with no matching source
    string.
    """
    return [
        f'[diag] {rel}: documented diagnostic not found in source: "{d}"'
        for d in diags
        if not diagnostic_is_documented(
            _diag_tokens(d, name), src_token_sets
        )
    ]


# -------------------------------------------------------------------- #
# Repository traversal                                                 #
# -------------------------------------------------------------------- #


def read(root, rel):
    path = os.path.join(root, rel)
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return fh.read()


def missing(root, rel):
    return not os.path.exists(os.path.join(root, rel))


def pod_files(root, marker):
    """Yield repo-relative paths of .pod files under directories whose
    path contains `marker` (e.g. "pod1", or "doc/pod" for every pod
    section), sorted within each directory for stable output.
    """
    marker = marker.replace("/", os.sep)
    for dirpath, _dirs, files in os.walk(root):
        if os.sep + marker not in dirpath + os.sep:
            continue
        for fn in sorted(files):
            if fn.endswith(".pod"):
                yield os.path.relpath(os.path.join(dirpath, fn), root)


def pod_inventory(pods):
    """Split pod paths into ((name, section) set, name set)."""
    avail_ns, avail_name = set(), set()
    for rel in pods:
        name = os.path.basename(rel)[:-4]
        avail_name.add(name)
        m = re.search(r"[\\/]pod(\d)[\\/]", rel)
        if m:
            avail_ns.add((name, m.group(1)))
    return avail_ns, avail_name


# Directories excluded from the bulk diagnostics corpus: test
# scaffolding (one test program's strings must not mask a sibling's bug)
# and non-native builds.
SKIP_DIRS = {"tests", "test", "arch-rtems", "build"}


def source_index(root):
    """Scan the tree once for .c files, returning (module -> [rel paths]
    without test scaffolding, basename -> [rel paths] including test
    programs).
    """
    module_c, prog_paths = {}, {}
    for dirpath, _dirs, files in os.walk(root):
        parts = set(dirpath.split(os.sep))
        if "arch-rtems" in parts or "build" in parts:
            continue
        reldir = os.path.relpath(dirpath, root)
        module = "" if reldir == "." else reldir.split(os.sep)[0]
        in_test = bool(SKIP_DIRS & parts)
        for fn in files:
            if not fn.endswith(".c"):
                continue
            rel = os.path.relpath(os.path.join(dirpath, fn), root)
            prog_paths.setdefault(fn[:-2], []).append(rel)
            if not in_test:
                module_c.setdefault(module, []).append(rel)
    return module_c, prog_paths


class SourceTokens:
    """Word sets of the C string literals of a module, cached per
    module. A program's diagnostics may be emitted by its own .c OR by
    library code it links, so a module's corpus is that module plus the
    shared ici/ foundation - NOT the global tree, which would mask
    genuine copy-paste bugs (an lgagent message that really lives in an
    unrelated module).
    """

    def __init__(self, root, module_c):
        self.root = root
        self.module_c = module_c
        self._cache = {}

    def paths(self, paths):
        sets = []
        for sc in paths:
            for lit in source_error_strings(read(self.root, sc)):
                sets.append(set(_words(lit)))
        return sets

    def module(self, module):
        if module not in self._cache:
            self._cache[module] = self.paths(
                set(
                    self.module_c.get(module, [])
                    + self.module_c.get("ici", [])
                )
            )
        return self._cache[module]


# -------------------------------------------------------------------- #
# Checks                                                               #
# -------------------------------------------------------------------- #


def check_admin_commands(root):
    """Checks 1 + 2: admin command and manage-subcommand parity."""
    findings = []
    for pod, src in ADMIN_PAIRS:
        if missing(root, pod) or missing(root, src):
            findings.append(f"[admin] MISSING FILE: {pod} or {src}")
            continue
        ptext, stext = read(root, pod), read(root, src)
        findings += admin_command_findings(pod, ptext, stext)
        findings += manage_subcommand_findings(pod, ptext, stext)
    return findings


def check_pod3_api(root):
    """Check 3: pod3 API parity against the declaring header(s)."""
    findings = []
    for pod, headers in sorted(POD3_API.items()):
        if missing(root, pod):
            findings.append(f"[api] MISSING FILE: {pod}")
            continue
        declared = set()
        for h in headers:
            if missing(root, h):
                findings.append(
                    f"[api] MISSING HEADER: {h} (for {pod})"
                )
                continue
            declared |= header_functions(read(root, h))
        findings += api_findings(
            pod, pod_api_functions(read(root, pod)), declared
        )
    return findings


def check_pod1_synopsis(root):
    """Check 4: every section-1 man page has a SYNOPSIS block."""
    findings = []
    for rel in pod_files(root, "pod1"):
        if os.path.basename(rel)[:-4] in POD1_SYNOPSIS_SKIP:
            continue
        if not pod_has_synopsis(read(root, rel)):
            findings.append(
                f"[pod1] {rel}: missing '=head1 SYNOPSIS' section"
            )
    return findings


def check_cross_references(root):
    """Check 5: every name(section) reference resolves to a pod or a
    known external man page.
    """
    pods = sorted(pod_files(root, os.path.join("doc", "pod")))
    avail_ns, avail_name = pod_inventory(pods)
    findings = []
    for rel in pods:
        findings += cross_reference_findings(
            rel,
            pod_cross_references(read(root, rel)),
            avail_ns,
            avail_name,
        )
    return findings


def check_admin_rc_pairing(root):
    """Check 6: an admin page's dos2unix note names that program's own
    rc file.
    """
    findings = []
    for rel in pod_files(root, "pod1"):
        name = os.path.basename(rel)[:-4]  # e.g. "ionsecadmin"
        if not name.endswith("admin"):
            continue
        findings += admin_rc_findings(
            rel, name, admin_config_file_refs(read(root, rel))
        )
    return findings


def check_diagnostics(root):
    """Check 7: pod1 DIAGNOSTICS entries vs the program's source
    strings. NOTE: this check is ONE-DIRECTIONAL (pod -> source). It
    verifies that each documented diagnostic corresponds to a real
    source string, catching fictional/stale/misattributed entries. It
    does NOT check completeness: source error strings
    (putErrmsg/putSysErrmsg/writeMemo) that are missing from the pod's
    DIAGNOSTICS section are NOT flagged. ION pods intentionally
    summarize ("...a variety of other diagnostics may also be
    reported."), so a strict source -> pod completeness check would be
    very noisy. TODO: optionally add a soft, report-only completeness
    mode that lists undocumented operator-facing source strings per
    program (advisory, not a CI failure) so a human can pick the
    relevant ones to document.
    """
    module_c, prog_paths = source_index(root)
    tokens = SourceTokens(root, module_c)
    findings = []
    for rel in pod_files(root, "pod1"):
        name = os.path.basename(rel)[:-4]
        if name not in prog_paths:  # no <name>.c -> not a program
            continue
        diags = pod_diagnostics(read(root, rel))
        if not diags:
            continue
        module = rel.split(os.sep)[0]
        in_module = set(module_c.get(module, []))
        # always fold in the program's own source, even when it is a
        # test program and therefore not part of the module corpus
        own = [sc for sc in prog_paths[name] if sc not in in_module]
        src_tokens = tokens.module(module) + tokens.paths(own)
        findings += diagnostics_findings(rel, name, diags, src_tokens)
    return findings


CHECKS = {
    "admin": check_admin_commands,
    "api": check_pod3_api,
    "pod1": check_pod1_synopsis,
    "xref": check_cross_references,
    "adminrc": check_admin_rc_pairing,
    "diag": check_diagnostics,
}


# -------------------------------------------------------------------- #
# Driver                                                               #
# -------------------------------------------------------------------- #


def collect_findings(root, checks=None):
    """Return a sorted list of finding strings (stable keys for
    baselining). `checks` optionally restricts the run to a subset of
    CHECKS names.
    """
    names = sorted(CHECKS) if checks is None else checks
    findings = []
    for name in names:
        findings += CHECKS[name](root)
    return sorted(set(findings))


def find_root(explicit):
    if explicit:
        return os.path.abspath(explicit)
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)  # tools/ -> repo root
    if os.path.isdir(os.path.join(root, "ici")):
        return root
    return os.getcwd()


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument(
        "--root", help="ION-DTN repository root (default: auto-detect)"
    )
    ap.add_argument(
        "--update-baseline",
        action="store_true",
        help="write current findings to the baseline and exit 0",
    )
    ap.add_argument(
        "--all",
        action="store_true",
        help="print every finding, including baselined ones",
    )
    ap.add_argument(
        "--check",
        action="append",
        choices=sorted(CHECKS),
        help="run only the named check (repeatable); implies --all "
        "reporting over that subset and never updates the "
        "baseline for the checks that did not run",
    )
    args = ap.parse_args()

    if args.check and args.update_baseline:
        ap.error("--check cannot be combined with --update-baseline")

    root = find_root(args.root)
    findings = collect_findings(root, args.check)
    baseline_path = os.path.join(root, BASELINE_FILE)

    if args.update_baseline:
        with open(baseline_path, "w", encoding="utf-8") as fh:
            json.dump(findings, fh, indent=2)
            fh.write("\n")
        print(f"Wrote {len(findings)} findings to {BASELINE_FILE}")
        return 0

    baseline = []
    if os.path.exists(baseline_path):
        with open(baseline_path, "r", encoding="utf-8") as fh:
            baseline = json.load(fh)
    baseline_set = set(baseline)

    new = [f for f in findings if f not in baseline_set]
    # a partial run cannot tell a pruned finding from one of the checks
    # it skipped, so the stale report is only meaningful for a full run
    stale = (
        []
        if args.check
        else [f for f in baseline if f not in set(findings)]
    )

    if args.all or args.check:
        print(f"All findings ({len(findings)}):")
        for f in findings:
            tag = " " if f in baseline_set else "*"
            print(f"  {tag} {f}")
        print()

    if stale:
        print(
            f"NOTE: {len(stale)} baselined finding(s) no longer present "
            f"(run --update-baseline to prune):"
        )
        for f in stale:
            print(f"  - {f}")
        print()

    if new:
        print(f"FAIL: {len(new)} new doc/code inconsistency(ies):")
        for f in new:
            print(f"  + {f}")
        print(
            "\nIf these are intentional, update docs/code or refresh the "
            "baseline with --update-baseline."
        )
        return 1

    print(f"OK: {len(findings)} known finding(s), no new drift.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
