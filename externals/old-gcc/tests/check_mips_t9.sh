#!/bin/sh
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "T9 operand smoke outputs: $task_out"
task_compile()
{
    task_lane=$1; task_bin=$2; shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G16 \
        -msoft-float -da "$@" -o raw.s "$task_tests/mips_t9.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok() { task_compile "$@"; test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"; }
task_bad()
{
    task_message=$1; shift
    if task_compile "$@"; then exit 1; else task_status=$?; fi
    test "$task_status" -eq 1
    grep -Fq "$task_message" "$task_out/$task_lane/stderr"
    ! grep -Eiq 'internal compiler error|segmentation fault|fatal signal' "$task_out/$task_lane/stderr"
}
task_body() { awk -v name="$1" '$0==name ":" {active=1; found=1} active {print} active && $1==".end" && $2==name {done=1; exit} END {exit !(found && done)}' "$2"; }
task_ok baseline "$task_baseline"
task_ok default "$task_prefix"
task_ok off "$task_prefix" -mt9-constraint=off
task_ok invalid-off "$task_prefix" -mt9-constraint=invalid -mt9-constraint=off
task_ok last-off "$task_prefix" -mt9-constraint=on -mt9-constraint=off
for task_lane in baseline off invalid-off last-off; do cmp "$task_out/default/raw.s" "$task_out/$task_lane/raw.s"; done
task_ok on-ordinary "$task_prefix" -mt9-constraint=on
task_ok on "$task_prefix" -mt9-constraint=on -DENABLE_T9
task_ok last-on "$task_prefix" -mt9-constraint=off -mt9-constraint=on -DENABLE_T9
cmp "$task_out/on/raw.s" "$task_out/last-on/raw.s"
task_bad 'inconsistent operand constraints' default-c "$task_prefix" -DENABLE_T9
task_bad 'inconsistent operand constraints' off-c "$task_prefix" -mt9-constraint=off -DENABLE_T9
task_bad 'bad value (invalid)' invalid "$task_prefix" -mt9-constraint=invalid
task_bad 'bad value (invalid)' last-invalid "$task_prefix" -mt9-constraint=on -mt9-constraint=invalid
task_bad 'impossible register constraint' wide "$task_prefix" -mt9-constraint=on -DENABLE_T9 -DNEGATIVE=1
task_bad 'fixed or forbidden register' clobber "$task_prefix" -mt9-constraint=on -DENABLE_T9 -DNEGATIVE=2
task_bad 'too many reloads' two "$task_prefix" -mt9-constraint=on -DENABLE_T9 -DNEGATIVE=3
task_bad 'fixed or forbidden register' fixed-t9 "$task_prefix" -mt9-constraint=on -DENABLE_T9 -ffixed-25
task_ok call-used "$task_prefix" -mt9-constraint=on -DENABLE_T9 -fcall-used-16
task_body requested_s0 "$task_out/on/raw.s" > "$task_out/default-s0.s"
task_body requested_s0 "$task_out/call-used/raw.s" > "$task_out/call-used-s0.s"
grep -Eq 'sw[[:space:]]+\$16,' "$task_out/default-s0.s"
! grep -Eq '(sw|lw)[[:space:]]+\$16,' "$task_out/call-used-s0.s"
# The pressured ordinary pseudo is really saved/reloaded across the real call.
task_body pressure "$task_out/on/raw.s" > "$task_out/pressure.s"
awk '/sw[[:space:]]+\$25,/ {save=1} /jal[[:space:]]+sink/ {if (!save) bad=1; call=1} /lw[[:space:]]+\$25,/ {if (!call) bad=1; reload=1} END {exit (bad || !(save && call && reload))}' "$task_out/pressure.s"
task_body effects "$task_out/on/raw.s" > "$task_out/effects.s"
awk '/sw[[:space:]].*t9_state/ {if (call) bad=1; store=1} /jal[[:space:]]+sink/ {if (!store) bad=1; call=1} /lw[[:space:]].*t9_state/ {if (!call) bad=1; load=1} END {exit (bad || !(store && call && load))}' "$task_out/effects.s"
grep -Eq '\(reg[^:]*:SI 25 t9\)' "$task_out/on/mips_t9.c.greg"
echo 'T9 controls passed: default/OFF, precedence, operand refusals, usage flags, real calls/effects.'
