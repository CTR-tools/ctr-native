#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Scratch/call/load policy smoke outputs: $task_out"

task_compile()
{
    task_name=$1
    task_source=$2
    shift 2
    (cd "$task_out" && "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float \
        -ffunction-sections -da "$@" -o "$task_out/$task_name.s" "$task_tests/$task_source")
}
task_body()
{
    awk -v name="$1" '
        $0 == name ":" { body=1; found=1 }
        body { print }
        body && ($1 == ".end" || $1 == ".size") && $2 ~ ("^" name "(,|$)") { done=1; exit }
        END { exit !(found && done) }
    ' "$task_out/$2.s"
}
task_local_labels()
{
    # Only defined function-local numeric labels may be renamed, bijectively.
    awk '
        { lines[NR]=$0 }
        /^[ \t]*\$L[0-9]+:$/ {
            label=$1; sub(/:$/, "", label)
            if (label in labels) bad=1
            ++count; labels[label]="$L" count
        }
        END {
            if (bad) exit 1
            for (i=1; i<=NR; ++i) {
                rest=lines[i]; result=""
                while (match(rest, /\$L[0-9]+/)) {
                    head=substr(rest, 1, RSTART-1); label=substr(rest, RSTART, RLENGTH)
                    rest=substr(rest, RSTART+RLENGTH)
                    if (!(label in labels) || substr(head, length(head), 1) ~ /[[:alnum:]_$]/ ||
                        substr(rest, 1, 1) ~ /[[:alnum:]_$]/) exit 1
                    result=result head labels[label]
                }
                print result rest
            }
        }
    ' "$1"
}
task_reject()
{
    task_name=$1
    task_diagnostic=$2
    shift 2
    if task_compile "$task_name" "$@" > "$task_out/$task_name.stdout" 2> "$task_out/$task_name.stderr"; then
        exit 1
    fi
    grep -Fq "$task_diagnostic" "$task_out/$task_name.stderr"
    if grep -Eiq 'internal compiler error|internal error|segmentation fault' "$task_out/$task_name.stderr"; then exit 1; fi
}

# Literal/folded masks and equivalent redeclarations must create actual calls.
for task_case in 1 2 3 4 5 6 7 8 9; do
    task_compile "mask-$task_case" psx_regcall.c "-DCASE=$task_case"
    grep -Eq '\.frame[[:space:]]+\$sp,16,\$31.*args= 0' "$task_out/mask-$task_case.s"
    grep -Fq '0x80070000,-4' "$task_out/mask-$task_case.s"
    grep -Eq 'jal[[:space:]]+ParserLeaf' "$task_out/mask-$task_case.s"
    for task_reg in 31 16 17 18; do
        grep -Eq '\(clobber \(reg:SI '"$task_reg"' ' "$task_out/psx_regcall.c.rtl"
    done
    task_actual=$(sed -n 's/.*(use (reg:SI \([0-9][0-9]*\).*/\1/p' \
        "$task_out/psx_regcall.c.rtl" | awk '$1 >= 2 && $1 <= 25' | sort -nu | tr '\n' ' ')
    case "$task_case" in
        7|8) task_expected='' ;;
        9) task_expected='2 3 8 9 14 15 24 25 ' ;;
        *) task_expected='14 ' ;;
    esac
    test "$task_actual" = "$task_expected"
done
task_reject mask-conflict 'mips_regcall_s012 must be repeated identically' psx_regcall.c -DCASE=20
for task_case in 21 22 23 24 25 26 27 28 29 30 31 32 33; do
    task_reject "mask-bad-$task_case" 'mips_regcall_s012 requires non-inline C void(void)' \
        psx_regcall.c "-DCASE=$task_case"
done
task_reject call-address 'require direct calls, not addresses' psx_regcall.c -DCASE=1 -DTEST_ADDRESS
task_reject call-redecl 'must be repeated identically' psx_regcall.c -DCASE=1 -DTEST_MISSING_REDECL
task_reject call-target 'requires non-inline C void(void)' psx_regcall.c -DCASE=1 -mcpu=r4000 -mips2

# The body marker never changes ordinary callers into zero-home register calls.
task_compile scratch psx_scratch.c
for task_name in ScratchCallerBefore ScratchCallerAfter; do
    task_body "$task_name" scratch | grep -Eq '\.frame.*args= 16'
    task_body "$task_name" scratch | grep -Eq 'jal[[:space:]]+ScratchRelay'
done
task_body ScratchRelay scratch > "$task_out/manual.s"
grep -Eq 'jal[[:space:]]+ScratchProvider' "$task_out/manual.s"
grep -Eq '\(use \(reg:SI 1 ' "$task_out/psx_scratch.c.rtl"
if grep -Eq '^[[:space:]]*\.(ent|frame|mask|fmask)|\$(sp|fp)' "$task_out/manual.s"; then exit 1; fi
for task_offset in 0 4 8 12; do
    grep -Eq 'sw[[:space:]]+\$[0-9]+,'"$task_offset"'\(\$1\)' "$task_out/manual.s"
    grep -Eq 'lw[[:space:]]+\$[0-9]+,'"$task_offset"'\(\$1\)' "$task_out/manual.s"
done
grep -Eq 'j[[:space:]]+\$31' "$task_out/manual.s"
for task_mode in O3 automatic; do
    if test "$task_mode" = O3; then set -- -O3; else set -- -finline-functions; fi
    task_compile "$task_mode" psx_scratch.c "$@"
    task_body ScratchDirect "$task_mode" | grep -Eq 'jal[[:space:]]+ScratchTiny'
    if task_body ScratchOrdinary "$task_mode" | grep -Eq 'jal[[:space:]]+ScratchOrdinaryTiny'; then exit 1; fi
done
task_reject manual-ordinary 'forbids ordinary/indirect/library calls' psx_scratch.c -DTEST_ORDINARY_CALL
task_reject manual-unasserted 'lacks an AT/scratch-window assertion' psx_scratch.c -DTEST_UNASSERTED_CALL
task_reject manual-stack 'manual AT ABI' psx_scratch.c -DTEST_STACK
task_reject manual-extra-save 'manual AT ABI needs an additional' psx_scratch.c -DTEST_EXTRA_SAVE
task_reject manual-debug 'unsupported manual AT ABI mode' psx_scratch.c -g
task_reject inline-initial 'supported non-inline C void prototype' psx_scratch.c -DTEST_INLINE_INITIAL
for task_order in EARLIER LATER; do
    task_reject "inline-$task_order" 'manual AT frame bodies cannot be declared inline' \
        psx_scratch.c "-DTEST_INLINE_$task_order"
done

# Only adjacent mapped-RAM volatile SI loads gain a slot, and only on request.
for task_mode in default off on last-off last-on; do
    set --
    case "$task_mode" in
        off|on) set -- "-mpsx-normal-ram-lw-delay=$task_mode" ;;
        last-off) set -- -mpsx-normal-ram-lw-delay=banana -mpsx-normal-ram-lw-delay=off ;;
        last-on) set -- -mpsx-normal-ram-lw-delay=off -mpsx-normal-ram-lw-delay=on ;;
    esac
    task_compile "lw-$task_mode" psx_normal_ram_lw_delay.c \
        -fno-schedule-insns -fno-schedule-insns2 -mpsx-normal-ram-lhu-delay=off "$@"
done
cmp "$task_out/lw-default.s" "$task_out/lw-off.s"
cmp "$task_out/lw-off.s" "$task_out/lw-last-off.s"
cmp "$task_out/lw-on.s" "$task_out/lw-last-on.s"
for task_pair in global_data:14 local_data:8 not_adjacent:3; do
    task_name=${task_pair%:*}; task_reg=${task_pair#*:}
    task_body "lw_$task_name" lw-on | sed '/^[[:space:]]*#/d' \
        | grep -A1 -E 'beq[[:space:]]+\$4,\$0,' \
        | grep -Eq 'lw[[:space:]]+\$'"$task_reg"',([24][08])\(\$6\)'
done
for task_name in fallthrough_reads_pending fallthrough_writes_pending taken_reads_pending \
    taken_writes_pending branch_reads_pending address_equals_destination opaque_asm_gap \
    empty_asm_gap second_volatile_read halfword_is_not_word unaligned_word; do
    task_body "lw_$task_name" lw-off > "$task_out/negative-off.s"
    task_body "lw_$task_name" lw-on > "$task_out/negative-on.s"
    task_local_labels "$task_out/negative-off.s" > "$task_out/negative-off-local.s"
    task_local_labels "$task_out/negative-on.s" > "$task_out/negative-on-local.s"
    cmp "$task_out/negative-off-local.s" "$task_out/negative-on-local.s"
done
task_reject lw-option 'bad value (banana)' psx_normal_ram_lw_delay.c -mpsx-normal-ram-lw-delay=banana

# Read-only fixed condition13 is legal; fixed destination8 remains excluded.
task_compile lhu-input psx_lhu_fixed_input.c -fno-schedule-insns -fno-schedule-insns2 \
    -ffixed-t5 -mpsx-normal-ram-lhu-delay=on -mpsx-normal-ram-lw-delay=off
task_compile lhu-output psx_lhu_fixed_input.c -fno-schedule-insns -fno-schedule-insns2 \
    -ffixed-t5 -ffixed-t0 -mpsx-normal-ram-lhu-delay=on -mpsx-normal-ram-lw-delay=off
task_body psx_normal_ram_lhu_delay lhu-input | sed '/^[[:space:]]*#/d' \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'
grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)' "$task_out/lhu-output.s"
if task_body psx_normal_ram_lhu_delay lhu-output | sed '/^[[:space:]]*#/d' \
    | grep -A1 -E 'beq[[:space:]]+\$13,\$0,' | grep -Eq 'lhu[[:space:]]+\$8,2\(\$11\)'; then exit 1; fi
