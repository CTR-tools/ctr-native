import struct
import re

from typing import List

branch_mnemonics = {
    "beq",
    "bgez",
    "bgtz",
    "blez",
    "bltz",
    "bne",
}
jump_mnemonics = {
    "j",
    "jal",
}
load_mnemonics = {
    "lb",
    "lbu",
    "lh",
    "lhu",
    "lw",
    "lwl",
    "lwr",
}
store_mnemonics = {
    "sb",
    "sh",
    "sw",
    "swl",
    "swr",
    "swc2",
}

single_reg_loads = {
    "mult",
    "multu",
    "div",
    "divu",
    "rem",
    "remu",
    "move",
    "negu",
    "nor",
}
double_reg_loads = {
    "and",
    "andi",
    "or",
    "ori",
    "xor",
    "xori",
    "addu",
    "subu",
    "sll",
    "slr",
    "srl",
    "sra",
    "slt",
    "slti",
    "sltu",
}


def strip_comments(line: str) -> str:
    if line.count("#") > 0:
        line = line.split("#")[0]
    return line.strip()


def line_loads_from_reg(line: str, r_source: str) -> bool:
    """
    NOTE: Returns True even if line might use $at expansion
    """
    line = strip_comments(line)

    # escape dollar
    r_source = r_source.replace("$", r"\$")

    if match := re.match(r"^([A-z][A-z0-9]*)\s+(.*)$", line):
        op, rest = match.group(1, 2)
    else:
        return False

    if op in load_mnemonics:
        # lwl	$9,7($2)
        if re.match(rf"^.*\(\s*{r_source}\s*\)$", rest):
            return True

    elif op in store_mnemonics:
        if re.match(rf"^.*\(\s*{r_source}\s*\)$", rest):
            return True
        # "line_loads_from_reg" is a bit of a lie
        if re.match(rf"^{r_source},.*$", rest):
            return True

    elif op == "lwc2":
        # lwc2 $5, 4( $4
        if re.match(rf"^.*\(\s*{r_source}\s*\)$", rest):
            return True

    elif op == "jal":
        if re.match(rf"^.*,\s*{r_source}$", rest):
            return True

    elif op == "j":
        if re.match(rf"^{r_source}$", rest):
            return True

    elif op in ("ctc2", "mtc0", "mtc2"):
        if re.match(rf"^{r_source},.*$", rest):
            return True

    elif op in ("mtlo", "mthi"):
        if re.match(rf"^{r_source}$", rest):
            return True

    elif op in branch_mnemonics:
        if re.match(rf"^{r_source},.*$", rest):
            return True
        if re.match(rf"^.*,\s*{r_source},.*$", rest):
            return True

    elif op in single_reg_loads:
        if re.match(rf"^.*,\s*{r_source}$", rest):
            return True
        if op.startswith("mult"):
            if re.match(rf"^{r_source},.*$", rest):
                return True
        if op.startswith("div") or op.startswith("rem"):
            # e.g. div	$3,$3,$7
            if re.match(rf"^.*,{r_source}.*$", rest):
                return True

    elif op in double_reg_loads:
        if re.match(rf"^.*,\s*{r_source},.*$", rest):
            return True
        if re.match(rf"^.*,.*,\s*{r_source}$", rest):
            return True

    return False


def cop2_read_dependency(line: str, destination: str):
    """Transfer-only CPU dependency: True/False, or None for an opaque op."""
    names = (
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
    )
    aliases = dict(zip(names, range(32)))
    aliases["s8"] = 30

    def register(token):
        token = token.removeprefix("$")
        if token.isdecimal():
            value = int(token)
            return value if 0 <= value < 32 else None
        return aliases.get(token)

    destination_id = register(destination)
    if destination_id is None:
        raise ValueError(f"Unknown COP2 CPU destination: {destination}")
    if destination_id == 0:
        return False
    line = strip_comments(line).split(";", 1)[0].strip()
    if not line:
        return None
    op, *rest = line.split(None, 1)
    args = [part.strip() for part in rest[0].split(",")] if rest else []
    if op in ("j", "jal") and len(args) == 1 and re.fullmatch(r"\$L(b|e)?\d+", args[0]):
        return False
    if op in ("nop", "li", "lui", "mflo", "mfhi", "mfc2", "cfc2", "cop2"):
        return False
    if op in ("lwc2", "swc2"):
        sources = args[1:]
    elif op in ("mtc2", "ctc2", "mtc0", "mtlo", "mthi"):
        sources = args[:1]
    elif op in load_mnemonics:
        sources = args[1:] + (args[:1] if op in ("lwl", "lwr") else [])
    elif op in ("sb", "sh", "sw", "swl", "swr", "mult", "multu"):
        sources = args
    elif op in ("beq", "bne"):
        sources = args[:2]
    elif op in ("div", "divu", "rem", "remu"):
        sources = args[1:] if len(args) == 3 else args
    elif op in (
        "add", "addu", "addi", "addiu", "sub", "subu", "and", "andi",
        "or", "ori", "xor", "xori", "nor", "slt", "sltu", "slti", "sltiu",
        "sll", "srl", "sra", "sllv", "srlv", "srav", "move", "neg", "negu", "not",
    ):
        sources = args[1:]
    elif op in ("j", "jr", "bgez", "bgtz", "blez", "bltz", "beqz", "bnez"):
        sources = args[:1]
    elif op in ("jal", "jalr"):
        sources = args[-1:]
    else:
        return None
    registers = [register(token) for arg in sources for token in re.findall(r"\$[A-Za-z0-9]+", arg)]
    if any(value is None for value in registers):
        return None
    return destination_id in registers


def is_number(value: str) -> bool:
    if re.match(r"^-?\d+$", value) or re.match(r"^-?0x[A-Fa-f0-9]+$", value):
        return True
    return False


def uses_at(line: str) -> bool:
    line = strip_comments(line)

    # sw	$2,%lo(s_attr)($3)
    if match := re.match(r"^s[wbh]\s+(\$[a-z0-9]+),\s*%lo\(([^(]+)\)\(([^)]+)\)", line):
        return False

    # sw	$2,D_801813A4
    # sw	$3,g_CurrentRoom+40
    # sw	$2,D_us_8017863C.4
    if match := re.match(r"^s[wbh]\s+(\$[a-z0-9]+),\s*(-?[A-z0-9_.+]+)$", line):
        operand = match.group(2)
        if not is_number(operand):
            return True

    # sb	$2,g_InputSaveName($3)
    # sw	$2,-26($16)
    elif match := re.match(r"^s[wbh]\s+(\$[a-z0-9]+),\s*([^(]+)\(([^)]+)\)", line):
        operand = match.group(2)

    # lw	$2,-1000($16)
    elif match := re.match(r"l[a-z]+\s+(\$[a-z0-9]+),\s*([^(]+)\(([^)]+)\)", line):
        operand = match.group(2)

    else:
        return False

    if is_number(operand):
        num = int(operand, 0)
        if -32769 < num < 32768:
            return False

    return True


def parse_load_or_store(rest: str):
    if match := re.match(r"(\$[a-z0-9]+),\s*%lo\(([^(]+)\)\(([^(]+)\)", rest):
        r_dest, operand, r_source = match.group(1, 2, 3)
        needs_expanding = False
    elif match := re.match(r"(\$[a-z0-9]+),\s*([^(]+)\(([^)]+)\)", rest):
        r_dest, operand, r_source = match.group(1, 2, 3)
        needs_expanding = True
    elif match := re.match(r"(\$[a-z0-9]+),\s*([^(]+)", rest):
        r_dest, operand = match.group(1, 2)
        r_source = None
        needs_expanding = True
    else:
        raise Exception(f"Unable to parse load/store instruction: {rest}")

    if re.match(r"^-?\d+$", operand) or re.match(r"^-?0x[A-Fa-f0-9]+$", operand):
        is_addend = False
    else:
        is_addend = True

    return (r_source, r_dest, operand, is_addend, needs_expanding)


def div_needs_expanding(line: str) -> bool:
    inst, *rest = line.split()
    if not (inst.startswith("div") or inst.startswith("rem")):
        return False

    r_dest, *_ = rest[0].split(",")
    return r_dest not in ("$zero", "$0")


def expand_load_immediate(line: str) -> List[str]:
    res = []

    match = re.match(r"li\s+(\$[0-9A-z]+),\s?(-?[x0-9a-fA-F]+)", line)
    assert match is not None, "li regex failed"

    r_dest = match.group(1)
    operand = int(match.group(2), 0)

    if 0 < operand < 0x10000:
        res.append(f"ori\t{r_dest},$zero,{operand}")
    elif operand >= 0x10000:
        res.append(f"lui\t{r_dest},({operand} >> 16) & 0xFFFF")
        if operand & 0xFFFF:
            res.append(f"ori\t{r_dest},{r_dest},{operand} & 0xFFFF")
    elif 0 > operand > -0x8000:
        res.append(f"addiu\t{r_dest},$zero,{operand}")
    elif operand == -0x8000:
        res.append(f"addiu\t{r_dest},$zero,{operand} & 0xFFFF")
    elif operand < -0x8000:
        res.append(f"lui\t{r_dest},({operand} >> 16) & 0xFFFF")
        if operand & 0xFFFF:
            res.append(f"ori\t{r_dest},{r_dest},{operand} & 0xFFFF")
    else:
        # TODO: raise an exception here instead?
        # ori is actually addiu on ASPSX 2.56+
        res.append(f"ori\t{r_dest},0")

    return res


def expand_move(line: str):
    line = strip_comments(line)
    op, *rest = line.split()
    if op == "move":
        args = " ".join(rest)
        r_dest, r_source = args.split(",")
        return f"addu\t{r_dest},{r_source},$zero"
    return line


def is_label(line: str):
    return re.match(r"\$L(b|e)?\d+:$", line)


def is_instruction(line: str, ignore_nop=False, ignore_set=False, ignore_label=False):
    if len(line) == 0:
        return False

    if ignore_nop and line == "#nop":
        return False
    if ignore_set and line in (
        ".set\treorder",
        ".set\tnoreorder",
        ".set\tvolatile",
        ".set\tnovolatile",
    ):
        return False
    if ignore_label and is_label(line):
        return False

    if line.startswith(".stab"):
        return False
    if line.startswith(".def") or line.startswith(".bend") or line.startswith(".begin"):
        return False
    if line.startswith(".loc"):
        return False

    if line.startswith("L") and line[1] not in "0123456789" and line.endswith(":"):
        return False
    if line in (".set\tmacro", ".set\tnomacro"):
        return False
    if line in ("#.set\tvolatile", "#.set\tnovolatile"):
        return False
    if line in ("#APP", "#NO_APP"):
        return False

    return True


def get_next_register(reg: str):
    lut = {
        # li $fx
        "$f0": "$f1",
        "$f2": "$f3",
        "$f4": "$f5",
        "$f6": "$f7",
        "$f12": "$f13",
        "$f14": "$f15",
        # names
        "$v0": "$v1",
        "$a0": "$a1",
        "$a2": "$a3",
        "$t0": "$t1",
        "$t2": "$t3",
        "$s0": "$s1",
        "$s2": "$s3",
        # nums
        "$2": "$3",  # $v0
        "$4": "$5",  # $a0
        "$6": "$7",  # $a2
        "$8": "$9",  # $t0
        "$10": "$11",  # t2
        "$18": "$19",  # s2
    }
    next_reg = lut.get(reg)
    assert next_reg is not None, f"Unknown mapping for {reg}"
    return next_reg


def expand_macro(line: str):
    res = []
    for l in strip_comments(line).split(";"):
        l = l.strip()
        if len(l) == 0:
            continue
        op, *rest = l.split()
        res.append(
            (op, " ".join(rest)),
        )
    return res


def load_immediate_single(line: str):
    res = []
    r1, value = line[5:].split(",")
    (num,) = struct.unpack(">i", struct.pack(">f", float(value)))
    upper = (num & 0xFFFF_0000) >> 16
    lower = (num & 0x0000_FFFF) >> 0

    res.append(f"lui\t{r1},0x{upper:X}")
    # we don't always need the lower part
    if lower:
        res.append(f"ori\t{r1},0x{lower:X}")
    return res


def load_immediate_double(line: str):
    res = []
    r1, value = line[5:].split(",")
    r2 = get_next_register(r1)
    (num,) = struct.unpack(">q", struct.pack(">d", float(value)))

    r1_upper = (num & 0x0000_0000_FFFF_0000) >> 16
    r1_lower = (num & 0x0000_0000_0000_FFFF) >> 0
    r2_upper = (num & 0xFFFF_0000_0000_0000) >> 48
    r2_lower = (num & 0x0000_FFFF_0000_0000) >> 32

    if r1_upper or r1_lower:
        res.append(f"lui\t{r1},0x{r1_upper:X}")
        if r1_lower:
            res.append(f"ori\t{r1},0x{r1_lower:X}")
    else:
        res.append(f"li\t{r1},0x0")

    res.append(f"lui\t{r2},0x{r2_upper:X}")
    if r2_lower:
        res.append(f"ori\t{r2},0x{r2_lower:X}")

    return res


class MaspsxProcessor:
    is_reorder = True
    cop2_is_reorder = True
    skip_instructions = 0
    file_num = 1
    line_index = 0

    def __init__(
        self,
        lines: List[str],
        sdata_limit=0,
        expand_div=False,
        expand_li=False,
        nop_at_expansion=False,
        nop_mflo_mfhi=True,
        sltu_at=False,
        addiu_at=False,
        div_uses_tge=False,
        gp_allow_offset=False,
        gp_allow_la=False,
        use_comm_section=False,
        use_comm_for_lcomm=False,
    ):
        self.lines = [x.strip() for x in lines]

        self.sdata_limit = sdata_limit

        self.expand_div = expand_div
        self.expand_li = expand_li

        self.nop_at_expansion = nop_at_expansion
        self.nop_mflo_mfhi = nop_mflo_mfhi

        self.sltu_at = sltu_at
        self.addiu_at = addiu_at
        self.div_uses_tge = div_uses_tge

        self.gp_allow_offset = gp_allow_offset
        self.gp_allow_la = gp_allow_la

        self.use_comm_section = use_comm_section
        self.use_comm_for_lcomm = use_comm_for_lcomm

        self.bss_entries: dict[str, int] = {}
        self.sbss_entries: dict[str, int] = {}
        self.sdata_entries: dict[str, int] = {}

        self.comm_symbols: set[str] = set()

    def preprocess_lines(self) -> None:
        in_sdata = False
        uses_size = False

        for line in self.lines:
            if line == "":
                continue

            if line.startswith(".align"):
                # TODO: worry about alignment later
                continue

            if line.startswith(".globl"):
                continue

            if line.startswith(".text"):
                in_sdata = False
                continue
            if line.startswith(".data"):
                in_sdata = False
                continue
            if line.startswith(".rdata"):
                in_sdata = False
                continue

            if line.startswith(".section") and line.endswith(".text"):
                in_sdata = False
                continue

            if line.startswith("#"):
                continue

            if line.startswith(".sdata"):
                in_sdata = True
                continue

            if line.startswith(".file"):
                in_sdata = False
                continue

            if line.startswith(".extern"):
                in_sdata = False
                continue

            if line.startswith(".comm") or line.startswith(".lcomm"):
                # e.g.	.comm	MENU_RadarScale_800AB480,4
                in_sdata = False
                _, var = line.split()
                symbol, size_str, *_ = var.split(",")
                size = int(size_str)
                if size <= self.sdata_limit:
                    self.sbss_entries[symbol] = size
                else:
                    self.bss_entries[symbol] = size

                if line.startswith(".comm"):
                    self.comm_symbols.add(symbol)
                continue

            if in_sdata:
                # NOTE: newer compilers emit .size for sdata, old ones do not...
                if match := re.match(r"\.size\s+([^,]+),([0-9]+)", line):
                    current_symbol = match.group(1)
                    size = int(match.group(2))
                    self.sdata_entries[current_symbol] = size
                    uses_size = True
                    continue

                if not uses_size:
                    if line.endswith(":"):
                        current_symbol = line.replace(":", "")
                        self.sdata_entries[current_symbol] = 0
                    else:
                        if line.startswith(".type"):
                            continue

                        if line.startswith(".space"):
                            _, size_str = line.split()
                            size = int(size_str)
                        elif line.startswith(".word"):
                            size = 4
                        elif line.startswith(".half") or line.startswith(".short"):
                            size = 2
                        elif line.startswith(".byte"):
                            size = 1
                        elif line.startswith(".ascii"):
                            # e.g. .ascii	"Map poly groups\000"
                            # NOTE: len('.ascii\t""') == 9
                            size = len(line) - 9
                        else:
                            raise Exception(
                                f"Unable to parse .sdata instruction: {line}"
                            )
                        self.sdata_entries[current_symbol] += size

    def process_lines(self):
        self.is_reorder = True
        self.cop2_is_reorder = True
        self.skip_instructions = 0
        self.file_num = 1

        self.bss_entries = {}
        self.sbss_entries = {}
        self.sdata_entries = {}

        self.preprocess_lines()

        res = []
        in_include_asm_hack = False
        for i, line in enumerate(self.lines):
            self.line_index = i

            if ".ent\t__maspsx_include_asm_hack" in line:
                in_include_asm_hack = True

            if in_include_asm_hack:
                if "# maspsx-keep" in line:
                    res += [line]
                else:
                    res += [f"# {line} # DEBUG: skipped due to include asm hack"]
                if ".end\t__maspsx_include_asm_hack" in line:
                    in_include_asm_hack = False
                continue

            if is_instruction(line) and self.skip_instructions > 0:
                self.skip_instructions -= 1
                res += [f"# {line}  # DEBUG: skipped"]
            else:
                res += self.process_line(line)

        for section, entries in [
            ("sbss", self.sbss_entries),
            ("bss", self.bss_entries),
        ]:
            for i, (symbol, size) in enumerate(entries.items()):
                if i == 0:
                    res.append(f".section .{section}")

                if self.use_comm_section and (
                    symbol in self.comm_symbols or self.use_comm_for_lcomm
                ):
                    # implicit alignment for COMMON
                    res.append(f"\t.comm {symbol},{size}")
                    continue

                if section == "sbss":
                    if size >= 8:
                        res.append("\t.align 3")
                    elif size >= 4:
                        res.append("\t.align 2")
                    elif size >= 2:
                        res.append("\t.align 1")

                # only mark bss symbols as global
                if section == "bss":
                    res.append(
                        f"\t.globl {symbol}",
                    )
                res.extend(
                    [
                        f"{symbol}:",
                        f"\t.space {size}",
                    ]
                )

        return res

    def get_next_instruction(
        self, skip=0, ignore_nop=False, ignore_set=False, ignore_label=False
    ):
        i = self.line_index + 1
        while i < len(self.lines):
            line = self.lines[i]
            if is_instruction(
                line,
                ignore_nop=ignore_nop,
                ignore_set=ignore_set,
                ignore_label=ignore_label,
            ):
                if skip == 0:
                    return line
                skip -= 1
            i += 1

        return ""  # warn user?

    def _uses_gp(self, line: str) -> bool:
        if self.sdata_limit == 0:
            return False

        line = strip_comments(line)
        if uses_at(line):
            op, *rest = line.split("\t")
            if op in load_mnemonics or op in store_mnemonics:
                (
                    _,
                    _,
                    operand,
                    _,
                    _,
                ) = parse_load_or_store(" ".join(rest))

                if operand.count("+") == 1:
                    symbol, _ = operand.split("+")
                    gp_allowed = self.gp_allow_offset or symbol not in self.comm_symbols
                else:
                    symbol = operand
                    gp_allowed = True

                if gp_allowed and (
                    symbol in self.sbss_entries or symbol in self.sdata_entries
                ):
                    return True

        return False

    def _handle_nop_before_next_instruction(
        self, next_instruction: str, r_dest: str, requires_gap=None
    ) -> List[str]:
        res: List[str] = []

        if requires_gap is None:
            requires_gap = line_loads_from_reg(next_instruction, r_dest)
        if requires_gap:
            nop_required = False

            if not uses_at(next_instruction):
                reason = f"'{next_instruction}' does not use $at"
                nop_required = True
            if self._uses_gp(next_instruction):
                reason = f"'{next_instruction}' uses $gp"
                nop_required = True
            if uses_at(next_instruction) and self.nop_at_expansion:
                reason = (
                    f"'{next_instruction}' inject nop beween {r_dest} and $at expansion"
                )
                nop_required = True

            if nop_required:
                label = self.get_next_instruction(
                    skip=0, ignore_nop=True, ignore_set=True
                )
                if is_label(label):
                    res.append(label)
                    self.skip_instructions = 1
                res.append(f"nop # DEBUG: Reuse of '{r_dest}'. {reason}")
        else:
            res.append(
                f"#nop # DEBUG: '{next_instruction}' does not load from {r_dest}"
            )

        return res

    def _handle_mflo_mfhi(self, r_source=None) -> List[str]:
        # we cannot use a div/mult within 2 instructions of mflo/mfhi
        res: List[str] = []

        if not self.nop_mflo_mfhi:
            return res

        next_instruction = self.get_next_instruction(
            skip=0, ignore_nop=True, ignore_set=True, ignore_label=True
        )
        next_next_instruction = self.get_next_instruction(
            skip=1, ignore_nop=True, ignore_set=True, ignore_label=True
        )

        if any(
            next_instruction.startswith(x)
            for x in ["mult\t", "multu\t", "div\t", "divu\t", "rem\t", "remu\t"]
        ):
            # #nop
            # #nop
            # mult...
            skip = 0
            while True:
                inst = self.get_next_instruction(skip=skip)
                skip += 1
                if inst == next_instruction:
                    res.append("nop")
                    res.append("nop")
                    if div_needs_expanding(inst):
                        res.append("# DEBUG: div needs expanding")
                        skip -= 1
                    else:
                        res.append(expand_move(inst))
                    break
                if not inst.startswith("#"):
                    res.append(expand_move(inst))
            self.skip_instructions = skip

        elif any(
            next_next_instruction.startswith(x)
            for x in ["mult\t", "multu\t", "div\t", "divu\t", "rem\t", "remu\t"]
        ):

            # #nop
            # #nop
            # bne or addu or lh ...
            # mult ...
            skip = 0
            no_reorder = False
            while True:
                inst = self.get_next_instruction(skip=skip)

                if inst.startswith(".set") and inst.endswith("noreorder"):
                    no_reorder = True
                    skip += 1
                    continue

                skip += 1
                if inst == next_instruction:
                    op, *_ = inst.strip().split()
                    if op in load_mnemonics:
                        # allow for $at handling later in the script
                        skip = 0
                        break

                    if op in ("mflo", "mfhi"):
                        # allow for mflo/mfhi handling later on
                        skip = 0
                        break

                    if op == "li":
                        expanded = expand_load_immediate(inst)

                        if self.expand_li:
                            res += expanded
                        else:
                            res.append(inst)

                        if len(expanded) == 2:
                            res.append(
                                "#nop  # DEBUG: mflo/mfhi with mult/div/rem and li expands to 2 ops"
                            )
                        else:
                            res.append(
                                "nop  # DEBUG: mflo/mfhi with mult/div/rem and li expands to 1 op"
                            )

                    else:

                        if no_reorder:
                            res.append(
                                "nop  # DEBUG: mflo/mfhi with mult/div/rem and 1 instruction (noreorder)"
                            )
                            res.append(".set\tnoreorder")
                            res.append(expand_move(inst))
                        else:
                            if r_source and line_loads_from_reg(inst, r_source):
                                # NOTE: only relevant when div has been expanded (i.e. -0 flag)
                                res.extend(
                                    [
                                        f"nop  # DEBUG: mflo/mfhi with mult/div/rem and 1 instruction which loads from {r_source}",
                                        expand_move(inst),
                                    ]
                                )
                            else:
                                if op in branch_mnemonics:
                                    res.extend(
                                        [
                                            inst,
                                            "nop # DEBUG: mflo/mfhi with mult/div/rem and 1 instruction (branch)",
                                        ]
                                    )
                                else:
                                    maybe_label = self.get_next_instruction(skip=skip)
                                    if is_label(maybe_label):
                                        res.extend(
                                            [
                                                expand_move(inst),
                                                maybe_label,
                                                "nop  # DEBUG: mflo/mfhi with mult/div/rem and 1 instruction (label)",
                                            ]
                                        )
                                        skip += 1
                                    else:
                                        res.extend(
                                            [
                                                expand_move(inst),
                                                "nop  # DEBUG: mflo/mfhi with mult/div/rem and 1 instruction",
                                            ]
                                        )

                elif inst == next_next_instruction:
                    # reached mult/div/rem
                    if div_needs_expanding(inst):
                        res.append("# DEBUG: div needs expanding")
                        skip -= 1
                    else:
                        res.append(inst)
                    break
                elif not inst.startswith("#"):
                    res.append(expand_move(inst))
            self.skip_instructions = skip

        else:
            # do nothing
            pass

        return res

    def process_line(self, line: str):
        res = []

        if len(line) == 0:
            return [line]

        if line.startswith("#"):
            return []

        if line.startswith("."):
            # Keep upstream branch/NOP parsing and register aliases unchanged.
            # COP2 transfers also recognize space-delimited ordering directives.
            if directive := re.fullmatch(r"\.set\s+(noreorder|reorder)", strip_comments(line)):
                self.cop2_is_reorder = directive[1] == "reorder"
            if (
                line.startswith(".def\t")
                or line.startswith(".begin\t")
                or line.startswith(".bend\t")
            ):
                # skip these coff directives - gnu as does not like them
                pass

            elif line.startswith(".set\t"):
                if line.endswith("\tnoreorder"):
                    self.is_reorder = False
                elif line.endswith("\treorder"):
                    self.is_reorder = True

            elif line.startswith(".file\t"):
                # fix same-numbered files
                _, file_num, filename = line.split(maxsplit=2)
                res.append(f".file\t{self.file_num} {filename}")
                self.file_num += 1

            elif line.startswith(".ent\t"):
                # enforce noreorder for each function
                res.append(line)
                res.append(".set\tnoreorder")

            elif line.startswith(".comm") or line.startswith(".lcomm"):
                # already handled via preprocess_lines
                pass

            elif line.startswith(".data"):
                res.append(".section .data")
            elif line.startswith(".sdata"):
                res.append(".section .sdata")
            elif line.startswith(".rdata"):
                res.append(".section .rodata")

            else:
                res.append(line)

            return res

        if line.startswith("$L"):
            return [line]

        actual_r_dest = None
        is_macro = ";" in line
        if is_macro:
            expanded = expand_macro(line)
            if len(expanded) > 0:
                actual_op, *actual_rest = expanded[-1]
                if actual_op in load_mnemonics:
                    _, actual_r_dest, _, _, _ = parse_load_or_store(
                        " ".join(actual_rest)
                    )

        op, *rest = line.split()

        if op in ("mfc2", "cfc2"):
            # Only source reorder regions; explicit noreorder remains literal.
            if ";" in strip_comments(line):
                raise ValueError("COP2 reads must be separate instructions, not leading semicolon macros")
            res.append(line)
            if self.cop2_is_reorder:
                r_dest = strip_comments(line).split(None, 1)[1].split(",", 1)[0].strip()
                next_instruction = ""
                for following in self.lines[self.line_index + 1:]:
                    if strip_comments(following) and is_instruction(
                        following, ignore_nop=True, ignore_set=True, ignore_label=True
                    ):
                        next_instruction = following
                        break
                dependency = cop2_read_dependency(next_instruction, r_dest)
                if dependency is None:
                    res.append("nop # Unrecognized CPU dependency after COP2 read")
                elif dependency:
                    mnemonic, operands = strip_comments(next_instruction).split(";", 1)[0].split(None, 1)
                    res.extend(self._handle_nop_before_next_instruction(
                        mnemonic + "\t" + operands, r_dest, requires_gap=True
                    ))

        elif op in load_mnemonics:
            r_source, r_dest, operand, is_addend, needs_expanding = parse_load_or_store(
                " ".join(rest)
            )

            next_instruction = self.get_next_instruction(
                skip=0, ignore_nop=True, ignore_set=True, ignore_label=True
            )
            # Naively handle scenario where *next* line is a macro
            if ";" in next_instruction:
                next_instruction = next_instruction.split(";")[0]

            if not needs_expanding:
                # newer GCCs can emit %hi() and %lo() separately...
                res.append(f"{line} # DEBUG: leaving for assembler to expand")
                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

            elif is_addend and r_source is None:
                # e.g. lb	$s0,D_800E52E0
                if operand.count("+") == 1:
                    symbol, offset = operand.split("+")
                    gp_rel = f"%gp_rel({symbol}+{offset})($gp)"
                    gp_allowed = self.gp_allow_offset or symbol not in self.comm_symbols
                else:
                    symbol = operand
                    gp_rel = f"%gp_rel({symbol})($gp)"
                    gp_allowed = True

                if gp_allowed and (
                    symbol in self.sdata_entries or symbol in self.sbss_entries
                ):
                    res.append(f"{op}\t{r_dest},{gp_rel}")
                else:
                    res.append(line)

                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

            elif is_addend and r_source:
                # e.g. lw	$2,test_sym($4)
                if self.addiu_at:
                    res.extend(
                        [
                            "# EXPAND_AT START",
                            ".set\tnoat",
                            f"lui\t$at,%hi({operand})",
                            f"addiu\t$at,$at,%lo({operand})",
                            f"addu\t$at,$at,{r_source}",
                            f"{op}\t{r_dest},0x0($at)",
                            ".set\tat",
                            "# EXPAND_AT END",
                        ]
                    )
                else:
                    res.extend(
                        [
                            "# EXPAND_AT START",
                            ".set\tnoat",
                            f"lui\t$at,%hi({operand})",
                            f"addu\t$at,$at,{r_source}",
                            f"{op}\t{r_dest},%lo({operand})($at)",
                            ".set\tat",
                            "# EXPAND_AT END",
                        ]
                    )

                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

            else:
                if r_source and (int(operand, 0) > 32767 or int(operand, 0) < -32768):
                    # e.g. lhu	$2,49344($2)
                    res.extend(
                        [
                            "# EXPAND_AT START",
                            ".set\tnoat",
                            f"lui\t$at,%hi({operand})",
                            f"addu\t$at,{r_source},$at",
                            f"{op}\t{r_dest},%lo({operand})($at)",
                            ".set\tat",
                            "# EXPAND_AT END",
                        ]
                    )
                else:
                    # e.g. lhu	$2,528482304
                    res.append(line)

                # Naively handle scenario where *current* line is a macro
                if actual_r_dest is not None:
                    r_dest = actual_r_dest

                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

        elif op in store_mnemonics or (op == "la" and self.sdata_limit > 0):
            r_source, r_dest, operand, is_addend, _ = parse_load_or_store(
                " ".join(rest)
            )

            if is_addend and r_source is None:
                # e.g. sw	$v0,D_800E52E0
                if operand.count("+") == 1:
                    symbol, offset = operand.split("+")
                    gp_rel = f"%gp_rel({symbol}+{offset})($gp)"
                    gp_allowed = self.gp_allow_offset or symbol not in self.comm_symbols
                else:
                    symbol = operand
                    gp_rel = f"%gp_rel({symbol})($gp)"
                    gp_allowed = True

                if op == "la" and not self.gp_allow_la:
                    gp_allowed = False

                if gp_allowed and (
                    symbol in self.sdata_entries or symbol in self.sbss_entries
                ):
                    res.append(f"{op}\t{r_dest},{gp_rel}")
                else:
                    res.append(line)
            elif is_addend and r_source:
                # e.g. sw	$a0,ctlbuf($v0)
                if self.addiu_at and op != "la":
                    res.extend(
                        [
                            "# EXPAND_AT START",
                            ".set\tnoat",
                            f"lui\t$at,%hi({operand})",
                            f"addiu\t$at,$at,%lo({operand})",
                            f"addu\t$at,$at,{r_source}",
                            f"{op}\t{r_dest},0x0($at)",
                            ".set\tat",
                            "# EXPAND_AT END",
                        ]
                    )
                else:
                    res.append(line)
            elif r_source and (int(operand, 0) > 32767 or int(operand, 0) < -32768):
                # e.g. sw	$2,56200($4)
                res.extend(
                    [
                        "# EXPAND_AT START",
                        ".set\tnoat",
                        f"lui\t$at,%hi({operand})",
                        f"addu\t$at,{r_source},$at",
                        f"{op}\t{r_dest},%lo({operand})($at)",
                        ".set\tat",
                        "# EXPAND_AT END",
                    ]
                )
            else:
                res.append(line)

        elif op in branch_mnemonics or op in jump_mnemonics:
            res.append(line)
            if self.is_reorder:
                res.append("nop  # DEBUG: branch/jump")

        elif op == "move":
            # expand move $2,$16 to addu $2,$16,$zero
            res.append(expand_move(line))

        elif op in ("addu", "subu", "sra", "srl", "srr", "sll", "or"):
            # no extra processing required
            res.append(line)
            # TODO: check if this line is a macro and insert a nop if required...

        elif op == "li":
            # TODO: handle non-soft floats?
            if self.expand_li:
                res += expand_load_immediate(line)
            else:
                res.append(line)

        elif op == "li.s":
            res += load_immediate_single(line)

        elif op == "li.d":
            res += load_immediate_double(line)

        elif op in ("mflo", "mfhi"):
            res.append(line)
            res += self._handle_mflo_mfhi()

        elif op == "break":
            # turn 'break 7' into 'break 0x0,0x7'
            num = int(rest[0], 0)
            line = f"break\t0x{num >> 10:X},0x{num & 0x3FF:X}"
            res.append(line)

        elif op in ("div", "rem"):
            r_dest, r_source, r_operand = rest[0].split(",")
            if r_dest in ("$zero", "$0"):
                # e.g. div $zero, $v0, $a0
                return [line]

            move_from = "mfhi" if op == "rem" else "mflo"
            if self.expand_div:
                res.extend(
                    [
                        "# EXPAND_DIV START",
                        ".set\tnoat",
                        f"div\t$zero,{r_source},{r_operand}",
                        f"bnez\t{r_operand},.L_NOT_DIV_BY_ZERO_{self.line_index}",
                        "nop",
                        "break\t0x7",
                        f".L_NOT_DIV_BY_ZERO_{self.line_index}:",
                        "addiu\t$at,$zero,-1",
                        f"bne\t{r_operand},$at,.L_DIV_BY_POSITIVE_SIGN_{self.line_index}",
                        "lui\t$at,0x8000",
                        f"bne\t{r_source},$at,.L_DIV_BY_POSITIVE_SIGN_{self.line_index}",
                        "nop",
                        "tge\t$zero,$zero,93" if self.div_uses_tge else "break\t0x6",
                        f".L_DIV_BY_POSITIVE_SIGN_{self.line_index}:",
                        f"{move_from}\t{r_dest}",
                        ".set\tat",
                        "# EXPAND_DIV END",
                    ]
                )
            else:
                res.extend(
                    [
                        "# EXPAND_ZERO_DIV START",
                        f"div\t$zero,{r_source},{r_operand}",
                        f"{move_from}\t{r_dest}",
                        "# EXPAND_ZERO_DIV END",
                    ]
                )

            extra_nops = self._handle_mflo_mfhi(r_source=r_dest)
            if len(extra_nops) > 0:
                res += extra_nops
            else:
                next_instruction = self.get_next_instruction(
                    skip=0, ignore_set=True, ignore_label=True
                )
                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

        elif op in ("divu", "remu"):
            r_dest, r_source, r_operand = rest[0].split(",")
            if r_dest in ("$zero", "$0"):
                # e.g. divu $zero, $v1, $a2
                return [line]

            move_from = "mfhi" if op == "remu" else "mflo"
            if self.expand_div:
                res.extend(
                    [
                        "# EXPAND_DIVU START",
                        ".set\tnoat",
                        f"divu\t$zero,{r_source},{r_operand}",
                        f"bnez\t{r_operand},.L_NOT_DIV_BY_ZERO_{self.line_index}",
                        "nop",
                        "break\t0x7",
                        f".L_NOT_DIV_BY_ZERO_{self.line_index}:",
                        f"{move_from}\t{r_dest}",
                        ".set\tat",
                        "# EXPAND_DIVU END",
                    ]
                )
            else:
                res.extend(
                    [
                        "# EXPAND_ZERO_DIVU START",
                        f"divu\t$zero,{r_source},{r_operand}",
                        f"{move_from}\t{r_dest}",
                        "# EXPAND_ZERO_DIVU END",
                    ]
                )

            extra_nops = self._handle_mflo_mfhi(r_source=r_dest)
            if len(extra_nops) > 0:
                res += extra_nops
            else:
                next_instruction = self.get_next_instruction(
                    skip=0, ignore_set=True, ignore_label=True
                )
                extra_nops = self._handle_nop_before_next_instruction(
                    next_instruction, r_dest
                )
                res.extend(extra_nops)

        elif op == "sltu":
            r_dest, r_source, r_operand = rest[0].split(",")
            if re.match(r"^-?\d+$", r_operand) or re.match(
                r"^-?0x[A-Fa-f0-9]+$", r_operand
            ):
                value = int(r_operand)
                if self.sltu_at and value < 0:
                    res.append(f"li\t$at,{r_operand}")
                    res.append(f"{op}\t{r_dest},{r_source},$at")
                else:
                    # TODO: do we want to expand sltu into sltiu?
                    res.append(line)
            else:
                res.append(line)

        else:
            res.append(line)

        return res
