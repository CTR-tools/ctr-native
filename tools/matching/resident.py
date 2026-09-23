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


def namespace_symbol_ranges(
    config: dict[str, Any], symbols: list[Symbol]
) -> list[tuple[Symbol, int]]:
    start = symbol_by_name(symbols, config["start_symbol"])
    end = symbol_by_name(symbols, config["end_symbol"])
    if start.address >= end.address:
        raise ctr_match.MatchError(
            f"namespace {config['name']!r} has an empty or reversed interval"
        )

    interval = [
        symbol for symbol in symbols if start.address <= symbol.address < end.address
    ]
    if not interval:
        raise ctr_match.MatchError(f"namespace {config['name']!r} has no symbols")
    if any(
        left.address >= right.address for left, right in zip(interval, interval[1:])
    ):
        raise ctr_match.MatchError(
            f"namespace {config['name']!r} has aliased or unordered symbols"
        )
    wrong_prefix = [
        symbol.name
        for symbol in interval
        if not symbol.name.startswith(config["symbol_prefix"])
    ]
    if wrong_prefix:
        raise ctr_match.MatchError(
            f"namespace {config['name']!r} contains symbols outside prefix "
            f"{config['symbol_prefix']!r}: {', '.join(wrong_prefix)}"
        )

    next_addresses = [symbol.address for symbol in interval[1:]] + [end.address]
    return [
        (symbol, next_address - symbol.address)
        for symbol, next_address in zip(interval, next_addresses)
    ]


def namespace_sources(config: dict[str, Any]) -> list[Path]:
    return [ctr_match.repository_path(path) for path in ctr_match.source_paths(config)]


def definition_count(text: str, symbol: str) -> int:
    pattern = re.compile(
        rf"(?m)^[ \t]*(?:(?:[A-Za-z_][A-Za-z0-9_]*)[ \t]+)+"
        rf"(?:\*[ \t]*)*{re.escape(symbol)}[ \t]*\([^;{{}}]*\)"
        rf"[ \t\r\n]*\{{"
    )
    return len(pattern.findall(text))


def discover_function_ranges(
    config: dict[str, Any], symbols: list[Symbol]
) -> list[FunctionRange]:
    sources = namespace_sources(config)
    source_text = {source: source.read_text() for source in sources}
    ranges: list[FunctionRange] = []
    for symbol, size in namespace_symbol_ranges(config, symbols):
        owners: list[Path] = []
        for source, text in source_text.items():
            count = definition_count(text, symbol.name)
            if count > 1:
                raise ctr_match.MatchError(
                    f"production source {source.relative_to(ctr_match.ROOT)} "
                    f"defines {symbol.name} {count} times"
                )
            if count == 1:
                owners.append(source)
        if len(owners) != 1:
            relative = [str(path.relative_to(ctr_match.ROOT)) for path in owners]
            raise ctr_match.MatchError(
                f"expected one production definition of {symbol.name}, "
                f"found {len(owners)}: {', '.join(relative) or 'none'}"
            )
        ranges.append(
            FunctionRange(
                name=symbol.name,
                address=symbol.address,
                size=size,
                source=str(owners[0].relative_to(ctr_match.ROOT)),
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
            functions[parts[-1]] = {
                "section": parts[-3],
                "size": int(parts[-2], 16),
            }
    return functions


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
    selected = f".{function.name}"
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
  {selected} 0x{function.address:08x} :
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
    symbols = {symbol.name: symbol.address for symbol in all_symbols}
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
        function.name,
        "-T",
        str(linker_script),
        "-o",
        str(linked_object),
    ]
    for name, address in symbols.items():
        if name != function.name:
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
    return {
        **asdict(function),
        "address": f"0x{function.address:08x}",
        **comparison,
        "candidate": str(candidate_binary.relative_to(ctr_match.ROOT)),
        "linker_script": str(linker_script.relative_to(ctr_match.ROOT)),
    }


def link_contiguous_block(
    manifest: dict[str, Any],
    toolchain: ctr_match.Toolchain,
    config: dict[str, Any],
    references: Path,
    functions: list[FunctionRange],
    object_file: Path,
) -> dict[str, Any]:
    """Check the actual function order and initialized data in one link."""
    output = BUILD_ROOT / config["name"] / "linked_block"
    output.mkdir(parents=True, exist_ok=True)
    linked_object = output / "block.elf"
    linker_script = output / "block.ld"
    symbol_script = output / "symbols.ld"
    block = config["linked_block"]
    start = functions[0].address
    size = sum(function.size for function in functions)
    rodata_address = ctr_match.parse_int(block["rodata_address"])
    rodata_size = ctr_match.parse_int(block["rodata_size"])
    sections = " ".join(f"*(.{function.name})" for function in functions)
    linker_script.write_text(
        "SECTIONS {\n"
        f"  .resident_block 0x{start:08x} : SUBALIGN(4) {{ {sections} }}\n"
        f"  .rodata 0x{rodata_address:08x} : SUBALIGN(4) "
        "{ *(.rodata*) *(.rdata*) }\n"
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
    ctr_match.run_checked([*command, "-o", str(linked_object), str(object_file)])

    symbols = ctr_match.linked_symbols(
        toolchain.binutils["objdump"], linked_object, start
    )
    placed = all(
        (symbol := symbols.get(function.name)) is not None
        and symbol["address"] == function.address
        and symbol["size"] == function.size
        and symbol["section"] == ".resident_block"
        for function in functions
    )

    code = output / "code.bin"
    rodata = output / "rodata.bin"
    ctr_match.extract_binary_section(toolchain, linked_object, ".resident_block", code)
    ctr_match.extract_binary_section(toolchain, linked_object, ".rodata", rodata)
    expected_code = output / "retail-code.bin"
    expected_code.write_bytes(
        ctr_match.reference_bytes(
            manifest,
            references,
            {"region": config["artifact"], "address": start, "size": size},
        )
    )
    code_comparison = ctr_match.compare_binary_files(
        toolchain,
        expected_code,
        code,
        start,
        output,
        "retail/resident-block",
        "candidate/resident-block",
    )
    expected_rodata = ctr_match.reference_bytes(
        manifest,
        references,
        {"region": config["artifact"], "address": rodata_address, "size": rodata_size},
    )
    actual_rodata = rodata.read_bytes()
    rodata_exact = actual_rodata == expected_rodata
    return {
        "address": f"0x{start:08x}",
        "size": size,
        "placement_exact": placed,
        "code": code_comparison,
        "rodata_address": f"0x{rodata_address:08x}",
        "rodata_size": rodata_size,
        "rodata_exact": rodata_exact,
        "rodata_candidate_size": len(actual_rodata),
        "rodata_expected_sha256": ctr_match.sha256_bytes(expected_rodata),
        "rodata_candidate_sha256": ctr_match.sha256_bytes(actual_rodata),
        "exact": placed and code_comparison["exact"] and rodata_exact,
    }


def error_result(
    function: FunctionRange, error: str, error_file: str | None = None
) -> dict[str, Any]:
    result = {
        **asdict(function),
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
    interval_size = sum(function.size for function in inventory)
    print(
        f"resident {config['name']}: {len(inventory)} functions, "
        f"0x{interval_size:x} bytes, {len(namespace_sources(config))} sources"
    )

    sources = {
        function.source: ctr_match.repository_path(function.source)
        for function in functions
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
    for function in functions:
        source_result = source_results[function.source]
        if error := source_result.get("error"):
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
        compiled = compiled_functions.get(function.name)
        if compiled is None:
            function_results.append(
                error_result(
                    function,
                    f"compiled object has no function section for {function.name}",
                )
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

    exact_count = sum(function["exact"] for function in function_results)
    complete = len(functions) == len(inventory)
    block_result = None
    if complete and "linked_block" in config:
        try:
            if len(source_results) != 1:
                raise ctr_match.MatchError("linked block requires one source")
            source_result = next(iter(source_results.values()))
            if not source_result["compiled"]:
                raise ctr_match.MatchError("linked block source does not compile")
            block_result = link_contiguous_block(
                manifest,
                toolchain,
                config,
                references,
                inventory,
                source_result["object_path"],
            )
        except ctr_match.MatchError as exc:
            block_result = {"exact": False, "error": str(exc)}
        status = "MATCH" if block_result["exact"] else "DIFF"
        if error := block_result.get("error"):
            print(f"{status:<7} linked block: {error}")
        else:
            code = block_result["code"]
            rodata = "exact" if block_result["rodata_exact"] else "different"
            placement = "exact" if block_result["placement_exact"] else "different"
            print(
                f"{status:<7} linked block: "
                f"{code['matching_bytes']}/{code['expected_size']} code bytes, "
                f"rodata {rodata}, placement {placement}"
            )
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
