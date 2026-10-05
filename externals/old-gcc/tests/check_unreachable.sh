#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Unreachable smoke outputs: $task_out"

task_body()
{
    awk -v name="$1" '
        $0 == name ":" { body = 1 }
        body { print }
        body && $1 == ".end" && $2 == name { exit }
    ' "$2"
}

for task_level in 0 2; do
    task_lane="$task_out/O$task_level"
    mkdir "$task_lane"
    (cd "$task_lane" && "$task_gcc" -B"$task_prefix/" -S "-O$task_level" \
        -G8 -msoft-float -ffunction-sections -dr -o raw.s "$task_tests/builtin_unreachable.c")
    awk '/^;; Function / { active = ($3 == "unreachable_terminal") } active' \
        "$task_lane/builtin_unreachable.c.rtl" > "$task_lane/terminal.rtl"
    awk '/^;; Function / { active = ($3 == "unreachable_framed") } active' \
        "$task_lane/builtin_unreachable.c.rtl" > "$task_lane/framed.rtl"
    test "$(grep -Ec '^\(barrier ' "$task_lane/terminal.rtl")" -ge 1
    test "$(grep -Ec '^\(barrier ' "$task_lane/framed.rtl")" -ge 2
    task_body unreachable_terminal "$task_lane/raw.s" > "$task_lane/terminal.s"
    task_body unreachable_framed "$task_lane/raw.s" > "$task_lane/framed.s"
    # Prior volatile effects survive; the assertion itself emits no implicit call.
    test "$(grep -Ec '^[[:space:]]*sw[[:space:]]' "$task_lane/terminal.s")" -ge 2
    grep -Eq '^[[:space:]]*(li|addiu|addu)[[:space:]].*,7([[:space:]]|$)' "$task_lane/terminal.s"
    if grep -Eq '^[[:space:]]*(jal|jalr)[[:space:]]' "$task_lane/terminal.s"; then
        exit 1
    fi
    # The unrelated reachable branch retains real local memory and its result.
    grep -Eq '^[[:space:]]*sw[[:space:]]' "$task_lane/framed.s"
    grep -Eq '^[[:space:]]*lw[[:space:]]' "$task_lane/framed.s"
    grep -Eq '^[[:space:]]*xori[[:space:]].*,0x5a5a' "$task_lane/framed.s"
    grep -Eq '^[[:space:]]*j[[:space:]]+\$31' "$task_lane/framed.s"
done
# O0 may serialize dead code: only optimized cleanup is required here.
test "$(grep -Ec '^[[:space:]]*sw[[:space:]]' "$task_out/O2/terminal.s")" -eq 2
if grep -Eq '^[[:space:]]*j[[:space:]]|.*,9([[:space:]]|$)' "$task_out/O2/terminal.s"; then
    exit 1
fi
grep -Eq '\.frame[[:space:]]+\$sp,32,\$31' "$task_out/O2/framed.s"
grep -Eq 'addu[[:space:]]+\$sp,\$sp,32' "$task_out/O2/framed.s"

if "$task_gcc" -B"$task_prefix/" -S -O2 -DTEST_INVALID_ARGS=1 \
    -o "$task_out/invalid.s" "$task_tests/builtin_unreachable.c" \
    > "$task_out/invalid.stdout" 2> "$task_out/invalid.stderr"; then
    exit 1
fi
grep -Fq 'too many arguments to function' "$task_out/invalid.stderr"
if grep -Eq 'internal compiler error|fatal signal' "$task_out/invalid.stderr"; then
    exit 1
fi
