#!/bin/sh
set -eu
task_gcc=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)/$(basename -- "$1")
task_new=$(CDPATH= cd -- "$2" && pwd); task_old=$(CDPATH= cd -- "${3:-$2}" && pwd)
task_here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d "${TMPDIR:-/tmp}/target-first.XXXXXXXX")
echo "Target-first controls: $task_out"
test "$task_old" != "$task_new" || echo 'Old==new: historical compiler identity is not qualified.'
task_hold()
{
    sha256sum "$task_here/check_delay_slot_target_first.sh" "$task_here/delay_slot_target_first.c"
    sha256sum "$task_gcc" "$task_new/cpp" "$task_new/cc1" \
        "$task_old/gcc" "$task_old/cpp" "$task_old/cc1"
}
task_hold > "$task_out/BEFORE.sha256"
trap 'task_status=$?; task_hold > "$task_out/AFTER.sha256"; cmp "$task_out/BEFORE.sha256" "$task_out/AFTER.sha256" || task_status=1; printf "exit=%s\n" "$task_status" > "$task_out/RESULT.txt"; exit "$task_status"' 0
task_compile()
{
    task_lane=$1; task_prefix=$2; shift 2
    task_driver=$task_gcc; test "$task_prefix" = "$task_new" || task_driver=$task_old/gcc
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_driver" -B"$task_prefix/" -S -O2 -G0 \
        -msoft-float -mno-split-addresses -da "$@" "$task_here/delay_slot_target_first.c" -o raw.s) \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr" || { test "${task_warning:-}" = yes;
        grep -q 'not supported' "$task_out/$task_lane/stderr"; }
}
task_body()
{
    awk -v name="$1" '$0 == name ":" { active=1; found=1 }
        active { print } active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$task_out/$2/raw.s"
}
task_instructions()
{
    awk '$1 ~ /^[a-z][a-z0-9]*$/ { gsub(/\$L[0-9]+/,"LABEL"); print $1, $2 }'
}
task_slot()
{
    awk -v expected="$1" -v branch="${2:-beq}" '$1 == ".set" && $2 == "noreorder" { mode=1 }
        $1 == ".set" && $2 == "reorder" { mode=0 }
        $1 == branch { pending=1; ++branches; next }
        $1 ~ /^[a-z][a-z0-9]*$/ {
            if (pending && mode && $1 " " $2 == expected) ++slots
            pending=0
        } END { exit !(branches == 1 && slots == 1) }'
}
task_rtl()
{
    awk -v name="$1" -v expected="$3" '
        /^;; Function / { active=($3 == name) }
        active && (depth || /^\(/) {
            node=node $0; text=$0; depth+=gsub(/\(/,"(",text)
            text=$0; depth-=gsub(/\)/,")",text)
            if (!depth) {
                if (index(node,"(sequence[") && index(node,"{branch_zero}")) {
                    ++count; if (index(node,"(insn/s ") && index(node,"(const_int 47)")) ++target
                }
                node=""
            }
        } END { exit !(count == 1 && target+0 == expected+0) }' "$task_out/$2/delay_slot_target_first.c.dbr"
}
task_positive()
{
    task_a=$1; task_b=$2
    for task_name in Prefer PreferOffset; do
        task_expected='li $4,47'
        test "$task_name" != PreferOffset || task_expected='addu $4,$16,47'
        task_body "$task_name" "$task_b" | task_slot "$task_expected"
        task_body "$task_name" "$task_a" | task_slot 'li $2,23'
        task_rtl "$task_name" "$task_b" 1; task_rtl "$task_name" "$task_a" 0
        task_body "$task_name" "$task_b" | task_instructions | awk '
            $1 == "li" && $2 == "$2,23" { constant=NR }
            $1 == "sw" && $2 == "$2,control_output" { store=NR }
            $1 == "j" && $2 == "LABEL" { ++local }
            END { exit !(constant && store > constant && !local) }'
    done
    for task_lane in "$task_a" "$task_b"; do
        task_body StrongNE "$task_lane" | task_slot 'li $4,47' bne
        task_rtl StrongNE "$task_lane" 1
    done
    for task_name in Live Trap Volatile Shared Strong StrongNE; do
        task_body "$task_name" "$task_a" | task_instructions > "$task_out/expected"
        task_body "$task_name" "$task_b" | task_instructions > "$task_out/actual"
        cmp "$task_out/expected" "$task_out/actual"
    done
}
task_compile default "$task_new"
task_compile off "$task_new" -fno-delay-slot-target-first
task_compile on "$task_new" -fdelay-slot-target-first
task_compile old "$task_old"
cmp "$task_out/old/raw.s" "$task_out/default/raw.s"
cmp "$task_out/default/raw.s" "$task_out/off/raw.s"
task_positive off on
task_body Live on | grep -Fq '$16,control_output'
task_body Trap on | grep -Fq '$4,0($16)'
grep -Fq '(mem/v:SI (symbol_ref:SI ("control_device")))' "$task_out/on/delay_slot_target_first.c.dbr"
task_body Shared off | awk '$1 ~ /^b(eq|ne)$/ { sub(/.*,/,"",$2); ++refs[$2] }
    END { for (label in refs) if (refs[label] == 2) found=1; exit !found }'
grep -q 'NOTE_INSN_LOOP_BEG' "$task_out/on/delay_slot_target_first.c.jump2"
task_compile last-off "$task_new" -fdelay-slot-target-first -fno-delay-slot-target-first
task_compile last-on "$task_new" -fno-delay-slot-target-first -fdelay-slot-target-first
cmp "$task_out/last-off/raw.s" "$task_out/off/raw.s"
cmp "$task_out/last-on/raw.s" "$task_out/on/raw.s"
task_compile disabled-off "$task_new" -fno-delayed-branch -fno-delay-slot-target-first
task_compile disabled-on "$task_new" -fno-delayed-branch -fdelay-slot-target-first
cmp "$task_out/disabled-off/raw.s" "$task_out/disabled-on/raw.s"
task_compile unscheduled-off "$task_new" -fno-schedule-insns -fno-schedule-insns2 -fno-delay-slot-target-first
task_compile unscheduled-on "$task_new" -fno-schedule-insns -fno-schedule-insns2 -fdelay-slot-target-first
task_positive unscheduled-off unscheduled-on
task_compile strong-off "$task_new" -fno-thread-jumps -fno-delay-slot-target-first
task_compile strong-on "$task_new" -fno-thread-jumps -fdelay-slot-target-first
task_positive strong-off strong-on
for task_pair in function:-p arcs:-fprofile-arcs coverage:-ftest-coverage; do
    task_name=${task_pair%:*}; task_flag=${task_pair#*:}
    task_compile "$task_name-off" "$task_new" "$task_flag" -fno-delay-slot-target-first
    task_compile "$task_name-on" "$task_new" "$task_flag" -fdelay-slot-target-first
    # Arc instrumentation embeds the lane-specific .da pathname as .ascii.
    awk '$1 != ".ascii"' "$task_out/$task_name-off/raw.s" > "$task_out/expected"
    awk '$1 != ".ascii"' "$task_out/$task_name-on/raw.s" > "$task_out/actual"
    cmp "$task_out/expected" "$task_out/actual"
done
grep -q 'mcount' "$task_out/function-on/raw.s"
grep -q '__bb_init_func' "$task_out/arcs-on/raw.s"
test -s "$task_out/coverage-on/delay_slot_target_first.bb"
task_warning=yes
for task_pair in blocks:-a trace:-ax; do
    task_name=${task_pair%:*}; task_flag=${task_pair#*:}
    task_compile "$task_name-off" "$task_new" "$task_flag" -fno-delay-slot-target-first
    task_compile "$task_name-on" "$task_new" "$task_flag" -fdelay-slot-target-first
    cmp "$task_out/$task_name-off/stderr" "$task_out/$task_name-on/stderr"
    task_positive "$task_name-off" "$task_name-on"
done
echo 'Target-first controls passed; static output only; real profile-note guard unexercised.'
