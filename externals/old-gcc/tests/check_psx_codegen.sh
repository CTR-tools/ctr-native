#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Code-generation policy smoke outputs: $task_out"
task_phases='rtl jump addressof cse loop cse2 bp flow combine sched lreg greg sched2 jump2 dbr'

task_compile()
{
    task_lane=$1; shift
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_prefix/" -S -O2 -G0 \
        -msoft-float -ffunction-sections -da "$@" -o raw.s "$task_tests/psx_codegen.c") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok()
{
    task_compile "$@"
    test ! -s "$task_out/$task_lane/stdout"; test ! -s "$task_out/$task_lane/stderr"
}
task_body()
{
    awk -v name="$1" '$0 == name ":" { active=1; found=1 }
        active { print }
        active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }' "$2"
}
task_literals()
{
    awk 'BEGIN { RS="" }
        /;; Function / { active=($3 == "LiteralJoin") }
        active && /\{movsi_internal2\}/ &&
            /\(set \(reg[^ :]*:SI [0-9]+ [^)]*\)[[:space:]]*\(const_int 9\)\)/ {
            text=$0; sub(/^.*\(set \(reg[^ :]*:SI /,"",text); sub(/ .*/,"",text)
            if (!(text in seen)) { seen[text]=1; count++ }
        }
        END { if (!count) exit 1; print count }
    ' "$1"
}

task_ok default
task_ok off -fno-schedule-store-load-order -mpreserve-small-si-constants=off
cmp "$task_out/default/raw.s" "$task_out/off/raw.s"
for task_phase in $task_phases; do
    cmp "$task_out/default/psx_codegen.c.$task_phase" "$task_out/off/psx_codegen.c.$task_phase"
done
task_ok memory -fschedule-store-load-order
task_ok literal -mpreserve-small-si-constants=on

# Derive the real store/load identities and retained TRUE edge from this dump.
awk 'BEGIN { RS="" }
    /;; Function / { active=($3 == "independent_bytes") }
    active && /^\(insn / {
        id=$2
        if (/\(set \(mem\/s:QI \(lo_sum:SI/ && /"result_bytes"/) store[FILENAME]=id
        if (/\(zero_extend:SI \(mem\/s:QI/ && /\(const_int 8\)/) {
            record[FILENAME]=$0
            text=$0; sub(/^.*\{zero_extendqisi2\} /,"",text)
            sub(/^.*\(insn_list /,"",text); sub(/ .*/,"",text); base[FILENAME]=text
        }
    }
    END {
        off=ARGV[1]; on=ARGV[2]
        if (!store[on] || !record[on] || !record[off] || !base[off] ||
            base[on] != base[off] ||
            record[on] !~ ("\\(insn_list:REG_DEP_ANTI " store[on] " ") ||
            record[off] ~ ("\\(insn_list:REG_DEP_ANTI " store[off] " ")) exit 1
    }
' "$task_out/off/psx_codegen.c.sched" "$task_out/memory/psx_codegen.c.sched"
for task_name in ordinary_alu possible_alias real_provider call_boundary volatile_boundary copy_words; do
    task_body "$task_name" "$task_out/off/raw.s" > "$task_out/before.s"
    task_body "$task_name" "$task_out/memory/raw.s" > "$task_out/after.s"
    cmp "$task_out/before.s" "$task_out/after.s"
done
task_before=$(task_literals "$task_out/literal/psx_codegen.c.greg")
task_after=$(task_literals "$task_out/literal/psx_codegen.c.dbr")
test "$task_before" -ge 3
test "$task_before" -eq "$task_after"
task_ok last-on -mpreserve-small-si-constants=off -mpreserve-small-si-constants=on
cmp "$task_out/literal/raw.s" "$task_out/last-on/raw.s"
for task_value in 0 32768; do
    for task_mode in off on; do
        task_ok "value-$task_value-$task_mode" "-DSMALL_LITERAL=$task_value" \
            "-mpreserve-small-si-constants=$task_mode"
        task_body LiteralJoin "$task_out/value-$task_value-$task_mode/raw.s" \
            > "$task_out/value-$task_value-$task_mode/body.s"
    done
    cmp "$task_out/value-$task_value-off/body.s" "$task_out/value-$task_value-on/body.s"
done

task_ok last-off -fschedule-store-load-order -fno-schedule-store-load-order \
    -mpreserve-small-si-constants=bad -mpreserve-small-si-constants=off
cmp "$task_out/off/raw.s" "$task_out/last-off/raw.s"
# A disabled first scheduler cannot gain an order edge.
task_ok disabled-off -fno-schedule-insns -fno-schedule-store-load-order
task_ok disabled-on -fno-schedule-insns -fschedule-store-load-order
cmp "$task_out/disabled-off/raw.s" "$task_out/disabled-on/raw.s"
task_ok isa2-off -mips2 -mcpu=r4000 -mpreserve-small-si-constants=off
task_ok isa2-on -mips2 -mcpu=r4000 -mpreserve-small-si-constants=on
cmp "$task_out/isa2-off/raw.s" "$task_out/isa2-on/raw.s"

for task_bad in small memory; do
    task_option=-mpreserve-small-si-constants=bad
    task_message='bad value (bad)'
    if test "$task_bad" = memory; then
        task_option=-fschedule-store-load-order=on
        task_message='Invalid option'
    fi
    if task_compile "bad-$task_bad" "$task_option"; then exit 1; else task_status=$?; fi
    test "$task_status" -eq 1
    grep -Fq "$task_message" "$task_out/bad-$task_bad/stderr"
    if grep -Eiq 'internal compiler error|segmentation fault|fatal signal' \
        "$task_out/bad-$task_bad/stderr"; then exit 1; fi
done
