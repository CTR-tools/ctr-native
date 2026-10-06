from __future__ import annotations

import copy
import io
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from types import SimpleNamespace
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parent))
import pipeline as ctr_match
import resident as ctr_resident
import match as cli


class ExtractFunctionTests(unittest.TestCase):
    def test_extracts_only_named_function(self) -> None:
        source = """
void before(void) {}
// Keep this comment outside the generated source.
void target(void)
{
    const char *brace = "}";
    /* { ignored } */
}
void after(void) {}
"""
        self.assertEqual(
            ctr_match.extract_function(source, "target"),
            'void target(void)\n{\n    const char *brace = "}";\n    /* { ignored } */\n}\n',
        )

    def test_rejects_ambiguous_definition(self) -> None:
        with self.assertRaises(ctr_match.MatchError):
            ctr_match.extract_function("void nope(void) {}\n", "target")

    def test_manifest_probes_extract_from_production_source(self) -> None:
        manifest = ctr_match.load_json(ctr_match.DEFAULT_MANIFEST)
        for probe in ctr_match.configured_targets(manifest, "probe"):
            source = ctr_match.ROOT / probe["source"]
            extracted = ctr_match.extract_function(source.read_text(), probe["symbol"])
            self.assertIn(probe["symbol"], extracted)


class ResidentDefinitionTests(unittest.TestCase):
    def test_assembly_label_owns_retail_only_function(self) -> None:
        source = '__asm__(".globl MEMCARD_GetInfo\\n"\n        "MEMCARD_GetInfo:\\n");'
        self.assertEqual(ctr_resident.definition_count(source, "MEMCARD_GetInfo"), 1)
        self.assertEqual(ctr_resident.definition_count(source, "OtherFunction"), 0)

    def test_c_definition_takes_precedence_over_conditional_assembly(self) -> None:
        source = 'void target(void) {}\n__asm__("target:\\n");'
        self.assertEqual(ctr_resident.definition_count(source, "target"), 1)

    def test_conditional_signatures_share_one_body(self) -> None:
        source = """
#ifdef CTR_NATIVE
static unsigned int *target(void *context)
#else
void target(void)
#endif
{
}
"""
        self.assertEqual(ctr_resident.definition_count(source, "target"), 1)
        self.assertEqual(ctr_resident.definition_count(source + source, "target"), 2)
        self.assertEqual(ctr_resident.definition_count("void target(void);\n", "target"), 0)


class SharedSectionsTests(unittest.TestCase):
    def test_initialized_data_requires_symbol_placement_and_complete_bounded_bytes(self) -> None:
        build = {
            "output_section": ".initialized",
            "symbol": "data", "symbol_address": "0x1000", "offset": 4, "size": 4,
        }
        section = {"address": 0x1000, "size": 8}
        symbol = {"address": 0x1000, "section": ".initialized"}
        self.assertEqual(
            ctr_match.initialized_data_range(build, section, {"data": symbol}, b"HEADGOOD"),
            (0x1004, b"GOOD"),
        )
        for symbols in ({}, {"data": {**symbol, "address": 0x1004}},
                        {"data": {**symbol, "section": ".bss"}}):
            with self.subTest(symbols=symbols), self.assertRaises(ctr_match.MatchError):
                ctr_match.initialized_data_range(build, section, symbols, b"HEADGOOD")
        for change in ({"offset": -1}, {"offset": 5}, {"size": 0}, {"size": -1}, {"size": 9}):
            invalid = {**build, **change}
            with self.subTest(change=change), self.assertRaises(ctr_match.MatchError):
                ctr_match.initialized_data_range(invalid, section, {"data": symbol}, b"HEADGOOD")
        for actual in (b"", b"HEADGOO", b"HEADGOODX"):
            with self.subTest(actual=actual), self.assertRaises(ctr_match.MatchError):
                ctr_match.initialized_data_range(build, section, {"data": symbol}, actual)

    def test_linked_section_reports_its_actual_address_and_size(self) -> None:
        headers = (
            "  0 .resident_block 0000048c 8003ddac 8003ddac 00000dac 2**2\n"
            "  1 .rodata 00000038 800115ac 800115ac 000005ac 2**2\n"
        )
        with mock.patch.object(ctr_match, "command_output", return_value=headers):
            self.assertEqual(
                ctr_match.linked_section(Path("objdump"), Path("block.elf"), ".rodata"),
                {"size": 0x38, "address": 0x800115AC},
            )
            with self.assertRaises(ctr_match.MatchError):
                ctr_match.linked_section(Path("objdump"), Path("block.elf"), ".data")

    def test_generated_tables_must_agree_with_the_emitted_artifact(self) -> None:
        toolchain = SimpleNamespace(binutils={"objdump": "objdump"})
        with tempfile.TemporaryDirectory() as directory:
            linked = Path(directory) / "overlay.elf"
            for address, table, accepted in ((0x80001004, b"KEY!", True),
                                             (0x80001004, b"BAD!", False),
                                             (0x80001004, b"KEY", False),
                                             (0x80000FFC, b"KEY!", False),
                                             (0x80001008, b"KEY!", False)):
                headers = (
                    "  0 .overlay 00000008 80001000 80001000 00001000 2**2\n"
                    "                  CONTENTS, ALLOC, LOAD, CODE\n"
                    f"  1 .table 00000004 {address:08x} {address:08x} 00002000 2**2\n"
                    "                  CONTENTS, ALLOC, LOAD, READONLY, DATA\n"
                    "  2 .bss 00000004 80001008 80001008 00003000 2**2\n"
                    "                  ALLOC\n"
                    "  3 .comment 00000004 00000000 00000000 00004000 2**0\n"
                    "                  CONTENTS, READONLY\n"
                )
                with self.subTest(address=address, table=table), \
                     mock.patch.object(ctr_match, "command_output", return_value=headers), \
                     mock.patch.object(ctr_match, "extract_binary_section", side_effect=lambda *args: args[-1].write_bytes(table)):
                    if accepted:
                        self.assertEqual(ctr_match.verify_shared_sections(toolchain, linked, ".overlay", 0x80001000, b"HEADKEY!"),
                                         [{"section": ".table", "offset": 4, "size": 4}])
                    else:
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_match.verify_shared_sections(toolchain, linked, ".overlay", 0x80001000, b"HEADKEY!")


class ComparisonTests(unittest.TestCase):
    def test_rejects_unknown_artifact_selection(self) -> None:
        manifest = {"artifacts": [{"id": "exe"}]}
        with self.assertRaises(ctr_match.MatchError):
            cli.parse_selected(manifest, ["typo"])

    def test_first_difference(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected = root / "expected.bin"
            actual = root / "actual.bin"
            expected.write_bytes(b"abcdef")
            actual.write_bytes(b"abcxef")
            self.assertEqual(ctr_match.first_file_difference(expected, actual), 3)

    def test_length_difference(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected = root / "expected.bin"
            actual = root / "actual.bin"
            expected.write_bytes(b"abcdef")
            actual.write_bytes(b"abc")
            self.assertEqual(ctr_match.first_file_difference(expected, actual), 3)

    def test_exact_files_have_no_difference(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            expected = root / "expected.bin"
            actual = root / "actual.bin"
            expected.write_bytes(b"same")
            actual.write_bytes(b"same")
            self.assertIsNone(ctr_match.first_file_difference(expected, actual))

    def test_range_comparison_counts_same_offset_words(self) -> None:
        expected = bytes.fromhex("01020304 05060708 090a0b0c")
        actual = bytes.fromhex("01020304 ffffffff")
        result = ctr_match.range_comparison(
            expected,
            actual,
            [{"name": "code", "kind": "code", "offset": "0", "size": "0xc"}],
        )[0]
        self.assertEqual(result["matching_bytes"], 4)
        self.assertEqual(result["matching_words"], 1)
        self.assertEqual(result["total_words"], 3)
        self.assertFalse(result["exact"])

    def test_range_comparison_can_align_a_candidate_symbol(self) -> None:
        expected = bytes.fromhex("00000000 01020304")
        actual = bytes.fromhex("01020304")
        result = ctr_match.range_comparison(
            expected,
            actual,
            [
                {
                    "name": "data",
                    "kind": "data",
                    "candidate_symbol": "data",
                    "offset": "0x4",
                    "size": "0x4",
                }
            ],
            {"data": {"offset": 0, "size": 4}},
        )[0]
        self.assertTrue(result["content_exact"])
        self.assertFalse(result["placement_exact"])
        self.assertEqual(result["placement_delta"], -4)


class ToolchainTests(unittest.TestCase):
    def test_manifest_pins_vendored_gcc(self) -> None:
        manifest = ctr_match.load_json(ctr_match.DEFAULT_MANIFEST)
        compiler = manifest["toolchain"]["compiler"]
        self.assertEqual(compiler["version"], "2.8.1")
        self.assertTrue(compiler["directory"].startswith("externals/"))

    def test_require_tool_rejects_wrong_hash(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "tool"
            path.write_bytes(b"not the pinned tool")
            with self.assertRaises(ctr_match.MatchError):
                ctr_match.require_tool(path, "test tool", "0" * 64)

    def test_repository_path_rejects_escape(self) -> None:
        with self.assertRaises(ctr_match.MatchError):
            ctr_match.repository_path("../outside")

    def test_assembler_tracks_included_sources(self) -> None:
        toolchain = SimpleNamespace(binutils={"as": Path("mips-as")})

        with mock.patch.object(ctr_match, "run_checked") as run_checked:
            ctr_match.assemble_mips_source(
                toolchain,
                Path("renderer.s"),
                Path("renderer.o"),
                0,
                [Path("game/RenderLevel/psx")],
                Path("renderer.d"),
            )

        run_checked.assert_called_once_with(
            [
                "mips-as",
                "-G0",
                "-Igame/RenderLevel/psx",
                "--MD",
                "renderer.d",
                "-o",
                "renderer.o",
                "renderer.s",
            ]
        )

    def test_source_flags_reach_only_the_selected_compiler_input(self) -> None:
        config = {"name": "sdk", "kind": "resident", "sources": ["core.c", "wrapper.c"],
                  "compiler_flags": ["-O2", "-G0"], "small_data_limit": 0,
                  "source_flags": {"wrapper.c": ["-fno-delayed-branch"]}}
        tools = SimpleNamespace(default_aspsx_version="2.77")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with (mock.patch.object(ctr_match, "ROOT", root),
                  mock.patch.object(ctr_match, "compile_c") as compile_c,
                  mock.patch.object(ctr_match, "normalize_compiler_directives"),
                  mock.patch.object(ctr_match, "assemble_compiler_output"),
                  mock.patch.object(ctr_match, "dependency_inputs", return_value=[])):
                for name in config["sources"]:
                    ctr_match.build_object(tools, config, root / name, root / "objects" / name)
                self.assertEqual([call.args[3] for call in compile_c.call_args_list], [
                    ["-O2", "-G0", "-ffunction-sections"],
                    ["-O2", "-G0", "-fno-delayed-branch", "-ffunction-sections"],
                ])
                for overrides in ({"missing.c": []}, {"wrapper.c": "-O0"},
                                  {"./wrapper.c": []}, {"wrapper.c": [0]},
                                  None, [], "", 0):
                    invalid = {**config, "source_flags": overrides}
                    compile_c.reset_mock()
                    with self.subTest(overrides=overrides), self.assertRaises(ctr_match.MatchError):
                        ctr_match.build_object(tools, invalid, root / "core.c", root / "invalid")
                    compile_c.assert_not_called()
                with self.assertRaises(ctr_match.MatchError):
                    ctr_match.source_paths({**config, "kind": "probe"})
                with self.assertRaises(ctr_match.MatchError):
                    ctr_match.source_paths({**config, "sources": ["entry.s"],
                                            "source_flags": {"entry.s": ["-O0"]}})


class ResidentNamespaceTests(unittest.TestCase):
    def test_resident_ownership_options_reject_other_target_kinds(self) -> None:
        for kind in ("artifact", "data", "probe"):
            for option in ("owned_symbols", "code_intervals"):
                with self.subTest(kind=kind, option=option), self.assertRaises(ctr_match.MatchError):
                    ctr_match.configured_targets({"targets": [{"name": "sample", "kind": kind, option: {}}]})

    def test_code_intervals_exclude_gaps_and_validate_before_selection(self) -> None:
        symbols = [ctr_resident.Symbol("Test_First", 0x1000), ctr_resident.Symbol("Other_Gap", 0x1004),
                   ctr_resident.Symbol("Test_Second", 0x2000), ctr_resident.Symbol("Test_End", 0x2004)]
        intervals = [{"start_symbol": "Test_First", "end_symbol": "Other_Gap"},
                     {"start_symbol": "Test_Second", "end_symbol": "Test_End"}]
        config = {"name": "sample", "symbol_prefix": "Test_", "source": "source.c",
                  "linked_block": {}, "code_intervals": intervals}
        self.assertEqual([(symbol.name, size) for symbol, size in ctr_resident.namespace_symbol_ranges(config, symbols)],
                         [("Test_First", 4), ("Test_Second", 4)])
        malformed = [None, {}, [], [None], [{}], [intervals[0], intervals[0]], list(reversed(intervals)),
                     [{"start_symbol": "Missing", "end_symbol": "Other_Gap"}],
                     [{"start_symbol": "Test_Second", "end_symbol": "Test_First"}],
                     [{"start_symbol": "Test_First", "end_symbol": "Test_End"}],
                     [{"start_symbol": "Test_First", "end_symbol": "Test_End"}, intervals[1]]]
        for value in malformed:
            with self.subTest(intervals=value), self.assertRaises(ctr_match.MatchError):
                ctr_resident.namespace_symbol_ranges({**config, "code_intervals": value}, symbols)
        for value in ({**config, "start_symbol": "Test_First"}, {**config, "end_symbol": "Test_End"},
                      {key: value for key, value in config.items() if key != "linked_block"}):
            with self.subTest(config=value), self.assertRaises(ctr_match.MatchError):
                ctr_resident.namespace_intervals(value, symbols)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source.c").write_text("void Test_First(void) {}\n")
            (root / "symbols.txt").write_text("00001000 Test_First\n00001004 Other_Gap\n00002000 Test_Second\n00002004 Test_End\n")
            with mock.patch.object(ctr_match, "ROOT", root), self.assertRaises(ctr_match.MatchError):
                ctr_resident.build_resident({"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]}, SimpleNamespace(),
                                           {**config, "artifact": "exe", "symbol_file": "symbols.txt"}, root,
                                           requested=["Test_First"])

    def test_code_interval_bytes_need_real_code_and_function_placement(self) -> None:
        intervals = [(".resident_block_0", ctr_resident.Symbol("Test_First", 0x1000), ctr_resident.Symbol("Gap", 0x1004)),
                     (".resident_block_1", ctr_resident.Symbol("Test_Second", 0x2000), ctr_resident.Symbol("End", 0x2004))]
        functions = [ctr_resident.FunctionRange("Test_First", 0x1000, 4, "source.c"),
                     ctr_resident.FunctionRange("Test_Second", 0x2000, 4, "source.c")]
        toolchain = SimpleNamespace(binutils={"objdump": Path("objdump")})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            elf = root / "block.elf"
            elf.write_bytes(b"unit-test-tool-fixture")
            for case in ("valid", "wrong-placement", "different-bytes", "ABS", "missing-code", "extra-allocated"):
                address = 0x2004 if case == "wrong-placement" else 0x2000
                flags = "CONTENTS, ALLOC, LOAD, READONLY, DATA" if case == "missing-code" else "CONTENTS, ALLOC, LOAD, READONLY, CODE"
                headers = (" 0 .resident_block_0 00000004 00001000 00001000 00000100 2**2\n"
                           "                  CONTENTS, ALLOC, LOAD, READONLY, CODE\n"
                           f" 1 .resident_block_1 00000004 {address:08x} {address:08x} 00000200 2**2\n"
                           f"                  {flags}\n")
                if case == "extra-allocated":
                    headers += " 2 .extra 00000004 00003000 00003000 00000300 2**2\n                  CONTENTS, ALLOC, LOAD, DATA\n"
                section = "*ABS*" if case == "ABS" else ".resident_block_1"
                table = ("00001000 g F .resident_block_0 00000004 Test_First\n"
                         f"{address:08x} g F {section} 00000004 Test_Second\n")
                def dump(argv):
                    return headers if "-h" in argv else table if "-t" in argv else ""
                def extract(_tools, _elf, name, destination):
                    destination.write_bytes(b"BAD!" if case == "different-bytes" and name.endswith("_1") else b"GOOD")
                with (self.subTest(case=case), mock.patch.object(ctr_match, "ROOT", root),
                      mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                      mock.patch.object(ctr_match, "command_output", side_effect=dump),
                      mock.patch.object(ctr_match, "extract_binary_section", side_effect=extract),
                      mock.patch.object(ctr_match, "reference_bytes", return_value=b"GOOD")):
                    if case in ("ABS", "missing-code", "extra-allocated"):
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.compare_linked_intervals({}, toolchain,
                                {"name": "sample", "artifact": "exe", "linked_block": {}}, root, functions, elf, intervals)
                    else:
                        result = ctr_resident.compare_linked_intervals({}, toolchain,
                            {"name": "sample", "artifact": "exe", "linked_block": {}}, root, functions, elf, intervals)
                        self.assertEqual(result["code"]["expected_size"], 8)
                        self.assertEqual(all(item["exact"] for item in result["functions"]), case == "valid")
                        self.assertEqual(result["code_placed"], case != "wrong-placement")
                        self.assertEqual(result["code"]["exact"], case == "valid")

    def test_partial_intervals_keep_all_owned_sources_and_reject_invalid_unselected_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "first.c").write_text("void Test_First(void) {}\n")
            (root / "second.c").write_text("void Actual_Second(void) {}\n")
            (root / "symbols.txt").write_text("00001000 Test_First\n00001004 Gap\n00002000 Test_Second\n00002004 End\n")
            config = {"name": "sample", "artifact": "exe", "symbol_file": "symbols.txt", "symbol_prefix": "Test_",
                      "sources": ["first.c", "second.c"], "owned_symbols": {"Test_Second": "Actual_Second"}, "linked_block": {},
                      "code_intervals": [{"start_symbol": "Test_First", "end_symbol": "Gap"},
                                         {"start_symbol": "Test_Second", "end_symbol": "End"}]}
            for invalid in (False, True):
                def compile_source(_config, _tools, source):
                    return {"compiled": True, "object_path": source.with_suffix(".o")}
                def inventory(_tool, obj):
                    name = "Test_First" if obj.stem == "first" else "Actual_Second"
                    entries = {name: {"count": 1, "size": 4, "section": "." + name}}
                    if invalid and obj.stem == "second":
                        entries["Extra"] = {"count": 1, "size": 4, "section": ".Actual_Second"}
                    return entries
                bodies = [{"name": name, "address": f"0x{address:08x}", "exact": True, "matching_bytes": 4,
                           "expected_size": 4, "matching_words": 1, "total_words": 1}
                          for name, address in (("Test_First", 0x1000), ("Test_Second", 0x2000))]
                block = {"exact": True, "code": {"matching_bytes": 8, "expected_size": 8},
                         "rodata_address": None, "placement_exact": True, "functions": bodies}
                with (self.subTest(invalid=invalid), mock.patch.object(ctr_match, "ROOT", root),
                      mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                      mock.patch.object(ctr_resident, "compile_source", side_effect=compile_source) as compile,
                      mock.patch.object(ctr_resident, "object_functions", side_effect=inventory),
                      mock.patch.object(ctr_resident, "link_function") as isolated,
                      mock.patch.object(ctr_resident, "link_contiguous_block", return_value=block) as shared,
                      mock.patch.object(ctr_match, "build_evidence", return_value={}), redirect_stdout(io.StringIO())):
                    result = ctr_resident.build_resident({"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]},
                        SimpleNamespace(binutils={"objdump": Path("objdump")}), config, root, requested=["Test_First"])
                self.assertEqual(compile.call_count, 2)
                isolated.assert_not_called()
                self.assertEqual(shared.call_count, int(not invalid))
                self.assertFalse(result["complete"])
                self.assertFalse(result["exact"])
                self.assertEqual(result["exact_function_count"], int(not invalid))
                self.assertEqual([body["name"] for body in result["functions"]], ["Test_First"])

    def test_owned_discovery_preserves_canonical_identity_and_optional_result_shape(self) -> None:
        symbols = [ctr_resident.Symbol("Test_First", 0x1000),
                   ctr_resident.Symbol("Test_Second", 0x1004),
                   ctr_resident.Symbol("Test_End", 0x1008)]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            config = {"name": "sample", "sources": ["source.c"],
                      "start_symbol": "Test_First", "end_symbol": "Test_End",
                      "symbol_prefix": "Test_"}
            with mock.patch.object(ctr_match, "ROOT", root):
                (root / "source.c").write_text("void Test_First(void) {}\nvoid Test_Second(void) {}\n")
                identity = ctr_resident.discover_function_ranges(config, symbols)
                for mapping in ({}, {"Test_Second": "Test_Second"}):
                    self.assertEqual(ctr_resident.discover_function_ranges(
                        {**config, "owned_symbols": mapping}, symbols), identity)
                self.assertNotIn("candidate_symbol", ctr_resident.error_result(identity[1], "red"))
                (root / "source.c").write_text("void Test_First(void) {}\nvoid Actual_Second(void) {}\n")
                mapped = ctr_resident.discover_function_ranges(
                    {**config, "owned_symbols": {"Test_Second": "Actual_Second"}}, symbols)
                self.assertEqual((mapped[1].name, mapped[1].address, mapped[1].size, mapped[1].source),
                                 ("Test_Second", 0x1004, 4, "source.c"))
                self.assertEqual(ctr_resident.error_result(mapped[1], "red")["candidate_symbol"], "Actual_Second")
                self.assertEqual(ctr_resident.selected_functions(mapped, ["Test_Second"]), [mapped[1]])
                with self.assertRaises(ctr_match.MatchError):
                    ctr_resident.selected_functions(mapped, ["Actual_Second"])

    def test_owned_discovery_rejects_invalid_mapping_and_literal_ownership(self) -> None:
        symbols = [ctr_resident.Symbol("Test_First", 0x1000),
                   ctr_resident.Symbol("Test_Second", 0x1004),
                   ctr_resident.Symbol("Test_End", 0x1008),
                   ctr_resident.Symbol("Outside", 0x2000)]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            config = {"name": "sample", "sources": ["source.c"],
                      "start_symbol": "Test_First", "end_symbol": "Test_End",
                      "symbol_prefix": "Test_"}
            with mock.patch.object(ctr_match, "ROOT", root):
                (root / "source.c").write_text("void Test_First(void) {}\nvoid Test_Second(void) {}\n")
                for mapping in (None, [], {"Typo": "Actual"}, {"Test_End": "Actual"},
                                {"bad-key": "Actual"}, {"Test_Second": "bad-name"},
                                {"Test_Second": ""}, {"Test_Second": None},
                                {"Test_First": "Test_Second"}, {"Test_First": "Outside"}):
                    with self.subTest(mapping=mapping), self.assertRaises(ctr_match.MatchError):
                        ctr_resident.discover_function_ranges({**config, "owned_symbols": mapping}, symbols)
                config["owned_symbols"] = {"Test_Second": "Actual_Second"}
                for definitions in ("", "void Actual_Second(void) {}\nvoid Actual_Second(void) {}\n"):
                    (root / "source.c").write_text("void Test_First(void) {}\n" + definitions)
                    with self.subTest(definitions=definitions), self.assertRaises(ctr_match.MatchError):
                        ctr_resident.discover_function_ranges(config, symbols)
                (root / "source.c").write_text("void Test_First(void) {}\nvoid Actual_Second(void) {}\n")
                (root / "other.c").write_text("void Actual_Second(void) {}\n")
                with self.assertRaises(ctr_match.MatchError):
                    ctr_resident.discover_function_ranges({**config, "sources": ["source.c", "other.c"]}, symbols)
                # An unselected invalid owner is still rejected before CLI filtering.
                (root / "source.c").write_text("void Test_First(void) {}\n")
                (root / "symbols.txt").write_text("00001000 Test_First\n00001004 Test_Second\n00001008 Test_End\n")
                with self.assertRaises(ctr_match.MatchError):
                    ctr_resident.build_resident({"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]},
                        SimpleNamespace(), {**config, "artifact": "exe", "symbol_file": "symbols.txt"}, root,
                        requested=["Test_First"])

    def test_owned_object_requires_one_positive_defined_function_section(self) -> None:
        function = ctr_resident.FunctionRange("Test_Second", 0x1004, 4, "source.c", "Actual_Second")
        valid = "00000000 g F .Actual_Second 00000004 Actual_Second\n"
        for text, accepted in ((valid, True), (valid + valid, False),
                               (valid.replace(".Actual_Second", "*ABS*"), False),
                               (valid.replace(".Actual_Second", "*UND*"), False),
                               (valid.replace("00000004", "00000000"), False),
                               (valid.replace(".Actual_Second", ".Wrong"), False), ("", False)):
            with self.subTest(text=text), mock.patch.object(ctr_match, "command_output", return_value=text):
                inventory = ctr_resident.object_functions(Path("objdump"), Path("source.o"))
                if accepted:
                    self.assertEqual(ctr_resident.owned_function(function, inventory, ".Actual_Second")["size"], 4)
                else:
                    with self.assertRaises(ctr_match.MatchError):
                        ctr_resident.owned_function(function, inventory, ".Actual_Second")

    def test_owned_input_section_cannot_contain_another_function(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source.c").write_text("void Actual_Second(void) {}\n")
            (root / "symbols.txt").write_text("00001004 Test_Second\n00001008 Test_End\n")
            config = {"name": "sample", "artifact": "exe", "symbol_file": "symbols.txt",
                      "start_symbol": "Test_Second", "end_symbol": "Test_End", "symbol_prefix": "Test_",
                      "source": "source.c", "owned_symbols": {"Test_Second": "Actual_Second"}}
            inventory = {"Actual_Second": {"section": ".Actual_Second", "size": 4, "count": 1},
                         "Unexpected_Helper": {"section": ".Actual_Second", "size": 4, "count": 1}}
            with (mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                  mock.patch.object(ctr_resident, "compile_source", return_value={"compiled": True, "object_path": root / "source.o"}),
                  mock.patch.object(ctr_resident, "object_functions", return_value=inventory),
                  mock.patch.object(ctr_resident, "link_function") as link,
                  mock.patch.object(ctr_match, "build_evidence", return_value={}), redirect_stdout(io.StringIO())):
                result = ctr_resident.build_resident(
                    {"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]},
                    SimpleNamespace(binutils={"objdump": Path("objdump")}), config, root)
            link.assert_not_called()
            self.assertFalse(result["exact"])
            self.assertEqual(result["functions"][0]["candidate_symbol"], "Actual_Second")
            self.assertIn("multiple functions share owned section", result["functions"][0]["error"])

    def test_identity_assembly_entry_can_enclose_a_c_body_in_its_section(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source.c").write_text(
                '__asm__(".globl Test_Entry\\n"\n "Test_Entry:\\n");\n'
                'void Test_Body(void) {}\n'
            )
            (root / "symbols.txt").write_text("00001000 Test_Entry\n00001008 Test_End\n")
            config = {"name": "sample", "artifact": "exe", "symbol_file": "symbols.txt",
                      "start_symbol": "Test_Entry", "end_symbol": "Test_End", "symbol_prefix": "Test_",
                      "source": "source.c"}
            table = "00000000 g F .Test_Entry 00000008 Test_Entry\n00000004 g F .Test_Entry 00000004 Test_Body\n"
            linked = {"name": "Test_Entry", "address": "0x00001000", "exact": True,
                      "matching_bytes": 8, "expected_size": 8, "matching_words": 2, "total_words": 2}
            with (mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                  mock.patch.object(ctr_resident, "compile_source", return_value={"compiled": True, "object_path": root / "source.o"}),
                  mock.patch.object(ctr_match, "command_output", return_value=table),
                  mock.patch.object(ctr_resident, "reference_slice", return_value=b"ENTRYBODY"),
                  mock.patch.object(ctr_resident, "link_function", return_value=linked) as link,
                  mock.patch.object(ctr_match, "build_evidence", return_value={}), redirect_stdout(io.StringIO())):
                result = ctr_resident.build_resident(
                    {"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]},
                    SimpleNamespace(binutils={"objdump": Path("objdump")}), config, root)
            link.assert_called_once()
            self.assertTrue(result["exact"])
            self.assertNotIn("candidate_symbol", result["functions"][0])

    def test_owned_function_link_uses_real_body_and_omitted_callee_context(self) -> None:
        symbols = [ctr_resident.Symbol("Test_First", 0x1000),
                   ctr_resident.Symbol("Test_Second", 0x1004)]
        mapped = ctr_resident.FunctionRange("Test_Second", 0x1004, 4, "source.c", "Actual_Second")
        callback = ctr_resident.FunctionRange("Test_First", 0x1000, 4, "source.c")
        toolchain = SimpleNamespace(binutils={"ld": Path("ld"), "objdump": Path("objdump")})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            config = {"name": "sample", "owned_symbols": {"Test_Second": "Actual_Second"}}
            for function in (mapped, callback):
                emitted = function.emitted_name
                table = f"{function.address:08x} g F .{function.name} 00000004 {emitted}\n"
                with (self.subTest(function=function.name), mock.patch.object(ctr_match, "ROOT", root),
                      mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                      mock.patch.object(ctr_match, "run_checked") as link,
                      mock.patch.object(ctr_match, "command_output", return_value=table),
                      mock.patch.object(ctr_match, "linked_section", return_value={"address": function.address, "size": 4}),
                      mock.patch.object(ctr_match, "extract_binary_section", side_effect=lambda *args: args[-1].write_bytes(b"GOOD")),
                      mock.patch.object(ctr_match, "compare_binary_files", return_value={"exact": True})):
                    result = ctr_resident.link_function(config, toolchain, symbols, function, root / "source.o",
                                                       [".Test_First", ".Actual_Second"], b"GOOD")
                    argv = link.call_args.args[0]
                    self.assertEqual(argv[argv.index("-e") + 1], emitted)
                    self.assertEqual(result["name"], function.name)
                    self.assertTrue(result["exact"])
                    self.assertEqual("candidate_symbol" in result, function is mapped)
                    script = (root / result["linker_script"]).read_text()
                    self.assertIn(f".{function.name} 0x{function.address:08x}", script)
                    self.assertEqual(script.count(f"*({'.' + emitted})"), 1)
                    self.assertNotIn(f"--defsym={emitted}=0x{function.address:08x}", argv)
                    if function is callback:
                        self.assertIn("--defsym=Actual_Second=0x00001004", argv)
                        ctr_resident.link_function({**config, "link_symbols": {"Actual_Second": "0x1004"}},
                            toolchain, symbols, callback, root / "source.o", [".Test_First", ".Actual_Second"], b"GOOD")
                    else:
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.link_function({**config, "link_symbols": {"Actual_Second": "0x1004"}},
                                toolchain, symbols, mapped, root / "source.o", [".Actual_Second"], b"GOOD")

    def test_owned_body_cannot_gain_exactness_from_equal_bytes_at_wrong_placement(self) -> None:
        function = ctr_resident.FunctionRange("Test_Second", 0x1004, 4, "source.c", "Actual_Second")
        toolchain = SimpleNamespace(binutils={"ld": Path("ld"), "objdump": Path("objdump")})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for address, size, section in ((0x1008, 4, ".Test_Second"), (0x1004, 8, ".Test_Second"),
                                           (0x1004, 4, "*ABS*"), (0x1004, 4, ".Wrong")):
                table = f"{address:08x} g F {section} {size:08x} Actual_Second\n"
                with (self.subTest(address=address, size=size, section=section),
                      mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                      mock.patch.object(ctr_match, "run_checked"), mock.patch.object(ctr_match, "command_output", return_value=table),
                      mock.patch.object(ctr_match, "linked_section", return_value={"address": 0x1004, "size": 4}),
                      mock.patch.object(ctr_match, "extract_binary_section", side_effect=lambda *args: args[-1].write_bytes(b"GOOD")),
                      mock.patch.object(ctr_match, "compare_binary_files", return_value={"exact": True})):
                    if section != ".Test_Second":
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.link_function({"name": "sample"}, toolchain, [], function, root / "source.o", [], b"GOOD")
                    else:
                        self.assertFalse(ctr_resident.link_function(
                            {"name": "sample"}, toolchain, [], function, root / "source.o", [], b"GOOD")["exact"])

    def test_owned_block_owns_every_selected_body_and_checks_placement(self) -> None:
        functions = [ctr_resident.FunctionRange("Test_First", 0x1000, 4, "source.c"),
                     ctr_resident.FunctionRange("Test_Second", 0x1004, 4, "source.c", "Actual_Second")]
        toolchain = SimpleNamespace(binutils={"ld": Path("ld"), "objdump": Path("objdump")})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "symbols.txt").write_text("00001000 Test_First\n00001004 Test_Second\n")
            config = {"name": "sample", "artifact": "exe", "linked_block": {}, "symbol_file": "symbols.txt"}
            for address, size, section in ((0x1004, 4, ".resident_block"), (0x1008, 4, ".resident_block"),
                                           (0x1004, 8, ".resident_block"), (0x1004, 4, "*ABS*")):
                table = "00001000 g F .resident_block 00000004 Test_First\n" + f"{address:08x} g F {section} {size:08x} Actual_Second\n"
                with (self.subTest(address=address, size=size, section=section), mock.patch.object(ctr_match, "ROOT", root),
                      mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"), mock.patch.object(ctr_match, "run_checked") as link,
                      mock.patch.object(ctr_match, "command_output", return_value=table),
                      mock.patch.object(ctr_match, "linked_section", return_value={"address": 0x1000, "size": 8}),
                      mock.patch.object(ctr_match, "reference_bytes", return_value=b"GOODNEXT"),
                      mock.patch.object(ctr_match, "extract_binary_section", side_effect=lambda *args: args[-1].write_bytes(b"GOODNEXT")),
                      mock.patch.object(ctr_match, "compare_binary_files", return_value={"exact": True})):
                    if section == "*ABS*":
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.link_contiguous_block({}, toolchain, config, root, functions, [root / "source.o"])
                    else:
                        result = ctr_resident.link_contiguous_block({}, toolchain, config, root, functions, [root / "source.o"])
                        self.assertEqual(result["exact"], address == 0x1004 and size == 4)
                        script = (root / "build/sample/linked_block/block.ld").read_text()
                        self.assertIn("*(.Test_First) *(.Actual_Second)", script)
                        self.assertIn("/DISCARD/ : { *(*) }", script)
                    for alias in ("Test_First", "Actual_Second"):
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.link_contiguous_block({}, toolchain,
                                {**config, "link_symbols": {alias: "0x1004"}}, root, functions, [root / "source.o"])

    def test_linked_block_requires_all_sources_and_preserves_configured_order(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "symbols.txt").write_text(
                "80010000 Test_First\n80010004 Test_Second\n80010008 Test_End\n"
            )
            (root / "first.c").write_text("void Test_First(void) {}\n")
            (root / "second.c").write_text("void Test_Second(void) {}\n")
            config = {
                "name": "sample",
                "artifact": "exe",
                "symbol_file": "symbols.txt",
                "start_symbol": "Test_First",
                "end_symbol": "Test_End",
                "symbol_prefix": "Test_",
                "sources": ["second.c", "first.c"],
                "linked_block": {},
            }
            manifest = {"artifacts": [{"id": "exe", "kind": "ps-x-exe"}]}
            toolchain = SimpleNamespace(
                binutils={"ld": Path("ld"), "objdump": Path("objdump")}
            )
            for failed_source in (None, "first.c", "second.c"):
                def compile_source(config, toolchain, source):
                    if source.name == failed_source:
                        return {"compiled": False, "error": "compiler failed"}
                    return {"compiled": True, "object_path": source.with_suffix(".o")}

                with (
                    self.subTest(failed_source=failed_source),
                    mock.patch.object(ctr_match, "ROOT", root),
                    mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                    mock.patch.object(
                        ctr_resident, "compile_source", side_effect=compile_source
                    ),
                    mock.patch.object(ctr_resident, "object_functions", return_value={}),
                    mock.patch.object(ctr_match, "build_evidence", return_value={}),
                    mock.patch.object(
                        ctr_match,
                        "run_checked",
                        side_effect=ctr_match.MatchError("stop at linker"),
                    ) as link,
                    redirect_stdout(io.StringIO()),
                ):
                    result = ctr_resident.build_resident(
                        manifest, toolchain, config, root
                    )

                self.assertFalse(result["linked_block"]["exact"])
                self.assertFalse(result["exact"])
                if failed_source is None:
                    link.assert_called_once()
                    self.assertEqual(
                        link.call_args.args[0][-2:],
                        [str(root / "second.o"), str(root / "first.o")],
                    )
                    self.assertEqual(result["linked_block"]["error"], "stop at linker")
                else:
                    link.assert_not_called()
                    self.assertEqual(
                        result["linked_block"]["error"],
                        f"linked block source does not compile: {failed_source}",
                    )

    def test_symbol_ranges_cover_the_interval_without_gaps(self) -> None:
        config = {
            "name": "sample",
            "start_symbol": "Veh_First",
            "end_symbol": "AfterVehicle",
            "symbol_prefix": "Veh_",
        }
        symbols = [
            ctr_resident.Symbol("BeforeVehicle", 0x0FF0),
            ctr_resident.Symbol("Veh_First", 0x1000),
            ctr_resident.Symbol("Veh_Second", 0x1020),
            ctr_resident.Symbol("AfterVehicle", 0x1050),
        ]

        ranges = ctr_resident.namespace_symbol_ranges(config, symbols)

        self.assertEqual(
            [(symbol.name, size) for symbol, size in ranges],
            [("Veh_First", 0x20), ("Veh_Second", 0x30)],
        )

    def test_resident_assembly_closes_small_data_before_a_function(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            assembly = Path(directory) / "source.s"
            assembly.write_text(
                "\t.sdata\nvalue:\n\t.word\t1\n"
                "\t.extern\tsdata_static+832, 4\n"
                "\t.extern\t312($28), 4\n"
                '\t.section .Veh_Test,"ax",@progbits\n'
            )

            ctr_match.normalize_compiler_directives(assembly)

            self.assertEqual(
                assembly.read_text(),
                "\t.sdata\nvalue:\n\t.word\t1\n\t.text\n"
                '\t.section .Veh_Test,"ax",@progbits\n',
            )

    def test_function_linker_script_keeps_reachable_helpers_nearby(self) -> None:
        function = ctr_resident.FunctionRange(
            name="Veh_Test",
            address=0x80050000,
            size=0x40,
            source="game/Vehicle/Test.c",
        )

        linker = ctr_resident.function_linker_script(
            function,
            [".Veh_Test", ".Veh_Test_Helper", ".Unrelated"],
        )

        self.assertIn(".Veh_Test 0x80050000", linker)
        self.assertEqual(linker.count("*(.Veh_Test)"), 1)
        self.assertIn("*(.Veh_Test_Helper)", linker)
        self.assertIn("*(.Unrelated)", linker)
        self.assertLess(
            linker.index(".Veh_Test 0x80050000"),
            linker.index(".__function_closure"),
        )

    def test_vehicle_inventory_maps_every_retail_symbol_to_production(self) -> None:
        config = ctr_match.target_by_name(
            ctr_match.load_json(ctr_match.DEFAULT_MANIFEST), "vehicle"
        )
        symbols = ctr_resident.parse_symbols(
            ctr_match.repository_path(config["symbol_file"])
        )

        functions = ctr_resident.discover_function_ranges(config, symbols)

        self.assertEqual(len(functions), 129)
        self.assertEqual(functions[0].name, "VehAfterColl_GetSurface")
        self.assertEqual(functions[0].address, 0x80057C44)
        self.assertEqual(functions[-1].name, "VehTurbo_ThTick")
        self.assertEqual(
            functions[-1].address + functions[-1].size,
            0x80069BB0,
        )
        self.assertEqual(len({function.name for function in functions}), 129)
        self.assertEqual(
            len({function.source for function in functions}),
            len(ctr_resident.namespace_sources(config)),
        )


class CheckTests(unittest.TestCase):
    def test_check_includes_every_kind_and_fails_on_any_mismatch(self) -> None:
        targets = [
            {"name": "221", "kind": "artifact"},
            {"name": "data", "kind": "data"},
            {"name": "vehicle", "kind": "resident"},
            {"name": "probe", "kind": "probe"},
        ]
        manifest = {"targets": targets}
        args = cli.build_parser().parse_args(["check"])
        for failed in (None, "221", "data", "vehicle", "probe"):
            with (
                self.subTest(failed=failed),
                mock.patch.object(cli, "load_json", return_value=manifest),
                mock.patch.object(cli, "resolve_toolchain"),
                mock.patch.object(cli, "print_toolchain"),
                mock.patch.object(cli, "reference_root", return_value=Path("/retail")),
                mock.patch.object(cli, "verify_references", return_value=[]),
                mock.patch.object(
                    cli,
                    "build_target",
                    side_effect=lambda manifest, tools, config, root: {
                        "exact": config["name"] != failed
                    },
                ) as build,
                redirect_stdout(io.StringIO()),
            ):
                self.assertEqual(cli.cmd_check(args), int(failed is not None))
                self.assertEqual(
                    [call.args[2]["name"] for call in build.call_args_list],
                    ["221", "data", "vehicle", "probe"],
                )

    def test_target_inventory_rejects_duplicates_and_unknown_names(self) -> None:
        config = {"name": "vehicle", "kind": "resident"}
        with self.assertRaises(ctr_match.MatchError):
            ctr_match.configured_targets({"targets": [config, config]})
        with self.assertRaises(ctr_match.MatchError):
            ctr_match.target_by_name({"targets": [config]}, "typo")

    def test_existing_build_commands_use_the_same_entry_point(self) -> None:
        for command in ("build", "artifact", "namespace"):
            args = cli.build_parser().parse_args([command, "vehicle"])
            self.assertIs(args.func, cli.cmd_build)
            self.assertEqual(args.target, "vehicle")


class JointResidentTests(unittest.TestCase):
    @staticmethod
    def elf32(sections):
        """Small executable fixture; section types/SHF bits are real ELF bytes."""
        actual = bytearray(52)
        names = b"\0" + b"\0".join(name.encode() for name, *_ in sections) + b"\0.shstrtab\0"
        headers = [bytes(40)]
        for name, address, contents, kind, flags in sections:
            headers.append(struct.pack("<10I", names.index(name.encode()), kind, flags, address,
                                       len(actual), len(contents), 0, 0, 1, 0))
            actual.extend(contents)
        headers.append(struct.pack("<10I", names.index(b".shstrtab"), 3, 0, 0, len(actual), len(names), 0, 0, 1, 0))
        actual.extend(names)
        section_offset = len(actual)
        actual.extend(b"".join(headers))
        struct.pack_into("<16sHHIIIIIHHHHHH", actual, 0, b"\x7fELF\x01\x01\x01" + bytes(9),
                         2, 8, 1, 0, 0, section_offset, 0, 52, 0, 0, 40, len(headers), len(headers) - 1)
        return bytes(actual)

    def fixture(self, root):
        functions = [ctr_resident.FunctionRange(name, 0x1000 + index * 16, 16, "source.c")
                     for index, name in enumerate(("Test_First", "Test_Second", "Test_Third"))]
        (root / "source.c").write_text("\n".join(f"void {f.name}(void) {{}}" for f in functions))
        (root / "symbols.txt").write_text("\n".join(f"{f.address:08x} {f.name}" for f in functions) + "\n00001030 End\n")
        (root / "joint.ld").write_text("reviewed joint linker layout")
        config = {"name": "joint", "kind": "resident", "artifact": "exe", "source": "source.c",
                  "symbol_file": "symbols.txt", "symbol_prefix": "Test_", "compiler_flags": [],
                  "linker_script": "joint.ld", "linked_block": {},
                  "code_intervals": [{"start_symbol": f.name, "end_symbol": functions[index + 1].name if index < 2 else "End"}
                                     for index, f in enumerate(functions)],
                  "data_ranges": [{"output_section": ".data.state", "symbol": name, "symbol_address": address, "offset": 0, "size": size}
                                  for name, address, size in (("state", "0x3000", 4), ("mode", "0x3004", 1))]}
        manifest = {"target": "test", "targets": [config],
                    "artifacts": [{"id": "exe", "kind": "ps-x-exe", "load_address": "0x1000"}],
                    "toolchain": {"compiler": {"directory": "gcc", "sha256": {}},
                                  "maspsx": {"directory": "maspsx", "sha256": {}, "aspsx_version": "2.77"},
                                  "binutils": {"prefix": "mips-", "sha256": {}}}}
        toolchain = SimpleNamespace(binutils={"ld": Path("ld"), "objdump": Path("objdump")},
                                    compiler_banner="test", compiler_hashes={}, default_aspsx_version="2.77",
                                    maspsx_commit="pinned", maspsx_hashes={}, assembler_banner="test", binutils_hashes={},
                                    config_hash=ctr_match.sha256_json(manifest["toolchain"]))
        code = [(f".resident_block_{index}", f.address, word * 4, 1, 6)
                for index, (f, word) in enumerate(zip(functions, (b"CODE", b"NEXT", b"LAST")))]
        final_table = "".join(f"{f.address:08x} g F .resident_block_{index} 00000010 {f.name}\n" for index, f in enumerate(functions))
        final_table += "00003000 g O .data.state 00000004 state\n00003004 g O .data.state 00000001 mode\n"
        input_table = "".join(f"00000000 g F .{f.name} 00000010 {f.name}\n" for f in functions)
        input_table += "00000000 g O .data.state_owner 00000004 state\n00000000 g O .data.mode_owner 00000001 mode\n"
        fixture = SimpleNamespace(config=config, manifest=manifest, tools=toolchain, functions=functions,
                                  sections=code + [(".data.state", 0x3000, b"DATAx", 1, 3)],
                                  table=final_table, input_tables={root / "source.o": input_table},
                                  bfd_override={}, extracted=[])

        def dump(argv):
            if "-t" in argv:
                return fixture.input_tables[Path(argv[-1])] if argv[-1].endswith(".o") else fixture.table
            if "-h" in argv:
                return "".join(f" {index} {name} {len(contents):08x} {address:08x} {address:08x} 00000034 2**0\n"
                               + "                  " + fixture.bfd_override.get(name, ", ".join(
                                   (["CONTENTS", "LOAD"] if kind != 8 else [])
                                   + (["ALLOC"] if flags & 2 else []) + (["READONLY"] if not flags & 1 else [])
                                   + (["CODE"] if flags & 4 else ["DATA"]))) + "\n"
                               for index, (name, address, contents, kind, flags) in enumerate(fixture.sections))
            return ""

        def link(argv):
            Path(argv[argv.index("-o") + 1]).write_bytes(self.elf32(fixture.sections))

        def extract(_tools, elf, name, destination):
            fixture.extracted.append(elf)
            actual = elf.read_bytes()
            index = next(index + 1 for index, section in enumerate(fixture.sections) if section[0] == name)
            header = struct.unpack_from("<10I", actual, struct.unpack_from("<I", actual, 32)[0] + index * 40)
            destination.write_bytes(actual[header[4]:header[4] + header[5]])

        def reference(_manifest, _references, interval):
            address, size = ctr_match.parse_int(interval["address"]), interval["size"]
            for _name, start, contents, _kind, _flags in code + [(".data.state", 0x3000, b"DATAx", 1, 3)]:
                if start <= address < start + len(contents):
                    return contents[address - start:address - start + size]
            raise AssertionError("unexpected oracle span")

        fixture.dump, fixture.link, fixture.extract, fixture.reference = dump, link, extract, reference
        return fixture

    def test_joint_link_rejects_false_owners_sections_and_aliases(self) -> None:
        cases = ("valid", "different-data", "width", "NOTYPE", "FUNC", "LOCAL", "WEAK", "hidden", "protected",
                 "ABS", "UND", "duplicate", "input-duplicate", "alias", "extra-ALLOC", "readonly", "NOBITS",
                 "executable-data", "section-padding", "owner-gap", "offset", "link-alias", "REL", "RELA", "duplicate-section")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for case in cases:
                f = self.fixture(root)
                object_files = [root / "source.o"]
                if case in ("width", "NOTYPE", "FUNC", "LOCAL", "WEAK", "hidden", "protected", "ABS", "UND"):
                    row = "00003000 g O .data.state 00000004 state"
                    replacement = {"width": row.replace("00000004", "00000008"), "NOTYPE": row.replace(" g O ", " g "),
                                   "FUNC": row.replace(" g O ", " g F "), "LOCAL": row.replace(" g O ", " l O "),
                                   "WEAK": row.replace(" g O ", " w O "), "hidden": row.replace(" state", " .hidden state"),
                                   "protected": row.replace(" state", " .protected state"),
                                   "ABS": row.replace(".data.state", "*ABS*"), "UND": row.replace(".data.state", "*UND*")}[case]
                    f.table = f.table.replace(row, replacement)
                elif case == "duplicate":
                    f.table += "00003000 l O .data.state 00000004 state\n"
                elif case == "input-duplicate":
                    second_provider = root / "second-provider.o"
                    f.input_tables[second_provider] = "00000000 g O .data.other_state 00000004 state\n"
                    object_files.append(second_provider)
                elif case == "alias":
                    f.table += "00003000 g O .data.state 00000004 alias\n"
                elif case == "extra-ALLOC":
                    f.sections.append((".extra", 0x4000, b"more", 8, 3))
                elif case in ("readonly", "NOBITS", "executable-data", "section-padding", "different-data"):
                    contents = b"FAILx" if case == "different-data" else b"DATAx\0" if case == "section-padding" else b"DATAx"
                    f.sections[-1] = (".data.state", 0x3000, contents, 8 if case == "NOBITS" else 1,
                                      2 if case == "readonly" else 7 if case == "executable-data" else 3)
                    f.bfd_override[".data.state"] = "CONTENTS, ALLOC, LOAD, DATA"
                elif case == "owner-gap":
                    f.config["data_ranges"][1]["symbol_address"] = "0x3005"
                    f.table = f.table.replace("00003004", "00003005")
                    f.sections[-1] = (".data.state", 0x3000, b"DATA\0x", 1, 3)
                elif case == "offset":
                    f.config["data_ranges"][0]["offset"] = 1
                elif case == "link-alias":
                    f.config["link_symbols"] = {"state": "0x3000"}
                elif case in ("REL", "RELA"):
                    f.sections.append((".rel.state", 0, bytes(8), 9 if case == "REL" else 4, 0))
                elif case == "duplicate-section":
                    f.sections.append(f.sections[-1])
                with (self.subTest(case=case), mock.patch.object(ctr_match, "ROOT", root),
                      mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                      mock.patch.object(ctr_match, "command_output", side_effect=f.dump),
                      mock.patch.object(ctr_match, "run_checked", side_effect=f.link) as link,
                      mock.patch.object(ctr_match, "extract_binary_section", side_effect=f.extract),
                      mock.patch.object(ctr_match, "reference_bytes", side_effect=f.reference)):
                    if case not in ("valid", "different-data"):
                        with self.assertRaises(ctr_match.MatchError):
                            ctr_resident.link_contiguous_block(f.manifest, f.tools, f.config, root, f.functions, object_files)
                        if case == "link-alias":
                            link.assert_not_called()
                        continue
                    block = ctr_resident.link_contiguous_block(f.manifest, f.tools, f.config, root, f.functions, object_files)
                    link.assert_called_once()
                    self.assertEqual(link.call_args.args[0][2], str(root / "joint.ld"))
                    self.assertEqual(set(f.extracted), {root / block["elf"]})
                    self.assertEqual((block["code"]["expected_size"], len(block["functions"]), block["data"]["expected_size"]), (48, 3, 5))
                    self.assertEqual(block["exact"], case == "valid")
                    self.assertTrue(all(item["exact"] for item in block["functions"]))

    def test_joint_input_owners_distinguish_definitions_from_undefined_consumers(self) -> None:
        cases = ("valid", "two-providers", "only-UND", "missing", "ABS", "COMMON", "NOTYPE", "FUNC",
                 "LOCAL", "WEAK", "hidden", "protected", "width")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            provider, consumer = root / "provider.o", root / "consumer.o"
            definition = "00000000 g O .data.state_owner 00000004 state\n"
            imports = "00000000         *UND* 00000000 state\n00000000         *UND* 00000000 mode\n"
            for case in cases:
                for consumer_first in (False, True):
                    f = self.fixture(root)
                    provider_table = f.input_tables.pop(root / "source.o")
                    consumer_table = imports
                    if case == "two-providers":
                        consumer_table = consumer_table.replace("00000000         *UND* 00000000 state\n", definition)
                    elif case == "only-UND":
                        provider_table = provider_table.replace(definition, "00000000         *UND* 00000000 state\n")
                    elif case == "missing":
                        provider_table = provider_table.replace(definition, "")
                        consumer_table = consumer_table.replace("00000000         *UND* 00000000 state\n", "")
                    elif case != "valid":
                        replacement = {"ABS": definition.replace(".data.state_owner", "*ABS*"),
                                       "COMMON": definition.replace(".data.state_owner", "*COM*"),
                                       "NOTYPE": definition.replace(" g O ", " g "),
                                       "FUNC": definition.replace(" g O ", " g F "),
                                       "LOCAL": definition.replace(" g O ", " l O "),
                                       "WEAK": definition.replace(" g O ", " w O "),
                                       "hidden": definition.replace(" state", " .hidden state"),
                                       "protected": definition.replace(" state", " .protected state"),
                                       "width": definition.replace("00000004", "00000008")}[case]
                        provider_table = provider_table.replace(definition, replacement)
                    f.input_tables = {provider: provider_table, consumer: consumer_table}
                    object_files = [consumer, provider] if consumer_first else [provider, consumer]
                    with (self.subTest(case=case, consumer_first=consumer_first), mock.patch.object(ctr_match, "ROOT", root),
                          mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                          mock.patch.object(ctr_match, "command_output", side_effect=f.dump),
                          mock.patch.object(ctr_match, "run_checked", side_effect=f.link) as link,
                          mock.patch.object(ctr_match, "extract_binary_section", side_effect=f.extract),
                          mock.patch.object(ctr_match, "reference_bytes", side_effect=f.reference)):
                        if case != "valid":
                            with self.assertRaises(ctr_match.MatchError):
                                ctr_resident.link_contiguous_block(f.manifest, f.tools, f.config, root, f.functions, object_files)
                            link.assert_not_called()
                            continue
                        block = ctr_resident.link_contiguous_block(f.manifest, f.tools, f.config, root, f.functions, object_files)
                        link.assert_called_once()
                        self.assertEqual(link.call_args.args[0][-2:], [str(path) for path in object_files])
                        self.assertTrue(block["exact"])
                        self.assertEqual(block["data"]["expected_size"], 5)

    def test_joint_currentness_binds_complete_owners_candidates_script_and_elf(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            f = self.fixture(root)
            with (mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_resident, "BUILD_ROOT", root / "build"),
                  mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt"),
                  mock.patch.object(ctr_match, "command_output", side_effect=f.dump),
                  mock.patch.object(ctr_match, "run_checked", side_effect=f.link) as link,
                  mock.patch.object(ctr_match, "extract_binary_section", side_effect=f.extract),
                  mock.patch.object(ctr_match, "reference_bytes", side_effect=f.reference),
                  mock.patch.object(ctr_resident, "compile_source", return_value={"compiled": True, "object_path": root / "source.o", "dependencies": ["source.c"]}),
                  mock.patch.object(ctr_resident, "link_function") as isolated, redirect_stdout(io.StringIO())):
                result = ctr_resident.build_resident(f.manifest, f.tools, f.config, root)
                link.assert_called_once()
                isolated.assert_not_called()
                self.assertEqual((result["function_count"], result["interval_size"]), (3, 48))
                self.assertTrue(result["exact"])
                self.assertTrue(ctr_match.result_is_current(f.manifest, f.config, result))
                block = result["linked_block"]
                paths = [root / item["candidate"] for item in block["data_ranges"]] + [root / block["elf"], root / "joint.ld"]
                for path in paths:
                    with self.subTest(path=path):
                        original = path.read_bytes()
                        path.write_bytes(original + b"changed")
                        self.assertFalse(ctr_match.result_is_current(f.manifest, f.config, result))
                        path.unlink()
                        self.assertFalse(ctr_match.result_is_current(f.manifest, f.config, result))
                        path.write_bytes(original)
                for case in ("missing", "partial", "reordered", "descriptor", "width", "type", "binding", "visibility", "count", "flags", "section-size", "summary", "error-block"):
                    changed = copy.deepcopy(result)
                    data = changed["linked_block"]["data_ranges"]
                    if case == "missing":
                        del changed["linked_block"]["data_ranges"]
                    elif case == "partial":
                        data.pop()
                    elif case == "reordered":
                        data.reverse()
                    elif case == "descriptor":
                        data[0]["data_range"]["offset"] = 1
                    elif case == "width":
                        data[0]["owner"]["size"] = 8
                    elif case in ("type", "binding", "visibility", "count"):
                        data[0]["owner"][case] = {"type": "NOTYPE", "binding": "LOCAL", "visibility": "HIDDEN", "count": 2}[case]
                    elif case == "flags":
                        data[0]["section"]["elf_flags"] = 2
                    elif case == "section-size":
                        data[0]["section"]["size"] = 6
                    elif case == "summary":
                        changed["linked_block"]["data"]["expected_size"] = 6
                    else:
                        changed["linked_block"] = {"exact": False, "error": "forged error without joint evidence"}
                        changed.update({"exact": False, "selected_exact": False, "exact_function_count": 0})
                    with self.subTest(case=case):
                        self.assertFalse(ctr_match.result_is_current(f.manifest, f.config, changed))
                changed_config = copy.deepcopy(f.config)
                changed_config["data_ranges"][0]["size"] = 3
                self.assertFalse(ctr_match.result_is_current(f.manifest, changed_config, result))
                with mock.patch.object(ctr_match, "run_checked", side_effect=ctr_match.MatchError("joint link failed")):
                    negative = ctr_resident.build_resident(f.manifest, f.tools, f.config, root)
                self.assertFalse(negative["exact"])
                self.assertTrue(ctr_match.result_is_current(f.manifest, f.config, negative))


class InitializedDataBuildTests(unittest.TestCase):
    def test_disjoint_data_sections_share_one_build_and_leave_gaps_unowned(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "data.c").write_text("two initialized sections")
            (root / "data.ld").write_text("disjoint section layout")
            (root / "symbols.txt").write_text("80010000 first\n80010010 second\n")
            oracle = root / "SCUS_944.26"
            oracle.write_bytes(b"HHGOOD" + b"not-owned!!!" + b"LAST")
            intervals = [
                {"output_section": ".first", "symbol": "first", "symbol_address": "0x80010000", "offset": 0, "size": 4},
                {"output_section": ".second", "symbol": "second", "symbol_address": "0x80010010", "offset": 0, "size": 4},
            ]
            config = {
                "name": "data-test", "kind": "data", "artifact": "exe", "source": "data.c",
                "symbol_file": "symbols.txt", "linker_script": "data.ld", "compiler_flags": [],
                "data_ranges": intervals,
            }
            manifest = {
                "target": "test", "targets": [config],
                "artifacts": [{"id": "exe", "kind": "overlay", "path": oracle.name,
                               "size": 22, "sha256": ctr_match.sha256_file(oracle),
                               "load_address": "0x80010000", "mapped_file_offset": 2}],
                "toolchain": {"compiler": {"directory": "gcc", "sha256": {}},
                              "maspsx": {"directory": "maspsx", "sha256": {}, "aspsx_version": "2.77"},
                              "binutils": {"prefix": "mips-", "sha256": {}}},
            }
            toolchain = SimpleNamespace(
                compiler_banner="test", compiler_hashes={}, default_aspsx_version="2.77",
                maspsx_commit="pinned", maspsx_hashes={}, assembler_banner="test", binutils_hashes={},
                config_hash=ctr_match.sha256_json(manifest["toolchain"]),
                binutils={"ld": Path("ld"), "objdump": Path("objdump")},
            )
            headers = (
                "  0 .first 00000004 80010000 80010000 00001000 2**2\n"
                "                  CONTENTS, ALLOC, LOAD, DATA\n"
                "  1 .second 00000004 80010010 80010010 00001010 2**2\n"
                "                  CONTENTS, ALLOC, LOAD, DATA\n"
            )

            def objdump(command):
                if "-t" in command:
                    return "80010000 g .first 00000000 first\n80010010 g .second 00000000 second\n"
                return headers if "-h" in command else ""

            with (
                mock.patch.object(ctr_match, "ROOT", root),
                mock.patch.object(ctr_match, "BUILD", root / "build"),
                mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt"),
                mock.patch.object(ctr_match, "build_object", return_value=(root / "data.o", ["data.c"])) as compile_object,
                mock.patch.object(ctr_match, "run_checked") as link,
                mock.patch.object(ctr_match, "command_output", side_effect=objdump),
                mock.patch.object(ctr_match, "extract_binary_section",
                                  side_effect=lambda *args: args[-1].write_bytes({".first": b"GOOD", ".second": b"LAST"}[args[-2]])),
                redirect_stdout(io.StringIO()),
            ):
                result = cli.build_target(manifest, toolchain, config, root)
                self.assertTrue(result["exact"])
                self.assertEqual((result["candidate_size"], result["expected_size"]), (8, 8))
                self.assertEqual([r["address"] for r in result["ranges"]], ["0x80010000", "0x80010010"])
                self.assertEqual([(root / r["candidate"]).read_bytes() for r in result["ranges"]], [b"GOOD", b"LAST"])
                compile_object.assert_called_once()
                link.assert_called_once()
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                self.assertFalse(ctr_match.result_is_current(manifest, config, {**result, "ranges": result["ranges"][:1]}))
                (root / result["ranges"][1]["candidate"]).write_bytes(b"FAIL")
                self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                for flags in ("CONTENTS, ALLOC, LOAD, DATA", "ALLOC"):
                    orphan = headers + "  2 .extra 00000004 80010008 80010008 00001008 2**2\n" + "                  " + flags + "\n"
                    with mock.patch.object(ctr_match, "command_output", side_effect=lambda cmd: orphan if "-h" in cmd else objdump(cmd)):
                        with self.assertRaises(ctr_match.MatchError):
                            cli.build_target(manifest, toolchain, config, root)
                config["data_ranges"] = [intervals[0], intervals[0]]
                with self.assertRaises(ctr_match.MatchError):
                    cli.build_target(manifest, toolchain, config, root)

    def test_build_reports_only_the_linked_data_range_and_tracks_its_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "data.c").write_text("shared initializer")
            (root / "data.ld").write_text("initialized section layout")
            (root / "symbols.txt").write_text("80010000 data\n")
            oracle = root / "SCUS_944.26"
            oracle.write_bytes(b"HHxxxxGOODxxxx")
            config = {
                "name": "data-test", "kind": "data", "artifact": "exe",
                "source": "data.c", "symbol_file": "symbols.txt",
                "linker_script": "data.ld", "output_section": ".initialized",
                "compiler_flags": [],
                "data_range": {"symbol": "data", "symbol_address": "0x80010000", "offset": 4, "size": 4},
            }
            manifest = {
                "target": "test", "targets": [config],
                "artifacts": [{"id": "exe", "kind": "overlay", "path": oracle.name,
                               "size": 14, "sha256": ctr_match.sha256_file(oracle),
                               "load_address": "0x80010000", "mapped_file_offset": 2}],
                "toolchain": {
                    "compiler": {"directory": "gcc", "sha256": {}},
                    "maspsx": {"directory": "maspsx", "sha256": {}, "aspsx_version": "2.77"},
                    "binutils": {"prefix": "mips-", "sha256": {}},
                },
            }
            toolchain = SimpleNamespace(
                compiler_banner="test", compiler_hashes={}, default_aspsx_version="2.77",
                maspsx_commit="pinned", maspsx_hashes={}, assembler_banner="test",
                binutils_hashes={}, config_hash=ctr_match.sha256_json(manifest["toolchain"]),
                binutils={"ld": Path("ld"), "objdump": Path("objdump")},
            )

            def objdump(command):
                if "-t" in command:
                    return "80010000 g .initialized 00000000 data\n"
                if "-h" in command:
                    return (
                        "  0 .initialized 0000000c 80010000 80010000 00001000 2**2\n"
                        "                  CONTENTS, ALLOC, LOAD, DATA\n"
                    )
                return ""

            output = io.StringIO()
            with (
                mock.patch.object(ctr_match, "ROOT", root),
                mock.patch.object(ctr_match, "BUILD", root / "build"),
                mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt"),
                mock.patch.object(ctr_match, "build_object", return_value=(root / "data.o", ["data.c"])),
                mock.patch.object(ctr_match, "run_checked"),
                mock.patch.object(ctr_match, "command_output", side_effect=objdump),
                mock.patch.object(ctr_match, "extract_binary_section",
                                  side_effect=lambda *args: args[-1].write_bytes(b"xxxxGOODxxxx")) as extract,
                redirect_stdout(output),
            ):
                self.assertEqual(ctr_match.configured_targets(manifest, "data"), [config])
                result = cli.build_target(manifest, toolchain, config, root)
                candidate = root / result["ranges"][0]["candidate"]
                self.assertTrue(result["exact"])
                self.assertEqual(candidate.read_bytes(), b"GOOD")
                self.assertEqual(result["scope"], "initialized-data-ranges")
                self.assertEqual(result["ranges"][0]["address"], "0x80010004")
                self.assertEqual(result["expected_size"], 4)
                self.assertEqual(len(result["ranges"]), 1)
                self.assertFalse((root / "build" / "reconstructed").exists())
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                for path in (candidate, root / "data.c", root / "data.ld", root / "symbols.txt"):
                    original = path.read_bytes()
                    path.write_bytes(original + b"changed")
                    self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                    path.write_bytes(original)
                config["data_range"]["offset"] = 5
                self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                config["data_range"]["offset"] = 4
                result["scope"] = "whole-artifact"
                self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                extract.side_effect = lambda *args: args[-1].write_bytes(b"xxxxBADDxxxx")
                mismatch = cli.build_target(manifest, toolchain, config, root)
                self.assertFalse(mismatch["exact"])
                self.assertEqual(
                    mismatch["first_mismatch_location"], "file+0x6 / vram 0x80010004"
                )
                self.assertFalse((root / "build" / "reconstructed").exists())
            self.assertIn("data-test (initialized-data-ranges)", output.getvalue())


class EvidenceTests(unittest.TestCase):
    def test_code_interval_currentness_binds_every_span_and_the_shared_elf(self) -> None:
        config = {"name": "test", "kind": "resident", "artifact": "exe", "source": "source.c",
                  "symbol_file": "symbols.txt", "compiler_flags": [], "linked_block": {},
                  "code_intervals": [{"start_symbol": "Test_First", "end_symbol": "Gap"},
                                     {"start_symbol": "Test_Second", "end_symbol": "End"}]}
        manifest = {"artifacts": [{"id": "exe"}], "toolchain": {"maspsx": {"aspsx_version": "2.77"}}}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source.c").write_text("void Test_First(void) {}\nvoid Test_Second(void) {}\n")
            (root / "symbols.txt").write_text("00001000 Test_First\n00001004 Gap\n00002000 Test_Second\n00002004 End\n")
            for name in ("candidate.bin", "first.bin", "second.bin", "block.elf"):
                (root / name).write_bytes(b"GOOD")
            intervals = [{**specification, "section": f".resident_block_{index}", "address": f"0x{address:08x}",
                          "size": 4, "candidate": candidate, "candidate_sha256": ctr_match.sha256_bytes(b"GOOD")}
                         for index, (specification, address, candidate) in enumerate(zip(config["code_intervals"],
                                                                                       (0x1000, 0x2000), ("first.bin", "second.bin")))]
            with (mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_match, "tool_inputs", return_value={}),
                  mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt"),
                  mock.patch.object(ctr_match, "pipeline_hash", return_value="pipeline")):
                result = {"schema_version": 2, "name": "test", "config_sha256": ctr_match.sha256_json(config),
                          "reference_config_sha256": ctr_match.sha256_json(manifest["artifacts"][0]), "pipeline_sha256": "pipeline",
                          "toolchain_config_sha256": ctr_match.sha256_json(manifest["toolchain"]),
                          "compiler_flags": ctr_match.effective_flags(config), "aspsx_version": "2.77",
                          "build_input_sha256": ctr_match.input_hashes(["source.c", "symbols.txt"]),
                          "functions": [{"name": "Test_First", "candidate": "candidate.bin", "candidate_sha256": ctr_match.sha256_bytes(b"GOOD")}],
                          "selected_function_count": 1, "function_count": 2, "complete": False,
                          "linked_block": {"elf": "block.elf", "elf_sha256": ctr_match.sha256_bytes(b"GOOD"), "code_intervals": intervals}}
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                for name in ("first.bin", "second.bin", "block.elf"):
                    (root / name).write_bytes(b"BAD!")
                    self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                    (root / name).write_bytes(b"GOOD")
                    (root / name).unlink()
                    self.assertFalse(ctr_match.result_is_current(manifest, config, result))
                    (root / name).write_bytes(b"GOOD")
                for replacement in (None, intervals[:1], [{**intervals[0], "section": "*ABS*"}, intervals[1]],
                                    [{**intervals[0], "address": "0x00001004"}, intervals[1]]):
                    changed = {**result, "linked_block": {**result["linked_block"], "code_intervals": replacement}}
                    self.assertFalse(ctr_match.result_is_current(manifest, config, changed))

    def test_resident_currentness_binds_optional_owned_name_including_errors(self) -> None:
        config = {"name": "test", "kind": "resident", "artifact": "exe", "source": "source.c",
                  "symbol_file": "symbols.txt", "compiler_flags": [],
                  "owned_symbols": {"Test_Read": "Actual_Read"}}
        manifest = {"artifacts": [{"id": "exe"}], "toolchain": {"maspsx": {"aspsx_version": "2.77"}}}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "source.c").write_text("void Actual_Read(void) {}\n")
            (root / "symbols.txt").write_text("00001000 Test_Read\n")
            (root / "candidate.bin").write_bytes(b"GOOD")
            with (mock.patch.object(ctr_match, "ROOT", root), mock.patch.object(ctr_match, "tool_inputs", return_value={}),
                  mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt"),
                  mock.patch.object(ctr_match, "pipeline_hash", return_value="pipeline")):
                record = {"name": "Test_Read", "candidate_symbol": "Actual_Read", "candidate": "candidate.bin",
                          "candidate_sha256": ctr_match.sha256_file(root / "candidate.bin")}
                result = {"schema_version": 2, "name": "test", "config_sha256": ctr_match.sha256_json(config),
                          "reference_config_sha256": ctr_match.sha256_json(manifest["artifacts"][0]),
                          "pipeline_sha256": "pipeline", "toolchain_config_sha256": ctr_match.sha256_json(manifest["toolchain"]),
                          "compiler_flags": ctr_match.effective_flags(config), "aspsx_version": "2.77",
                          "build_input_sha256": ctr_match.input_hashes(["source.c", "symbols.txt"]),
                          "functions": [record], "selected_function_count": 1, "function_count": 1, "complete": True}
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                for item in ({**record, "candidate_symbol": "Wrong"}, {k:v for k,v in record.items() if k != "candidate_symbol"},
                             {"name": "Test_Read", "error": "compile failed"}):
                    self.assertFalse(ctr_match.result_is_current(manifest, config, {**result, "functions": [item]}))
                self.assertTrue(ctr_match.result_is_current(manifest, config,
                    {**result, "functions": [{"name": "Test_Read", "candidate_symbol": "Actual_Read", "error": "compile failed"}]}))
                identity_config = {**config, "owned_symbols": {}}
                identity_result = {**result, "config_sha256": ctr_match.sha256_json(identity_config),
                                   "functions": [{k:v for k,v in record.items() if k != "candidate_symbol"}]}
                self.assertTrue(ctr_match.result_is_current(manifest, identity_config, identity_result))
                self.assertFalse(ctr_match.result_is_current(manifest, identity_config,
                    {**identity_result, "functions": [{**identity_result["functions"][0], "candidate_symbol": None}]}))

    def test_evidence_expires_when_an_input_or_candidate_changes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source.c"
            header = root / "header.h"
            candidate = root / "candidate.bin"
            source.write_text("source")
            header.write_text("header")
            candidate.write_bytes(b"code")
            config = {
                "name": "test",
                "kind": "probe",
                "source": "source.c",
                "region": "exe",
                "compiler_flags": [],
            }
            manifest = {
                "target": "test",
                "artifacts": [{"id": "exe", "sha256": "retail"}],
                "toolchain": {
                    "compiler": {"directory": "gcc", "sha256": {}},
                    "maspsx": {
                        "directory": "maspsx",
                        "sha256": {},
                        "aspsx_version": "2.77",
                    },
                    "binutils": {"prefix": "mips-", "sha256": {}},
                },
            }
            tools = SimpleNamespace(
                compiler_banner="2.8.1",
                compiler_hashes={},
                default_aspsx_version="2.77",
                maspsx_commit="pinned",
                maspsx_hashes={},
                assembler_banner="2.40",
                binutils_hashes={},
                config_hash=ctr_match.sha256_json(manifest["toolchain"]),
            )
            with mock.patch.object(ctr_match, "ROOT", root):
                result = {
                    **ctr_match.build_evidence(manifest, tools, config, ["header.h"]),
                    "candidate": "candidate.bin",
                    "candidate_sha256": ctr_match.sha256_file(candidate),
                    "exact": True,
                }
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                for path, content in (
                    (source, "source"),
                    (header, "header"),
                    (candidate, "code"),
                ):
                    path.write_text("changed")
                    self.assertFalse(
                        ctr_match.result_is_current(manifest, config, result)
                    )
                    path.write_text(content)
                del result["build_input_sha256"]["source.c"]
                self.assertFalse(ctr_match.result_is_current(manifest, config, result))

    def test_mixed_input_evidence_binds_each_resolved_recipe(self) -> None:
        config = {"name": "sdk", "kind": "resident", "artifact": "exe",
                  "sources": ["core.c", "wrapper.c", "entry.s"], "symbol_file": "symbols.txt",
                  "compiler_flags": ["-O2", "-G0"],
                  "source_flags": {"wrapper.c": ["-fno-delayed-branch"]}}
        manifest = {"target": "test", "artifacts": [{"id": "exe"}], "toolchain": {
            "compiler": {"directory": "gcc", "sha256": {}},
            "maspsx": {"directory": "maspsx", "sha256": {}, "aspsx_version": "2.77"},
            "binutils": {"prefix": "mips-", "sha256": {}},
        }}
        tools = SimpleNamespace(compiler_banner="test", compiler_hashes={}, default_aspsx_version="2.77",
                                maspsx_commit="pinned", maspsx_hashes={}, assembler_banner="test",
                                binutils_hashes={}, config_hash=ctr_match.sha256_json(manifest["toolchain"]))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in (*config["sources"], "symbols.txt"):
                (root / name).write_text("input")
            (root / "candidate.bin").write_bytes(b"code")
            with (mock.patch.object(ctr_match, "ROOT", root),
                  mock.patch.object(ctr_match, "SYMBOL_FILE", root / "symbols.txt")):
                result = {**ctr_match.build_evidence(manifest, tools, config, config["sources"]),
                          "functions": [{"name": "CdReset", "candidate": "candidate.bin",
                                         "candidate_sha256": ctr_match.sha256_file(root / "candidate.bin")}],
                          "selected_function_count": 1, "function_count": 1, "complete": True}
                self.assertEqual(result["source_compiler_flags"], {
                    "core.c": ["-O2", "-G0", "-ffunction-sections"],
                    "wrapper.c": ["-O2", "-G0", "-fno-delayed-branch", "-ffunction-sections"],
                })
                self.assertTrue(ctr_match.result_is_current(manifest, config, result))
                changed = copy.deepcopy(result)
                changed["source_compiler_flags"]["wrapper.c"].remove("-fno-delayed-branch")
                self.assertFalse(ctr_match.result_is_current(manifest, config, changed))
                del changed["source_compiler_flags"]
                self.assertFalse(ctr_match.result_is_current(manifest, config, changed))

    def test_partial_resident_result_is_reported_as_partial(self) -> None:
        config = {"name": "vehicle", "kind": "resident"}
        result = {
            "complete": False,
            "exact": False,
            "selected_exact": True,
            "exact_function_count": 1,
            "function_count": 129,
        }
        args = cli.build_parser().parse_args(["status"])
        output = io.StringIO()
        with (
            mock.patch.object(
                cli,
                "load_json",
                side_effect=[{"target": "test", "targets": [config]}, result],
            ),
            mock.patch.object(cli, "result_path", return_value=Path("result.json")),
            mock.patch.object(cli, "result_is_current", return_value=True),
            redirect_stdout(output),
        ):
            self.assertEqual(cli.cmd_status(args), 0)
        self.assertIn("PARTIAL", output.getvalue())
        self.assertIn("1/129", output.getvalue())
        self.assertNotIn("MATCH", output.getvalue())


class PsxHeaderTests(unittest.TestCase):
    def test_header_contract(self) -> None:
        artifact = {
            "id": "exe",
            "load_address": "0x80010000",
            "mapped_size": "0x7d800",
            "header": {
                "entrypoint": "0x8007793c",
                "global_pointer": "0",
                "stack_pointer": "0x801ffff0",
            },
        }
        header = bytearray(0x800)
        header[:8] = b"PS-X EXE"
        struct.pack_into("<I", header, 0x10, 0x8007793C)
        struct.pack_into("<I", header, 0x14, 0)
        struct.pack_into("<I", header, 0x18, 0x80010000)
        struct.pack_into("<I", header, 0x1C, 0x7D800)
        struct.pack_into("<I", header, 0x30, 0x801FFFF0)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "SCUS_944.26"
            path.write_bytes(header)
            self.assertEqual(ctr_match.check_psx_exe_header(path, artifact), [])


if __name__ == "__main__":
    unittest.main()
