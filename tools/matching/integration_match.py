"""Explicit pinned-tool tests: python3 tools/matching/integration_match.py --output NEW_DIR.

Requires an executable manifest tool tuple; unsupported hosts fail, never skip.
Owned synthetic comparators test metadata/placement, not retail game bytes.
The existing assembler helper does not explicitly select a soft-float ELF tag.
"""
from __future__ import annotations

import argparse
import copy
from contextlib import contextmanager, redirect_stdout
import io
import struct
import tempfile
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pipeline as matching
import resident


class AdjacentToolTests(unittest.TestCase):
    @classmethod
    def checked(cls, argv, stdin_path=None):
        cls.serial += 1
        log = cls.root / "commands" / f"{cls.serial:03d}"
        result = subprocess.run([str(arg) for arg in argv],
            input=Path(stdin_path).read_bytes() if stdin_path else None,
            capture_output=True, timeout=30,
            env={"PATH": "/usr/bin:/bin", "LANG": "C", "LC_ALL": "C", "TZ": "UTC",
                 "PYTHONDONTWRITEBYTECODE": "1"})
        log.with_suffix(".stdout").write_bytes(result.stdout)
        log.with_suffix(".stderr").write_bytes(result.stderr)
        log.with_suffix(".json").write_text(json.dumps(
            {"argv": [str(arg) for arg in argv], "exit": result.returncode}, indent=2) + "\n")
        if result.returncode:
            raise matching.MatchError(f"command failed ({result.returncode}): {argv}\n"
                                     + result.stderr.decode(errors="replace"))

    @classmethod
    def setUpClass(cls):
        cls.root = OUTPUT
        cls.root.mkdir(parents=True)
        (cls.root / "commands").mkdir()
        cls.serial = 0
        cls.tools = matching.resolve_toolchain(matching.load_json(MANIFEST))
        fixture = matching.ROOT / "externals/old-gcc/tests/psx_continuations.c"
        with mock.patch.object(matching, "run_checked", side_effect=cls.checked):
            for name, extra in (("pair", []), ("missing", ["-DOMIT_CONTINUE"]),
                                ("ordinary", ["-DORDINARY_CALL"])):
                asm, obj = cls.root / (name + ".s"), cls.root / (name + ".o")
                matching.compile_c(cls.tools, fixture, asm,
                    ["-O2", "-G0", "-msoft-float", "-ffunction-sections", *extra])
                matching.assemble_compiler_output(cls.tools, asm, obj, "2.77", 0)
        functions = resident.object_functions(cls.tools.binutils["objdump"], cls.root / "pair.o")
        cls.first, cls.second = functions["Begin"]["size"], functions["Continue"]["size"]
        cls.functions = [resident.FunctionRange("Begin", 0x100000, cls.first, fixture.name),
                        resident.FunctionRange("Continue", 0x100000 + cls.first, cls.second, fixture.name)]
        cls.checked([cls.tools.binutils["objcopy"], "--remove-section", ".mips.adjacent",
                     cls.root / "pair.o", cls.root / "bare.o"])

    def link(self, name, objects, *, padding=0, drop=False):
        symbols = self.root / (name + ".symbols")
        symbols.write_text(f"00100000 Begin\n{0x100000 + self.first:08x} Continue\n"
                           f"{0x100000 + self.first + self.second:08x} End\n")
        config = {"name": name, "artifact": "exe", "symbol_file": symbols.name,
                  "linked_block": {}, "code_intervals": [{"start_symbol": "Begin", "end_symbol": "End"}]}
        if padding or drop:
            info = "" if drop else ".mips.adjacent 0 (INFO) : { KEEP(*(.mips.adjacent)) }"
            script = self.root / (name + ".ld")
            script.write_text("SECTIONS { .resident_block_0 0x100000 : SUBALIGN(4) { "
                f"*(.Begin) . += {padding}; *(.Continue) }} {info} /DISCARD/ : {{ *(*) }} }}\n")
            config["linker_script"] = script.name
        with (mock.patch.object(matching, "ROOT", self.root),
              mock.patch.object(resident, "BUILD_ROOT", self.root / "build"),
              mock.patch.object(matching, "run_checked", side_effect=self.checked),
              mock.patch.object(matching, "reference_bytes", side_effect=lambda _m, _r, span: bytes(span["size"]))):
            return resident.link_contiguous_block({}, self.tools, config, self.root, self.functions, objects)

    def metadata(self, name, body, flags=""):
        source, obj = self.root / (name + ".s"), self.root / (name + ".o")
        source.write_text(f'.section .mips.adjacent,"{flags}",@progbits\n{body}\n')
        self.checked([self.tools.binutils["as"], "-G0", "-o", obj, source])
        return obj

    def test_valid_INFO_and_isolation(self):
        result = self.link("valid", [self.root / "pair.o"])
        self.assertTrue(result["code_placement_exact"])
        self.assertFalse(result["exact"])
        self.assertEqual(resident.adjacent_metadata_size(self.tools.binutils["objdump"],
                         self.root / "build/valid/linked_block/block.elf"), 8)
        with self.assertRaisesRegex(matching.MatchError, "not an isolated function"):
            resident.link_function({"name": "isolated"}, self.tools, [], self.functions[0],
                                   self.root / "pair.o", [], b"")
        self.assertEqual(resident.adjacent_metadata_size(self.tools.binutils["objdump"],
                                                       self.root / "ordinary.o"), 0)

    def test_missing_and_correct_address_absolute_FUNC(self):
        with self.assertRaisesRegex(matching.MatchError, "selected real positive"):
            self.link("missing", [self.root / "missing.o"])
        source, obj = self.root / "absolute.s", self.root / "absolute.o"
        source.write_text(".globl Continue\n.type Continue,@function\n"
                          f".equ Continue,{0x100000 + self.first}\n.size Continue,{self.second}\n")
        self.checked([self.tools.binutils["as"], "-G0", "-o", obj, source])
        with self.assertRaisesRegex(matching.MatchError, "selected real positive"):
            self.link("absolute", [self.root / "missing.o", obj])

    def test_equal_interior_and_duplicate(self):
        for name, body in (("interior", f".word Begin+{self.first}+4,Continue+4"),
                           ("duplicate", f".word Begin+{self.first},Continue\n" * 2)):
            obj = self.metadata(name, body)
            with self.subTest(name=name), self.assertRaisesRegex(matching.MatchError, "unique distinct function end"):
                self.link(name, [self.root / "bare.o", obj])

    def test_badsize_allocated_padding_and_discard(self):
        for name, obj in (("badsize", self.metadata("badsize", ".byte 0")),
                          ("allocated", self.metadata("allocated", f".word Begin+{self.first},Continue", "a"))):
            with self.subTest(name=name), self.assertRaisesRegex(matching.MatchError, "nonallocated eight-byte"):
                self.link(name, [self.root / "bare.o", obj])
        with self.assertRaisesRegex(matching.MatchError, "unique distinct function end"):
            self.link("padding", [self.root / "pair.o"], padding=4)
        with self.assertRaisesRegex(matching.MatchError, "record count differs"):
            self.link("discard", [self.root / "pair.o"], drop=True)



class StoragePaddingToolTests(unittest.TestCase):
    @contextmanager
    def real_recipe(self, *, layout_change=None, config_change=None, source_change=None, extra_source="", oracle_padding=bytes(4)):
        """Only filesystem location is adapted; compilation/assembly/link are real pinned tools."""
        pinned = matching.load_json(MANIFEST)
        tools = matching.resolve_toolchain(pinned)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "assets").mkdir()
            source = 'unsigned int state __attribute__((section(".bss.state")));\n__asm__(".type state,@object\\n.size state,4\\n");\nint Test_Read(void) { return 0; }\n' + extra_source
            if source_change:
                source = source_change(source)
            (root / "source.c").write_text(source)
            interval_size = 8 + len(oracle_padding)
            (root / "symbols.txt").write_text(f"80010000 Test_Read\n{0x80010000 + interval_size:08x} End\n")
            layout = 'SECTIONS { .resident_block_0 0x80010000 : SUBALIGN(4) { *(.Test_Read) . = 12; } = 0\n .bss.state 0x80020000 (NOLOAD) : SUBALIGN(1) { *(.bss.state) }\n /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.comment) } }\n'
            if layout_change:
                layout = layout_change(layout)
            (root / "test.ld").write_text(layout)
            # Independent hand-specified MIPS return/zero value plus explicit tail.
            payload = struct.pack("<II", 0x03e00008, 0x00001021) + oracle_padding
            header = bytearray(0x800)
            header[:8] = b"PS-X EXE"
            (root / "assets/oracle.bin").write_bytes(header + payload)
            config = {"name": "test", "kind": "resident", "artifact": "exe", "source": "source.c", "symbol_file": "symbols.txt",
                      "symbol_prefix": "Test_", "compiler_flags": ["-O2", "-G0", "-msoft-float", "-fno-common"],
                      "include_directories": [], "small_data_limit": 0, "aspsx_version": "2.77", "linker_script": "test.ld", "linked_block": {},
                      "code_intervals": [{"start_symbol": "Test_Read", "end_symbol": "End"}], "function_padding": {"Test_Read": 4},
                      "bss_ranges": [{"output_section": ".bss.state", "symbol": "state", "symbol_address": "0x80020000", "offset": 0, "size": 4}]}
            if config_change:
                config_change(config)
            manifest = {"target": "real-mips-fixture", "reference_root": "assets", "toolchain": pinned["toolchain"], "targets": [config],
                        "artifacts": [{"id": "exe", "kind": "ps-x-exe", "path": "oracle.bin", "size": 0x800 + interval_size,
                                       "load_address": "0x80010000", "mapped_file_offset": "0x800", "mapped_size": interval_size,
                                       "sha256": matching.sha256_file(root / "assets/oracle.bin")}]}
            with (mock.patch.object(matching, "ROOT", root), mock.patch.object(matching, "BUILD", root / "build"),
                  mock.patch.object(matching, "SYMBOL_FILE", root / "symbols.txt"),
                  mock.patch.object(matching, "tool_inputs", return_value={}), mock.patch.object(matching, "resolve_toolchain", return_value=tools),
                  mock.patch.object(resident, "BUILD_ROOT", root / "build/namespaces"), redirect_stdout(io.StringIO())):
                result = resident.build_resident(manifest, tools, config, root / "assets")
                yield root, manifest, config, tools, result

    def test_real_pinned_mips_recipe_proves_nobits_bodies_and_zero_padding(self):
        with self.real_recipe() as (root, manifest, config, tools, result):
            self.assertTrue(result["exact"])
            block = result["linked_block"]
            self.assertEqual(block["code"], {"matching_bytes": 8, "expected_size": 8, "exact": True})
            self.assertEqual(block["padding"], {"size": 4, "exact": True})
            self.assertEqual(block["layout"], {"matching_bytes": 12, "expected_size": 12, "exact": True})
            self.assertEqual(block["bss"], {"size": 4, "owner_count": 1, "placement_exact": True})
            self.assertEqual(result["functions"][0]["owner"]["size"], 8)
            self.assertEqual(block["bss_ranges"][0]["section"]["elf_type"], 8)
            self.assertTrue(matching.result_is_current(manifest, config, result))

    def test_new_features_reject_mixed_data_and_rodata_at_build_and_cache_entry(self):
        with self.real_recipe() as (root, manifest, config, tools, result):
            changes = (
                {"data_ranges": [{"output_section": ".data", "symbol": "other", "symbol_address": "0x80010000", "offset": 0, "size": 4}]},
                {"data_range": {"symbol": "other", "symbol_address": "0x80010000", "offset": 0, "size": 4}},
                {"linked_block": {"rodata_address": "0x80030000", "rodata_size": 4}},
                {"linked_block": {"rodata_size": 4}},
            )
            for change in changes:
                mixed = {**config, **change}
                forged = {**result, "config_sha256": matching.sha256_json(mixed)}
                with self.subTest(change=change), mock.patch.object(resident, "compile_source") as compile:
                    with self.assertRaisesRegex(matching.MatchError, "does not support mixed"):
                        resident.build_resident(manifest, tools, mixed, root / "assets")
                    compile.assert_not_called()
                    self.assertFalse(matching.result_is_current(manifest, mixed, forged))
            for value in (None, {}, [None], [{"symbol": "state"}]):
                malformed = {**config, "bss_ranges": value}
                forged = {**result, "config_sha256": matching.sha256_json(malformed)}
                with self.subTest(malformed=value), self.assertRaises(matching.MatchError):
                    resident.build_resident(manifest, tools, malformed, root / "assets")
                self.assertFalse(matching.result_is_current(manifest, malformed, forged))

    def test_real_recipe_rejects_initialized_bss_and_bad_layout_or_owners(self):
        cases = {
            "initialized-zero-is-not-BSS": {"layout_change": lambda text: text.replace("(NOLOAD)", "")},
            "wrong-address": {"config_change": lambda config: config["bss_ranges"][0].update(symbol_address="0x80020004")},
            "wrong-width": {"config_change": lambda config: config["bss_ranges"][0].update(size=8)},
            "duplicate-owner": {"config_change": lambda config: config["bss_ranges"].append(copy.deepcopy(config["bss_ranges"][0]))},
            "ABS-substitute": {"config_change": lambda config: config.update(link_symbols={"state": "0x80020000"})},
            "COMMON-input-is-not-definition": {"config_change": lambda config: config["compiler_flags"].remove("-fno-common"),
                                               "source_change": lambda text: text.replace('unsigned int state __attribute__((section(".bss.state")));', 'unsigned int state;')},
            "nonzero-candidate-tail": {"layout_change": lambda text: text.replace("} = 0", "} = 1")},
            "nonzero-oracle-tail": {"oracle_padding": b"BAD!"},
            "forged-body-length": {"config_change": lambda config: config["function_padding"].update(Test_Read=0)},
            "unallocated-BSS": {"layout_change": lambda text: text.replace("(NOLOAD)", "(INFO)")},
            "undeclared-owner": {"extra_source": 'unsigned int extra __attribute__((section(".bss.state")));\n__asm__(".type extra,@object\\n.size extra,4\\n");\n'},
            "CODE-OBJECT-body-alias": {"extra_source": '__asm__(".globl alias\\n.set alias,Test_Read\\n.type alias,@object\\n.size alias,4\\n");\n'},
            "CODE-NOTYPE-body-alias": {"extra_source": '__asm__(".globl alias\\n.set alias,Test_Read\\n.type alias,@notype\\n.size alias,4\\n");\n'},
            "CODE-OBJECT-padding-alias": {"extra_source": '__asm__(".globl alias\\n.set alias,Test_Read+8\\n.type alias,@object\\n.size alias,4\\n");\n'},
            "CODE-NOTYPE-padding-alias": {"extra_source": '__asm__(".globl alias\\n.set alias,Test_Read+8\\n.type alias,@notype\\n.size alias,4\\n");\n'},
        }
        for name, arguments in cases.items():
            with self.subTest(name=name), self.real_recipe(**arguments) as (_, _, _, _, result):
                self.assertFalse(result["exact"])
                if name == "forged-body-length":
                    self.assertEqual(result["functions"][0]["owner"]["size"], 8)
                    self.assertEqual(result["functions"][0]["expected_size"], 12)
                else:
                    self.assertTrue(result["linked_block"].get("error"), "negative must fail ownership/padding, not silently lose comparison bytes")

    def test_cached_proof_tampering_is_checked_against_real_elf(self):
        with self.real_recipe() as (root, manifest, config, tools, result):
            self.assertTrue(matching.result_is_current(manifest, config, result))
            mutations = [
                lambda r: r["linked_block"]["bss_ranges"][0]["owner"].update(size=8),
                lambda r: r["linked_block"]["bss_ranges"][0]["section"].update(elf_type=1),
                lambda r: r["linked_block"]["bss_ranges"][0].update(file_bytes=4),
                lambda r: r["linked_block"].update(bss_ranges=[]),
                lambda r: r["linked_block"]["bss"].update(size=8),
                lambda r: r["linked_block"]["padding"].update(size=0),
                lambda r: r["linked_block"]["function_padding"][0].update(size=8),
                lambda r: r["linked_block"]["code"].update(expected_size=12),
                lambda r: r["linked_block"]["layout"].update(expected_size=8),
                lambda r: r["functions"][0].update(body_size=12),
                lambda r: r["functions"][0]["owner"].update(size=12),
                lambda r: r["functions"][0].update(matching_bytes=0),
                lambda r: r["functions"][0].update(name="Unknown"),
                lambda r: r["linked_block"].update(input_owners=[]),
                lambda r: r["linked_block"].update(rodata_address="0x80030000", rodata_exact=False),
                lambda r: r["linked_block"].update(data_ranges=[], data={"exact": False}),
                lambda r: r["linked_block"]["input_owners"][0]["owners"]["state"].update(type="NOTYPE"),
                lambda r: r.update(exact_function_count=0),
            ]
            for index, mutate in enumerate(mutations):
                changed = copy.deepcopy(result)
                mutate(changed)
                with self.subTest(mutation=index):
                    self.assertFalse(matching.result_is_current(manifest, config, changed))
            forged_config = copy.deepcopy(config)
            forged_config["function_padding"]["Test_Read"] = 0
            forged = copy.deepcopy(result)
            forged["config_sha256"] = matching.sha256_json(forged_config)
            forged["functions"][0].update(body_size=12, padding_size=0)
            forged["functions"][0]["owner"]["size"] = 12
            self.assertFalse(matching.result_is_current(manifest, forged_config, forged))
            path = root / result["linked_block"]["elf"]
            output_owners = matching.linked_symbols(tools.binutils["objdump"], path, 0)
            forged = copy.deepcopy(result)
            forged["linked_block"]["input_owners"] = [{
                "object": result["linked_block"]["elf"], "sha256": matching.sha256_file(path),
                "owners": {"state": output_owners["state"]},
            }]
            self.assertFalse(matching.result_is_current(manifest, config, forged))
            object_path = root / result["linked_block"]["input_owners"][0]["object"]
            original_object = object_path.read_bytes()
            try:
                object_path.write_bytes(path.read_bytes())
                forged["linked_block"]["input_owners"][0].update(
                    object=result["linked_block"]["input_owners"][0]["object"],
                    sha256=matching.sha256_file(object_path))
                self.assertFalse(matching.result_is_current(manifest, config, forged))
            finally:
                object_path.write_bytes(original_object)
            path = root / result["linked_block"]["elf"]
            raw = bytearray(path.read_bytes())
            section_offset = struct.unpack_from("<I", raw, 32)[0]
            section_count = struct.unpack_from("<H", raw, 48)[0]
            for index in range(section_count):
                offset = section_offset + index * 40
                if struct.unpack_from("<I", raw, offset + 4)[0] == 8:
                    struct.pack_into("<I", raw, offset + 4, 1)
                    break
            path.write_bytes(raw)
            forged = copy.deepcopy(result)
            forged["linked_block"]["elf_sha256"] = matching.sha256_file(path)
            self.assertFalse(matching.result_is_current(manifest, config, forged))

    def test_bss_only_cached_code_totals_are_replayed(self):
        with self.real_recipe(
            config_change=lambda config: config.pop("function_padding"),
            layout_change=lambda text: text.replace(". = 12", ". = 8"),
            oracle_padding=b"",
        ) as (_, manifest, config, _, result):
            self.assertTrue(result["exact"])
            self.assertTrue(matching.result_is_current(manifest, config, result))
            forged = copy.deepcopy(result)
            forged["linked_block"]["code"] = {"expected_size": 0, "matching_bytes": 123456, "exact": False}
            self.assertFalse(matching.result_is_current(manifest, config, forged))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=matching.DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, required=True, help="fresh persistent output directory")
    args, remaining = parser.parse_known_args()
    MANIFEST, OUTPUT = args.manifest.resolve(), args.output.resolve()
    unittest.main(argv=[sys.argv[0], *remaining], failfast=True)
