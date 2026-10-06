#!/usr/bin/env python3
"""Link resident functions at retail addresses and verify configured blocks."""

from __future__ import annotations

import json
import re
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Iterable


import pipeline as ctr_match


BUILD_ROOT = ctr_match.BUILD / "namespaces"


@dataclass(frozen=True)
class Symbol:
    name: str
    address: int


@dataclass(frozen=True)
class FunctionRange:
    name: str
    address: int
    size: int
    source: str
    candidate_symbol: str | None = None

    @property
    def emitted_name(self) -> str:
        return self.candidate_symbol or self.name

    def result_fields(self) -> dict[str, Any]:
        fields = asdict(self)
        if self.emitted_name == self.name:
            fields.pop("candidate_symbol")
        return fields


def parse_symbols(path: Path) -> list[Symbol]:
    return sorted(
        (
            Symbol(name=name, address=address)
            for name, address in ctr_match.load_symbol_file(path).items()
        ),
        key=lambda symbol: (symbol.address, symbol.name),
    )


def symbol_by_name(symbols: Iterable[Symbol], name: str) -> Symbol:
    for symbol in symbols:
        if symbol.name == name:
            return symbol
    raise ctr_match.MatchError(f"symbol map has no symbol named {name!r}")


def namespace_intervals(
    config: dict[str, Any], symbols: list[Symbol]
) -> list[tuple[Symbol, Symbol]]:
    specifications = config.get("code_intervals", [config])
    if "code_intervals" in config and (
        "start_symbol" in config
        or "end_symbol" in config
        or not isinstance(config.get("linked_block"), dict)
    ):
        raise ctr_match.MatchError(
            "code_intervals requires linked_block and replaces singleton boundaries"
        )
    if not isinstance(specifications, list) or not specifications:
        raise ctr_match.MatchError("code_intervals must be a nonempty ordered list")
    intervals = []
    for specification in specifications:
        if not isinstance(specification, dict) or any(
            not isinstance(specification.get(key), str)
            for key in ("start_symbol", "end_symbol")
        ):
            raise ctr_match.MatchError(
                "each code interval needs start_symbol and end_symbol"
            )
        start = symbol_by_name(symbols, specification["start_symbol"])
        end = symbol_by_name(symbols, specification["end_symbol"])
        if start.address >= end.address:
            raise ctr_match.MatchError(
                f"namespace {config['name']!r} has an empty or reversed interval"
            )
        if intervals and start.address < intervals[-1][1].address:
            raise ctr_match.MatchError("code intervals must be ordered and nonoverlapping")
        intervals.append((start, end))
    return intervals


def namespace_symbol_ranges(
    config: dict[str, Any], symbols: list[Symbol]
) -> list[tuple[Symbol, int]]:
    ranges = []
    for start, end in namespace_intervals(config, symbols):
        interval = [
            symbol for symbol in symbols
            if start.address <= symbol.address < end.address
        ]
        if any(
            left.address >= right.address for left, right in zip(interval, interval[1:])
        ):
            raise ctr_match.MatchError(
                f"namespace {config['name']!r} has aliased or unordered symbols"
            )
        wrong_prefix = [
            symbol.name for symbol in interval
            if not symbol.name.startswith(config["symbol_prefix"])
        ]
        if wrong_prefix:
            raise ctr_match.MatchError(
                f"namespace {config['name']!r} contains symbols outside prefix "
                f"{config['symbol_prefix']!r}: {', '.join(wrong_prefix)}"
            )
        next_addresses = [symbol.address for symbol in interval[1:]] + [end.address]
        ranges.extend(
            (symbol, address - symbol.address)
            for symbol, address in zip(interval, next_addresses)
        )
    return ranges


def namespace_sources(config: dict[str, Any]) -> list[Path]:
    return [ctr_match.repository_path(path) for path in ctr_match.source_paths(config)]


def definition_count(text: str, symbol: str) -> int:
    pattern = re.compile(
        rf"(?m)^[ \t]*(?:(?:[A-Za-z_][A-Za-z0-9_]*)[ \t]+)+"
        rf"(?:\*[ \t]*)*{re.escape(symbol)}[ \t]*\([^;{{}}]*\)"
        rf"(?:[ \t\r\n]|^[ \t]*#[ \t]*endif\b[^\n]*(?:\n|$))*\{{"
    )
    c_count = len(pattern.findall(text))
    if c_count:
        return c_count
    # Retail-only sources may define a function through global assembly.
    label = re.compile(rf'(?m)^[ \t]*"{re.escape(symbol)}:\\n"')
    return len(label.findall(text))


def discover_function_ranges(
    config: dict[str, Any], symbols: list[Symbol]
) -> list[FunctionRange]:
    interval = namespace_symbol_ranges(config, symbols)
    owned = config.get("owned_symbols", {})
    if not isinstance(owned, dict) or any(
        not isinstance(name, str)
        or not isinstance(candidate, str)
        or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name)
        or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", candidate)
        for name, candidate in owned.items()
    ):
        raise ctr_match.MatchError("owned_symbols must map identifiers to identifiers")
    names = {symbol.name for symbol, _ in interval}
    if unknown := sorted(set(owned) - names):
        raise ctr_match.MatchError(
            f"owned_symbols names outside namespace interval: {', '.join(unknown)}"
        )
    resolved = {symbol.name: owned.get(symbol.name, symbol.name) for symbol, _ in interval}
    if len(set(resolved.values())) != len(resolved):
        raise ctr_match.MatchError("namespace functions select the same owned symbol")
    canonical = {symbol.name: symbol.address for symbol in symbols}
    for symbol, _ in interval:
        candidate = resolved[symbol.name]
        if candidate in canonical and canonical[candidate] != symbol.address:
            raise ctr_match.MatchError(
                f"owned symbol {candidate!r} collides with another retail address"
            )
    sources = namespace_sources(config)
    source_text = {source: source.read_text() for source in sources}
    ranges: list[FunctionRange] = []
    for symbol, size in interval:
        candidate = resolved[symbol.name]
        owners: list[Path] = []
        for source, text in source_text.items():
            count = definition_count(text, candidate)
            if count > 1:
                raise ctr_match.MatchError(
                    f"production source {source.relative_to(ctr_match.ROOT)} "
                    f"defines {candidate} {count} times"
                )
            if count == 1:
                owners.append(source)
        if len(owners) != 1:
            relative = [str(path.relative_to(ctr_match.ROOT)) for path in owners]
            raise ctr_match.MatchError(
                f"expected one production definition of {candidate}, "
                f"found {len(owners)}: {', '.join(relative) or 'none'}"
            )
        ranges.append(
            FunctionRange(
                name=symbol.name,
                address=symbol.address,
                size=size,
                source=str(owners[0].relative_to(ctr_match.ROOT)),
                candidate_symbol=candidate if candidate != symbol.name else None,
            )
        )

    owners = {ctr_match.repository_path(function.source) for function in ranges}
    unowned = [source for source in sources if source not in owners]
    if unowned:
        relative = ", ".join(
            str(source.relative_to(ctr_match.ROOT)) for source in unowned
        )
        raise ctr_match.MatchError(
            f"namespace {config['name']!r} has source(s) without a retail "
            f"function: {relative}"
        )
    return ranges


def compile_source(
    config: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    source: Path,
) -> dict[str, Any]:
    relative = source.relative_to(ctr_match.ROOT)
    output = BUILD_ROOT / config["name"] / "sources" / relative.with_suffix("")
    error_file = output / "compile-error.txt"
    output.mkdir(parents=True, exist_ok=True)
    try:
        object_file, dependencies = ctr_match.build_object(
            toolchain,
            config,
            source,
            output / source.name,
        )
    except ctr_match.MatchError as exc:
        error_file.write_text(f"{exc}\n")
        return {
            "source": str(source.relative_to(ctr_match.ROOT)),
            "source_sha256": ctr_match.sha256_file(source),
            "compiled": False,
            "error": str(exc),
            "error_file": str(error_file.relative_to(ctr_match.ROOT)),
        }

    error_file.unlink(missing_ok=True)
    return {
        "source": str(relative),
        "object_path": object_file,
        "dependencies": dependencies,
        "compiled": True,
    }


def object_functions(objdump: Path, object_file: Path) -> dict[str, dict[str, Any]]:
    output = ctr_match.command_output([str(objdump), "-t", str(object_file)])
    functions: dict[str, dict[str, Any]] = {}
    for line in output.splitlines():
        parts = line.split()
        if (
            len(parts) >= 6
            and re.fullmatch(r"[0-9a-fA-F]{8}", parts[0])
            and parts[2] == "F"
            and re.fullmatch(r"[0-9a-fA-F]{8}", parts[-2])
        ):
            name = parts[-1]
            count = functions.get(name, {}).get("count", 0) + 1
            functions[name] = {
                "address": int(parts[0], 16),
                "section": parts[-3],
                "size": int(parts[-2], 16),
                "count": count,
            }
    return functions


def owned_function(
    function: FunctionRange, symbols: dict[str, dict[str, Any]], section: str
) -> dict[str, Any]:
    symbol = symbols.get(function.emitted_name)
    if (
        symbol is None
        or symbol["count"] != 1
        or symbol["size"] <= 0
        or symbol["section"] != section
    ):
        raise ctr_match.MatchError(
            f"expected one defined function {function.emitted_name!r} in {section!r}"
        )
    return symbol


def reject_owned_aliases(config: dict[str, Any], functions: Iterable[FunctionRange]) -> None:
    aliases = set(config.get("link_symbols", {})) & {
        function.emitted_name for function in functions
    }
    if aliases:
        raise ctr_match.MatchError(
            f"link_symbols replaces selected function(s): {', '.join(sorted(aliases))}"
        )


def reference_slice(
    manifest: dict[str, Any],
    references: Path,
    config: dict[str, Any],
    function: FunctionRange,
) -> bytes:
    return ctr_match.reference_bytes(
        manifest,
        references,
        {
            "symbol": function.name,
            "region": config["artifact"],
            "address": function.address,
            "size": function.size,
        },
    )


def function_linker_script(
    function: FunctionRange,
    function_sections: list[str],
    rodata_address: int | None = None,
) -> str:
    selected = f".{function.emitted_name}"
    closure = "\n".join(
        f"    *({section})"
        for section in sorted(set(function_sections))
        if section != selected
    )
    # Retail jump tables can be word-aligned despite GCC's eight-byte alignment.
    rodata_location = (
        f"  .rodata 0x{rodata_address:08x} : SUBALIGN(4) {{ *(.rodata*) *(.rdata*) }}"
        if rodata_address is not None
        else "  .rodata : { *(.rodata*) *(.rdata*) }"
    )
    return f"""SECTIONS
{{
  .{function.name} 0x{function.address:08x} :
  {{
    *({selected})
  }}
  .__function_closure :
  {{
{closure}
  }}
{rodata_location}
  .data : {{ *(.data*) *(.sdata*) }}
  .bss : {{ *(.bss*) *(.sbss*) *(COMMON) }}
}}
"""


def link_function(
    config: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    all_symbols: list[Symbol],
    function: FunctionRange,
    object_file: Path,
    function_sections: list[str],
    expected: bytes,
) -> dict[str, Any]:
    output = BUILD_ROOT / config["name"] / "functions" / function.name
    output.mkdir(parents=True, exist_ok=True)
    linked_object = output / f"{function.name}.elf"
    candidate_binary = output / "candidate.bin"
    expected_binary = output / "retail.bin"
    linker_script = output / "function.ld"
    error_file = output / "link-error.txt"
    section = f".{function.name}"
    reject_owned_aliases(config, [function])
    symbols = {symbol.name: symbol.address for symbol in all_symbols}
    symbols.update(
        {
            candidate: symbol_by_name(all_symbols, name).address
            for name, candidate in config.get("owned_symbols", {}).items()
        }
    )
    symbols.update(
        {
            name: ctr_match.parse_int(address)
            for name, address in config.get("link_symbols", {}).items()
        }
    )

    function_rodata = config.get("function_rodata_addresses", {}).get(function.name)
    linker_script.write_text(
        function_linker_script(
            function,
            function_sections,
            (
                ctr_match.parse_int(function_rodata)
                if function_rodata is not None
                else None
            ),
        )
    )
    command = [
        str(toolchain.binutils["ld"]),
        "--gc-sections",
        "-e",
        function.emitted_name,
        "-T",
        str(linker_script),
        "-o",
        str(linked_object),
    ]
    for name, address in symbols.items():
        if name != function.emitted_name:
            command.append(f"--defsym={name}=0x{address:08x}")
    command.append(str(object_file))
    try:
        ctr_match.run_checked(command)
    except ctr_match.MatchError as exc:
        error_file.write_text(f"{exc}\n")
        lines = [line for line in str(exc).splitlines() if line.strip()]
        summary = lines[-1] if lines else str(exc)
        raise ctr_match.MatchError(summary) from exc
    error_file.unlink(missing_ok=True)
    linked = owned_function(
        function,
        object_functions(toolchain.binutils["objdump"], linked_object),
        section,
    )
    placement_exact = (
        linked["address"] == function.address
        and linked["size"] == function.size
        and ctr_match.linked_section(toolchain.binutils["objdump"], linked_object, section)
        == {"address": function.address, "size": function.size}
    )
    ctr_match.extract_binary_section(
        toolchain, linked_object, section, candidate_binary
    )

    expected_binary.write_bytes(expected)
    comparison = ctr_match.compare_binary_files(
        toolchain,
        expected_binary,
        candidate_binary,
        function.address,
        output,
        f"retail/{function.name}",
        f"candidate/{function.name}",
    )
    comparison["exact"] = comparison["exact"] and placement_exact
    return {
        **function.result_fields(),
        "address": f"0x{function.address:08x}",
        **comparison,
        "candidate": str(candidate_binary.relative_to(ctr_match.ROOT)),
        "linker_script": str(linker_script.relative_to(ctr_match.ROOT)),
    }


def compare_linked_intervals(
    manifest: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    config: dict[str, Any],
    references: Path,
    functions: list[FunctionRange],
    linked_object: Path,
    intervals: list[tuple[str, Symbol, Symbol]],
) -> dict[str, Any]:
    """Measure function bodies in their real cross-interval link context."""
    output = linked_object.parent
    joint = "data_ranges" in config
    if joint:
        sections = ctr_match.allocated_sections(toolchain, linked_object, elf_metadata=True)
    else:
        headers = ctr_match.command_output(
            [str(toolchain.binutils["objdump"]), "-h", str(linked_object)]
        )
        sections = {
            name: {
                "size": int(size, 16),
                "address": int(address, 16),
                "flags": {flag.strip() for flag in flags.split(",")},
            }
            for name, size, address, flags in re.findall(
                r"(?m)^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+"
                r"([0-9a-fA-F]+)[^\n]*\n[ \t]+([^\n]+)",
                headers,
            )
        }
    allowed = {section for section, _, _ in intervals}
    if joint:
        allowed.update(interval["output_section"] for interval in ctr_match.data_ranges(config))
    if "rodata_address" in config["linked_block"]:
        allowed.add(".rodata")
    if any(
        section not in allowed and entry["size"] and "ALLOC" in entry["flags"]
        for section, entry in sections.items()
    ):
        raise ctr_match.MatchError("linked intervals contain an undeclared allocated section")
    owners = ctr_match.linked_symbols(toolchain.binutils["objdump"], linked_object, 0) if joint else {}
    symbols = owners if joint else object_functions(toolchain.binutils["objdump"], linked_object)
    if joint:
        if set(sections) != allowed:
            raise ctr_match.MatchError("joint intervals have a missing allocated section")
        ctr_match.reject_overlapping_ranges(sections.values(), "joint allocated sections")
        ctr_match.verify_initialized_owners(ctr_match.data_ranges(config), sections, owners)
    spans = []
    results = []
    for index, (section, start, end) in enumerate(intervals):
        entry = sections.get(section)
        if entry is None or not {"CONTENTS", "ALLOC", "LOAD", "CODE"}.issubset(
            entry["flags"]
        ):
            raise ctr_match.MatchError(f"expected allocated CODE section {section!r}")
        if joint and (entry["elf_type"] != 1 or entry["elf_flags"] != 6):
            raise ctr_match.MatchError(f"joint CODE section {section!r} is not AX PROGBITS")
        group = [
            function for function in functions
            if start.address <= function.address < end.address
        ]
        owned = [owned_function(function, symbols, section) for function in group]
        if joint and any(owners[function.emitted_name]["binding"] != "GLOBAL"
                         or owners[function.emitted_name]["visibility"] != "DEFAULT"
                         or owners[function.emitted_name]["type"] != "FUNC"
                         or owners[function.emitted_name]["count"] != 1 for function in group):
            raise ctr_match.MatchError("joint CODE requires one GLOBAL DEFAULT FUNC for every body")
        placed = (
            entry["address"] == start.address
            and entry["size"] == end.address - start.address
        )
        directory = output / f"interval-{index}"
        directory.mkdir(parents=True, exist_ok=True)
        code = directory / "code.bin"
        ctr_match.extract_binary_section(toolchain, linked_object, section, code)
        actual = code.read_bytes()
        expected = directory / "retail-code.bin"
        expected.write_bytes(
            ctr_match.reference_bytes(
                manifest, references,
                {"region": config["artifact"], "address": start.address,
                 "size": end.address - start.address},
            )
        )
        comparison = ctr_match.compare_binary_files(
            toolchain, expected, code, start.address, directory,
            "retail/resident-interval", "candidate/resident-interval",
        )
        for function, symbol in zip(group, owned):
            function_placed = (
                symbol["address"] == function.address and symbol["size"] == function.size
            )
            placed &= function_placed
            offset = symbol["address"] - entry["address"]
            if offset < 0 or offset + symbol["size"] > len(actual):
                raise ctr_match.MatchError(
                    f"owned function {function.emitted_name!r} is outside its CODE interval"
                )
            destination = BUILD_ROOT / config["name"] / "functions" / function.name
            destination.mkdir(parents=True, exist_ok=True)
            candidate = destination / "candidate.bin"
            candidate.write_bytes(actual[offset:offset + symbol["size"]])
            reference = destination / "retail.bin"
            reference.write_bytes(reference_slice(manifest, references, config, function))
            body = ctr_match.compare_binary_files(
                toolchain, reference, candidate, function.address, destination,
                "retail/function", "candidate/function",
            )
            results.append({
                **function.result_fields(), "address": f"0x{function.address:08x}",
                **body, "placement_exact": function_placed,
                "exact": body["exact"] and function_placed,
                "candidate": str(candidate.relative_to(ctr_match.ROOT)),
                **({"owner": owners[function.emitted_name]} if joint else {}),
            })
        spans.append({
            "section": section, "start_symbol": start.name, "end_symbol": end.name,
            "address": f"0x{start.address:08x}", "size": end.address - start.address,
            "placement_exact": placed, **comparison,
            "exact": comparison["exact"] and placed,
            "candidate": str(code.relative_to(ctr_match.ROOT)),
        })
    data = []
    if joint:
        data = ctr_match.compare_initialized_ranges(
            manifest, toolchain, config, references, linked_object, sections, owners,
        )
        for item in data:
            item["owner"] = owners[item["name"]]
    return {
        "elf": str(linked_object.relative_to(ctr_match.ROOT)),
        "elf_sha256": ctr_match.sha256_file(linked_object),
        "code_intervals": spans, "functions": results,
        "code_placed": all(span["placement_exact"] for span in spans),
        "code": {
            "matching_bytes": sum(span["matching_bytes"] for span in spans),
            "expected_size": sum(span["size"] for span in spans),
            "exact": all(span["exact"] for span in spans),
        },
        **({"data_ranges": data, "data": {
            "matching_bytes": sum(item["matching_bytes"] for item in data),
            "expected_size": sum(item["expected_size"] for item in data),
            "exact": all(item["exact"] for item in data),
        }} if joint else {}),
    }


def link_contiguous_block(
    manifest: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    config: dict[str, Any],
    references: Path,
    functions: list[FunctionRange],
    object_files: list[Path],
) -> dict[str, Any]:
    """Check actual function placement and optional initialized data in one link."""
    output = BUILD_ROOT / config["name"] / "linked_block"
    output.mkdir(parents=True, exist_ok=True)
    linked_object = output / "block.elf"
    linker_script = output / "block.ld"
    symbol_script = output / "symbols.ld"
    block = config["linked_block"]
    joint = "data_ranges" in config
    if joint and ("linker_script" not in config or "code_intervals" not in config):
        raise ctr_match.MatchError("joint initialized owners require linker_script and code_intervals")
    start = functions[0].address
    size = sum(function.size for function in functions)
    if ("rodata_address" in block) != ("rodata_size" in block):
        raise ctr_match.MatchError("linked block needs both rodata address and size")
    has_rodata = "rodata_address" in block
    rodata_address = ctr_match.parse_int(block["rodata_address"]) if has_rodata else None
    rodata_size = ctr_match.parse_int(block["rodata_size"]) if has_rodata else 0
    reject_owned_aliases(config, functions)
    if joint:
        ranges = ctr_match.data_ranges(config)
        selected = {interval["symbol"] for interval in ranges}
        if selected.intersection(config.get("link_symbols", {})):
            raise ctr_match.MatchError("link_symbols replaces a joint initialized owner")
        inputs = {}
        for object_file in object_files:
            for name, owner in ctr_match.linked_symbols(toolchain.binutils["objdump"], object_file, 0).items():
                if name in selected and owner["section"] != "*UND*":
                    count = inputs.get(name, {}).get("count", 0) + owner["count"]
                    inputs[name] = {**owner, "count": count}
        for interval in ranges:
            owner = inputs.get(interval["symbol"], {})
            if (owner.get("type") != "OBJECT" or owner.get("binding") != "GLOBAL"
                    or owner.get("visibility") != "DEFAULT" or owner.get("count") != 1
                    or owner.get("size") != ctr_match.parse_int(interval["size"])
                    or not owner.get("section", "").startswith(".")):
                raise ctr_match.MatchError("joint initialized owner is not a real input OBJECT definition")
    intervals = []
    if "code_intervals" in config:
        intervals = [
            (f".resident_block_{index}", first, last)
            for index, (first, last) in enumerate(namespace_intervals(
                config, parse_symbols(ctr_match.repository_path(config["symbol_file"]))
            ))
        ]
        code_script = ""
        for section, first, last in intervals:
            sections = " ".join(
                f"*(.{function.emitted_name})" for function in functions
                if first.address <= function.address < last.address
            )
            code_script += f"  {section} 0x{first.address:08x} : SUBALIGN(4) {{ {sections} }}\n"
    else:
        sections = " ".join(f"*(.{function.emitted_name})" for function in functions)
        code_script = f"  .resident_block 0x{start:08x} : SUBALIGN(4) {{ {sections} }}\n"
    rodata_script = (
        f"  .rodata 0x{rodata_address:08x} : SUBALIGN(4) "
        "{ *(.rodata*) *(.rdata*) }\n"
        if rodata_address is not None
        else ""
    )
    if "linker_script" in config:
        linker_script = ctr_match.repository_path(config["linker_script"])
    else:
        linker_script.write_text(
            "SECTIONS {\n"
            f"{code_script}"
            f"{rodata_script}"
            "  /DISCARD/ : { *(*) }\n"
            "}\n"
        )
    ctr_match.write_symbol_script(
        ctr_match.repository_path(config["symbol_file"]), symbol_script
    )
    command = [
        str(toolchain.binutils["ld"]),
        "-T",
        str(linker_script),
        "-T",
        str(symbol_script),
    ]
    command.extend(
        f"--defsym={name}=0x{ctr_match.parse_int(address):08x}"
        for name, address in config.get("link_symbols", {}).items()
    )
    ctr_match.run_checked(
        [*command, "-o", str(linked_object), *(str(path) for path in object_files)]
    )

    interval_result = None
    if intervals:
        interval_result = compare_linked_intervals(
            manifest, toolchain, config, references, functions, linked_object, intervals
        )
        code_placed = interval_result["code_placed"]
    else:
        symbols = object_functions(toolchain.binutils["objdump"], linked_object)
        owned = [
            owned_function(function, symbols, ".resident_block") for function in functions
        ]
        code_section = ctr_match.linked_section(
            toolchain.binutils["objdump"], linked_object, ".resident_block"
        )
        code_placed = code_section == {"address": start, "size": size} and all(
            symbol["address"] == function.address and symbol["size"] == function.size
            for function, symbol in zip(functions, owned)
        )
    rodata_section = (
        ctr_match.linked_section(toolchain.binutils["objdump"], linked_object, ".rodata")
        if has_rodata
        else None
    )
    rodata_placed = (
        rodata_section == {"address": rodata_address, "size": rodata_size}
        if has_rodata
        else None
    )

    if interval_result is not None:
        code_comparison = interval_result["code"]
    else:
        code = output / "code.bin"
        ctr_match.extract_binary_section(toolchain, linked_object, ".resident_block", code)
        expected_code = output / "retail-code.bin"
        expected_code.write_bytes(
            ctr_match.reference_bytes(
                manifest, references, {"region": config["artifact"], "address": start, "size": size},
            )
        )
        code_comparison = ctr_match.compare_binary_files(
            toolchain, expected_code, code, start, output,
            "retail/resident-block", "candidate/resident-block",
        )
    if has_rodata:
        rodata = output / "rodata.bin"
        ctr_match.extract_binary_section(toolchain, linked_object, ".rodata", rodata)
        expected_rodata = ctr_match.reference_bytes(
            manifest,
            references,
            {"region": config["artifact"], "address": rodata_address, "size": rodata_size},
        )
        actual_rodata = rodata.read_bytes()
    else:
        expected_rodata = actual_rodata = b""
    rodata_exact = actual_rodata == expected_rodata if has_rodata else None
    placement_exact = code_placed and (not has_rodata or rodata_placed)
    return {
        **({key: interval_result[key]
            for key in ("elf", "elf_sha256", "code_intervals", "functions")}
           if interval_result is not None else {}),
        **({key: interval_result[key] for key in ("data_ranges", "data")} if joint else {}),
        "address": f"0x{start:08x}",
        "size": size,
        "code_placement_exact": code_placed,
        "rodata_placement_exact": rodata_placed,
        "placement_exact": placement_exact,
        "code": code_comparison,
        "rodata_address": f"0x{rodata_address:08x}" if has_rodata else None,
        "rodata_size": rodata_size if has_rodata else None,
        "rodata_exact": rodata_exact,
        "rodata_candidate_size": len(actual_rodata) if has_rodata else None,
        "rodata_expected_sha256": (
            ctr_match.sha256_bytes(expected_rodata) if has_rodata else None
        ),
        "rodata_candidate_sha256": (
            ctr_match.sha256_bytes(actual_rodata) if has_rodata else None
        ),
        "exact": placement_exact and code_comparison["exact"] and (
            not has_rodata or rodata_exact
        ) and (not joint or interval_result["data"]["exact"]),
    }


def error_result(
    function: FunctionRange, error: str, error_file: str | None = None
) -> dict[str, Any]:
    result = {
        **function.result_fields(),
        "address": f"0x{function.address:08x}",
        "expected_size": function.size,
        "candidate_size": 0,
        "matching_bytes": 0,
        "matching_words": 0,
        "total_words": function.size // 4,
        "matching_byte_percent": 0.0,
        "exact": False,
        "error": error,
    }
    if error_file is not None:
        result["error_file"] = error_file
    return result


def write_result(destination: Path, result: dict[str, Any]) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "result.json").write_text(json.dumps(result, indent=2) + "\n")


def selected_functions(
    functions: list[FunctionRange], requested: list[str] | None
) -> list[FunctionRange]:
    if not requested:
        return functions
    by_name = {function.name: function for function in functions}
    unknown = sorted(set(requested) - set(by_name))
    if unknown:
        raise ctr_match.MatchError(
            f"namespace has no function(s): {', '.join(unknown)}"
        )
    requested_set = set(requested)
    return [function for function in functions if function.name in requested_set]


def build_resident(
    manifest: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    config: dict[str, Any],
    references: Path,
    requested: list[str] | None = None,
) -> dict[str, Any]:
    artifact = ctr_match.artifact_by_id(manifest, config["artifact"])
    if artifact["kind"] != "ps-x-exe":
        raise ctr_match.MatchError(f"{config['name']!r} is not a resident EXE target")
    symbols = parse_symbols(ctr_match.repository_path(config["symbol_file"]))
    inventory = discover_function_ranges(config, symbols)
    functions = selected_functions(inventory, requested)
    multiple_intervals = "code_intervals" in config
    interval_size = sum(function.size for function in inventory)
    print(
        f"resident {config['name']}: {len(inventory)} functions, "
        f"0x{interval_size:x} bytes, {len(namespace_sources(config))} sources"
    )

    sources = {
        function.source: ctr_match.repository_path(function.source)
        for function in (inventory if multiple_intervals else functions)
    }
    source_results: dict[str, dict[str, Any]] = {}
    for source_name, source in sources.items():
        compiled = compile_source(config, toolchain, source)
        source_results[source_name] = compiled
        if error := compiled.get("error"):
            first_line = error.splitlines()[-1] if error.splitlines() else error
            print(f"ERROR   {source_name}: {first_line}")
        else:
            print(f"OK      {source_name}")

    function_results: list[dict[str, Any]] = []
    owned_inputs_valid = True
    for function in (inventory if multiple_intervals else functions):
        source_result = source_results[function.source]
        if error := source_result.get("error"):
            owned_inputs_valid = False
            function_results.append(
                error_result(
                    function,
                    "source does not compile with GCC 2.8.1",
                    source_result.get("error_file"),
                )
            )
            continue
        object_file = source_result["object_path"]
        compiled_functions = object_functions(
            toolchain.binutils["objdump"], object_file
        )
        try:
            section = f".{function.emitted_name}"
            owned_function(function, compiled_functions, section)
            # Identity assembly entries can deliberately enclose a C body.
            if function.candidate_symbol is not None and sum(
                entry["section"] == section for entry in compiled_functions.values()
            ) != 1:
                raise ctr_match.MatchError(f"multiple functions share owned section {section!r}")
        except ctr_match.MatchError as exc:
            owned_inputs_valid = False
            function_results.append(
                error_result(
                    function,
                    str(exc),
                )
            )
            continue
        if multiple_intervals:
            function_results.append(
                error_result(function, "linked CODE intervals unavailable")
            )
            continue
        expected = reference_slice(manifest, references, config, function)
        try:
            result = link_function(
                config,
                toolchain,
                symbols,
                function,
                object_file,
                [entry["section"] for entry in compiled_functions.values()],
                expected,
            )
        except ctr_match.MatchError as exc:
            error_file = (
                BUILD_ROOT
                / config["name"]
                / "functions"
                / function.name
                / "link-error.txt"
            )
            result = error_result(
                function,
                str(exc),
                str(error_file.relative_to(ctr_match.ROOT))
                if error_file.is_file()
                else None,
            )
        function_results.append(result)
        if error := result.get("error"):
            print(f"ERROR   {function.name} {result['address']}: {error}")
        else:
            status = "MATCH" if result["exact"] else "DIFF"
            print(
                f"{status:<7} {function.name} {result['address']}: "
                f"{result['matching_bytes']}/{result['expected_size']} bytes, "
                f"{result['matching_words']}/{result['total_words']} words"
            )

    complete = len(functions) == len(inventory)
    block_result = None
    if (complete or multiple_intervals) and "linked_block" in config:
        try:
            if multiple_intervals and not owned_inputs_valid:
                raise ctr_match.MatchError(
                    "linked intervals contain an invalid owned input"
                )
            object_files = []
            for source in namespace_sources(config):
                source_name = str(source.relative_to(ctr_match.ROOT))
                source_result = source_results[source_name]
                if not source_result["compiled"]:
                    raise ctr_match.MatchError(
                        f"linked block source does not compile: {source_name}"
                    )
                object_files.append(source_result["object_path"])
            block_result = link_contiguous_block(
                manifest,
                toolchain,
                config,
                references,
                inventory,
                object_files,
            )
            if multiple_intervals:
                function_results = block_result.pop("functions")
        except ctr_match.MatchError as exc:
            block_result = {"exact": False, "error": str(exc)}
        status = "MATCH" if block_result["exact"] else "DIFF"
        if error := block_result.get("error"):
            print(f"{status:<7} linked block: {error}")
        else:
            code = block_result["code"]
            if block_result["rodata_address"] is None:
                rodata = "unmeasured"
            else:
                rodata = "exact" if block_result["rodata_exact"] else "different"
            placement = "exact" if block_result["placement_exact"] else "different"
            print(
                f"{status:<7} linked block: "
                f"{code['matching_bytes']}/{code['expected_size']} code bytes, "
                f"rodata {rodata}, placement {placement}"
                + (f", initialized data {block_result['data']['matching_bytes']}/{block_result['data']['expected_size']} bytes"
                   if "data" in block_result else "")
            )
    if multiple_intervals:
        selected_names = {function.name for function in functions}
        function_results = [
            result for result in function_results if result["name"] in selected_names
        ]
        for result in function_results:
            status = "MATCH" if result["exact"] else "DIFF"
            print(
                f"{status:<7} {result['name']} {result['address']}: "
                f"{result['matching_bytes']}/{result['expected_size']} bytes, "
                f"{result['matching_words']}/{result['total_words']} words"
            )
    exact_count = sum(function["exact"] for function in function_results)
    selected_exact = exact_count == len(functions) and (
        block_result is None or block_result["exact"]
    )
    result = {
        **ctr_match.build_evidence(
            manifest,
            toolchain,
            config,
            [
                path
                for source in source_results.values()
                for path in source.get("dependencies", [])
            ],
        ),
        "artifact": config["artifact"],
        "interval_start": f"0x{inventory[0].address:08x}",
        "interval_end": f"0x{inventory[-1].address + inventory[-1].size:08x}",
        "interval_size": interval_size,
        "function_count": len(inventory),
        "selected_function_count": len(functions),
        "exact_function_count": exact_count,
        "complete": complete,
        "selected_exact": selected_exact,
        "functions": function_results,
        "linked_block": block_result,
        "exact": complete and selected_exact,
    }
    write_result(BUILD_ROOT / config["name"], result)
    print(f"{exact_count}/{len(functions)} selected function ranges match exactly")
    return result
