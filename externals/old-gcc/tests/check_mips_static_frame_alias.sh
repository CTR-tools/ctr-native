#!/bin/sh
set -eu
task_gcc=$1
task_prefix=$2
task_baseline=${3:-$task_prefix}
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Static/frame alias smoke outputs: $task_out"
task_compile()
{
    task_lane=$1; task_bin=$2; shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_bin/" -S -O2 -G0 \
        -msoft-float -da "$@" -o raw.s "$task_tests/mips_static_frame_alias.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok() { task_compile "$@"; test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"; }
task_body() { awk -v n="$1" '$0==n ":" {a=1} a {print} a && $1==".end" && $2==n {exit}' "$task_out/$2/raw.s"; }
# Count actual TRUE dependencies from SP stores to the first byte read.
# Optional bounds isolate reload locals from outgoing/caller/save homes.
task_edges()
{
    awk -v n="$1" -v low="${3:-0}" -v high="${4:-100000}" '
      /;; Function / { active=($3==n) }
      active && /^\(insn/ {
        record=$0; split($0,a," "); uid=a[2]
        while (getline>0 && $0!="") record=record "\n" $0
        if (match(record,/\(set \(mem[^:]*:SI \(plus:SI \(reg:SI 29 sp\)[[:space:]]+\(const_int /)) {
          t=substr(record,RSTART+RLENGTH); sub(/\).*/,"",t); homes[uid]=t+0
        }
        if (!seen && record ~ /\(set \(reg/ && record ~ /\(mem[^:]*:QI/) {
          seen=1
          while (match(record,/\(insn_list [0-9]+/)) {
            t=substr(record,RSTART,RLENGTH); sub(/^.* /,"",t); deps[t]=1
            record=substr(record,RSTART+RLENGTH)
          }
        }
      }
      END { if (!seen) exit 1; for (d in deps) if (d in homes && homes[d]>=low && homes[d]<high) count++; print count+0 }
    ' "$task_out/$2/mips_static_frame_alias.c.sched2"
}
task_ok base "$task_baseline"
task_ok default "$task_prefix"
task_ok off "$task_prefix" -mprologue-static-alias=off
task_ok last-off "$task_prefix" -mprologue-static-alias=invalid -mprologue-static-alias=on -mprologue-static-alias=off
for task_lane in base off last-off; do cmp "$task_out/default/raw.s" "$task_out/$task_lane/raw.s"; done
task_ok on "$task_prefix" -mprologue-static-alias=on
task_ok last-on "$task_prefix" -mprologue-static-alias=off -mprologue-static-alias=on
cmp "$task_out/on/raw.s" "$task_out/last-on/raw.s"
for task_name in static_plain static_alias static_local; do
    test "$(task_edges "$task_name" off)" -gt 0
    test "$(task_edges "$task_name" on)" -eq 0
done
for task_name in static_extent static_unknown static_indirect static_volatile static_exposed static_dynamic static_source; do
    test "$(task_edges "$task_name" off)" -gt 0
    test "$(task_edges "$task_name" off)" -eq "$(task_edges "$task_name" on)"
done
# Concrete private stores precede this read; only their alias edges disappear.
task_body static_reload on | grep -Eq '\.frame[[:space:]]+\$sp,104,\$31.*vars= 40.*args= 24'
test "$(task_edges static_reload off 24 64)" -gt 0
test "$(task_edges static_reload on 24 64)" -eq 0
test "$(task_edges static_source off 24 32)" -eq 2
test "$(task_edges static_source on 24 32)" -eq 2
task_body static_reload on | awk '
  /lbu[[:space:]]/ {read=1}
  !read && /sw[[:space:]]+\$[0-9]+,(24|28|32|36|40)\(\$sp\)/ {private++}
  END {exit !(read && private>=3)}'
# The explicit register spellings are legal section-qualified extern views.
task_ok views-off "$task_prefix" -G8 -DTEST_REGISTER_VIEWS -mprologue-static-alias=off
task_ok views-on "$task_prefix" -G8 -DTEST_REGISTER_VIEWS -mprologue-static-alias=on
cmp "$task_out/views-off/raw.s" "$task_out/views-on/raw.s"
for task_name in static_sp static_gp; do test "$(task_edges "$task_name" views-on)" -gt 0; done
grep -Fq '20($29)' "$task_out/views-on/raw.s"
grep -Fq '20($28)' "$task_out/views-on/raw.s"
for task_name in static_called static_branch static_asm; do
    task_body "$task_name" on | awk -v n="$task_name" '
      /sw[[:space:]]+\$31,.*\(\$sp\)/ {save=NR}
      /jal[[:space:]]+consume/ {if (!call) call=NR}
      /(bne|beq)[[:space:]]/ {branch=NR}
      $1=="#APP" {source_asm=NR}
      /lbu[[:space:]]/ {read=NR}
      END {
        if (n=="static_called") ok=save && call && save<call && call<read
        else if (n=="static_branch") ok=branch && branch<read
        else ok=save && source_asm && save<source_asm && source_asm<read
        exit !(read && ok)}'
done
task_ok mips2 "$task_prefix" -mips2 -mcpu=R4000 -mprologue-static-alias=on
test "$(task_edges static_plain mips2)" -gt 0
if task_compile invalid "$task_prefix" -mprologue-static-alias=invalid; then exit 1; fi
grep -Fq 'bad value (invalid)' "$task_out/invalid/stderr"
if grep -Eiq 'internal compiler error|segmentation fault|fatal signal' "$task_out/invalid/stderr"; then exit 1; fi
echo 'Static/frame controls passed: default/OFF, private reload, static extents, register views, contexts and options.'
