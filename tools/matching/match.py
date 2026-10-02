#!/usr/bin/env python3
"""Build, compare, and inspect configured CTR matching targets."""

from __future__ import annotations

import argparse
import json
import sys
import subprocess
from pathlib import Path
from typing import Any, Iterable

from pipeline import (
    BUILD,
    DEFAULT_MANIFEST,
    MatchError,
    Toolchain,
    build_artifact,
    build_probe,
    compare_artifacts,
    configured_targets,
    load_json,
    probe_by_symbol,
    reference_root,
    resolve_toolchain,
    result_is_current,
    result_path,
    target_by_name,
    verify_references,
    repository_path,
)
from resident import (
    build_resident,
    discover_function_ranges,
    parse_symbols,
    selected_functions,
)


def print_verification(results: Iterable[dict[str, Any]]) -> bool:
    exact = True
    for result in results:
        if result["errors"]:
            exact = False
            print(f"FAIL    {result['id']}: {'; '.join(result['errors'])}")
        else:
            print(
                f"OK      {result['id']}: {result['actual_size']} bytes "
                f"{result['actual_sha256'][:12]}"
            )
    return exact


def print_comparison(results: Iterable[dict[str, Any]]) -> bool:
    exact = True
    for result in results:
        if result["exact"]:
            print(
                f"MATCH   {result['id']}: {result['candidate_size']} bytes "
                f"{result['candidate_sha256'][:12]}"
            )
        else:
            exact = False
            detail = result.get("error")
            if detail is None:
                detail = (
                    "first difference at "
                    f"{result.get('first_difference_location', 'unknown')}; "
                    f"candidate size {result['candidate_size']}, "
                    f"expected {result['expected_size']}"
                )
            print(f"DIFF    {result['id']}: {detail}")
    return exact


def parse_selected(
    manifest: dict[str, Any], values: list[str] | None
) -> set[str] | None:
    if not values:
        return None
    selected = set(values)
    known = {artifact["id"] for artifact in manifest["artifacts"]}
    if unknown := selected - known:
        raise MatchError(f"unknown artifact(s): {', '.join(sorted(unknown))}")
    return selected


def print_toolchain(toolchain: Toolchain) -> None:
    print(
        f"OK      GCC {toolchain.compiler_banner}: "
        f"{toolchain.compiler_hashes['gcc'][:12]}"
    )
    print(
        f"OK      maspsx {toolchain.maspsx_commit[:12]}: "
        f"{toolchain.maspsx_hashes['maspsx.py'][:12]}"
    )
    print(
        f"OK      {toolchain.assembler_banner}: {toolchain.binutils_hashes['as'][:12]}"
    )


def cmd_toolchain(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    print_toolchain(resolve_toolchain(manifest))
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    results = verify_references(
        manifest,
        reference_root(manifest, args.reference_root),
        parse_selected(manifest, args.artifact),
    )
    return 0 if print_verification(results) else 1


def cmd_compare(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    results = compare_artifacts(
        manifest,
        reference_root(manifest, args.reference_root),
        Path(args.candidate_root).expanduser().resolve(),
        parse_selected(manifest, args.artifact),
    )
    if args.report:
        report = Path(args.report)
        report.parent.mkdir(parents=True, exist_ok=True)
        report.write_text(json.dumps(results, indent=2) + "\n")
    return 0 if print_comparison(results) else 1


def print_artifact_result(result: dict[str, Any]) -> bool:
    status = "MATCH" if result["exact"] else "DIFF"
    print(
        f"{status:<7} {result['name']} ({result['scope']}): "
        f"{result['candidate_size']}/{result['expected_size']} bytes; "
        f"{result['matching_bytes']} bytes equal at the same offsets "
        f"({result['matching_byte_percent']:.2f}%)"
    )
    if not result["exact"]:
        print(f"        first difference: {result['first_mismatch_location']}")
    for item in result["ranges"]:
        detail = f"{item['matching_bytes']}/{item['size']} bytes"
        if item["kind"] == "code":
            detail += f", {item['matching_words']}/{item['total_words']} words"
        if item["placement_delta"]:
            detail += f", placement {item['placement_delta']:+#x}"
        print(f"        {item['name']}: {detail}")
        if not item["exact"] and item.get("objdump_diff"):
            print(f"        objdump diff: {item['objdump_diff']}")
    if result.get("objdump_diff"):
        print(f"        objdump diff: {result['objdump_diff']}")
    return bool(result["exact"])


def build_target(
    manifest: dict[str, Any],
    toolchain: Toolchain,
    config: dict[str, Any],
    references: Path,
    functions: list[str] | None = None,
) -> dict[str, Any]:
    if functions and config["kind"] != "resident":
        raise MatchError(
            "--function selects resident functions; other targets build as a whole"
        )
    if config["kind"] == "resident":
        result = build_resident(manifest, toolchain, config, references, functions)
        print(
            f"{config['name']}: {result['exact_function_count']}/{result['selected_function_count']} function ranges exact"
        )
    elif config["kind"] in ("artifact", "data"):
        result = build_artifact(manifest, toolchain, config, references)
        print_artifact_result(result)
    else:
        result = build_probe(
            manifest,
            toolchain,
            config,
            config.get("aspsx_version", toolchain.default_aspsx_version),
            references,
        )
        status = "MATCH" if result["exact"] else "DIFF"
        print(
            f"{status:<7} {config['name']}: extracted function, {result['candidate_size']}/{result['expected_size']} bytes"
        )
    return result


def cmd_check(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    targets = (
        [target_by_name(manifest, name) for name in args.targets]
        if args.targets
        else configured_targets(manifest)
    )
    toolchain = resolve_toolchain(manifest)
    print_toolchain(toolchain)
    references = reference_root(manifest, args.reference_root)
    if not print_verification(verify_references(manifest, references)):
        return 1
    exact = 0
    for config in targets:
        try:
            result = build_target(manifest, toolchain, config, references)
            exact += int(result["exact"])
        except MatchError as exc:
            print(f"ERROR   {config['name']}: {exc}")
    print(f"{exact}/{len(targets)} configured targets matched at their declared scope")
    return 0 if exact == len(targets) else 1


def cmd_build(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    config = target_by_name(manifest, args.target)
    if args.inventory_only:
        if config["kind"] != "resident":
            raise MatchError("--inventory-only requires a resident target")
        functions = discover_function_ranges(
            config, parse_symbols(repository_path(config["symbol_file"]))
        )
        for function in selected_functions(functions, args.function):
            print(
                f"{function.name:<48} 0x{function.address:08x} 0x{function.size:x} {function.source}"
            )
        return 0
    toolchain = resolve_toolchain(manifest)
    print_toolchain(toolchain)
    references = reference_root(manifest, args.reference_root)
    artifact = config.get("artifact", config.get("region"))
    if not print_verification(verify_references(manifest, references, {artifact})):
        return 1
    result = build_target(manifest, toolchain, config, references, args.function)
    return 0 if result.get("selected_exact", result["exact"]) else 1


def cmd_probe(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    toolchain = resolve_toolchain(manifest)
    probe = probe_by_symbol(manifest, args.symbol)
    aspsx_version = args.aspsx_version or toolchain.default_aspsx_version
    result = build_probe(
        manifest,
        toolchain,
        probe,
        aspsx_version,
        reference_root(manifest, args.reference_root),
        args.optimization,
    )
    status = "MATCH" if result["exact"] else "DIFF"
    print(
        f"{status:<7} {result['symbol']} {result['address']}: "
        f"{result['candidate_size']}/{result['expected_size']} bytes; "
        f"GCC {result['compiler']}, ASPSX {result['aspsx_version']}"
    )
    if not result["exact"] and result["first_mismatch_offsets"]:
        offsets = ", ".join(
            f"0x{value:x}" for value in result["first_mismatch_offsets"]
        )
        print(f"        first mismatch offsets: {offsets}")
    return 0 if result["exact"] else 1


def cmd_matrix(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    toolchain = resolve_toolchain(manifest)
    probe = probe_by_symbol(manifest, args.symbol)
    compiler = toolchain.compiler_version
    references = reference_root(manifest, args.reference_root)

    matrix_results: list[dict[str, Any]] = []
    matches = 0
    total = 0
    optimizations: list[str | None]
    if args.all_optimizations:
        optimizations = ["0", "1", "2", "3"]
    elif args.optimization:
        optimizations = args.optimization
    else:
        optimizations = [None]

    for aspsx_version in probe["aspsx_versions"]:
        for optimization in optimizations:
            total += 1
            try:
                result = build_probe(
                    manifest,
                    toolchain,
                    probe,
                    aspsx_version,
                    references,
                    optimization,
                )
            except MatchError as exc:
                result = {
                    "compiler": compiler,
                    "aspsx_version": aspsx_version,
                    "optimization": optimization,
                    "exact": False,
                    "error": str(exc),
                }
                matrix_results.append(result)
                opt_label = (
                    f"O{optimization}" if optimization is not None else "default"
                )
                print(
                    f"ERROR   GCC {compiler:<7} {opt_label:<7} "
                    f"ASPSX {aspsx_version}: compile failed"
                )
                continue
            matrix_results.append(result)
            matches += int(result["exact"])
            status = "MATCH" if result["exact"] else "DIFF"
            opt_label = f"O{optimization}" if optimization is not None else "default"
            print(f"{status:<7} GCC {compiler:<7} {opt_label:<7} ASPSX {aspsx_version}")
    matrix_report = BUILD / "matrices" / f"{probe['symbol']}.json"
    matrix_report.parent.mkdir(parents=True, exist_ok=True)
    matrix_report.write_text(json.dumps(matrix_results, indent=2) + "\n")
    print(f"{matches}/{total} matrix entries matched")
    return 0


def cmd_status(args: argparse.Namespace) -> int:
    manifest = load_json(Path(args.manifest))
    print(f"target: {manifest['target']}")
    for config in configured_targets(manifest):
        path = result_path(manifest, config)
        try:
            result = load_json(path)
            current = result_is_current(manifest, config, result)
        except MatchError:
            current = False
        if not current:
            status = "STALE" if path.is_file() else "MISSING"
            detail = "rebuild required"
        elif config["kind"] == "resident" and not result["complete"]:
            status = "PARTIAL"
            detail = f"{result['exact_function_count']}/{result['function_count']} function ranges proven"
        else:
            status = "MATCH" if result["exact"] else "DIFF"
            detail = result["scope"]
            if config["kind"] == "resident":
                detail += (
                    f" ({result['exact_function_count']}/{result['function_count']})"
                )
        print(f"{status:<7} {config['name']:<32} {detail}")
    print(
        "Function ranges do not prove a complete EXE; complete artifacts do not prove a reconstructed disc."
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="CTR byte-matching pipeline; exact bytes are the acceptance oracle."
    )
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST))
    subparsers = parser.add_subparsers(dest="command", required=True)

    toolchain = subparsers.add_parser(
        "toolchain", help="verify the vendored matching tools"
    )
    toolchain.set_defaults(func=cmd_toolchain)

    verify = subparsers.add_parser("verify", help="verify the retail oracle")
    verify.add_argument("--reference-root")
    verify.add_argument("--artifact", action="append")
    verify.set_defaults(func=cmd_verify)

    compare = subparsers.add_parser(
        "compare", help="compare reconstructed artifacts with retail"
    )
    compare.add_argument("candidate_root")
    compare.add_argument("--reference-root")
    compare.add_argument("--artifact", action="append")
    compare.add_argument("--report")
    compare.set_defaults(func=cmd_compare)

    check = subparsers.add_parser(
        "check", help="verify the toolchain, retail inputs, and source corpus"
    )
    check.add_argument(
        "targets",
        nargs="*",
        help="optional target names; default: all configured targets",
    )
    check.add_argument("--reference-root")
    check.set_defaults(func=cmd_check)

    probe = subparsers.add_parser(
        "probe", help="compile one source probe and compare its retail bytes"
    )
    probe.add_argument("symbol")
    probe.add_argument("--aspsx-version")
    probe.add_argument("--reference-root")
    probe.add_argument("--optimization", choices=("0", "1", "2", "3"))
    probe.set_defaults(func=cmd_probe)

    build = subparsers.add_parser(
        "build",
        aliases=["artifact", "namespace"],
        help="build and compare a configured target",
    )
    build.add_argument("target")
    build.add_argument("--reference-root")
    build.add_argument("--function", action="append")
    build.add_argument("--inventory-only", action="store_true")
    build.set_defaults(func=cmd_build)

    matrix = subparsers.add_parser(
        "matrix", help="run an ASPSX/optimization matrix with vendored GCC"
    )
    matrix.add_argument("symbol")
    matrix.add_argument("--reference-root")
    matrix.add_argument("--optimization", action="append", choices=("0", "1", "2", "3"))
    matrix.add_argument("--all-optimizations", action="store_true")
    matrix.set_defaults(func=cmd_matrix)

    status = subparsers.add_parser(
        "status", help="show evidence without upgrading unproven progress"
    )
    status.set_defaults(func=cmd_status)

    return parser


def main() -> int:
    try:
        args = build_parser().parse_args()
        return args.func(args)
    except MatchError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"ERROR: command failed with exit code {exc.returncode}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
