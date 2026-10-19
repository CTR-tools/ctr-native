import unittest

from maspsx import MaspsxProcessor, cop2_read_dependency

from .util import strip_comments


class TestCop2Read(unittest.TestCase):
    def test_cpu_operands_and_register_aliases(self):
        for line, destination, expected in (
            ("sw $v0,4($a0)", "$2", True),
            ("sw $3,4($2)", "$v0", True),
            ("xor $3,$4,$2", "$v0", True),
            ("xori $3,$v0,1", "$2", True),
            ("addiu $3,$s8,4", "$30", True),
            ("jr $ra", "$31", True),
            ("jalr $31,$v0", "$2", True),
            ("lwl $v0,0($4)", "$2", True),
            ("mtc2 $v0,$14", "$2", True),
            ("ctc2 $2,$31", "$v0", True),
            ("lwc2 $14,0($v0)", "$2", True),
            ("swc2 $14,0($2)", "$v0", True),
            ("beq $2,$3,$L1", "$v0", True),
            ("bne $3,$2,$L1", "$v0", True),
            ("beq $4,$5,$L2", "$2", False),
            ("bne $4,$5,$mystery", "$2", False),
            ("lwc2 $2,0($4)", "$2", False),
            ("swc2 $2,0($4)", "$2", False),
            ("mtc2 $4,$2", "$2", False),
            ("ctc2 $4,$2", "$2", False),
            ("mfc2 $3,$2", "$2", False),
            ("cfc2 $3,$2", "$2", False),
            ("cop2 0x280030", "$2", False),
            ("nop", "$2", False),
            ("addu $3,$4,$5", "$2", False),
            ("sw $0,0($4)", "$zero", False),
            ("j $L1", "$2", False),
            ("jal $Lb2", "$2", False),
            ("jal $Le3", "$2", False),
            ("j $v0", "$2", True),
            ("jal $31,$2", "$2", True),
            ("opaque $2", "$2", None),
            ("addu $3,$mystery,$4", "$2", None),
            ("j $mystery", "$2", None),
            ("jr $L1", "$2", None),
        ):
            with self.subTest(line=line, destination=destination):
                self.assertIs(cop2_read_dependency(line, destination), expected)
        with self.assertRaises(ValueError):
            cop2_read_dependency("nop", "$mystery")

    def test_reorder_load_delays(self):
        for op in ("mfc2", "cfc2"):
            for following, expected in (
                (["#nop", "sw\t$2,4($4)"], ["nop", "sw\t$2,4($4)"]),
                (["nop", "sw\t$2,4($4)"], ["nop", "sw\t$2,4($4)"]),
                (["xor\t$3,$v0,$4"], ["nop", "xor\t$3,$v0,$4"]),
                (["move\t$3,$4", "sw\t$2,0($5)"],
                 ["addu\t$3,$4,$zero", "sw\t$2,0($5)"]),
                (["opaque\t$3"], ["nop", "opaque\t$3"]),
                (["lwc2\t$2,0($4)"], ["lwc2\t$2,0($4)"]),
                (["j\t$L900"], ["j\t$L900", "nop"]),
                ([".set\tnoreorder", "j\t$31", "sw\t$2,4($4)"],
                 ["j\t$31", "sw\t$2,4($4)"]),
            ):
                transfer = op + "\t$2,$14"
                with self.subTest(op=op, following=following):
                    self.assertEqual(self.process([transfer] + following),
                                     [transfer] + expected)

    def test_expansions_and_literal_noreorder(self):
        for lines, options, expected in (
            ([".comm\tvalue,4", "mfc2\t$2,$14", "sw\t$2,value"],
             {"sdata_limit": 8},
             ["mfc2\t$2,$14", "nop", "sw\t$2,%gp_rel(value)($gp)",
              ".section .sbss", ".align 2", "value:", ".space 4"]),
            (["mfc2\t$2,$14", "sw\t$2,external_value"], {},
             ["mfc2\t$2,$14", "sw\t$2,external_value"]),
            # Explicit noreorder is the author's responsibility, even if unsafe.
            ([".set\tnoreorder", "mfc2\t$2,$14", "sw\t$2,0($4)"], {},
             ["mfc2\t$2,$14", "sw\t$2,0($4)"]),
            (["lw\t$2,0($4)", "sw\t$2,4($4)"], {},
             ["lw\t$2,0($4)", "nop", "sw\t$2,4($4)"]),
            (["lw\t$2,0($4)", "sw\t$3,4($4)"], {},
             ["lw\t$2,0($4)", "sw\t$3,4($4)"]),
        ):
            with self.subTest(lines=lines, options=options):
                self.assertEqual(self.process(lines, **options), expected)
        with self.assertRaisesRegex(ValueError, "COP2 reads must be separate"):
            self.process(["mfc2 $2,$14;sw $2,0($4)"])
        for separator in (" ", "\t"):
            with self.subTest(separator=separator):
                output = self.process([
                    ".set" + separator + "noreorder # explicit instruction order",
                    "mfc2\t$2,$14", "sw\t$2,0($4)",
                    ".set" + separator + "reorder # restore scheduling",
                    "mfc2\t$2,$14", "sw\t$2,4($4)",
                ])
                self.assertEqual([line for line in output if not line.startswith(".set")],
                                 ["mfc2\t$2,$14", "sw\t$2,0($4)",
                                  "mfc2\t$2,$14", "nop", "sw\t$2,4($4)"])
        self.assertEqual(self.process([".set $14,$12", "addu $2,$14,$4"]),
                         [".set $14,$12", "addu $2,$14,$4"])

    @staticmethod
    def process(lines, **options):
        return strip_comments(MaspsxProcessor(lines, **options).process_lines())
