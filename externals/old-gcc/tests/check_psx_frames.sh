#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Frame policy smoke outputs: $task_out"

task_compile()
{
    task_name=$1
    shift
    "$task_gcc" -B"$task_prefix/" -S -O2 -G8 -msoft-float -ffunction-sections \
        "$@" -o "$task_out/$task_name.s" "$task_tests/psx_frame_policies.c"
}

task_body()
{
    awk -v name="$1" '
        $0 == name ":" { body = 1 }
        body { print }
        body && $1 == ".end" && $2 == name { exit }
    ' "$task_out/$2.s"
}

task_compile default
task_compile off -mstack-align=8 -mprologue-arg-alias=off -mframed-return-boundary=off -mordered-gp-prologue=off
cmp "$task_out/default.s" "$task_out/off.s"
task_compile word -mstack-align=4
task_compile alias -mstack-align=4 -mprologue-arg-alias=on
task_compile return -mstack-align=4 -mprologue-arg-alias=on -mframed-return-boundary=on

# Only the last supplied value is interpreted, even after an invalid earlier one.
task_compile last-off -mstack-align=banana -mstack-align=8 \
    -mprologue-arg-alias=banana -mprologue-arg-alias=off \
    -mframed-return-boundary=banana -mframed-return-boundary=off \
    -mordered-gp-prologue=banana -mordered-gp-prologue=off
cmp "$task_out/default.s" "$task_out/last-off.s"
task_compile last-on -mstack-align=8 -mstack-align=4 \
    -mprologue-arg-alias=off -mprologue-arg-alias=on \
    -mframed-return-boundary=off -mframed-return-boundary=on
cmp "$task_out/return.s" "$task_out/last-on.s"

for task_arg in 0 1 2 3; do
    task_body "frame_a$task_arg" default | grep -Eq '\.frame[[:space:]]+\$sp,24,\$31'
    for task_mode in word alias; do
        task_body "frame_a$task_arg" "$task_mode" | awk -v mode="$task_mode" '
            /subu[[:space:]]+\$sp,\$sp,20/ { allocation = NR }
            /sw[[:space:]]+\$31,16\(\$sp\)/ { save = NR }
            /lw[[:space:]]+\$4,4\(\$[4-7]\)/ { load = NR }
            END { exit !(allocation && save && load && allocation < save &&
                (mode == "alias" ? allocation < load && load < save : save < load)) }
        '
    done
done

task_body frame_indirect alias | awk '
    /lw[[:space:]]+\$2,0\(\$4\)/ { first = NR }
    /sw[[:space:]]+\$31,16\(\$sp\)/ { save = NR }
    /lw[[:space:]]+\$4,4\(\$2\)/ { second = NR }
    END { exit !(first && save && second && first < save && save < second) }
'
for task_name in frame_integer frame_changed frame_volatile frame_basic_asm frame_memory_asm; do
    task_body "$task_name" alias | awk '
        /sw[[:space:]]+\$31,16\(\$sp\)/ { save = NR }
        /lw[[:space:]]+\$4,4\(\$4\)/ { load = NR }
        END { exit !(save && load && save < load) }
    '
done
for task_name in frame_basic_asm frame_memory_asm; do
    task_body "$task_name" alias | awk '
        /sw[[:space:]]+\$31,16\(\$sp\)/ { save = NR }
        $1 == "#APP" { source_asm = NR }
        /lw[[:space:]]+\$4,4\(\$4\)/ { load = NR }
        END { exit !(save && source_asm && load && save < source_asm && source_asm < load) }
    '
done

# Protect real framed returns; leave the frameless delay-slot computation alone.
task_body frame_leaf word > "$task_out/leaf-word.s"
task_body frame_leaf return > "$task_out/leaf-return.s"
cmp "$task_out/leaf-word.s" "$task_out/leaf-return.s"
for task_name in frame_a0 frame_branch; do
    task_body "$task_name" alias | grep -A1 -E 'j[[:space:]]+\$31' \
        | grep -Eq 'addu[[:space:]]+\$sp,\$sp,'
    task_body "$task_name" return | awk '
        /addu[[:space:]]+\$sp,\$sp,/ { restore = NR }
        /j[[:space:]]+\$31/ { returns++; if (!restore || restore != NR - 1) bad = 1 }
        END { exit !(returns && !bad) }
    '
done

# Check actual selected-register homes, including sparse masks, before any body work.
task_compile ordered -mordered-gp-prologue=on
task_compile ordered-last -mordered-gp-prologue=banana -mordered-gp-prologue=on
cmp "$task_out/ordered.s" "$task_out/ordered-last.s"
task_compile sparse-off -ffixed-17 -ffixed-19 -mordered-gp-prologue=off
task_compile sparse-on -ffixed-17 -ffixed-19 -mordered-gp-prologue=on
task_compile debug-off -g -mordered-gp-prologue=off
task_compile debug-on -g -mordered-gp-prologue=on
grep -Eq '^[[:space:]]*\.loc[[:space:]]' "$task_out/debug-on.s"

for task_mode in ordered sparse-on debug-on; do
    task_regs='16 17 18 31'
    task_mask='0x80070000,-4'
    task_before=default
    if test "$task_mode" = sparse-on; then
        task_regs='16 18 20 31'; task_mask='0x80150000,-4'; task_before=sparse-off
    elif test "$task_mode" = debug-on; then
        task_before=debug-off
    fi
    task_body frame_branch "$task_mode" | awk -v regs="$task_regs" -v mask="$task_mask" '
        BEGIN { split(regs, r, " "); for (i=1; i<=4; ++i) home[i]="$" r[i] "," (12+4*i) "($sp)" }
        $1 == ".frame" { frame = ($2 == "$sp,32,$31") }
        $1 == ".mask" { metadata = ($2 == mask) }
        $1 ~ /^[a-z]/ && $1 !~ /:$/ {
            ++instructions
            if (instructions == 1 && ($1 != "subu" || $2 != "$sp,$sp,32")) bad=1
            if (instructions >= 2 && instructions <= 5 && ($1 != "sw" || $2 != home[instructions-1])) bad=1
            for (i=1; i<=4; ++i) {
                if ($1 == "sw" && $2 == home[i]) ++save[i]
                if ($1 == "lw" && $2 == home[i]) ++restore[i]
            }
            if ($1 == "move" && $2 ~ /,\$[4-7]$/) ++copies
        }
        END { for (i=1; i<=4; ++i) if (save[i] != 1 || restore[i] != 1) bad=1
              exit !(frame && metadata && copies >= 3 && instructions > 5 && !bad) }
    '
    for task_lane in "$task_before" "$task_mode"; do
        task_body frame_branch "$task_lane" | awk '
            $1 == ".frame" || $1 == ".mask" || $1 == ".fmask" ||
                ($1 == "lw" && $2 ~ /\(\$sp\)/) { print $1, $2 }
        ' > "$task_out/$task_lane-layout"
    done
    cmp "$task_out/$task_before-layout" "$task_out/$task_mode-layout"
done
task_body frame_leaf default > "$task_out/leaf-default.s"
task_body frame_leaf ordered > "$task_out/leaf-ordered.s"
cmp "$task_out/leaf-default.s" "$task_out/leaf-ordered.s"

# Exclusions must contain the actual construct, not merely a target flag.
for task_context in excluded dynamic fpu pic profile; do
    case "$task_context" in
        excluded) set -- -DTEST_ORDER_EXCLUDED=1 ;;
        dynamic) set -- -DTEST_DYNAMIC_FRAME=1 ;;
        fpu) set -- -DTEST_ORDER_FPU=1 -mhard-float ;;
        pic) set -- -DTEST_ORDER_PIC=1 -G0 -fPIC -mabicalls ;;
        profile) set -- -p ;;
    esac
    task_compile "$task_context-off" "$@" -mordered-gp-prologue=off
    task_compile "$task_context-on" "$@" -mordered-gp-prologue=on
    cmp "$task_out/$task_context-off.s" "$task_out/$task_context-on.s"
done
task_body frame_order_large excluded-on | grep -Eq '\.frame[[:space:]]+\$sp,40024,\$31'
task_body frame_order_aggregate excluded-on | grep -Eq 'sw[[:space:]]+\$[4-7],.*\(\$sp\)'
for task_arg in 5 6 7; do
    task_body frame_order_variadic excluded-on | grep -Eq 'sw[[:space:]]+\$'"$task_arg"',.*\(\$sp\)'
done
task_body frame_dynamic dynamic-on | grep -Eq '\.frame[[:space:]]+\$(fp|30),'
task_body frame_order_fpu fpu-on | grep -Eq '\.fmask[[:space:]]+0x00300000,-8'
task_body frame_order_fpu fpu-on | grep -Eq 's\.d[[:space:]]+\$f20,'
task_body frame_order_fpu fpu-on | grep -Eq 'l\.d[[:space:]]+\$f20,'
test "$(task_body frame_order_fpu fpu-on | grep -c 'jal[[:space:]]*frame_fpu_call')" -eq 2
task_body frame_order_pic pic-on | grep -Eq '\.cpload[[:space:]]+\$25'
task_body frame_order_pic pic-on | grep -Eq '\.cprestore[[:space:]]'
grep -Eq 'jal[[:space:]]+_mcount' "$task_out/profile-on.s"

for task_option in stack-align prologue-arg-alias framed-return-boundary ordered-gp-prologue; do
    if task_compile bad "-m$task_option=banana" > "$task_out/bad.stdout" 2> "$task_out/bad.stderr"; then
        exit 1
    fi
    grep -Fq 'bad value (banana)' "$task_out/bad.stderr"
    grep -Fq "$task_option" "$task_out/bad.stderr"
done
for task_case in ALIGNED_SLOT DYNAMIC_FRAME; do
    if task_compile rejected -mstack-align=4 "-DTEST_$task_case=1" \
        > "$task_out/rejected.stdout" 2> "$task_out/rejected.stderr"; then
        exit 1
    fi
    grep -Fq -- '-mstack-align=4' "$task_out/rejected.stderr"
done
if task_compile unsupported -mstack-align=4 -mcpu=r4000 -mips2 \
    > "$task_out/unsupported.stdout" 2> "$task_out/unsupported.stderr"; then
    exit 1
fi
grep -Fq -- '-mstack-align=4 requires little-endian MIPS-I/ABI32 soft-float without PIC' \
    "$task_out/unsupported.stderr"
