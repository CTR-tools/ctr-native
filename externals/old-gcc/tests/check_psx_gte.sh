#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Compiler smoke outputs: $task_out"

"$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
    -fno-schedule-insns -fno-schedule-insns2 \
    -o "$task_out/gte.s" "$task_tests/psx_gte_transfers.c"
grep -Eq 'mfc2[[:space:]]+\$2,\$14' "$task_out/gte.s"
grep -Eq 'lwc2[[:space:]]+\$0,4\(\$4\)' "$task_out/gte.s"

for task_mode in default off on; do
    set --
    if [ "$task_mode" != default ]; then
        set -- "-mpsx-normal-ram-lhu-delay=$task_mode"
    fi
    "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
        -fno-schedule-insns -fno-schedule-insns2 "$@" \
        -o "$task_out/lhu-$task_mode.s" "$task_tests/psx_normal_ram_lhu_delay.c"
done
cmp "$task_out/lhu-default.s" "$task_out/lhu-off.s"
sed '/^[[:space:]]*#/d' "$task_out/lhu-on.s" \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' \
    | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'
if sed '/^[[:space:]]*#/d' "$task_out/lhu-default.s" \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' \
    | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'; then
    exit 1
fi
