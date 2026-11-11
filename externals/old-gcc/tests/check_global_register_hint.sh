#!/bin/sh
# Finite GLOBAL allocation controls; not execution or a fixed-register ABI.
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "GLOBAL register-hint smoke outputs: $task_out"
task_compile()
{
    task_lane=$1; task_bin=$2; task_source=$3; shift 3
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G8 \
        -msoft-float -ffunction-sections -dgl "$@" -o raw.s "$task_tests/$task_source") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr"
}
task_compile base "$task_baseline" global_register_hint.c '-DGH(name)='
task_compile off "$task_prefix" global_register_hint.c '-DGH(name)='
task_compile on "$task_prefix" global_register_hint.c
cmp "$task_out/base/raw.s" "$task_out/off/raw.s"
for task_case in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24; do
    if "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float \
        -DCASE="$task_case" -o "$task_out/invalid-$task_case.s" \
        "$task_tests/global_register_hint_invalid.c" \
        > "$task_out/invalid-$task_case.stdout" 2> "$task_out/invalid-$task_case.stderr"; then
        exit 1
    else task_status=$?; fi
    test "$task_status" -eq 1
    grep -q global_register_hint "$task_out/invalid-$task_case.stderr"
    ! grep -Eiq 'internal compiler error|segmentation fault|fatal signal|aborted' "$task_out/invalid-$task_case.stderr"
done
task_compile class-base "$task_baseline" global_register_hint_class.c '-DGH(name)=' -mt9-constraint=on
task_compile class-off "$task_prefix" global_register_hint_class.c '-DGH(name)=' -mt9-constraint=on
task_compile class-on "$task_prefix" global_register_hint_class.c -mt9-constraint=on
cmp "$task_out/class-base/raw.s" "$task_out/class-off/raw.s"
for task_mode in off on; do
    mkdir "$task_out/o0-$task_mode"
    if test "$task_mode" = off; then
        (cd "$task_out/o0-$task_mode" && "$task_gcc" -B"$task_prefix/" -S -O0 -G8 \
            -msoft-float '-DGH(name)=' -o raw.s "$task_tests/global_register_hint.c") \
            > "$task_out/o0-$task_mode/stdout" 2> "$task_out/o0-$task_mode/stderr"
    else
        (cd "$task_out/o0-$task_mode" && "$task_gcc" -B"$task_prefix/" -S -O0 -G8 \
            -msoft-float -o raw.s "$task_tests/global_register_hint.c") \
            > "$task_out/o0-$task_mode/stdout" 2> "$task_out/o0-$task_mode/stderr"
    fi
    test ! -s "$task_out/o0-$task_mode/stdout"
    test ! -s "$task_out/o0-$task_mode/stderr"
done
cmp "$task_out/o0-off/raw.s" "$task_out/o0-on/raw.s"
python3 - "$task_out" <<'PY'
from pathlib import Path
import re
import sys
out = Path(sys.argv[1])
def functions(text):
    parts = re.split(r'^;; Function ([^\s]+)\s*$', text, flags=re.M)
    return {parts[i]: parts[i+1] for i in range(1, len(parts), 2)}
def graph(text):
    hints = [tuple(map(int, row)) for row in re.findall(
        r'^;; Global hint pseudo (\d+) allocno (\d+) register (\d+)\.$', text, re.M)]
    chosen = [tuple(map(int, row)) for row in re.findall(
        r'^;; Global hint allocno (\d+) chose (\d+)\.$', text, re.M)]
    unavailable = [tuple(map(int, row)) for row in re.findall(
        r'^;; Global hint allocno (\d+) unavailable (\d+)\.$', text, re.M)]
    body = text.split(';; Register dispositions:', 1)
    homes = {int(p): int(r) for p, r in re.findall(r'\b(\d+) in (\d+)\b', body[-1])}
    conflicts = {int(p): {int(q) for q in values.split()} for p, values in re.findall(
        r'^;; (\d+) conflicts:([^\n]*)$', text, re.M)}
    match = re.search(r'^;; \d+ regs to allocate:([^\n]*)$', text, re.M)
    order = [[int(p) for p in item.split('+')] for item in (match[1].split() if match else [])
             if re.fullmatch(r'\d+(?:\+\d+)*', item)]
    return hints, chosen, unavailable, homes, conflicts, order
def body(text, name):
    match = re.search(r'^' + re.escape(name) + r':\n(.*?)^\s*\.end\s+' + re.escape(name) + r'\s*$', text, re.M | re.S)
    assert match, name
    labels = {}
    return re.sub(r'(?<![\w])\$?L\d+(?![\w])',
                  lambda m: labels.setdefault(m[0], 'LOCAL_LABEL_' + str(len(labels))), match[1])
dumps = functions((out / 'on/global_register_hint.c.greg').read_text())
on = (out / 'on/raw.s').read_text()
off = (out / 'off/raw.s').read_text()
for name in ('plain_before', 'plain_after', 'zero_globals', 'local_only'):
    assert not graph(dumps[name])[0], name
    assert body(on, name) == body(off, name), name
for name, wanted in [('preferred_roles', {17, 21}), ('nested_owners', {21, 22}),
                     ('pointer_owner', {23}), ('narrow_owner', {22})]:
    hints, chosen, unavailable, homes, conflicts, order = graph(dumps[name])
    assert {r for p, a, r in hints} == wanted, name
    assert all(homes.get(p) == r and (a, r) in chosen for p, a, r in hints), name
    if name == 'preferred_roles':
        positions = [next(i for i, group in enumerate(order) if p in group) for p, a, r in hints]
        assert sorted(set(positions)) == list(range(len(set(positions)))), name
inline = graph(dumps['inline_twice'])[0]
assert len(inline) == 2 and len({p for p, a, r in inline}) == 2
nested = [(fn, h) for fn, text in dumps.items() for h in graph(text)[0] if h[2] == 20]
assert nested and all(fn == 'gnu_nested' or fn.startswith('inner') for fn, h in nested)
for name, color in [('local_conflict', 21), ('unavailable_callcolor', 2)]:
    hints, chosen, unavailable, homes, conflicts, order = graph(dumps[name])
    assert len(hints) == 1 and hints[0][2] == color, name
    p, a, r = hints[0]
    assert (a, r) in unavailable and homes.get(p) not in (None, r), name
    assert body(on, name) == body(off, name), name
hints, chosen, unavailable, homes, conflicts, order = graph(dumps['legal_caller_color'])
assert len(hints) == 1 and hints[0][2] == 8
assert all(homes.get(p) == r and (a, r) in chosen for p, a, r in hints)
hints, chosen, unavailable, homes, conflicts, order = graph(dumps['live_color_conflict'])
assert len(hints) == 2 and hints[0][1] != hints[1][1] and all(r == 21 for p, a, r in hints)
assert hints[1][0] in conflicts[hints[0][0]]
allocated = [homes.get(p) for p, a, r in hints]
assert None not in allocated and allocated.count(21) == 1 and len(set(allocated)) == 2
class_dump = functions((out / 'class-on/global_register_hint_class.c.greg').read_text())
hints, chosen, unavailable, homes, conflicts, order = graph(class_dump['restricted_class'])
assert len(hints) == 1 and hints[0][2] == 21
p, a, r = hints[0]
assert homes.get(p) == 25 and (a, r) in unavailable
assert body((out / 'class-on/raw.s').read_text(), 'restricted_class') == body((out / 'class-off/raw.s').read_text(), 'restricted_class')
PY
echo 'GLOBAL controls passed: OFF identity, word/HI/pointer/nested/inline owners, legal/fallback colors, class/call exclusions, O0, invalid forms.'
