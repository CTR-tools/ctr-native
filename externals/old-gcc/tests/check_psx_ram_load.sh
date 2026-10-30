#!/bin/sh
set -eu

task_gcc=$1
task_prefix=$2
task_tests=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
task_out=$(mktemp -d)
echo "Mapped-RAM/epilogue policy smoke outputs: $task_out"
task_phases='rtl jump addressof cse loop cse2 bp flow combine sched lreg greg sched2 jump2 dbr'

task_compile()
{
    task_lane=$1
    task_source=$2
    shift 2
    mkdir "$task_out/$task_lane"
    (cd "$task_out/$task_lane" && "$task_gcc" -B"$task_prefix/" -S -O2 -G8 \
        -msoft-float -ffunction-sections -da "$@" -o raw.s "$task_tests/$task_source") \
        > "$task_out/$task_lane/stdout" 2> "$task_out/$task_lane/stderr"
}
task_ok()
{
    task_compile "$@"
    test ! -s "$task_out/$task_lane/stdout"
    test ! -s "$task_out/$task_lane/stderr"
}
task_body()
{
    awk -v name="$1" '
        $0 == name ":" { active=1; found=1 }
        active { print }
        active && $1 == ".end" && $2 == name { done=1; exit }
        END { exit !(found && done) }
    ' "$2"
}
task_rtl_body()
{
    awk -v name="$1" '/^;; Function / { active=($3 == name); found+=active }
        active { print } END { exit !found }' "$2"
}
task_modes()
{
    task_stem=$1; task_input=$2; task_option=$3
    shift 3
    for task_mode in default off on last-off last-on; do
        case "$task_mode" in
            default) task_ok "$task_stem-$task_mode" "$task_input" "$@" ;;
            off|on) task_ok "$task_stem-$task_mode" "$task_input" "$@" "-m$task_option=$task_mode" ;;
            last-off) task_ok "$task_stem-$task_mode" "$task_input" "$@" "-m$task_option=banana" "-m$task_option=off" ;;
            last-on) task_ok "$task_stem-$task_mode" "$task_input" "$@" "-m$task_option=off" "-m$task_option=on" ;;
        esac
    done
    cmp "$task_out/$task_stem-default/raw.s" "$task_out/$task_stem-off/raw.s"
    cmp "$task_out/$task_stem-off/raw.s" "$task_out/$task_stem-last-off/raw.s"
    cmp "$task_out/$task_stem-on/raw.s" "$task_out/$task_stem-last-on/raw.s"
    for task_phase in $task_phases; do
        cmp "$task_out/$task_stem-default/$task_input.$task_phase" "$task_out/$task_stem-off/$task_input.$task_phase"
    done
}

# Ordinary SI speculation is separate from both existing volatile policies.
task_modes ram psx_prebranch_lw.c psx-prebranch-lw-delay \
    -fno-schedule-insns -fno-schedule-insns2 -mclone-fixed-epilogues=off \
    -mpsx-normal-ram-lhu-delay=off -mpsx-normal-ram-lw-delay=off
task_modes oldlive psx_prebranch_old_live.c psx-prebranch-lw-delay \
    -fno-schedule-insns -fno-schedule-insns2 -mclone-fixed-epilogues=off \
    -mpsx-normal-ram-lhu-delay=off -mpsx-normal-ram-lw-delay=off
for task_case in ram:psx_prebranch_lw.c oldlive:psx_prebranch_old_live.c; do
    task_stem=${task_case%:*}; task_input=${task_case#*:}
    for task_phase in $task_phases; do
        if test "$task_phase" != dbr; then
            cmp "$task_out/$task_stem-off/$task_input.$task_phase" "$task_out/$task_stem-on/$task_input.$task_phase"
        fi
    done
done

# Textual BNE/LW adjacency alone is not an installed physical slot.
for task_mode in off on; do
    task_rtl_body ram_forward_alias "$task_out/ram-$task_mode/psx_prebranch_lw.c.dbr" \
        > "$task_out/ram-$task_mode/forward.dbr"
    task_want=0; if test "$task_mode" = on; then task_want=1; fi
    awk -v want="$task_want" 'BEGIN { RS="" }
        /\(sequence\[/ && /if_then_else \(ne:SI \(reg[^)]*:SI 9 / &&
        /\(set \(reg[^)]*:SI 9 / && /\(mem[^)]*:SI \(reg[^)]*:SI 4 / &&
        /\{movsi_internal2\}/ { ++slots }
        END { exit (slots != want) }
    ' "$task_out/ram-$task_mode/forward.dbr"
done
task_body ram_forward_alias "$task_out/ram-on/raw.s" | awk '
    $1 == ".set" && $2 == "noreorder" { ordered=1 }
    $1 == ".set" && $2 == "reorder" { ordered=0 }
    $1 == target ":" { taken=1 }
    $1 ~ /^[a-z]/ && $1 !~ /:$/ {
        if (taken) { if ($1 != "lw" || $2 != "$2,4($5)") bad=1; taken=0; targetgap++ }
        if (state == 2) { if ($1 != "lw" || $2 != "$8,4($4)") bad=1; state=0; fallgap++ }
        if (state == 1) { if ($1 != "lw" || $2 != "$9,0($4)") bad=1; state=2; slots++ }
        if ($1 == "bne" && $2 ~ /^\$9,\$0,/) {
            if (!ordered || previous != "srl $9,$6,16") bad=1
            split($2, fields, ","); target=fields[3]; state=1; branches++
        }
        previous=$1 " " $2
    }
    END { exit !(branches == 1 && slots == 1 && fallgap == 1 && targetgap == 1 && !state && !bad) }
'

# The old value is needed beyond an independent taken-edge load: refuse motion.
cmp "$task_out/oldlive-off/raw.s" "$task_out/oldlive-on/raw.s"
cmp "$task_out/oldlive-off/psx_prebranch_old_live.c.dbr" "$task_out/oldlive-on/psx_prebranch_old_live.c.dbr"
awk 'BEGIN { RS="" }
    /\(sequence\[/ && /if_then_else \(ne:SI \(reg[^)]*:SI 9 / &&
    /\(set \(reg[^)]*:SI 9 / && /\(mem[^)]*:SI \(reg[^)]*:SI 4 / { bad=1 }
    END { exit bad }
' "$task_out/oldlive-on/psx_prebranch_old_live.c.dbr"
task_body ram_shared_reject_old_live "$task_out/oldlive-on/raw.s" | awk '
    $1 == "bne" && $2 ~ /^\$9,\$0,/ { split($2, fields, ","); target=fields[3]; branches++ }
    $1 == target ":" { taken=1 }
    taken && $1 ~ /^[a-z]/ && $1 !~ /:$/ {
        if (taken == 1) { if ($1 != "lw" || $2 != "$8,4($5)") bad=1; taken=2 }
        else if ($1 == "xor" && $2 == "$2,$9,$8") { olduse++; taken=0 }
    }
    END { exit !(branches == 1 && olduse == 1 && !bad) }
'

# Clone the generated fixed void epilogue, not a handwritten register template.
task_modes clone psx_clone_fixed.c clone-fixed-epilogues -mpsx-prebranch-lw-delay=off
task_modes operands psx_clone_operands.c clone-fixed-epilogues -mpsx-prebranch-lw-delay=off
for task_pair in clone:two_paths clone:empty_constraints operands:operand_constraints; do
    task_stem=${task_pair%:*}; task_name=${task_pair#*:}
    task_body "$task_name" "$task_out/$task_stem-off/raw.s" > "$task_out/before.s"
    task_body "$task_name" "$task_out/$task_stem-on/raw.s" > "$task_out/after.s"
    awk 'FNR == 1 { ++lane }
        $1 == ".frame" { split($2, f, ","); frame[lane]=f[2]+0; if (f[1] != "$sp") bad=1 }
        $1 == ".mask" || $1 == ".fmask" { metadata[lane]=metadata[lane] $0 "\n" }
        $1 == "sw" || $1 == "lw" {
            split($2, f, ","); reg=f[1]; sub(/^\$/, "", reg); reg+=0
            if ((reg == 31 || (reg >= 16 && reg <= 23)) && f[2] ~ /\(\$sp\)$/) {
                if ($1 == "sw") saves[lane, $2]++; else restores[lane, $2]++
                if (lane == 1 && $1 == "sw") homes[$2]=1
                if (lane == 2 && !($2 in homes)) bad=1
            }
        }
        $1 == "j" && $2 == "$31" { returns[lane]++ }
        $1 == "subu" && $2 ~ /^\$sp,\$sp,/ { split($2, f, ","); if (f[3]+0 != frame[lane]) bad=1; allocations[lane]++ }
        ($1 == "addu" || $1 == "addiu") && $2 ~ /^\$sp,\$sp,/ {
            split($2, f, ","); if (f[3]+0 != frame[lane]) bad=1; teardowns[lane]++
        }
        END {
            if (!frame[1] || frame[1] > 32767 || frame[1] != frame[2] || metadata[1] != metadata[2]) bad=1
            for (home in homes) {
                count++; if (home ~ /^\$31,/) ra=1
                if (saves[1,home] != 1 || saves[2,home] != 1 || restores[1,home] != 1 || restores[2,home] != 2) bad=1
            }
            exit !(count > 1 && ra && allocations[1] == 1 && allocations[2] == 1 && returns[1] == 1 && returns[2] == 2 && teardowns[1] == 1 && teardowns[2] == 2 && !bad)
        }
    ' "$task_out/before.s" "$task_out/after.s"
done
for task_name in no_frame returns_value three_paths dynamic_frame large_frame opaque_effect variable_arguments; do
    task_body "$task_name" "$task_out/clone-off/raw.s" > "$task_out/before.s"
    task_body "$task_name" "$task_out/clone-on/raw.s" > "$task_out/after.s"
    cmp "$task_out/before.s" "$task_out/after.s"
done
task_body no_frame "$task_out/clone-on/raw.s" | grep -Eq '\.frame[[:space:]]+\$sp,0,\$31'
task_body dynamic_frame "$task_out/clone-on/raw.s" | grep -Eq '\.frame[[:space:]]+\$(fp|30),'
task_body large_frame "$task_out/clone-on/raw.s" | awk '$1 == ".frame" { split($2,f,","); found=(f[2]+0 > 32767) } END { exit !found }'
task_body opaque_effect "$task_out/clone-on/raw.s" | grep -Fq '# opaque control'
for task_mode in off on; do
    task_body opaque_operand_constraints "$task_out/operands-$task_mode/raw.s" > "$task_out/opaque-$task_mode.s"
done
cmp "$task_out/opaque-off.s" "$task_out/opaque-on.s"
for task_mode in off on; do
    task_ok "boundary-$task_mode" psx_clone_fixed.c -mframed-return-boundary=on "-mclone-fixed-epilogues=$task_mode"
done
cmp "$task_out/boundary-off/raw.s" "$task_out/boundary-on/raw.s"

for task_option in psx-prebranch-lw-delay clone-fixed-epilogues; do
    if task_compile "bad-$task_option" psx_prebranch_lw.c "-m$task_option=banana"; then exit 1; else task_status=$?; fi
    test "$task_status" -eq 1
    grep -Fq 'bad value (banana)' "$task_out/bad-$task_option/stderr"
    grep -Fq "$task_option" "$task_out/bad-$task_option/stderr"
    if grep -Eiq 'internal compiler error|internal error|segmentation fault|fatal signal' "$task_out/bad-$task_option/stderr"; then exit 1; fi
done
