"""Explicit pinned-tool tests: python3 tools/matching/integration_match.py --output NEW_DIR.

Requires an executable manifest tool tuple; unsupported hosts fail, never skip.
Owned synthetic comparators test metadata/placement, not retail game bytes.
The existing assembler helper does not explicitly select a soft-float ELF tag.
"""
from __future__ import annotations

import argparse
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
        fixture = Path(__file__).resolve().parents[2] / "externals/old-gcc/tests/psx_continuations.c"
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


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=matching.DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, required=True, help="fresh persistent output directory")
    args, remaining = parser.parse_known_args()
    MANIFEST, OUTPUT = args.manifest.resolve(), args.output.resolve()
    unittest.main(argv=[sys.argv[0], *remaining], failfast=True)
