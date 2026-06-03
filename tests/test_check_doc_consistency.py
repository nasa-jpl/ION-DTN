#!/usr/bin/env python3
"""Unit tests for tools/check_doc_consistency.py.

Dependency-free (stdlib unittest only), like the checker itself:

    python3 test_check_doc_consistency.py
"""

import importlib.util
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from typing import Any

current_file = Path(__file__).resolve()
script_path = (
    current_file.parent.parent / ".github" / "scripts" / "check_doc_consistency.py"
)

spec = importlib.util.spec_from_file_location("check_doc_consistency", script_path)

if spec is not None and spec.loader is not None:
    cdc: Any = importlib.util.module_from_spec(spec)
    sys.modules["check_doc_consistency"] = cdc
    spec.loader.exec_module(cdc)
else:
    raise ImportError(f"Could not load script from {script_path}")


# --------------------------------------------------------------------------- #
# C scanning                                                                  #
# --------------------------------------------------------------------------- #


class TopLevelCaseLettersTest(unittest.TestCase):
    def test_collects_depth1_cases_only(self):
        src = """
        static int processLine(char *line, char **tokens, int tokenCount)
        {
            switch (*(tokens[0]))
            {
            case 'a':
                add();
                return 0;

            case 'w':
                switch (tokens[1][0])    /* nested: watch characters */
                {
                case 'z':
                    return 0;
                }

                return 0;

            case 'q':
                return 1;
            }
        }
        """
        self.assertEqual(cdc.top_level_case_letters(src), {"a", "w", "q"})

    def test_ignores_literals_and_comments(self):
        src = """
        switch (*(tokens[0]))
        {
        case 'l':
            PUTS("case 'x': not a command");   /* case 'y': neither */
            return 0;
        }
        """
        self.assertEqual(cdc.top_level_case_letters(src), {"l"})

    def test_unions_two_stage_dispatch(self):
        src = """
        switch (cmdCode[0]) { case 'a': return 0; }
        switch (*(tokens[0])) { case 'b': return 0; }
        """
        self.assertEqual(cdc.top_level_case_letters(src), {"a", "b"})

    def test_no_dispatch_switch(self):
        self.assertEqual(cdc.top_level_case_letters("int main(void) { }"), set())


class ManageSubcommandsTest(unittest.TestCase):
    def test_only_inside_execute_manage(self):
        src = """
        static void executeManage(int tokenCount, char **tokens)
        {
            if (strcmp(tokens[1], "heapmax") == 0) { return; }
            if (strcmp(tokens[1], "usage") == 0) { return; }
        }

        static void executeList(int tokenCount, char **tokens)
        {
            if (strcmp(tokens[1], "span") == 0) { return; }
        }
        """
        self.assertEqual(cdc.manage_subcommands_in_src(src), {"heapmax", "usage"})

    def test_absent_function(self):
        self.assertEqual(cdc.manage_subcommands_in_src("void f(void) {}"), set())


class HeaderFunctionsTest(unittest.TestCase):
    def test_prototypes_macros_and_comments(self):
        hdr = """
        /* Copyright (c) 2024, some banner (text) */
        #define sdr_malloc(sdr, size)   Sdr_malloc(__FILE__, __LINE__, sdr, size)
        extern int  sdr_begin_xn(Sdr sdr);
        void        sdr_exit_xn(Sdr sdr);
        extern char *sdr_string(Sdr sdr,
                        char *from);
        // int sdr_commented(void);
        """
        names = cdc.header_functions(hdr)
        self.assertEqual(
            names, {"sdr_malloc", "sdr_begin_xn", "sdr_exit_xn", "sdr_string"}
        )

    def test_multiline_macro_body_does_not_bleed(self):
        hdr = """
        #define ION_CHECK(x)    \\
                do { if (!(x)) putErrmsg("bad", NULL); } while (0)
        extern int  ionAttach(void);
        """
        self.assertEqual(cdc.header_functions(hdr), {"ION_CHECK", "ionAttach"})


class NamespacePrefixesTest(unittest.TestCase):
    def test_underscore_and_camel_case(self):
        self.assertEqual(cdc.namespace_prefixes({"bp_send", "bp_receive"}), {"bp_"})
        self.assertEqual(cdc.namespace_prefixes({"ionAttach", "ionDetach"}), {"ion"})


class SourceErrorStringsTest(unittest.TestCase):
    def test_joins_continuations_and_drops_comments(self):
        src = """
        /* putErrmsg("commented out", NULL); */
        putErrmsg("Can't open \\
endpoint.", NULL);
        writeMemo("[i] bpclock has ended.\\n");
        """
        strings = cdc.source_error_strings(src)
        self.assertIn("Can't open endpoint.", strings)
        self.assertIn("[i] bpclock has ended. ", strings)  # \\n -> space
        self.assertNotIn("commented out", strings)


# --------------------------------------------------------------------------- #
# POD parsing                                                                 #
# --------------------------------------------------------------------------- #


class PodCommandLettersTest(unittest.TestCase):
    def test_commands_and_prose_items(self):
        pod = """
=item B<a> <duct>

=item B<t> [B<p> [I<timeout>]]

=item B<?>

=item B<Plan>

=item B<m heapmax> <max database heap>
"""
        self.assertEqual(cdc.pod_command_letters(pod), {"a", "t", "?", "m"})

    def test_manage_subcommands(self):
        pod = """
=item B<m heapmax>

=item B<m usage>

=item B<l span>
"""
        self.assertEqual(cdc.pod_manage_subcommands(pod), {"heapmax", "usage"})


class PodApiFunctionsTest(unittest.TestCase):
    def test_prototypes_including_wrapped_ones(self):
        pod = """
=item int bp_send(BpSAP sap, char *destEid)

=item int bssRun(char *bssName, char *path,
        char *buffer, RTBHandler handler)

=item Bundle Status Report (BSR) messages

=item void bp_detach()
"""
        self.assertEqual(cdc.pod_api_functions(pod), {"bp_send", "bssRun", "bp_detach"})


class PodCrossReferencesTest(unittest.TestCase):
    def test_strips_wrappers_and_verbatim_blocks(self):
        pod = """
See also bpadmin(1), I<ionrc>(5) and B<bp>(3).

    result = open(2);

Wait 2 hours (2) before retrying.
"""
        refs = cdc.pod_cross_references(pod)
        self.assertEqual(refs, {("bpadmin", "1"), ("ionrc", "5"), ("bp", "3")})


class PodDiagnosticsTest(unittest.TestCase):
    def test_section_scoped_items_without_headings(self):
        pod = """
=head1 DESCRIPTION

=item not a diagnostic

=head1 DIAGNOSTICS

=over 4

=item bpclock can't attach to BP.

=item B<Normal operation>

=item Can't dispatch events.

=back

=head1 BUGS

=item bug item
"""
        self.assertEqual(
            cdc.pod_diagnostics(pod),
            ["bpclock can't attach to BP.", "Can't dispatch events."],
        )

    def test_no_diagnostics_section(self):
        self.assertEqual(cdc.pod_diagnostics("=head1 NAME\n"), [])


class AdminConfigFileRefsTest(unittest.TestCase):
    def test_named_file_in_dos2unix_boilerplate(self):
        pod = (
            "If you edit the B<ionrc> file on a Windows machine, be sure to "
            "use dos2unix ..."
        )
        self.assertEqual(cdc.admin_config_file_refs(pod), {"ionrc"})


class DiagnosticMatchingTest(unittest.TestCase):
    def _tokens(self, *literals):
        return [set(cdc._words(s)) for s in literals]

    def test_program_prefix_and_placeholders_ignored(self):
        diag = cdc._diag_tokens("lgsend: can't open own endpoint I<eid>.", "lgsend")
        self.assertEqual(diag, {"cant", "open", "own", "endpoint"})
        self.assertTrue(
            cdc.diagnostic_is_documented(diag, self._tokens("Can't open own endpoint."))
        )

    def test_stale_message_is_flagged(self):
        diag = cdc._diag_tokens("lgsend: fgets failed", "lgsend")
        self.assertFalse(
            cdc.diagnostic_is_documented(
                diag, self._tokens("igets failed", "Can't send bundle.")
            )
        )

    def test_empty_diagnostic_is_accepted(self):
        self.assertTrue(cdc.diagnostic_is_documented(set(), []))


# --------------------------------------------------------------------------- #
# Finding builders                                                            #
# --------------------------------------------------------------------------- #


class FindingBuilderTest(unittest.TestCase):
    def test_admin_command_findings_both_directions(self):
        pod = "=item B<a>\n\n=item B<z>\n"
        src = "switch (*(tokens[0])) { case 'a': return 0; case 'q': return 1; }"
        self.assertEqual(
            cdc.admin_command_findings("x/xrc.pod", pod, src),
            [
                "[admin] x/xrc.pod: command 'q' implemented but undocumented",
                "[admin] x/xrc.pod: command 'z' documented but not implemented",
            ],
        )

    def test_manage_subcommand_findings(self):
        pod = "=item B<m usage>\n"
        src = (
            "void executeManage(int c, char **tokens) "
            '{ if (strcmp(tokens[1], "heapmax") == 0) { } }'
        )
        self.assertEqual(
            cdc.manage_subcommand_findings("x/xrc.pod", pod, src),
            [
                "[manage] x/xrc.pod: 'm heapmax' implemented but undocumented",
                "[manage] x/xrc.pod: 'm usage' documented but not implemented",
            ],
        )

    def test_api_findings_limit_reverse_direction_to_namespace(self):
        findings = cdc.api_findings(
            "bp.pod",
            {"bp_send"},
            {"bp_send", "bp_receive", "MAXLEN", "unrelatedHelper"},
        )
        self.assertEqual(
            findings,
            ["[api] bp.pod: 'bp_receive' declared in header(s) but undocumented"],
        )

    def test_api_findings_documented_but_undeclared(self):
        self.assertEqual(
            cdc.api_findings("bp.pod", {"bp_gone"}, set()),
            ["[api] bp.pod: 'bp_gone' documented but not declared in header(s)"],
        )

    def test_cross_reference_findings(self):
        avail_ns = {("bp", "3"), ("bpadmin", "1"), ("ionrc", "5")}
        avail_name = {"bp", "bpadmin", "ionrc"}
        refs = {("bp", "3"), ("ionrc", "3"), ("nosuch", "1"), ("open", "2")}
        self.assertEqual(
            cdc.cross_reference_findings("x.pod", refs, avail_ns, avail_name),
            [
                "[xref] x.pod: 'ionrc(3)' refers to an existing page at the "
                "wrong section",
                "[xref] x.pod: 'nosuch(1)' has no matching pod (broken "
                "cross-reference)",
            ],
        )

    def test_admin_rc_findings(self):
        self.assertEqual(
            cdc.admin_rc_findings("p/ionsecadmin.pod", "ionsecadmin", {"ionsecrc"}), []
        )
        self.assertEqual(
            cdc.admin_rc_findings("p/ionsecadmin.pod", "ionsecadmin", {"ionrc"}),
            [
                "[adminrc] p/ionsecadmin.pod: config-file note names 'ionrc' but "
                "ionsecadmin reads 'ionsecrc'"
            ],
        )

    def test_diagnostics_findings(self):
        src_tokens = [set(cdc._words("Can't dispatch events."))]
        self.assertEqual(
            cdc.diagnostics_findings(
                "p/bpclock.pod",
                "bpclock",
                ["Can't dispatch events.", "Can't reach the moon."],
                src_tokens,
            ),
            [
                "[diag] p/bpclock.pod: documented diagnostic not found in "
                'source: "Can\'t reach the moon."'
            ],
        )


# --------------------------------------------------------------------------- #
# End-to-end over a synthetic tree                                            #
# --------------------------------------------------------------------------- #

POD5 = """=head1 NAME

fooadmin configuration file

=over 4

=item B<a> <thing>

Add.

=item B<z>

Bogus, no handler.

=item B<m usage>

Report usage.

=back
"""

POD1 = """=head1 NAME

fooadmin - foo administration

=head1 SYNOPSIS

B<fooadmin> [ I<commands_filename> ]

=head1 DESCRIPTION

See foorc(5) and nosuch(1).

=head1 DIAGNOSTICS

If you edit the foorc file on a Windows machine, use dos2unix first.

=over 4

=item Can't dispatch events.

Real message.

=item Can't reach the moon.

Fictional message.

=back
"""

SRC = """
static void executeManage(int tokenCount, char **tokens)
{
        if (strcmp(tokens[1], "heapmax") == 0)
        {
                return;
        }
}

static int processLine(char *line, char **tokens, int tokenCount)
{
        switch (*(tokens[0]))
        {
        case 'a':
                putErrmsg("Can't dispatch events.", NULL);
                return 0;

        case 'm':
                executeManage(tokenCount, tokens);
                return 0;
        }
}
"""


class CollectFindingsTest(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="cdc-test-")
        self.addCleanup(shutil.rmtree, self.root)
        for rel, text in (
            ("foo/doc/pod5/foorc.pod", POD5),
            ("foo/doc/pod1/fooadmin.pod", POD1),
            ("foo/utils/fooadmin.c", SRC),
        ):
            path = os.path.join(self.root, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(text)
        # point the checker's configuration at the synthetic tree
        self._saved = (cdc.ADMIN_PAIRS, cdc.POD3_API)
        cdc.ADMIN_PAIRS = [("foo/doc/pod5/foorc.pod", "foo/utils/fooadmin.c")]
        cdc.POD3_API = {}
        self.addCleanup(self._restore)

    def _restore(self):
        cdc.ADMIN_PAIRS, cdc.POD3_API = self._saved

    def test_every_check_reports_its_planted_defect(self):
        findings = cdc.collect_findings(self.root)
        self.assertEqual(findings, sorted(set(findings)))  # sorted, deduped
        joined = "\n".join(findings)
        self.assertIn("command 'z' documented but not implemented", joined)
        self.assertIn("'m heapmax' implemented but undocumented", joined)
        self.assertIn("'m usage' documented but not implemented", joined)
        self.assertIn("'nosuch(1)' has no matching pod", joined)
        self.assertIn('not found in source: "Can\'t reach the moon."', joined)
        # correct entries stay quiet
        self.assertNotIn("command 'a'", joined)
        self.assertNotIn("'foorc(5)'", joined)
        self.assertNotIn("config-file note names", joined)
        self.assertNotIn("SYNOPSIS", joined)
        self.assertNotIn("Can't dispatch events.", joined)

    def test_check_subset_runs_only_that_check(self):
        findings = cdc.collect_findings(self.root, ["xref"])
        self.assertTrue(all(f.startswith("[xref]") for f in findings))

    def test_missing_files_are_reported(self):
        cdc.ADMIN_PAIRS = [("foo/doc/pod5/gone.pod", "foo/utils/gone.c")]
        self.assertIn(
            "[admin] MISSING FILE: foo/doc/pod5/gone.pod or foo/utils/gone.c",
            cdc.collect_findings(self.root, ["admin"]),
        )

    def test_missing_synopsis_is_reported(self):
        path = os.path.join(self.root, "foo/doc/pod1/fooadmin.pod")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(POD1.replace("=head1 SYNOPSIS", "=head1 USAGE"))
        self.assertIn(
            "[pod1] foo/doc/pod1/fooadmin.pod: missing '=head1 SYNOPSIS' section",
            cdc.collect_findings(self.root, ["pod1"]),
        )

    def test_wrong_rc_file_in_dos2unix_note_is_reported(self):
        path = os.path.join(self.root, "foo/doc/pod1/fooadmin.pod")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(POD1.replace("edit the foorc file", "edit the ionrc file"))
        self.assertEqual(
            cdc.collect_findings(self.root, ["adminrc"]),
            [
                "[adminrc] foo/doc/pod1/fooadmin.pod: config-file note names "
                "'ionrc' but fooadmin reads 'foorc'"
            ],
        )


if __name__ == "__main__":
    unittest.main()
