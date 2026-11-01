#!/bin/sh
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Loop-optimization smoke outputs: $task_out"
task_compile()
{
    task_lane=$1; task_bin=$2; task_level=$3; shift 3
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S "-O$task_level" \
        -G0 -msoft-float -da "$@" -o raw.s "$task_tests/loop_optimize.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"
}
task_equal()
{
    for task_file in raw.s loop_optimize.c.*; do
        cmp "$task_out/$1/$task_file" "$task_out/$2/$task_file"
    done
}
for task_level in 0 2; do
    task_compile "base$task_level" "$task_baseline" "$task_level"
    task_compile "default$task_level" "$task_prefix" "$task_level"
    task_compile "on$task_level" "$task_prefix" "$task_level" -floop-optimize
    task_compile "off$task_level" "$task_prefix" "$task_level" -fno-loop-optimize
    task_compile "last-on$task_level" "$task_prefix" "$task_level" -fno-loop-optimize -floop-optimize
    task_compile "last-off$task_level" "$task_prefix" "$task_level" -floop-optimize -fno-loop-optimize
    (cd "$task_out/default$task_level" && task_equal "base$task_level" "default$task_level")
    (cd "$task_out/default$task_level" && task_equal "default$task_level" "on$task_level")
    (cd "$task_out/default$task_level" && task_equal "on$task_level" "last-on$task_level")
    (cd "$task_out/default$task_level" && task_equal "off$task_level" "last-off$task_level")
    cmp "$task_out/off$task_level/loop_optimize.c.addressof" "$task_out/off$task_level/loop_optimize.c.loop"
done
(cd "$task_out/default0" && task_equal default0 off0)
if cmp -s "$task_out/on2/raw.s" "$task_out/off2/raw.s"; then exit 1; fi
# Inspect the real loop pass: product hoisted ON, retained inside the loop OFF.
for task_mode in on off; do
    awk -v mode="$task_mode" '
        /;; Function / { active=($3 == "loop_invariant") }
        active && /NOTE_INSN_LOOP_BEG/ { inside=1; begin++ }
        active && /NOTE_INSN_LOOP_END/ { inside=0; end++ }
        active && /\(set \(reg:SI/ { set=1; next }
        active && set { if (/\(mult:SI/) { count++; where=inside } set=0 }
        END { exit !(begin==1 && end==1 && count==1 && where==(mode=="off")) }
    ' "$task_out/$task_mode"2/loop_optimize.c.loop
    # Both paths retain the volatile read/write and the real call in the loop.
    awk '
        /;; Function / { active=($3 == "loop_effects") }
        active && /NOTE_INSN_LOOP_BEG/ { inside=1; begin++ }
        active && /NOTE_INSN_LOOP_END/ { inside=0; end++ }
        active && /mem\/v:SI/ { volatile++; if (!inside) bad=1 }
        active && /^\(call_insn / { calls++; if (!inside) bad=1 }
        END { exit (bad || !(begin==1 && end==1 && volatile==2 && calls==1)) }
    ' "$task_out/$task_mode"2/loop_optimize.c.loop
    for task_phase in rtl cse2 flow combine sched greg sched2 dbr; do
        test -s "$task_out/$task_mode"2/loop_optimize.c."$task_phase"
    done
done
echo 'Loop-optimization controls passed (O0/O2; default/ON; precedence; RTL effects).'
