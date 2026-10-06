#!/usr/bin/env python3
"""Deterministic byte-matching pipeline for CTR NTSC-U 926."""

from __future__ import annotations

import difflib
import hashlib
import json
import re
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[2]
PIPELINE_FILES = tuple(
    Path(__file__).with_name(name)
    for name in ("match.py", "pipeline.py", "resident.py")
)
METADATA = ROOT / "metadata" / "retail" / "ntsc-u-926"
DEFAULT_MANIFEST = METADATA / "matching.json"
SYMBOL_FILE = METADATA / "symbols" / "syms926.txt"
BUILD = ROOT / "build" / "matching" / "ntsc-u-926"
CHUNK_SIZE = 1024 * 1024


class MatchError(RuntimeError):
    """A concise, user-facing pipeline error."""


@dataclass(frozen=True)
class Toolchain:
    compiler_version: str
    compiler_banner: str
    compiler_directory: Path
    compiler: Path
    compiler_hashes: dict[str, str]
    maspsx: Path
    maspsx_commit: str
    maspsx_hashes: dict[str, str]
    default_aspsx_version: str
    assembler_banner: str
    binutils: dict[str, Path]
    binutils_hashes: dict[str, str]
    config_hash: str


def parse_int(value: int | str) -> int:
    if isinstance(value, int):
        return value
    return int(value, 0)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text())
    except FileNotFoundError as exc:
        raise MatchError(f"missing file: {path}") from exc
    except json.JSONDecodeError as exc:
        raise MatchError(f"invalid JSON in {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise MatchError(f"expected an object in {path}")
    return value


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(CHUNK_SIZE):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_json(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    return sha256_bytes(encoded)


def artifact_by_id(manifest: dict[str, Any], artifact_id: str) -> dict[str, Any]:
    for artifact in manifest["artifacts"]:
        if artifact["id"] == artifact_id:
            return artifact
    raise MatchError(f"manifest has no artifact named {artifact_id!r}")


def configured_targets(
    manifest: dict[str, Any], kind: str | None = None
) -> list[dict[str, Any]]:
    targets = manifest.get("targets")
    if not isinstance(targets, list):
        raise MatchError("matching manifest requires a targets list (schema 2)")
    names: set[str] = set()
    for target in targets:
        name = target["name"]
        if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", name) or name in names:
            raise MatchError(f"invalid or duplicate target name: {name!r}")
        if target["kind"] not in ("artifact", "data", "resident", "probe"):
            raise MatchError(f"unknown build kind for {name!r}: {target['kind']!r}")
        if "owned_symbols" in target and target["kind"] != "resident":
            raise MatchError("owned_symbols is only supported for resident targets")
        if "code_intervals" in target and target["kind"] != "resident":
            raise MatchError("code_intervals is only supported for resident targets")
        names.add(name)
    return [target for target in targets if kind is None or target["kind"] == kind]


def target_by_name(manifest: dict[str, Any], name: str) -> dict[str, Any]:
    for target in configured_targets(manifest):
        if target["name"] == name:
            return target
    raise MatchError(f"unknown matching target {name!r}")


def probe_by_symbol(manifest: dict[str, Any], symbol: str) -> dict[str, Any]:
    target = target_by_name(manifest, symbol)
    if target["kind"] != "probe":
        raise MatchError(f"{symbol!r} is not a compiler probe")
    return target


def reference_root(manifest: dict[str, Any], override: str | None) -> Path:
    if override:
        return Path(override).expanduser().resolve()
    return ROOT / manifest["reference_root"]


def check_psx_exe_header(path: Path, artifact: dict[str, Any]) -> list[str]:
    header = path.read_bytes()[:0x800]
    errors: list[str] = []
    if len(header) != 0x800:
        return [f"{artifact['id']}: PS-X EXE header is shorter than 0x800 bytes"]
    if header[:8] != b"PS-X EXE":
        errors.append(f"{artifact['id']}: missing PS-X EXE magic")

    fields = {
        "entrypoint": struct.unpack_from("<I", header, 0x10)[0],
        "global_pointer": struct.unpack_from("<I", header, 0x14)[0],
        "load_address": struct.unpack_from("<I", header, 0x18)[0],
        "mapped_size": struct.unpack_from("<I", header, 0x1C)[0],
        "stack_pointer": struct.unpack_from("<I", header, 0x30)[0],
    }
    expected = {
        "entrypoint": parse_int(artifact["header"]["entrypoint"]),
        "global_pointer": parse_int(artifact["header"]["global_pointer"]),
        "load_address": parse_int(artifact["load_address"]),
        "mapped_size": parse_int(artifact["mapped_size"]),
        "stack_pointer": parse_int(artifact["header"]["stack_pointer"]),
    }
    for name, actual in fields.items():
        if actual != expected[name]:
            errors.append(
                f"{artifact['id']}: {name} is 0x{actual:08x}, "
                f"expected 0x{expected[name]:08x}"
            )
    return errors


def verify_references(
    manifest: dict[str, Any], root: Path, selected: set[str] | None = None
) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for artifact in manifest["artifacts"]:
        if selected is not None and artifact["id"] not in selected:
            continue
        path = root / artifact["path"]
        result = {
            "id": artifact["id"],
            "path": str(path),
            "expected_size": artifact["size"],
            "expected_sha256": artifact["sha256"],
            "errors": [],
        }
        if not path.is_file():
            result["errors"].append("missing")
            results.append(result)
            continue

        actual_size = path.stat().st_size
        actual_hash = sha256_file(path)
        result["actual_size"] = actual_size
        result["actual_sha256"] = actual_hash
        if actual_size != artifact["size"]:
            result["errors"].append(
                f"size is {actual_size}, expected {artifact['size']}"
            )
        if actual_hash != artifact["sha256"]:
            result["errors"].append(
                f"sha256 is {actual_hash}, expected {artifact['sha256']}"
            )
        if artifact["kind"] == "ps-x-exe":
            result["errors"].extend(check_psx_exe_header(path, artifact))
        results.append(result)
    return results


def first_file_difference(expected: Path, actual: Path) -> int | None:
    offset = 0
    with expected.open("rb") as lhs, actual.open("rb") as rhs:
        while True:
            left = lhs.read(CHUNK_SIZE)
            right = rhs.read(CHUNK_SIZE)
            common = min(len(left), len(right))
            if left[:common] != right[:common]:
                for index, (left_byte, right_byte) in enumerate(
                    zip(left[:common], right[:common])
                ):
                    if left_byte != right_byte:
                        return offset + index
            if len(left) != len(right):
                return offset + common
            if not left:
                return None
            offset += len(left)


def mismatch_location(artifact: dict[str, Any], file_offset: int) -> str:
    mapped_offset = parse_int(artifact.get("mapped_file_offset", 0))
    if "load_address" in artifact and file_offset >= mapped_offset:
        address = parse_int(artifact["load_address"]) + file_offset - mapped_offset
        return f"file+0x{file_offset:x} / vram 0x{address:08x}"
    return f"file+0x{file_offset:x}"


def compare_artifacts(
    manifest: dict[str, Any],
    expected_root: Path,
    candidate_root: Path,
    selected: set[str] | None = None,
) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    for artifact in manifest["artifacts"]:
        if selected is not None and artifact["id"] not in selected:
            continue
        expected = expected_root / artifact["path"]
        actual = candidate_root / artifact["path"]
        result: dict[str, Any] = {
            "id": artifact["id"],
            "expected": str(expected),
            "candidate": str(actual),
            "exact": False,
        }
        if not expected.is_file():
            result["error"] = "reference missing"
        elif not actual.is_file():
            result["error"] = "candidate missing"
        else:
            result["expected_size"] = expected.stat().st_size
            result["candidate_size"] = actual.stat().st_size
            result["expected_sha256"] = sha256_file(expected)
            result["candidate_sha256"] = sha256_file(actual)
            if (
                result["expected_size"] != artifact["size"]
                or result["expected_sha256"] != artifact["sha256"]
            ):
                result["error"] = "reference does not match the committed oracle"
                results.append(result)
                continue
            result["exact"] = (
                result["expected_size"] == result["candidate_size"]
                and result["expected_sha256"] == result["candidate_sha256"]
            )
            if not result["exact"]:
                difference = first_file_difference(expected, actual)
                if difference is not None:
                    result["first_difference"] = difference
                    result["first_difference_location"] = mismatch_location(
                        artifact, difference
                    )
        results.append(result)
    return results


def _scan_balanced_body(text: str, opening_brace: int) -> int:
    depth = 0
    state = "code"
    index = opening_brace
    while index < len(text):
        char = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if char == "/" and following == "/":
                state = "line-comment"
                index += 2
                continue
            if char == "/" and following == "*":
                state = "block-comment"
                index += 2
                continue
            if char == '"':
                state = "string"
            elif char == "'":
                state = "character"
            elif char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    return index + 1
        elif state == "line-comment":
            if char == "\n":
                state = "code"
        elif state == "block-comment":
            if char == "*" and following == "/":
                state = "code"
                index += 2
                continue
        elif state in {"string", "character"}:
            if char == "\\":
                index += 2
                continue
            if (state == "string" and char == '"') or (
                state == "character" and char == "'"
            ):
                state = "code"
        index += 1
    raise MatchError("unterminated function body")


def extract_function(text: str, symbol: str) -> str:
    pattern = re.compile(
        rf"(?m)^[ \t]*[^\n;{{}}]*\b{re.escape(symbol)}[ \t]*\([^;{{}}]*\)"
        rf"[ \t]*(?:\n[ \t]*)?\{{"
    )
    matches = list(pattern.finditer(text))
    if len(matches) != 1:
        raise MatchError(f"expected one definition of {symbol}, found {len(matches)}")
    match = matches[0]
    opening_brace = text.find("{", match.start(), match.end())
    end = _scan_balanced_body(text, opening_brace)
    return text[match.start() : end].strip() + "\n"


def command_output(command: list[str]) -> str:
    result = subprocess.run(
        command, check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    return result.stdout.strip()


def repository_path(value: str) -> Path:
    path = (ROOT / value).resolve()
    try:
        path.relative_to(ROOT)
    except ValueError as exc:
        raise MatchError(f"path escapes the repository: {value}") from exc
    return path


def require_tool(path: Path, description: str, expected_hash: str) -> Path:
    if not path.is_file():
        raise MatchError(f"vendored {description} is missing: {path}")
    actual_hash = sha256_file(path)
    if actual_hash != expected_hash:
        raise MatchError(
            f"vendored {description} has sha256 {actual_hash}, expected {expected_hash}"
        )
    return path


def resolve_toolchain(manifest: dict[str, Any]) -> Toolchain:
    config = manifest["toolchain"]
    compiler = config["compiler"]
    gcc_dir = repository_path(compiler["directory"])
    compiler_tools = {}
    for name, expected_hash in compiler["sha256"].items():
        compiler_tools[name] = require_tool(
            gcc_dir / name,
            f"GCC {compiler['version']} {name}",
            expected_hash,
        )
    gcc = compiler_tools[compiler["executable"]]

    maspsx_config = config["maspsx"]
    maspsx_dir = repository_path(maspsx_config["directory"])
    maspsx_files = {}
    for name, expected_hash in maspsx_config["sha256"].items():
        maspsx_files[name] = require_tool(
            maspsx_dir / name,
            f"maspsx {name}",
            expected_hash,
        )
    maspsx = maspsx_files[maspsx_config["entrypoint"]]

    binutils = config["binutils"]
    prefix = repository_path(binutils["prefix"])
    tools = {}
    for name, expected_hash in binutils["sha256"].items():
        tools[name] = require_tool(
            Path(f"{prefix}{name}"),
            f"GNU binutils {name}",
            expected_hash,
        )

    compiler_banner = command_output([str(gcc), "--version"]).splitlines()[0]
    if compiler_banner != compiler["version"]:
        raise MatchError(
            f"vendored GCC reports {compiler_banner}, expected {compiler['version']}"
        )

    assembler_banner = command_output([str(tools["as"]), "--version"]).splitlines()[0]
    if binutils["version"] not in assembler_banner:
        raise MatchError(
            f"vendored assembler reports {assembler_banner}, "
            f"expected binutils {binutils['version']}"
        )

    return Toolchain(
        compiler_version=compiler["version"],
        compiler_banner=compiler_banner,
        compiler_directory=gcc_dir,
        compiler=gcc,
        compiler_hashes=compiler["sha256"],
        maspsx=maspsx,
        maspsx_commit=maspsx_config["commit"],
        maspsx_hashes=maspsx_config["sha256"],
        default_aspsx_version=maspsx_config["aspsx_version"],
        assembler_banner=assembler_banner,
        binutils=tools,
        binutils_hashes=binutils["sha256"],
        config_hash=sha256_json(config),
    )


def reference_bytes(
    manifest: dict[str, Any], root: Path, probe: dict[str, Any]
) -> bytes:
    artifact = artifact_by_id(manifest, probe["region"])
    address = parse_int(probe["address"])
    load_address = parse_int(artifact["load_address"])
    file_offset = parse_int(artifact.get("mapped_file_offset", 0))
    offset = file_offset + address - load_address
    if offset < 0:
        raise MatchError(f"{probe['symbol']} precedes {artifact['id']} load address")
    path = root / artifact["path"]
    if not path.is_file():
        raise MatchError(f"reference artifact is missing: {path}")
    if (
        path.stat().st_size != artifact["size"]
        or sha256_file(path) != artifact["sha256"]
    ):
        raise MatchError(
            f"reference artifact does not match the committed oracle: {path}"
        )
    with path.open("rb") as stream:
        stream.seek(offset)
        value = stream.read(probe["size"])
    if len(value) != probe["size"]:
        raise MatchError(
            f"reference slice for {probe['symbol']} is {len(value)} bytes, "
            f"expected {probe['size']}"
        )
    return value


def run_checked(command: list[str], stdin_path: Path | None = None) -> None:
    if stdin_path is None:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        with stdin_path.open("rb") as stream:
            result = subprocess.run(
                command, stdin=stream, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
    if result.returncode != 0:
        rendered = " ".join(command)
        details = result.stderr.decode(errors="replace").strip()
        suffix = f"\n{details}" if details else ""
        raise MatchError(f"command failed ({result.returncode}): {rendered}{suffix}")


def compile_c(
    toolchain: Toolchain,
    source: Path,
    assembly: Path,
    flags: Iterable[str],
    include_directories: Iterable[Path] = (),
    forced_includes: Iterable[Path] = (),
    dependency_file: Path | None = None,
) -> None:
    command = [
        str(toolchain.compiler),
        f"-B{toolchain.compiler_directory}/",
        "-x",
        "c",
        "-S",
        "-o",
        str(assembly),
        *flags,
    ]
    if dependency_file is not None:
        command.append(f"-Wp,-MD,{dependency_file}")
    command.extend(f"-I{path}" for path in include_directories)
    for path in forced_includes:
        command.extend(["-include", str(path)])
    command.append(str(source))
    run_checked(command)


def assemble_compiler_output(
    toolchain: Toolchain,
    assembly: Path,
    object_file: Path,
    aspsx_version: str,
    small_data_limit: int,
) -> None:
    run_checked(
        [
            sys.executable,
            str(toolchain.maspsx),
            f"--aspsx-version={aspsx_version}",
            "--expand-div",
            "--run-assembler",
            f"--gnu-as-path={toolchain.binutils['as']}",
            "--dont-force-G0",
            f"-G{small_data_limit}",
            "-o",
            str(object_file),
        ],
        stdin_path=assembly,
    )


def assemble_mips_source(
    toolchain: Toolchain,
    source: Path,
    object_file: Path,
    small_data_limit: int,
    include_directories: Iterable[Path] = (),
    dependency_file: Path | None = None,
) -> None:
    command = [
        str(toolchain.binutils["as"]),
        f"-G{small_data_limit}",
    ]
    command.extend(f"-I{path}" for path in include_directories)
    if dependency_file is not None:
        command.extend(["--MD", str(dependency_file)])
    command.extend(["-o", str(object_file), str(source)])
    run_checked(command)


def extract_binary_section(
    toolchain: Toolchain,
    source: Path,
    section: str,
    output: Path,
) -> None:
    run_checked(
        [
            str(toolchain.binutils["objcopy"]),
            "-O",
            "binary",
            "-j",
            section,
            str(source),
            str(output),
        ]
    )


def mismatch_offsets(expected: bytes, actual: bytes, limit: int = 16) -> list[int]:
    offsets = [
        index
        for index, (left, right) in enumerate(zip(expected, actual))
        if left != right
    ]
    if len(actual) != len(expected):
        offsets.extend(
            range(min(len(actual), len(expected)), max(len(actual), len(expected)))
        )
    return offsets[:limit]


def range_comparison(
    expected: bytes,
    actual: bytes,
    ranges: list[dict[str, Any]],
    symbols: dict[str, dict[str, int]] | None = None,
) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    symbols = symbols or {}
    for item in ranges:
        offset = parse_int(item["offset"])
        size = parse_int(item["size"])
        candidate_offset = offset
        candidate_size = size
        if symbol_name := item.get("candidate_symbol"):
            if symbol_name not in symbols:
                raise MatchError(f"linked artifact has no symbol named {symbol_name!r}")
            candidate_offset = symbols[symbol_name]["offset"]
            symbol_size = symbols[symbol_name]["size"]
            if symbol_size:
                candidate_size = min(size, symbol_size)
        expected_slice = expected[offset : offset + size]
        actual_slice = actual[candidate_offset : candidate_offset + candidate_size]
        matching_bytes = sum(
            left == right for left, right in zip(expected_slice, actual_slice)
        )
        result = {
            **item,
            "offset": offset,
            "size": size,
            "candidate_offset": candidate_offset,
            "candidate_size": len(actual_slice),
            "placement_delta": candidate_offset - offset,
            "matching_bytes": matching_bytes,
            "expected_sha256": sha256_bytes(expected_slice),
            "candidate_sha256": sha256_bytes(actual_slice),
            "content_exact": actual_slice == expected_slice,
            "placement_exact": candidate_offset == offset,
            "exact": candidate_offset == offset and actual_slice == expected_slice,
        }
        if item["kind"] == "code":
            matching_words = sum(
                actual_slice[index : index + 4] == expected_slice[index : index + 4]
                for index in range(0, len(expected_slice), 4)
                if len(actual_slice[index : index + 4]) == 4
            )
            result["matching_words"] = matching_words
            result["total_words"] = len(expected_slice) // 4
        results.append(result)
    return results


def linked_symbols(
    objdump: Path, linked_object: Path, load_address: int
) -> dict[str, dict[str, Any]]:
    output = command_output([str(objdump), "-t", str(linked_object)])
    symbols: dict[str, dict[str, Any]] = {}
    for line in output.splitlines():
        parts = line.split()
        visibility = "DEFAULT"
        if len(parts) >= 3 and parts[-2] in (".hidden", ".protected", ".internal"):
            visibility = parts.pop(-2)[1:].upper()
        if (
            len(parts) >= 3
            and re.fullmatch(r"[0-9a-fA-F]{8}", parts[0])
            and re.fullmatch(r"[0-9a-fA-F]{8}", parts[-2])
        ):
            address = int(parts[0], 16)
            flags = parts[1:-3]
            name = parts[-1]
            symbols[parts[-1]] = {
                "address": address,
                "offset": address - load_address,
                "size": int(parts[-2], 16),
                "section": parts[-3],
                "kind": "code" if "F" in flags else "data",
                "type": "FUNC" if "F" in flags else "OBJECT" if "O" in flags else "NOTYPE",
                "binding": "WEAK" if "w" in flags or "v" in flags else "GLOBAL" if "g" in flags else "LOCAL",
                "visibility": visibility,
                "count": symbols.get(name, {}).get("count", 0) + 1,
            }
    return symbols


def linked_section(objdump: Path, linked_object: Path, name: str) -> dict[str, int]:
    headers = command_output([str(objdump), "-h", str(linked_object)])
    match = re.search(
        rf"(?m)^\s*\d+\s+{re.escape(name)}\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+",
        headers,
    )
    if match is None:
        raise MatchError(f"linked artifact has no section named {name!r}")
    return {"size": int(match[1], 16), "address": int(match[2], 16)}


def normalized_objdump(command: list[str]) -> str:
    output = command_output(command)
    instruction = re.compile(r"^[0-9a-f]+:\s")
    lines = [line.rstrip() for line in output.splitlines() if instruction.match(line)]
    return "\n".join(lines) + "\n"


def compare_binary_files(
    toolchain: Toolchain,
    expected_path: Path,
    candidate_path: Path,
    load_address: int,
    output: Path,
    expected_label: str,
    candidate_label: str,
) -> dict[str, Any]:
    expected = expected_path.read_bytes()
    actual = candidate_path.read_bytes()
    objdump_base = [
        str(toolchain.binutils["objdump"]),
        "-D",
        "-b",
        "binary",
        "-m",
        "mips:3000",
        "-EL",
        f"--adjust-vma=0x{load_address:08x}",
    ]
    expected_disassembly = normalized_objdump([*objdump_base, str(expected_path)])
    candidate_disassembly = normalized_objdump([*objdump_base, str(candidate_path)])
    expected_objdump = output / "retail.objdump"
    candidate_objdump = output / "candidate.objdump"
    diff_path = output / "objdump.diff"
    expected_objdump.write_text(expected_disassembly)
    candidate_objdump.write_text(candidate_disassembly)
    diff_path.write_text(
        "".join(
            difflib.unified_diff(
                expected_disassembly.splitlines(keepends=True),
                candidate_disassembly.splitlines(keepends=True),
                fromfile=expected_label,
                tofile=candidate_label,
            )
        )
    )

    matching_bytes = sum(left == right for left, right in zip(expected, actual))
    compared_size = max(len(expected), len(actual))
    matching_words = sum(
        actual[index : index + 4] == expected[index : index + 4]
        for index in range(0, len(expected), 4)
        if len(actual[index : index + 4]) == 4
    )
    return {
        "expected_size": len(expected),
        "candidate_size": len(actual),
        "matching_bytes": matching_bytes,
        "matching_words": matching_words,
        "total_words": len(expected) // 4,
        "matching_byte_percent": matching_bytes * 100 / compared_size
        if compared_size
        else 100.0,
        "expected_sha256": sha256_bytes(expected),
        "candidate_sha256": sha256_bytes(actual),
        "first_mismatch_offsets": mismatch_offsets(expected, actual),
        "objdump_diff": str(diff_path.relative_to(ROOT)),
        "exact": actual == expected,
    }


def dependency_inputs(path: Path) -> list[str]:
    text = path.read_text().replace("\\\n", " ")
    try:
        _, values = text.split(":", 1)
    except ValueError as exc:
        raise MatchError(f"invalid dependency file: {path}") from exc
    inputs: list[str] = []
    for value in values.split():
        dependency = repository_path(value)
        inputs.append(str(dependency.relative_to(ROOT)))
    return inputs


def load_symbol_file(path: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        value = line.split("//", 1)[0].strip()
        if not value:
            continue
        fields = value.split()
        if len(fields) > 2 and fields[1] == "------":
            continue
        if len(fields) != 2 or not re.fullmatch(r"[0-9a-fA-F]{8}", fields[0]):
            raise MatchError(f"invalid symbol at {path}:{line_number}: {line}")
        name = fields[1]
        if name in symbols:
            raise MatchError(f"duplicate symbol {name!r} in {path}")
        symbols[name] = int(fields[0], 16)
    return symbols


def write_symbol_script(source: Path, destination: Path) -> None:
    """Expose retail addresses only for names the artifact does not define."""
    symbols = load_symbol_file(source)
    destination.write_text(
        "".join(
            f"PROVIDE({name} = 0x{address:08x});\n" for name, address in symbols.items()
        )
    )


def artifact_ranges(
    build: dict[str, Any],
    load_address: int,
    linked: dict[str, dict[str, Any]],
) -> list[dict[str, Any]]:
    ranges = list(build.get("ranges", []))
    retail = load_symbol_file(SYMBOL_FILE)
    reported = {value.get("candidate_symbol", value["name"]) for value in ranges}
    for name, symbol in linked.items():
        if (
            name not in reported
            and name in retail
            and symbol["kind"] == "code"
            and symbol["section"] == build["output_section"]
            and symbol["size"] > 0
        ):
            ranges.append(
                {
                    "name": name,
                    "kind": "code",
                    "candidate_symbol": name,
                    "offset": f"0x{retail[name] - load_address:x}",
                    "size": f"0x{symbol['size']:x}",
                }
            )

    return sorted(ranges, key=lambda value: parse_int(value["offset"]))


def source_paths(config: dict[str, Any]) -> list[str]:
    sources = config.get("sources")
    if isinstance(sources, dict):
        directory = repository_path(sources["directory"])
        paths = sorted(directory.glob(sources["glob"]))
        sources = [str(path.relative_to(ROOT)) for path in paths]
    if not sources:
        sources = [config["source"]] if "source" in config else []
    if not sources:
        raise MatchError(f"target {config['name']!r} has no production sources")
    overrides = config.get("source_flags", {})
    if not isinstance(overrides, dict):
        raise MatchError("source_flags must map C source paths to flag lists")
    if overrides and config["kind"] == "probe":
        raise MatchError("source_flags apply to complete C inputs, not extracted probes")
    for path, flags in overrides.items():
        if path not in sources or Path(path).suffix.lower() != ".c":
            raise MatchError(f"source_flags entry {path!r} is not a declared C input")
        if path != str(repository_path(path).relative_to(ROOT)):
            raise MatchError(f"source_flags path {path!r} must be repository-relative and canonical")
        if not isinstance(flags, list) or not all(isinstance(flag, str) for flag in flags):
            raise MatchError(f"source_flags entry {path!r} must be a flag list")
    return list(sources)


def effective_flags(config: dict[str, Any], source: str | None = None) -> list[str]:
    flags = list(config["compiler_flags"])
    if "source_flags" in config:
        source_paths(config)
        flags.extend(config["source_flags"].get(source, []))
    if config["kind"] == "resident" and "-ffunction-sections" not in flags:
        flags.append("-ffunction-sections")
    return flags


def source_compiler_flags(config: dict[str, Any]) -> dict[str, list[str]] | None:
    if "source_flags" not in config:
        return None
    sources = source_paths(config)
    if not config["source_flags"]:
        return None
    return {source: effective_flags(config, source) for source in sources
            if Path(source).suffix.lower() == ".c"}


def declared_inputs(config: dict[str, Any]) -> set[str]:
    paths = set(source_paths(config)) | set(config.get("forced_includes", []))
    if config["kind"] != "probe":
        paths.add(config.get("symbol_file", str(SYMBOL_FILE.relative_to(ROOT))))
    if "linker_script" in config:
        paths.add(config["linker_script"])
    return paths


def input_hashes(paths: Iterable[str]) -> dict[str, str]:
    return {path: sha256_file(repository_path(path)) for path in sorted(set(paths))}


def pipeline_hash() -> str:
    return sha256_json({path.name: sha256_file(path) for path in PIPELINE_FILES})


def tool_inputs(manifest: dict[str, Any]) -> dict[str, str]:
    tools = manifest["toolchain"]
    return {
        **{
            f"{tools['compiler']['directory']}/{name}": digest
            for name, digest in tools["compiler"]["sha256"].items()
        },
        **{
            f"{tools['maspsx']['directory']}/{name}": digest
            for name, digest in tools["maspsx"]["sha256"].items()
        },
        **{
            f"{tools['binutils']['prefix']}{name}": digest
            for name, digest in tools["binutils"]["sha256"].items()
        },
    }


def build_evidence(
    manifest: dict[str, Any],
    toolchain: Toolchain,
    config: dict[str, Any],
    dependencies: Iterable[str],
) -> dict[str, Any]:
    artifact = artifact_by_id(manifest, config.get("artifact", config.get("region")))
    inputs = input_hashes(declared_inputs(config) | set(dependencies))
    inputs.update(tool_inputs(manifest))
    evidence = {
        "schema_version": 2,
        "target": manifest["target"],
        "name": config["name"],
        "kind": config["kind"],
        "scope": {
            "artifact": "whole-artifact",
            "data": "initialized-data-ranges",
            "resident": "function-ranges",
            "probe": "extracted-function",
        }[config["kind"]],
        "config_sha256": sha256_json(config),
        "reference_config_sha256": sha256_json(artifact),
        "pipeline_sha256": pipeline_hash(),
        "build_input_sha256": inputs,
        "compiler_version": toolchain.compiler_banner,
        "compiler_sha256": toolchain.compiler_hashes,
        "compiler_flags": effective_flags(config),
        "aspsx_version": config.get("aspsx_version", toolchain.default_aspsx_version),
        "maspsx_commit": toolchain.maspsx_commit,
        "maspsx_sha256": toolchain.maspsx_hashes,
        "binutils_version": toolchain.assembler_banner,
        "binutils_sha256": toolchain.binutils_hashes,
        "toolchain_config_sha256": toolchain.config_hash,
    }
    source_flags = source_compiler_flags(config)
    if source_flags is not None:
        evidence["source_compiler_flags"] = source_flags
    return evidence


def result_path(manifest: dict[str, Any], config: dict[str, Any]) -> Path:
    if config["kind"] == "probe":
        tools = manifest["toolchain"]
        version = config.get("aspsx_version", tools["maspsx"]["aspsx_version"])
        variant = f"gcc-{tools['compiler']['version']}-aspsx-{version}"
        return BUILD / "probes" / config["name"] / variant / "result.json"
    directory = "artifacts" if config["kind"] in ("artifact", "data") else "namespaces"
    return BUILD / directory / config["name"] / "result.json"


def result_is_current(
    manifest: dict[str, Any],
    config: dict[str, Any],
    result: dict[str, Any],
) -> bool:
    try:
        artifact = artifact_by_id(
            manifest, config.get("artifact", config.get("region"))
        )
        inputs = result["build_input_sha256"]
        required = declared_inputs(config) | set(tool_inputs(manifest))
        if not required.issubset(inputs):
            return False
        source_flags = source_compiler_flags(config)
        if (
            result["schema_version"] != 2
            or result["name"] != config["name"]
            or result["config_sha256"] != sha256_json(config)
            or result["reference_config_sha256"] != sha256_json(artifact)
            or result["pipeline_sha256"] != pipeline_hash()
            or result["toolchain_config_sha256"] != sha256_json(manifest["toolchain"])
            or result["compiler_flags"] != effective_flags(config)
            or result.get("source_compiler_flags") != source_flags
            or (
                config["kind"] == "data"
                and (
                    result.get("kind") != "data"
                    or result.get("scope") != "initialized-data-ranges"
                )
            )
            or result["aspsx_version"]
            != config.get(
                "aspsx_version", manifest["toolchain"]["maspsx"]["aspsx_version"]
            )
            or inputs != input_hashes(inputs)
        ):
            return False
        owned = config.get("owned_symbols", {})
        candidates = (
            result["functions"] if config["kind"] == "resident"
            else result["ranges"] if config["kind"] == "data"
            else [result]
        )
        if config["kind"] == "data" and (
            [item["data_range"] for item in candidates] != data_ranges(config)
            or any(item.get("error") for item in candidates)
        ):
            return False
        if config["kind"] == "resident" and (
            len(candidates) != result["selected_function_count"]
            or result["complete"] != (len(candidates) == result["function_count"])
            or len({item["name"] for item in candidates}) != len(candidates)
            or not isinstance(owned, dict)
            or any(
                item.get("candidate_symbol", item["name"])
                != owned.get(item["name"], item["name"])
                or ("candidate_symbol" in item)
                != (owned.get(item["name"], item["name"]) != item["name"])
                for item in candidates
            )
        ):
            return False
        if "code_intervals" in config:
            block = result.get("linked_block")
            if not isinstance(block, dict):
                return False
            if "data_ranges" in config and block.get("error") and (
                result.get("exact") or result.get("selected_exact") or result.get("exact_function_count")
                or any(item.get("exact") for item in candidates)
            ):
                return False
            if not block.get("error"):
                symbols = load_symbol_file(repository_path(config["symbol_file"]))
                intervals = block["code_intervals"]
                if len(intervals) != len(config["code_intervals"]):
                    return False
                for index, (specification, interval) in enumerate(
                    zip(config["code_intervals"], intervals)
                ):
                    start = symbols[specification["start_symbol"]]
                    end = symbols[specification["end_symbol"]]
                    if (
                        interval["start_symbol"] != specification["start_symbol"]
                        or interval["end_symbol"] != specification["end_symbol"]
                        or interval["section"] != f".resident_block_{index}"
                        or interval["address"] != f"0x{start:08x}"
                        or interval["size"] != end - start
                        or sha256_file(repository_path(interval["candidate"]))
                        != interval["candidate_sha256"]
                    ):
                        return False
                if sha256_file(repository_path(block["elf"])) != block["elf_sha256"]:
                    return False
                if "data_ranges" in config:
                    ranges = block["data_ranges"]
                    if [item["data_range"] for item in ranges] != data_ranges(config):
                        return False
                    sections = {}
                    symbols = {}
                    for item in ranges:
                        specification = item["data_range"]
                        section = item["section"]
                        name = specification["output_section"]
                        size = parse_int(specification["size"])
                        if (item.get("error") or item["name"] != specification["symbol"]
                                or item["kind"] != "data" or section["name"] != name
                                or item["address"] != f"0x{parse_int(specification['symbol_address']):08x}"
                                or item["size"] != size or item["expected_size"] != size
                                or item["candidate_size"] != size
                                or sha256_file(repository_path(item["candidate"])) != item["candidate_sha256"]
                                or (name in sections and sections[name] != section)):
                            return False
                        sections[name] = section
                        symbols[item["name"]] = item["owner"]
                    verify_initialized_owners(data_ranges(config), sections, symbols)
                    if block["data"] != {
                        "matching_bytes": sum(item["matching_bytes"] for item in ranges),
                        "expected_size": sum(item["expected_size"] for item in ranges),
                        "exact": all(item["exact"] for item in ranges),
                    }:
                        return False
                    for item in candidates:
                        owner = item["owner"]
                        if (owner["type"] != "FUNC" or owner["binding"] != "GLOBAL"
                                or owner["visibility"] != "DEFAULT" or owner["count"] != 1
                                or owner["address"] != parse_int(item["address"])
                                or owner["size"] != item["size"]
                                or owner["section"] not in {span["section"] for span in intervals}):
                            return False
        return all(
            item.get("error")
            or (
                sha256_file(repository_path(item["candidate"]))
                == item["candidate_sha256"]
            )
            for item in candidates
        )
    except (KeyError, TypeError, OSError, MatchError):
        return False


def normalize_compiler_directives(assembly: Path) -> None:
    """Normalize GCC directives that maspsx cannot pass to GNU as.

    This only removes redundant expression-named extern declarations and
    closes GCC's small-data state before custom function sections. It never
    changes instructions or injects candidate bytes.
    """
    lines: list[str] = []
    in_small_data = False
    for line in assembly.read_text().splitlines(keepends=True):
        directive = line.strip()
        if re.fullmatch(r"\.extern\s+[^,]+[+-]\d+\s*,\s*\d+", directive) or re.fullmatch(
            r"\.extern\s+[+-]?\d+\(\$(?:28|gp)\)\s*,\s*\d+", directive
        ):
            # GCC emits invalid .extern directives for declarations bound to
            # a linker expression or a gp-relative slot. The load/store still
            # carries that exact address, so the declaration is redundant.
            continue
        if directive == ".sdata":
            in_small_data = True
        elif in_small_data and directive.startswith(".section "):
            # maspsx parses directives as small-data values until a standard
            # section switch closes that state. GCC 2.8 emits the custom
            # function section directly, so make the implied switch explicit.
            lines.append("\t.text\n")
            in_small_data = False
        elif directive in (".text", ".data", ".rdata", ".rodata"):
            in_small_data = False
        lines.append(line)
    assembly.write_text("".join(lines))


def build_object(
    toolchain: Toolchain,
    config: dict[str, Any],
    source: Path,
    output: Path,
) -> tuple[Path, list[str]]:
    output.parent.mkdir(parents=True, exist_ok=True)
    object_file = output.with_suffix(".o")
    dependency_file = output.with_suffix(".d")
    includes = [repository_path(path) for path in config.get("include_directories", [])]
    if source.suffix.lower() == ".c":
        assembly = output.with_suffix(".s")
        compile_c(
            toolchain,
            source,
            assembly,
            effective_flags(config, str(source.relative_to(ROOT))),
            includes,
            [repository_path(path) for path in config.get("forced_includes", [])],
            dependency_file,
        )
        if config["kind"] == "resident":
            normalize_compiler_directives(assembly)
        assemble_compiler_output(
            toolchain,
            assembly,
            object_file,
            config.get("aspsx_version", toolchain.default_aspsx_version),
            config["small_data_limit"],
        )
    elif source.suffix.lower() == ".s":
        assemble_mips_source(
            toolchain,
            source,
            object_file,
            config["small_data_limit"],
            includes,
            dependency_file,
        )
    else:
        raise MatchError(f"unsupported source: {source}")
    return object_file, dependency_inputs(dependency_file)


def allocated_sections(
    toolchain: Toolchain, linked: Path, *, elf_metadata: bool = False,
) -> dict[str, dict[str, Any]]:
    """Keep initialized bytes distinct from allocated-only storage such as BSS."""
    headers = command_output([str(toolchain.binutils["objdump"]), "-h", str(linked)])
    sections = {}
    pattern = r"^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)[^\n]*\n\s+([^\n]+)"
    for name, size_hex, address_hex, flags in re.findall(pattern, headers, re.MULTILINE):
        size, address = int(size_hex, 16), int(address_hex, 16)
        attributes = {flag.strip() for flag in flags.split(",")}
        if size and "ALLOC" in attributes:
            if elf_metadata and name in sections:
                raise MatchError("joint ELF has duplicate allocated section names")
            sections[name] = {"size": size, "address": address, "initialized": "CONTENTS" in attributes}
            if elf_metadata:
                sections[name]["flags"] = sorted(attributes)
    if elf_metadata:
        # objdump's BFD flags do not expose literal ELF section types/SHF bits.
        # Read only the bounded ELF32/MIPS section table for this joint audit.
        actual = linked.read_bytes()
        if (len(actual) < 52 or actual[:6] != b"\x7fELF\x01\x01"
                or struct.unpack_from("<HH", actual, 16) != (2, 8)):
            raise MatchError("joint output is not an executable little-endian MIPS ELF32")
        offset = struct.unpack_from("<I", actual, 32)[0]
        width, count, names_index = struct.unpack_from("<HHH", actual, 46)
        if width != 40 or not count or names_index >= count or offset + width * count > len(actual):
            raise MatchError("joint ELF section table is incomplete")
        headers = [struct.unpack_from("<10I", actual, offset + index * width) for index in range(count)]
        names_header = headers[names_index]
        names = actual[names_header[4]:names_header[4] + names_header[5]]
        if len(names) != names_header[5]:
            raise MatchError("joint ELF section names are incomplete")
        allocated = set()
        for header in headers:
            name_end = names.find(b"\0", header[0])
            if name_end < 0:
                raise MatchError("joint ELF section name is invalid")
            name = names[header[0]:name_end].decode("ascii", errors="replace")
            if header[1] in (4, 9) and header[5]:
                raise MatchError("joint ELF retains relocations")
            if not header[5] or not header[2] & 2:
                continue
            if name in allocated:
                raise MatchError("joint ELF has duplicate allocated section names")
            allocated.add(name)
            entry = sections.get(name)
            if entry is None or (entry["address"], entry["size"]) != (header[3], header[5]):
                raise MatchError("joint ELF allocated section metadata disagrees with objdump")
            entry.update({"elf_type": header[1], "elf_flags": header[2]})
            if header[1] == 1 and header[4] + header[5] > len(actual):
                raise MatchError("joint ELF initialized section is incomplete")
        if allocated != set(sections):
            raise MatchError("joint ELF allocated section metadata disagrees with objdump")
    return sections


def verify_shared_sections(
    toolchain: Toolchain, linked: Path, output_section: str, load_address: int, actual: bytes,
) -> list[dict[str, Any]]:
    """Every additional initialized section must agree with its artifact view."""
    views = []
    for name, section in allocated_sections(toolchain, linked).items():
        if name == output_section or not section["initialized"]:
            continue
        size, address = section["size"], section["address"]
        offset = address - load_address
        if offset < 0 or offset + size > len(actual):
            raise MatchError(f"initialized section {name} lies outside {output_section}")
        section_binary = linked.with_name(f"shared-{len(views)}.bin")
        extract_binary_section(toolchain, linked, name, section_binary)
        if section_binary.read_bytes() != actual[offset:offset + size]:
            raise MatchError(f"shared section {name} disagrees with {output_section} at 0x{offset:x}")
        views.append({"section": name, "offset": offset, "size": size})
    return views


def data_ranges(build: dict[str, Any]) -> list[dict[str, Any]]:
    """Normalize the legacy single view at the data module's entry point."""
    if "data_ranges" not in build:
        return [{**build["data_range"], "output_section": build["output_section"]}]
    intervals = build["data_ranges"]
    if "data_range" in build or not isinstance(intervals, list) or not intervals:
        raise MatchError("data target requires one nonempty range list")
    return intervals


def initialized_data_range(
    interval: dict[str, Any],
    section: dict[str, int],
    symbols: dict[str, dict[str, Any]],
    actual: bytes,
) -> tuple[int, bytes]:
    """Select a bounded initialized view only after proving its symbol placement."""
    name = interval["symbol"]
    address = parse_int(interval["symbol_address"])
    offset = parse_int(interval["offset"])
    size = parse_int(interval["size"])
    symbol = symbols.get(name)
    if (
        symbol is None
        or symbol["address"] != address
        or symbol["section"] != interval["output_section"]
    ):
        raise MatchError(f"initialized data symbol {name!r} is missing or misplaced")
    if len(actual) != section["size"]:
        raise MatchError("initialized section extraction is incomplete")
    start = address - section["address"] + offset
    if offset < 0 or size <= 0 or start < 0 or start + size > len(actual):
        raise MatchError("initialized data range lies outside its emitted section")
    return address + offset, actual[start : start + size]


def reject_overlapping_ranges(ranges: Iterable[dict[str, Any]], description: str) -> None:
    spans = sorted((parse_int(item["address"]), parse_int(item["address"]) + item["size"]) for item in ranges)
    if any(left[1] > right[0] for left, right in zip(spans, spans[1:])):
        raise MatchError(f"{description} overlap")


def verify_initialized_owners(
    intervals: list[dict[str, Any]], sections: dict[str, dict[str, Any]],
    symbols: dict[str, dict[str, Any]],
) -> None:
    """Joint resident ranges own whole real objects and complete mutable sections."""
    names = {interval["symbol"] for interval in intervals}
    if len(names) != len(intervals):
        raise MatchError("joint initialized owners are duplicated")
    declared = {interval["output_section"] for interval in intervals}
    for interval in intervals:
        name = interval["symbol"]
        owner = symbols.get(name, {})
        if (parse_int(interval["offset"]) != 0 or parse_int(interval["size"]) <= 0
                or owner.get("type") != "OBJECT" or owner.get("binding") != "GLOBAL"
                or owner.get("visibility") != "DEFAULT" or owner.get("count") != 1
                or owner.get("section") != interval["output_section"]
                or owner.get("address") != parse_int(interval["symbol_address"])
                or owner.get("size") != parse_int(interval["size"])):
            raise MatchError(f"joint initialized owner {name!r} is not one complete GLOBAL DEFAULT OBJECT")
    for name in declared:
        section = sections[name]
        if (section.get("elf_type") != 1 or section.get("elf_flags") != 3
                or not section.get("initialized")
                or not {"CONTENTS", "ALLOC", "LOAD", "DATA"}.issubset(section.get("flags", []))
                or {"READONLY", "CODE"}.intersection(section.get("flags", []))):
            raise MatchError(f"joint initialized section {name!r} is not mutable AW PROGBITS")
        group = sorted((symbols[item["symbol"]] for item in intervals if item["output_section"] == name),
                       key=lambda owner: owner["address"])
        cursor = section["address"]
        for owner in group:
            if owner["address"] != cursor:
                raise MatchError("joint initialized owners overlap or leave a gap")
            cursor += owner["size"]
        if cursor != section["address"] + section["size"]:
            raise MatchError("joint initialized owners do not cover their complete section")
    if {name for name, owner in symbols.items() if owner["section"] in declared and owner["type"] == "OBJECT"} != names:
        raise MatchError("joint initialized section contains an undeclared object or alias")


def compare_initialized_ranges(
    manifest: dict[str, Any], toolchain: Toolchain, build: dict[str, Any], references: Path,
    linked: Path, sections: dict[str, dict[str, Any]], symbols: dict[str, dict[str, Any]],
) -> list[dict[str, Any]]:
    """Compare bounded emitted views in an already linked ELF."""
    intervals = data_ranges(build)
    output = linked.parent
    artifact = artifact_by_id(manifest, build["artifact"])
    load_address = parse_int(artifact["load_address"])
    emitted = {}
    ranges = []
    for index, interval in enumerate(intervals):
        name = interval["output_section"]
        directory = output / "data-ranges" / str(index)
        directory.mkdir(parents=True, exist_ok=True)
        if name not in emitted:
            extracted = directory / "section.bin"
            extract_binary_section(toolchain, linked, name, extracted)
            emitted[name] = extracted.read_bytes()
        address, actual = initialized_data_range(interval, sections[name], symbols, emitted[name])
        candidate = directory / "candidate.bin"
        expected_path = directory / "retail.bin"
        candidate.write_bytes(actual)
        expected_path.write_bytes(reference_bytes(manifest, references, {
            "region": artifact["id"], "address": address, "size": len(actual),
            "symbol": interval["symbol"],
        }))
        comparison = compare_binary_files(
            toolchain, expected_path, candidate, address, directory,
            f"retail/{interval['symbol']}", f"candidate/{interval['symbol']}",
        )
        mismatch = comparison["first_mismatch_offsets"][:1]
        file_offset = address - load_address + parse_int(artifact.get("mapped_file_offset", 0))
        ranges.append({
            **comparison, "name": interval["symbol"], "kind": "data", "size": len(actual),
            "address": f"0x{address:08x}", "placement_delta": 0,
            "section": {"name": name, **sections[name]}, "data_range": interval,
            "candidate": str(candidate.relative_to(ROOT)),
            "first_mismatch_location": mismatch_location(artifact, file_offset + mismatch[0]) if mismatch else None,
        })
    reject_overlapping_ranges(ranges, "initialized data ranges")
    return ranges


def compare_initialized_data(
    manifest: dict[str, Any], toolchain: Toolchain, build: dict[str, Any],
    references: Path, linked: Path, dependencies: Iterable[str],
) -> dict[str, Any]:
    """Verify owned section views without turning intervening gaps into output."""
    artifact = artifact_by_id(manifest, build["artifact"])
    symbols = linked_symbols(toolchain.binutils["objdump"], linked, parse_int(artifact["load_address"]))
    sections = allocated_sections(toolchain, linked)
    declared = {interval["output_section"] for interval in data_ranges(build)}
    if declared != set(sections) or any(not section["initialized"] for section in sections.values()):
        raise MatchError("data target has a missing, unowned or uninitialized allocated section")
    reject_overlapping_ranges(sections.values(), "allocated data sections")
    ranges = compare_initialized_ranges(manifest, toolchain, build, references, linked, sections, symbols)
    size = sum(item["expected_size"] for item in ranges)
    matching = sum(item["matching_bytes"] for item in ranges)
    result = {
        **build_evidence(manifest, toolchain, build, dependencies),
        "artifact": artifact["id"], "ranges": ranges,
        "candidate_size": sum(item["candidate_size"] for item in ranges), "expected_size": size,
        "matching_bytes": matching, "matching_byte_percent": matching * 100 / size,
        "exact": all(item["exact"] for item in ranges),
        "first_mismatch_location": next((item["first_mismatch_location"] for item in ranges if not item["exact"]), None),
    }
    (linked.parent / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def build_artifact(
    manifest: dict[str, Any],
    toolchain: Toolchain,
    build: dict[str, Any],
    references: Path,
) -> dict[str, Any]:
    artifact = artifact_by_id(manifest, build["artifact"])
    verification = verify_references(manifest, references, {artifact["id"]})[0]
    if verification["errors"]:
        raise MatchError(
            f"reference artifact {artifact['id']} failed verification: "
            f"{'; '.join(verification['errors'])}"
        )

    sources = [repository_path(path) for path in source_paths(build)]
    linker_script = repository_path(build["linker_script"])
    output = result_path(manifest, build).parent
    output.mkdir(parents=True, exist_ok=True)
    linked_object = output / f"{artifact['id']}.elf"
    candidate_binary = output / f"{artifact['id']}.bin"

    object_files: list[Path] = []
    dependencies: list[str] = []
    for source in sources:
        object_file, inputs = build_object(
            toolchain, build, source, output / "objects" / source.relative_to(ROOT)
        )
        object_files.append(object_file)
        dependencies.extend(inputs)

    link_command = [
        str(toolchain.binutils["ld"]),
        "-T",
        str(linker_script),
        "-o",
        str(linked_object),
        f"--defsym=__overlay_load_address={artifact['load_address']}",
        *build.get("linker_flags", []),
    ]
    if "overlay_id" in build:
        link_command.append(f"--defsym=__overlay_id={build['overlay_id']}")
    symbol_script = output / "retail-symbols.ld"
    write_symbol_script(SYMBOL_FILE, symbol_script)
    link_command.extend(("-T", str(symbol_script)))
    for symbol, address in build.get("address_aliases", {}).items():
        link_command.append(f"--defsym={symbol}={address}")
    link_command.extend(str(object_file) for object_file in object_files)
    run_checked(link_command)
    if build["kind"] == "data":
        return compare_initialized_data(manifest, toolchain, build, references, linked_object, dependencies)
    extract_binary_section(
        toolchain,
        linked_object,
        build["output_section"],
        candidate_binary,
    )

    actual = candidate_binary.read_bytes()
    load_address = parse_int(artifact["load_address"])
    symbols = linked_symbols(toolchain.binutils["objdump"], linked_object, load_address)
    expected_path = references / artifact["path"]
    expected = expected_path.read_bytes()
    shared_sections = verify_shared_sections(
        toolchain, linked_object, build["output_section"], load_address, actual
    )
    reconstructed = BUILD / "reconstructed" / artifact["path"]
    reconstructed.parent.mkdir(parents=True, exist_ok=True)
    reconstructed.write_bytes(actual)
    ranges = artifact_ranges(build, load_address, symbols)

    comparison = compare_binary_files(
        toolchain,
        expected_path,
        candidate_binary,
        load_address,
        output,
        f"retail/{build['name']}",
        f"candidate/{build['name']}",
    )
    first_mismatch = comparison["first_mismatch_offsets"][:1]
    result = {
        **build_evidence(manifest, toolchain, build, dependencies),
        "artifact": artifact["id"],
        "shared_sections": shared_sections,
        **comparison,
        "first_mismatch_offset": first_mismatch[0] if first_mismatch else None,
        "first_mismatch_location": (
            mismatch_location(artifact, first_mismatch[0])
            if first_mismatch
            else None
        ),
        "ranges": range_comparison(expected, actual, ranges, symbols),
        "candidate": str(candidate_binary.relative_to(ROOT)),
    }
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def compiler_flags(probe: dict[str, Any], optimization: str | None) -> list[str]:
    flags = list(probe["compiler_flags"])
    if optimization is None:
        return flags
    flags = [flag for flag in flags if not re.fullmatch(r"-O(?:[0-3s]|fast)?", flag)]
    flags.insert(0, f"-O{optimization}")
    return flags


def build_probe(
    manifest: dict[str, Any],
    toolchain: Toolchain,
    probe: dict[str, Any],
    aspsx_version: str,
    references: Path,
    optimization: str | None = None,
) -> dict[str, Any]:
    compiler = toolchain.compiler_version
    linker = toolchain.binutils["ld"]

    output = result_path(manifest, {**probe, "aspsx_version": aspsx_version}).parent
    if optimization is not None:
        output = output.with_name(
            f"gcc-{compiler}-O{optimization}-aspsx-{aspsx_version}"
        )
    output.mkdir(parents=True, exist_ok=True)
    source_path = ROOT / probe["source"]
    source_text = probe.get("compile_prelude", "")
    if source_text and not source_text.endswith("\n"):
        source_text += "\n"
    source_text += extract_function(source_path.read_text(), probe["symbol"])
    extracted_source = output / f"{probe['symbol']}.c"
    linked_object = output / f"{probe['symbol']}.elf"
    candidate_binary = output / f"{probe['symbol']}.bin"
    extracted_source.write_text(source_text)

    flags = compiler_flags(probe, optimization)
    object_file, dependencies = build_object(
        toolchain,
        {**probe, "compiler_flags": flags, "aspsx_version": aspsx_version},
        extracted_source,
        output / probe["symbol"],
    )
    binary_input = object_file
    if link := probe.get("link"):
        link_command = [
            linker,
            f"-Ttext={link['text_address']}",
            "-e",
            probe["symbol"],
            "-o",
            str(linked_object),
        ]
        for symbol, address in link.get("symbols", {}).items():
            link_command.append(f"--defsym={symbol}={address}")
        link_command.append(str(object_file))
        run_checked(link_command)
        binary_input = linked_object
    extract_binary_section(toolchain, binary_input, ".text", candidate_binary)

    expected = reference_bytes(manifest, references, probe)
    expected_path = output / "retail.bin"
    expected_path.write_bytes(expected)
    comparison = compare_binary_files(
        toolchain,
        expected_path,
        candidate_binary,
        parse_int(probe["address"]),
        output,
        f"retail/{probe['symbol']}",
        f"candidate/{probe['symbol']}",
    )
    result = {
        **build_evidence(manifest, toolchain, probe, dependencies),
        "symbol": probe["symbol"],
        "source": probe["source"],
        "source_sha256": sha256_file(source_path),
        "extracted_source_sha256": sha256_bytes(source_text.encode()),
        "region": probe["region"],
        "address": probe["address"],
        **comparison,
        "compiler": compiler,
        "compiler_flags": flags,
        "optimization": optimization,
        "aspsx_version": aspsx_version,
        "candidate": str(candidate_binary.relative_to(ROOT)),
    }
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result
